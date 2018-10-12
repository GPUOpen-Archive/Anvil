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

Anvil::ImageCreateInfoUniquePtr Anvil::ImageCreateInfo::create_no_alloc(const Anvil::BaseDevice*          in_device_ptr,
                                                                        Anvil::ImageType                  in_type,
                                                                        Anvil::Format                     in_format,
                                                                        Anvil::ImageTiling                in_tiling,
                                                                        Anvil::ImageUsageFlags            in_usage,
                                                                        uint32_t                          in_base_mipmap_width,
                                                                        uint32_t                          in_base_mipmap_height,
                                                                        uint32_t                          in_base_mipmap_depth,
                                                                        uint32_t                          in_n_layers,
                                                                        Anvil::SampleCountFlagBits        in_sample_count,
                                                                        Anvil::QueueFamilyFlags           in_queue_families,
                                                                        Anvil::SharingMode                in_sharing_mode,
                                                                        bool                              in_use_full_mipmap_chain,
                                                                        ImageCreateFlags                  in_create_flags,
                                                                        Anvil::ImageLayout                in_post_alloc_image_layout,
                                                                        const std::vector<MipmapRawData>* in_opt_mipmaps_ptr)
{
    Anvil::ImageCreateInfoUniquePtr result_ptr(nullptr,
                                               std::default_delete<Anvil::ImageCreateInfo>() );

    if ((in_create_flags & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT)   != 0 ||
        (in_create_flags & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0)
    {
        in_post_alloc_image_layout = Anvil::ImageLayout::UNDEFINED;
    }

    result_ptr.reset(
        new ImageCreateInfo(Anvil::ImageInternalType::NO_ALLOC,
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
                            ((in_opt_mipmaps_ptr != nullptr) && (in_opt_mipmaps_ptr->size() > 0)) ? Anvil::ImageLayout::PREINITIALIZED : Anvil::ImageLayout::UNDEFINED,
                            in_post_alloc_image_layout,
                            in_opt_mipmaps_ptr,
                            Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE,
                            Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
                            Anvil::MemoryFeatureFlagBits::NONE) 
    );

    return result_ptr;
}

Anvil::ImageCreateInfoUniquePtr Anvil::ImageCreateInfo::create_alloc(const Anvil::BaseDevice*          in_device_ptr,
                                                                     Anvil::ImageType                  in_type,
                                                                     Anvil::Format                     in_format,
                                                                     Anvil::ImageTiling                in_tiling,
                                                                     Anvil::ImageUsageFlags            in_usage,
                                                                     uint32_t                          in_base_mipmap_width,
                                                                     uint32_t                          in_base_mipmap_height,
                                                                     uint32_t                          in_base_mipmap_depth,
                                                                     uint32_t                          in_n_layers,
                                                                     Anvil::SampleCountFlagBits        in_sample_count,
                                                                     Anvil::QueueFamilyFlags           in_queue_families,
                                                                     Anvil::SharingMode                in_sharing_mode,
                                                                     bool                              in_use_full_mipmap_chain,
                                                                     MemoryFeatureFlags                in_memory_features,
                                                                     ImageCreateFlags                  in_create_flags,
                                                                     Anvil::ImageLayout                in_post_alloc_image_layout,
                                                                     const std::vector<MipmapRawData>* in_opt_mipmaps_ptr)
{
    Anvil::ImageCreateInfoUniquePtr result_ptr(nullptr,
                                               std::default_delete<Anvil::ImageCreateInfo>() );

    anvil_assert((in_create_flags & Anvil::ImageCreateFlagBits::SPARSE_ALIASED_BIT)   == 0);
    anvil_assert((in_create_flags & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT)   == 0);
    anvil_assert((in_create_flags & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) == 0);

    result_ptr.reset(
        new ImageCreateInfo(Anvil::ImageInternalType::ALLOC,
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
                            ((in_opt_mipmaps_ptr != nullptr) && (in_opt_mipmaps_ptr->size() > 0)) ? Anvil::ImageLayout::PREINITIALIZED : Anvil::ImageLayout::UNDEFINED,
                            in_post_alloc_image_layout,
                            in_opt_mipmaps_ptr,
                            Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE,
                            Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
                            in_memory_features) 
    );

    return result_ptr;
}

Anvil::ImageCreateInfoUniquePtr Anvil::ImageCreateInfo::create_peer_no_alloc(const Anvil::BaseDevice* in_device_ptr,
                                                                             const Anvil::Swapchain*  in_swapchain_ptr,
                                                                             uint32_t                 in_n_swapchain_image)
{
    const auto                      memory_features        ((in_device_ptr->get_type() == Anvil::DeviceType::MULTI_GPU) ? Anvil::MemoryFeatureFlagBits::MULTI_INSTANCE_BIT
                                                                                                                        : Anvil::MemoryFeatureFlagBits::NONE);
    Anvil::ImageCreateInfoUniquePtr result_ptr             (nullptr,
                                                            std::default_delete<Anvil::ImageCreateInfo>() );
    Anvil::Image*                   swapchain_image_ptr    (in_swapchain_ptr->get_image(in_n_swapchain_image) );
    uint32_t                        swapchain_image_size[3]{0, 0, 0};

    swapchain_image_ptr->get_image_mipmap_size(0, /* n_mipmap */
                                               swapchain_image_size + 0,
                                               swapchain_image_size + 1,
                                               swapchain_image_size + 2);

    result_ptr.reset(
        new ImageCreateInfo(Anvil::ImageInternalType::PEER_NO_ALLOC,
                            in_device_ptr,
                            Anvil::ImageType::_2D,
                            in_swapchain_ptr->get_create_info_ptr()->get_format(),
                            Anvil::ImageTiling::OPTIMAL,
                            swapchain_image_ptr->get_create_info_ptr()->get_sharing_mode(),
                            swapchain_image_ptr->get_create_info_ptr()->get_usage_flags (),
                            swapchain_image_size[0],
                            swapchain_image_size[1],
                            1,                                                                /* in_base_mipmap_depth */
                            swapchain_image_ptr->get_create_info_ptr()->get_n_layers(),
                            Anvil::SampleCountFlagBits::_1_BIT,
                            false, /* in_use_full_mipmap_chain */
                            Anvil::ImageCreateFlagBits::NONE,
                            swapchain_image_ptr->get_create_info_ptr()->get_queue_families(),
                            Anvil::ImageLayout::UNDEFINED,                                        /* in_post_create_image_layout */
                            Anvil::ImageLayout::UNDEFINED,                                        /* in_post_alloc_image_layout  */
                            nullptr,                                                              /* in_opt_mipmaps_ptr          */
                            Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE,
                            Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
                            memory_features) 
    );

    result_ptr->set_swapchain            (in_swapchain_ptr);
    result_ptr->set_swapchain_image_index(in_n_swapchain_image);

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
        new Anvil::ImageCreateInfo(Anvil::ImageInternalType::SWAPCHAIN_WRAPPER,
                                   in_device_ptr,
                                   Anvil::ImageType::_2D,
                                   swapchain_create_info_ptr->get_format(),
                                   Anvil::ImageTiling::OPTIMAL,
                                   Anvil::SharingMode::EXCLUSIVE,
                                   swapchain_create_info_ptr->get_usage_flags(),
                                   swapchain_create_info_ptr->get_rendering_surface()->get_width (),
                                   swapchain_create_info_ptr->get_rendering_surface()->get_height(),
                                   1, /* base_mipmap_depth */
                                   1, /* in_n_layers       */
                                   Anvil::SampleCountFlagBits::_1_BIT,
                                   false, /* in_use_full_mipmap_chain */
                                   Anvil::ImageCreateFlagBits::NONE,
                                   Anvil::QueueFamilyFlagBits::NONE,
                                   Anvil::ImageLayout::UNDEFINED,
                                   Anvil::ImageLayout::UNDEFINED,
                                   nullptr, /* in_opt_mipmaps_ptr */
                                   Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE,
                                   Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
                                   Anvil::MemoryFeatureFlagBits::NONE)
    );

    if (result_ptr != nullptr)
    {
        result_ptr->m_swapchain_image   = in_image;
        result_ptr->m_n_swapchain_image = in_n_swapchain_image;
        result_ptr->m_swapchain_ptr     = in_swapchain_ptr;
    }

    return result_ptr;
}

Anvil::ImageCreateInfo::ImageCreateInfo(Anvil::ImageInternalType             in_internal_type,
                                        const Anvil::BaseDevice*             in_device_ptr,
                                        Anvil::ImageType                     in_type_vk,
                                        Anvil::Format                        in_format,
                                        Anvil::ImageTiling                   in_tiling,
                                        Anvil::SharingMode                   in_sharing_mode, 
                                        Anvil::ImageUsageFlags               in_usage,
                                        uint32_t                             in_base_mipmap_width,
                                        uint32_t                             in_base_mipmap_height,
                                        uint32_t                             in_base_mipmap_depth,
                                        uint32_t                             in_n_layers,
                                        Anvil::SampleCountFlagBits           in_sample_count,
                                        bool                                 in_use_full_mipmap_chain,
                                        ImageCreateFlags                     in_create_flags,
                                        Anvil::QueueFamilyFlags              in_queue_families,
                                        Anvil::ImageLayout                   in_post_create_image_layout,
                                        const Anvil::ImageLayout&            in_post_alloc_image_layout,
                                        const std::vector<MipmapRawData>*    in_opt_mipmaps_ptr,
                                        const Anvil::MTSafety&               in_mt_safety,
                                        Anvil::ExternalMemoryHandleTypeFlags in_exportable_external_memory_handle_types,
                                        const Anvil::MemoryFeatureFlags&     in_memory_features)
     :m_create_flags                           (in_create_flags),
      m_depth                                  (in_base_mipmap_depth),
      m_device_ptr                             (in_device_ptr),
      m_exportable_external_memory_handle_types(in_exportable_external_memory_handle_types),
      m_format                                 (in_format),
      m_height                                 (in_base_mipmap_height),
      m_internal_type                          (in_internal_type),
      m_memory_features                        (in_memory_features),
      m_mipmaps_to_upload                      ((in_opt_mipmaps_ptr != nullptr) ? *in_opt_mipmaps_ptr : std::vector<MipmapRawData>() ),
      m_mt_safety                              (in_mt_safety),
      m_n_layers                               (in_n_layers),
      m_n_swapchain_image                      (UINT32_MAX),
      m_post_alloc_layout                      (in_post_alloc_image_layout),
      m_post_create_layout                     (in_post_create_image_layout),
      m_queue_families                         (in_queue_families),
      m_sample_count                           (in_sample_count),
      m_sharing_mode                           (in_sharing_mode),
      m_swapchain_ptr                          (nullptr),
      m_tiling                                 (in_tiling),
      m_type_vk                                (in_type_vk),
      m_usage_flags                            (in_usage),
      m_use_full_mipmap_chain                  (in_use_full_mipmap_chain),
      m_width                                  (in_base_mipmap_width)
{
    #if defined(_DEBUG)
    {
        if ((m_create_flags & Anvil::ImageCreateFlagBits::BLOCK_TEXEL_VIEW_COMPATIBLE_BIT) != 0)
        {
            anvil_assert((m_create_flags & Anvil::ImageCreateFlagBits::MUTABLE_FORMAT_BIT) != 0);
            anvil_assert(m_use_full_mipmap_chain                                           == false);
            anvil_assert(m_n_layers                                                        == 1);
        }
    }
    #endif
}

/* Please see header for specification */
void Anvil::ImageCreateInfo::get_image_view_formats(uint32_t*             out_n_image_view_formats_ptr,
                                                    const Anvil::Format** out_image_view_formats_ptr_ptr) const
{
    const uint32_t n_image_view_formats = static_cast<uint32_t>(m_image_view_formats.size() );

    *out_n_image_view_formats_ptr   = n_image_view_formats;
    *out_image_view_formats_ptr_ptr = (n_image_view_formats != 0) ? &m_image_view_formats.at(0) : nullptr;
}

/* Please see header for specification */
void Anvil::ImageCreateInfo::set_image_view_formats(const uint32_t&      in_n_image_view_formats,
                                                    const Anvil::Format* in_image_view_formats_ptr)
{
    anvil_assert(in_n_image_view_formats != 0);

    m_image_view_formats.resize(in_n_image_view_formats);

    memcpy(&m_image_view_formats.at(0),
           in_image_view_formats_ptr,
           sizeof(Anvil::Format) * in_n_image_view_formats);
}
