#pragma once

#include "geo.grpc.pb.h"

#include <absl/log/log.h>
#include <grpc/grpc.h>
#include <grpcpp/support/server_callback.h>

#include <format>

namespace geo
{

class ISearchEngine;

// Reactor class for handling unary (non-streaming) responses for the GetWeather RPC.
// This class is responsible for processing a weather request and returning aggregated
// historical weather data for the specified locations and date range.
class GetWeatherReactor : public grpc::ServerUnaryReactor
{
public:
   // Constructor for the GetWeatherReactor.
   // @param context: Server context.
   // @param request: The incoming WeatherRequest from the client.
   // @param response: The WeatherResponse to be populated and sent back to the client.
   // @param searchEngine: Reference to the search engine used to get weather data.
   GetWeatherReactor(grpc::CallbackServerContext* context, const geoproto::WeatherRequest& request,
      geoproto::WeatherResponse& response, ISearchEngine& searchEngine);

private:
   // Called when the RPC is completed. Logs the completion and cleans up the reactor.
   void OnDone() override
   {
      LOG(INFO) << std::format("GetWeather() RPC completed");
      delete this;
   }

   // Called when the RPC is cancelled by the client. Logs the cancellation.
   void OnCancel() override { LOG(ERROR) << std::format("GetWeather() RPC cancelled"); }
};

}  // namespace geo
