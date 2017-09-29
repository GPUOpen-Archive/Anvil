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

/** Implements a wrapper for a single Vulkan sampler. Implemented in order to:
 *
 *  - simplify debugging, life-time management and usage.
 *  - let ObjectTracker detect leaking event instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_SAMPLER_H
#define WRAPPERS_SAMPLER_H

#include "misc/debug_marker.h"
#include "misc/types.h"


namespace Anvil
{
    /** Wrapper class for Vulkan samplers */
    class Sampler : public DebugMarkerSupportProvider<Sampler>
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  Creates a single Sampler instance and registers the object in Object Tracker.
         *
         *  For argument discussion, please consult Vulkan API specification.
         */
        static std::shared_ptr<Sampler> create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                               VkFilter                         in_mag_filter,
                                               VkFilter                         in_min_filter,
                                               VkSamplerMipmapMode              in_mipmap_mode,
                                               VkSamplerAddressMode             in_address_mode_u,
                                               VkSamplerAddressMode             in_address_mode_v,
                                               VkSamplerAddressMode             in_address_mode_w,
                                               float                            in_lod_bias,
                                               float                            in_max_anisotropy,
                                               bool                             in_compare_enable,
                                               VkCompareOp                      in_compare_op,
                                               float                            in_min_lod,
                                               float                            in_max_lod,
                                               VkBorderColor                    in_border_color,
                                               bool                             in_use_unnormalized_coordinates);

        /** Destructor.
         *
         *  Releases the underlying Vulkan Sampler instance and signs the wrapper object out from
         *  the Object Tracker.
         **/
        virtual ~Sampler();

        VkSamplerAddressMode get_address_mode_u() const
        {
            return m_address_mode_u;
        }

        VkSamplerAddressMode get_address_mode_v() const
        {
            return m_address_mode_v;
        }

        VkSamplerAddressMode get_address_mode_w() const
        {
            return m_address_mode_w;
        }

        VkBorderColor get_border_color() const
        {
            return m_border_color;
        }

        VkCompareOp get_compare_op() const
        {
            return m_compare_op;
        }

        /** Retrieves a raw Vulkan handle for the underlying VkSampler instance. */
        VkSampler get_sampler() const
        {
            return m_sampler;
        }

        float get_lod_bias() const
        {
            return m_lod_bias;
        }

        VkFilter get_mag_filter() const
        {
            return m_mag_filter;
        }

        float get_max_anisotropy() const
        {
            return m_max_anisotropy;
        }

        float get_max_lod() const
        {
            return m_max_lod;
        }

        VkFilter get_min_filter() const
        {
            return m_min_filter;
        }

        float get_min_lod() const
        {
            return m_min_lod;
        }

        VkSamplerMipmapMode get_mipmap_mode() const
        {
            return m_mipmap_mode;
        }

        /** Retrieves a pointer to the raw Vulkan handle for the underlying VkSampler instance. */
        const VkSampler* get_sampler_ptr() const
        {
            return &m_sampler;
        }

        bool is_compare_enabled() const
        {
            return m_compare_enable;
        }

        bool uses_unnormalized_coordinates() const
        {
            return m_use_unnormalized_coordinates;
        }

    private:
        /* Private functions */

        /* Please see create() for specification */
        Sampler(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                VkFilter                         in_mag_filter,
                VkFilter                         in_min_filter,
                VkSamplerMipmapMode              in_mipmap_mode,
                VkSamplerAddressMode             in_address_mode_u,
                VkSamplerAddressMode             in_address_mode_v,
                VkSamplerAddressMode             in_address_mode_w,
                float                            in_lod_bias,
                float                            in_max_anisotropy,
                bool                             in_compare_enable,
                VkCompareOp                      in_compare_op,
                float                            in_min_lod,
                float                            in_max_lod,
                VkBorderColor                    in_border_color,
                bool                             in_use_unnormalized_coordinates);

        Sampler           (const Sampler&);
        Sampler& operator=(const Sampler&);

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

        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        VkSampler                        m_sampler;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SAMPLER_H */