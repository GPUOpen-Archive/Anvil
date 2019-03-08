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
#ifndef MISC_IMAGE_VIEW_CREATE_INFO_H
#define MISC_IMAGE_VIEW_CREATE_INFO_H

#include "misc/types.h"


namespace Anvil
{
    class ImageViewCreateInfo
    {
    public:
        /* Public functions */

        /** Use this function if you need to create a single-sample 1D image view wrapper instance.
         *
         *  @param in_device_ptr          Device to use.
         *  @param in_image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                                object will be retained and release at ImageView release time.
         *  @param in_n_base_layer        Base layer index.
         *  @param in_n_base_mipmap_level Base mipmap level.
         *  @param in_n_mipmaps           Number of mipmaps to include in the view.
         *  @param in_aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param in_format              Image view format.
         *  @param in_swizzle_red         Channel to use for the red component when sampling the view.
         *  @param in_swizzle_green       Channel to use for the green component when sampling the view.
         *  @param in_swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param in_swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         * NOTE: Sampler YCbCr Conversion support is disabled by default. In order to enable it for the about-to-be-created
         *       image view, please call set_sampler_ycbcr_conversion_ptr().
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:                        Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         * - Sampler YCbCr Conversion support: disabled
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_1D(const Anvil::BaseDevice* in_device_ptr,
                                                             Image*                   in_image_ptr,
                                                             uint32_t                 in_n_base_layer,
                                                             uint32_t                 in_n_base_mipmap_level,
                                                             uint32_t                 in_n_mipmaps,
                                                             Anvil::ImageAspectFlags  in_aspect_mask,
                                                             Anvil::Format            in_format,
                                                             Anvil::ComponentSwizzle  in_swizzle_red,
                                                             Anvil::ComponentSwizzle  in_swizzle_green,
                                                             Anvil::ComponentSwizzle  in_swizzle_blue,
                                                             Anvil::ComponentSwizzle  in_swizzle_alpha);

        /** Use this function if you need to create a single-sample 1D array image view wrapper instance.
         *
         *  @param in_device_ptr          Device to use.
         *  @param in_image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                                object will be retained and release at ImageView release time.
         *  @param in_n_base_layer        Base layer index.
         *  @param in_n_layers            Number of layers to include in the view.
         *  @param in_n_base_mipmap_level Base mipmap level.
         *  @param in_n_mipmaps           Number of mipmaps to include in the view.
         *  @param in_aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param in_format              Image view format.
         *  @param in_swizzle_red         Channel to use for the red component when sampling the view.
         *  @param in_swizzle_green       Channel to use for the green component when sampling the view.
         *  @param in_swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param in_swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         * NOTE: Sampler YCbCr Conversion support is disabled by default. In order to enable it for the about-to-be-created
         *       image view, please call set_sampler_ycbcr_conversion_ptr().
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:                        Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         * - Sampler YCbCr Conversion support: disabled
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_1D_array(const Anvil::BaseDevice* in_device_ptr,
                                                                   Image*                   in_image_ptr,
                                                                   uint32_t                 in_n_base_layer,
                                                                   uint32_t                 in_n_layers,
                                                                   uint32_t                 in_n_base_mipmap_level,
                                                                   uint32_t                 in_n_mipmaps,
                                                                   Anvil::ImageAspectFlags  in_aspect_mask,
                                                                   Anvil::Format            in_format,
                                                                   Anvil::ComponentSwizzle  in_swizzle_red,
                                                                   Anvil::ComponentSwizzle  in_swizzle_green,
                                                                   Anvil::ComponentSwizzle  in_swizzle_blue,
                                                                   Anvil::ComponentSwizzle  in_swizzle_alpha);


        /** Use this function if you need to create a single-sample or a multi-sample 2D image view wrapper instance. The view will be
         *  single-sample if @param in_image_ptr uses 1 sample per texel, and multi-sample otherwise.
         *
         *  @param in_device_ptr          Device to use
         *  @param in_image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                                object will be retained and release at ImageView release time.
         *  @param in_n_base_layer        Base layer index.
         *  @param in_n_base_mipmap_level Base mipmap level.
         *  @param in_n_mipmaps           Number of mipmaps to include in the view.
         *  @param in_aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param in_format              Image view format.
         *  @param in_swizzle_red         Channel to use for the red component when sampling the view.
         *  @param in_swizzle_green       Channel to use for the green component when sampling the view.
         *  @param in_swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param in_swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         * NOTE: Sampler YCbCr Conversion support is disabled by default. In order to enable it for the about-to-be-created
         *       image view, please call set_sampler_ycbcr_conversion_ptr().
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:                        Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         * - Sampler YCbCr Conversion support: disabled
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_2D(const Anvil::BaseDevice* in_device_ptr,
                                                             Image*                   in_image_ptr,
                                                             uint32_t                 in_n_base_layer,
                                                             uint32_t                 in_n_base_mipmap_level,
                                                             uint32_t                 in_n_mipmaps,
                                                             Anvil::ImageAspectFlags  in_aspect_mask,
                                                             Anvil::Format            in_format,
                                                             Anvil::ComponentSwizzle  in_swizzle_red,
                                                             Anvil::ComponentSwizzle  in_swizzle_green,
                                                             Anvil::ComponentSwizzle  in_swizzle_blue,
                                                             Anvil::ComponentSwizzle  in_swizzle_alpha);

        /** Use this function if you need to create a single-sample or a multi-sample 2D array image view wrapper instance. The view will be
         *  single-sample if @param in_image_ptr uses 1 sample per texel, and multi-sample otherwise.
         *
         *  @param in_device_ptr          Device to use.
         *  @param in_image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                                object will be retained and release at ImageView release time.
         *  @param in_n_base_layer        Base layer index.
         *  @param in_n_layers            Number of layers to include in the view.
         *  @param in_n_base_mipmap_level Base mipmap level.
         *  @param in_n_mipmaps           Number of mipmaps to include in the view.
         *  @param in_aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param in_format              Image view format.
         *  @param in_swizzle_red         Channel to use for the red component when sampling the view.
         *  @param in_swizzle_green       Channel to use for the green component when sampling the view.
         *  @param in_swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param in_swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         * NOTE: Sampler YCbCr Conversion support is disabled by default. In order to enable it for the about-to-be-created
         *       image view, please call set_sampler_ycbcr_conversion_ptr().
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:                        Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         * - Sampler YCbCr Conversion support: disabled
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_2D_array(const Anvil::BaseDevice* in_device_ptr,
                                                                   Anvil::Image*            in_image_ptr,
                                                                   uint32_t                 in_n_base_layer,
                                                                   uint32_t                 in_n_layers,
                                                                   uint32_t                 in_n_base_mipmap_level,
                                                                   uint32_t                 in_n_mipmaps,
                                                                   Anvil::ImageAspectFlags  in_aspect_mask,
                                                                   Anvil::Format            in_format,
                                                                   Anvil::ComponentSwizzle  in_swizzle_red,
                                                                   Anvil::ComponentSwizzle  in_swizzle_green,
                                                                   Anvil::ComponentSwizzle  in_swizzle_blue,
                                                                   Anvil::ComponentSwizzle  in_swizzle_alpha);

        /** Use this function if you need to create a single-sample 3D image view wrapper instance.
         *
         *  @param in_device_ptr          Device to use.
         *  @param in_image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                                object will be retained and release at ImageView release time.
         *  @param in_n_base_mipmap_level Base mipmap level.
         *  @param in_n_mipmaps           Number of mipmaps to include in the view.
         *  @param in_aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param in_format              Image view format.
         *  @param in_swizzle_red         Channel to use for the red component when sampling the view.
         *  @param in_swizzle_green       Channel to use for the green component when sampling the view.
         *  @param in_swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param in_swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         * NOTE: Sampler YCbCr Conversion support is disabled by default. In order to enable it for the about-to-be-created
         *       image view, please call set_sampler_ycbcr_conversion_ptr().
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:                        Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         * - Sampler YCbCr Conversion support: disabled
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_3D(const Anvil::BaseDevice* in_device_ptr,
                                                             Image*                   in_image_ptr,
                                                             uint32_t                 in_n_base_mipmap_level,
                                                             uint32_t                 in_n_mipmaps,
                                                             Anvil::ImageAspectFlags  in_aspect_mask,
                                                             Anvil::Format            in_format,
                                                             Anvil::ComponentSwizzle  in_swizzle_red,
                                                             Anvil::ComponentSwizzle  in_swizzle_green,
                                                             Anvil::ComponentSwizzle  in_swizzle_blue,
                                                             Anvil::ComponentSwizzle  in_swizzle_alpha);

        /** Use this function if you need to create a cube-map image view wrapper instance.
         *
         *  @param in_device_ptr          Device to use.
         *  @param in_image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                                object will be retained and release at ImageView release time.
         *  @param in_n_base_layer        Base layer index.
         *  @param in_n_base_mipmap_level Base mipmap level.
         *  @param in_n_mipmaps           Number of mipmaps to include in the view.
         *  @param in_aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param in_format              Image view format.
         *  @param in_swizzle_red         Channel to use for the red component when sampling the view.
         *  @param in_swizzle_green       Channel to use for the green component when sampling the view.
         *  @param in_swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param in_swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         * NOTE: Sampler YCbCr Conversion support is disabled by default. In order to enable it for the about-to-be-created
         *       image view, please call set_sampler_ycbcr_conversion_ptr().
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:                        Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         * - Sampler YCbCr Conversion support: disabled
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_cube_map(const Anvil::BaseDevice* in_device_ptr,
                                                                   Anvil::Image*            in_image_ptr,
                                                                   uint32_t                 in_n_base_layer,
                                                                   uint32_t                 in_n_base_mipmap_level,
                                                                   uint32_t                 in_n_mipmaps,
                                                                   Anvil::ImageAspectFlags  in_aspect_mask,
                                                                   Anvil::Format            in_format,
                                                                   Anvil::ComponentSwizzle  in_swizzle_red,
                                                                   Anvil::ComponentSwizzle  in_swizzle_green,
                                                                   Anvil::ComponentSwizzle  in_swizzle_blue,
                                                                   Anvil::ComponentSwizzle  in_swizzle_alpha);

        /** Use this function if you need to create a cube-map array image view wrapper instance.
         *
         *  @param in_device_ptr          Device to use.
         *  @param in_image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                                object will be retained and release at ImageView release time.
         *  @param in_n_base_layer        Base layer index.
         *  @param in_n_cube_maps         Number of cube-maps to include in the view. The number of layers created
         *                                for the view will be equal to @param in_n_cube_maps * 6.
         *  @param in_n_base_mipmap_level Base mipmap level.
         *  @param in_n_mipmaps           Number of mipmaps to include in the view.
         *  @param in_aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param in_format              Image view format.
         *  @param in_swizzle_red         Channel to use for the red component when sampling the view.
         *  @param in_swizzle_green       Channel to use for the green component when sampling the view.
         *  @param in_swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param in_swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         * NOTE: Sampler YCbCr Conversion support is disabled by default. In order to enable it for the about-to-be-created
         *       image view, please call set_sampler_ycbcr_conversion_ptr().
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:                        Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         * - Sampler YCbCr Conversion support: disabled
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_cube_map_array(const Anvil::BaseDevice* in_device_ptr,
                                                                         Anvil::Image*            in_image_ptr,
                                                                         uint32_t                 in_n_base_layer,
                                                                         uint32_t                 in_n_cube_maps,
                                                                         uint32_t                 in_n_base_mipmap_level,
                                                                         uint32_t                 in_n_mipmaps,
                                                                         Anvil::ImageAspectFlags  in_aspect_mask,
                                                                         Anvil::Format            in_format,
                                                                         Anvil::ComponentSwizzle  in_swizzle_red,
                                                                         Anvil::ComponentSwizzle  in_swizzle_green,
                                                                         Anvil::ComponentSwizzle  in_swizzle_blue,
                                                                         Anvil::ComponentSwizzle  in_swizzle_alpha);

        /* Returns the aspect assigned to the image view */
        Anvil::ImageAspectFlags get_aspect() const
        {
            return m_aspect_mask;
        }

        /* Returns base layer index used by the image view */
        uint32_t get_base_layer() const
        {
            return m_n_base_layer;
        }

        /* Returns base mip level used by the image view */
        uint32_t get_base_mipmap_level() const
        {
            return m_n_base_mipmap_level;
        }

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        /** Returns image view's format */
        Anvil::Format get_format() const
        {
            return m_format;
        }

        const Anvil::MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }

        /** Returns number of layers encapsulated by the image view */
        uint32_t get_n_layers() const
        {
            return m_n_layers;
        }

        /** Returns number of mipmaps encapsulated by the image view */
        uint32_t get_n_mipmaps() const
        {
            return m_n_mipmaps;
        }

        /** Returns a pointer to the parent image, from which the image view has been created. */
        Anvil::Image* get_parent_image() const
        {
            return m_parent_image_ptr;
        }

        const Anvil::SamplerYCbCrConversion* get_sampler_ycbcr_conversion_ptr() const
        {
            return m_sampler_ycbcr_conversion_ptr;
        }

        /** Returns swizzle array assigned to the image view */
        const std::array<Anvil::ComponentSwizzle, 4>& get_swizzle_array() const
        {
            return m_swizzle_array;
        }

        /** Returns image view type of the image view instance */
        Anvil::ImageViewType get_type() const
        {
            return m_type;
        }

        /** Returns usage flags associated with the image view.
         *
         *  NOTE: If the function returns Anvil::ImageUsageFlagBits::NONE, the image view inherits usage bits
         *        from the parent image.
         *
         */
        const Anvil::ImageUsageFlags& get_usage() const
        {
            return m_usage;
        }

        void set_aspect(const Anvil::ImageAspectFlags& in_aspect)
        {
            m_aspect_mask = in_aspect;
        }

        void set_base_layer(const uint32_t& in_n_base_layer)
        {
            m_n_base_layer = in_n_base_layer;
        }

        void set_base_mipmap_level(const uint32_t& in_n_base_mipmap_level)
        {
            m_n_base_mipmap_level = in_n_base_mipmap_level;
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

        void set_n_layers(const uint32_t& in_n_layers)
        {
            m_n_layers = in_n_layers;
        }

        void set_n_mipmaps(const uint32_t& in_n_mipmaps)
        {
            m_n_mipmaps = in_n_mipmaps;
        }

        void set_parent_image(Anvil::Image* in_parent_image_ptr)
        {
            m_parent_image_ptr = in_parent_image_ptr;
        }

        /* Attaches or detaches already attached SamplerYCBCRConversion object from the create info struct.
         * This information will be used at image view creation time.
         *
         * NOTE: Requires VK_KHR_sampler_ycbcr_conversion
         *
         * @param in_sampler_ycbcr_conversion_ptr If not nullptr, the specified object will be passed to the implementation
         *                                        at sampler creation time by chaining VkSamplerYcbcrConversionInfo struct.
         *                                        If nullptr, the struct will not be chained.
         **/
        void set_sampler_ycbcr_conversion_ptr(const Anvil::SamplerYCbCrConversion* in_sampler_ycbcr_conversion_ptr)
        {
            m_sampler_ycbcr_conversion_ptr = in_sampler_ycbcr_conversion_ptr;
        }

        void set_swizzle_array(const std::array<Anvil::ComponentSwizzle, 4>& in_swizzle_array)
        {
            m_swizzle_array = in_swizzle_array;
        }

        /* By default, image views inherit usage flags from the parent image. You can use this setter function to override
         * the default behavior with a subset of parent image's usage flags.
         *
         * Requires VK_KHR_maintenance2.
         */
        void set_usage(const Anvil::ImageUsageFlags& in_usage)
        {
            m_usage = in_usage;
        }

    private:

        /* Private functions */
        ImageViewCreateInfo(const Anvil::ImageAspectFlags& in_aspect_mask,
                            const Anvil::BaseDevice*       in_device_ptr,
                            const Anvil::Format            in_format,
                            const uint32_t                 in_n_base_layer,
                            const uint32_t                 in_n_base_mipmap_level,
                            const uint32_t                 in_n_layers,
                            const uint32_t                 in_n_mipmaps,
                            Anvil::Image*                  in_parent_image_ptr,
                            const Anvil::ComponentSwizzle* in_swizzle_array_ptr,
                            const Anvil::ImageViewType     in_type,
                            const Anvil::MTSafety&         in_mt_safety);

        /* Private variables */

        Anvil::ImageAspectFlags m_aspect_mask;

        const Anvil::BaseDevice*               m_device_ptr;
        Anvil::Format                          m_format;
        Anvil::MTSafety                        m_mt_safety;
        uint32_t                               m_n_base_layer;
        uint32_t                               m_n_base_mipmap_level;
        uint32_t                               m_n_layers;
        uint32_t                               m_n_mipmaps;
        Anvil::Image*                          m_parent_image_ptr;
        const Anvil::SamplerYCbCrConversion*   m_sampler_ycbcr_conversion_ptr;
        std::array<Anvil::ComponentSwizzle, 4> m_swizzle_array;
        Anvil::ImageViewType                   m_type;
        Anvil::ImageUsageFlags                 m_usage;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(ImageViewCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(ImageViewCreateInfo);
    };

}; /* namespace Anvil */

#endif/* MISC_IMAGE_VIEW_CREATE_INFO_H */
