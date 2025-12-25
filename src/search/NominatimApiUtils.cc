#define RAPIDJSON_HAS_STDSTRING 1

#include "NominatimApiUtils.h"

#include "../utils/JsonUtils.h"
#include "../utils/WebClient.h"

#include <absl/log/log.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <array>
#include <charconv>
#include <cmath>
#include <format>
#include <string>
#include <unordered_map>

namespace
{

using namespace geo;
using namespace geo::nominatim;

const auto sc_chunkSize = 50u;  // Maximum number of OSM IDs to process in a single API request.
                                // from https://nominatim.org/release-docs/latest/api/Lookup/#endpoint

// Formats a request string for the Nominatim API lookup endpoint.
// @param itBegin: Iterator to the start of the OSM IDs list.
// @param itEnd: Iterator to the end of the OSM IDs list.
// @param language: Optional language parameter for the request.
// @return: Formatted request string.
std::string formatRelationLookupRequest(
   const OsmIds::const_iterator& itBegin, const OsmIds::const_iterator& itEnd, const char* language = nullptr)
{
   std::string request = "format=json&osm_ids=";
   for (auto itID = itBegin; itID != itEnd; ++itID)
   {
      if (itID != itBegin)
         request += ",";
      request += "R" + std::to_string(*itID);
   }
   return request + (language ? std::format("&accept-language={}", language) : "");
}

// Converts a string view to a double value.
// @param s: String view containing the numeric value.
// @return: Parsed double value or NAN if parsing fails.
double getDoubleFromString(const std::string_view& s)
{
   double value = 0;
   const auto result = std::from_chars(s.data(), s.data() + s.size(), value);
   return result.ec == std::errc{} ? value : NAN;
}

// Converts a JSON value to a RelationInfo object.
// @param value: JSON value representing a relation.
// @param addressType: The type of address (e.g., "city", "town", "state").
// @return: A RelationInfo object populated with data from the JSON value.
template <typename TObject, typename TJsonValue>
TObject jsonToObject(TJsonValue& value, const std::string& addressType)
{
   TObject result;
   result.osmId = json::GetInt64(json::Get(value, "osm_id"));
   result.name = json::GetString(json::Get(value, "address", addressType.c_str()));
   result.country = json::GetString(json::Get(value, "address", "country"));
   result.latitude = getDoubleFromString(json::GetString(json::Get(value, "lat")));
   result.longitude = getDoubleFromString(json::GetString(json::Get(value, "lon")));
   return result;
}

// Splits the list of OSM IDs into chunks and processes each chunk using the provided handler.
// @param relationIds: List of OSM IDs to process.
// @param fn: Handler function to process each chunk.
template <typename THandler>
void forEachChunk(const OsmIds& relationIds, THandler fn)
{
   for (auto itBegin = relationIds.begin(); itBegin < relationIds.end(); itBegin = std::next(itBegin, sc_chunkSize))
   {
      const auto itEnd = std::min(std::next(itBegin, sc_chunkSize), relationIds.end());
      fn(itBegin, itEnd);
   }
}

// Splits the list of OSM IDs into chunks, sends requests to the Nominatim API, and processes the responses.
// @param relationIds: List of OSM IDs to process.
// @param client: WebClient instance to interact with the Nominatim API.
// @param responseHandler: Handler function to process each API response.
template <typename THandler>
void splitInChunksAndParseResponses(
   const OsmIds& relationIds, WebClient& client, THandler responseHandler, const char* language = nullptr)
{
   forEachChunk(relationIds,
      [&client, responseHandler, language](const auto& itBegin, const auto& itEnd)
      {
         const std::string request = formatRelationLookupRequest(itBegin, itEnd, language);
         const std::string response = client.Get(request);
         if (response.empty())
            return;

         rapidjson::Document document;
         document.Parse(response.c_str());

         responseHandler(document);
      });
}

}  // namespace

namespace geo::nominatim
{

RelationInfos LookupRelationInformation(const OsmIds& relationIds, WebClient& nominatimApiClient)
{
   RelationInfos regions;
   std::unordered_map<OsmId, std::size_t> pendingIdsForEnglishNames;

   auto handleResponse = [&regions, &pendingIdsForEnglishNames](const rapidjson::Document& document, bool isEnglish)
   {
      for (const auto& item : document.GetArray())
      {
         const auto addressType = json::GetString(json::Get(item, "addresstype"));
         const auto osmId = json::GetInt64(json::Get(item, "osm_id"));
         if (osmId == 0 || addressType.empty())
            continue;

         if (!pendingIdsForEnglishNames.contains(osmId))
         {
            if (isEnglish)
               continue;  // Fill English fields only for already known entries.

            regions.emplace_back(jsonToObject<RelationInfo>(item, std::string(addressType)));
            pendingIdsForEnglishNames.emplace(osmId, regions.size() - 1);
         }
         else if (isEnglish)
         {
            auto& info = regions.at(pendingIdsForEnglishNames.at(osmId));
            info.name_en = json::GetString(json::Get(item, "address", addressType.data()));
            info.country_en = json::GetString(json::Get(item, "address", "country"));
         }
      }
   };

   splitInChunksAndParseResponses(relationIds, nominatimApiClient,
      [&handleResponse](const rapidjson::Document& document)
      {
         handleResponse(document, false);
      });

   splitInChunksAndParseResponses(
      relationIds, nominatimApiClient,
      [&handleResponse](const rapidjson::Document& document)
      {
         handleResponse(document, true);
      },
      "en");

   return regions;
}

RelationInfos LookupRelationInformationForCities(const OsmIds& relationIds, Match match, WebClient& nominatimApiClient)
{
   RelationInfos cities;
   std::unordered_map<OsmId, std::size_t> pendingIdsForEnglishNames;

   splitInChunksAndParseResponses(relationIds, nominatimApiClient,
      [&cities, &pendingIdsForEnglishNames, match](const rapidjson::Document& document)
      {
         auto areCloseCoordinates = [](const RelationInfo& c1, const RelationInfo& c2)
         {
            return std::abs(c1.latitude - c2.latitude) < 1 && std::abs(c1.longitude - c2.longitude) < 1;
         };

         // Order is important when CitySearch.Match.Best is used.
         // For example, latitude=41.1172364, longitude=1.2546057 is Tarragona "city",
         // but it is also Catalonia "state". And "city" is the best match here.
         // However, latitude=11.5730391, longitude=104.857807 is Phnom Penh "state",
         // and there is no "city" at this point at all.
         //
         // When CitySearch.Match.Any is used we need to collect all matching things.
         // But it is worth to apply some heuristic too - if "city" is already found then
         // "state" with same (or close) coordinates is not needed.
         //
         // The list is probably incomplete as there is no any documentation on this API tricks.
         constexpr std::array<const char*, 3> sc_types = {"city", "town", "state"};

         for (auto type : sc_types)
         {
            for (const auto& item : document.GetArray())
            {
               if (json::GetString(json::Get(item, "addresstype")) == type)
               {
                  auto newObject = jsonToObject<RelationInfo>(item, type);
                  bool needAdd = true;
                  if (match == Match::Any)
                  {
                     for (auto& c : cities)
                     {
                        if (areCloseCoordinates(c, newObject))
                        {
                           needAdd = false;
                           break;
                        }
                     }
                  }

                  if (needAdd)
                  {
                     cities.emplace_back(std::move(newObject));
                     pendingIdsForEnglishNames.emplace(cities.back().osmId, cities.size() - 1);
#ifndef NDEBUG
                     LOG(INFO) << std::format("addresstype {}, osm_id {}, lat {}, lon {}", type,
                        json::GetInt64(json::Get(item, "osm_id")), json::GetString(json::Get(item, "lat")),
                        json::GetString(json::Get(item, "lon")));
#endif
                  }
               }
               if (match == Match::Best && !cities.empty())
                  break;
            }
            if (match == Match::Best && !cities.empty())
               break;
         }
      });

   splitInChunksAndParseResponses(
      relationIds, nominatimApiClient,
      [&pendingIdsForEnglishNames, &cities](const rapidjson::Document& document)
      {
         for (const auto& item : document.GetArray())
         {
            const auto addresstype = json::GetString(json::Get(item, "addresstype"));
            const auto osmId = json::GetInt64(json::Get(item, "osm_id"));
            if (osmId == 0)
               continue;

            if (!pendingIdsForEnglishNames.contains(osmId))
               continue;

            auto& info = cities.at(pendingIdsForEnglishNames.at(osmId));
            info.name_en = json::GetString(json::Get(item, "address", addresstype.data()));
            info.country_en = json::GetString(json::Get(item, "address", "country"));
         }
      },
      "en");

   return cities;
}

}  // namespace geo::nominatim