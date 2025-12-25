#pragma once

namespace geoproto
{
class CitiesRequest;
class RegionsRequest;
class WeatherRequest;
}  // namespace geoproto

namespace geo
{

// Helper function to validate the CitiesRequest. Ensures that either a position or city name is provided.
// Checks if the position coordinates are valid (latitude and longitude within acceptable ranges).
// Returns an error string or nullptr if a request is valid.
const char* ValidateCitiesRequest(const geoproto::CitiesRequest& request);

// Helper function to validate the RegionsRequest. Ensures that position and preferences are provided.
// Validates that the coordinates (latitude and longitude) are within acceptable ranges.
// Returns an error string or nullptr if a request is valid.
const char* ValidateRegionsRequest(const geoproto::RegionsRequest& request);

// Helper function to validate the WeatherRequest. Ensures that locations and date range are provided.
// Validates that the coordinates (latitude and longitude) are within acceptable ranges.
// Returns an error string or nullptr if a request is valid.
const char* ValidateWeatherRequest(const geoproto::WeatherRequest& request);

}  // namespace geo
