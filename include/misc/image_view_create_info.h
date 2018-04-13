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
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:   MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_1D(const Anvil::BaseDevice* in_device_ptr,
                                                             Image*                   in_image_ptr,
                                                             uint32_t                 in_n_base_layer,
                                                             uint32_t                 in_n_base_mipmap_level,
                                                             uint32_t                 in_n_mipmaps,
                                                             VkImageAspectFlags       in_aspect_mask,
                                                             VkFormat                 in_format,
                                                             VkComponentSwizzle       in_swizzle_red,
                                                             VkComponentSwizzle       in_swizzle_green,
                                                             VkComponentSwizzle       in_swizzle_blue,
                                                             VkComponentSwizzle       in_swizzle_alpha);

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
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:   MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_1D_array(const Anvil::BaseDevice* in_device_ptr,
                                                                   Image*                   in_image_ptr,
                                                                   uint32_t                 in_n_base_layer,
                                                                   uint32_t                 in_n_layers,
                                                                   uint32_t                 in_n_base_mipmap_level,
                                                                   uint32_t                 in_n_mipmaps,
                                                                   VkImageAspectFlags       in_aspect_mask,
                                                                   VkFormat                 in_format,
                                                                   VkComponentSwizzle       in_swizzle_red,
                                                                   VkComponentSwizzle       in_swizzle_green,
                                                                   VkComponentSwizzle       in_swizzle_blue,
                                                                   VkComponentSwizzle       in_swizzle_alpha);


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
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:   MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_2D(const Anvil::BaseDevice* in_device_ptr,
                                                             Image*                   in_image_ptr,
                                                             uint32_t                 in_n_base_layer,
                                                             uint32_t                 in_n_base_mipmap_level,
                                                             uint32_t                 in_n_mipmaps,
                                                             VkImageAspectFlags       in_aspect_mask,
                                                             VkFormat                 in_format,
                                                             VkComponentSwizzle       in_swizzle_red,
                                                             VkComponentSwizzle       in_swizzle_green,
                                                             VkComponentSwizzle       in_swizzle_blue,
                                                             VkComponentSwizzle       in_swizzle_alpha);

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
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:   MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_2D_array(const Anvil::BaseDevice* in_device_ptr,
                                                                   Anvil::Image*            in_image_ptr,
                                                                   uint32_t                 in_n_base_layer,
                                                                   uint32_t                 in_n_layers,
                                                                   uint32_t                 in_n_base_mipmap_level,
                                                                   uint32_t                 in_n_mipmaps,
                                                                   VkImageAspectFlags       in_aspect_mask,
                                                                   VkFormat                 in_format,
                                                                   VkComponentSwizzle       in_swizzle_red,
                                                                   VkComponentSwizzle       in_swizzle_green,
                                                                   VkComponentSwizzle       in_swizzle_blue,
                                                                   VkComponentSwizzle       in_swizzle_alpha);

        /** Use this function if you need to create a single-sample 3D image view wrapper instance.
         *
         *  @param in_device_ptr          Device to use.
         *  @param in_image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                                object will be retained and release at ImageView release time.
         *  @param in_n_base_slice        Base slice index.
         *  @param in_n_slices            Number of slices to include in the view.
         *  @param in_n_base_mipmap_level Base mipmap level.
         *  @param in_n_mipmaps           Number of mipmaps to include in the view.
         *  @param in_aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param in_format              Image view format.
         *  @param in_swizzle_red         Channel to use for the red component when sampling the view.
         *  @param in_swizzle_green       Channel to use for the green component when sampling the view.
         *  @param in_swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param in_swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:   MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_3D(const Anvil::BaseDevice* in_device_ptr,
                                                             Image*                   in_image_ptr,
                                                             uint32_t                 in_n_base_slice,
                                                             uint32_t                 in_n_slices,
                                                             uint32_t                 in_n_base_mipmap_level,
                                                             uint32_t                 in_n_mipmaps,
                                                             VkImageAspectFlags       in_aspect_mask,
                                                             VkFormat                 in_format,
                                                             VkComponentSwizzle       in_swizzle_red,
                                                             VkComponentSwizzle       in_swizzle_green,
                                                             VkComponentSwizzle       in_swizzle_blue,
                                                             VkComponentSwizzle       in_swizzle_alpha);

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
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:   MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_cube_map(const Anvil::BaseDevice* in_device_ptr,
                                                                   Anvil::Image*            in_image_ptr,
                                                                   uint32_t                 in_n_base_layer,
                                                                   uint32_t                 in_n_base_mipmap_level,
                                                                   uint32_t                 in_n_mipmaps,
                                                                   VkImageAspectFlags       in_aspect_mask,
                                                                   VkFormat                 in_format,
                                                                   VkComponentSwizzle       in_swizzle_red,
                                                                   VkComponentSwizzle       in_swizzle_green,
                                                                   VkComponentSwizzle       in_swizzle_blue,
                                                                   VkComponentSwizzle       in_swizzle_alpha);

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
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety:   MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static Anvil::ImageViewCreateInfoUniquePtr create_cube_map_array(const Anvil::BaseDevice* in_device_ptr,
                                                                         Anvil::Image*            in_image_ptr,
                                                                         uint32_t                 in_n_base_layer,
                                                                         uint32_t                 in_n_cube_maps,
                                                                         uint32_t                 in_n_base_mipmap_level,
                                                                         uint32_t                 in_n_mipmaps,
                                                                         VkImageAspectFlags       in_aspect_mask,
                                                                         VkFormat                 in_format,
                                                                         VkComponentSwizzle       in_swizzle_red,
                                                                         VkComponentSwizzle       in_swizzle_green,
                                                                         VkComponentSwizzle       in_swizzle_blue,
                                                                         VkComponentSwizzle       in_swizzle_alpha);

        /* Returns the aspect assigned to the image view */
        VkImageAspectFlags get_aspect() const
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
        const VkFormat get_format() const
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

        /** Returns number of slices encapsulated by the image view */
        uint32_t get_n_slices() const
        {
            return m_n_slices;
        }

        /** Returns a pointer to the parent image, from which the image view has been created. */
        Anvil::Image* get_parent_image() const
        {
            return m_parent_image_ptr;
        }

        /** Returns swizzle array assigned to the image view */
        const std::array<VkComponentSwizzle, 4>& get_swizzle_array() const
        {
            return m_swizzle_array;
        }

        /** Returns image view type of the image view instance */
        const VkImageViewType get_type() const
        {
            return m_type;
        }

        void set_aspect(const VkImageAspectFlags& in_aspect)
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

        void set_format(const VkFormat& in_format)
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

        void set_n_slices(const uint32_t& in_n_slices)
        {
            m_n_slices = in_n_slices;
        }

        void set_parent_image(Anvil::Image* in_parent_image_ptr)
        {
            m_parent_image_ptr = in_parent_image_ptr;
        }

        void set_swizzle_array(const std::array<VkComponentSwizzle, 4>& in_swizzle_array)
        {
            m_swizzle_array = in_swizzle_array;
        }

    private:

        /* Private functions */
        ImageViewCreateInfo(const VkImageAspectFlags& in_aspect_mask,
                            const Anvil::BaseDevice*  in_device_ptr,
                            const VkFormat            in_format,
                            const uint32_t            in_n_base_layer,
                            const uint32_t            in_n_base_mipmap_level,
                            const uint32_t            in_n_layers,
                            const uint32_t            in_n_mipmaps,
                            const uint32_t            in_n_slices,
                            Anvil::Image*             in_parent_image_ptr,
                            const VkComponentSwizzle* in_swizzle_array_ptr,
                            const VkImageViewType     in_type,
                            const Anvil::MTSafety&    in_mt_safety);

        /* Private variables */

        VkImageAspectFlagsVariable(m_aspect_mask);

        const Anvil::BaseDevice*          m_device_ptr;
        VkFormat                          m_format;
        Anvil::MTSafety                   m_mt_safety;
        uint32_t                          m_n_base_layer;
        uint32_t                          m_n_base_mipmap_level;
        uint32_t                          m_n_layers;
        uint32_t                          m_n_mipmaps;
        uint32_t                          m_n_slices;
        Anvil::Image*                     m_parent_image_ptr;
        std::array<VkComponentSwizzle, 4> m_swizzle_array;
        const VkImageViewType             m_type;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(ImageViewCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(ImageViewCreateInfo);
    };

}; /* namespace Anvil */

#endif/* MISC_IMAGE_VIEW_CREATE_INFO_H */