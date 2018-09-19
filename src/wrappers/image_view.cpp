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

#include "misc/debug.h"
#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/object_tracker.h"
#include "misc/struct_chainer.h"
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"

/* Please see header for specification */
Anvil::ImageViewUniquePtr Anvil::ImageView::create(Anvil::ImageViewCreateInfoUniquePtr in_create_info_ptr)
{
    Anvil::ImageViewUniquePtr result_ptr = Anvil::ImageViewUniquePtr(nullptr,
                                                                     std::default_delete<Anvil::ImageView>() );

    result_ptr.reset(
        new Anvil::ImageView(std::move(in_create_info_ptr) )
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

Anvil::ImageView::ImageView(Anvil::ImageViewCreateInfoUniquePtr in_create_info_ptr)
    :DebugMarkerSupportProvider(in_create_info_ptr->get_device(),
                                VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT),
     MTSafetySupportProvider   (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                in_create_info_ptr->get_device   () ))
{
    m_create_info_ptr = std::move(in_create_info_ptr);

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
        lock();
        {
            vkDestroyImageView(m_device_ptr->get_device_vk(),
                               m_image_view,
                               nullptr /* pAllocator */);
        }
        unlock();

        m_image_view = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
void Anvil::ImageView::get_base_mipmap_size(uint32_t* out_opt_base_mipmap_width_ptr,
                                            uint32_t* out_opt_base_mipmap_height_ptr,
                                            uint32_t* out_opt_base_mipmap_depth_ptr) const
{
    const auto n_base_mip_level(m_create_info_ptr->get_base_mipmap_level() );
    const auto parent_image_ptr(m_create_info_ptr->get_parent_image     () );
    bool       result          (false);
    uint32_t   result_depth    (0);

    ANVIL_REDUNDANT_VARIABLE(result);

    result = parent_image_ptr->get_image_mipmap_size(n_base_mip_level,
                                                     out_opt_base_mipmap_width_ptr,
                                                     out_opt_base_mipmap_height_ptr,
                                                     nullptr);
    anvil_assert(result);

    switch (m_create_info_ptr->get_type() )
    {
        case VK_IMAGE_VIEW_TYPE_1D:         result_depth = 1;                                 break;
        case VK_IMAGE_VIEW_TYPE_1D_ARRAY:   result_depth = m_create_info_ptr->get_n_layers(); break;
        case VK_IMAGE_VIEW_TYPE_2D:         result_depth = 1;                                 break;
        case VK_IMAGE_VIEW_TYPE_2D_ARRAY:   result_depth = m_create_info_ptr->get_n_layers(); break;
        case VK_IMAGE_VIEW_TYPE_3D:         result_depth = m_create_info_ptr->get_n_slices(); break;
        case VK_IMAGE_VIEW_TYPE_CUBE:       result_depth = m_create_info_ptr->get_n_layers(); break;
        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY: result_depth = m_create_info_ptr->get_n_layers(); break;

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

VkImageSubresourceRange Anvil::ImageView::get_subresource_range() const
{
    VkImageSubresourceRange result;

    result.aspectMask     = m_create_info_ptr->get_aspect           ();
    result.baseArrayLayer = m_create_info_ptr->get_base_layer       ();
    result.baseMipLevel   = m_create_info_ptr->get_base_mipmap_level();
    result.layerCount     = m_create_info_ptr->get_n_layers         ();
    result.levelCount     = m_create_info_ptr->get_n_mipmaps        ();

    return result;
}

/** Performs a number of image view type-specific sanity checks and creates the requested
 *  Vulkan image view instance.
 *
 *  For argument discussion, please see documentation for the constructors above.
 *
 *  @return true if the function executed successfully, false otherwise.
 **/
bool Anvil::ImageView::init()
{
    const auto                                  aspect_mask               = m_create_info_ptr->get_aspect           ();
    const auto                                  format                    = m_create_info_ptr->get_format           ();
    const auto                                  image_view_type           = m_create_info_ptr->get_type             ();
    const auto                                  n_base_layer              = m_create_info_ptr->get_base_layer       ();
    const auto                                  n_base_mip                = m_create_info_ptr->get_base_mipmap_level();
    const auto                                  n_layers                  = m_create_info_ptr->get_n_layers         ();
    const auto                                  n_mips                    = m_create_info_ptr->get_n_mipmaps        ();
    uint32_t                                    parent_image_n_layers     = 0;
    uint32_t                                    parent_image_n_mipmaps    = 0;
    auto                                        parent_image_ptr          = m_create_info_ptr->get_parent_image();
    bool                                        result                    = false;
    VkResult                                    result_vk;
    Anvil::StructChainer<VkImageViewCreateInfo> struct_chainer;
    const auto&                                 swizzle_array             = m_create_info_ptr->get_swizzle_array();
    auto                                        usage                     = m_create_info_ptr->get_usage        ();

    parent_image_n_mipmaps = parent_image_ptr->get_n_mipmaps();

    if (parent_image_ptr->get_create_info_ptr()->get_type() != Anvil::ImageType::_3D)
    {
        parent_image_n_layers = parent_image_ptr->get_create_info_ptr()->get_n_layers();
    }
    else
    {
        parent_image_ptr->get_image_mipmap_size(0,       /* in_n_mipmap        */
                                                nullptr, /* out_opt_width_ptr  */
                                                nullptr, /* out_opt_height_ptr */
                                               &parent_image_n_layers);
    }

    if (!(parent_image_n_layers >= n_base_layer + n_layers))
    {
        anvil_assert(parent_image_n_layers >= n_base_layer + n_layers);

        goto end;
    }

    if (!(parent_image_n_mipmaps >= n_base_mip + n_mips))
    {
        anvil_assert(parent_image_n_mipmaps >= n_base_mip + n_mips);

        goto end;
    }

    if (usage != Anvil::ImageUsageFlagBits::IMAGE_USAGE_UNKNOWN                 &&
        !m_device_ptr->is_extension_enabled(VK_KHR_MAINTENANCE2_EXTENSION_NAME) )
    {
        anvil_assert(m_device_ptr->is_extension_enabled(VK_KHR_MAINTENANCE2_EXTENSION_NAME) );

        goto end;
    }

    if (parent_image_ptr->get_create_info_ptr()->get_type() == Anvil::ImageType::_3D)
    {
        if (image_view_type == VK_IMAGE_VIEW_TYPE_2D       ||
            image_view_type == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
        {
            if (!m_device_ptr->get_extension_info()->khr_maintenance1() )
            {
                anvil_assert(m_device_ptr->get_extension_info()->khr_maintenance1());

                goto end;
            }

            if ((parent_image_ptr->get_create_info_ptr()->get_create_flags() & Anvil::IMAGE_CREATE_FLAG_2D_ARRAY_COMPATIBLE_BIT) == 0)
            {
                anvil_assert((parent_image_ptr->get_create_info_ptr()->get_create_flags() & Anvil::IMAGE_CREATE_FLAG_2D_ARRAY_COMPATIBLE_BIT) != 0);

                goto end;
            }
        }
    }

    /* Create the image view instance */
    {
        VkImageViewCreateInfo image_view_create_info;

        image_view_create_info.components.a                    = static_cast<VkComponentSwizzle>(swizzle_array[3]);
        image_view_create_info.components.b                    = static_cast<VkComponentSwizzle>(swizzle_array[2]);
        image_view_create_info.components.g                    = static_cast<VkComponentSwizzle>(swizzle_array[1]);
        image_view_create_info.components.r                    = static_cast<VkComponentSwizzle>(swizzle_array[0]);
        image_view_create_info.flags                           = 0;
        image_view_create_info.format                          = static_cast<VkFormat>(format);
        image_view_create_info.image                           = parent_image_ptr->get_image();
        image_view_create_info.pNext                           = nullptr;
        image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.subresourceRange.aspectMask     = aspect_mask;
        image_view_create_info.subresourceRange.baseArrayLayer = n_base_layer;
        image_view_create_info.subresourceRange.baseMipLevel   = n_base_mip;
        image_view_create_info.subresourceRange.layerCount     = n_layers;
        image_view_create_info.subresourceRange.levelCount     = n_mips;
        image_view_create_info.viewType                        = image_view_type;

        struct_chainer.append_struct(image_view_create_info);
    }

    if (usage != Anvil::ImageUsageFlagBits::IMAGE_USAGE_UNKNOWN)
    {
        VkImageViewUsageCreateInfo usage_create_info;

        usage_create_info.pNext = nullptr;
        usage_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO_KHR;
        usage_create_info.usage = static_cast<VkImageUsageFlags>(usage);

        struct_chainer.append_struct(usage_create_info);
    }

    {
        auto chain_ptr = struct_chainer.create_chain();

        result_vk = vkCreateImageView(m_device_ptr->get_device_vk(),
                                      chain_ptr->get_root_struct(),
                                      nullptr, /* pAllocator */
                                     &m_image_view);
    }

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert_vk_call_succeeded(result_vk);

        goto end;
    }

    /* Cache the properties */
    set_vk_handle(m_image_view);

    /* All done */
    result = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::ImageView::intersects(const Anvil::ImageView* in_image_view_ptr) const
{
    auto parent_image_ptr = m_create_info_ptr->get_parent_image();
    bool result           = false;

    if (parent_image_ptr == in_image_view_ptr->m_create_info_ptr->get_parent_image() )
    {
        auto in_subresource_range   = in_image_view_ptr->get_subresource_range();
        auto this_subresource_range = get_subresource_range();

        /* Aspect mask */
        if ((this_subresource_range.aspectMask & in_subresource_range.aspectMask) != 0)
        {
            /* Layers + mips */
            if (!((this_subresource_range.baseArrayLayer                                     < in_subresource_range.baseArrayLayer    &&
                   this_subresource_range.baseArrayLayer + this_subresource_range.layerCount < in_subresource_range.baseArrayLayer)   ||
                  (in_subresource_range.baseArrayLayer                                       < this_subresource_range.baseArrayLayer  &&
                   in_subresource_range.baseArrayLayer   + in_subresource_range.layerCount   < this_subresource_range.baseArrayLayer) ))
            {
                if (!((this_subresource_range.baseMipLevel                                     < in_subresource_range.baseMipLevel    &&
                       this_subresource_range.baseMipLevel + this_subresource_range.levelCount < in_subresource_range.baseMipLevel)   ||
                      (in_subresource_range.baseMipLevel                                       < this_subresource_range.baseMipLevel  &&
                       in_subresource_range.baseMipLevel   + in_subresource_range.levelCount   < this_subresource_range.baseMipLevel) ))
                {
                    result = true;
                }
            }
        }
    }

    return result;
}
