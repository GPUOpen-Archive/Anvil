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
#include "wrappers/pipeline_cache.h"


/** Please see header for specification */
Anvil::PipelineCache::PipelineCache(const Anvil::BaseDevice* in_device_ptr,
                                    bool                     in_mt_safe,
                                    size_t                   in_initial_data_size,
                                    const void*              in_initial_data)
    :DebugMarkerSupportProvider(in_device_ptr,
                                Anvil::ObjectType::PIPELINE_CACHE),
     MTSafetySupportProvider   (in_mt_safe),
     m_device_ptr              (in_device_ptr),
     m_pipeline_cache          (VK_NULL_HANDLE)
{
    VkPipelineCacheCreateInfo cache_create_info;
    VkResult                  result_vk        (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    cache_create_info.flags           = 0;
    cache_create_info.initialDataSize = in_initial_data_size;
    cache_create_info.pInitialData    = in_initial_data;
    cache_create_info.pNext           = nullptr;
    cache_create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    result_vk = Anvil::Vulkan::vkCreatePipelineCache(m_device_ptr->get_device_vk(),
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
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::PIPELINE_CACHE,
                                                  this);
}

/** Please see header for specification */
Anvil::PipelineCache::~PipelineCache()
{
    /* Unregister the instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::PIPELINE_CACHE,
                                                    this);

    if (m_pipeline_cache != VK_NULL_HANDLE)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyPipelineCache(m_device_ptr->get_device_vk(),
                                                  m_pipeline_cache,
                                                  nullptr /* pAllocator */);
        }
        unlock();

        m_pipeline_cache = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
Anvil::PipelineCacheUniquePtr Anvil::PipelineCache::create(const Anvil::BaseDevice* in_device_ptr,
                                                           bool                     in_mt_safe,
                                                           size_t                   in_initial_data_size,
                                                           const void*              in_initial_data)
{
    PipelineCacheUniquePtr result_ptr(nullptr,
                                      std::default_delete<PipelineCache>() );

    result_ptr.reset(
        new Anvil::PipelineCache(in_device_ptr,
                                 in_mt_safe,
                                 in_initial_data_size,
                                 in_initial_data)
    );

    return result_ptr;
}

/** Please see header for specification */
bool Anvil::PipelineCache::get_data(size_t* out_n_data_bytes_ptr,
                                    void*   out_data_ptr)
{
    VkResult result_vk;

    result_vk = Anvil::Vulkan::vkGetPipelineCacheData(m_device_ptr->get_device_vk(),
                                                      m_pipeline_cache,
                                                      out_n_data_bytes_ptr,
                                                      out_data_ptr);

    return is_vk_call_successful(result_vk);
}

/** Please see header for specification */
bool Anvil::PipelineCache::merge(uint32_t                           in_n_pipeline_caches,
                                 const Anvil::PipelineCache* const* in_src_cache_ptrs)
{
    VkResult                     result_vk;
    std::vector<VkPipelineCache> src_pipeline_caches(in_n_pipeline_caches);

    anvil_assert(in_n_pipeline_caches < sizeof(src_pipeline_caches) / sizeof(src_pipeline_caches[0]) );


    for (uint32_t n_pipeline_cache = 0;
                  n_pipeline_cache < in_n_pipeline_caches;
                ++n_pipeline_cache)
    {
        src_pipeline_caches[n_pipeline_cache] = in_src_cache_ptrs[n_pipeline_cache]->get_pipeline_cache();
    }

    lock();
    {
        result_vk = Anvil::Vulkan::vkMergePipelineCaches(m_device_ptr->get_device_vk(),
                                                         m_pipeline_cache,
                                                         in_n_pipeline_caches,
                                                        &src_pipeline_caches.at(0) );
    }
    unlock();

    anvil_assert(result_vk);

    return is_vk_call_successful(result_vk);
}
