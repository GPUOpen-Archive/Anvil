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
#include "misc/image_create_info.h"
#include "misc/swapchain_create_info.h"
#include "wrappers/swapchain.h"

Anvil::ImageCreateInfoUniquePtr Anvil::ImageCreateInfo::create_nonsparse_no_alloc(const Anvil::BaseDevice*          in_device_ptr,
                                                                                  VkImageType                       in_type,
                                                                                  VkFormat                          in_format,
                                                                                  VkImageTiling                     in_tiling,
                                                                                  VkImageUsageFlags                 in_usage,
                                                                                  uint32_t                          in_base_mipmap_width,
                                                                                  uint32_t                          in_base_mipmap_height,
                                                                                  uint32_t                          in_base_mipmap_depth,
                                                                                  uint32_t                          in_n_layers,
                                                                                  VkSampleCountFlagBits             in_sample_count,
                                                                                  Anvil::QueueFamilyBits            in_queue_families,
                                                                                  VkSharingMode                     in_sharing_mode,
                                                                                  bool                              in_use_full_mipmap_chain,
                                                                                  ImageCreateFlags                  in_create_flags,
                                                                                  VkImageLayout                     in_post_alloc_image_layout,
                                                                                  const std::vector<MipmapRawData>* in_opt_mipmaps_ptr)
{
    Anvil::ImageCreateInfoUniquePtr result_ptr(nullptr,
                                               std::default_delete<Anvil::ImageCreateInfo>() );

    result_ptr.reset(
        new ImageCreateInfo(Anvil::ImageType::NONSPARSE_NO_ALLOC,
                            in_device_ptr,
                            in_type,
                            in_format,
                            in_tiling,
                            in_sharing_mode,
                            in_usage,
                            in_base_mipmap_width,
                            in_base_mipmap_height,
                            in_base_mipmap_depth,
                            in_n_layers,
                            in_sample_count,
                            in_use_full_mipmap_chain,
                            in_create_flags,
                            in_queue_families,
                            ((in_opt_mipmaps_ptr != nullptr) && (in_opt_mipmaps_ptr->size() > 0)) ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED,
                            in_post_alloc_image_layout,
                            in_opt_mipmaps_ptr,
                            Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE,
                            0, /* in_external_memory_handle_types */
                            0, /* in_memory_features              */
                            Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED) 
    );

    return result_ptr;
}

Anvil::ImageCreateInfoUniquePtr Anvil::ImageCreateInfo::create_nonsparse_alloc(const Anvil::BaseDevice*          in_device_ptr,
                                                                               VkImageType                       in_type,
                                                                               VkFormat                          in_format,
                                                                               VkImageTiling                     in_tiling,
                                                                               VkImageUsageFlags                 in_usage,
                                                                               uint32_t                          in_base_mipmap_width,
                                                                               uint32_t                          in_base_mipmap_height,
                                                                               uint32_t                          in_base_mipmap_depth,
                                                                               uint32_t                          in_n_layers,
                                                                               VkSampleCountFlagBits             in_sample_count,
                                                                               Anvil::QueueFamilyBits            in_queue_families,
                                                                               VkSharingMode                     in_sharing_mode,
                                                                               bool                              in_use_full_mipmap_chain,
                                                                               MemoryFeatureFlags                in_memory_features,
                                                                               ImageCreateFlags                  in_create_flags,
                                                                               VkImageLayout                     in_post_alloc_image_layout,
                                                                               const std::vector<MipmapRawData>* in_opt_mipmaps_ptr)
{
    Anvil::ImageCreateInfoUniquePtr result_ptr(nullptr,
                                               std::default_delete<Anvil::ImageCreateInfo>() );

    result_ptr.reset(
        new ImageCreateInfo(Anvil::ImageType::NONSPARSE_ALLOC,
                            in_device_ptr,
                            in_type,
                            in_format,
                            in_tiling,
                            in_sharing_mode,
                            in_usage,
                            in_base_mipmap_width,
                            in_base_mipmap_height,
                            in_base_mipmap_depth,
                            in_n_layers,
                            in_sample_count,
                            in_use_full_mipmap_chain,
                            in_create_flags,
                            in_queue_families,
                            ((in_opt_mipmaps_ptr != nullptr) && (in_opt_mipmaps_ptr->size() > 0)) ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED,
                            in_post_alloc_image_layout,
                            in_opt_mipmaps_ptr,
                            Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE,
                            0, /* in_external_memory_handle_types */
                            in_memory_features,
                            Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED) 
    );

    return result_ptr;
}

Anvil::ImageCreateInfoUniquePtr Anvil::ImageCreateInfo::create_sparse_no_alloc(const Anvil::BaseDevice*    in_device_ptr,
                                                                               VkImageType                 in_type,
                                                                               VkFormat                    in_format,
                                                                               VkImageTiling               in_tiling,
                                                                               VkImageUsageFlags           in_usage,
                                                                               uint32_t                    in_base_mipmap_width,
                                                                               uint32_t                    in_base_mipmap_height,
                                                                               uint32_t                    in_base_mipmap_depth,
                                                                               uint32_t                    in_n_layers,
                                                                               VkSampleCountFlagBits       in_sample_count,
                                                                               Anvil::QueueFamilyBits      in_queue_families,
                                                                               VkSharingMode               in_sharing_mode,
                                                                               bool                        in_use_full_mipmap_chain,
                                                                               ImageCreateFlags            in_create_flags,
                                                                               Anvil::SparseResidencyScope in_residency_scope)
{
    Anvil::ImageCreateInfoUniquePtr result_ptr(nullptr,
                                               std::default_delete<Anvil::ImageCreateInfo>() );

    result_ptr.reset(
        new ImageCreateInfo(Anvil::ImageType::SPARSE_NO_ALLOC,
                            in_device_ptr,
                            in_type,
                            in_format,
                            in_tiling,
                            in_sharing_mode,
                            in_usage,
                            in_base_mipmap_width,
                            in_base_mipmap_height,
                            in_base_mipmap_depth,
                            in_n_layers,
                            in_sample_count,
                            in_use_full_mipmap_chain,
                            in_create_flags,
                            in_queue_families,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            nullptr, /* in_opt_mipmaps_ptr */
                            Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE,
                            0, /* in_external_memory_handle_types */
                            0, /* in_memory_features              */
                            in_residency_scope)
    );

    return result_ptr;
}

Anvil::ImageCreateInfoUniquePtr Anvil::ImageCreateInfo::create_swapchain_wrapper(const Anvil::BaseDevice* in_device_ptr,
                                                                                 const Anvil::Swapchain*  in_swapchain_ptr,
                                                                                 const VkImage&           in_image,
                                                                                 const uint32_t&          in_n_swapchain_image)
{
    Anvil::ImageCreateInfoUniquePtr result_ptr               (nullptr,
                                                              std::default_delete<Anvil::ImageCreateInfo>() );
    const auto&                     swapchain_create_info_ptr(in_swapchain_ptr->get_create_info_ptr() );

    result_ptr.reset(
        new Anvil::ImageCreateInfo(Anvil::ImageType::SWAPCHAIN_WRAPPER,
                                   in_device_ptr,
                                   VK_IMAGE_TYPE_2D,
                                   swapchain_create_info_ptr->get_format(),
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_SHARING_MODE_EXCLUSIVE,
                                   swapchain_create_info_ptr->get_usage_flags(),
                                   swapchain_create_info_ptr->get_rendering_surface()->get_width (),
                                   swapchain_create_info_ptr->get_rendering_surface()->get_height(),
                                   1, /* base_mipmap_depth */
                                   1, /* in_n_layers       */
                                   VK_SAMPLE_COUNT_1_BIT,
                                   false, /* in_use_full_mipmap_chain */
                                   0,     /* in_create_flags          */
                                   0,     /* in_queue_families        */
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   nullptr, /* in_opt_mipmaps_ptr */
                                   Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE,
                                   0, /* in_external_memory_handle_types */
                                   0, /* in_memory_features               */
                                   Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED)
    );

    if (result_ptr != nullptr)
    {
        result_ptr->m_swapchain_image   = in_image;
        result_ptr->m_n_swapchain_image = in_n_swapchain_image;
        result_ptr->m_swapchain_ptr     = in_swapchain_ptr;
    }

    return result_ptr;
}

Anvil::ImageCreateInfo::ImageCreateInfo(Anvil::ImageType                    in_type,
                                        const Anvil::BaseDevice*            in_device_ptr,
                                        VkImageType                         in_type_vk,
                                        VkFormat                            in_format,
                                        VkImageTiling                       in_tiling,
                                        VkSharingMode                       in_sharing_mode, 
                                        VkImageUsageFlags                   in_usage,
                                        uint32_t                            in_base_mipmap_width,
                                        uint32_t                            in_base_mipmap_height,
                                        uint32_t                            in_base_mipmap_depth,
                                        uint32_t                            in_n_layers,
                                        VkSampleCountFlagBits               in_sample_count,
                                        bool                                in_use_full_mipmap_chain,
                                        ImageCreateFlags                    in_create_flags,
                                        Anvil::QueueFamilyBits              in_queue_families,
                                        VkImageLayout                       in_post_create_image_layout,
                                        const VkImageLayout&                in_post_alloc_image_layout,
                                        const std::vector<MipmapRawData>*   in_opt_mipmaps_ptr,
                                        const Anvil::MTSafety&              in_mt_safety,
                                        Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types,
                                        const Anvil::MemoryFeatureFlags&    in_memory_features,
                                        const Anvil::SparseResidencyScope&  in_residency_scope)
     :m_create_flags                (in_create_flags),
      m_depth                       (in_base_mipmap_depth),
      m_device_ptr                  (in_device_ptr),
      m_external_memory_handle_types(in_external_memory_handle_types),
      m_format                      (in_format),
      m_height                      (in_base_mipmap_height),
      m_memory_features             (in_memory_features),
      m_mipmaps_to_upload           ((in_opt_mipmaps_ptr != nullptr) ? *in_opt_mipmaps_ptr : std::vector<MipmapRawData>() ),
      m_mt_safety                   (in_mt_safety),
      m_n_layers                    (in_n_layers),
      m_n_swapchain_image           (UINT32_MAX),
      m_post_alloc_layout           (in_post_alloc_image_layout),
      m_post_create_layout          (in_post_create_image_layout),
      m_queue_families              (in_queue_families),
      m_residency_scope             (in_residency_scope),
      m_sample_count                (in_sample_count),
      m_sharing_mode                (in_sharing_mode),
      m_swapchain_ptr               (nullptr),
      m_tiling                      (in_tiling),
      m_type                        (in_type),
      m_type_vk                     (in_type_vk),
      m_usage_flags                 (static_cast<VkImageUsageFlagBits>(in_usage)),
      m_use_full_mipmap_chain       (in_use_full_mipmap_chain),
      m_width                       (in_base_mipmap_width)
{
    /* Stub */
}
