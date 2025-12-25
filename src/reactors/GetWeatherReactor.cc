#include "GetWeatherReactor.h"

#include "../search/SearchEngineItf.h"
#include "../utils/TimeUtils.h"
#include "../utils/grpcUtils.h"
#include "RequestValidators.h"

#include <absl/log/log.h>

#include <algorithm>
#include <format>
#include <limits>

namespace geo
{

GetWeatherReactor::GetWeatherReactor(grpc::CallbackServerContext* context, const geoproto::WeatherRequest& request,
   geoproto::WeatherResponse& response, ISearchEngine& searchEngine)
{
   if (auto errorString = ValidateWeatherRequest(request))
   {
      LOG(ERROR) << std::format("Bad request, client-id={}", geo::ExtractClientId(*context));
      Finish(grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, errorString});
      return;
   }

   // Convert protobuf timestamps to DateRange
   const TimePoint fromTime = TimestampToTimePoint(request.from_date());
   const TimePoint toTime = TimestampToTimePoint(request.to_date());
   const DateRange dateRange{TimePointToDate(fromTime), TimePointToDate(toTime)};

   // Process each location
   for (const auto& location : request.locations())
   {
      // Get weather data for this location
      WeatherInfoVector weatherData =
         searchEngine.GetWeather(location.latitude(), location.longitude(), dateRange, request.num_years());

      // Aggregate weather data into a single Weather message
      geoproto::Weather* weather = response.add_historical_weather();

      if (weatherData.empty())
      {
         // No data available, set default values
         weather->set_max_temperature(0.0);
         weather->set_min_temperature(0.0);
         weather->set_average_temperature(0.0);
      }
      else
      {
         double minTemp = std::numeric_limits<double>::max();
         double maxTemp = std::numeric_limits<double>::lowest();
         double sumTemp = 0.0;
         std::size_t count = 0;

         for (const auto& info : weatherData)
         {
            minTemp = std::min(minTemp, info.temperatureMin);
            maxTemp = std::max(maxTemp, info.temperatureMax);
            sumTemp += info.temperatureAverage;
            ++count;
         }

         weather->set_max_temperature(maxTemp);
         weather->set_min_temperature(minTemp);
         weather->set_average_temperature(sumTemp / static_cast<double>(count));
      }
   }

   // Finish the RPC with a success status.
   Finish(grpc::Status::OK);
}

}  // namespace geo
