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

/** Implements various function which provide detailed information about Vulkan formats. **/
#ifndef WRAPPERS_FORMATS_H
#define WRAPPERS_FORMATS_H

#include "misc/types.h"


namespace Anvil
{
    class Formats
    {
    public:
        /* Returns a list of formats compatible with @param in_format.
         *
         * The returned array includes @param in_format.
         *
         * @param in_format                      Format to use to determine compatibility class to use for the query.
         * @param out_n_compatible_formats_ptr   Deref will be set to the number of formats accessible under *out_compatible_formats_ptr_ptr.
         *                                       Must not be nullptr.
         * @param out_compatible_formats_ptr_ptr Deref will be set to a ptr to a linear list of formats compatible with @param in_format.
         *                                       The list MAY include YUV formats.
         *
         * @return true if successful, false otherwise.
         */
        static bool get_compatible_formats(Anvil::Format         in_format,
                                           uint32_t*             out_n_compatible_formats_ptr,
                                           const Anvil::Format** out_compatible_formats_ptr_ptr);

        static bool get_compressed_format_block_size(Anvil::Format in_format,
                                                     uint32_t*     out_block_size_uvec2_ptr,
                                                     uint32_t*     out_n_bytes_per_block_ptr);

        /** Returns an Anvil::Format which meets the user-specified criteria.
         *
         *  This function does not support block formats.
         *
         *  This function will only return one of the non-YUV formats whose component sizes
         *  match the specified number of bits.
         *
         *  For formats which use less than 4 components, set irrelevant n_component*_bits arguments
         *  to 0.
         *
         *  @param in_component_layout  Component layout the format must follow.
         *  @param in_format_type       Type of the format.
         *  @param in_n_component0_bits Number of bits used by the zeroth component. Must not be 0.
         *  @param in_n_component1_bits Number of bits used by the first component. Pass 0 if the component
         *                              is irrelevant.
         *  @param in_n_component2_bits Number of bits used by the second component. Pass 0 if the component
         *                              is irrelevant.
         *  @param in_n_component3_bits Number of bits used by the third component. Pass 0 if the component
         *                              is irrelevant.
         *
         *  @return A corresponding Anvil::Format value OR Anvil::Format::UNKNOWN if no recognized Vulkan format
         *          meets the specified requirements.
         */
        static Anvil::Format get_format(ComponentLayout in_component_layout,
                                        FormatType      in_format_type,
                                        uint32_t        in_n_component0_bits,
                                        uint32_t        in_n_component1_bits,
                                        uint32_t        in_n_component2_bits,
                                        uint32_t        in_n_component3_bits);

        /** Returns image aspects exposed by a given image format.
         *
         *  Supports both non-YUV and YUV formats.
         *
         *  @param in_format       Format to use for the query.
         *  @param out_aspects_ptr Deref will be used to store the result data. Must not be NULL.
         *
         *  @return true if successful, false otherwise.
         **/
        static bool get_format_aspects(Anvil::Format                         in_format,
                                       std::vector<Anvil::ImageAspectFlags>* out_aspects_ptr);

        /** Returns bit layout for the specified format.
         *
         *  NOTE: Only non-compressed non-YUV formats are supported.
         *  NOTE: Components not used by the specified format have start and end bit indices set to UINT32_MAX.
         *
         *  @param in_format                                     Non-compressed format to use for the query.
         *  @param out_opt_red_component_start_bit_index_ptr     If not null, deref will be set to the bit index, from which red component data starts.
         *  @param out_opt_red_component_end_bit_index_ptr       If not null, deref will be set to the bit index, at which red component data ends. (data under the bit includes the data!)
         *  @param out_opt_green_component_start_bit_index_ptr   If not null, deref will be set to the bit index, from which green component data starts.
         *  @param out_opt_green_component_end_bit_index_ptr     If not null, deref will be set to the bit index, at which green component data ends. (data under the bit includes the data!)
         *  @param out_opt_blue_component_start_bit_index_ptr    If not null, deref will be set to the bit index, from which blue component data starts.
         *  @param out_opt_blue_component_end_bit_index_ptr      If not null, deref will be set to the bit index, at which blue component data ends. (data under the bit includes the data!)
         *  @param out_opt_alpha_component_start_bit_index_ptr   If not null, deref will be set to the bit index, from which alpha component data starts.
         *  @param out_opt_alpha_component_end_bit_index_ptr     If not null, deref will be set to the bit index, at which alpha component data ends. (data under the bit includes the data!)
         *  @param out_opt_shared_component_start_bit_index_ptr  If not null, deref will be set to the bit index, from which shared component data starts.
         *  @param out_opt_shared_component_end_bit_index_ptr    If not null, deref will be set to the bit index, at which shared component data ends. (data under the bit includes the data!)
         *  @param out_opt_depth_component_start_bit_index_ptr   If not null, deref will be set to the bit index, from which depth component data starts.
         *  @param out_opt_depth_component_end_bit_index_ptr     If not null, deref will be set to the bit index, at which depth component data ends. (data under the bit includes the data!)
         *  @param out_opt_stencil_component_start_bit_index_ptr If not null, deref will be set to the bit index, from which stencil component data starts.
         *  @param out_opt_stencil_component_end_bit_index_ptr   If not null, deref will be set to the bit index, at which stencil component data ends. (data under the bit includes the data!)
         */
        static void get_format_bit_layout_nonyuv(Anvil::Format in_format,
                                                 uint32_t*     out_opt_red_component_start_bit_index_ptr     = nullptr,
                                                 uint32_t*     out_opt_red_component_end_bit_index_ptr       = nullptr,
                                                 uint32_t*     out_opt_green_component_start_bit_index_ptr   = nullptr,
                                                 uint32_t*     out_opt_green_component_end_bit_index_ptr     = nullptr,
                                                 uint32_t*     out_opt_blue_component_start_bit_index_ptr    = nullptr,
                                                 uint32_t*     out_opt_blue_component_end_bit_index_ptr      = nullptr,
                                                 uint32_t*     out_opt_alpha_component_start_bit_index_ptr   = nullptr,
                                                 uint32_t*     out_opt_alpha_component_end_bit_index_ptr     = nullptr,
                                                 uint32_t*     out_opt_shared_component_start_bit_index_ptr  = nullptr,
                                                 uint32_t*     out_opt_shared_component_end_bit_index_ptr    = nullptr,
                                                 uint32_t*     out_opt_depth_component_start_bit_index_ptr   = nullptr,
                                                 uint32_t*     out_opt_depth_component_end_bit_index_ptr     = nullptr,
                                                 uint32_t*     out_opt_stencil_component_start_bit_index_ptr = nullptr,
                                                 uint32_t*     out_opt_stencil_component_end_bit_index_ptr   = nullptr);

        /* Works analogously to get_format_bit_layout_nonyuv() but only supports YUV formats. */
        static void get_format_bit_layout_yuv(Anvil::Format in_format,
                                              uint32_t*     out_opt_plane0_r0_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane0_r0_end_bit_index_ptr   = nullptr,
                                              uint32_t*     out_opt_plane0_g0_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane0_g0_end_bit_index_ptr   = nullptr,
                                              uint32_t*     out_opt_plane0_b0_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane0_b0_end_bit_index_ptr   = nullptr,
                                              uint32_t*     out_opt_plane0_a0_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane0_a0_end_bit_index_ptr   = nullptr,
                                              uint32_t*     out_opt_plane0_g1_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane0_g1_end_bit_index_ptr   = nullptr,
                                              uint32_t*     out_opt_plane1_r0_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane1_r0_end_bit_index_ptr   = nullptr,
                                              uint32_t*     out_opt_plane1_g0_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane1_g0_end_bit_index_ptr   = nullptr,
                                              uint32_t*     out_opt_plane1_b0_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane1_b0_end_bit_index_ptr   = nullptr,
                                              uint32_t*     out_opt_plane2_r0_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane2_r0_end_bit_index_ptr   = nullptr,
                                              uint32_t*     out_opt_plane2_g0_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane2_g0_end_bit_index_ptr   = nullptr,
                                              uint32_t*     out_opt_plane2_b0_start_bit_index_ptr = nullptr,
                                              uint32_t*     out_opt_plane2_b0_end_bit_index_ptr   = nullptr);

        /** Tells what component layout is used by @param in_format.
         *
         *  NOTE: This function does NOT support YUV KHR format. Please check the overloaded function if
         *        you would like to use for YUV KHR format.
         */
        static ComponentLayout get_format_component_layout_nonyuv(Anvil::Format in_format);

        /** Tells what component layout is used by @param in_format at specified subresource @param in_aspect.
         *
         *  NOTE: Only YUV KHR formats are supported.
         */
        static ComponentLayout get_format_component_layout_yuv(Anvil::Format              in_format,
                                                               Anvil::ImageAspectFlagBits in_aspect);

        /** Tells the number of components used by @param in_format
         *
         *  NOTE: This function does NOT support YUV KHR formats.
         */
        static uint32_t get_format_n_components_nonyuv(Anvil::Format in_format);

        /** Tells the number of components used by @param in_format under specified subresource @param in_aspect.
         *
         *  NOTE: Only YUV KHR formats are supported.
         */
        static uint32_t get_format_n_components_yuv(Anvil::Format              in_format,
                                                    Anvil::ImageAspectFlagBits in_aspect);

        /* Tells the number of bits used for each component in case of Vulkan format specified
         * under @param in_format.
         *
         * NOTE: This function does NOT support YUV KHR format. Please check the overloaded function if
         *       you would like to use for YUV KHR format.
         * NOTE: Number of bits reported for each component uses ordering as reported for the format
         *       via get_format_component_layout(). This is especially important in the context of packed formats.
         *
         * @param in_format             Vulkan format to use for the query.
         * @param out_channel0_bits_ptr Deref will be set to the number of bits used for channel 0. Must
         *                              not be nullptr.
         * @param out_channel1_bits_ptr Deref will be set to the number of bits used for channel 1. Must
         *                              not be nullptr.
         * @param out_channel2_bits_ptr Deref will be set to the number of bits used for channel 2. Must
         *                              not be nullptr.
         * @param out_channel3_bits_ptr Deref will be set to the number of bits used for channel 3. Must
         *                              not be nullptr.
         */
        static void get_format_n_component_bits_nonyuv(Anvil::Format in_format,
                                                       uint32_t*     out_channel0_bits_ptr,
                                                       uint32_t*     out_channel1_bits_ptr,
                                                       uint32_t*     out_channel2_bits_ptr,
                                                       uint32_t*     out_channel3_bits_ptr);

        /* Tells the number of bits used for each component in case of Vulkan format specified
         * under @param in_format at subresource @param in_aspect.
         *
         * NOTE: Only YUV KHR fromats are supported.
         * NOTE: Number of bits reported for each component uses ordering as reported for the format
         *       via get_format_component_layout(). This is especially important in the context of packed formats.
         *
         * @param in_format             Vulkan format to use for the query.
         * @param in_aspect             Image aspect to use for the query.
         * @param out_channel0_bits_ptr Deref will be set to the number of bits used for channel 0. Must
         *                              not be nullptr.
         * @param out_channel1_bits_ptr Deref will be set to the number of bits used for channel 1. Must
         *                              not be nullptr.
         * @param out_channel2_bits_ptr Deref will be set to the number of bits used for channel 2. Must
         *                              not be nullptr.
         * @param out_channel3_bits_ptr Deref will be set to the number of bits used for channel 3. Must
         *                              not be nullptr.
         */
        static void get_format_n_component_bits_yuv(Anvil::Format              in_format,
                                                    Anvil::ImageAspectFlagBits in_aspect,
                                                    uint32_t*                  out_channel0_bits_ptr,
                                                    uint32_t*                  out_channel1_bits_ptr,
                                                    uint32_t*                  out_channel2_bits_ptr,
                                                    uint32_t*                  out_channel3_bits_ptr);

        /** Tells the number of planes exposed by the specified format.
         *
         *  For non-YUV formats, this function always returns 1.
         *
         *  @param in_format Format to use for the query.
         */
        static uint32_t get_format_n_planes(Anvil::Format in_format);

        /** Tells the number of bits unused for each component in case of Vulkan format specified
         *  under @param in_format at subresource @param in_aspect.
         *
         *  NOTE: Only YUV KHR formats are supported.
         *  NOTE: Number of bits reported for each component uses ordering as reported for the format
         *        via get_format_component_layout(). This is especially important in the context of packed formats.
         *
         * @param in_format                    Vulkan format to use for the query.
         * @param in_aspect                    Image aspect to use for the query.
         * @param out_channel0_unused_bits_ptr Deref will be set to the number of bits unused for channel 0. Must
         *                                     not be nullptr.
         * @param out_channel1_unused_bits_ptr Deref will be set to the number of bits unused for channel 1. Must
         *                                     not be nullptr.
         * @param out_channel2_unused_bits_ptr Deref will be set to the number of bits unused for channel 2. Must
         *                                     not be nullptr.
         * @param out_channel3_unused_bits_ptr Deref will be set to the number of bits unused for channel 3. Must
         *                                     not be nullptr.
         */
        static void get_format_n_unused_component_bits_yuv(Anvil::Format              in_format,
                                                           Anvil::ImageAspectFlagBits in_aspect,
                                                           uint32_t*                  out_channel0_unused_bits_ptr,
                                                           uint32_t*                  out_channel1_unused_bits_ptr,
                                                           uint32_t*                  out_channel2_unused_bits_ptr,
                                                           uint32_t*                  out_channel3_unused_bits_ptr);

        /* Returns a raw C string for specified format, or NULL if the format is unknown.
         *
         * Both non-YUV and YUV formats are supported.
         */
        static const char* get_format_name(Anvil::Format in_format);

        /** Tells the format type used by @param in_format.
         *
         * Both non-YUV and YUV formats are supported.
         */
        static FormatType get_format_type(Anvil::Format in_format);

        /** Returns the extent of subresource @param in_aspect with specified format @param in_format.
         *
         *  NOTE: Only YUV KHR formats are supported.
         *
         * @param in_format            VUlkan format.
         * @param in_aspect            Image aspect for specific subresource of the format.
         * @param in_image_extent      Image extent specified when creating image.
         * @param out_plane_extent_ptr Deref will be set to the extent of the specified subresource.
         *                             Must not be nullptr.
         */
        static void get_yuv_format_plane_extent(Anvil::Format              in_format,
                                                Anvil::ImageAspectFlagBits in_aspect,
                                                VkExtent3D                 in_image_extent,
                                                VkExtent3D*                out_plane_extent_ptr);

        /** Tells whether @param in_format includes a depth aspect.
         *
         *  NOTE: YUV KHR formats are NOT supported.
         */
        static bool has_depth_aspect(Anvil::Format in_format);

        /** Tells whether @param in_format includes a stencil aspect.
         *
         *  NOTE: YUV KHR formats are NOT supported.
         */
        static bool has_stencil_aspect(Anvil::Format in_format);

        /** Tells whether @param in_format format is a block format. */
        static bool is_format_compressed(Anvil::Format in_format);

        /** Tells whether @param in_format format is a multiplanar format. */
        static bool is_format_multiplanar(Anvil::Format in_format);

        /** Tells whether @param in_format is a KHR YUV format */
        static bool is_format_yuv_khr(Anvil::Format in_format);

        /** Tells whether @param in_format is a packed format.
         *
         * Both YUV and non-YUV formats are supported.
         **/
        static bool is_format_packed(Anvil::Format in_format);

    private:
        /** Returns the index of subresource info by given @param in_format and @param in_aspect.
         *
         *  NOTE: Only YUV KHR formats are supported.
         */
        static uint32_t get_yuv_format_plane_index(Anvil::Format              in_format,
                                                   Anvil::ImageAspectFlagBits in_aspect);

        /** Returns the number of subresources by given @param in_format.
         *
         *  NOTE: Only YUV KHR formats are supported.
         */
        static uint32_t get_yuv_format_n_planes(Anvil::Format in_format);
    };
};

#endif /* WRAPPERS_FORMATS_H */
