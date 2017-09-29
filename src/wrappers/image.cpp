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
#include "misc/formats.h"
#include "misc/object_tracker.h"
#include "wrappers/buffer.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include "wrappers/physical_device.h"
#include "wrappers/queue.h"
#include "wrappers/swapchain.h"
#include <math.h>


#ifdef max
    #undef max
#endif

/* Please see header for specification */
Anvil::Image::Image(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
                    VkImageType                       in_type,
                    VkFormat                          in_format,
                    VkImageTiling                     in_tiling,
                    VkSharingMode                     in_sharing_mode,
                    VkImageUsageFlags                 in_usage,
                    uint32_t                          in_base_mipmap_width,
                    uint32_t                          in_base_mipmap_height,
                    uint32_t                          in_base_mipmap_depth,
                    uint32_t                          in_n_layers,
                    VkSampleCountFlagBits             in_sample_count,
                    bool                              in_use_full_mipmap_chain,
                    Anvil::ImageCreateFlags           in_create_flags,
                    Anvil::QueueFamilyBits            in_queue_families,
                    VkImageLayout                     in_post_create_image_layout,
                    const std::vector<MipmapRawData>* in_opt_mipmaps_ptr)
    :CallbacksSupportProvider                (IMAGE_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider              (in_device_ptr,
                                              VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT),
     m_alignment                             (0),
     m_create_flags                          (in_create_flags),
     m_depth                                 (in_base_mipmap_depth),
     m_device_ptr                            (in_device_ptr),
     m_format                                (in_format),
     m_has_transitioned_to_post_create_layout(false),
     m_height                                (in_base_mipmap_height),
     m_image                                 (VK_NULL_HANDLE),
     m_image_owner                           (true),
     m_is_sparse                             (false),
     m_is_swapchain_image                    (false),
     m_memory_types                          (0),
     m_n_mipmaps                             (0),
     m_n_layers                              (in_n_layers),
     m_n_slices                              (0),
     m_post_create_layout                    (in_post_create_image_layout),
     m_queue_families                        (in_queue_families),
     m_residency_scope                       (Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED),
     m_sample_count                          (in_sample_count),
     m_sharing_mode                          (in_sharing_mode),
     m_storage_size                          (0),
     m_tiling                                (in_tiling),
     m_type                                  (in_type),
     m_usage                                 (static_cast<VkImageUsageFlagBits>(in_usage)),
     m_uses_full_mipmap_chain                (in_use_full_mipmap_chain),
     m_width                                 (in_base_mipmap_width),
     m_memory_owner                          (false)
{
    if (in_opt_mipmaps_ptr != nullptr)
    {
        m_mipmaps_to_upload = *in_opt_mipmaps_ptr;
    }

    /* Register this instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_IMAGE,
                                                 this);

}

/* Please see header for specification */
Anvil::Image::Image(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
                    VkImageType                       in_type,
                    VkFormat                          in_format,
                    VkImageTiling                     in_tiling,
                    VkSharingMode                     in_sharing_mode,
                    VkImageUsageFlags                 in_usage,
                    uint32_t                          in_base_mipmap_width,
                    uint32_t                          in_base_mipmap_height,
                    uint32_t                          in_base_mipmap_depth,
                    uint32_t                          in_n_layers,
                    VkSampleCountFlagBits             in_sample_count,
                    Anvil::QueueFamilyBits            in_queue_families,
                    bool                              in_use_full_mipmap_chain,
                    Anvil::ImageCreateFlags           in_create_flags,
                    VkImageLayout                     in_post_create_image_layout,
                    const std::vector<MipmapRawData>* in_mipmaps_ptr)
    :CallbacksSupportProvider                (IMAGE_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider              (in_device_ptr,
                                              VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT),
     m_alignment                             (0),
     m_create_flags                          (in_create_flags),
     m_depth                                 (in_base_mipmap_depth),
     m_device_ptr                            (in_device_ptr),
     m_format                                (in_format),
     m_has_transitioned_to_post_create_layout(false),
     m_height                                (in_base_mipmap_height),
     m_image                                 (VK_NULL_HANDLE),
     m_image_owner                           (true),
     m_is_sparse                             (false),
     m_is_swapchain_image                    (false),
     m_memory_owner                          (true),
     m_memory_types                          (0),
     m_n_layers                              (in_n_layers),
     m_n_mipmaps                             (0),
     m_n_slices                              (0),
     m_post_create_layout                    (in_post_create_image_layout),
     m_queue_families                        (in_queue_families),
     m_residency_scope                       (Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED),
     m_sample_count                          (in_sample_count),
     m_sharing_mode                          (in_sharing_mode),
     m_storage_size                          (0),
     m_tiling                                (in_tiling),
     m_type                                  (in_type),
     m_usage                                 (static_cast<VkImageUsageFlagBits>(in_usage)),
     m_uses_full_mipmap_chain                (in_use_full_mipmap_chain),
     m_width                                 (in_base_mipmap_width)
{
    if (in_mipmaps_ptr != nullptr)
    {
        m_mipmaps_to_upload = *in_mipmaps_ptr;
    }

    /* Register this instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_IMAGE,
                                                 this);
}

/* Please see header for specification */
Anvil::Image::Image(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
                    VkImage                           in_image,
                    VkFormat                          in_format,
                    VkImageTiling                     in_tiling,
                    VkSharingMode                     in_sharing_mode,
                    VkImageUsageFlags                 in_usage,
                    uint32_t                          in_base_mipmap_width,
                    uint32_t                          in_base_mipmap_height,
                    uint32_t                          in_base_mipmap_depth,
                    uint32_t                          in_n_layers,
                    uint32_t                          in_n_mipmaps,
                    VkSampleCountFlagBits             in_sample_count,
                    uint32_t                          in_n_slices,
                    Anvil::ImageCreateFlags           in_create_flags,
                    Anvil::QueueFamilyBits            in_queue_families)
    :CallbacksSupportProvider                (IMAGE_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider              (in_device_ptr,
                                              VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT),
     m_alignment                             (0),
     m_create_flags                          (in_create_flags),
     m_depth                                 (in_base_mipmap_depth),
     m_device_ptr                            (in_device_ptr),
     m_format                                (in_format),
     m_has_transitioned_to_post_create_layout(true),
     m_height                                (in_base_mipmap_height),
     m_image                                 (in_image),
     m_image_owner                           (false),
     m_is_sparse                             (false),
     m_is_swapchain_image                    (false),
     m_memory_owner                          (false),
     m_memory_types                          (UINT32_MAX),
     m_n_layers                              (in_n_layers),
     m_n_mipmaps                             (in_n_mipmaps),
     m_n_slices                              (in_n_slices),
     m_post_create_layout                    (VK_IMAGE_LAYOUT_UNDEFINED),
     m_queue_families                        (in_queue_families),
     m_residency_scope                       (Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED),
     m_sample_count                          (in_sample_count),
     m_sharing_mode                          (in_sharing_mode),
     m_storage_size                          (UINT64_MAX),
     m_tiling                                (in_tiling),
     m_type                                  (VK_IMAGE_TYPE_MAX_ENUM),
     m_usage                                 (static_cast<VkImageUsageFlagBits>(in_usage)),
     m_uses_full_mipmap_chain                (false),
     m_width                                 (in_base_mipmap_width)
{
    /* Register this instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_IMAGE,
                                                 this);
}

/** Please see header for specification */
Anvil::Image::Image(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                    VkImageType                      in_type,
                    VkFormat                         in_format,
                    VkImageTiling                    in_tiling,
                    VkImageUsageFlags                in_usage,
                    uint32_t                         in_base_mipmap_width,
                    uint32_t                         in_base_mipmap_height,
                    uint32_t                         in_base_mipmap_depth,
                    uint32_t                         in_n_layers,
                    VkSampleCountFlagBits            in_sample_count,
                    Anvil::QueueFamilyBits           in_queue_families,
                    VkSharingMode                    in_sharing_mode,
                    bool                             in_use_full_mipmap_chain,
                    Anvil::ImageCreateFlags          in_create_flags,
                    Anvil::SparseResidencyScope      in_residency_scope)
    :CallbacksSupportProvider                (IMAGE_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider              (in_device_ptr,
                                              VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT),
     m_alignment                             (0),
     m_create_flags                          (in_create_flags),
     m_depth                                 (in_base_mipmap_depth),
     m_device_ptr                            (in_device_ptr),
     m_format                                (in_format),
     m_has_transitioned_to_post_create_layout(true),
     m_height                                (in_base_mipmap_height),
     m_image                                 (VK_NULL_HANDLE),
     m_image_owner                           (true),
     m_is_sparse                             (true),
     m_is_swapchain_image                    (false),
     m_memory_owner                          (false),
     m_memory_types                          (UINT32_MAX),
     m_n_layers                              (in_n_layers),
     m_n_mipmaps                             (0),
     m_n_slices                              (0),
     m_post_create_layout                    (VK_IMAGE_LAYOUT_UNDEFINED),
     m_queue_families                        (in_queue_families),
     m_residency_scope                       (in_residency_scope),
     m_sample_count                          (in_sample_count),
     m_sharing_mode                          (in_sharing_mode),
     m_storage_size                          (UINT64_MAX),
     m_tiling                                (in_tiling),
     m_type                                  (in_type),
     m_usage                                 (static_cast<VkImageUsageFlagBits>(in_usage)),
     m_uses_full_mipmap_chain                (in_use_full_mipmap_chain),
     m_width                                 (in_base_mipmap_width)
{
    /* Register this instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_IMAGE,
                                                 this);
}

/** Please see header for specification */
void Anvil::Image::change_image_layout(std::shared_ptr<Anvil::Queue>  in_queue_ptr,
                                       VkAccessFlags                  in_src_access_mask,
                                       VkImageLayout                  in_src_layout,
                                       VkAccessFlags                  in_dst_access_mask,
                                       VkImageLayout                  in_dst_layout,
                                       const VkImageSubresourceRange& in_subresource_range)
{
    std::shared_ptr<Anvil::BaseDevice>           device_locked_ptr             (m_device_ptr);
    Anvil::QueueFamilyType                       in_queue_family_type          (Anvil::QUEUE_FAMILY_TYPE_UNDEFINED);
    const uint32_t                               in_queue_family_index         (in_queue_ptr->get_queue_family_index() );
    auto                                         mem_block_ptr                 (get_memory_block() );
    std::shared_ptr<Anvil::PrimaryCommandBuffer> transition_command_buffer_ptr;

    /* mem_block_ptr is only used here in order to trigger memory allocator bakes if there are any pending. Other than that,
     * we are not going to be accessing it in this func.
     */
    ANVIL_REDUNDANT_VARIABLE(mem_block_ptr);

    in_queue_family_type          = device_locked_ptr->get_queue_family_type(in_queue_ptr->get_queue_family_index() );
    transition_command_buffer_ptr = device_locked_ptr->get_command_pool     (in_queue_family_type)->alloc_primary_level_command_buffer();

    transition_command_buffer_ptr->start_recording(true,   /* one_time_submit          */
                                                   false); /* simultaneous_use_allowed */
    {
        Anvil::ImageBarrier image_barrier(in_src_access_mask,
                                          in_dst_access_mask,
                                          true, /* in_by_region_barrier */
                                          in_src_layout,
                                          in_dst_layout,
                                          (m_sharing_mode == VK_SHARING_MODE_CONCURRENT) ? VK_QUEUE_FAMILY_IGNORED : in_queue_family_index,
                                          (m_sharing_mode == VK_SHARING_MODE_CONCURRENT) ? VK_QUEUE_FAMILY_IGNORED : in_queue_family_index,
                                          shared_from_this(),
                                          in_subresource_range);

        transition_command_buffer_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                               VK_FALSE,       /* in_by_region                   */
                                                               0,              /* in_memory_barrier_count        */
                                                               nullptr,        /* in_memory_barrier_ptrs         */
                                                               0,              /* in_buffer_memory_barrier_count */
                                                               nullptr,        /* in_buffer_memory_barrier_ptrs  */
                                                               1,              /* in_image_memory_barrier_count  */
                                                              &image_barrier);
    }
    transition_command_buffer_ptr->stop_recording();

    in_queue_ptr->submit_command_buffer(transition_command_buffer_ptr,
                                        true /* should_block */);
}

/** Please see header for specification */
std::shared_ptr<Anvil::Image> Anvil::Image::create_nonsparse(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
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
                                                             VkImageLayout                     in_post_create_image_layout,
                                                             const std::vector<MipmapRawData>* in_opt_mipmaps_ptr)
{
    std::shared_ptr<Anvil::Image> result_ptr(
        new Image(in_device_ptr,
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
                  in_post_create_image_layout,
                  in_opt_mipmaps_ptr)
    );

    result_ptr->init(in_use_full_mipmap_chain,
                     0,        /* in_memory_features                  */
                     nullptr); /* start_image_layout_ptr - irrelevant */

    return result_ptr;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Image> Anvil::Image::create_nonsparse(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
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
                                                             VkImageLayout                     in_post_create_image_layout,
                                                             const std::vector<MipmapRawData>* in_mipmaps_ptr)
{
    const VkImageLayout start_image_layout = (in_mipmaps_ptr != nullptr && in_mipmaps_ptr->size() > 0) ? VK_IMAGE_LAYOUT_PREINITIALIZED
                                                                                                       : VK_IMAGE_LAYOUT_UNDEFINED;

    std::shared_ptr<Anvil::Image> new_image_ptr(
        new Anvil::Image(in_device_ptr,
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
                         in_queue_families,
                         in_use_full_mipmap_chain,
                         in_create_flags,
                         in_post_create_image_layout,
                         in_mipmaps_ptr)
    );

    new_image_ptr->init(in_use_full_mipmap_chain,
                        in_memory_features,
                       &start_image_layout);

    return new_image_ptr;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Image> Anvil::Image::create_nonsparse(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                             const VkSwapchainCreateInfoKHR&  in_swapchain_create_info,
                                                             VkImage                          in_image)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(in_device_ptr);

    std::shared_ptr<Anvil::Image> new_image_ptr(
        new Anvil::Image(in_device_ptr,
                         in_image,
                         in_swapchain_create_info.imageFormat,
                         VK_IMAGE_TILING_OPTIMAL,
                         in_swapchain_create_info.imageSharingMode,
                         in_swapchain_create_info.imageUsage,
                         in_swapchain_create_info.imageExtent.width,
                         in_swapchain_create_info.imageExtent.height,
                         1, /* base_mipmap_depth */
                         in_swapchain_create_info.imageArrayLayers,
                         1, /* n_mipmaps */
                         VK_SAMPLE_COUNT_1_BIT,
                         1,  /* n_slices       */
                         0, /* in_create_flags */
                         Anvil::QUEUE_FAMILY_TYPE_UNDEFINED)
    );

    new_image_ptr->m_memory_types       = 0;
    new_image_ptr->m_storage_size       = 0;
    new_image_ptr->m_type               = VK_IMAGE_TYPE_2D;
    new_image_ptr->m_is_swapchain_image = true;

    new_image_ptr->init_mipmap_props();

    return new_image_ptr;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Image> Anvil::Image::create_sparse(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                          VkImageType                      in_type,
                                                          VkFormat                         in_format,
                                                          VkImageTiling                    in_tiling,
                                                          VkImageUsageFlags                in_usage,
                                                          uint32_t                         in_base_mipmap_width,
                                                          uint32_t                         in_base_mipmap_height,
                                                          uint32_t                         in_base_mipmap_depth,
                                                          uint32_t                         in_n_layers,
                                                          VkSampleCountFlagBits            in_sample_count,
                                                          Anvil::QueueFamilyBits           in_queue_families,
                                                          VkSharingMode                    in_sharing_mode,
                                                          bool                             in_use_full_mipmap_chain,
                                                          ImageCreateFlags                 in_create_flags,
                                                          Anvil::SparseResidencyScope      in_residency_scope,
                                                          VkImageLayout                    in_initial_layout)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(in_device_ptr);
    const VkPhysicalDeviceFeatures&    features         (device_locked_ptr->get_physical_device_features() );
    std::shared_ptr<Anvil::Image>      result_ptr;

    anvil_assert(in_initial_layout == VK_IMAGE_LAYOUT_PREINITIALIZED ||
                 in_initial_layout == VK_IMAGE_LAYOUT_UNDEFINED);

    /* Make sure requested functionality is supported by the device */
    if (!features.sparseBinding)
    {
        anvil_assert(features.sparseBinding);

        goto end;
    }

    if (in_residency_scope != Anvil::SPARSE_RESIDENCY_SCOPE_NONE)
    {
        /* For sparse residency, we need to verify the device can actually support the requested configuration */
        std::vector<VkSparseImageFormatProperties> result_properties;

        device_locked_ptr->get_physical_device_sparse_image_format_properties(in_format,
                                                                              in_type,
                                                                              in_sample_count,
                                                                              in_usage,
                                                                              in_tiling,
                                                                              result_properties);

        if (result_properties.size() == 0)
        {
            goto end;
        }

        switch (in_type)
        {
            case VK_IMAGE_TYPE_1D:
            {
                /* Not supported in Vulkan 1.0 */
                anvil_assert_fail();

                goto end;
            }

            case VK_IMAGE_TYPE_2D:
            {
                if (!features.sparseResidencyImage2D)
                {
                    anvil_assert_fail();

                    goto end;
                }

                break;
            }

            case VK_IMAGE_TYPE_3D:
            {
                if (!features.sparseResidencyImage3D)
                {
                    anvil_assert_fail();

                    goto end;
                }

                break;
            }

            default:
            {
                anvil_assert_fail();

                goto end;
            }
        }

        switch (in_sample_count)
        {
            case VK_SAMPLE_COUNT_1_BIT:
            {
                /* No validation required */
                break;
            }

            case VK_SAMPLE_COUNT_2_BIT:
            {
                if (!features.sparseResidency2Samples)
                {
                    anvil_assert(features.sparseResidency2Samples);

                    goto end;
                }

                break;
            }

            case VK_SAMPLE_COUNT_4_BIT:
            {
                if (!features.sparseResidency4Samples)
                {
                    anvil_assert(features.sparseResidency4Samples);

                    goto end;
                }

                break;
            }

            case VK_SAMPLE_COUNT_8_BIT:
            {
                if (!features.sparseResidency8Samples)
                {
                    anvil_assert(features.sparseResidency8Samples);

                    goto end;
                }

                break;
            }

            case VK_SAMPLE_COUNT_16_BIT:
            {
                if (!features.sparseResidency4Samples)
                {
                    anvil_assert(features.sparseResidency16Samples);

                    goto end;
                }

                break;
            }

            default:
            {
                anvil_assert_fail();

                goto end;
            }
        }
    }

    /* Spawn a new Image instance and initialize it */
    result_ptr.reset(
        new Anvil::Image(in_device_ptr,
                         in_type,
                         in_format,
                         in_tiling,
                         in_usage,
                         in_base_mipmap_width,
                         in_base_mipmap_height,
                         in_base_mipmap_depth,
                         in_n_layers,
                         in_sample_count,
                         in_queue_families,
                         in_sharing_mode,
                         in_use_full_mipmap_chain,
                         in_create_flags,
                         in_residency_scope)
    );

    result_ptr->init(in_use_full_mipmap_chain,
                     0,                  /* in_memory_features              */
                    &in_initial_layout); /* start_image_layout - irrelevant */

end:
    return result_ptr;
}

/** Please see header for specification */
bool Anvil::Image::get_aspect_subresource_layout(VkImageAspectFlags   in_aspect,
                                                 uint32_t             in_n_layer,
                                                 uint32_t             in_n_mip,
                                                 VkSubresourceLayout* out_subresource_layout_ptr) const
{
    auto aspect_iterator = m_aspects.find(static_cast<VkImageAspectFlagBits>(in_aspect) );
    bool result          = false;

    anvil_assert(m_tiling == VK_IMAGE_TILING_LINEAR);

    if (aspect_iterator != m_aspects.end() )
    {
        auto layer_mip_iterator = aspect_iterator->second.find(LayerMipKey(in_n_layer, in_n_mip) );

        if (layer_mip_iterator != aspect_iterator->second.end() )
        {
            *out_subresource_layout_ptr = layer_mip_iterator->second;
            result                      = true;
        }
    }

    return result;
}

/** Please see header for specification */
std::shared_ptr<Anvil::MemoryBlock> Anvil::Image::get_memory_block()
{
    bool is_callback_needed = false;

    if (m_is_sparse)
    {
        ImageCallbackIsAllocPendingQueryData callback_arg(shared_from_this() );

        callback(IMAGE_CALLBACK_ID_IS_ALLOC_PENDING,
                &callback_arg);

        is_callback_needed = callback_arg.result;
    }
    else
    if (m_memory_block_ptr == nullptr)
    {
        is_callback_needed = true;
    }

    if (is_callback_needed)
    {
        callback_safe(IMAGE_CALLBACK_ID_MEMORY_BLOCK_NEEDED,
                      this);
    }

    return m_memory_block_ptr;
}

/** Private function which initializes the Image instance.
 *
 *  For argument discussion, please see documentation of the constructors.
 **/
void Anvil::Image::init(bool                      in_use_full_mipmap_chain,
                        Anvil::MemoryFeatureFlags in_memory_features,
                        const VkImageLayout*      in_start_image_layout_ptr)
{
    std::vector<VkImageAspectFlags>    aspects_used;
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkImageFormatProperties            image_format_props;
    uint32_t                           max_dimension;
    uint32_t                           n_queue_family_indices  = 0;
    uint32_t                           queue_family_indices[3];
    VkResult                           result     (VK_ERROR_INITIALIZATION_FAILED);
    bool                               result_bool(false);

    ANVIL_REDUNDANT_VARIABLE(result);
    ANVIL_REDUNDANT_VARIABLE(result_bool);

    if (m_memory_owner                                           &&
        (in_memory_features & MEMORY_FEATURE_FLAG_MAPPABLE) == 0)
    {
        anvil_assert((in_memory_features & MEMORY_FEATURE_FLAG_HOST_COHERENT) == 0);
    }

    /* Determine the maximum dimension size. */
    max_dimension = m_width;

    if (max_dimension < m_height)
    {
        max_dimension = m_height;
    }

    if (max_dimension < m_depth)
    {
        max_dimension = m_depth;
    }

    /* Form the queue family array */
    Anvil::Utils::convert_queue_family_bits_to_family_indices(m_device_ptr,
                                                              m_queue_families,
                                                              queue_family_indices,
                                                             &n_queue_family_indices);

    /* Is the requested texture size valid? */
    result_bool = device_locked_ptr->get_physical_device_image_format_properties(m_format,
                                                                                 m_type,
                                                                                 m_tiling,
                                                                                 m_usage,
                                                                                 0, /* flags */
                                                                                 image_format_props);
    anvil_assert(result_bool);

    anvil_assert(m_width  >= 1 &&
                 m_height >= 1 &&
                 m_depth  >= 1);

    anvil_assert(m_width <= (uint32_t) image_format_props.maxExtent.width);

    if (m_height > 1)
    {
        anvil_assert(m_height <= (uint32_t) image_format_props.maxExtent.height);
    }

    if (m_depth > 1)
    {
        anvil_assert(m_depth <= (uint32_t) image_format_props.maxExtent.depth);
    }

    anvil_assert(m_n_layers >= 1);

    if (m_n_layers > 1)
    {
        anvil_assert(m_n_layers <= (uint32_t) image_format_props.maxArrayLayers);
    }

    /* If multisample image is requested, make sure the number of samples is supported. */
    anvil_assert(m_sample_count >= 1);

    if (m_sample_count > 1)
    {
        anvil_assert((image_format_props.sampleCounts & m_sample_count) != 0);
    }

    /* Create the image object */
    VkImageCreateInfo  image_create_info;
    VkImageCreateFlags image_flags       = 0;

    if ( (m_create_flags & Anvil::IMAGE_CREATE_FLAG_CUBE_COMPATIBLE_BIT) != 0)
    {
        anvil_assert(m_type           == VK_IMAGE_TYPE_2D);
        anvil_assert((m_n_layers % 6) == 0);

        image_flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    if ( (m_create_flags & Anvil::IMAGE_CREATE_FLAG_MUTABLE_FORMAT_BIT) != 0)
    {
        image_flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }

    if ( (m_create_flags & Anvil::IMAGE_CREATE_FLAG_2D_ARRAY_COMPATIBLE_BIT) != 0)
    {
        anvil_assert(device_locked_ptr->is_khr_maintenance1_extension_enabled() );
        anvil_assert(m_type == VK_IMAGE_TYPE_3D);

        image_flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
    }

    if (m_is_sparse)
    {
        /* Convert residency scope to Vulkan image create flags */
        switch (m_residency_scope)
        {
            case Anvil::SPARSE_RESIDENCY_SCOPE_ALIASED:    image_flags |= VK_IMAGE_CREATE_SPARSE_ALIASED_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_BINDING_BIT; break;
            case Anvil::SPARSE_RESIDENCY_SCOPE_NONALIASED: image_flags |=                                      VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_BINDING_BIT; break;
            case Anvil::SPARSE_RESIDENCY_SCOPE_NONE:       image_flags |=                                                                             VK_IMAGE_CREATE_SPARSE_BINDING_BIT; break;

            default:
            {
                anvil_assert_fail();
            }
        }
    }

    image_create_info.arrayLayers           = m_n_layers;
    image_create_info.extent.depth          = m_depth;
    image_create_info.extent.height         = m_height;
    image_create_info.extent.width          = m_width;
    image_create_info.flags                 = image_flags;
    image_create_info.format                = m_format;
    image_create_info.imageType             = m_type;
    image_create_info.initialLayout         = (in_start_image_layout_ptr != nullptr)           ? *in_start_image_layout_ptr
                                            : (m_mipmaps_to_upload.size() > 0)                 ? VK_IMAGE_LAYOUT_PREINITIALIZED
                                                                                               : VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.mipLevels             = static_cast<uint32_t>((in_use_full_mipmap_chain) ? (1 + log2(max_dimension)) : 1);
    image_create_info.pNext                 = nullptr;
    image_create_info.pQueueFamilyIndices   = queue_family_indices;
    image_create_info.queueFamilyIndexCount = n_queue_family_indices;
    image_create_info.samples               = static_cast<VkSampleCountFlagBits>(m_sample_count);
    image_create_info.sharingMode           = m_sharing_mode;
    image_create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.tiling                = m_tiling;
    image_create_info.usage                 = m_usage;

    result = vkCreateImage(device_locked_ptr->get_device_vk(),
                          &image_create_info,
                           nullptr, /* pAllocator */
                          &m_image);

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        set_vk_handle(m_image);
    }

    m_n_mipmaps = image_create_info.mipLevels;
    m_n_slices  = (m_type == VK_IMAGE_TYPE_3D) ? m_depth : 1;

    /* Extract various image properties we're going to need later */
    vkGetImageMemoryRequirements(device_locked_ptr->get_device_vk(),
                                 m_image,
                                &m_memory_reqs);

    m_alignment    = m_memory_reqs.alignment;
    m_memory_types = m_memory_reqs.memoryTypeBits;
    m_storage_size = m_memory_reqs.size;

    /* Cache aspect subresource properties if we're dealing with a linear image */
    if (m_tiling == VK_IMAGE_TILING_LINEAR)
    {
        Anvil::Formats::get_format_aspects(m_format,
                                          &aspects_used);

        for (const auto& current_aspect : aspects_used)
        {
            VkImageSubresource subresource;

            subresource.aspectMask = current_aspect;

            for (uint32_t n_layer = 0;
                          n_layer < m_n_layers;
                        ++n_layer)
            {
                subresource.arrayLayer = n_layer;

                for (uint32_t n_mip = 0;
                              n_mip < m_n_mipmaps;
                            ++n_mip)
                {
                    VkSubresourceLayout subresource_layout;

                    subresource.mipLevel = n_mip;

                    vkGetImageSubresourceLayout(device_locked_ptr->get_device_vk(),
                                                m_image,
                                               &subresource,
                                               &subresource_layout);

                    m_aspects[static_cast<VkImageAspectFlagBits>(current_aspect)][LayerMipKey(n_layer, n_mip)] = subresource_layout;
                }
            }
        }
    }

    /* Initialize mipmap props storage */
    init_mipmap_props();

    if (m_is_sparse                                     &&
        m_residency_scope != SPARSE_RESIDENCY_SCOPE_NONE)
    {
        uint32_t                                     n_reqs                  = 0;
        std::vector<VkSparseImageMemoryRequirements> sparse_image_memory_reqs;

        /* Retrieve image aspect properties. Since Vulkan lets a single props structure to refer to more than
         * just a single aspect, we first cache the exposed info in a vec and then distribute the information to
         * a map, whose key is allowed to consist of a single bit ( = individual aspect) only */
        vkGetImageSparseMemoryRequirements(device_locked_ptr->get_device_vk(),
                                           m_image,
                                          &n_reqs,
                                           nullptr);

        anvil_assert(n_reqs >= 1);

        sparse_image_memory_reqs.resize(n_reqs);

        vkGetImageSparseMemoryRequirements(device_locked_ptr->get_device_vk(),
                                           m_image,
                                          &n_reqs,
                                          &sparse_image_memory_reqs[0]);

        for (const auto& image_memory_req : sparse_image_memory_reqs)
        {
            for (uint32_t n_bit   = 0;
                   (1u << n_bit) <= image_memory_req.formatProperties.aspectMask;
                        ++n_bit)
            {
                VkImageAspectFlagBits aspect = static_cast<VkImageAspectFlagBits>(1 << n_bit);

                if ((image_memory_req.formatProperties.aspectMask & aspect) == 0)
                {
                    continue;
                }

                anvil_assert(m_sparse_aspect_props.find(aspect) == m_sparse_aspect_props.end() );

                m_sparse_aspect_props[aspect] = Anvil::SparseImageAspectProperties(image_memory_req);
            }
        }

        /* Continue by setting up storage for page occupancy data */
        init_page_occupancy(sparse_image_memory_reqs);

        /* Finally, partially-resident images require metadata aspect to be bound memory before they can be
         * accessed. Allocate & associate a dedicated memory block to the image */
        auto metadata_aspect_iterator = m_sparse_aspect_props.find(VK_IMAGE_ASPECT_METADATA_BIT);

        if (metadata_aspect_iterator != m_sparse_aspect_props.end() )
        {
            Anvil::SparseMemoryBindInfoID               metadata_binding_bind_info_id;
            Anvil::Utils::SparseMemoryBindingUpdateInfo metadata_binding_update;
            std::shared_ptr<Anvil::Queue>               sparse_queue_ptr(device_locked_ptr->get_sparse_binding_queue(0) );

            /* TODO: Right now, we only support cases where image uses only one metadata block, no matter how many
             *       layers there are.  */
            anvil_assert((metadata_aspect_iterator->second.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) != 0);

            /* Instantiate the memory block which is going to hold the metadata
             *
             * NOTE: We should use an instance-wide allocator, preferably resizable, for this.
             */
            m_metadata_memory_block_ptr = Anvil::MemoryBlock::create(m_device_ptr,
                                                                     m_memory_reqs.memoryTypeBits,
                                                                     metadata_aspect_iterator->second.mip_tail_size,
                                                                     0); /* in_memory_features */

            /* Set up bind info update structure. */
            metadata_binding_bind_info_id = metadata_binding_update.add_bind_info(0,        /* n_signal_semaphores       */
                                                                                  nullptr,  /* opt_signal_semaphores_ptr */
                                                                                  0,        /* n_wait_semaphores         */
                                                                                  nullptr); /* opt_wait_semaphores_ptr   */

            metadata_binding_update.append_opaque_image_memory_update(metadata_binding_bind_info_id,
                                                                      shared_from_this(),
                                                                      metadata_aspect_iterator->second.mip_tail_offset,
                                                                      metadata_aspect_iterator->second.mip_tail_size,
                                                                      VK_SPARSE_MEMORY_BIND_METADATA_BIT,
                                                                      m_metadata_memory_block_ptr,
                                                                      0); /* opt_memory_block_start_offset */

            result_bool = sparse_queue_ptr->bind_sparse_memory(metadata_binding_update);
            anvil_assert(result_bool);
        }
    }
    else
    if (m_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_NONE && m_is_sparse)
    {
        m_page_tracker_ptr.reset(
            new Anvil::PageTracker(m_storage_size,
                                   m_alignment)
        );
    }

    if (m_memory_owner)
    {
        anvil_assert(!m_is_sparse);

        /* Allocate memory for the image */
        auto memory_block_ptr = Anvil::MemoryBlock::create(m_device_ptr,
                                                           m_memory_reqs.memoryTypeBits,
                                                           m_memory_reqs.size,
                                                           in_memory_features);

        set_memory(memory_block_ptr);
    }
}

/** Initializes page occupancy data. Should only be used for sparse images.
 *
 *  @param memory_reqs Image memory requirements.
 **/
void Anvil::Image::init_page_occupancy(const std::vector<VkSparseImageMemoryRequirements>& in_memory_reqs)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

    anvil_assert(m_residency_scope != Anvil::SPARSE_RESIDENCY_SCOPE_NONE       &&
                 m_residency_scope != Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED);

    /* First, allocate space for an AspectPageOccupancyData instance for all aspects used by the image.
     *
     * Vulkan may interleave data of more than one aspect in one memory region, so we need to go the extra
     * mile to ensure the same AspectPageOccupancyData structure is assigned to such aspects.
     */
    for (const auto& memory_req : in_memory_reqs)
    {
        std::shared_ptr<AspectPageOccupancyData> occupancy_data_ptr;

        for (uint32_t n_aspect_bit = 0;
               (1u << n_aspect_bit) <= memory_req.formatProperties.aspectMask;
                    ++n_aspect_bit)
        {
            VkImageAspectFlagBits current_aspect = static_cast<VkImageAspectFlagBits>(1 << n_aspect_bit);

            if ((memory_req.formatProperties.aspectMask & current_aspect) == 0)
            {
                continue;
            }

            if (m_sparse_aspect_page_occupancy.find(current_aspect) != m_sparse_aspect_page_occupancy.end() )
            {
                occupancy_data_ptr = m_sparse_aspect_page_occupancy.at(current_aspect);

                break;
            }
        }

        if (occupancy_data_ptr == nullptr)
        {
            occupancy_data_ptr.reset(
                new AspectPageOccupancyData()
            );
        }

        for (uint32_t n_aspect_bit = 0;
               (1u << n_aspect_bit) <= memory_req.formatProperties.aspectMask;
                    ++n_aspect_bit)
        {
            VkImageAspectFlagBits current_aspect = static_cast<VkImageAspectFlagBits>(1 << n_aspect_bit);

            if ((memory_req.formatProperties.aspectMask & current_aspect) == 0)
            {
                continue;
            }

            m_sparse_aspect_page_occupancy[current_aspect] = occupancy_data_ptr;
        }
    }

    /* Next, iterate over each aspect and initialize storage for the mip chain */
    anvil_assert(m_memory_reqs.alignment != 0);

    for (auto occupancy_iterator  = m_sparse_aspect_page_occupancy.begin();
              occupancy_iterator != m_sparse_aspect_page_occupancy.end();
            ++occupancy_iterator)
    {
        const VkImageAspectFlagBits&                    current_aspect                = occupancy_iterator->first;
        decltype(m_sparse_aspect_props)::const_iterator current_aspect_props_iterator;
        std::shared_ptr<AspectPageOccupancyData>        page_occupancy_ptr            = occupancy_iterator->second;

        if (page_occupancy_ptr->layers.size() > 0)
        {
            /* Already initialized */
            continue;
        }

        if (current_aspect == VK_IMAGE_ASPECT_METADATA_BIT)
        {
            /* Don't initialize per-layer occupancy data for metadata aspect */
            continue;
        }

        current_aspect_props_iterator = m_sparse_aspect_props.find(current_aspect);
        anvil_assert(current_aspect_props_iterator != m_sparse_aspect_props.end() );

        anvil_assert(current_aspect_props_iterator->second.granularity.width  >= 1); 
        anvil_assert(current_aspect_props_iterator->second.granularity.height >= 1);
        anvil_assert(current_aspect_props_iterator->second.granularity.depth  >= 1);

        /* Initialize storage for layer data.. */
        for (uint32_t n_layer = 0;
                      n_layer < m_n_layers;
                    ++n_layer)
        {
            page_occupancy_ptr->layers.push_back(AspectPageOccupancyLayerData() );

            auto& current_layer = page_occupancy_ptr->layers.back();

            /* Tail can be, but does not necessarily have to be a part of non-zero layers. Take this into account,
             * when determining how many pages we need to alloc space for it. */
            if (current_aspect_props_iterator->second.mip_tail_size > 0)
            {
                const bool is_single_miptail = (current_aspect_props_iterator->second.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) != 0;

                if ((is_single_miptail && n_layer == 0) ||
                    !is_single_miptail)
                {
                    anvil_assert( (current_aspect_props_iterator->second.mip_tail_size % m_memory_reqs.alignment) == 0);

                    current_layer.n_total_tail_pages = static_cast<uint32_t>(current_aspect_props_iterator->second.mip_tail_size / m_memory_reqs.alignment);

                    current_layer.tail_occupancy.resize(1 + current_layer.n_total_tail_pages / (sizeof(PageOccupancyStatus) * 8 /* bits in byte */) );
                }
                else
                {
                    current_layer.n_total_tail_pages = 0;
                }
            }
            else
            {
                current_layer.n_total_tail_pages = 0;
            }


            for (const auto& current_mip : m_mipmaps)
            {
                AspectPageOccupancyLayerMipData mip_data(current_mip.width,
                                                         current_mip.height,
                                                         current_mip.depth,
                                                         current_aspect_props_iterator->second.granularity.width,
                                                         current_aspect_props_iterator->second.granularity.height,
                                                         current_aspect_props_iterator->second.granularity.depth);

                anvil_assert(current_mip.width  >= 1);
                anvil_assert(current_mip.height >= 1);
                anvil_assert(current_mip.depth  >= 1);

                current_layer.mips.push_back(mip_data);
            }
        }
    }
}

/** Releases the Vulkan image object, as well as the memory object associated with the Image instance. */
Anvil::Image::~Image()
{
    if (m_image       != VK_NULL_HANDLE &&
        m_image_owner)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroyImage(device_locked_ptr->get_device_vk(),
                       m_image,
                       nullptr /* pAllocator */);

        m_image = VK_NULL_HANDLE;
    }

    m_memory_block_ptr.reset();

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_IMAGE,
                                                   this);
}

/* Please see header for specification */
const VkImage& Anvil::Image::get_image()
{
    if (!m_is_sparse)
    {
        if (m_memory_block_ptr == nullptr)
        {
            get_memory_block();
        }
    }

    return m_image;
}

/* Please see header for specification */
VkExtent2D Anvil::Image::get_image_extent_2D(uint32_t in_n_mipmap) const
{
    VkExtent2D result;
    uint32_t   size[2];

    if (!get_image_mipmap_size(in_n_mipmap,
                               size + 0,
                               size + 1,
                               nullptr) )
    {
        anvil_assert_fail();

        goto end;
    }

    result.height = size[1];
    result.width  = size[0];

end:
    return result;
}

/* Please see header for specification */
VkExtent3D Anvil::Image::get_image_extent_3D(uint32_t in_n_mipmap) const
{
    VkExtent3D result;
    uint32_t   size[3];

    if (!get_image_mipmap_size(in_n_mipmap,
                               size + 0,
                               size + 1,
                               size + 2) )
    {
        anvil_assert_fail();

        goto end;
    }

    result.depth  = size[2];
    result.height = size[1];
    result.width  = size[0];

end:
    return result;
}

/* Please see header for specification */
bool Anvil::Image::get_image_mipmap_size(uint32_t  in_n_mipmap,
                                         uint32_t* out_opt_width_ptr,
                                         uint32_t* out_opt_height_ptr,
                                         uint32_t* out_opt_depth_ptr) const
{
    bool result = false;

    /* Is this a sensible mipmap index? */
    if (m_mipmaps.size() <= in_n_mipmap)
    {
        goto end;
    }

    /* Return the result data.. */
    if (out_opt_width_ptr != nullptr)
    {
        *out_opt_width_ptr = m_mipmaps[in_n_mipmap].width;
    }

    if (out_opt_height_ptr != nullptr)
    {
        *out_opt_height_ptr = m_mipmaps[in_n_mipmap].height;
    }

    if (out_opt_depth_ptr != nullptr)
    {
        *out_opt_depth_ptr = m_mipmaps[in_n_mipmap].depth;
    }

    /* All done */
    result = true;

end:
    return result;
}

/** Please see header for specification */
bool Anvil::Image::get_sparse_image_aspect_properties(const VkImageAspectFlagBits                in_aspect,
                                                      const Anvil::SparseImageAspectProperties** out_result_ptr_ptr) const
{
    decltype(m_sparse_aspect_props)::const_iterator prop_iterator;
    bool                                            result = false;

    if (!m_is_sparse)
    {
        anvil_assert(m_is_sparse);

        goto end;
    }

    if (m_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_NONE)
    {
        anvil_assert(m_residency_scope != Anvil::SPARSE_RESIDENCY_SCOPE_NONE);

        goto end;
    }

    prop_iterator = m_sparse_aspect_props.find(in_aspect);

    if (prop_iterator == m_sparse_aspect_props.end() )
    {
        anvil_assert(prop_iterator != m_sparse_aspect_props.end() );

        goto end;
    }

    *out_result_ptr_ptr = &prop_iterator->second;
    result              = true;

end:
    return result;
}

/** TODO */
VkImageCreateInfo Anvil::Image::get_create_info_for_swapchain(std::shared_ptr<const Anvil::Swapchain> in_swapchain_ptr)
{
    VkImageCreateInfo result;

    in_swapchain_ptr->get_image(0)->get_image_mipmap_size(0, /* n_mipmap */
                                                         &result.extent.width,
                                                         &result.extent.height,
                                                         &result.arrayLayers);

    result.extent.depth          = 1;
    result.flags                 = in_swapchain_ptr->get_flags();
    result.format                = in_swapchain_ptr->get_image_format();
    result.imageType             = VK_IMAGE_TYPE_2D;
    result.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    result.mipLevels             = 1;
    result.pNext                 = nullptr;
    result.pQueueFamilyIndices   = nullptr;
    result.queueFamilyIndexCount = UINT32_MAX;
    result.samples               = VK_SAMPLE_COUNT_1_BIT;
    result.sharingMode           = in_swapchain_ptr->get_image(0)->get_image_sharing_mode();
    result.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    result.tiling                = VK_IMAGE_TILING_OPTIMAL;
    result.usage                 = in_swapchain_ptr->get_image(0)->get_image_usage();

    return result;
}

/** Please see header for specification */
VkImageSubresourceRange Anvil::Image::get_subresource_range() const
{
    VkImageSubresourceRange result;

    switch (m_format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        {
            result.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            break;
        }

        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        {
            result.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

            break;
        }

        case VK_FORMAT_S8_UINT:
        {
            result.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;

            break;
        }

        default:
        {
            result.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            break;
        }
    }

    result.baseArrayLayer = 0;
    result.baseMipLevel   = 0;
    result.layerCount     = m_n_layers;
    result.levelCount     = m_n_mipmaps;

    return result;
}

/** Please see header for specification */
bool Anvil::Image::has_aspects(VkImageAspectFlags in_aspects) const
{
    VkImageAspectFlags checked_aspects = 0;
    bool               result          = true;

    if (m_is_sparse                                      &&
        m_residency_scope != SPARSE_RESIDENCY_SCOPE_NONE)
    {
        for (uint32_t n_bit = 0;
                      n_bit < sizeof(uint32_t) * 8 /* bits in byte */ && result;
                    ++n_bit)
        {
            VkImageAspectFlagBits current_aspect = static_cast<VkImageAspectFlagBits>(1 << n_bit);

            if ((in_aspects & current_aspect)              != 0 &&
                m_sparse_aspect_props.find(current_aspect) == m_sparse_aspect_props.end() )
            {
                result = false;
            }
        }
    }
    else
    {
        const Anvil::ComponentLayout component_layout       = Anvil::Formats::get_format_component_layout(m_format);
        bool                         has_color_components   = false;
        bool                         has_depth_components   = false;
        bool                         has_stencil_components = false;

        switch (component_layout)
        {
            case COMPONENT_LAYOUT_ABGR:
            case COMPONENT_LAYOUT_ARGB:
            case COMPONENT_LAYOUT_BGR:
            case COMPONENT_LAYOUT_BGRA:
            case COMPONENT_LAYOUT_EBGR:
            case COMPONENT_LAYOUT_R:
            case COMPONENT_LAYOUT_RG:
            case COMPONENT_LAYOUT_RGB:
            case COMPONENT_LAYOUT_RGBA:
            {
                has_color_components = true;

                break;
            }

            case COMPONENT_LAYOUT_D:
            case COMPONENT_LAYOUT_XD:
            {
                has_depth_components = true;

                break;
            }

            case COMPONENT_LAYOUT_DS:
            {
                has_depth_components   = true;
                has_stencil_components = true;

                break;
            }

            case COMPONENT_LAYOUT_S:
            {
                has_stencil_components = true;

                break;
            }

            default:
            {
                anvil_assert_fail();
            }
        }

        if (in_aspects & VK_IMAGE_ASPECT_COLOR_BIT)
        {
            result &= has_color_components;

            checked_aspects |= VK_IMAGE_ASPECT_COLOR_BIT;
        }

        if (result && (in_aspects & VK_IMAGE_ASPECT_DEPTH_BIT) != 0)
        {
            result &= has_depth_components;

            checked_aspects |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        if (result && (in_aspects & VK_IMAGE_ASPECT_STENCIL_BIT) != 0)
        {
            result &= has_stencil_components;

            checked_aspects |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        anvil_assert(!result                                   ||
                      result && checked_aspects == in_aspects);
    }

    return result;
}

/** Fills the m_mipmaps vector with mipmap size data. */
void Anvil::Image::init_mipmap_props()
{
    uint32_t current_mipmap_size[3] =
    {
        m_width,
        m_height,
        m_depth,
    };

    anvil_assert(m_n_mipmaps      != 0);
    anvil_assert(m_mipmaps.size() == 0);

    for (uint32_t mipmap_level = 0;
                  mipmap_level < m_n_mipmaps;
                ++mipmap_level)
    {
        m_mipmaps.push_back(Mipmap(current_mipmap_size[0],
                                   current_mipmap_size[1],
                                   current_mipmap_size[2]) );

        current_mipmap_size[0] /= 2;
        current_mipmap_size[1] /= 2;

        if (m_type == VK_IMAGE_TYPE_3D)
        {
            current_mipmap_size[2] /= 2;
        }

        if (current_mipmap_size[0] < 1)
        {
            current_mipmap_size[0] = 1;
        }

        if (current_mipmap_size[1] < 1)
        {
            current_mipmap_size[1] = 1;
        }

        if (current_mipmap_size[2] < 1)
        {
            current_mipmap_size[2] = 1;
        }
    }
}

/* Please see header for specification */
bool Anvil::Image::is_memory_bound_for_texel(VkImageAspectFlagBits in_aspect,
                                             uint32_t              in_n_layer,
                                             uint32_t              in_n_mip,
                                             uint32_t              in_x,
                                             uint32_t              in_y,
                                             uint32_t              in_z) const
{
    bool result = false;

    /* Sanity checks */
    anvil_assert(m_is_sparse);
    anvil_assert(m_residency_scope == SPARSE_RESIDENCY_SCOPE_ALIASED     ||
                 m_residency_scope == SPARSE_RESIDENCY_SCOPE_NONALIASED);

    anvil_assert(m_n_layers  > in_n_layer);
    anvil_assert(m_n_mipmaps > in_n_mip);

    anvil_assert(m_sparse_aspect_page_occupancy.find(in_aspect) != m_sparse_aspect_page_occupancy.end() );

    /* Retrieve the tile status */
    const auto& aspect_data = m_sparse_aspect_props.at(in_aspect);
    const auto& layer_data  = m_sparse_aspect_page_occupancy.at(in_aspect)->layers.at(in_n_layer);

    if (in_n_mip >= aspect_data.mip_tail_first_lod)
    {
        /* For tails, we only have enough info to work at layer granularity */
        const VkDeviceSize layer_start_offset = aspect_data.mip_tail_offset + aspect_data.mip_tail_stride * in_n_layer;

        ANVIL_REDUNDANT_VARIABLE_CONST(layer_start_offset);

        /* TODO */
        result = true;
    }
    else
    {
        const auto&    mip_data   = layer_data.mips.at(in_n_mip);
        const uint32_t tile_index = mip_data.get_texture_space_xyz_to_block_mapping_index(in_x, in_y, in_z);

        result = (mip_data.tile_to_block_mappings.at(tile_index) != nullptr);
    }

    return result;
}

/** Updates page tracker (for non-resident images) OR tile-to-block mappings & tail page counteres,
 *  as per the specified opaque image memory update properties.
 *
 *  @param in_resource_offset           Raw image data offset, from which the update has been performed
 *  @param in_size                      Size of the updated region.
 *  @param in_memory_block_ptr          Memory block, bound to the specified region.
 *  @param in_memory_block_start_offset Start offset relative to @param memory_block_ptr used for the update.
 **/
void Anvil::Image::on_memory_backing_opaque_update(VkDeviceSize                        in_resource_offset,
                                                   VkDeviceSize                        in_size,
                                                   std::shared_ptr<Anvil::MemoryBlock> in_memory_block_ptr,
                                                   VkDeviceSize                        in_memory_block_start_offset)
{
    const bool is_unbinding = (in_memory_block_ptr == nullptr);

    /* Sanity checks */
    anvil_assert(m_is_sparse);

    if (in_memory_block_ptr != nullptr)
    {
        anvil_assert(in_memory_block_ptr->get_size() <= in_memory_block_start_offset + in_size);
    }

    if (m_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_NONE)
    {
        /* Non-resident image: underlying memory is viewed as an opaque linear region. */
        m_page_tracker_ptr->set_binding(in_memory_block_ptr,
                                        in_memory_block_start_offset,
                                        in_resource_offset,
                                        in_size);
    }
    else
    {
        /* The following use cases are expected to make us reach this block:
         *
         * 1) Application is about to bind a memory backing to all tiles OR unbind memory backing
         *    from all tiles forming all aspects at once.
         *
         * 2) Application wants to bind (or unbind) a memory backing to/from miptail tile(s).
         *
         * 3) Anvil::Image has requested to bind memory to the metadata aspect. At the moment, this can only
         *    be invoked from within the wrapper's code, and the memory block used for the operation is owned
         *    by the Image wrapper, so there's nothing we need to do here.
         */
        bool is_miptail_tile_binding_operation = false;
        auto metadata_aspect_iterator          = m_sparse_aspect_props.find(VK_IMAGE_ASPECT_METADATA_BIT);

        if (metadata_aspect_iterator != m_sparse_aspect_props.end() )
        {
            /* Handle case 3) */
            if (in_resource_offset == metadata_aspect_iterator->second.mip_tail_offset &&
                in_size            == metadata_aspect_iterator->second.mip_tail_size)
            {
                return;
            }
            else
            {
                /* TODO: We do not currently support cases where the application tries to bind its own memory
                 *       block to the metadata aspect.
                 */
                anvil_assert(in_resource_offset < metadata_aspect_iterator->second.mip_tail_offset);
            }
        }

        /* Handle case 2) */
        for (auto aspect_iterator  = m_sparse_aspect_page_occupancy.begin();
                  aspect_iterator != m_sparse_aspect_page_occupancy.end()   && !is_miptail_tile_binding_operation;
                ++aspect_iterator)
        {
            VkImageAspectFlagBits              current_aspect       = aspect_iterator->first;
            const SparseImageAspectProperties& current_aspect_props = m_sparse_aspect_props.at(current_aspect);
            auto                               occupancy_data_ptr   = aspect_iterator->second;

            ANVIL_REDUNDANT_VARIABLE_CONST(current_aspect_props);

            if ( in_resource_offset       != 0 &&
                !is_miptail_tile_binding_operation)
            {
                is_miptail_tile_binding_operation = (in_size == current_aspect_props.mip_tail_size);

                if (is_miptail_tile_binding_operation)
                {
                    is_miptail_tile_binding_operation = false;

                    for (uint32_t n_layer = 0;
                                  n_layer < m_n_layers;
                                ++n_layer)
                    {
                        auto& current_layer = occupancy_data_ptr->layers.at(n_layer);

                        if (in_resource_offset == current_aspect_props.mip_tail_offset + current_aspect_props.mip_tail_stride * n_layer &&
                            in_size            == current_aspect_props.mip_tail_size)
                        {
                            is_miptail_tile_binding_operation = true;

                            memset(&current_layer.tail_occupancy[0],
                                   (!is_unbinding) ? ~0 : 0,
                                   current_layer.tail_occupancy.size() * sizeof(current_layer.tail_occupancy[0]));

                            if (!is_unbinding)
                            {
                                current_layer.tail_pages_per_binding[in_memory_block_ptr] = current_layer.n_total_tail_pages;
                            }
                            else
                            {
                                current_layer.tail_pages_per_binding.clear();
                            }

                            break;
                        }
                    }
                }
            }
        }

        /* If not case 2) and 3), this has got to be case 1) */
        if (!is_miptail_tile_binding_operation)
        {
            for (auto aspect_iterator  = m_sparse_aspect_page_occupancy.begin();
                      aspect_iterator != m_sparse_aspect_page_occupancy.end();
                    ++aspect_iterator)
            {
                auto occupancy_data_ptr = aspect_iterator->second;

                for (auto& current_layer : occupancy_data_ptr->layers)
                {
                    if (in_resource_offset == 0)
                    {
                        for (auto& current_mip : current_layer.mips)
                        {
                            const uint32_t n_mip_tiles = static_cast<uint32_t>(current_mip.tile_to_block_mappings.size() );

                            for (uint32_t n_mip_tile = 0;
                                          n_mip_tile < n_mip_tiles;
                                        ++n_mip_tile)
                            {
                                current_mip.tile_to_block_mappings.at(n_mip_tile) = in_memory_block_ptr;
                            }
                        }
                    }
                }
            }
        }
    }
}

/** Updates page tracker (for non-resident images) OR tile-to-block mappings & tail page counteres,
 *  as per the specified opaque image update properties.
 *
 *  @param in_subresource               Subresource specified for the update.
 *  @param in_offset                    Offset specified for the update.
 *  @param in_extent                    Extent specified for the update.
 *  @param in_memory_block_ptr          Memory block, bound to the specified region.
 *  @param in_memory_block_start_offset Start offset relative to @param memory_block_ptr used for the update.
 **/
void Anvil::Image::on_memory_backing_update(const VkImageSubresource&           in_subresource,
                                            VkOffset3D                          in_offset,
                                            VkExtent3D                          in_extent,
                                            std::shared_ptr<Anvil::MemoryBlock> in_memory_block_ptr,
                                            VkDeviceSize                        in_memory_block_start_offset)
{
    AspectPageOccupancyLayerData*                      aspect_layer_ptr;
    AspectPageOccupancyLayerMipData*                   aspect_layer_mip_ptr;
    decltype(m_sparse_aspect_page_occupancy)::iterator aspect_page_occupancy_iterator;
    decltype(m_sparse_aspect_props)::iterator          aspect_props_iterator;

    anvil_assert(m_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_ALIASED     ||
                 m_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_NONALIASED);

    ANVIL_REDUNDANT_ARGUMENT(in_memory_block_start_offset);


    if (in_subresource.aspectMask == VK_IMAGE_ASPECT_METADATA_BIT)
    {
        /* Metadata is not tracked since it needs to be completely bound in order for the sparse image to be usable. */
        anvil_assert_fail();

        return;
    }

    aspect_page_occupancy_iterator = m_sparse_aspect_page_occupancy.find(static_cast<VkImageAspectFlagBits>(in_subresource.aspectMask) );
    aspect_props_iterator          = m_sparse_aspect_props.find         (static_cast<VkImageAspectFlagBits>(in_subresource.aspectMask) );

    anvil_assert(aspect_page_occupancy_iterator != m_sparse_aspect_page_occupancy.end() );
    anvil_assert(aspect_props_iterator          != m_sparse_aspect_props.end() );

    anvil_assert((in_offset.x      % aspect_props_iterator->second.granularity.width)  == 0 &&
                 (in_offset.y      % aspect_props_iterator->second.granularity.height) == 0 &&
                 (in_offset.z      % aspect_props_iterator->second.granularity.depth)  == 0);
    anvil_assert((in_extent.width  % aspect_props_iterator->second.granularity.width)  == 0 &&
                 (in_extent.height % aspect_props_iterator->second.granularity.height) == 0 &&
                 (in_extent.depth  % aspect_props_iterator->second.granularity.depth)  == 0);

    anvil_assert(aspect_page_occupancy_iterator->second->layers.size() >= in_subresource.arrayLayer);
    aspect_layer_ptr = &aspect_page_occupancy_iterator->second->layers.at(in_subresource.arrayLayer);

    anvil_assert(aspect_layer_ptr->mips.size() > in_subresource.mipLevel);
    aspect_layer_mip_ptr = &aspect_layer_ptr->mips.at(in_subresource.mipLevel);

    const uint32_t extent_tile[] =
    {
        in_extent.width  / aspect_props_iterator->second.granularity.width,
        in_extent.height / aspect_props_iterator->second.granularity.height,
        in_extent.depth  / aspect_props_iterator->second.granularity.depth
    };
    const uint32_t offset_tile[] =
    {
        in_offset.x / aspect_props_iterator->second.granularity.width,
        in_offset.y / aspect_props_iterator->second.granularity.height,
        in_offset.z / aspect_props_iterator->second.granularity.depth
    };

    /* We're going to favor readability over performance in the loops below. */
    for (uint32_t current_x_tile = offset_tile[0];
                  current_x_tile < offset_tile[0] + extent_tile[0];
                ++current_x_tile)
    {
        for (uint32_t current_y_tile = offset_tile[1];
                      current_y_tile < offset_tile[1] + extent_tile[1];
                    ++current_y_tile)
        {
            for (uint32_t current_z_tile = offset_tile[2];
                          current_z_tile < offset_tile[2] + extent_tile[2];
                        ++current_z_tile)
            {
                const uint32_t tile_index = aspect_layer_mip_ptr->get_tile_space_xyz_to_block_mapping_index(current_x_tile,
                                                                                                            current_y_tile,
                                                                                                            current_z_tile);

                anvil_assert(aspect_layer_mip_ptr->tile_to_block_mappings.size() > tile_index);

                /* Assign the memory block (potentially null) to the tile */
                aspect_layer_mip_ptr->tile_to_block_mappings.at(tile_index) = in_memory_block_ptr;
            }
        }
    }
}

/* Please see header for specification */
bool Anvil::Image::set_memory(std::shared_ptr<Anvil::MemoryBlock> in_memory_block_ptr)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    const Anvil::DeviceType            device_type      (device_locked_ptr->get_type() );
    VkResult                           result           (VK_ERROR_INITIALIZATION_FAILED);

    /* Sanity checks */
    anvil_assert(in_memory_block_ptr != nullptr);
    anvil_assert(!m_is_sparse);
    anvil_assert(m_mipmaps.size()   >  0);
    anvil_assert(m_memory_block_ptr == nullptr);

    /* Bind the memory object to the image object */
    if (device_type == Anvil::DEVICE_TYPE_SINGLE_GPU)
    {
        result = vkBindImageMemory(device_locked_ptr->get_device_vk(),
                                   m_image,
                                   in_memory_block_ptr->get_memory(),
                                   in_memory_block_ptr->get_start_offset() );
    }

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        VkImageLayout src_image_layout = (m_tiling == VK_IMAGE_TILING_LINEAR && m_mipmaps_to_upload.size() > 0) ? VK_IMAGE_LAYOUT_PREINITIALIZED
                                                                                                                : VK_IMAGE_LAYOUT_UNDEFINED;

        m_memory_block_ptr = in_memory_block_ptr;

        /* Fill the storage with mipmap contents, if mipmap data was specified at input */
        if (m_mipmaps_to_upload.size() > 0)
        {
            upload_mipmaps(&m_mipmaps_to_upload,
                           src_image_layout,
                          &src_image_layout);
        }

        if (m_post_create_layout != VK_IMAGE_LAYOUT_PREINITIALIZED &&
            m_post_create_layout != VK_IMAGE_LAYOUT_UNDEFINED)
        {
            const uint32_t n_mipmaps_to_upload = static_cast<uint32_t>(m_mipmaps_to_upload.size());
            VkAccessFlags  src_access_mask     = 0;

            if (n_mipmaps_to_upload > 0)
            {
                if (m_tiling == VK_IMAGE_TILING_LINEAR)
                {
                    src_access_mask = VK_ACCESS_HOST_WRITE_BIT;
                }
                else
                {
                    src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
                }
            }

            transition_to_post_create_image_layout(src_access_mask,
                                                   src_image_layout);
        }

        m_mipmaps_to_upload.clear();
    }

    return is_vk_call_successful(result);
}

/* Transitions the underlying Vulkan image to the layout stored in m_post_create_layout.
 *
 * @param in_source_access_mask All access types used to fill the image with data.
 * @param in_src_layout         Layout to transition from.
 **/
void Anvil::Image::transition_to_post_create_image_layout(VkAccessFlags in_source_access_mask,
                                                          VkImageLayout in_src_layout)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

    anvil_assert(!m_has_transitioned_to_post_create_layout);

    change_image_layout(device_locked_ptr->get_universal_queue(0),
                        in_source_access_mask,
                        in_src_layout,
                        Anvil::Utils::get_access_mask_from_image_layout(m_post_create_layout),
                        m_post_create_layout,
                        get_subresource_range() );

    m_has_transitioned_to_post_create_layout = true;
}

/** Please see header for specification */
void Anvil::Image::upload_mipmaps(const std::vector<MipmapRawData>* in_mipmaps_ptr,
                                  VkImageLayout                     in_current_image_layout,
                                  VkImageLayout*                    out_new_image_layout_ptr)
{
    std::shared_ptr<Anvil::BaseDevice>                                         device_locked_ptr                  (m_device_ptr);
    std::map<VkImageAspectFlagBits, std::vector<const Anvil::MipmapRawData*> > image_aspect_to_mipmap_raw_data_map;
    VkImageAspectFlags                                                         image_aspects_touched              (0);
    std::shared_ptr<Anvil::MemoryBlock>                                        image_mem_block_ptr;
    VkImageSubresourceRange                                                    image_subresource_range;
    const VkDeviceSize                                                         sparse_page_size                   ( (m_page_tracker_ptr != nullptr) ? m_page_tracker_ptr->get_page_size() : 0);
    const std::shared_ptr<Anvil::Queue>                                        universal_queue_ptr                (device_locked_ptr->get_universal_queue(0) );

    /* Make sure image has been assigned at least one memory block before we go ahead with the upload process */
    image_mem_block_ptr = get_memory_block();

    /* Each image aspect needs to be modified separately. Iterate over the input vector and move MipmapRawData
     * to separate vectors corresponding to which aspect they need to update. */
    for (auto mipmap_iterator =  in_mipmaps_ptr->cbegin();
              mipmap_iterator != in_mipmaps_ptr->cend();
            ++mipmap_iterator)
    {
        image_aspect_to_mipmap_raw_data_map[mipmap_iterator->aspect].push_back(&(*mipmap_iterator));
    }

    for (auto mipmap_iterator =  in_mipmaps_ptr->cbegin();
              mipmap_iterator != in_mipmaps_ptr->cend();
            ++mipmap_iterator)
    {
        image_aspects_touched |= mipmap_iterator->aspect;
    }

    /* Fill the buffer memory with data, according to the specified layout requirements,
     * if linear tiling is used.
     *
     * For optimal tiling, we need to copy the raw data to temporary buffer
     * and use vkCmdCopyBufferToImage() to let the driver rearrange the data as needed.
     */
    image_subresource_range.aspectMask     = image_aspects_touched;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.baseMipLevel   = 0;
    image_subresource_range.layerCount     = m_n_layers;
    image_subresource_range.levelCount     = m_n_mipmaps;

    if (m_tiling == VK_IMAGE_TILING_LINEAR)
    {
        /* TODO: Transition the subresource ranges, if necessary. */
        anvil_assert(in_current_image_layout != VK_IMAGE_LAYOUT_UNDEFINED);

        for (auto   aspect_to_mipmap_data_iterator  = image_aspect_to_mipmap_raw_data_map.begin();
                    aspect_to_mipmap_data_iterator != image_aspect_to_mipmap_raw_data_map.end();
                  ++aspect_to_mipmap_data_iterator)
        {
            VkImageAspectFlagBits current_aspect        = aspect_to_mipmap_data_iterator->first;
            const auto&           mipmap_raw_data_items = aspect_to_mipmap_data_iterator->second;

            for (auto mipmap_raw_data_item_iterator  = mipmap_raw_data_items.begin();
                      mipmap_raw_data_item_iterator != mipmap_raw_data_items.end();
                    ++mipmap_raw_data_item_iterator)
            {
                const unsigned char* current_mipmap_data_ptr          = nullptr;
                uint32_t             current_mipmap_height            = 0;
                const auto&          current_mipmap_raw_data_item_ptr = *mipmap_raw_data_item_iterator;
                uint32_t             current_mipmap_slices            = 0;
                uint32_t             current_row_size                 = 0;
                uint32_t             current_slice_size               = 0;
                VkDeviceSize         dst_slice_offset                 = 0;
                const unsigned char* src_slice_ptr                    = nullptr;
                VkImageSubresource   image_subresource;
                VkSubresourceLayout  image_subresource_layout;

                current_mipmap_data_ptr = (current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_uchar_ptr     != nullptr) ? current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_uchar_ptr.get()
                                        : (current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_uchar_raw_ptr != nullptr) ? current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_uchar_raw_ptr
                                                                                                                                  : &(*current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_uchar_vec_ptr)[0];

                image_subresource.arrayLayer = current_mipmap_raw_data_item_ptr->n_layer;
                image_subresource.aspectMask = current_aspect;
                image_subresource.mipLevel   = current_mipmap_raw_data_item_ptr->n_mipmap;

                vkGetImageSubresourceLayout(device_locked_ptr->get_device_vk(),
                                            m_image,
                                           &image_subresource,
                                           &image_subresource_layout);

                /* Determine row size for the mipmap.
                 *
                 * NOTE: Current implementation can only handle power-of-two textures.
                 */
                anvil_assert(m_height == 1 || (m_height % 2) == 0);

                current_mipmap_height = m_height / (1 << current_mipmap_raw_data_item_ptr->n_mipmap);

                if (current_mipmap_height < 1)
                {
                    current_mipmap_height = 1;
                }

                current_mipmap_slices = current_mipmap_raw_data_item_ptr->n_slices;
                current_row_size      = current_mipmap_raw_data_item_ptr->row_size;
                current_slice_size    = (current_row_size * current_mipmap_height);

                if (current_mipmap_slices < 1)
                {
                    current_mipmap_slices = 1;
                }

                for (unsigned int n_slice = 0;
                                  n_slice < current_mipmap_slices;
                                ++n_slice)
                {
                    dst_slice_offset = image_subresource_layout.offset               +
                                       image_subresource_layout.depthPitch * n_slice;
                    src_slice_ptr    = current_mipmap_data_ptr                       +
                                       current_slice_size                  * n_slice;

                    for (unsigned int n_row = 0;
                                      n_row < current_mipmap_height;
                                    ++n_row, dst_slice_offset += image_subresource_layout.rowPitch)
                    {
                        std::shared_ptr<Anvil::MemoryBlock> mem_block_ptr;
                        VkDeviceSize                        write_dst_slice_offset = dst_slice_offset;

                        if (!m_is_sparse)
                        {
                            mem_block_ptr = m_memory_block_ptr;
                        }
                        else
                        if (m_residency_scope == SPARSE_RESIDENCY_SCOPE_NONE)
                        {
                            const VkDeviceSize dst_slice_offset_page_aligned = Anvil::Utils::round_down(dst_slice_offset,
                                                                                                        sparse_page_size);
                            VkDeviceSize       memory_region_start_offset    = UINT32_MAX;

                            anvil_assert(sparse_page_size                                     != 0);
                            anvil_assert(( dst_slice_offset_page_aligned % sparse_page_size)  == 0);

                            mem_block_ptr = m_page_tracker_ptr->get_memory_block(dst_slice_offset_page_aligned,
                                                                                &memory_region_start_offset);
                            anvil_assert(mem_block_ptr != nullptr);

                            if (dst_slice_offset + image_subresource_layout.rowPitch > dst_slice_offset_page_aligned + sparse_page_size)
                            {
                                VkDeviceSize dummy;
                                auto         mem_block2_ptr = m_page_tracker_ptr->get_memory_block(dst_slice_offset_page_aligned + sparse_page_size,
                                                                                                  &dummy);

                                // todo: the slice spans across >1 memory blocks. need more than just one write op to handle this case correctly.
                                anvil_assert(mem_block2_ptr == mem_block_ptr);
                            }

                            write_dst_slice_offset = memory_region_start_offset + (dst_slice_offset - dst_slice_offset_page_aligned);
                        }
                        else
                        {
                            /* todo */
                            anvil_assert_fail();
                        }

                        mem_block_ptr->write(write_dst_slice_offset,
                                             current_row_size,
                                             src_slice_ptr + current_row_size * n_row);
                    }
                }
            }
        }

        *out_new_image_layout_ptr = VK_IMAGE_LAYOUT_PREINITIALIZED;
    }
    else
    {
        anvil_assert(m_tiling == VK_IMAGE_TILING_OPTIMAL);

        std::shared_ptr<Anvil::Buffer>               temp_buffer_ptr;
        std::shared_ptr<Anvil::PrimaryCommandBuffer> temp_cmdbuf_ptr;
        VkDeviceSize                                 total_raw_mips_size = 0;

        /* Count how much space all specified mipmaps take in raw format. */
        for (auto mipmap_iterator  = in_mipmaps_ptr->cbegin();
                  mipmap_iterator != in_mipmaps_ptr->cend();
                ++mipmap_iterator)
        {
            total_raw_mips_size += mipmap_iterator->n_slices * mipmap_iterator->data_size;

            /* Mip offsets must be rounded up to 4 due to the following "Valid Usage" requirement of VkBufferImageCopy struct:
             *
             * "bufferOffset must be a multiple of 4"
             */
            if ((total_raw_mips_size % 4) != 0)
            {
                total_raw_mips_size += (4 - (total_raw_mips_size % 4));
            }
        }

        /* Merge data of all mips into one buffer, cache the offsets and push the merged data
         * to the buffer memory. */
        std::vector<VkDeviceSize> mip_data_offsets;

        VkDeviceSize          current_mip_offset = 0;
        std::unique_ptr<char> merged_mip_storage(new char[static_cast<uint32_t>(total_raw_mips_size)]);

        /* NOTE: The memcpy() call, as well as the way we implement copy op calls below, assume
         *       POT resolution of the base mipmap
         */
        anvil_assert(m_height < 2 || (m_height % 2) == 0);

        for (auto mipmap_iterator  = in_mipmaps_ptr->cbegin();
                  mipmap_iterator != in_mipmaps_ptr->cend();
                ++mipmap_iterator)
        {
            const auto&          current_mipmap          = *mipmap_iterator;
            const unsigned char* current_mipmap_data_ptr;

            current_mipmap_data_ptr = (mipmap_iterator->linear_tightly_packed_data_uchar_ptr     != nullptr) ? mipmap_iterator->linear_tightly_packed_data_uchar_ptr.get()
                                    : (mipmap_iterator->linear_tightly_packed_data_uchar_raw_ptr != nullptr) ? mipmap_iterator->linear_tightly_packed_data_uchar_raw_ptr
                                                                                                             : &(*mipmap_iterator->linear_tightly_packed_data_uchar_vec_ptr)[0];

            mip_data_offsets.push_back(current_mip_offset);

            memcpy(merged_mip_storage.get() + current_mip_offset,
                   current_mipmap_data_ptr,
                   current_mipmap.n_slices * current_mipmap.data_size);

            current_mip_offset += current_mipmap.n_slices * current_mipmap.data_size;

            /* Mip offset must be rounded up to 4 due to the following "Valid Usage" requirement of VkBufferImageCopy struct:
             *
             * "bufferOffset must be a multiple of 4"
             */
            if ((current_mip_offset % 4) != 0)
            {
                current_mip_offset = Anvil::Utils::round_up(current_mip_offset, static_cast<VkDeviceSize>(4) );
            }
        }

        temp_buffer_ptr = Anvil::Buffer::create_nonsparse(m_device_ptr,
                                                          total_raw_mips_size,
                                                          Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
                                                          VK_SHARING_MODE_EXCLUSIVE,
                                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                          0, /* in_memory_features */
                                                          merged_mip_storage.get() );

        merged_mip_storage.reset();

        /* Set up a command buffer we will use to copy the data to the image */
        temp_cmdbuf_ptr = device_locked_ptr->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)->alloc_primary_level_command_buffer();
        anvil_assert(temp_cmdbuf_ptr != nullptr);

        temp_cmdbuf_ptr->start_recording(true, /* one_time_submit          */
                                         false /* simultaneous_use_allowed */);
        {
            std::vector<VkBufferImageCopy> copy_regions;

            /* Transfer the image to the transfer_destination layout if not already in this or general layout */
            if (in_current_image_layout != VK_IMAGE_LAYOUT_GENERAL              &&
                in_current_image_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                Anvil::ImageBarrier image_barrier(0, /* source_access_mask */
                                                   VK_ACCESS_TRANSFER_WRITE_BIT,
                                                   false,
                                                   in_current_image_layout,
                                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                   (m_sharing_mode == VK_SHARING_MODE_EXCLUSIVE) ? universal_queue_ptr->get_queue_family_index() : VK_QUEUE_FAMILY_IGNORED,
                                                   (m_sharing_mode == VK_SHARING_MODE_EXCLUSIVE) ? universal_queue_ptr->get_queue_family_index() : VK_QUEUE_FAMILY_IGNORED,
                                                   shared_from_this(),
                                                   image_subresource_range);

                temp_cmdbuf_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                         VK_FALSE,       /* in_by_region                   */
                                                         0,              /* in_memory_barrier_count        */
                                                         nullptr,        /* in_memory_barrier_ptrs         */
                                                         0,              /* in_buffer_memory_barrier_count */
                                                         nullptr,        /* in_buffer_memory_barrier_ptrs  */
                                                         1,              /* in_image_memory_barrier_count  */
                                                        &image_barrier);

                *out_new_image_layout_ptr = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            }
            else
            {
                *out_new_image_layout_ptr = in_current_image_layout;
            }

            /* Issue the buffer->image copy op */
            copy_regions.reserve(in_mipmaps_ptr->size() );

            for (auto mipmap_iterator  = in_mipmaps_ptr->cbegin();
                      mipmap_iterator != in_mipmaps_ptr->cend();
                    ++mipmap_iterator)
            {
                VkBufferImageCopy current_copy_region;
                const auto&       current_mipmap = *mipmap_iterator;

                current_copy_region.bufferImageHeight               = std::max(m_height / (1 << current_mipmap.n_mipmap), 1u);
                current_copy_region.bufferOffset                    = mip_data_offsets[static_cast<uint32_t>(mipmap_iterator - in_mipmaps_ptr->cbegin()) ];
                current_copy_region.bufferRowLength                 = 0;
                current_copy_region.imageOffset.x                   = 0;
                current_copy_region.imageOffset.y                   = 0;
                current_copy_region.imageOffset.z                   = 0;
                current_copy_region.imageSubresource.baseArrayLayer = current_mipmap.n_layer;
                current_copy_region.imageSubresource.layerCount     = current_mipmap.n_layers;
                current_copy_region.imageSubresource.aspectMask     = current_mipmap.aspect;
                current_copy_region.imageSubresource.mipLevel       = current_mipmap.n_mipmap;
                current_copy_region.imageExtent.depth               = current_mipmap.n_slices;
                current_copy_region.imageExtent.height              = std::max(m_height / (1 << current_mipmap.n_mipmap), 1u);
                current_copy_region.imageExtent.width               = std::max(m_width  / (1 << current_mipmap.n_mipmap), 1u);

                if (current_copy_region.imageExtent.depth < 1)
                {
                    current_copy_region.imageExtent.depth = 1;
                }

                if (current_copy_region.imageExtent.height < 1)
                {
                    current_copy_region.imageExtent.height = 1;
                }

                if (current_copy_region.imageExtent.width < 1)
                {
                    current_copy_region.imageExtent.width = 1;
                }

                copy_regions.push_back(current_copy_region);
            }

            temp_cmdbuf_ptr->record_copy_buffer_to_image(temp_buffer_ptr,
                                                         shared_from_this(),
                                                         *out_new_image_layout_ptr,
                                                         static_cast<uint32_t>(copy_regions.size() ),
                                                        &copy_regions[0]);
        }
        temp_cmdbuf_ptr->stop_recording();

        /* Execute the command buffer */
        universal_queue_ptr->submit_command_buffer(temp_cmdbuf_ptr,
                                                   true /* should_block */);
    }
}