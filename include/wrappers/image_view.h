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

/** Defines an image view wrapper class which simplifies the following processes:
 *
 *  - Instantation of an image view, and its destruction.
 *  - Reference counting of Image and Image View instances
 **/
#ifndef WRAPPERS_IMAGE_VIEW_H
#define WRAPPERS_IMAGE_VIEW_H

#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
    class ImageView : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Creates a single-sample 1D image view wrapper instance.
         *
         *  @param device_ptr          Device to use.
         *  @param image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                             object will be retained and release at ImageView release time.
         *  @param n_base_layer        Base layer index.
         *  @param n_base_mipmap_level Base mipmap level.
         *  @param n_mipmaps           Number of mipmaps to include in the view.
         *  @param aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param format              Image view format.
         *  @param swizzle_red         Channel to use for the red component when sampling the view.
         *  @param swizzle_green       Channel to use for the green component when sampling the view.
         *  @param swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static ImageView* create_1D_image_view(Anvil::Device*        device_ptr,
                                               Image*                image_ptr,
                                               uint32_t              n_base_layer,
                                               uint32_t              n_base_mipmap_level,
                                               uint32_t              n_mipmaps,
                                               VkImageAspectFlagBits aspect_mask,
                                               VkFormat              format,
                                               VkComponentSwizzle    swizzle_red,
                                               VkComponentSwizzle    swizzle_green,
                                               VkComponentSwizzle    swizzle_blue,
                                               VkComponentSwizzle    swizzle_alpha);

        /** Creates a single-sample 1D array image view wrapper instance.
         *
         *  @param device_ptr          Device to use.
         *  @param image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                             object will be retained and release at ImageView release time.
         *  @param n_base_layer        Base layer index.
         *  @param n_layers            Number of layers to include in the view.
         *  @param n_base_mipmap_level Base mipmap level.
         *  @param n_mipmaps           Number of mipmaps to include in the view.
         *  @param aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param format              Image view format.
         *  @param swizzle_red         Channel to use for the red component when sampling the view.
         *  @param swizzle_green       Channel to use for the green component when sampling the view.
         *  @param swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static ImageView* create_1D_array_image_view(Anvil::Device*        device_ptr,
                                                     Image*                image_ptr,
                                                     uint32_t              n_base_layer,
                                                     uint32_t              n_layers,
                                                     uint32_t              n_base_mipmap_level,
                                                     uint32_t              n_mipmaps,
                                                     VkImageAspectFlagBits aspect_mask,
                                                     VkFormat              format,
                                                     VkComponentSwizzle    swizzle_red,
                                                     VkComponentSwizzle    swizzle_green,
                                                     VkComponentSwizzle    swizzle_blue,
                                                     VkComponentSwizzle    swizzle_alpha);

        /** Creates a single-sample or a multi-sample 2D image view wrapper instance. The view will be
         *  single-sample if @param image_ptr uses 1 sample per texel, and multi-sample otherwise.
         *
         *  @param device_ptr          Device to use
         *  @param image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                             object will be retained and release at ImageView release time.
         *  @param n_base_layer        Base layer index.
         *  @param n_base_mipmap_level Base mipmap level.
         *  @param n_mipmaps           Number of mipmaps to include in the view.
         *  @param aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param format              Image view format.
         *  @param swizzle_red         Channel to use for the red component when sampling the view.
         *  @param swizzle_green       Channel to use for the green component when sampling the view.
         *  @param swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static ImageView* create_2D_image_view(Anvil::Device*        device_ptr,
                                               Image*                image_ptr,
                                               uint32_t              n_base_layer,
                                               uint32_t              n_base_mipmap_level,
                                               uint32_t              n_mipmaps,
                                               VkImageAspectFlagBits aspect_mask,
                                               VkFormat              format,
                                               VkComponentSwizzle    swizzle_red,
                                               VkComponentSwizzle    swizzle_green,
                                               VkComponentSwizzle    swizzle_blue,
                                               VkComponentSwizzle    swizzle_alpha);

        /** Creates a single-sample or a multi-sample 2D array image view wrapper instance. The view will be
         *  single-sample if @param image_ptr uses 1 sample per texel, and multi-sample otherwise.
         *
         *  @param device_ptr          Device to use.
         *  @param image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                             object will be retained and release at ImageView release time.
         *  @param n_base_layer        Base layer index.
         *  @param n_layers            Number of layers to include in the view.
         *  @param n_base_mipmap_level Base mipmap level.
         *  @param n_mipmaps           Number of mipmaps to include in the view.
         *  @param aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param format              Image view format.
         *  @param swizzle_red         Channel to use for the red component when sampling the view.
         *  @param swizzle_green       Channel to use for the green component when sampling the view.
         *  @param swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static ImageView* create_2D_array_image_view(Anvil::Device*        device_ptr,
                                                     Image*                image_ptr,
                                                     uint32_t              n_base_layer,
                                                     uint32_t              n_layers,
                                                     uint32_t              n_base_mipmap_level,
                                                     uint32_t              n_mipmaps,
                                                     VkImageAspectFlagBits aspect_mask,
                                                     VkFormat              format,
                                                     VkComponentSwizzle    swizzle_red,
                                                     VkComponentSwizzle    swizzle_green,
                                                     VkComponentSwizzle    swizzle_blue,
                                                     VkComponentSwizzle    swizzle_alpha);

        /** Creates a single-sample 3D image view wrapper instance.
         *
         *  @param device_ptr          Device to use.
         *  @param image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                             object will be retained and release at ImageView release time.
         *  @param n_base_slice        Base slice index.
         *  @param n_slices            Number of slices to include in the view.
         *  @param n_base_mipmap_level Base mipmap level.
         *  @param n_mipmaps           Number of mipmaps to include in the view.
         *  @param aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param format              Image view format.
         *  @param swizzle_red         Channel to use for the red component when sampling the view.
         *  @param swizzle_green       Channel to use for the green component when sampling the view.
         *  @param swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static ImageView* create_3D_image_view(Anvil::Device*        device_ptr,
                                               Image*                image_ptr,
                                               uint32_t              n_base_slice,
                                               uint32_t              n_slices,
                                               uint32_t              n_base_mipmap_level,
                                               uint32_t              n_mipmaps,
                                               VkImageAspectFlagBits aspect_mask,
                                               VkFormat              format,
                                               VkComponentSwizzle    swizzle_red,
                                               VkComponentSwizzle    swizzle_green,
                                               VkComponentSwizzle    swizzle_blue,
                                               VkComponentSwizzle    swizzle_alpha);

        /** Creates a cube-map image view wrapper instance.
         *
         *  @param device_ptr          Device to use.
         *  @param image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                             object will be retained and release at ImageView release time.
         *  @param n_base_layer        Base layer index.
         *  @param n_base_mipmap_level Base mipmap level.
         *  @param n_mipmaps           Number of mipmaps to include in the view.
         *  @param aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param format              Image view format.
         *  @param swizzle_red         Channel to use for the red component when sampling the view.
         *  @param swizzle_green       Channel to use for the green component when sampling the view.
         *  @param swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static ImageView* create_cube_map_image_view(Anvil::Device*        device_ptr,
                                                     Image*                image_ptr,
                                                     uint32_t              n_base_layer,
                                                     uint32_t              n_base_mipmap_level,
                                                     uint32_t              n_mipmaps,
                                                     VkImageAspectFlagBits aspect_mask,
                                                     VkFormat              format,
                                                     VkComponentSwizzle    swizzle_red,
                                                     VkComponentSwizzle    swizzle_green,
                                                     VkComponentSwizzle    swizzle_blue,
                                                     VkComponentSwizzle    swizzle_alpha);

        /** Creates a cube-map array image view wrapper instance.
         *
         *  @param device_ptr          Device to use.
         *  @param image_ptr           Image instance to create a view for. Must not be nullptr. The specified
         *                             object will be retained and release at ImageView release time.
         *  @param n_base_layer        Base layer index.
         *  @param n_cube_maps         Number of cube-maps to include in the view. The number of layers created
         *                             for the view will be equal to @param n_cube_maps * 6.
         *  @param n_base_mipmap_level Base mipmap level.
         *  @param n_mipmaps           Number of mipmaps to include in the view.
         *  @param aspect_mask         Image aspect mask to use when creating the Vulkan image view instance.
         *  @param format              Image view format.
         *  @param swizzle_red         Channel to use for the red component when sampling the view.
         *  @param swizzle_green       Channel to use for the green component when sampling the view.
         *  @param swizzle_blue        Channel to use for the blue component when sampling the view.
         *  @param swizzle_alpha       Channel to use for the alpha component when sampling the view.
         *
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static ImageView* create_cube_map_array_image_view(Anvil::Device*         device_ptr,
                                                           Image*                 image_ptr,
                                                           uint32_t               n_base_layer,
                                                           uint32_t               n_cube_maps,
                                                           uint32_t               n_base_mipmap_level,
                                                           uint32_t               n_mipmaps,
                                                           VkImageAspectFlagBits  aspect_mask,
                                                           VkFormat               format,
                                                           VkComponentSwizzle     swizzle_red,
                                                           VkComponentSwizzle     swizzle_green,
                                                           VkComponentSwizzle     swizzle_blue,
                                                           VkComponentSwizzle     swizzle_alpha);

        /* Returns the internally maintained Vulkan image view instance. */
        uint32_t get_base_mipmap_level() const
        {
            return m_n_base_mipmap_level;
        }

        /** Returns dimensions of the base mipmap, as seen from the image view.
         *
         *  @param out_opt_base_mipmap_width_ptr  If not nullptr, deref will be set to mipmap's width.
         *  @param out_opt_base_mipmap_height_ptr If not nullptr, deref will be set to mipmap's height.
         *  @param out_opt_base_mipmap_depth_ptr  If not nullptr, deref will be set to mipmap's depth.
         *
         **/
        void get_base_mipmap_size(uint32_t* out_opt_base_mipmap_width_ptr,
                                  uint32_t* out_opt_base_mipmap_height_ptr,
                                  uint32_t* out_opt_base_mipmap_depth_ptr) const;

        /** Returns image view's format */
        const VkFormat get_image_format() const
        {
            return m_format;
        }

        /** Returns the encapsulated raw Vulkan image view handle. */
        const VkImageView get_image_view() const
        {
            return m_image_view;
        }

        /** Returns a pointer to the parent image, from which the image view has been created. */
        Anvil::Image* get_parent_image()
        {
            return m_parent_image_ptr;
        }

    private:
        /* Private functions */
        ImageView           (const ImageView&);
        ImageView& operator=(const ImageView&);

        ImageView(Anvil::Device* device_ptr,
                  Anvil::Image*  parent_image_ptr);

        virtual ~ImageView();

        bool init(VkImageViewType           image_view_type,
                  uint32_t                  n_base_layer,
                  uint32_t                  n_layers,
                  uint32_t                  n_slices,
                  uint32_t                  n_base_mipmap_level,
                  uint32_t                  n_mipmaps,
                  VkImageAspectFlagBits     aspect_mask,
                  VkFormat                  format,
                  const VkComponentSwizzle* swizzle_rgba_ptr);

        /* Private members */
        Anvil::Device* m_device_ptr;
        VkImageView    m_image_view;
        Anvil::Image*  m_parent_image_ptr;

        VkImageAspectFlagBits m_aspect_mask;
        VkFormat              m_format;
        uint32_t              m_n_base_mipmap_level;
        uint32_t              m_n_layers;
        uint32_t              m_n_mipmaps;
        uint32_t              m_n_slices;
    };

    /** Delete functor. Useful if you need to wrap the image view instance in an auto pointer */
    struct ImageViewDeleter
    {
        void operator()(ImageView* image_view_ptr) const
        {
            image_view_ptr->release();
        }
    };
};

#endif /* WRAPPERS_IMAGE_VIEW_H */