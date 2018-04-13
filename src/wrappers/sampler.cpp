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
#include "misc/sampler_create_info.h"
#include "misc/struct_chainer.h"
#include "wrappers/device.h"
#include "wrappers/physical_device.h"
#include "wrappers/sampler.h"

/** Please see header for specification */
Anvil::Sampler::Sampler(Anvil::SamplerCreateInfoUniquePtr in_create_info_ptr)
    :DebugMarkerSupportProvider(in_create_info_ptr->get_device(),
                                VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT),
     MTSafetySupportProvider   (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                in_create_info_ptr->get_device   () ))
{
    m_create_info_ptr = std::move(in_create_info_ptr);

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
        lock();
        {
            vkDestroySampler(m_device_ptr->get_device_vk(),
                             m_sampler,
                             nullptr /* pAllocator */);
        }
        unlock();

        m_sampler = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
Anvil::SamplerUniquePtr Anvil::Sampler::create(Anvil::SamplerCreateInfoUniquePtr in_create_info_ptr)
{
    SamplerUniquePtr result_ptr(nullptr,
                                std::default_delete<Sampler>() );

    result_ptr.reset(
        new Anvil::Sampler(std::move(in_create_info_ptr) )
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool Anvil::Sampler::init()
{
    VkResult                                  result         (VK_ERROR_INITIALIZATION_FAILED);
    Anvil::StructChainer<VkSamplerCreateInfo> struct_chainer;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Spawn a new sampler */
    {
        const auto          max_anisotropy      = m_create_info_ptr->get_max_anisotropy();
        VkSamplerCreateInfo sampler_create_info;

        sampler_create_info.addressModeU            = m_create_info_ptr->get_address_mode_u();
        sampler_create_info.addressModeV            = m_create_info_ptr->get_address_mode_v();
        sampler_create_info.addressModeW            = m_create_info_ptr->get_address_mode_w();
        sampler_create_info.anisotropyEnable        = (max_anisotropy > 1.0f) ? VK_TRUE : VK_FALSE;
        sampler_create_info.borderColor             = m_create_info_ptr->get_border_color();
        sampler_create_info.compareEnable           = m_create_info_ptr->is_compare_enabled() ? VK_TRUE : VK_FALSE;
        sampler_create_info.compareOp               = m_create_info_ptr->get_compare_op();
        sampler_create_info.flags                   = 0;
        sampler_create_info.magFilter               = m_create_info_ptr->get_mag_filter();
        sampler_create_info.maxAnisotropy           = max_anisotropy;
        sampler_create_info.maxLod                  = m_create_info_ptr->get_max_lod();
        sampler_create_info.minFilter               = m_create_info_ptr->get_min_filter();
        sampler_create_info.minLod                  = m_create_info_ptr->get_min_lod();
        sampler_create_info.mipLodBias              = m_create_info_ptr->get_lod_bias();
        sampler_create_info.mipmapMode              = m_create_info_ptr->get_mipmap_mode();
        sampler_create_info.pNext                   = nullptr;
        sampler_create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.unnormalizedCoordinates = m_create_info_ptr->uses_unnormalized_coordinates() ? VK_TRUE : VK_FALSE;

        struct_chainer.append_struct(sampler_create_info);
    }

    {
        auto chain_ptr = struct_chainer.create_chain();

        result = vkCreateSampler(m_device_ptr->get_device_vk(),
                                 chain_ptr->get_root_struct(),
                                 nullptr, /* pAllocator */
                                &m_sampler);
    }

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        set_vk_handle(m_sampler);
    }

    /* All done */
    return is_vk_call_successful(result);
}