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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/device.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/query_pool.h"


/* Please see header for specification */
Anvil::QueryPool::QueryPool(const Anvil::BaseDevice* in_device_ptr,
                            VkQueryType              in_query_type,
                            uint32_t                 in_n_max_concurrent_queries,
                            bool                     in_mt_safe)
    :DebugMarkerSupportProvider(in_device_ptr,
                                Anvil::ObjectType::QUERY_POOL),
     MTSafetySupportProvider   (in_mt_safe),
     m_device_ptr              (in_device_ptr),
     m_n_max_indices           (in_n_max_concurrent_queries),
     m_query_type              (in_query_type)
{
    anvil_assert(in_query_type == VK_QUERY_TYPE_OCCLUSION                     ||
                 in_query_type == VK_QUERY_TYPE_TIMESTAMP                     ||
                 in_query_type == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT);

    init(in_query_type,
         Anvil::QueryPipelineStatisticFlagBits::NONE,
         in_n_max_concurrent_queries);

    /* Register the pool wrapper instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::QUERY_POOL,
                                                  this);
}

/* Please see header for specification */
Anvil::QueryPool::QueryPool(const Anvil::BaseDevice*           in_device_ptr,
                            VkQueryType                        in_query_type,
                            Anvil::QueryPipelineStatisticFlags in_pipeline_statistics,
                            uint32_t                           in_n_max_concurrent_queries,
                            bool                               in_mt_safe)
    :DebugMarkerSupportProvider(in_device_ptr,
                                Anvil::ObjectType::QUERY_POOL),
     MTSafetySupportProvider   (in_mt_safe),
     m_device_ptr              (in_device_ptr),
     m_n_max_indices           (in_n_max_concurrent_queries),
     m_query_type              (in_query_type)
{
    init(in_query_type,
         in_pipeline_statistics,
         in_n_max_concurrent_queries);

    /* Register the pool wrapper instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::QUERY_POOL,
                                                  this);
}


/* Please see header for specification */
Anvil::QueryPool::~QueryPool()
{
    /* Unregister the pool wrapper instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::QUERY_POOL,
                                                    this);

    if (m_query_pool_vk != VK_NULL_HANDLE)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyQueryPool(m_device_ptr->get_device_vk(),
                                              m_query_pool_vk,
                                              nullptr /* pAllocator */);
        }
        unlock();

        m_query_pool_vk = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
Anvil::QueryPoolUniquePtr Anvil::QueryPool::create_non_ps_query_pool(const Anvil::BaseDevice* in_device_ptr,
                                                                     VkQueryType              in_query_type,
                                                                     uint32_t                 in_n_max_concurrent_queries,
                                                                     MTSafety                 in_mt_safety)
{
    const bool         mt_safe    = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                    in_device_ptr);
    QueryPoolUniquePtr result_ptr(nullptr,
                                  std::default_delete<QueryPool>() );

    result_ptr.reset(
        new Anvil::QueryPool(in_device_ptr,
                             in_query_type,
                             in_n_max_concurrent_queries,
                             mt_safe)
    );

    return result_ptr;
}

/* Please see header for specification */
Anvil::QueryPoolUniquePtr Anvil::QueryPool::create_ps_query_pool(const Anvil::BaseDevice*           in_device_ptr,
                                                                 Anvil::QueryPipelineStatisticFlags in_pipeline_statistics,
                                                                 uint32_t                           in_n_max_concurrent_queries,
                                                                 MTSafety                           in_mt_safety)
{
    const bool         mt_safe    = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                    in_device_ptr);
    QueryPoolUniquePtr result_ptr(nullptr,
                                  std::default_delete<QueryPool>() );

    result_ptr.reset(
        new Anvil::QueryPool(in_device_ptr,
                             VK_QUERY_TYPE_PIPELINE_STATISTICS,
                             in_pipeline_statistics,
                             in_n_max_concurrent_queries,
                             mt_safe)
    );

    return result_ptr;
}

bool Anvil::QueryPool::get_query_pool_results_internal(const uint32_t&                in_first_query_index,
                                                       const uint32_t&                in_n_queries,
                                                       const Anvil::QueryResultFlags& in_query_props,
                                                       const bool&                    in_should_return_uint64,
                                                       void*                          out_results_ptr,
                                                       bool*                          out_all_query_results_retrieved_ptr)
{
    VkQueryResultFlags flags             = 0;
    uint32_t           result_query_size = 0;
    bool               result            = false;
    VkResult           result_vk;

    if (in_first_query_index + in_n_queries > m_n_max_indices)
    {
        anvil_assert(in_first_query_index + in_n_queries <= m_n_max_indices);

        goto end;
    }

    if (in_should_return_uint64)
    {
        flags             |= VK_QUERY_RESULT_64_BIT;
        result_query_size  = sizeof(uint64_t);
    }
    else
    {
        result_query_size = sizeof(uint32_t);
    }

    if ((in_query_props & Anvil::QueryResultFlagBits::PARTIAL_BIT) != 0)
    {
        flags |= VK_QUERY_RESULT_PARTIAL_BIT;

        if (m_query_type == VK_QUERY_TYPE_TIMESTAMP)
        {
            anvil_assert(m_query_type != VK_QUERY_TYPE_TIMESTAMP);

            goto end;
        }
    }

    if ((in_query_props & Anvil::QueryResultFlagBits::WAIT_BIT) != 0)
    {
        flags |= VK_QUERY_RESULT_WAIT_BIT;
    }

    if ((in_query_props & Anvil::QueryResultFlagBits::WITH_AVAILABILITY_BIT) != 0)
    {
        flags             |= VK_QUERY_RESULT_WITH_AVAILABILITY_BIT;
        result_query_size *= 2;
    }

    /* Execute the request */
    result_vk = Anvil::Vulkan::vkGetQueryPoolResults(m_device_ptr->get_device_vk(),
                                                     m_query_pool_vk,
                                                     in_first_query_index,
                                                     in_n_queries,
                                                     result_query_size * in_n_queries,
                                                     out_results_ptr,
                                                     (in_should_return_uint64) ? sizeof(uint64_t) : sizeof(uint32_t),
                                                     flags);

    if ((in_query_props & Anvil::QueryResultFlagBits::PARTIAL_BIT) != 0)
    {
        result                               = is_vk_call_successful(result_vk);
        *out_all_query_results_retrieved_ptr = (result_vk == VK_SUCCESS);
    }
    else
    {
        result                               = is_vk_call_successful(result_vk);
        *out_all_query_results_retrieved_ptr = (result);
    }

    /* All done */
end:
    return result;
}

/* Please see header for specification */
void Anvil::QueryPool::init(VkQueryType                        in_query_type,
                            Anvil::QueryPipelineStatisticFlags in_pipeline_statistics,
                            uint32_t                           in_n_max_concurrent_queries)
{
    VkQueryPoolCreateInfo create_info;
    VkResult              result_vk   (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    create_info.flags              = 0;
    create_info.pipelineStatistics = (in_query_type == VK_QUERY_TYPE_PIPELINE_STATISTICS) ? in_pipeline_statistics.get_vk()
                                                                                          : 0;
    create_info.pNext              = nullptr;
    create_info.queryCount         = in_n_max_concurrent_queries;
    create_info.queryType          = in_query_type;
    create_info.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;

    result_vk = Anvil::Vulkan::vkCreateQueryPool(m_device_ptr->get_device_vk(),
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
