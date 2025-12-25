#pragma once

#include "../../proto/ProtoTypes.h"
#include "NominatimApiUtils.h"
#include "OverpassApiUtils.h"
#include "SearchEngineItf.h"

#include <set>
#include <string>

namespace geo
{

class WebClient;

class SearchEngine : public ISearchEngine
{
public:
   // Constructs a SearchEngine with references to Overpass, Nominatim and OpenMeteo API clients
   SearchEngine(WebClient& overpassApiClient, WebClient& nominatimApiClient, WebClient& openMeteoApiClient);

   // See ISearchEngine::FindCitiesByName for documentation
   GeoProtoPlaces FindCitiesByName(const std::string& name, bool includeDetails) override;

   // See ISearchEngine::FindCitiesByPosition for documentation
   GeoProtoPlaces FindCitiesByPosition(double latitude, double longitude, bool includeDetails) override;

   // See ISearchEngine::StartFindRegions for documentation
   IncrementalSearchHandler StartFindRegions() override;

   // See ISearchEngine::GetWeather for documentation
   WeatherInfoVector GetWeather(
      double latitude, double longitude, const DateRange& dateRange, std::uint32_t numYears = 3) override;

private:
   // Finds region information within a bounding box based on preferences
   nominatim::RelationInfos findRegions(
      const BoundingBox& bbox, const RegionPreferences& prefs, std::set<overpass::OsmId>& processed);

private:
   WebClient& m_overpassApiClient;   // Client for Overpass API requests
   WebClient& m_nominatimApiClient;  // Client for Nominatim API requests
   WebClient& m_openMeteoApiClient;  // Client for Open Meteo API requests
};

}  // namespace geo