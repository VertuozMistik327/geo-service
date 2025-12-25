#pragma once

#include "geo.grpc.pb.h"
#include "geo.pb.h"
#include "search/SearchEngineItf.h"
#include "utils/WebClient.h"

#include <memory>

namespace grpc
{
class CallbackServerContext;
class ServerUnaryReactor;
}  // namespace grpc

namespace geo
{

// Forward declaration of Configuration class. Configuration holds system-wide settings.
class Configuration;

// GeoServiceImpl implements the gRPC service defined in Geo.proto to handle geo-related queries.
// It extends the CallbackService class to implement methods for city and region retrieval.
class GeoServiceImpl final : public geoproto::Geo::CallbackService
{
public:
   // Constructor for GeoServiceImpl. Initializes with configuration settings.
   // The constructor sets up API clients and the search engine.
   // @param configuration: Reference to the configuration object for settings.
   explicit GeoServiceImpl(const Configuration& configuration);

   // gRPC method to retrieve a list of cities based on either geographic position or city name.
   // The method is called when a client sends a CitiesRequest.
   // If the request is valid, a new GetCitiesReactor is created to handle the query.
   grpc::ServerUnaryReactor* GetCities(grpc::CallbackServerContext* context, const geoproto::CitiesRequest* request,
      geoproto::CitiesResponse* response) override;

   // gRPC method to retrieve a list of regions in a single response (non-streaming version).
   // This method handles region queries based on geographic position and user preferences,
   // returning all results in one response rather than streaming them.
   grpc::ServerUnaryReactor* GetRegions(grpc::CallbackServerContext* context, const geoproto::RegionsRequest* request,
      geoproto::RegionsResponse* response) override;

   // gRPC method to stream regions based on geographic position and user preferences.
   // This method streams responses, enabling clients to receive multiple region data.
   // If the request is valid, a new GetRegionsStreamReactor is created to stream region data.
   grpc::ServerWriteReactor<geoproto::RegionsResponse>* GetRegionsStream(
      grpc::CallbackServerContext* context, const geoproto::RegionsRequest* request) override;

   // gRPC method to retrieve weather information.
   // The method is called when a client sends a WeatherRequest.
   // If the request is valid, a new GetWeatherReactor is created to handle the query.
   grpc::ServerUnaryReactor* GetWeather(grpc::CallbackServerContext* context, const geoproto::WeatherRequest* request,
      ::geoproto::WeatherResponse* response) override;

private:
   // WebClient instances to interact with the Overpass API, Nominatim API and Open Meteo API.
   WebClient m_overpassApiClient;
   WebClient m_nominatimApiClient;
   WebClient m_openMeteoApiClient;

   // A search engine for handling location-based queries, uses Overpass and Nominatim APIs.
   std::unique_ptr<ISearchEngine> m_searchEngine;
};

}  // namespace geo
