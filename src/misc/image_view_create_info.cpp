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
#include "misc/image_view_create_info.h"

Anvil::ImageViewCreateInfoUniquePtr Anvil::ImageViewCreateInfo::create_1D(const Anvil::BaseDevice* in_device_ptr,
                                                                          Image*                   in_image_ptr,
                                                                          uint32_t                 in_n_base_layer,
                                                                          uint32_t                 in_n_base_mipmap_level,
                                                                          uint32_t                 in_n_mipmaps,
                                                                          VkImageAspectFlags       in_aspect_mask,
                                                                          VkFormat                 in_format,
                                                                          VkComponentSwizzle       in_swizzle_red,
                                                                          VkComponentSwizzle       in_swizzle_green,
                                                                          VkComponentSwizzle       in_swizzle_blue,
                                                                          VkComponentSwizzle       in_swizzle_alpha)
{
    Anvil::ImageViewCreateInfoUniquePtr result_ptr    (nullptr,
                                                       std::default_delete<Anvil::ImageViewCreateInfo>() );
    const VkComponentSwizzle            swizzle_rgba[] = 
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    result_ptr.reset(
        new Anvil::ImageViewCreateInfo(in_aspect_mask,
                                       in_device_ptr,
                                       in_format,
                                       in_n_base_layer,
                                       in_n_base_mipmap_level,
                                       1, /* in_n_layers */
                                       in_n_mipmaps,
                                       1, /* in_n_slices */
                                       in_image_ptr,
                                       swizzle_rgba,
                                       VK_IMAGE_VIEW_TYPE_1D,
                                       Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE)
    );

    return result_ptr;
}

Anvil::ImageViewCreateInfoUniquePtr Anvil::ImageViewCreateInfo::create_1D_array(const Anvil::BaseDevice* in_device_ptr,
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
                                                                                VkComponentSwizzle       in_swizzle_alpha)
{
    Anvil::ImageViewCreateInfoUniquePtr result_ptr    (nullptr,
                                                       std::default_delete<Anvil::ImageViewCreateInfo>() );
    const VkComponentSwizzle            swizzle_rgba[] = 
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    result_ptr.reset(
        new Anvil::ImageViewCreateInfo(in_aspect_mask,
                                       in_device_ptr,
                                       in_format,
                                       in_n_base_layer,
                                       in_n_base_mipmap_level,
                                       in_n_layers,
                                       in_n_mipmaps,
                                       1, /* in_n_slices */
                                       in_image_ptr,
                                       swizzle_rgba,
                                       VK_IMAGE_VIEW_TYPE_1D_ARRAY,
                                       Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE)
    );

    return result_ptr;
}

Anvil::ImageViewCreateInfoUniquePtr Anvil::ImageViewCreateInfo::create_2D(const Anvil::BaseDevice* in_device_ptr,
                                                                          Image*                   in_image_ptr,
                                                                          uint32_t                 in_n_base_layer,
                                                                          uint32_t                 in_n_base_mipmap_level,
                                                                          uint32_t                 in_n_mipmaps,
                                                                          VkImageAspectFlags       in_aspect_mask,
                                                                          VkFormat                 in_format,
                                                                          VkComponentSwizzle       in_swizzle_red,
                                                                          VkComponentSwizzle       in_swizzle_green,
                                                                          VkComponentSwizzle       in_swizzle_blue,
                                                                          VkComponentSwizzle       in_swizzle_alpha)
{
    Anvil::ImageViewCreateInfoUniquePtr result_ptr    (nullptr,
                                                       std::default_delete<Anvil::ImageViewCreateInfo>() );
    const VkComponentSwizzle            swizzle_rgba[] = 
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    result_ptr.reset(
        new Anvil::ImageViewCreateInfo(in_aspect_mask,
                                       in_device_ptr,
                                       in_format,
                                       in_n_base_layer,
                                       in_n_base_mipmap_level,
                                       1, /* in_n_layers */
                                       in_n_mipmaps,
                                       1, /* in_n_slices */
                                       in_image_ptr,
                                       swizzle_rgba,
                                       VK_IMAGE_VIEW_TYPE_2D,
                                       Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE)
    );

    return result_ptr;
}

Anvil::ImageViewCreateInfoUniquePtr Anvil::ImageViewCreateInfo::create_2D_array(const Anvil::BaseDevice* in_device_ptr,
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
                                                                                VkComponentSwizzle       in_swizzle_alpha)
{
    Anvil::ImageViewCreateInfoUniquePtr result_ptr    (nullptr,
                                                       std::default_delete<Anvil::ImageViewCreateInfo>() );
    const VkComponentSwizzle            swizzle_rgba[] = 
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    result_ptr.reset(
        new Anvil::ImageViewCreateInfo(in_aspect_mask,
                                       in_device_ptr,
                                       in_format,
                                       in_n_base_layer,
                                       in_n_base_mipmap_level,
                                       in_n_layers,
                                       in_n_mipmaps,
                                       1, /* in_n_slices */
                                       in_image_ptr,
                                       swizzle_rgba,
                                       VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                                       Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE)
    );

    return result_ptr;
}

Anvil::ImageViewCreateInfoUniquePtr Anvil::ImageViewCreateInfo::create_3D(const Anvil::BaseDevice* in_device_ptr,
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
                                                                          VkComponentSwizzle       in_swizzle_alpha)
{
    Anvil::ImageViewCreateInfoUniquePtr result_ptr    (nullptr,
                                                       std::default_delete<Anvil::ImageViewCreateInfo>() );
    const VkComponentSwizzle            swizzle_rgba[] = 
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    result_ptr.reset(
        new Anvil::ImageViewCreateInfo(in_aspect_mask,
                                       in_device_ptr,
                                       in_format,
                                       in_n_base_slice,
                                       in_n_base_mipmap_level,
                                       1, /* in_n_layers */
                                       in_n_mipmaps,
                                       in_n_slices,
                                       in_image_ptr,
                                       swizzle_rgba,
                                       VK_IMAGE_VIEW_TYPE_3D,
                                       Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE)
    );

    return result_ptr;
}

Anvil::ImageViewCreateInfoUniquePtr Anvil::ImageViewCreateInfo::create_cube_map(const Anvil::BaseDevice* in_device_ptr,
                                                                                Anvil::Image*            in_image_ptr,
                                                                                uint32_t                 in_n_base_layer,
                                                                                uint32_t                 in_n_base_mipmap_level,
                                                                                uint32_t                 in_n_mipmaps,
                                                                                VkImageAspectFlags       in_aspect_mask,
                                                                                VkFormat                 in_format,
                                                                                VkComponentSwizzle       in_swizzle_red,
                                                                                VkComponentSwizzle       in_swizzle_green,
                                                                                VkComponentSwizzle       in_swizzle_blue,
                                                                                VkComponentSwizzle       in_swizzle_alpha)
{
    Anvil::ImageViewCreateInfoUniquePtr result_ptr    (nullptr,
                                                       std::default_delete<Anvil::ImageViewCreateInfo>() );
    const VkComponentSwizzle            swizzle_rgba[] = 
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    result_ptr.reset(
        new Anvil::ImageViewCreateInfo(in_aspect_mask,
                                       in_device_ptr,
                                       in_format,
                                       in_n_base_layer,
                                       in_n_base_mipmap_level,
                                       6, /* in_n_layers */
                                       in_n_mipmaps,
                                       1, /* in_n_slices */
                                       in_image_ptr,
                                       swizzle_rgba,
                                       VK_IMAGE_VIEW_TYPE_CUBE,
                                       Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE)
    );

    return result_ptr;
}

Anvil::ImageViewCreateInfoUniquePtr Anvil::ImageViewCreateInfo::create_cube_map_array(const Anvil::BaseDevice* in_device_ptr,
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
                                                                                      VkComponentSwizzle       in_swizzle_alpha)
{
    Anvil::ImageViewCreateInfoUniquePtr result_ptr    (nullptr,
                                                       std::default_delete<Anvil::ImageViewCreateInfo>() );
    const VkComponentSwizzle            swizzle_rgba[] = 
    {
        in_swizzle_red,
        in_swizzle_green,
        in_swizzle_blue,
        in_swizzle_alpha
    };

    result_ptr.reset(
        new Anvil::ImageViewCreateInfo(in_aspect_mask,
                                       in_device_ptr,
                                       in_format,
                                       in_n_base_layer,
                                       in_n_base_mipmap_level,
                                       in_n_cube_maps * 6,
                                       in_n_mipmaps,
                                       1, /* in_n_slices */
                                       in_image_ptr,
                                       swizzle_rgba,
                                       VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
                                       Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE)
    );

    return result_ptr;
}

Anvil::ImageViewCreateInfo::ImageViewCreateInfo(const VkImageAspectFlags& in_aspect_mask,
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
                                                const Anvil::MTSafety&    in_mt_safety)
    :m_aspect_mask        (in_aspect_mask),
     m_device_ptr         (in_device_ptr),
     m_format             (in_format),
     m_mt_safety          (in_mt_safety),
     m_n_base_layer       (in_n_base_layer),
     m_n_base_mipmap_level(in_n_base_mipmap_level),
     m_n_layers           (in_n_layers),
     m_n_mipmaps          (in_n_mipmaps),
     m_n_slices           (in_n_slices),
     m_parent_image_ptr   (in_parent_image_ptr),
     m_swizzle_array      ({in_swizzle_array_ptr[0], in_swizzle_array_ptr[1], in_swizzle_array_ptr[2], in_swizzle_array_ptr[3]}),
     m_type               (in_type)
{
    anvil_assert(in_parent_image_ptr != nullptr);
}
