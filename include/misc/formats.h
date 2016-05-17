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

/** Implements various function which provide detailed information about Vulkan formats. **/
#ifndef WRAPPERS_FORMATS_H
#define WRAPPERS_FORMATS_H

#include "misc/types.h"


namespace Anvil
{
    class Formats
    {
    public:
        /** Returns a VkFormat which meets the user-specified criteria.
         *
         *  This function does not support block formats.
         *
         *  For formats which use less than 4 components, set irrelevant n_component*_bits arguments
         *  to 0.
         *
         *  @param component_layout  Component layout the format must follow.
         *  @param format_type       Type of the format.
         *  @param n_component0_bits Number of bits used by the zeroth component. Must not be 0.
         *  @param n_component1_bits Number of bits used by the first component. Pass 0 if the component
         *                           is irrelevant.
         *  @param n_component2_bits Number of bits used by the second component. Pass 0 if the component
         *                           is irrelevant.
         *  @param n_component3_bits Number of bits used by the third component. Pass 0 if the component
         *                           is irrelevant.
         *
         *  @return A corresponding VkFormat value OR VK_FORMAT_UNKNOWN if no recognized Vulkan format
         *          meets the specified requirements.
         */
        static VkFormat get_format(ComponentLayout component_layout,
                                   FormatType      format_type,
                                   uint32_t        n_component0_bits,
                                   uint32_t        n_component1_bits,
                                   uint32_t        n_component2_bits,
                                   uint32_t        n_component3_bits);

        /** Tells what component layout is used by @param format. */
        static ComponentLayout get_format_component_layout(VkFormat format);

        /** Tells the number of components used by @param format */
        static uint32_t get_format_n_components(VkFormat format);

        /* Tells the number of bits used for each component in case of Vulkan format specified
         * under @param format.
         *
         * @param format                Vulkan format to use for the query.
         * @param out_channel0_bits_ptr Deref will be set to the number of bits used for channel 0. Must
         *                              not be nullptr.
         * @param out_channel1_bits_ptr Deref will be set to the number of bits used for channel 1. Must
         *                              not be nullptr.
         * @param out_channel2_bits_ptr Deref will be set to the number of bits used for channel 2. Must
         *                              not be nullptr.
         * @param out_channel3_bits_ptr Deref will be set to the number of bits used for channel 3. Must
         *                              not be nullptr.
         */
        static void get_format_n_component_bits(VkFormat  format,
                                                uint32_t* out_channel0_bits_ptr,
                                                uint32_t* out_channel1_bits_ptr,
                                                uint32_t* out_channel2_bits_ptr,
                                                uint32_t* out_channel3_bits_ptr);

        /** Tells the format type used by @param format. */
        static FormatType get_format_type(VkFormat format);

        /** Tells whether @param format format is a block format. */
        static bool is_format_compressed(VkFormat format);
    };
};

#endif /* WRAPPERS_FORMATS_H */
