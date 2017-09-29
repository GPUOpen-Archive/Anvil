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
#include "wrappers/physical_device.h"
#include "wrappers/sampler.h"

/** Please see header for specification */
Anvil::Sampler::Sampler(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
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
                        bool                             in_use_unnormalized_coordinates)
    :DebugMarkerSupportProvider    (in_device_ptr,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT),
     m_address_mode_u              (in_address_mode_u),
     m_address_mode_v              (in_address_mode_v),
     m_address_mode_w              (in_address_mode_w),
     m_border_color                (in_border_color),
     m_compare_enable              (in_compare_enable),
     m_compare_op                  (in_compare_op),
     m_device_ptr                  (in_device_ptr),
     m_lod_bias                    (in_lod_bias),
     m_mag_filter                  (in_mag_filter),
     m_max_anisotropy              (in_max_anisotropy),
     m_max_lod                     (in_max_lod),
     m_min_filter                  (in_min_filter),
     m_mipmap_mode                 (in_mipmap_mode),
     m_sampler                     (VK_NULL_HANDLE),
     m_use_unnormalized_coordinates(in_use_unnormalized_coordinates)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr  (m_device_ptr);
    VkSamplerCreateInfo                sampler_create_info;
    VkResult                           result             (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Spawn a new sampler */
    sampler_create_info.addressModeU            = in_address_mode_u;
    sampler_create_info.addressModeV            = in_address_mode_v;
    sampler_create_info.addressModeW            = in_address_mode_w;
    sampler_create_info.anisotropyEnable        = static_cast<VkBool32>((in_max_anisotropy > 1.0f) ? VK_TRUE : VK_FALSE);
    sampler_create_info.borderColor             = in_border_color;
    sampler_create_info.compareEnable           = static_cast<VkBool32>(in_compare_enable ? VK_TRUE : VK_FALSE);
    sampler_create_info.compareOp               = in_compare_op;
    sampler_create_info.flags                   = 0;
    sampler_create_info.magFilter               = in_mag_filter;
    sampler_create_info.maxAnisotropy           = in_max_anisotropy;
    sampler_create_info.maxLod                  = in_max_lod;
    sampler_create_info.minFilter               = in_min_filter;
    sampler_create_info.minLod                  = in_min_lod;
    sampler_create_info.mipLodBias              = in_lod_bias;
    sampler_create_info.mipmapMode              = in_mipmap_mode;
    sampler_create_info.pNext                   = nullptr;
    sampler_create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.unnormalizedCoordinates = static_cast<VkBool32>(in_use_unnormalized_coordinates ? VK_TRUE : VK_FALSE);

    result = vkCreateSampler(device_locked_ptr->get_device_vk(),
                            &sampler_create_info,
                             nullptr, /* pAllocator */
                            &m_sampler);

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        set_vk_handle(m_sampler);
    }

    /* Register the event instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_SAMPLER,
                                                  this);
}

/* Please see header for specification */
Anvil::Sampler::~Sampler()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_SAMPLER,
                                                    this);

    if (m_sampler != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroySampler(device_locked_ptr->get_device_vk(),
                         m_sampler,
                         nullptr /* pAllocator */);

        m_sampler = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
std::shared_ptr<Anvil::Sampler> Anvil::Sampler::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
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
                                                       bool                             in_use_unnormalized_coordinates)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(in_device_ptr);
    std::shared_ptr<Anvil::Sampler>    result_ptr;

    result_ptr.reset(
        new Anvil::Sampler(in_device_ptr,
                           in_mag_filter,
                           in_min_filter,
                           in_mipmap_mode,
                           in_address_mode_u,
                           in_address_mode_v,
                           in_address_mode_w,
                           in_lod_bias,
                           in_max_anisotropy,
                           in_compare_enable,
                           in_compare_op,
                           in_min_lod,
                           in_max_lod,
                           in_border_color,
                           in_use_unnormalized_coordinates)
    );

    return result_ptr;
}