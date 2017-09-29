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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/device.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/query_pool.h"


/* Please see header for specification */
Anvil::QueryPool::QueryPool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                            VkQueryType                      in_query_type,
                            uint32_t                         in_n_max_concurrent_queries)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT),
     m_device_ptr              (in_device_ptr),
     m_n_max_indices           (in_n_max_concurrent_queries)
{
    anvil_assert(in_query_type == VK_QUERY_TYPE_OCCLUSION ||
                 in_query_type == VK_QUERY_TYPE_TIMESTAMP);

    init(in_device_ptr,
         in_query_type,
         0, /* in_flags - irrelevant */
         in_n_max_concurrent_queries);

    /* Register the pool wrapper instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_QUERY_POOL,
                                                  this);
}

/* Please see header for specification */
Anvil::QueryPool::QueryPool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                            VkQueryType                      in_query_type,
                            VkFlags                          in_query_flags,
                            uint32_t                         in_n_max_concurrent_queries)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT),
     m_device_ptr   (in_device_ptr),
     m_n_max_indices(in_n_max_concurrent_queries)
{
    init(in_device_ptr,
         in_query_type,
         in_query_flags,
         in_n_max_concurrent_queries);

    /* Register the pool wrapper instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_QUERY_POOL,
                                                  this);
}


/* Please see header for specification */
Anvil::QueryPool::~QueryPool()
{
    /* Unregister the pool wrapper instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_QUERY_POOL,
                                                    this);

    if (m_query_pool_vk != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroyQueryPool(device_locked_ptr->get_device_vk(),
                           m_query_pool_vk,
                           nullptr /* pAllocator */);

        m_query_pool_vk = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
std::shared_ptr<Anvil::QueryPool> Anvil::QueryPool::create_non_ps_query_pool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                             VkQueryType                      in_query_type,
                                                                             uint32_t                         in_n_max_concurrent_queries)
{
    std::shared_ptr<Anvil::QueryPool> result_ptr;

    result_ptr.reset(
        new Anvil::QueryPool(in_device_ptr,
                             in_query_type,
                             in_n_max_concurrent_queries)
    );

    return result_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::QueryPool> Anvil::QueryPool::create_ps_query_pool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                         VkQueryPipelineStatisticFlags    in_pipeline_statistics,
                                                                         uint32_t                         in_n_max_concurrent_queries)
{
    std::shared_ptr<Anvil::QueryPool> result_ptr;

    result_ptr.reset(
        new Anvil::QueryPool(in_device_ptr,
                             VK_QUERY_TYPE_PIPELINE_STATISTICS,
                             in_pipeline_statistics,
                             in_n_max_concurrent_queries)
    );

    return result_ptr;
}

/* Please see header for specification */
void Anvil::QueryPool::init(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
                            VkQueryType                       in_query_type,
                            VkFlags                           in_query_flags,
                            uint32_t                          in_n_max_concurrent_queries)
{
    VkQueryPoolCreateInfo              create_info;
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkResult                           result_vk        (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    create_info.flags              = 0;
    create_info.pipelineStatistics = (in_query_type == VK_QUERY_TYPE_PIPELINE_STATISTICS) ? in_query_flags : 0;
    create_info.pNext              = nullptr;
    create_info.queryCount         = in_n_max_concurrent_queries;
    create_info.queryType          = in_query_type;
    create_info.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;

    result_vk = vkCreateQueryPool(device_locked_ptr->get_device_vk(),
                                 &create_info,
                                  nullptr, /* pAllocator */
                                 &m_query_pool_vk);

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk) )
    {
        anvil_assert(m_query_pool_vk != VK_NULL_HANDLE);

        set_vk_handle(m_query_pool_vk);
    }
}
