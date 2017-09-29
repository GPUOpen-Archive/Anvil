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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_1D(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                              std::shared_ptr<Image>           in_image_ptr,
                                                              uint32_t                         in_n_base_layer,
                                                              uint32_t                         in_n_base_mipmap_level,
                                                              uint32_t                         in_n_mipmaps,
                                                              VkImageAspectFlags               in_aspect_mask,
                                                              VkFormat                         in_format,
                                                              VkComponentSwizzle               in_swizzle_red,
                                                              VkComponentSwizzle               in_swizzle_green,
                                                              VkComponentSwizzle               in_swizzle_blue,
                                                              VkComponentSwizzle               in_swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr (new ImageView(in_device_ptr,
                                                                 in_image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_1D,
                                  in_n_base_layer,
                                  1, /* n_layers */
                                  1, /* n_slices */
                                  in_n_base_mipmap_level,
                                  in_n_mipmaps,
                                  in_aspect_mask,
                                  in_format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_1D_array(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
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
                                                                    VkComponentSwizzle               in_swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(in_device_ptr,
                                                                in_image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_1D_ARRAY,
                                  in_n_base_layer,
                                  in_n_layers,
                                  1, /* n_slices */
                                  in_n_base_mipmap_level,
                                  in_n_mipmaps,
                                  in_aspect_mask,
                                  in_format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_2D(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                              std::shared_ptr<Anvil::Image>    in_image_ptr,
                                                              uint32_t                         in_n_base_layer,
                                                              uint32_t                         in_n_base_mipmap_level,
                                                              uint32_t                         in_n_mipmaps,
                                                              VkImageAspectFlags               in_aspect_mask,
                                                              VkFormat                         in_format,
                                                              VkComponentSwizzle               in_swizzle_red,
                                                              VkComponentSwizzle               in_swizzle_green,
                                                              VkComponentSwizzle               in_swizzle_blue,
                                                              VkComponentSwizzle               in_swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(in_device_ptr,
                                                                in_image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_2D,
                                  in_n_base_layer,
                                  1, /* n_layers */
                                  1, /* n_slices */
                                  in_n_base_mipmap_level,
                                  in_n_mipmaps,
                                  in_aspect_mask,
                                  in_format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_2D_array(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
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
                                                                    VkComponentSwizzle               in_swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(in_device_ptr,
                                                                in_image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                                  in_n_base_layer,
                                  in_n_layers,
                                  1, /* n_slices */
                                  in_n_base_mipmap_level,
                                  in_n_mipmaps,
                                  in_aspect_mask,
                                  in_format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_3D(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                              std::shared_ptr<Anvil::Image>    in_image_ptr,
                                                              uint32_t                         in_n_base_slice,
                                                              uint32_t                         in_n_slices,
                                                              uint32_t                         in_n_base_mipmap_level,
                                                              uint32_t                         in_n_mipmaps,
                                                              VkImageAspectFlags               in_aspect_mask,
                                                              VkFormat                         in_format,
                                                              VkComponentSwizzle               in_swizzle_red,
                                                              VkComponentSwizzle               in_swizzle_green,
                                                              VkComponentSwizzle               in_swizzle_blue,
                                                              VkComponentSwizzle               in_swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(in_device_ptr,
                                                                in_image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_3D,
                                  in_n_base_slice,
                                  1, /* n_layers */
                                  in_n_slices,
                                  in_n_base_mipmap_level,
                                  in_n_mipmaps,
                                  in_aspect_mask,
                                  in_format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_cube_map(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                    std::shared_ptr<Anvil::Image>    in_image_ptr,
                                                                    uint32_t                         in_n_base_layer,
                                                                    uint32_t                         in_n_base_mipmap_level,
                                                                    uint32_t                         in_n_mipmaps,
                                                                    VkImageAspectFlags               in_aspect_mask,
                                                                    VkFormat                         in_format,
                                                                    VkComponentSwizzle               in_swizzle_red,
                                                                    VkComponentSwizzle               in_swizzle_green,
                                                                    VkComponentSwizzle               in_swizzle_blue,
                                                                    VkComponentSwizzle               in_swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(in_device_ptr,
                                                                in_image_ptr) );
    const VkComponentSwizzle  swizzle_rgba[]     =
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_CUBE,
                                  in_n_base_layer,
                                  6, /* n_layers */
                                  1, /* n_slices */
                                  in_n_base_mipmap_level,
                                  in_n_mipmaps,
                                  in_aspect_mask,
                                  in_format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_cube_map_array(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
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
                                                                          VkComponentSwizzle               in_swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(in_device_ptr,
                                                                in_image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
                                  in_n_base_layer,
                                  in_n_cube_maps * 6,
                                  1, /* n_slices */
                                  in_n_base_mipmap_level,
                                  in_n_mipmaps,
                                  in_aspect_mask,
                                  in_format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/** Constructor. Retains the user-specified Image instance and sets all members
 *  to default values.
 *
 *  @param in_device_ptr       Device to use. Must not be nullptr.
 *  @param in_parent_image_ptr Image to create the view for. Must not be nullptr.
 **/
Anvil::ImageView::ImageView(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                            std::shared_ptr<Anvil::Image>    in_parent_image_ptr)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT)
{
    anvil_assert(in_parent_image_ptr != nullptr);

    m_device_ptr          = in_device_ptr;
    m_format              = VK_FORMAT_UNDEFINED;
    m_image_view          = VK_NULL_HANDLE;
    m_n_base_mipmap_level = UINT32_MAX;
    m_n_layers            = UINT32_MAX;
    m_n_mipmaps           = UINT32_MAX;
    m_n_slices            = UINT32_MAX;
    m_parent_image_ptr    = in_parent_image_ptr;

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_IMAGE_VIEW,
                                                 this);
}

Anvil::ImageView::~ImageView()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_IMAGE_VIEW,
                                                   this);

    if (m_image_view != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroyImageView(device_locked_ptr->get_device_vk(),
                           m_image_view,
                           nullptr /* pAllocator */);

        m_image_view = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
void Anvil::ImageView::get_base_mipmap_size(uint32_t* out_opt_base_mipmap_width_ptr,
                                            uint32_t* out_opt_base_mipmap_height_ptr,
                                            uint32_t* out_opt_base_mipmap_depth_ptr) const
{
    bool     result      (false);
    uint32_t result_depth(0);

    ANVIL_REDUNDANT_VARIABLE(result);

    result = m_parent_image_ptr->get_image_mipmap_size(m_n_base_mipmap_level,
                                                       out_opt_base_mipmap_width_ptr,
                                                       out_opt_base_mipmap_height_ptr,
                                                       nullptr);
    anvil_assert(result);

    switch (m_type)
    {
        case VK_IMAGE_VIEW_TYPE_1D:         result_depth = 1;          break;
        case VK_IMAGE_VIEW_TYPE_1D_ARRAY:   result_depth = m_n_layers; break;
        case VK_IMAGE_VIEW_TYPE_2D:         result_depth = 1;          break;
        case VK_IMAGE_VIEW_TYPE_2D_ARRAY:   result_depth = m_n_layers; break;
        case VK_IMAGE_VIEW_TYPE_3D:         result_depth = m_n_slices; break;
        case VK_IMAGE_VIEW_TYPE_CUBE:       result_depth = m_n_layers; break;
        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY: result_depth = m_n_layers; break;

        default:
        {
            anvil_assert_fail();
        }
    }

    if (out_opt_base_mipmap_depth_ptr != nullptr)
    {
        *out_opt_base_mipmap_depth_ptr = result_depth;
    }
}

/** Performs a number of image view type-specific sanity checks and creates the requested
 *  Vulkan image view instance.
 *
 *  For argument discussion, please see documentation for the constructors above.
 *
 *  @return true if the function executed successfully, false otherwise.
 **/
bool Anvil::ImageView::init(VkImageViewType           in_image_view_type,
                            uint32_t                  in_n_base_layer,
                            uint32_t                  in_n_layers,
                            uint32_t                  in_n_slices,
                            uint32_t                  in_n_base_mipmap_level,
                            uint32_t                  in_n_mipmaps,
                            VkImageAspectFlags        in_aspect_mask,
                            VkFormat                  in_format,
                            const VkComponentSwizzle* in_swizzle_rgba_ptr)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkImageViewCreateInfo              image_view_create_info;
    VkFormat                           parent_image_format    = VK_FORMAT_UNDEFINED;
    uint32_t                           parent_image_n_layers  = 0;
    uint32_t                           parent_image_n_mipmaps = 0;
    bool                               result                 = false;
    VkResult                           result_vk;

    /* Sanity checks. Only focus on the basic stuff - hopefully more complicated issues
     * will be caught by the validation layers.
     */
    if (!(m_parent_image_ptr != nullptr) )
    {
        anvil_assert(m_parent_image_ptr != nullptr);

        goto end;
    }

    if (!m_parent_image_ptr->is_swapchain_image()            &&
        !m_parent_image_ptr->is_sparse         ()            &&
         m_parent_image_ptr->get_memory_block  () == nullptr)
    {
        anvil_assert(!(m_parent_image_ptr->get_memory_block() == nullptr));

        goto end;
    }

    parent_image_format    = m_parent_image_ptr->get_image_format   ();
    parent_image_n_mipmaps = m_parent_image_ptr->get_image_n_mipmaps();

    if (m_parent_image_ptr->get_image_type() != VK_IMAGE_TYPE_3D)
    {
        parent_image_n_layers = m_parent_image_ptr->get_image_n_layers ();
    }
    else
    {
        m_parent_image_ptr->get_image_mipmap_size(0,       /* in_n_mipmap        */
                                                  nullptr, /* out_opt_width_ptr  */
                                                  nullptr, /* out_opt_height_ptr */
                                                 &parent_image_n_layers);
    }

    if (!(parent_image_n_layers >= in_n_base_layer + in_n_layers))
    {
        anvil_assert(parent_image_n_layers >= in_n_base_layer + in_n_layers);

        goto end;
    }

    if (!(parent_image_n_mipmaps >= in_n_base_mipmap_level + in_n_mipmaps))
    {
        anvil_assert(parent_image_n_mipmaps >= in_n_base_mipmap_level + in_n_mipmaps);

        goto end;
    }

    if (((m_parent_image_ptr->get_image_create_flags() & Anvil::IMAGE_CREATE_FLAG_MUTABLE_FORMAT_BIT) == 0)         &&
         (parent_image_format                                                                         != in_format))
    {
        anvil_assert(parent_image_format == in_format);

        goto end;
    }

    if (m_parent_image_ptr->get_image_type() == VK_IMAGE_TYPE_3D)
    {
        if (in_image_view_type == VK_IMAGE_VIEW_TYPE_2D       ||
            in_image_view_type == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
        {
            if (!device_locked_ptr->is_khr_maintenance1_extension_enabled() )
            {
                anvil_assert(device_locked_ptr->is_khr_maintenance1_extension_enabled());

                goto end;
            }

            if ((m_parent_image_ptr->get_image_create_flags() & Anvil::IMAGE_CREATE_FLAG_2D_ARRAY_COMPATIBLE_BIT) == 0)
            {
                anvil_assert((m_parent_image_ptr->get_image_create_flags() & Anvil::IMAGE_CREATE_FLAG_2D_ARRAY_COMPATIBLE_BIT) != 0);

                goto end;
            }
        }
    }

    /* Create the image view instance */
    image_view_create_info.components.a                    = in_swizzle_rgba_ptr[3];
    image_view_create_info.components.b                    = in_swizzle_rgba_ptr[2];
    image_view_create_info.components.g                    = in_swizzle_rgba_ptr[1];
    image_view_create_info.components.r                    = in_swizzle_rgba_ptr[0];
    image_view_create_info.flags                           = 0;
    image_view_create_info.format                          = in_format;
    image_view_create_info.image                           = m_parent_image_ptr->get_image();
    image_view_create_info.pNext                           = nullptr;
    image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.subresourceRange.aspectMask     = in_aspect_mask;
    image_view_create_info.subresourceRange.baseArrayLayer = in_n_base_layer;
    image_view_create_info.subresourceRange.baseMipLevel   = in_n_base_mipmap_level;
    image_view_create_info.subresourceRange.layerCount     = in_n_layers;
    image_view_create_info.subresourceRange.levelCount     = in_n_mipmaps;
    image_view_create_info.viewType                        = in_image_view_type;

    result_vk = vkCreateImageView(device_locked_ptr->get_device_vk(),
                                 &image_view_create_info,
                                  nullptr, /* pAllocator */
                                 &m_image_view);

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert_vk_call_succeeded(result_vk);

        goto end;
    }

    /* Cache the properties */
    set_vk_handle(m_image_view);

    m_aspect_mask         = in_aspect_mask;
    m_format              = in_format;
    m_n_base_layer        = in_n_base_layer;
    m_n_base_mipmap_level = in_n_base_mipmap_level;
    m_n_layers            = in_n_layers;
    m_n_mipmaps           = in_n_mipmaps;
    m_n_slices            = in_n_slices;
    m_type                = in_image_view_type;

    memcpy(m_swizzle_array,
           in_swizzle_rgba_ptr,
           sizeof(m_swizzle_array) );

    /* All done */
    result = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::ImageView::intersects(std::shared_ptr<Anvil::ImageView> in_image_view_ptr) const
{
    bool result = false;

    if (get_parent_image() == in_image_view_ptr->get_parent_image() )
    {
        auto in_subresource_range   = in_image_view_ptr->get_subresource_range();
        auto this_subresource_range = get_subresource_range();

        /* Aspect mask */
        if ((this_subresource_range.aspectMask & in_subresource_range.aspectMask) != 0)
        {
            result = true;
        }
        else
        /* Layers + mips */
        if (!(this_subresource_range.baseArrayLayer                                     < in_subresource_range.baseArrayLayer   &&
              this_subresource_range.baseArrayLayer + this_subresource_range.layerCount < in_subresource_range.baseArrayLayer   &&
              in_subresource_range.baseArrayLayer                                       < this_subresource_range.baseArrayLayer &&
              in_subresource_range.baseArrayLayer   + in_subresource_range.layerCount   < this_subresource_range.baseArrayLayer) )
        {
            if (!(this_subresource_range.baseMipLevel                                     < in_subresource_range.baseMipLevel   &&
                  this_subresource_range.baseMipLevel + this_subresource_range.levelCount < in_subresource_range.baseMipLevel   &&
                  in_subresource_range.baseMipLevel                                       < this_subresource_range.baseMipLevel &&
                  in_subresource_range.baseMipLevel   + in_subresource_range.levelCount   < this_subresource_range.baseMipLevel) )
            {
                result = true;
            }
        }
    }

    return result;
}
