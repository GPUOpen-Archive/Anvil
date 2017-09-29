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
#include "wrappers/pipeline_cache.h"


/** Please see header for specification */
Anvil::PipelineCache::PipelineCache(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                    size_t                           in_initial_data_size,
                                    const void*                      in_initial_data)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT),
     m_device_ptr              (in_device_ptr),
     m_pipeline_cache          (VK_NULL_HANDLE)
{
    VkPipelineCacheCreateInfo          cache_create_info;
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(in_device_ptr);
    VkResult                           result_vk        (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    cache_create_info.flags           = 0;
    cache_create_info.initialDataSize = in_initial_data_size;
    cache_create_info.pInitialData    = in_initial_data;
    cache_create_info.pNext           = nullptr;
    cache_create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    result_vk = vkCreatePipelineCache(device_locked_ptr->get_device_vk(),
                                     &cache_create_info,
                                      nullptr, /* pAllocator */
                                     &m_pipeline_cache);

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk) )
    {
        set_vk_handle(m_pipeline_cache);
    }

    anvil_assert(m_pipeline_cache != VK_NULL_HANDLE);

    /* Register the instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_PIPELINE_CACHE,
                                                  this);
}

/** Please see header for specification */
Anvil::PipelineCache::~PipelineCache()
{
    /* Unregister the instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_PIPELINE_CACHE,
                                                    this);

    if (m_pipeline_cache != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroyPipelineCache(device_locked_ptr->get_device_vk(),
                               m_pipeline_cache,
                               nullptr /* pAllocator */);

        m_pipeline_cache = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
std::shared_ptr<Anvil::PipelineCache> Anvil::PipelineCache::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                   size_t                           in_initial_data_size,
                                                                   const void*                      in_initial_data)
{
    std::shared_ptr<Anvil::PipelineCache> result_ptr;

    result_ptr.reset(
        new Anvil::PipelineCache(in_device_ptr,
                                 in_initial_data_size,
                                 in_initial_data)
    );

    return result_ptr;
}

/** Please see header for specification */
bool Anvil::PipelineCache::get_data(size_t*      out_n_data_bytes_ptr,
                                    const void** out_data_ptr)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkResult                           result_vk;

    result_vk = vkGetPipelineCacheData(device_locked_ptr->get_device_vk(),
                                       m_pipeline_cache,
                                       out_n_data_bytes_ptr,
                                       out_data_ptr);

    anvil_assert(result_vk);

    return is_vk_call_successful(result_vk);
}

/** Please see header for specification */
bool Anvil::PipelineCache::merge(uint32_t                                           in_n_pipeline_caches,
                                 std::shared_ptr<const Anvil::PipelineCache> const* in_src_cache_ptrs)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkResult                           result_vk;
    VkPipelineCache                    src_pipeline_caches[64];

    anvil_assert(in_n_pipeline_caches < sizeof(src_pipeline_caches) / sizeof(src_pipeline_caches[0]) );

    for (uint32_t n_pipeline_cache = 0;
                  n_pipeline_cache < in_n_pipeline_caches;
                ++n_pipeline_cache)
    {
        src_pipeline_caches[n_pipeline_cache] = in_src_cache_ptrs[n_pipeline_cache]->get_pipeline_cache();
    }

    result_vk = vkMergePipelineCaches(device_locked_ptr->get_device_vk(),
                                      m_pipeline_cache,
                                      in_n_pipeline_caches,
                                      src_pipeline_caches);

    anvil_assert(result_vk);

    return is_vk_call_successful(result_vk);
}
