#include "GetRegionsStreamReactor.h"

#include "../utils/GeoUtils.h"
#include "../utils/grpcUtils.h"
#include "RequestValidators.h"

#include <format>

namespace geo
{

GetRegionsStreamReactor::GetRegionsStreamReactor(grpc::CallbackServerContext* context,
   const geoproto::RegionsRequest& request, ISearchEngine& searchEngine, std::uint32_t maxBoxWidth,
   std::uint32_t maxBoxHeight)
{
   if (auto errorString = ValidateRegionsRequest(request))
   {
      LOG(ERROR) << std::format("Bad request, client-id={}", geo::ExtractClientId(*context));
      Finish(grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, errorString});
      return;
   }

   // Convert protocol buffer properties to search engine preferences
   ISearchEngine::RegionPreferences::Properties props = {
      request.prefs().properties().begin(), request.prefs().properties().end()};
   m_prefs = ISearchEngine::RegionPreferences{request.prefs().mask(), std::move(props)};

   // Create bounding boxes around requested position (converting km to meters)
   m_boundingBoxes = CreateBoundingBoxes(request.position().latitude(), request.position().longitude(),
      request.distance_km() * 1000, maxBoxWidth, maxBoxHeight);

   // Initialize the incremental search handler
   m_searchHandler = searchEngine.StartFindRegions();

   // Start processing the first bounding box
   processNextBox();
}

void GetRegionsStreamReactor::OnWriteDone(bool ok)
{
   if (!ok)
   {
      LOG(ERROR) << "GetRegionsStream() write failed";
      Finish(grpc::Status{grpc::StatusCode::UNKNOWN, "Write failed"});
      return;
   }

   // Process the next bounding box
   processNextBox();
}

void GetRegionsStreamReactor::processNextBox()
{
   // Search for regions in each bounding box until we find some or run out of boxes
   while (m_currentBoxIndex < m_boundingBoxes.size())
   {
      const auto& bbox = m_boundingBoxes[m_currentBoxIndex];
      ++m_currentBoxIndex;

      // Find regions in the current bounding box
      auto regions = m_searchHandler(bbox, m_prefs);

      // If regions were found, send them to the client
      if (!regions.empty())
      {
         m_response.Clear();
         for (auto& region : regions)
         {
            *m_response.add_regions() = std::move(region);
         }

         StartWrite(&m_response);
         return;  // Wait for OnWriteDone to be called
      }
   }

   // All bounding boxes have been processed, finish the RPC
   Finish(grpc::Status::OK);
}

}  // namespace geo
