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
#include "wrappers/pipeline_cache.h"


/** Please see header for specification */
Anvil::PipelineCache::PipelineCache(Anvil::Device* device_ptr,
                                    size_t         initial_data_size,
                                    const void*    initial_data)
    :m_device_ptr    (device_ptr),
     m_pipeline_cache(VK_NULL_HANDLE)
{
    VkPipelineCacheCreateInfo cache_create_info;
    VkResult                  result_vk;

    anvil_assert(device_ptr != nullptr);

    cache_create_info.flags           = 0;
    cache_create_info.initialDataSize = initial_data_size;
    cache_create_info.pInitialData    = initial_data;
    cache_create_info.pNext           = nullptr;
    cache_create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    result_vk = vkCreatePipelineCache(device_ptr->get_device_vk(),
                                     &cache_create_info,
                                      nullptr, /* pAllocator */
                                     &m_pipeline_cache);

    anvil_assert_vk_call_succeeded(result_vk);
    anvil_assert                  (m_pipeline_cache != VK_NULL_HANDLE);

    /* Register the instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_PIPELINE_CACHE,
                                                  this);
}

/** Please see header for specification */
Anvil::PipelineCache::~PipelineCache()
{
    if (m_pipeline_cache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(m_device_ptr->get_device_vk(),
                               m_pipeline_cache,
                               nullptr /* pAllocator */);

        m_pipeline_cache = VK_NULL_HANDLE;
    }

    /* Unregister the instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_PIPELINE_CACHE,
                                                    this);
}

/** Please see header for specification */
bool Anvil::PipelineCache::get_data(size_t*      out_n_data_bytes_ptr,
                                    const void** out_data_ptr)
{
    VkResult result_vk;

    result_vk = vkGetPipelineCacheData(m_device_ptr->get_device_vk(),
                                       m_pipeline_cache,
                                       out_n_data_bytes_ptr,
                                       out_data_ptr);

    anvil_assert(result_vk);

    return is_vk_call_successful(result_vk);
}

/** Please see header for specification */
bool Anvil::PipelineCache::merge(uint32_t                           n_pipeline_caches,
                                 const Anvil::PipelineCache* const* src_cache_ptrs)
{
    VkResult        result_vk;
    VkPipelineCache src_pipeline_caches[64];

    anvil_assert(n_pipeline_caches < sizeof(src_pipeline_caches) / sizeof(src_pipeline_caches[0]) );

    for (uint32_t n_pipeline_cache = 0;
                  n_pipeline_cache < n_pipeline_caches;
                ++n_pipeline_cache)
    {
        src_pipeline_caches[n_pipeline_cache] = src_cache_ptrs[n_pipeline_cache]->get_pipeline_cache();
    }

    result_vk = vkMergePipelineCaches(m_device_ptr->get_device_vk(),
                                      m_pipeline_cache,
                                      n_pipeline_caches,
                                      src_pipeline_caches);

    anvil_assert(result_vk);

    return is_vk_call_successful(result_vk);
}
