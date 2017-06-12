//
// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
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

#include "misc/debug_marker.h"
#include "misc/ref_counter.h"
#include "misc/pools.h"
#include "misc/types.h"


namespace Anvil
{
    /* Implements a query pool wrapper. */
    class QueryPool : public DebugMarkerSupportProvider<QueryPool>
    {
    public:
        /* Public functions */

        /** Creates a new occlusion/timestamp query pool.
         *
         *  Note that a query pool preallocates the requested number of queries. This number
         *  cannot be increased after the object is spawned.
         *
         *  @param in_device_ptr               Device to use.
         *  @param in_query_type               Type of the query to create the query pool for. May
         *                                     either be VK_QUERY_TYPE_OCCLUSION or
         *                                     VK_QUERY_TYPE_PIPELINE_STATISTICS.
         *  @param in_n_max_concurrent_queries Maximum number of queries which are going to be in-flight
         *                                     for this query pool.
         *
         **/
        static std::shared_ptr<QueryPool> create_non_ps_query_pool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                   VkQueryType                      in_query_type,
                                                                   uint32_t                         in_n_max_concurrent_queries);

        /** Creates a new pipeline statistics query pool.
         *
         *  Note that a query pool preallocates the requested number of queries. This number
         *  cannot be increased after the object is spawned.
         *
         *  @param in_device_ptr               Device to use.
         *  @param in_pipeline_statistics      Pipeline statistics flags the query should support.
         *  @param in_n_max_concurrent_queries Number of queries to preallocate in the pool.
         *
         **/
        static std::shared_ptr<QueryPool> create_ps_query_pool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                               VkQueryPipelineStatisticFlags    in_pipeline_statistics,
                                                               uint32_t                         in_n_max_concurrent_queries);

        /** Destructor */
        virtual ~QueryPool();

        /** Retrieves pool capacity */
        uint32_t get_capacity() const
        {
            return m_n_max_indices;
        }

        /** Retrieves the raw Vulkan handle of the encapsulated query pool. */
        VkQueryPool get_query_pool() const
        {
            return m_query_pool_vk;
        }


    private:
        /* Constructor. Please see corresponding create() for specification */
        explicit QueryPool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                           VkQueryType                      in_query_type,
                           uint32_t                         in_n_max_concurrent_queries);

        /* Constructor. Please see corresponding create() for specification */
        explicit QueryPool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                           VkQueryType                      in_query_type,
                           VkFlags                          in_query_flags,
                           uint32_t                         in_n_max_concurrent_queries);

        /** Initializes the Vulkan counterpart.
         *
         *  @param in_device_ptr               Device to create the query pool for. Must not be nullptr.
         *  @param in_query_type               Type of the query to instantiate the pool for.
         *  @param in_flags                    Query flags to instantiate the pool with.
         *  @param in_n_max_concurrent_queries Maximum number of queries which can be used concurrently with
         *                                     the query pool.
         **/
        void init(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                  VkQueryType                      in_query_type,
                  VkFlags                          in_flags,
                  uint32_t                         in_n_max_concurrent_queries);

        /* Private variables */
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        uint32_t                         m_n_max_indices;
        VkQueryPool                      m_query_pool_vk;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_QUERY_POOL_H */
