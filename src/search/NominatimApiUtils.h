#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace geo
{
class WebClient;
}  // namespace geo

namespace geo::nominatim
{

using OsmId = std::int64_t;         // Type alias for OpenStreetMap (OSM) IDs.
using OsmIds = std::vector<OsmId>;  // Type alias for a list of OSM IDs.

// Enum to define the matching strategy for city lookups.
enum Match
{
   Best,  // Select objects with the most relevant "addresstype" only (e.g., "city" over "state").
   Any    // Select objects with any relevant "addresstype" (e.g., "city", "town", or "state").
};

// Structure to hold information about a geographic relation (e.g., city, town, state).
struct RelationInfo
{
   std::int64_t osmId = 0;  // OSM ID of the relation.
   std::string name;        // Name of the relation in the native language.
   std::string name_en;     // Name of the relation in English.
   std::string country;     // Country name in the native language.
   std::string country_en;  // Country name in English.
   double latitude = 0;     // Latitude of the relation's center.
   double longitude = 0;    // Longitude of the relation's center.
};

using RelationInfos = std::vector<RelationInfo>;  // Type alias for a list of RelationInfo objects.

// Requests the Nominatim Address Lookup API for objects with the given OSM IDs.
// See https://nominatim.org/release-docs/latest/api/Lookup/
// @param relationIds: List of OSM IDs to look up.
// @param nominatimApiClient: WebClient instance to interact with the Nominatim API.
// @return: A list of RelationInfo objects containing details about the requested relations.
RelationInfos LookupRelationInformation(const OsmIds& relationIds, WebClient& nominatimApiClient);

// Requests the Nominatim Address Lookup API for objects with the given OSM IDs,
// filtering results to include only those with "addresstype" relevant for cities.
// @param relationIds: List of OSM IDs to look up.
// @param match: Matching strategy (Best or Any).
// @param nominatimApiClient: WebClient instance to interact with the Nominatim API.
// @return: A list of RelationInfo objects containing details about the requested cities.
RelationInfos LookupRelationInformationForCities(const OsmIds& relationIds, Match match, WebClient& nominatimApiClient);

}  // namespace geo::nominatim

// Examples:
// https://nominatim.openstreetmap.org/lookup?osm_ids=R146656&format=json
// Returns this:
// [
//   {
//     "place_id": 256068956,
//     "licence": "Data (c) OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright",
//     "osm_type": "relation",
//     "osm_id": 146656,
//     "lat": "53.4794892",
//     "lon": "-2.2451148",
//     "class": "boundary",
//     "type": "administrative",
//     "place_rank": 16,
//     "importance": 0.7302810816515201,
//     "addresstype": "city",
//     "name": "Manchester",
//     "display_name": "Manchester, Greater Manchester, England, United Kingdom",
//     "address": {
//       "city": "Manchester",
//       "ISO3166-2-lvl8": "GB-MAN",
//       "state_district": "Greater Manchester",
//       "state": "England",
//       "ISO3166-2-lvl4": "GB-ENG",
//       "country": "United Kingdom",
//       "country_code": "gb"
//     },
//     "boundingbox": [
//       "53.3401044",
//       "53.5445923",
//       "-2.3199185",
//       "-2.1468288"
//     ]
//   }
// ]
// We expect that "addresstype" value (city in this case) is always found under "address".
// RelationInfo.name is address.city in native language
