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
   "("
   "rel[\"name\"=\"{0}\"][\"boundary\"=\"administrative\"];"
   "rel[\"name:en\"=\"{0}\"][\"boundary\"=\"administrative\"];"
   "rel[\"name\"=\"{0}\"][\"place\"~\"^(city|town|state)$\"];"
   "rel[\"name:en\"=\"{0}\"][\"place\"~\"^(city|town|state)$\"];"
   ");"
   "out ids;";

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

}  // namespace geo::overpass