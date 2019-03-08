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
#ifndef MISC_SAMPLER_YCBCR_CONVERSION_CREATE_INFO_H
#define MISC_SAMPLER_YCBCR_CONVERSION_CREATE_INFO_H

#include "misc/types.h"

namespace Anvil
{
    class SamplerYCbCrConversionCreateInfo
    {
    public:
        /* Public functions */

        /** TODO
         *
         *  NOTE: This object can only be used for Vulkan devices with VK_KHR_sampler_ycbcr_conversion extension support.
         *
         *  For argument discussion, please consult Vulkan API specification.
         */
        static SamplerYCbCrConversionCreateInfoUniquePtr create(const Anvil::BaseDevice*                  in_device_ptr,
                                                                const Anvil::Format&                      in_format,
                                                                const Anvil::SamplerYCbCrModelConversion& in_ycbcr_model_conversion,
                                                                const Anvil::SamplerYCbCrRange&           in_ycbcr_range,
                                                                const Anvil::ComponentMapping&            in_components,
                                                                const Anvil::ChromaLocation&              in_x_chroma_offset,
                                                                const Anvil::ChromaLocation&              in_y_chroma_offset,
                                                                const Anvil::Filter&                      in_chroma_filter,
                                                                const bool&                               in_should_force_explicit_reconstruction,
                                                                MTSafety                                  in_mt_safety                            = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE);

        const Anvil::Filter& get_chroma_filter() const
        {
            return m_chroma_filter;
        }

        const Anvil::ComponentMapping& get_components() const
        {
            return m_components;
        }

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        const Anvil::Format& get_format() const
        {
            return m_format;
        }

        const Anvil::MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }

        const Anvil::SamplerYCbCrModelConversion& get_ycbcr_model_conversion() const
        {
            return m_ycbcr_model_conversion;
        }

        const Anvil::ChromaLocation& get_x_chroma_offset() const
        {
            return m_x_chroma_offset;
        }

        const Anvil::SamplerYCbCrRange& get_ycbcr_range() const
        {
            return m_ycbcr_range;
        }

        const Anvil::ChromaLocation& get_y_chroma_offset() const
        {
            return m_y_chroma_offset;
        }

        void set_chroma_filter(const Anvil::Filter& in_chroma_filter)
        {
            m_chroma_filter = in_chroma_filter;
        }

        void set_components(const Anvil::ComponentMapping& in_components)
        {
            m_components = in_components;
        }

        void set_device(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        void set_format(const Anvil::Format& in_format)
        {
            m_format = in_format;
        }

        void set_mt_safety(const Anvil::MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        void set_ycbcr_model_conversion(const Anvil::SamplerYCbCrModelConversion& in_conversion)
        {
            m_ycbcr_model_conversion = in_conversion;
        }

        void set_x_chroma_offset(const Anvil::ChromaLocation& in_x_chroma_offset)
        {
            m_x_chroma_offset = in_x_chroma_offset;
        }

        void set_ycbcr_range(const Anvil::SamplerYCbCrRange& in_range)
        {
            m_ycbcr_range = in_range;
        }

        void set_y_chroma_offset(const Anvil::ChromaLocation& in_y_chroma_offset)
        {
            m_y_chroma_offset = in_y_chroma_offset;
        }

        void set_should_force_explicit_reconstruction(const bool& in_should_force_explicit_reconstruction)
        {
            m_should_force_explicit_reconstruction = in_should_force_explicit_reconstruction;
        }

        const bool& should_force_explicit_reconstruction() const
        {
            return m_should_force_explicit_reconstruction;
        }

    private:
        /* Private functions */

        SamplerYCbCrConversionCreateInfo(const Anvil::BaseDevice*                  in_device_ptr,
                                         const Anvil::Format&                      in_format,
                                         const Anvil::SamplerYCbCrModelConversion& in_ycbcr_model_conversion,
                                         const Anvil::SamplerYCbCrRange&           in_ycbcr_range,
                                         const Anvil::ComponentMapping&            in_components,
                                         const Anvil::ChromaLocation&              in_x_chroma_offset,
                                         const Anvil::ChromaLocation&              in_y_chroma_offset,
                                         const Anvil::Filter&                      in_chroma_filter,
                                         const bool&                               in_should_force_explicit_reconstruction,
                                         MTSafety                                  in_mt_safety);

        /* Private variables */

        Anvil::Filter                      m_chroma_filter;
        Anvil::ComponentMapping            m_components;
        const Anvil::BaseDevice*           m_device_ptr;
        Anvil::Format                      m_format;
        MTSafety                           m_mt_safety;
        bool                               m_should_force_explicit_reconstruction;
        Anvil::ChromaLocation              m_x_chroma_offset;
        Anvil::ChromaLocation              m_y_chroma_offset;
        Anvil::SamplerYCbCrModelConversion m_ycbcr_model_conversion;
        Anvil::SamplerYCbCrRange           m_ycbcr_range;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(SamplerYCbCrConversionCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(SamplerYCbCrConversionCreateInfo);
    };
}; /* namespace Anvil */

#endif /* MISC_SAMPLER_YCBCR_CONVERSION_CREATE_INFO_H */