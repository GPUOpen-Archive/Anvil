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

#include "misc/sampler_create_info.h"

Anvil::SamplerCreateInfoUniquePtr Anvil::SamplerCreateInfo::create(const Anvil::BaseDevice*  in_device_ptr,
                                                                   Anvil::Filter             in_mag_filter,
                                                                   Anvil::Filter             in_min_filter,
                                                                   Anvil::SamplerMipmapMode  in_mipmap_mode,
                                                                   Anvil::SamplerAddressMode in_address_mode_u,
                                                                   Anvil::SamplerAddressMode in_address_mode_v,
                                                                   Anvil::SamplerAddressMode in_address_mode_w,
                                                                   float                     in_lod_bias,
                                                                   float                     in_max_anisotropy,
                                                                   bool                      in_compare_enable,
                                                                   Anvil::CompareOp          in_compare_op,
                                                                   float                     in_min_lod,
                                                                   float                     in_max_lod,
                                                                   Anvil::BorderColor        in_border_color,
                                                                   bool                      in_use_unnormalized_coordinates,
                                                                   MTSafety                  in_mt_safety)
{
    Anvil::SamplerCreateInfoUniquePtr result_ptr;

    result_ptr.reset(
        new Anvil::SamplerCreateInfo(in_device_ptr,
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
                                     in_use_unnormalized_coordinates,
                                     in_mt_safety)
    );

    return result_ptr;
}

Anvil::SamplerCreateInfo::SamplerCreateInfo(const Anvil::BaseDevice*    in_device_ptr,
                                            Anvil::Filter               in_mag_filter,
                                            Anvil::Filter               in_min_filter,
                                            Anvil::SamplerMipmapMode    in_mipmap_mode,
                                            Anvil::SamplerAddressMode   in_address_mode_u,
                                            Anvil::SamplerAddressMode   in_address_mode_v,
                                            Anvil::SamplerAddressMode   in_address_mode_w,
                                            float                       in_lod_bias,
                                            float                       in_max_anisotropy,
                                            bool                        in_compare_enable,
                                            Anvil::CompareOp            in_compare_op,
                                            float                       in_min_lod,
                                            float                       in_max_lod,
                                            Anvil::BorderColor          in_border_color,
                                            bool                        in_use_unnormalized_coordinates,
                                            MTSafety                    in_mt_safety)
    :m_address_mode_u              (in_address_mode_u),
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
     m_min_lod                     (in_min_lod),
     m_mipmap_mode                 (in_mipmap_mode),
     m_mt_safety                   (in_mt_safety),
     m_sampler_reduction_mode      (Anvil::SamplerReductionMode::UNKNOWN),
     m_sampler_ycbcr_conversion_ptr(nullptr),
     m_use_unnormalized_coordinates(in_use_unnormalized_coordinates)
{
    /* Stub */
}
