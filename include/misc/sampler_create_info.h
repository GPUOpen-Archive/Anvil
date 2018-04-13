//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef MISC_SAMPLER_CREATE_INFO_H
#define MISC_SAMPLER_CREATE_INFO_H

#include "misc/types.h"

namespace Anvil
{
    class SamplerCreateInfo
    {
    public:
        /* Public functions */

        /** TODO
         *
         *  For argument discussion, please consult Vulkan API specification.
         */
        static SamplerCreateInfoUniquePtr create(const Anvil::BaseDevice* in_device_ptr,
                                                 VkFilter                 in_mag_filter,
                                                 VkFilter                 in_min_filter,
                                                 VkSamplerMipmapMode      in_mipmap_mode,
                                                 VkSamplerAddressMode     in_address_mode_u,
                                                 VkSamplerAddressMode     in_address_mode_v,
                                                 VkSamplerAddressMode     in_address_mode_w,
                                                 float                    in_lod_bias,
                                                 float                    in_max_anisotropy,
                                                 bool                     in_compare_enable,
                                                 VkCompareOp              in_compare_op,
                                                 float                    in_min_lod,
                                                 float                    in_max_lod,
                                                 VkBorderColor            in_border_color,
                                                 bool                     in_use_unnormalized_coordinates,
                                                 MTSafety                 in_mt_safety           = MT_SAFETY_INHERIT_FROM_PARENT_DEVICE);

        const VkSamplerAddressMode& get_address_mode_u() const
        {
            return m_address_mode_u;
        }

        const VkSamplerAddressMode& get_address_mode_v() const
        {
            return m_address_mode_v;
        }

        const VkSamplerAddressMode& get_address_mode_w() const
        {
            return m_address_mode_w;
        }

        const VkBorderColor& get_border_color() const
        {
            return m_border_color;
        }

        const VkCompareOp& get_compare_op() const
        {
            return m_compare_op;
        }

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        const float& get_lod_bias() const
        {
            return m_lod_bias;
        }

        const VkFilter& get_mag_filter() const
        {
            return m_mag_filter;
        }

        const float& get_max_anisotropy() const
        {
            return m_max_anisotropy;
        }

        const float& get_max_lod() const
        {
            return m_max_lod;
        }

        const VkFilter& get_min_filter() const
        {
            return m_min_filter;
        }

        const float& get_min_lod() const
        {
            return m_min_lod;
        }

        const VkSamplerMipmapMode& get_mipmap_mode() const
        {
            return m_mipmap_mode;
        }

        const Anvil::MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }

        const bool& is_compare_enabled() const
        {
            return m_compare_enable;
        }

        void set_address_mode_u(const VkSamplerAddressMode& in_address_mode_u)
        {
            m_address_mode_u = in_address_mode_u;
        }

        void set_address_mode_v(const VkSamplerAddressMode& in_address_mode_v)
        {
            m_address_mode_v = in_address_mode_v;
        }

        void set_address_mode_w(const VkSamplerAddressMode& in_address_mode_w)
        {
            m_address_mode_w = in_address_mode_w;
        }

        void set_border_color(const VkBorderColor& in_border_color)
        {
            m_border_color = in_border_color;
        }

        void set_compare_op(const VkCompareOp& in_compare_op)
        {
            m_compare_op = in_compare_op;
        }

        void set_device(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        void set_lod_bias(const float& in_lod_bias)
        {
            m_lod_bias = in_lod_bias;
        }

        void set_mag_filter(const VkFilter& in_mag_filter)
        {
            m_mag_filter = in_mag_filter;
        }

        void set_max_anisotropy(const float& in_max_anisotropy)
        {
            m_max_anisotropy = in_max_anisotropy;
        }

        void set_max_lod(const float& in_max_lod)
        {
            m_max_lod = in_max_lod;
        }

        void set_min_filter(const VkFilter& in_min_filter)
        {
            m_min_filter = in_min_filter;
        }

        void set_min_lod(const float& in_min_lod)
        {
            m_min_lod = in_min_lod;
        }

        void set_mipmap_mode(const VkSamplerMipmapMode& in_mipmap_mode)
        {
            m_mipmap_mode = in_mipmap_mode;
        }

        void set_mt_safety(const Anvil::MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        void set_is_compare_enabled(const bool& in_compare_enable)
        {
            m_compare_enable = in_compare_enable;
        }

        void set_uses_unnormalized_coordinates(const bool& in_use_unnormalized_coordinates)
        {
            m_use_unnormalized_coordinates = in_use_unnormalized_coordinates;
        }

        const bool& uses_unnormalized_coordinates() const
        {
            return m_use_unnormalized_coordinates;
        }

    private:
        /* Private functions */

        SamplerCreateInfo(const Anvil::BaseDevice* in_device_ptr,
                          VkFilter                 in_mag_filter,
                          VkFilter                 in_min_filter,
                          VkSamplerMipmapMode      in_mipmap_mode,
                          VkSamplerAddressMode     in_address_mode_u,
                          VkSamplerAddressMode     in_address_mode_v,
                          VkSamplerAddressMode     in_address_mode_w,
                          float                    in_lod_bias,
                          float                    in_max_anisotropy,
                          bool                     in_compare_enable,
                          VkCompareOp              in_compare_op,
                          float                    in_min_lod,
                          float                    in_max_lod,
                          VkBorderColor            in_border_color,
                          bool                     in_use_unnormalized_coordinates,
                          MTSafety                 in_mt_safety);

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
        Anvil::MTSafety      m_mt_safety;
        bool                 m_use_unnormalized_coordinates;

        const Anvil::BaseDevice* m_device_ptr;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(SamplerCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(SamplerCreateInfo);
    };
}; /* namespace Anvil */

#endif /* MISC_SAMPLER_CREATE_INFO_H */