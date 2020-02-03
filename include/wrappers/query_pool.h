//
// Copyright (c) 2017-2018 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/mt_safety.h"
#include "misc/ref_counter.h"
#include "misc/pools.h"
#include "misc/types.h"


namespace Anvil
{
    /* Implements a query pool wrapper. */
    class QueryPool : public DebugMarkerSupportProvider<QueryPool>,
                      public MTSafetySupportProvider
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
        static Anvil::QueryPoolUniquePtr create_non_ps_query_pool(const Anvil::BaseDevice* in_device_ptr,
                                                                  VkQueryType              in_query_type,
                                                                  uint32_t                 in_n_max_concurrent_queries,
                                                                  MTSafety                 in_mt_safety                = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE);

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
        static Anvil::QueryPoolUniquePtr create_ps_query_pool(const Anvil::BaseDevice*           in_device_ptr,
                                                              Anvil::QueryPipelineStatisticFlags in_pipeline_statistics,
                                                              uint32_t                           in_n_max_concurrent_queries,
                                                              MTSafety                           in_mt_safety                = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE);

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

        /* Uses vkGetQueryPoolResults() to retrieve result values for the user-specified query range.
         *
         * NOTE: It is assumed result values are to be returned to a tightly-packed array of size
         *       @param in_n_queries * sizeof(query result type). if @param in_query_props includes QUERY_RESULT_WITH_AVAILABILITY_BIT,
         *       twice the amount of memory is needed for result storage.
         *
         * NOTE: It is caller's responsibility to follow the requirements listed in the spec which guarantee
         *       the results returned by this entrypoint are correct.
         *
         **/
        bool get_query_pool_results(const uint32_t&                in_first_query_index,
                                    const uint32_t&                in_n_queries,
                                    const Anvil::QueryResultFlags& in_query_props,
                                    uint32_t*                      out_results_ptr,
                                    bool*                          out_all_query_results_retrieved_ptr)
        {
            return get_query_pool_results_internal(in_first_query_index,
                                                   in_n_queries,
                                                   in_query_props,
                                                   false, /* should_return_uint64 */
                                                   out_results_ptr,
                                                   out_all_query_results_retrieved_ptr);
        }

        bool get_query_pool_results(const uint32_t&                in_first_query_index,
                                    const uint32_t&                in_n_queries,
                                    const Anvil::QueryResultFlags& in_query_props,
                                    uint64_t*                      out_results_ptr,
                                    bool*                          out_all_query_results_retrieved_ptr)
        {
            return get_query_pool_results_internal(in_first_query_index,
                                                   in_n_queries,
                                                   in_query_props,
                                                   true, /* should_return_uint64 */
                                                   out_results_ptr,
                                                   out_all_query_results_retrieved_ptr);
        }

    private:
        /* Constructor. Please see corresponding create() for specification */
        explicit QueryPool(const Anvil::BaseDevice* in_device_ptr,
                           VkQueryType              in_query_type,
                           uint32_t                 in_n_max_concurrent_queries,
                           bool                     in_mt_safe);

        /* Constructor. Please see corresponding create() for specification */
        explicit QueryPool(const Anvil::BaseDevice*           in_device_ptr,
                           VkQueryType                        in_query_type,
                           Anvil::QueryPipelineStatisticFlags in_pipeline_statistics,
                           uint32_t                           in_n_max_concurrent_queries,
                           bool                               in_mt_safe);

        bool get_query_pool_results_internal(const uint32_t&                in_first_query_index,
                                             const uint32_t&                in_n_queries,
                                             const Anvil::QueryResultFlags& in_query_props,
                                             const bool&                    in_should_return_uint64,
                                             void*                          out_results_ptr,
                                             bool*                          out_all_query_results_retrieved_ptr);

        /** Initializes the Vulkan counterpart.
         *
         *  @param in_query_type               Type of the query to instantiate the pool for.
         *  @param in_flags                    Query flags to instantiate the pool with.
         *  @param in_n_max_concurrent_queries Maximum number of queries which can be used concurrently with
         *                                     the query pool.
         **/
        void init(VkQueryType                        in_query_type,
                  Anvil::QueryPipelineStatisticFlags in_pipeline_statistics,
                  uint32_t                           in_n_max_concurrent_queries);

        /* Private variables */
        const Anvil::BaseDevice* m_device_ptr;
        uint32_t                 m_n_max_indices;
        VkQueryPool              m_query_pool_vk;
        const VkQueryType        m_query_type;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_QUERY_POOL_H */
