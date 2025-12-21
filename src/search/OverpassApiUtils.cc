#include "OverpassApiUtils.h"

#include "../utils/JsonUtils.h"
#include "../utils/WebClient.h"
#include "ProtoTypes.h"

#include <rapidjson/document.h>

#include <format>

namespace
{

using namespace geo;

// Overpass API query format to find relations by name or English name.
constexpr const char* sz_requestByNameFormat =  //
   "[out:json];"
   "rel[\"name\"=\"{0}\"][\"boundary\"=\"administrative\"];"
   "out ids;";  // Return ids.

// Overpass API query format to find relations by coordinates.
constexpr const char* sz_requestByCoordinatesFormat =
   "[out:json];"
   "is_in({},{}) -> .areas;"  // Save "area" entities which contain a point with the given coordinates to .areas set.
   "("
   "rel(pivot.areas)[\"boundary\"=\"administrative\"];"
   "rel(pivot.areas)[\"place\"~\"^(city|town|state)$\"];"
   ");"         // Save "relation" entities with administrative boundary type or with city|town|state place
                // which define the outlines of the found "area" entities to the result set.
   "out ids;";  // Return ids.

}  // namespace

namespace geo::overpass
{

GeoProtoTaggedFeatures ExtractCityDetails(const std::string& json)
{
   if (json.empty())
      return {};

   rapidjson::Document document;
   document.Parse(json.c_str());
   if (!document.IsObject() || !document.HasMember("elements"))
      return {};

   GeoProtoTaggedFeatures features;
   for (const auto& element : document["elements"].GetArray())
   {

      if (json::GetString(json::Get(element, "type")) != "node")
         continue;

      // Check if it has tourism tag with value hotel or museum
      const auto& tags = json::Get(element, "tags");
      if (!tags.IsObject() || !tags.HasMember("tourism"))
         continue;

      const std::string_view tourismValue = json::GetString(json::Get(tags, "tourism"));
      if (tourismValue != "hotel" && tourismValue != "museum")
         continue;

      // Create TaggedFeature
      GeoProtoTaggedFeature feature;

      // Set position
      const double lat = json::GetDouble(json::Get(element, "lat"));
      const double lon = json::GetDouble(json::Get(element, "lon"));
      feature.mutable_position()->set_latitude(lat);
      feature.mutable_position()->set_longitude(lon);

      // Set tourism tag
      (*feature.mutable_tags())["tourism"] = std::string(tourismValue);

      // Set name tag if available
      if (tags.HasMember("name"))
      {
         const std::string_view name = json::GetString(json::Get(tags, "name"));
         if (!name.empty())
            (*feature.mutable_tags())["name"] = std::string(name);
      }

      // Set name:en tag if available
      if (tags.HasMember("name:en"))
      {
         const std::string_view nameEn = json::GetString(json::Get(tags, "name:en"));
         if (!nameEn.empty())
            (*feature.mutable_tags())["name:en"] = std::string(nameEn);
      }

      features.emplace_back(std::move(feature));
   }

   return features;
}

OsmIds ExtractRelationIds(const std::string& json)
{
   if (json.empty())
      return {};

   rapidjson::Document document;
   document.Parse(json.c_str());
   if (!document.IsObject())
      return {};

   OsmIds result;
   for (const auto& e : document["elements"].GetArray())
   {
      const auto& id = json::Get(e, "id");
      if (!id.IsNull() && json::GetString(json::Get(e, "type")) == "relation")
         result.emplace_back(json::GetInt64(id));
   }
   return result;
}

OsmIds LoadRelationIdsByName(WebClient& client, const std::string& name)
{
   const std::string request = std::format(sz_requestByNameFormat, name);
   const std::string response = client.Post(request);
   return ExtractRelationIds(response);
}

OsmIds LoadRelationIdsByLocation(WebClient& client, double latitude, double longitude)
{
   const std::string request = std::format(sz_requestByCoordinatesFormat, latitude, longitude);
   const std::string response = client.Post(request);
   return ExtractRelationIds(response);
}

GeoProtoTaggedFeatures LoadCityDetailsByRelationId(WebClient& client, OsmId relationId)
{
   const std::string request = std::format(
      R"(
      [out:json];
      rel(id: {})[boundary=administrative];
      map_to_area->.cityArea;
      (
      node[tourism=hotel](area.cityArea);
      node[tourism=museum](area.cityArea);
      );
      out center;)",
      relationId);

   const std::string response = client.Post(request);
   return ExtractCityDetails(response);
}

}  // namespace geo::overpass
