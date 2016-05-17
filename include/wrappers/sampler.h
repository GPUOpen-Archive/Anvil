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

/** Implements a wrapper for a single Vulkan sampler. Implemented in order to:
 *
 *  - simplify debugging, life-time management and usage.
 *  - let ObjectTracker detect leaking event instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_SAMPLER_H
#define WRAPPERS_SAMPLER_H

#include "misc/ref_counter.h"
#include "misc/types.h"


namespace Anvil
{
    /** Wrapper class for Vulkan samplers */
    class Sampler : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  Creates a single Sampler instance and registers the object in Object Tracker.
         *
         *  For argument discussion, please consult Vulkan API specification.
         */
        Sampler(Anvil::Device*       device_ptr,
                VkFilter             mag_filter,
                VkFilter             min_filter,
                VkSamplerMipmapMode  mipmap_mode,
                VkSamplerAddressMode address_mode_u,
                VkSamplerAddressMode address_mode_v,
                VkSamplerAddressMode address_mode_w,
                float                lod_bias,
                float                max_anisotropy,
                bool                 compare_enable,
                VkCompareOp          compare_op,
                float                min_lod,
                float                max_lod,
                VkBorderColor        border_color,
                bool                 use_unnormalized_coordinates);

        /** Retrieves a raw Vulkan handle for the underlying VkSampler instance. */
        VkSampler get_sampler() const
        {
            return m_sampler;
        }

        /** Retrieves a pointer to the raw Vulkan handle for the underlying VkSampler instance. */
        const VkSampler* get_sampler_ptr() const
        {
            return &m_sampler;
        }

    private:
        /* Private functions */
        Sampler           (const Sampler&);
        Sampler& operator=(const Sampler&);

        virtual ~Sampler();

        /* Private variables */
        VkSamplerAddressMode m_address_mode_u;
        VkSamplerAddressMode m_address_mode_v;
        VkSamplerAddressMode m_address_mode_w;
        VkBorderColor        m_border_color;
        bool                 m_compare_enable;
        VkCompareOp          m_compare_op;
        float                m_lod_bias;
        VkFilter             m_mag_filter;
        float                m_max_anisotropy;
        float                m_max_lod;
        VkFilter             m_min_filter;
        float                m_min_lod;
        VkSamplerMipmapMode  m_mipmap_mode;
        bool                 m_use_unnormalized_coordinates;

        Anvil::Device* m_device_ptr;
        VkSampler      m_sampler;
    };

    /** Functor to delete the sampler wrapper. Useful for encapsulating sampler wrappers
     *  inside auto pointers.
     */
    struct SamplerDeleter
    {
        void operator()(Sampler* sampler_ptr) const
        {
            sampler_ptr->release();
        }
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SAMPLER_H */