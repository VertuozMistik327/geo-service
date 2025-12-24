#pragma once

#include "../search/SearchEngineItf.h"
#include "geo.grpc.pb.h"

#include <absl/log/log.h>
#include <grpc/grpc.h>
#include <grpcpp/support/server_callback.h>

#include <cstdint>
#include <vector>

namespace geo
{

// Reactor class for handling streaming responses for the GetRegionsStream RPC.
// This class processes a request and streams region data back to the client
// as results are found for each bounding box partition.
class GetRegionsStreamReactor : public grpc::ServerWriteReactor<geoproto::RegionsResponse>
{
public:
   // Constructor for the GetRegionsStreamReactor.
   // @param context: Server context.
   // @param request: The incoming RegionsRequest containing search parameters.
   // @param searchEngine: Reference to the search engine used to find regions.
   // @param maxBoxWidth: Maximum width of each sub-box in degrees longitude.
   // @param maxBoxHeight: Maximum height of each sub-box in degrees latitude.
   GetRegionsStreamReactor(grpc::CallbackServerContext* context, const geoproto::RegionsRequest& request,
      ISearchEngine& searchEngine, std::uint32_t maxBoxWidth, std::uint32_t maxBoxHeight);

private:
   // Called when a write operation completes. Processes the next bounding box partition.
   void OnWriteDone(bool ok) override;

   // Called when the RPC is completed. Logs completion and cleans up the reactor.
   void OnDone() override
   {
      LOG(INFO) << "GetRegionsStream() RPC completed";
      delete this;
   }

   // Called when the RPC is cancelled. Logs the cancellation.
   void OnCancel() override { LOG(ERROR) << "GetRegionsStream() RPC cancelled"; }

   // Processes the next bounding box and sends results to the client.
   void processNextBox();

   // Search preferences extracted from the request.
   ISearchEngine::RegionPreferences m_prefs;

   // Vector of bounding boxes to process.
   std::vector<std::array<double, 4>> m_boundingBoxes;

   // Current index in the bounding boxes vector.
   std::size_t m_currentBoxIndex = 0;

   // Handler for incremental region search.
   ISearchEngine::IncrementalSearchHandler m_searchHandler;

   // Response message to be reused for each partition.
   geoproto::RegionsResponse m_response;
};

}  // namespace geo
