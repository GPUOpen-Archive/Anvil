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
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_1D(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                              std::shared_ptr<Image>           image_ptr,
                                                              uint32_t                         n_base_layer,
                                                              uint32_t                         n_base_mipmap_level,
                                                              uint32_t                         n_mipmaps,
                                                              VkImageAspectFlagBits            aspect_mask,
                                                              VkFormat                         format,
                                                              VkComponentSwizzle               swizzle_red,
                                                              VkComponentSwizzle               swizzle_green,
                                                              VkComponentSwizzle               swizzle_blue,
                                                              VkComponentSwizzle               swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr (new ImageView(device_ptr,
                                                                 image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        swizzle_red,
        swizzle_green,
        swizzle_blue,
        swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_1D,
                                  n_base_layer,
                                  1, /* n_layers */
                                  1, /* n_slices */
                                  n_base_mipmap_level,
                                  n_mipmaps,
                                  aspect_mask,
                                  format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_1D_array(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                                    std::shared_ptr<Anvil::Image>    image_ptr,
                                                                    uint32_t                         n_base_layer,
                                                                    uint32_t                         n_layers,
                                                                    uint32_t                         n_base_mipmap_level,
                                                                    uint32_t                         n_mipmaps,
                                                                    VkImageAspectFlagBits            aspect_mask,
                                                                    VkFormat                         format,
                                                                    VkComponentSwizzle               swizzle_red,
                                                                    VkComponentSwizzle               swizzle_green,
                                                                    VkComponentSwizzle               swizzle_blue,
                                                                    VkComponentSwizzle               swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(device_ptr,
                                                                image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        swizzle_red,
        swizzle_green,
        swizzle_blue,
        swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_1D_ARRAY,
                                  n_base_layer,
                                  n_layers,
                                  1, /* n_slices */
                                  n_base_mipmap_level,
                                  n_mipmaps,
                                  aspect_mask,
                                  format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_2D(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                              std::shared_ptr<Anvil::Image>    image_ptr,
                                                              uint32_t                         n_base_layer,
                                                              uint32_t                         n_base_mipmap_level,
                                                              uint32_t                         n_mipmaps,
                                                              VkImageAspectFlagBits            aspect_mask,
                                                              VkFormat                         format,
                                                              VkComponentSwizzle               swizzle_red,
                                                              VkComponentSwizzle               swizzle_green,
                                                              VkComponentSwizzle               swizzle_blue,
                                                              VkComponentSwizzle               swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(device_ptr,
                                                                image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        swizzle_red,
        swizzle_green,
        swizzle_blue,
        swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_2D,
                                  n_base_layer,
                                  1, /* n_layers */
                                  1, /* n_slices */
                                  n_base_mipmap_level,
                                  n_mipmaps,
                                  aspect_mask,
                                  format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_2D_array(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                                    std::shared_ptr<Anvil::Image>    image_ptr,
                                                                    uint32_t                         n_base_layer,
                                                                    uint32_t                         n_layers,
                                                                    uint32_t                         n_base_mipmap_level,
                                                                    uint32_t                         n_mipmaps,
                                                                    VkImageAspectFlagBits            aspect_mask,
                                                                    VkFormat                         format,
                                                                    VkComponentSwizzle               swizzle_red,
                                                                    VkComponentSwizzle               swizzle_green,
                                                                    VkComponentSwizzle               swizzle_blue,
                                                                    VkComponentSwizzle               swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(device_ptr,
                                                                image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        swizzle_red,
        swizzle_green,
        swizzle_blue,
        swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                                  n_base_layer,
                                  n_layers,
                                  1, /* n_slices */
                                  n_base_mipmap_level,
                                  n_mipmaps,
                                  aspect_mask,
                                  format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_3D(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                              std::shared_ptr<Anvil::Image>    image_ptr,
                                                              uint32_t                         n_base_slice,
                                                              uint32_t                         n_slices,
                                                              uint32_t                         n_base_mipmap_level,
                                                              uint32_t                         n_mipmaps,
                                                              VkImageAspectFlagBits            aspect_mask,
                                                              VkFormat                         format,
                                                              VkComponentSwizzle               swizzle_red,
                                                              VkComponentSwizzle               swizzle_green,
                                                              VkComponentSwizzle               swizzle_blue,
                                                              VkComponentSwizzle               swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(device_ptr,
                                                                image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        swizzle_red,
        swizzle_green,
        swizzle_blue,
        swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_3D,
                                  n_base_slice,
                                  1, /* n_layers */
                                  n_slices,
                                  n_base_mipmap_level,
                                  n_mipmaps,
                                  aspect_mask,
                                  format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_cube_map(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                                    std::shared_ptr<Anvil::Image>    image_ptr,
                                                                    uint32_t                         n_base_layer,
                                                                    uint32_t                         n_base_mipmap_level,
                                                                    uint32_t                         n_mipmaps,
                                                                    VkImageAspectFlagBits            aspect_mask,
                                                                    VkFormat                         format,
                                                                    VkComponentSwizzle               swizzle_red,
                                                                    VkComponentSwizzle               swizzle_green,
                                                                    VkComponentSwizzle               swizzle_blue,
                                                                    VkComponentSwizzle               swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(device_ptr,
                                                                image_ptr) );
    const VkComponentSwizzle  swizzle_rgba[]     =
    {
        swizzle_red,
        swizzle_green,
        swizzle_blue,
        swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_CUBE,
                                  n_base_layer,
                                  6, /* n_layers */
                                  1, /* n_slices */
                                  n_base_mipmap_level,
                                  n_mipmaps,
                                  aspect_mask,
                                  format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::ImageView::create_cube_map_array(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                                          std::shared_ptr<Anvil::Image>    image_ptr,
                                                                          uint32_t                         n_base_layer,
                                                                          uint32_t                         n_cube_maps,
                                                                          uint32_t                         n_base_mipmap_level,
                                                                          uint32_t                         n_mipmaps,
                                                                          VkImageAspectFlagBits            aspect_mask,
                                                                          VkFormat                         format,
                                                                          VkComponentSwizzle               swizzle_red,
                                                                          VkComponentSwizzle               swizzle_green,
                                                                          VkComponentSwizzle               swizzle_blue,
                                                                          VkComponentSwizzle               swizzle_alpha)
{
    std::shared_ptr<ImageView> new_image_view_ptr(new ImageView(device_ptr,
                                                                image_ptr) );
    const VkComponentSwizzle   swizzle_rgba[]     =
    {
        swizzle_red,
        swizzle_green,
        swizzle_blue,
        swizzle_alpha
    };

    if (!new_image_view_ptr->init(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
                                  n_base_layer,
                                  n_cube_maps * 6,
                                  1, /* n_slices */
                                  n_base_mipmap_level,
                                  n_mipmaps,
                                  aspect_mask,
                                  format,
                                  swizzle_rgba) )
    {
        new_image_view_ptr = nullptr;
    }

    return new_image_view_ptr;
}

/** Constructor. Retains the user-specified Image instance and sets all members
 *  to default values.
 *
 *  @param device_ptr       Device to use. Must not be nullptr.
 *  @param parent_image_ptr Image to create the view for. Must not be nullptr.
 **/
Anvil::ImageView::ImageView(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                            std::shared_ptr<Anvil::Image>    parent_image_ptr)
{
    anvil_assert(parent_image_ptr != nullptr);

    m_device_ptr          = device_ptr;
    m_format              = VK_FORMAT_UNDEFINED;
    m_image_view          = VK_NULL_HANDLE;
    m_n_base_mipmap_level = UINT32_MAX;
    m_n_layers            = UINT32_MAX;
    m_n_mipmaps           = UINT32_MAX;
    m_n_slices            = UINT32_MAX;
    m_parent_image_ptr    = parent_image_ptr;

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_IMAGE_VIEW,
                                                 this);
}

Anvil::ImageView::~ImageView()
{
    if (m_image_view != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroyImageView(device_locked_ptr->get_device_vk(),
                           m_image_view,
                           nullptr /* pAllocator */);

        m_image_view = VK_NULL_HANDLE;
    }

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_IMAGE_VIEW,
                                                   this);
}

/* Please see header for specification */
void Anvil::ImageView::get_base_mipmap_size(uint32_t* out_opt_base_mipmap_width_ptr,
                                            uint32_t* out_opt_base_mipmap_height_ptr,
                                            uint32_t* out_opt_base_mipmap_depth_ptr) const
{
    bool result(false);

    ANVIL_REDUNDANT_VARIABLE(result);

    result = m_parent_image_ptr->get_image_mipmap_size(m_n_base_mipmap_level,
                                                       out_opt_base_mipmap_width_ptr,
                                                       out_opt_base_mipmap_height_ptr,
                                                       out_opt_base_mipmap_depth_ptr);

    anvil_assert(result);
}

/** Performs a number of image view type-specific sanity checks and creates the requested
 *  Vulkan image view instance.
 *
 *  For argument discussion, please see documentation for the constructors above.
 *
 *  @return true if the function executed successfully, false otherwise.
 **/
bool Anvil::ImageView::init(VkImageViewType           image_view_type,
                            uint32_t                  n_base_layer,
                            uint32_t                  n_layers,
                            uint32_t                  n_slices,
                            uint32_t                  n_base_mipmap_level,
                            uint32_t                  n_mipmaps,
                            VkImageAspectFlagBits     aspect_mask,
                            VkFormat                  format,
                            const VkComponentSwizzle* swizzle_rgba_ptr)
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
    parent_image_n_layers  = m_parent_image_ptr->get_image_n_layers ();
    parent_image_n_mipmaps = m_parent_image_ptr->get_image_n_mipmaps();

    if (!(parent_image_n_layers >= n_base_layer + n_layers))
    {
        anvil_assert(parent_image_n_layers >= n_base_layer + n_layers);

        goto end;
    }

    if (!(parent_image_n_mipmaps >= n_base_mipmap_level + n_mipmaps))
    {
        anvil_assert(parent_image_n_mipmaps >= n_base_mipmap_level + n_mipmaps);

        goto end;
    }

    if (! m_parent_image_ptr->is_image_mutable() &&
        !(  parent_image_format == format))
    {
        anvil_assert(parent_image_format == format);

        goto end;
    }

    /* Create the image view instance */
    image_view_create_info.components.a                    = swizzle_rgba_ptr[3];
    image_view_create_info.components.b                    = swizzle_rgba_ptr[2];
    image_view_create_info.components.g                    = swizzle_rgba_ptr[1];
    image_view_create_info.components.r                    = swizzle_rgba_ptr[0];
    image_view_create_info.flags                           = 0;
    image_view_create_info.format                          = format;
    image_view_create_info.image                           = m_parent_image_ptr->get_image();
    image_view_create_info.pNext                           = nullptr;
    image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.subresourceRange.aspectMask     = aspect_mask;
    image_view_create_info.subresourceRange.baseArrayLayer = n_base_layer;
    image_view_create_info.subresourceRange.baseMipLevel   = n_base_mipmap_level;
    image_view_create_info.subresourceRange.layerCount     = n_layers;
    image_view_create_info.subresourceRange.levelCount     = n_mipmaps;
    image_view_create_info.viewType                        = image_view_type;

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
    m_aspect_mask         = aspect_mask;
    m_format              = format;
    m_n_base_layer        = n_base_layer;
    m_n_base_mipmap_level = n_base_mipmap_level;
    m_n_layers            = n_layers;
    m_n_mipmaps           = n_mipmaps;
    m_n_slices            = n_slices;
    m_type                = image_view_type;

    memcpy(m_swizzle_array,
           swizzle_rgba_ptr,
           sizeof(m_swizzle_array) );

    /* All done */
    result = true;

end:
    return result;
}
