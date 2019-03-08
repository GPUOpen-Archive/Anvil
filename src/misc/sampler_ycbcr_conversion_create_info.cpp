//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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

Anvil::SamplerYCbCrConversionCreateInfoUniquePtr Anvil::SamplerYCbCrConversionCreateInfo::create(const Anvil::BaseDevice*                  in_device_ptr,
                                                                                                 const Anvil::Format&                      in_format,
                                                                                                 const Anvil::SamplerYCbCrModelConversion& in_ycbcr_model_conversion,
                                                                                                 const Anvil::SamplerYCbCrRange&           in_ycbcr_range,
                                                                                                 const Anvil::ComponentMapping&            in_components,
                                                                                                 const Anvil::ChromaLocation&              in_x_chroma_offset,
                                                                                                 const Anvil::ChromaLocation&              in_y_chroma_offset,
                                                                                                 const Anvil::Filter&                      in_chroma_filter,
                                                                                                 const bool&                               in_should_force_explicit_reconstruction,
                                                                                                 MTSafety                                  in_mt_safety)
{
    Anvil::SamplerYCbCrConversionCreateInfoUniquePtr result_ptr;

    result_ptr.reset(
        new Anvil::SamplerYCbCrConversionCreateInfo(in_device_ptr,
                                                    in_format,
                                                    in_ycbcr_model_conversion,
                                                    in_ycbcr_range,
                                                    in_components,
                                                    in_x_chroma_offset,
                                                    in_y_chroma_offset,
                                                    in_chroma_filter,
                                                    in_should_force_explicit_reconstruction,
                                                    in_mt_safety)
    );

    anvil_assert(result_ptr != nullptr);
    return result_ptr;
}

Anvil::SamplerYCbCrConversionCreateInfo::SamplerYCbCrConversionCreateInfo(const Anvil::BaseDevice*                  in_device_ptr,
                                                                          const Anvil::Format&                      in_format,
                                                                          const Anvil::SamplerYCbCrModelConversion& in_ycbcr_model_conversion,
                                                                          const Anvil::SamplerYCbCrRange&           in_ycbcr_range,
                                                                          const Anvil::ComponentMapping&            in_components,
                                                                          const Anvil::ChromaLocation&              in_x_chroma_offset,
                                                                          const Anvil::ChromaLocation&              in_y_chroma_offset,
                                                                          const Anvil::Filter&                      in_chroma_filter,
                                                                          const bool&                               in_should_force_explicit_reconstruction,
                                                                          MTSafety                                  in_mt_safety)
    :m_chroma_filter                       (in_chroma_filter),
     m_components                          (in_components),
     m_device_ptr                          (in_device_ptr),
     m_format                              (in_format),
     m_mt_safety                           (in_mt_safety),
     m_should_force_explicit_reconstruction(in_should_force_explicit_reconstruction),
     m_x_chroma_offset                     (in_x_chroma_offset),
     m_y_chroma_offset                     (in_y_chroma_offset),
     m_ycbcr_model_conversion              (in_ycbcr_model_conversion),
     m_ycbcr_range                         (in_ycbcr_range)
{
    /* Stub */
}
