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

/** Implements a wrapper for a single Vulkan pipeline cache. Implemented in order to:
 *
 *  - manage life-time of pipeline cache instances.
 *  - let ObjectTracker detect leaking queue pipeline cache instances.
 *
 *  The wrapper is NOT thread-safe.
 **/

#ifndef WRAPPERS_PIPELINE_CACHE_H
#define WRAPPERS_PIPELINE_CACHE_H

#include "misc/ref_counter.h"
#include "misc/types.h"


namespace Anvil
{
    class PipelineCache : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  @param device_ptr        Vulkan device to initialize the pipeline cache with.
         *  @param initial_data_size Number of bytes available under @param initial_data. Can be 0.
         *  @param initial_data      Initial data to initialize the new pipeline cache instance with.
         *                           May be nullptr if @param initial_data_size is 0.
         **/
        PipelineCache(Anvil::Device* device_ptr,
                      size_t         initial_data_size = 0,
                      const void*    initial_data      = nullptr);

        /** Retrieves pipeline cache data.
         *
         *  @param out_n_data_bytes_ptr Deref will be set to the number of bytes @param out_data_ptr must
         *                              provide. Must not be nullptr.
         *  @param out_data_ptr         Pointer to the buffer to hold the requested data. May be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_data(size_t*      out_n_data_bytes_ptr,
                      const void** out_data_ptr);

        /** Retrieves raw Vulkan pipeline cache handle */
        const VkPipelineCache& get_pipeline_cache() const
        {
            return m_pipeline_cache;
        }

        /** Adds cached pipelines in @param src_cache_ptrs to this pipeline instance.
         *
         *  @param n_pipeline_caches Number of pipeline caches under @param src_cache_ptrs.
         *  @param src_cache_ptrs    Pipeline caches to merge with. Must not be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool merge(uint32_t                           n_pipeline_caches,
                   const Anvil::PipelineCache* const* src_cache_ptrs);

    private:
        /* Private functions */
        PipelineCache           (const PipelineCache&);
        PipelineCache& operator=(const PipelineCache&);

        virtual ~PipelineCache();

        /* Private variables */
        Anvil::Device*  m_device_ptr;
        VkPipelineCache m_pipeline_cache;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_PIPELINE_CACHE_H */