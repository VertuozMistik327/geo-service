#pragma once

#include "../../proto/ProtoTypes.h"
#include "../utils/GeoUtils.h"
#include "../utils/TimeUtils.h"
#include "../utils/WeatherInfo.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

namespace geo
{

class ISearchEngine
{
public:
   virtual ~ISearchEngine() = default;

   // Searches for cities matching the specified name
   // @param name The city name to search for
   // @param includeDetails If true, includes additional details like features in the response
   // @return GeoProtoPlaces containing matching cities
   virtual GeoProtoPlaces FindCitiesByName(const std::string& name, bool includeDetails) = 0;

   // Searches for cities at or near the specified geographic coordinates
   // @param latitude The latitude coordinate (-90 to 90)
   // @param longitude The longitude coordinate (-180 to 180)
   // @param includeDetails If true, includes additional details like features in the response
   // @return GeoProtoPlaces containing cities found at or near the coordinates
   virtual GeoProtoPlaces FindCitiesByPosition(double latitude, double longitude, bool includeDetails) = 0;

   struct RegionPreferences
   {
      // Bitmask of geoproto.RegionsResponse.Properties values specifying desired region features
      std::uint32_t objects;

      using Properties = std::unordered_map<std::string, std::string>;
      Properties properties;  // Additional key-value pairs for filtering region features (e.g., "minPeakHeight")
   };

   // Initiates an incremental search for regions within bounding boxes
   // @return A function handler that can be called repeatedly with different bounding boxes and preferences
   //         to find regions incrementally, optimizing for looped searches
   using IncrementalSearchHandler = std::function<GeoProtoPlaces(const BoundingBox&, const RegionPreferences&)>;
   virtual IncrementalSearchHandler StartFindRegions() = 0;

   // Returns aggregated weather for given location using historical data.
   // @param latitude: Latitude of the location.
   // @param longitude: Longitude of the location.
   // @param dateRange: Date range for which to request weather.
   // @param numYears: Number of historical years to aggregate (default: 3).
   // @return: Aggregated weather data from all requested years.
   virtual WeatherInfoVector GetWeather(
      double latitude, double longitude, const DateRange& dateRange, std::uint32_t numYears = 3) = 0;
};

}  // namespace geo
