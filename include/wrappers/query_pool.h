//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef WRAPPERS_QUERY_POOL_H
#define WRAPPERS_QUERY_POOL_H

#include "misc/ref_counter.h"
#include "misc/pools.h"
#include "misc/types.h"


namespace Anvil
{
    /** Implements IPoolWorker interface for a query pool worker. Used internally
     *  by QueryPool wrapper.
     **/
    class QueryPoolWorker : public IPoolWorker<QueryIndex>
    {
    public:
        /* Public functions */

        /** Constructor
         *
         *  @param device_ptr               Device to create the query pool for. Must not be nullptr.
         *  @param query_type               Type of the query to instantiate the worker for.
         *  @param ps_flags                 Pipeline Statistic flags to instantiate the worker with.
         *  @param n_max_concurrent_queries Maximum number of queries which can be used concurrently with
         *                                  the query pool.
         **/
        QueryPoolWorker(Anvil::Device*                device_ptr,
                        VkQueryType                   query_type,
                        VkQueryPipelineStatisticFlags ps_flags,
                        uint32_t                      n_max_concurrent_queries);

        /** Destructor. */
        virtual ~QueryPoolWorker();

        /** Reserve a query from the pool and returns a QueryIndex value holding the allocated query index. */
        QueryIndex* create_item();

        /** Retrieves the raw Vulkan handle of the encapsulated query pool. */
        VkQueryPool get_query_pool() const
        {
            return m_query_pool_vk;
        }

        /** Releases a query at @param item_ptr index back to the pool. */
        void release_item(QueryIndex* item_ptr);

    private:
        /* Private functions */

        /** Stub - do not use */
        void reset_item(QueryIndex* item_ptr);

        /* Private variables */
        Anvil::Device* m_device_ptr;
        uint32_t       m_indices_created;
        uint32_t       m_n_max_indices;
        VkQueryPool    m_query_pool_vk;
    };

    /* Implements a query pool wrapper. */
    class QueryPool : public GenericPool<QueryIndex>,
                      public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor which should be used to initialize a new query pool instance
         *  for occlusion or timestamp queries.
         *
         *  Note that a query pool preallocates the requested number of queries. This number
         *  cannot be increased after the object is spawned.
         *
         *  @param device_ptr               Device to use.
         *  @param query_type               Type of the query to create the query pool for. May
         *                                  either be VK_QUERY_TYPE_OCCLUSION or
         *                                  VK_QUERY_TYPE_PIPELINE_STATISTICS.
         *  @param n_max_concurrent_queries Maximum number of queries which are going to be in-flight
         *                                  for this query pool.
         *
         **/
        explicit QueryPool(Anvil::Device* device_ptr,
                           VkQueryType    query_type,
                           uint32_t       n_max_concurrent_queries);

        /** Constructor which should be used to initialize a new query pool instance
         *  for pipeline statistics queries.
         *
         *  Note that a query pool preallocates the requested number of queries. This number
         *  cannot be increased after the object is spawned.
         *
         *  @param device_ptr               Device to use.
         *  @param pipeline_statistics      Pipeline statistics flags the query should support.
         *  @param n_max_concurrent_queries Number of queries to preallocate in the pool.
         *
         **/
        explicit QueryPool(Anvil::Device*                device_ptr,
                           VkQueryPipelineStatisticFlags pipeline_statistics,
                           uint32_t                      n_max_concurrent_queries);

        /** Destructor */
        virtual ~QueryPool();

        /** Retrieves a raw Vulkan handle of the encapsulated query pool */
        VkQueryPool get_query_pool() const
        {
            return dynamic_cast<const QueryPoolWorker*>(get_worker_ptr() )->get_query_pool();
        }

        /** Returns an unused query index and removes it from the pool. The query index should
         *  be returned to the pool when no longer used.
         *
         *  This function will throw an assertion failure, if the client has already retrieved
         *  all available queries.
         *
         *  NOTE: It is user's responsibility to reset the retrieved query before using it!
         *
         **/
        QueryIndex* pop_query_from_pool();

        /** Returns a user-assigned query back to the pool. */
        void return_query_to_pool(QueryIndex query_index);
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_QUERY_POOL_H */
