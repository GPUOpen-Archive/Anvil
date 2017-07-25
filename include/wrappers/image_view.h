//
// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
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

#include "misc/debug_marker.h"
#include "misc/types.h"

namespace Anvil
{
    class ImageView : public DebugMarkerSupportProvider<ImageView>
    {
    public:
        /* Public functions */

        /** Creates a single-sample 1D image view wrapper instance.
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
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static std::shared_ptr<ImageView> create_1D(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                    std::shared_ptr<Image>           in_image_ptr,
                                                    uint32_t                         in_n_base_layer,
                                                    uint32_t                         in_n_base_mipmap_level,
                                                    uint32_t                         in_n_mipmaps,
                                                    VkImageAspectFlags               in_aspect_mask,
                                                    VkFormat                         in_format,
                                                    VkComponentSwizzle               in_swizzle_red,
                                                    VkComponentSwizzle               in_swizzle_green,
                                                    VkComponentSwizzle               in_swizzle_blue,
                                                    VkComponentSwizzle               in_swizzle_alpha);

        /** Creates a single-sample 1D array image view wrapper instance.
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
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static std::shared_ptr<ImageView> create_1D_array(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                          std::shared_ptr<Image>           in_image_ptr,
                                                          uint32_t                         in_n_base_layer,
                                                          uint32_t                         in_n_layers,
                                                          uint32_t                         in_n_base_mipmap_level,
                                                          uint32_t                         in_n_mipmaps,
                                                          VkImageAspectFlags               in_aspect_mask,
                                                          VkFormat                         in_format,
                                                          VkComponentSwizzle               in_swizzle_red,
                                                          VkComponentSwizzle               in_swizzle_green,
                                                          VkComponentSwizzle               in_swizzle_blue,
                                                          VkComponentSwizzle               in_swizzle_alpha);

        /** Creates a single-sample or a multi-sample 2D image view wrapper instance. The view will be
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
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static std::shared_ptr<ImageView> create_2D(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                    std::shared_ptr<Image>           in_image_ptr,
                                                    uint32_t                         in_n_base_layer,
                                                    uint32_t                         in_n_base_mipmap_level,
                                                    uint32_t                         in_n_mipmaps,
                                                    VkImageAspectFlags               in_aspect_mask,
                                                    VkFormat                         in_format,
                                                    VkComponentSwizzle               in_swizzle_red,
                                                    VkComponentSwizzle               in_swizzle_green,
                                                    VkComponentSwizzle               in_swizzle_blue,
                                                    VkComponentSwizzle               in_swizzle_alpha);

        /** Creates a single-sample or a multi-sample 2D array image view wrapper instance. The view will be
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
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static std::shared_ptr<ImageView> create_2D_array(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                          std::shared_ptr<Anvil::Image>    in_image_ptr,
                                                          uint32_t                         in_n_base_layer,
                                                          uint32_t                         in_n_layers,
                                                          uint32_t                         in_n_base_mipmap_level,
                                                          uint32_t                         in_n_mipmaps,
                                                          VkImageAspectFlags               in_aspect_mask,
                                                          VkFormat                         in_format,
                                                          VkComponentSwizzle               in_swizzle_red,
                                                          VkComponentSwizzle               in_swizzle_green,
                                                          VkComponentSwizzle               in_swizzle_blue,
                                                          VkComponentSwizzle               in_swizzle_alpha);

        /** Creates a single-sample 3D image view wrapper instance.
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
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static std::shared_ptr<ImageView> create_3D(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                    std::shared_ptr<Image>           in_image_ptr,
                                                    uint32_t                         in_n_base_slice,
                                                    uint32_t                         in_n_slices,
                                                    uint32_t                         in_n_base_mipmap_level,
                                                    uint32_t                         in_n_mipmaps,
                                                    VkImageAspectFlags               in_aspect_mask,
                                                    VkFormat                         in_format,
                                                    VkComponentSwizzle               in_swizzle_red,
                                                    VkComponentSwizzle               in_swizzle_green,
                                                    VkComponentSwizzle               in_swizzle_blue,
                                                    VkComponentSwizzle               in_swizzle_alpha);

        /** Creates a cube-map image view wrapper instance.
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
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static std::shared_ptr<ImageView> create_cube_map(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                          std::shared_ptr<Anvil::Image>    in_image_ptr,
                                                          uint32_t                         in_n_base_layer,
                                                          uint32_t                         in_n_base_mipmap_level,
                                                          uint32_t                         in_n_mipmaps,
                                                          VkImageAspectFlags               in_aspect_mask,
                                                          VkFormat                         in_format,
                                                          VkComponentSwizzle               in_swizzle_red,
                                                          VkComponentSwizzle               in_swizzle_green,
                                                          VkComponentSwizzle               in_swizzle_blue,
                                                          VkComponentSwizzle               in_swizzle_alpha);

        /** Creates a cube-map array image view wrapper instance.
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
         *  @return New ImageView instance, if function executes successfully; nullptr otherwise.
         **/
        static std::shared_ptr<ImageView> create_cube_map_array(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                std::shared_ptr<Anvil::Image>    in_image_ptr,
                                                                uint32_t                         in_n_base_layer,
                                                                uint32_t                         in_n_cube_maps,
                                                                uint32_t                         in_n_base_mipmap_level,
                                                                uint32_t                         in_n_mipmaps,
                                                                VkImageAspectFlags               in_aspect_mask,
                                                                VkFormat                         in_format,
                                                                VkComponentSwizzle               in_swizzle_red,
                                                                VkComponentSwizzle               in_swizzle_green,
                                                                VkComponentSwizzle               in_swizzle_blue,
                                                                VkComponentSwizzle               in_swizzle_alpha);

        /** Destructor. Should only be called when the reference counter drops to zero.
         *
         *  Releases all internal Vulkan objects and the owned Image instance.
         **/
        virtual ~ImageView();

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
        std::shared_ptr<Anvil::Image> get_parent_image() const
        {
            return m_parent_image_ptr;
        }

        /** Returns a VkImageSubresourceRange struct that describes the range of subresources covered
         *  by this image view.
         */
        VkImageSubresourceRange get_subresource_range() const
        {
            VkImageSubresourceRange result;

            result.aspectMask     = m_aspect_mask;
            result.baseArrayLayer = m_n_base_layer;
            result.baseMipLevel   = m_n_base_mipmap_level;
            result.layerCount     = (m_type == VK_IMAGE_VIEW_TYPE_3D) ? m_n_slices : m_n_layers;
            result.levelCount     = m_n_mipmaps;

            return result;
        }

        /** Returns swizzle array assigned to the image view */
        void get_swizzle_array(VkComponentSwizzle* out_swizzle_array_ptr) const
        {
            memcpy(out_swizzle_array_ptr,
                   m_swizzle_array,
                   sizeof(m_swizzle_array) );
        }

        /** Returns image view type of the image view instance */
        const VkImageViewType get_type() const
        {
            return m_type;
        }

        /** Tells whether the subresource range described by this image view intersects
         *  with another image view's subres range.
         *
         *  NOTE: This function returns false if the underlying parent images do not match.
         *
         *  @param in_image_view_ptr Image view to use for the query. Must not be null.
         *
         *  @return True if an intersection was found, false otherwise.
         */
        bool intersects(std::shared_ptr<Anvil::ImageView> in_image_view_ptr) const;

    private:
        /* Private functions */
        ImageView           (const ImageView&);
        ImageView& operator=(const ImageView&);

        ImageView(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                  std::shared_ptr<Anvil::Image>    in_parent_image_ptr);

        bool init(VkImageViewType           in_image_view_type,
                  uint32_t                  in_n_base_layer,
                  uint32_t                  in_n_layers,
                  uint32_t                  in_n_slices,
                  uint32_t                  in_n_base_mipmap_level,
                  uint32_t                  in_n_mipmaps,
                  VkImageAspectFlags        in_aspect_mask,
                  VkFormat                  in_format,
                  const VkComponentSwizzle* in_swizzle_rgba_ptr);

        /* Private members */
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        VkImageView                      m_image_view;
        std::shared_ptr<Anvil::Image>    m_parent_image_ptr;

        VkImageAspectFlagsVariable(m_aspect_mask);

        VkFormat           m_format;
        uint32_t           m_n_base_layer;
        uint32_t           m_n_base_mipmap_level;
        uint32_t           m_n_layers;
        uint32_t           m_n_mipmaps;
        uint32_t           m_n_slices;
        VkComponentSwizzle m_swizzle_array[4];
        VkImageViewType    m_type;
    };
};

#endif /* WRAPPERS_IMAGE_VIEW_H */