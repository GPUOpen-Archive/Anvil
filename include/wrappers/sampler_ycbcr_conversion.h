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
#ifndef WRAPPERS_SAMPLER_YCBCR_CONVERSION_H
#define WRAPPERS_SAMPLER_YCBCR_CONVERSION_H

#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    class SamplerYCbCrConversion : public DebugMarkerSupportProvider<SamplerYCbCrConversion>,
                                   public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        virtual ~SamplerYCbCrConversion();

        /** TODO
         *
         *  NOTE: This object can only be used for Vulkan devices with VK_KHR_sampler_ycbcr_conversion extension support.
         *
         *  For argument discussion, please consult Vulkan API specification.
         */
        static SamplerYCbCrConversionUniquePtr create(Anvil::SamplerYCbCrConversionCreateInfoUniquePtr in_create_info_ptr);

        const Anvil::SamplerYCbCrConversionCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        VkSamplerYcbcrConversion get_sampler_ycbcr_conversion_vk() const
        {
            return m_sampler_ycbcr_conversion_vk;
        }

    private:
        /* Private functions */

        SamplerYCbCrConversion(Anvil::SamplerYCbCrConversionCreateInfoUniquePtr in_create_info_ptr);

        bool init();

        /* Private variables */

        Anvil::SamplerYCbCrConversionCreateInfoUniquePtr m_create_info_ptr;
        VkSamplerYcbcrConversion                         m_sampler_ycbcr_conversion_vk;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(SamplerYCbCrConversion);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(SamplerYCbCrConversion);
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SAMPLER_YCBCR_CONVERSION_H */