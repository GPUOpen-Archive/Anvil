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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/device.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/query_pool.h"

/* Please see header for specification */
Anvil::QueryPoolWorker::QueryPoolWorker(Anvil::Device*                device_ptr,
                                        VkQueryType                   query_type,
                                        VkQueryPipelineStatisticFlags ps_flags,
                                        uint32_t                      n_max_concurrent_queries)
    :m_device_ptr     (device_ptr),
     m_indices_created(0),
     m_n_max_indices  (n_max_concurrent_queries),
     m_query_pool_vk  (VK_NULL_HANDLE)
{
    VkQueryPoolCreateInfo create_info;
    VkResult              result_vk;

    create_info.flags              = 0;
    create_info.pipelineStatistics = ps_flags;
    create_info.pNext              = nullptr;
    create_info.queryCount         = n_max_concurrent_queries;
    create_info.queryType          = query_type;
    create_info.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;

    result_vk = vkCreateQueryPool(m_device_ptr->get_device_vk(),
                                 &create_info,
                                  nullptr, /* pAllocator */
                                 &m_query_pool_vk);

    anvil_assert                  (m_query_pool_vk != VK_NULL_HANDLE);
    anvil_assert_vk_call_succeeded(result_vk);
}

/* Please see header for specification */
Anvil::QueryPoolWorker::~QueryPoolWorker()
{
    if (m_query_pool_vk != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(m_device_ptr->get_device_vk(),
                           m_query_pool_vk,
                           nullptr /* pAllocator */);

        m_query_pool_vk = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
Anvil::QueryIndex* Anvil::QueryPoolWorker::create_item()
{
    /* Query indices start from 0 and increase linearly until the pool size, as specified
     * at creation time, is reached */
    anvil_assert(m_indices_created < m_n_max_indices);

    return reinterpret_cast<QueryIndex*>( static_cast<uintptr_t>(m_indices_created++) );
}

/* Please see header for specification */
void Anvil::QueryPoolWorker::release_item(QueryIndex* item_ptr)
{
    /* Stub - Vulkan query pool does not require us to return queries */
}

/* Please see header for specification */
void Anvil::QueryPoolWorker::reset_item(QueryIndex* item_ptr)
{
    /* It is user's responsibility to reset the retrieved queries. If we were to handle this,
     * we'd need to build a command buffer every time a reset request arrives, which would be
     * extremely inefficient.
     */
    anvil_assert(false);
}

/* Please see header for specification */
Anvil::QueryPool::QueryPool(Anvil::Device* device_ptr,
                            VkQueryType    query_type,
                            uint32_t       n_max_concurrent_queries)

    :GenericPool(n_max_concurrent_queries,
                 new QueryPoolWorker(device_ptr,
                                     query_type,
                                     0, /* ps_flags - irrelevant */
                                     n_max_concurrent_queries) )
{
    anvil_assert(query_type == VK_QUERY_TYPE_OCCLUSION ||
                query_type == VK_QUERY_TYPE_TIMESTAMP);

    /* Register the pool wrapper instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_QUERY_POOL,
                                                  this);
}

/* Please see header for specification */
Anvil::QueryPool::QueryPool(Anvil::Device*                device_ptr,
                            VkQueryPipelineStatisticFlags pipeline_statistics,
                            uint32_t                      n_max_concurrent_queries)

    :GenericPool(n_max_concurrent_queries,
                 new QueryPoolWorker(device_ptr,
                                     VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                     pipeline_statistics,
                                     n_max_concurrent_queries) )
{
    /* Register the pool wrapper instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_QUERY_POOL,
                                                  this);
}


/* Please see header for specification */
Anvil::QueryPool::~QueryPool()
{
    /* Unregister the pool wrapper instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_QUERY_POOL,
                                                    this);
}

/* Please see header for specification */
Anvil::QueryIndex* Anvil::QueryPool::pop_query_from_pool()
{
    return reinterpret_cast<Anvil::QueryIndex*>(GenericPool::get_item() );
}

/* Please see header for specification */
void Anvil::QueryPool::return_query_to_pool(QueryIndex query_index)
{
    GenericPool::return_item(reinterpret_cast<QueryIndex*>( static_cast<uintptr_t>(query_index) ));
}
