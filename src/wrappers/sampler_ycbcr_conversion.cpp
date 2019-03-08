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

#include "misc/sampler_ycbcr_conversion_create_info.h"
#include "wrappers/device.h"
#include "wrappers/sampler_ycbcr_conversion.h"

Anvil::SamplerYCbCrConversion::SamplerYCbCrConversion(Anvil::SamplerYCbCrConversionCreateInfoUniquePtr in_create_info_ptr)
    :Anvil::DebugMarkerSupportProvider<SamplerYCbCrConversion>(in_create_info_ptr->get_device(),
                                                               Anvil::ObjectType::SAMPLER_YCBCR_CONVERSION),
     Anvil::MTSafetySupportProvider                           (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                                               in_create_info_ptr->get_device   () )),
     m_sampler_ycbcr_conversion_vk                            (VK_NULL_HANDLE)
{
     m_create_info_ptr = std::move(in_create_info_ptr);
}

Anvil::SamplerYCbCrConversion::~SamplerYCbCrConversion()
{
    if (m_sampler_ycbcr_conversion_vk != VK_NULL_HANDLE)
    {
        auto  device_ptr  = m_create_info_ptr->get_device                                     ();
        auto& entrypoints = device_ptr->get_extension_khr_sampler_ycbcr_conversion_entrypoints();

        lock();
        {
            entrypoints.vkDestroySamplerYcbcrConversionKHR(device_ptr->get_device_vk(),
                                                           m_sampler_ycbcr_conversion_vk,
                                                           nullptr); /* pAllocator */
        }
        unlock();

        m_sampler_ycbcr_conversion_vk = VK_NULL_HANDLE;
    }
}

Anvil::SamplerYCbCrConversionUniquePtr Anvil::SamplerYCbCrConversion::create(Anvil::SamplerYCbCrConversionCreateInfoUniquePtr in_create_info_ptr)
{
    Anvil::SamplerYCbCrConversionUniquePtr result_ptr(nullptr,
                                                      std::default_delete<Anvil::SamplerYCbCrConversion>() );

    result_ptr.reset(
        new Anvil::SamplerYCbCrConversion(std::move(in_create_info_ptr) )
    );

    anvil_assert(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool Anvil::SamplerYCbCrConversion::init()
{
    VkSamplerYcbcrConversionCreateInfoKHR create_info;
    const auto&                           entrypoints = m_create_info_ptr->get_device()->get_extension_khr_sampler_ycbcr_conversion_entrypoints();
    bool                                  result      = false;

    create_info.chromaFilter                = static_cast<VkFilter>(m_create_info_ptr->get_chroma_filter() );
    create_info.components                  = m_create_info_ptr->get_components().get_vk                ();
    create_info.forceExplicitReconstruction = m_create_info_ptr->should_force_explicit_reconstruction   ();
    create_info.format                      = static_cast<VkFormat>(m_create_info_ptr->get_format       () );
    create_info.pNext                       = nullptr;
    create_info.sType                       = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO_KHR;
    create_info.xChromaOffset               = static_cast<VkChromaLocation>                (m_create_info_ptr->get_x_chroma_offset       () );
    create_info.ycbcrModel                  = static_cast<VkSamplerYcbcrModelConversionKHR>(m_create_info_ptr->get_ycbcr_model_conversion() );
    create_info.ycbcrRange                  = static_cast<VkSamplerYcbcrRangeKHR>          (m_create_info_ptr->get_ycbcr_range           () );
    create_info.yChromaOffset               = static_cast<VkChromaLocation>                (m_create_info_ptr->get_y_chroma_offset       () );

    if (is_vk_call_successful(entrypoints.vkCreateSamplerYcbcrConversionKHR(m_device_ptr->get_device_vk(),
                                                                           &create_info,
                                                                            nullptr, /* pAllocator */
                                                                           &m_sampler_ycbcr_conversion_vk) ))
    {
        result = true;

        set_vk_handle(m_sampler_ycbcr_conversion_vk);
    }

    return result;
}
