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

#include "misc/buffer_create_info.h"
#include "misc/debug.h"
#include "misc/formats.h"
#include "misc/image_create_info.h"
#include "misc/memory_block_create_info.h"
#include "misc/object_tracker.h"
#include "misc/struct_chainer.h"
#include "misc/swapchain_create_info.h"
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

#ifdef min
    #undef min
#endif

/* Please see header for specification */
Anvil::Image::Image(Anvil::ImageCreateInfoUniquePtr in_create_info_ptr)
    :CallbacksSupportProvider               (IMAGE_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider             (in_create_info_ptr->get_device(),
                                             Anvil::ObjectType::IMAGE),
     MTSafetySupportProvider                (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                             in_create_info_ptr->get_device   () )),
     m_has_transitioned_to_post_alloc_layout(false),
     m_image                                (VK_NULL_HANDLE),
     m_n_mipmaps                            (0),
     m_swapchain_memory_assigned            (false)
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    /* Register this instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::IMAGE,
                                                 this);

}

/** Releases the Vulkan image object, as well as the memory object associated with the Image instance. */
Anvil::Image::~Image()
{
    if (m_image                                != VK_NULL_HANDLE                              &&
        m_create_info_ptr->get_internal_type() != Anvil::ImageInternalType::SWAPCHAIN_WRAPPER)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyImage(m_device_ptr->get_device_vk(),
                                          m_image,
                                          nullptr /* pAllocator */);
        }
        unlock();

        m_image = VK_NULL_HANDLE;
    }

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::IMAGE,
                                                   this);
}

/** Please see header for specification */
void Anvil::Image::change_image_layout(Anvil::Queue*                       in_queue_ptr,
                                       Anvil::AccessFlags                  in_src_access_mask,
                                       Anvil::ImageLayout                  in_src_layout,
                                       Anvil::AccessFlags                  in_dst_access_mask,
                                       Anvil::ImageLayout                  in_dst_layout,
                                       const Anvil::ImageSubresourceRange& in_subresource_range,
                                       const uint32_t                      in_opt_n_wait_semaphores,
                                       const Anvil::PipelineStageFlags*    in_opt_wait_dst_stage_mask_ptrs,
                                       Anvil::Semaphore* const*            in_opt_wait_semaphore_ptrs,
                                       const uint32_t                      in_opt_n_set_semaphores,
                                       Anvil::Semaphore* const*            in_opt_set_semaphore_ptrs)
{
    const Anvil::DeviceType              device_type                  (m_device_ptr->get_type() );
    const uint32_t                       in_queue_family_index        (in_queue_ptr->get_queue_family_index() );
    auto                                 mem_block_ptr                (get_memory_block() );
    Anvil::PrimaryCommandBufferUniquePtr transition_command_buffer_ptr;

    /* mem_block_ptr is only used here in order to trigger memory allocator bakes if there are any pending. Other than that,
     * we are not going to be accessing it in this func.
     */
    ANVIL_REDUNDANT_VARIABLE(mem_block_ptr);

    transition_command_buffer_ptr = m_device_ptr->get_command_pool_for_queue_family_index(in_queue_ptr->get_queue_family_index())->alloc_primary_level_command_buffer();

    transition_command_buffer_ptr->start_recording(true,   /* one_time_submit          */
                                                   false); /* simultaneous_use_allowed */
    {
        const auto          sharing_mode   (m_create_info_ptr->get_sharing_mode() );
        const auto          queue_fam_index((sharing_mode == Anvil::SharingMode::CONCURRENT) ? VK_QUEUE_FAMILY_IGNORED : in_queue_family_index);

        Anvil::ImageBarrier image_barrier  (in_src_access_mask,
                                            in_dst_access_mask,
                                            in_src_layout,
                                            in_dst_layout,
                                            queue_fam_index,
                                            queue_fam_index,
                                            this,
                                            in_subresource_range);

        transition_command_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                               Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                               Anvil::DependencyFlagBits::NONE,
                                                               0,              /* in_memory_barrier_count        */
                                                               nullptr,        /* in_memory_barrier_ptrs         */
                                                               0,              /* in_buffer_memory_barrier_count */
                                                               nullptr,        /* in_buffer_memory_barrier_ptrs  */
                                                               1,              /* in_image_memory_barrier_count  */
                                                              &image_barrier);
    }
    transition_command_buffer_ptr->stop_recording();

    if (device_type == Anvil::DeviceType::SINGLE_GPU)
    {
        Anvil::CommandBufferBase* cmd_buffer_ptr = transition_command_buffer_ptr.get();

        in_queue_ptr->submit(
            Anvil::SubmitInfo::create(1, /* in_n_cmd_bufers */
                                     &cmd_buffer_ptr,
                                      in_opt_n_set_semaphores,
                                      in_opt_set_semaphore_ptrs,
                                      in_opt_n_wait_semaphores,
                                      in_opt_wait_semaphore_ptrs,
                                      in_opt_wait_dst_stage_mask_ptrs,
                                      true /* should_block */)
        );
    }
    else
    {
        Anvil::CommandBufferMGPUSubmission cmd_buffer_submission;
        const Anvil::MGPUDevice*           mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );

        cmd_buffer_submission.cmd_buffer_ptr = transition_command_buffer_ptr.get      ();
        cmd_buffer_submission.device_mask    = (1 << mgpu_device_ptr->get_n_physical_devices()) - 1;

        /* TODO */
        anvil_assert(in_opt_n_set_semaphores  == 0);
        anvil_assert(in_opt_n_wait_semaphores == 0);

        in_queue_ptr->submit(
            Anvil::SubmitInfo::create_execute(&cmd_buffer_submission,
                                              1, /* in_n_command_buffer_submissions */
                                              true /* should_block */)
        );
    }
}

/** Please see header for specification */
Anvil::ImageUniquePtr Anvil::Image::create(Anvil::ImageCreateInfoUniquePtr in_create_info_ptr)
{
    ImageUniquePtr result_ptr(new Image(std::move(in_create_info_ptr) ),
                              std::default_delete<Image>() );

    if (result_ptr != nullptr)
    {
        const auto image_type = result_ptr->m_create_info_ptr->get_internal_type();

        switch (image_type)
        {
            case Anvil::ImageInternalType::ALLOC:
            case Anvil::ImageInternalType::NO_ALLOC:
            case Anvil::ImageInternalType::PEER_NO_ALLOC:
            {
                if (!result_ptr->init() )
                {
                    result_ptr.reset();
                }

                break;
            }

            case Anvil::ImageInternalType::SWAPCHAIN_WRAPPER:
            {
                result_ptr->m_image     = result_ptr->m_create_info_ptr->get_swapchain_image();
                result_ptr->m_n_mipmaps = 1;

                /* NOTE: Swapchain never uses YUV image format. */
                auto& plane_props = result_ptr->m_plane_index_to_memory_properties_map[0];

                plane_props.alignment                     = 0;
                plane_props.memory_types                  = 0;
                plane_props.prefers_dedicated_allocation  = false;
                plane_props.requires_dedicated_allocation = false;
                plane_props.storage_size                  = 0;

                anvil_assert(result_ptr->m_image != VK_NULL_HANDLE);

                result_ptr->init_mipmap_props();

                if ((result_ptr->m_create_info_ptr->get_swapchain()->get_create_info_ptr()->get_flags() & Anvil::SwapchainCreateFlagBits::SPLIT_INSTANCE_BIND_REGIONS_BIT) != 0)
                {
                    result_ptr->init_sfr_tile_size();
                }

                break;
            }

            default:
            {
                anvil_assert_fail();

                result_ptr.reset();
            }
        }

        if (image_type == Anvil::ImageInternalType::PEER_NO_ALLOC)
        {
            const std::vector<uint32_t> physical_device_indices = result_ptr->m_create_info_ptr->get_device_indices();
            const auto&                 sfr_rects               = result_ptr->m_create_info_ptr->get_sfr_rects();
            const auto                  n_sfr_rects             = static_cast<uint32_t>(sfr_rects.size() );

            result_ptr->set_swapchain_memory_internal(result_ptr->m_create_info_ptr->get_swapchain_image_index(),
                                                      n_sfr_rects,
                                                      (n_sfr_rects > 0) ? &sfr_rects.at(0) : nullptr,
                                                      static_cast<uint32_t>(physical_device_indices.size() ),
                                                      (physical_device_indices.size() > 0) ? &physical_device_indices.at(0) : nullptr);
        }
    }

    return result_ptr;
}

/** TODO */
bool Anvil::Image::do_sanity_checks_for_physical_device_binding(const Anvil::MemoryBlock* in_memory_block_ptr,
                                                                uint32_t                  in_n_physical_devices) const
{
    const uint32_t           memory_block_type_index(in_memory_block_ptr->get_create_info_ptr()->get_memory_type_index() );
    const Anvil::MGPUDevice* mgpu_device_ptr        (dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );
    bool                     result                 (false);

    if (!m_device_ptr->is_extension_enabled(VK_KHR_DEVICE_GROUP_EXTENSION_NAME) )
    {
        anvil_assert(m_device_ptr->is_extension_enabled(VK_KHR_DEVICE_GROUP_EXTENSION_NAME) );

        goto end;
    }

    if (in_n_physical_devices != mgpu_device_ptr->get_n_physical_devices() )
    {
        anvil_assert(in_n_physical_devices == mgpu_device_ptr->get_n_physical_devices());

        goto end;
    }

    if ((m_device_ptr->get_physical_device_memory_properties().types[memory_block_type_index].heap_ptr->flags & Anvil::MemoryHeapFlagBits::MULTI_INSTANCE_BIT_KHR) == 0)
    {
        anvil_assert((m_device_ptr->get_physical_device_memory_properties().types[memory_block_type_index].heap_ptr->flags & Anvil::MemoryHeapFlagBits::MULTI_INSTANCE_BIT_KHR) != 0);

        goto end;
    }

    result = true;

end:
    return result;
}

/** TODO */
bool Anvil::Image::do_sanity_checks_for_sfr_binding(uint32_t        in_n_SFR_rects,
                                                    const VkRect2D* in_SFRs_ptr) const
{
    const Anvil::DeviceType                         device_type                  (m_device_ptr->get_type() );
    bool                                            result                       (false);
    const Anvil::SparseImageFormatProperties*       sparse_image_format_props_ptr(nullptr);
    std::vector<Anvil::SparseImageFormatProperties> sparse_image_format_props_vec;

    anvil_assert( m_create_info_ptr->get_internal_type()                                                   == Anvil::ImageInternalType::NO_ALLOC &&
                 (m_create_info_ptr->get_create_flags () & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT) == 0);
    anvil_assert(m_mipmap_props.size                 () >  0);
    anvil_assert(m_memory_blocks_owned.size          () == 0);

    if (!m_device_ptr->is_extension_enabled(VK_KHR_DEVICE_GROUP_EXTENSION_NAME) )
    {
        anvil_assert(m_device_ptr->is_extension_enabled(VK_KHR_DEVICE_GROUP_EXTENSION_NAME) );

        goto end;
    }

    if (device_type != Anvil::DeviceType::MULTI_GPU)
    {
        anvil_assert(device_type == Anvil::DeviceType::MULTI_GPU);

        goto end;
    }

    if (!m_device_ptr->get_physical_device_sparse_image_format_properties(m_create_info_ptr->get_format      (),
                                                                          m_create_info_ptr->get_type        (),
                                                                          m_create_info_ptr->get_sample_count(),
                                                                          m_create_info_ptr->get_usage_flags (),
                                                                          m_create_info_ptr->get_tiling      (),
                                                                          sparse_image_format_props_vec) )
    {
        anvil_assert_fail();

        goto end;
    }

    if (sparse_image_format_props_vec.size() != 1)
    {
        anvil_assert(sparse_image_format_props_vec.size() == 1);

        goto end;
    }

    if (!m_device_ptr->get_physical_device_sparse_image_format_properties(m_create_info_ptr->get_format      (),
                                                                          m_create_info_ptr->get_type        (),
                                                                          m_create_info_ptr->get_sample_count(),
                                                                          m_create_info_ptr->get_usage_flags (),
                                                                          m_create_info_ptr->get_tiling      (),
                                                                          sparse_image_format_props_vec) )
    {
        anvil_assert_fail();

        goto end;
    }

    sparse_image_format_props_ptr = &sparse_image_format_props_vec.at(0);

    for (uint32_t n_rect = 0;
                  n_rect < in_n_SFR_rects;
                ++n_rect)
    {
        const auto& current_rect = in_SFRs_ptr[n_rect];

        if ((current_rect.offset.x % sparse_image_format_props_ptr->image_granularity.width)  != 0 ||
            (current_rect.offset.y % sparse_image_format_props_ptr->image_granularity.height) != 0)
        {
            anvil_assert_fail();

            goto end;
        }

        if (((current_rect.offset.x + current_rect.extent.width)  % sparse_image_format_props_ptr->image_granularity.width)  != 0 &&
            ((current_rect.offset.x + current_rect.extent.height) % sparse_image_format_props_ptr->image_granularity.height) != 0)
        {
            anvil_assert_fail();

            goto end;
        }
    }

    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::Image::get_aspect_subresource_layout(Anvil::ImageAspectFlagBits in_aspect,
                                                 uint32_t                   in_n_layer,
                                                 uint32_t                   in_n_mip,
                                                 Anvil::SubresourceLayout*  out_subresource_layout_ptr) const
{
    auto aspect_iterator = m_linear_image_aspect_data.find(in_aspect);
    bool result          = false;

    anvil_assert(m_create_info_ptr->get_tiling() == Anvil::ImageTiling::LINEAR);

    if (aspect_iterator != m_linear_image_aspect_data.end() )
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
Anvil::MemoryBlock* Anvil::Image::get_memory_block(const uint32_t& in_n_plane)
{
    bool       is_callback_needed   = false;
    const auto is_prt               = (m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0;
    const auto uses_sparse_bindings = (m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT)   != 0;

    if (uses_sparse_bindings)
    {
        IsImageMemoryAllocPendingQueryCallbackArgument callback_arg(this);

        callback(IMAGE_CALLBACK_ID_IS_ALLOC_PENDING,
                &callback_arg);

        is_callback_needed = callback_arg.result;
    }
    else
    if (m_memory_blocks_owned.size() == 0)
    {
        is_callback_needed = true;
    }

    if (is_callback_needed)
    {
        OnMemoryBlockNeededForImageCallbackArgument callback_argument(this);

        callback_safe(IMAGE_CALLBACK_ID_MEMORY_BLOCK_NEEDED,
                     &callback_argument);
    }

    if (uses_sparse_bindings)
    {
        if (!is_prt)
        {
            const auto& plane_memory_reqs = m_plane_index_to_memory_properties_map.at(in_n_plane);

            anvil_assert(plane_memory_reqs.page_tracker_ptr != nullptr);

            return plane_memory_reqs.page_tracker_ptr->get_memory_block(0);
        }
        else
        {
            /* More than just one memory block may exist. You need to use page tracker manually. */
            return nullptr;
        }
    }
    else
    {
        /* TODO: Refactoring needed for better support of disjoint YUV images .. */
        anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::CREATE_DISJOINT_BIT) == 0);

        return (m_memory_blocks_owned.size() > 0) ? m_memory_blocks_owned.at(0).get()
                                                  : nullptr;
    }
}

/** Private function which initializes the Image instance.
 *
 *  For argument discussion, please see documentation of the constructors.
 **/
bool Anvil::Image::init()
{
    VkResult result      = VK_ERROR_INITIALIZATION_FAILED;
    bool     result_bool = true;

    /* Sanity checks */
    {
        const auto memory_features = m_create_info_ptr->get_memory_features();

        if ((memory_features & Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) == 0)
        {
            anvil_assert((memory_features & Anvil::MemoryFeatureFlagBits::HOST_COHERENT_BIT) == 0);
        }

        if ( m_create_info_ptr->get_internal_type()                                                                                                    == Anvil::ImageInternalType::SWAPCHAIN_WRAPPER &&
            (m_create_info_ptr->get_swapchain()->get_create_info_ptr()->get_flags() & Anvil::SwapchainCreateFlagBits::SPLIT_INSTANCE_BIND_REGIONS_BIT) != 0)
        {
            anvil_assert(!m_create_info_ptr->uses_full_mipmap_chain() );
            anvil_assert(m_create_info_ptr->get_n_layers           () == 1);
            anvil_assert(m_create_info_ptr->get_type               () == Anvil::ImageType::_2D);
            anvil_assert(m_create_info_ptr->get_tiling             () == Anvil::ImageTiling::OPTIMAL);
        }

        if ( (m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT) != 0)
        {
            anvil_assert(m_create_info_ptr->get_type     ()      == Anvil::ImageType::_2D);
            anvil_assert((m_create_info_ptr->get_n_layers() % 6) == 0);
        }

        if ( (m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::_2D_ARRAY_COMPATIBLE_BIT) != 0)
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_maintenance1() );
            anvil_assert(m_create_info_ptr->get_type                         () == Anvil::ImageType::_3D);
        }

        if ( (m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPLIT_INSTANCE_BIND_REGIONS_BIT) != 0)
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_bind_memory2() );
        }

        if ( (m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::ALIAS_BIT) != 0)
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_bind_memory2() );
        }
    }

    /* If application intends to fill the image with data at bring-up time, make sure the image supports transfer_dst usage. */
    if (m_create_info_ptr->get_mipmaps_to_upload().size() > 0)
    {
        m_create_info_ptr->set_usage_flags(m_create_info_ptr->get_usage_flags() | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT);
    }

    /* Cache the number of mips we want the image to use. */
    {
        const auto max_dimension = std::max(std::max(m_create_info_ptr->get_base_mip_depth(),
                                                     m_create_info_ptr->get_base_mip_height() ),
                                            m_create_info_ptr->get_base_mip_width() );

        m_n_mipmaps = static_cast<uint32_t>((m_create_info_ptr->uses_full_mipmap_chain() ) ? (1 + log2(max_dimension))
                                                                                           : 1);
    }

    /* Create the image object */
    {
        Anvil::StructChainer<VkImageCreateInfo> struct_chainer;

        {
            VkImageCreateInfo image_create_info;
            uint32_t          n_queue_family_indices   = 0;
            uint32_t          queue_family_indices[3];

            Anvil::Utils::convert_queue_family_bits_to_family_indices(m_device_ptr,
                                                                      m_create_info_ptr->get_queue_families(),
                                                                      queue_family_indices,
                                                                     &n_queue_family_indices);

            image_create_info.arrayLayers           = m_create_info_ptr->get_n_layers       ();
            image_create_info.extent.depth          = m_create_info_ptr->get_base_mip_depth ();
            image_create_info.extent.height         = m_create_info_ptr->get_base_mip_height();
            image_create_info.extent.width          = m_create_info_ptr->get_base_mip_width ();
            image_create_info.flags                 = m_create_info_ptr->get_create_flags   ().get_vk();
            image_create_info.format                = static_cast<VkFormat>     (m_create_info_ptr->get_format                  () );
            image_create_info.imageType             = static_cast<VkImageType>  (m_create_info_ptr->get_type                    () );
            image_create_info.initialLayout         = static_cast<VkImageLayout>(m_create_info_ptr->get_post_create_image_layout() );
            image_create_info.mipLevels             = m_n_mipmaps;
            image_create_info.pNext                 = nullptr;
            image_create_info.pQueueFamilyIndices   = queue_family_indices;
            image_create_info.queueFamilyIndexCount = n_queue_family_indices;
            image_create_info.samples               = static_cast<VkSampleCountFlagBits>(m_create_info_ptr->get_sample_count() );
            image_create_info.sharingMode           = static_cast<VkSharingMode>        (m_create_info_ptr->get_sharing_mode() );
            image_create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            image_create_info.tiling                = static_cast<VkImageTiling>(m_create_info_ptr->get_tiling() );
            image_create_info.usage                 = m_create_info_ptr->get_usage_flags().get_vk();

            if (m_create_info_ptr->get_external_memory_handle_types() != 0)
            {
                anvil_assert(static_cast<Anvil::ImageLayout>(image_create_info.initialLayout) == Anvil::ImageLayout::UNDEFINED);
            }

            struct_chainer.append_struct(image_create_info);
        }

        if (m_create_info_ptr->get_external_memory_handle_types() != 0)
        {
            VkExternalMemoryImageCreateInfoKHR external_memory_image_create_info;
            const auto                         handle_types                      = m_create_info_ptr->get_external_memory_handle_types();

            external_memory_image_create_info.handleTypes = handle_types.get_vk();
            external_memory_image_create_info.pNext       = nullptr;
            external_memory_image_create_info.sType       = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR;

            struct_chainer.append_struct(external_memory_image_create_info);
        }

        {
            const Anvil::Format* compatible_image_view_formats   = nullptr;
            uint32_t             n_compatible_image_view_formats = 0;

            m_create_info_ptr->get_image_view_formats(&n_compatible_image_view_formats,
                                                      &compatible_image_view_formats);

            if (n_compatible_image_view_formats > 0)
            {
                VkImageFormatListCreateInfoKHR image_format_list_create_info;

                anvil_assert((m_create_info_ptr->get_create_flags                     () & Anvil::ImageCreateFlagBits::MUTABLE_FORMAT_BIT) != 0);
                anvil_assert(m_device_ptr->get_extension_info()->khr_image_format_list() );

                image_format_list_create_info.pNext           = nullptr;
                image_format_list_create_info.pViewFormats    = reinterpret_cast<const VkFormat*>(compatible_image_view_formats);
                image_format_list_create_info.sType           = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
                image_format_list_create_info.viewFormatCount = n_compatible_image_view_formats;

                struct_chainer.append_struct(image_format_list_create_info);
            }
        }

        if (m_create_info_ptr->get_stencil_image_aspect_usage() != Anvil::ImageUsageFlagBits::NONE)
        {
            VkImageStencilUsageCreateInfoEXT create_info;

            anvil_assert(m_device_ptr->get_extension_info()->ext_separate_stencil_usage() );

            create_info.pNext        = nullptr;
            create_info.stencilUsage = m_create_info_ptr->get_stencil_image_aspect_usage().get_vk();
            create_info.sType        = VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO_EXT;

            struct_chainer.append_struct(create_info);
        }

        {
            auto struct_chain_ptr = struct_chainer.create_chain();

            result = Anvil::Vulkan::vkCreateImage(m_device_ptr->get_device_vk      (),
                                                  struct_chain_ptr->get_root_struct(),
                                                  nullptr, /* pAllocator */
                                                 &m_image);
        }

        if (!is_vk_call_successful(result) )
        {
            anvil_assert_vk_call_succeeded(result);

            result_bool = false;
            goto end;
        }
    }

    /* Cache the handle .. */
    set_vk_handle(m_image);

    /* Prepare metadata for the generated image. */
    {
        std::vector<Anvil::ImageAspectFlags> image_aspects;

        const bool     is_disjoint_yuv_image      = (m_create_info_ptr->get_create_flags () & Anvil::ImageCreateFlagBits::CREATE_DISJOINT_BIT)             != 0;
        const bool     is_linear_image            = (m_create_info_ptr->get_tiling       ()                                                                == Anvil::ImageTiling::LINEAR);
        const bool     is_prt_image               = (m_create_info_ptr->get_create_flags () & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT)            != 0;
        const bool     is_sparse_binding_image    = (m_create_info_ptr->get_create_flags () & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT)              != 0;
        const bool     is_swapchain_wrapper_image = (m_create_info_ptr->get_internal_type()                                                                == Anvil::ImageInternalType::SWAPCHAIN_WRAPPER);
        const bool     uses_sfr                   = (m_create_info_ptr->get_create_flags () & Anvil::ImageCreateFlagBits::SPLIT_INSTANCE_BIND_REGIONS_BIT) != 0;
        const bool     is_yuv_format              = Anvil::Formats::is_format_yuv_khr  (m_create_info_ptr->get_format() );
        const uint32_t n_format_planes            = Anvil::Formats::get_format_n_planes(m_create_info_ptr->get_format() );

        ANVIL_REDUNDANT_VARIABLE_CONST(is_yuv_format);

        Anvil::Formats::get_format_aspects(m_create_info_ptr->get_format(),
                                          &image_aspects);

        if (!is_swapchain_wrapper_image)
        {
            /* Extract various image properties we're going to need later.
             *
             * Prefer facilities exposed by VK_KHR_get_memory_requirements2 unless unsupported. */
            if (m_device_ptr->get_extension_info()->khr_get_memory_requirements2() )
            {
                const auto                                                     gmr2_entrypoints                   = m_device_ptr->get_extension_khr_get_memory_requirements2_entrypoints();
                Anvil::StructChainer<VkImageMemoryRequirementsInfo2KHR>        input_struct_chainer;
                Anvil::StructChainUniquePtr<VkImageMemoryRequirementsInfo2KHR> input_struct_chain_ptr;
                const bool                                                     khr_dedicated_allocation_available = m_device_ptr->get_extension_info()->khr_dedicated_allocation();
                Anvil::StructID                                                memory_dedicated_reqs_struct_id    = UINT32_MAX;
                Anvil::StructChainer<VkMemoryRequirements2KHR>                 result_struct_chainer;
                Anvil::StructChainUniquePtr<VkMemoryRequirements2KHR>          result_struct_chain_ptr;

                /* NOTE: For disjoint images, we need to cache mem reqs for individual planes. This requires use of VkImagePlaneMemoryRequirementsInfoKHR struct. */
                const uint32_t n_planes_to_query = (is_disjoint_yuv_image) ? n_format_planes
                                                                           : 1;

                for (uint32_t n_current_plane = 0;
                              n_current_plane < n_planes_to_query;
                            ++n_current_plane)
                {
                    auto& current_plane_properties = m_plane_index_to_memory_properties_map[n_current_plane];

                    {
                        VkImageMemoryRequirementsInfo2KHR info;

                        info.image = m_image;
                        info.pNext = nullptr;
                        info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR;

                        input_struct_chainer.append_struct(info);
                    }

                    if (is_disjoint_yuv_image)
                    {
                        VkImagePlaneMemoryRequirementsInfoKHR plane_mem_reqs_info;

                        plane_mem_reqs_info.planeAspect = (n_current_plane == 0) ? VK_IMAGE_ASPECT_PLANE_0_BIT_KHR
                                                        : (n_current_plane == 1) ? VK_IMAGE_ASPECT_PLANE_1_BIT_KHR
                                                                                 : VK_IMAGE_ASPECT_PLANE_2_BIT_KHR;
                        plane_mem_reqs_info.pNext       = nullptr;
                        plane_mem_reqs_info.sType       = VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO_KHR;

                        input_struct_chainer.append_struct(plane_mem_reqs_info);

                        anvil_assert(n_current_plane <= 2);
                    }

                    {
                        VkMemoryRequirements2KHR result_reqs;

                        result_reqs.pNext = nullptr;
                        result_reqs.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;

                        result_struct_chainer.append_struct(result_reqs);
                    }

                    if (khr_dedicated_allocation_available)
                    {
                        VkMemoryDedicatedRequirementsKHR memory_dedicated_reqs;

                        memory_dedicated_reqs.pNext                       = nullptr;
                        memory_dedicated_reqs.prefersDedicatedAllocation  = VK_FALSE;
                        memory_dedicated_reqs.requiresDedicatedAllocation = VK_FALSE;
                        memory_dedicated_reqs.sType                       = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;

                        memory_dedicated_reqs_struct_id = result_struct_chainer.append_struct(memory_dedicated_reqs);
                    }

                    input_struct_chain_ptr  = input_struct_chainer.create_chain ();
                    result_struct_chain_ptr = result_struct_chainer.create_chain();

                    gmr2_entrypoints.vkGetImageMemoryRequirements2KHR(m_device_ptr->get_device_vk             (),
                                                                      input_struct_chain_ptr->get_root_struct (),
                                                                      result_struct_chain_ptr->get_root_struct() );


                    const auto& memory_reqs = result_struct_chain_ptr->get_root_struct()->memoryRequirements;

                    current_plane_properties.alignment    = memory_reqs.alignment;
                    current_plane_properties.memory_types = memory_reqs.memoryTypeBits;
                    current_plane_properties.storage_size = memory_reqs.size;

                    if (memory_dedicated_reqs_struct_id != UINT32_MAX)
                    {
                        const auto memory_dedicated_reqs_ptr = result_struct_chain_ptr->get_struct_with_id<VkMemoryDedicatedRequirementsKHR>(memory_dedicated_reqs_struct_id);

                        current_plane_properties.prefers_dedicated_allocation  = (memory_dedicated_reqs_ptr->prefersDedicatedAllocation  == VK_TRUE);
                        current_plane_properties.requires_dedicated_allocation = (memory_dedicated_reqs_ptr->requiresDedicatedAllocation == VK_TRUE);
                    }
                }
            }
            else
            {
                /* NOTE: YUV formats are NOT supported unless GIMR2 is available. */
                auto&                current_plane_properties = m_plane_index_to_memory_properties_map[0];
                VkMemoryRequirements memory_reqs;

                anvil_assert(!is_yuv_format);

                Anvil::Vulkan::vkGetImageMemoryRequirements(m_device_ptr->get_device_vk(),
                                                            m_image,
                                                           &memory_reqs);

                current_plane_properties.alignment    = memory_reqs.alignment;
                current_plane_properties.memory_types = memory_reqs.memoryTypeBits;
                current_plane_properties.storage_size = memory_reqs.size;
            }
        }

        /* Cache aspect subresource properties if we're dealing with a linear image */
        if (is_linear_image)
        {
            const auto n_layers = m_create_info_ptr->get_n_layers();

            /* NOTE: For YUV formats,     image_aspects holds PLANE_.. aspects.
             *       For non-YUV formats, the vector is filled with COLOR/DEPTH/STENCIL aspects.
             */
            for (const auto& current_aspect : image_aspects)
            {
                Anvil::ImageSubresource subresource;

                subresource.aspect_mask = current_aspect;

                for (uint32_t n_layer = 0;
                              n_layer < n_layers;
                            ++n_layer)
                {
                    subresource.array_layer = n_layer;

                    for (uint32_t n_mip = 0;
                                  n_mip < m_n_mipmaps;
                                ++n_mip)
                    {
                        Anvil::SubresourceLayout subresource_layout;

                        subresource.mip_level = n_mip;

                        Anvil::Vulkan::vkGetImageSubresourceLayout(m_device_ptr->get_device_vk(),
                                                                   m_image,
                                                                   reinterpret_cast<const VkImageSubresource*>(&subresource),
                                                                   reinterpret_cast<VkSubresourceLayout*>     (&subresource_layout) );

                        m_linear_image_aspect_data[static_cast<Anvil::ImageAspectFlagBits>(current_aspect.get_vk() )][LayerMipKey(n_layer, n_mip)] = subresource_layout;
                    }
                }
            }
        }

        init_mipmap_props();

        if (is_prt_image)
        {
            uint32_t                                          n_reqs                  = 0;
            std::vector<Anvil::SparseImageMemoryRequirements> sparse_image_memory_reqs;

            anvil_assert(m_create_info_ptr->get_internal_type() != Anvil::ImageInternalType::SWAPCHAIN_WRAPPER); /* TODO: can images, to which swapchains can be bound, be sparse? */

            /* Retrieve image aspect properties. Since Vulkan lets a single props structure to refer to more than
             * just a single aspect, we first cache the exposed info in a vec and then distribute the information to
             * a map, whose key is allowed to consist of a single bit ( = individual aspect) only
             *
             * Prefer facilities exposed by VK_KHR_get_memory_requirements2 unless unsupported. This will be leveraged
             * in the future, but for now admittedly is a bit of a moot move.
             */
            if (m_device_ptr->get_extension_info()->khr_get_memory_requirements2() )
            {
                const auto                                       gmr2_entrypoints = m_device_ptr->get_extension_khr_get_memory_requirements2_entrypoints();
                VkImageSparseMemoryRequirementsInfo2KHR          info;
                uint32_t                                         n_current_item = 0;
                std::vector<VkSparseImageMemoryRequirements2KHR> temp_vec;

                info.image = m_image;
                info.pNext = nullptr;
                info.sType = VK_STRUCTURE_TYPE_IMAGE_SPARSE_MEMORY_REQUIREMENTS_INFO_2_KHR;

                gmr2_entrypoints.vkGetImageSparseMemoryRequirements2KHR(m_device_ptr->get_device_vk(),
                                                                       &info,
                                                                       &n_reqs,
                                                                        nullptr); /* pSparseMemoryRequirements */

                anvil_assert(n_reqs >= 1);
                sparse_image_memory_reqs.resize(n_reqs);
                temp_vec.resize                (n_reqs);

                gmr2_entrypoints.vkGetImageSparseMemoryRequirements2KHR(m_device_ptr->get_device_vk(),
                                                                       &info,
                                                                       &n_reqs,
                                                                       &temp_vec[0]); /* pSparseMemoryRequirements */

                for (const auto& current_temp_vec_item : temp_vec)
                {
                    sparse_image_memory_reqs[n_current_item] = current_temp_vec_item.memoryRequirements;

                    /* Move on */
                    n_current_item++;
                }
            }
            else
            {
                Anvil::Vulkan::vkGetImageSparseMemoryRequirements(m_device_ptr->get_device_vk(),
                                                                  m_image,
                                                                 &n_reqs,
                                                                  nullptr);

                anvil_assert(n_reqs >= 1);
                sparse_image_memory_reqs.resize(n_reqs);

                Anvil::Vulkan::vkGetImageSparseMemoryRequirements(m_device_ptr->get_device_vk(),
                                                                  m_image,
                                                                 &n_reqs,
                                                                  reinterpret_cast<VkSparseImageMemoryRequirements*>(&sparse_image_memory_reqs[0]) );
            }

            for (const auto& image_memory_req : sparse_image_memory_reqs)
            {
                for (uint32_t n_bit   = 0;
                       (1u << n_bit) <= image_memory_req.format_properties.aspect_mask.get_vk();
                            ++n_bit)
                {
                    VkImageAspectFlagBits aspect = static_cast<VkImageAspectFlagBits>(1 << n_bit);

                    if ((image_memory_req.format_properties.aspect_mask.get_vk() & aspect) == 0)
                    {
                        continue;
                    }

                    anvil_assert(m_sparse_aspect_props.find(static_cast<Anvil::ImageAspectFlagBits>(aspect) ) == m_sparse_aspect_props.end() );

                    m_sparse_aspect_props[static_cast<Anvil::ImageAspectFlagBits>(aspect)] = Anvil::SparseImageAspectProperties(image_memory_req);
                }
            }

            /* Continue by setting up storage for page occupancy data */
            init_page_occupancy(sparse_image_memory_reqs);
        }
        else
        if (is_sparse_binding_image)
        {
            for (auto& current_plane_item : m_plane_index_to_memory_properties_map)
            {
                current_plane_item.second.page_tracker_ptr.reset(
                    new Anvil::PageTracker(current_plane_item.second.storage_size,
                                           current_plane_item.second.alignment)
                );
            }
        }

        /* If the image has been initialized for SFR bindings, calculate & cache SFR tile size */
        if (uses_sfr)
        {
            init_sfr_tile_size();
        }
    }

    /* If the image was created with alloc-memory-at-init-time flag, make sure to do so before we leave */
    {
        const bool should_alloc_and_bind_mem = (m_create_info_ptr->get_internal_type() == Anvil::ImageInternalType::ALLOC);

        if (should_alloc_and_bind_mem)
        {
            /* Allocate memory for the image. We need one memory block per each plane. */
            Anvil::MemoryBlockUniquePtr memory_block_ptr;

            for (auto& current_plane_item : m_plane_index_to_memory_properties_map)
            {
                auto create_info_ptr = Anvil::MemoryBlockCreateInfo::create_regular(m_device_ptr,
                                                                                    current_plane_item.second.memory_types,
                                                                                    current_plane_item.second.storage_size,
                                                                                    m_create_info_ptr->get_memory_features() );

                create_info_ptr->set_mt_safety(Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe()) );

                if (current_plane_item.second.requires_dedicated_allocation)
                {
                    create_info_ptr->use_dedicated_allocation(nullptr, /* in_opt_buffer_ptr */
                                                              this);
                }

                memory_block_ptr = Anvil::MemoryBlock::create(std::move(create_info_ptr) );
            }

            if (memory_block_ptr == nullptr)
            {
                anvil_assert(memory_block_ptr != nullptr);

                result_bool = false;
                goto end;
            }

            if (!set_memory(std::move(memory_block_ptr) ))
            {
                anvil_assert_fail();

                result_bool = false;
                goto end;
            }
        }
    }

end:
    return result_bool;
}

/** Initializes page occupancy data. Should only be used for sparse images.
 *
 *  @param memory_reqs Image memory requirements.
 **/
void Anvil::Image::init_page_occupancy(const std::vector<Anvil::SparseImageMemoryRequirements>& in_memory_reqs)
{
    anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0);

    /* First, allocate space for an AspectPageOccupancyData instance for all aspects used by the image.
     *
     * Vulkan may interleave data of more than one aspect in one memory region, so we need to go the extra
     * mile to ensure the same AspectPageOccupancyData structure is assigned to such aspects.
     */
    for (const auto& memory_req : in_memory_reqs)
    {
        AspectPageOccupancyData* occupancy_data_ptr = nullptr;

        for (uint32_t n_aspect_bit = 0;
               (1u << n_aspect_bit) <= memory_req.format_properties.aspect_mask.get_vk();
                    ++n_aspect_bit)
        {
            VkImageAspectFlagBits current_aspect = static_cast<VkImageAspectFlagBits>(1 << n_aspect_bit);

            if ((memory_req.format_properties.aspect_mask.get_vk() & current_aspect) == 0)
            {
                continue;
            }

            if (m_sparse_aspect_page_occupancy.find(static_cast<Anvil::ImageAspectFlagBits>(current_aspect) ) != m_sparse_aspect_page_occupancy.end() )
            {
                occupancy_data_ptr = m_sparse_aspect_page_occupancy.at(static_cast<Anvil::ImageAspectFlagBits>(current_aspect) );

                break;
            }
        }

        if (occupancy_data_ptr == nullptr)
        {
            m_sparse_aspect_page_occupancy_data_items_owned.push_back(
                std::unique_ptr<AspectPageOccupancyData>(new AspectPageOccupancyData() )
            );
        }

        for (uint32_t n_aspect_bit = 0;
               (1u << n_aspect_bit) <= memory_req.format_properties.aspect_mask.get_vk();
                    ++n_aspect_bit)
        {
            Anvil::ImageAspectFlagBits current_aspect = static_cast<Anvil::ImageAspectFlagBits>(1 << n_aspect_bit);

            if ((memory_req.format_properties.aspect_mask & current_aspect) == 0)
            {
                continue;
            }

            m_sparse_aspect_page_occupancy[static_cast<Anvil::ImageAspectFlagBits>(current_aspect)] = m_sparse_aspect_page_occupancy_data_items_owned.back().get();
        }
    }

    /* Next, iterate over each aspect and initialize storage for the mip chain */
    for (auto occupancy_iterator  = m_sparse_aspect_page_occupancy.begin();
              occupancy_iterator != m_sparse_aspect_page_occupancy.end();
            ++occupancy_iterator)
    {
        const Anvil::ImageAspectFlagBits&               current_aspect                = occupancy_iterator->first;
        decltype(m_sparse_aspect_props)::const_iterator current_aspect_props_iterator;
        AspectPageOccupancyData*                        page_occupancy_ptr            = occupancy_iterator->second;
        const auto&                                     plane_index                   = (current_aspect == Anvil::ImageAspectFlagBits::PLANE_1_BIT) ? 1
                                                                                      : (current_aspect == Anvil::ImageAspectFlagBits::PLANE_2_BIT) ? 2
                                                                                                                                                    : 0;
        const auto&                                     plane_memory_reqs             = m_plane_index_to_memory_properties_map.at(plane_index);

        anvil_assert(plane_memory_reqs.alignment != 0);

        if (page_occupancy_ptr->layers.size() > 0)
        {
            /* Already initialized */
            continue;
        }

        if (current_aspect == Anvil::ImageAspectFlagBits::METADATA_BIT)
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
                      n_layer < m_create_info_ptr->get_n_layers();
                    ++n_layer)
        {
            page_occupancy_ptr->layers.push_back(AspectPageOccupancyLayerData() );

            auto& current_layer = page_occupancy_ptr->layers.back();

            /* Tail can be, but does not necessarily have to be a part of non-zero layers. Take this into account,
             * when determining how many pages we need to alloc space for it. */
            if (current_aspect_props_iterator->second.mip_tail_size > 0)
            {
                const bool is_single_miptail = (current_aspect_props_iterator->second.flags & Anvil::SparseImageFormatFlagBits::SINGLE_MIPTAIL_BIT) != 0;

                if ((is_single_miptail && n_layer == 0) ||
                    !is_single_miptail)
                {
                    anvil_assert( (current_aspect_props_iterator->second.mip_tail_size % plane_memory_reqs.alignment) == 0);

                    current_layer.n_total_tail_pages = static_cast<uint32_t>(current_aspect_props_iterator->second.mip_tail_size / plane_memory_reqs.alignment);

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


            for (const auto& current_mip : m_mipmap_props)
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

/** TODO */
void Anvil::Image::init_sfr_tile_size()
{
    std::vector<Anvil::ImageAspectFlags> aspects;
    uint32_t                             n_channel_bits[4];
    uint32_t                             n_bytes_per_texel;

    anvil_assert(m_create_info_ptr->get_type() == Anvil::ImageType::_2D);

    Anvil::Formats::get_format_aspects(m_create_info_ptr->get_format(),
                                      &aspects);

    if (aspects.size() != 1)
    {
        /* TODO: It is currently undefined how SFR tile size should be calculated for images
         *       with more than 1 aspect. */
        anvil_assert(aspects.size() == 1);

        goto end;
    }

    if (Anvil::Formats::is_format_compressed(m_create_info_ptr->get_format() ) )
    {
        /* TODO */
        anvil_assert(!Anvil::Formats::is_format_compressed(m_create_info_ptr->get_format() ));

        goto end;
    }

    Anvil::Formats::get_format_n_component_bits_nonyuv(m_create_info_ptr->get_format(),
                                                       n_channel_bits + 0,
                                                       n_channel_bits + 1,
                                                       n_channel_bits + 2,
                                                       n_channel_bits + 3);

    n_bytes_per_texel = (n_channel_bits[0] + n_channel_bits[1] + n_channel_bits[2] + n_channel_bits[3]) / 8;

    switch (n_bytes_per_texel)
    {
        case 1:
        {
            m_sfr_tile_size.width  = 256;
            m_sfr_tile_size.height = 256;

            break;
        }

        case 2:
        {
            m_sfr_tile_size.width  = 256;
            m_sfr_tile_size.height = 128;

            break;
        }

        case 4:
        {
            m_sfr_tile_size.width  = 128;
            m_sfr_tile_size.height = 128;

            break;
        }

        case 8:
        {
            m_sfr_tile_size.width  = 128;
            m_sfr_tile_size.height = 64;

            break;
        }

        case 16:
        {
            m_sfr_tile_size.width  = 64;
            m_sfr_tile_size.height = 64;

            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }
end:
    ;
}

/* Please see header for specification */
const VkImage& Anvil::Image::get_image(const bool& in_bake_memory_if_necessary)
{
    if ((m_create_info_ptr->get_create_flags () & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT) == 0)
    {
        if (m_memory_blocks_owned.size() == 0 &&
            in_bake_memory_if_necessary)
        {
            get_memory_block();
        }
    }

    return m_image;
}

/* Please see header for specification */
VkExtent2D Anvil::Image::get_image_extent_2D(uint32_t in_n_mipmap) const
{
    VkExtent2D result = {0u, 0u};
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
    VkExtent3D result = {0u, 0u, 0u};
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
    if (m_mipmap_props.size() <= in_n_mipmap)
    {
        goto end;
    }

    /* Return the result data.. */
    if (out_opt_width_ptr != nullptr)
    {
        *out_opt_width_ptr = m_mipmap_props[in_n_mipmap].width;
    }

    if (out_opt_height_ptr != nullptr)
    {
        *out_opt_height_ptr = m_mipmap_props[in_n_mipmap].height;
    }

    if (out_opt_depth_ptr != nullptr)
    {
        *out_opt_depth_ptr = m_mipmap_props[in_n_mipmap].depth;
    }

    /* All done */
    result = true;

end:
    return result;
}

/** Please see header for specification */
bool Anvil::Image::get_SFR_tile_size(VkExtent2D* out_result_ptr) const
{
    const Anvil::MGPUDevice* device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr));
    bool                     result    (false);

    if (device_ptr == nullptr)
    {
        anvil_assert_fail();

        goto end;
    }

    /* All done */
    *out_result_ptr = m_sfr_tile_size;
    result          = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::Image::get_sparse_image_aspect_properties(const Anvil::ImageAspectFlagBits           in_aspect,
                                                      const Anvil::SparseImageAspectProperties** out_result_ptr_ptr) const
{
    decltype(m_sparse_aspect_props)::const_iterator prop_iterator;
    bool                                            result = false;

    if (m_create_info_ptr->get_internal_type() != Anvil::ImageInternalType::NO_ALLOC)
    {
        anvil_assert(m_create_info_ptr->get_internal_type() == Anvil::ImageInternalType::NO_ALLOC);

        goto end;
    }

    if ((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) == 0)
    {
        anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0);

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

/** Please see header for specification */
Anvil::ImageSubresourceRange Anvil::Image::get_subresource_range() const
{
    Anvil::ImageSubresourceRange result;

    switch (m_create_info_ptr->get_format() )
    {
        case Anvil::Format::D16_UNORM:
        case Anvil::Format::D32_SFLOAT:
        {
            result.aspect_mask = Anvil::ImageAspectFlagBits::DEPTH_BIT;

            break;
        }

        case Anvil::Format::D16_UNORM_S8_UINT:
        case Anvil::Format::D24_UNORM_S8_UINT:
        case Anvil::Format::D32_SFLOAT_S8_UINT:
        {
            result.aspect_mask = Anvil::ImageAspectFlagBits::DEPTH_BIT | Anvil::ImageAspectFlagBits::STENCIL_BIT;

            break;
        }

        case Anvil::Format::S8_UINT:
        {
            result.aspect_mask = Anvil::ImageAspectFlagBits::STENCIL_BIT;

            break;
        }

        default:
        {
            result.aspect_mask = Anvil::ImageAspectFlagBits::COLOR_BIT;

            break;
        }
    }

    result.base_array_layer = 0;
    result.base_mip_level   = 0;
    result.layer_count     = VK_REMAINING_ARRAY_LAYERS;
    result.level_count     = VK_REMAINING_MIP_LEVELS;

    return result;
}

/** Please see header for specification */
bool Anvil::Image::has_aspects(const Anvil::ImageAspectFlags& in_aspects) const
{
    Anvil::ImageAspectFlags checked_aspects;
    bool                    result          = true;

    if ((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0)
    {
        for (uint32_t n_bit = 0;
                      n_bit < sizeof(uint32_t) * 8 /* bits in byte */ && result;
                    ++n_bit)
        {
            Anvil::ImageAspectFlagBits current_aspect = static_cast<Anvil::ImageAspectFlagBits>(1 << n_bit);

            if ((in_aspects & current_aspect)              != 0 &&
                m_sparse_aspect_props.find(current_aspect) == m_sparse_aspect_props.end() )
            {
                result = false;
            }
        }
    }
    else
    {
        const auto format        = m_create_info_ptr->get_format    ();
        const bool is_yuv_format = Anvil::Formats::is_format_yuv_khr(format);

        if (is_yuv_format)
        {
            const auto n_planes = Anvil::Formats::get_format_n_planes(format);

            if ((in_aspects & Anvil::ImageAspectFlagBits::PLANE_0_BIT) == Anvil::ImageAspectFlagBits::PLANE_0_BIT)
            {
                /* No check needed. YUV formats always use plane 0 */
                checked_aspects |= Anvil::ImageAspectFlagBits::PLANE_0_BIT;
            }

            if ((in_aspects & Anvil::ImageAspectFlagBits::PLANE_1_BIT) == Anvil::ImageAspectFlagBits::PLANE_1_BIT)
            {
                result &= (n_planes >= 2);

                checked_aspects |= Anvil::ImageAspectFlagBits::PLANE_1_BIT;
            }

            if ((in_aspects & Anvil::ImageAspectFlagBits::PLANE_2_BIT) == Anvil::ImageAspectFlagBits::PLANE_2_BIT)
            {
                result &= (n_planes >= 3);

                checked_aspects |= Anvil::ImageAspectFlagBits::PLANE_2_BIT;
            }
        }
        else
        {
            const Anvil::ComponentLayout component_layout       = Anvil::Formats::get_format_component_layout_nonyuv(format);
            bool                         has_color_components   = false;
            bool                         has_depth_components   = false;
            bool                         has_stencil_components = false;

            switch (component_layout)
            {
                case ComponentLayout::ABGR:
                case ComponentLayout::ARGB:
                case ComponentLayout::BGR:
                case ComponentLayout::BGRA:
                case ComponentLayout::EBGR:
                case ComponentLayout::R:
                case ComponentLayout::RG:
                case ComponentLayout::RGB:
                case ComponentLayout::RGBA:
                {
                    has_color_components = true;

                    break;
                }

                case ComponentLayout::D:
                case ComponentLayout::XD:
                {
                    has_depth_components = true;

                    break;
                }

                case ComponentLayout::DS:
                {
                    has_depth_components   = true;
                    has_stencil_components = true;

                    break;
                }

                case ComponentLayout::S:
                {
                    has_stencil_components = true;

                    break;
                }

                default:
                {
                    anvil_assert_fail();
                }
            }

            if ((in_aspects & Anvil::ImageAspectFlagBits::COLOR_BIT) != 0)
            {
                result &= has_color_components;

                checked_aspects |= Anvil::ImageAspectFlagBits::COLOR_BIT;
            }

            if (result && (in_aspects & Anvil::ImageAspectFlagBits::DEPTH_BIT) != 0)
            {
                result &= has_depth_components;

                checked_aspects |= Anvil::ImageAspectFlagBits::DEPTH_BIT;
            }

            if (result && (in_aspects & Anvil::ImageAspectFlagBits::STENCIL_BIT) != 0)
            {
                result &= has_stencil_components;

                checked_aspects |= Anvil::ImageAspectFlagBits::STENCIL_BIT;
            }
        }

        anvil_assert( !result                                   ||
                     ( result && checked_aspects == in_aspects));
    }

    return result;
}

/** Fills the m_mipmaps vector with mipmap size data. */
void Anvil::Image::init_mipmap_props()
{
    uint32_t current_mipmap_size[3] =
    {
        m_create_info_ptr->get_base_mip_width (),
        m_create_info_ptr->get_base_mip_height(),
        m_create_info_ptr->get_base_mip_depth ()
    };

    anvil_assert(m_n_mipmaps           != 0);
    anvil_assert(m_mipmap_props.size() == 0);

    for (uint32_t mipmap_level = 0;
                  mipmap_level < m_n_mipmaps;
                ++mipmap_level)
    {
        m_mipmap_props.push_back(Mipmap(current_mipmap_size[0],
                                        current_mipmap_size[1],
                                        current_mipmap_size[2]) );

        current_mipmap_size[0] /= 2;
        current_mipmap_size[1] /= 2;

        if (m_create_info_ptr->get_type() == Anvil::ImageType::_3D)
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
bool Anvil::Image::is_memory_bound_for_texel(Anvil::ImageAspectFlagBits in_aspect,
                                             uint32_t                   in_n_layer,
                                             uint32_t                   in_n_mip,
                                             uint32_t                   in_x,
                                             uint32_t                   in_y,
                                             uint32_t                   in_z) const
{
    bool result = false;

    /* Sanity checks */
    anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0);

    anvil_assert(m_create_info_ptr->get_n_layers () > in_n_layer);
    anvil_assert(m_n_mipmaps                        > in_n_mip);

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
 *  @param in_resource_offset             Raw image data offset, from which the update has been performed
 *  @param in_size                        Size of the updated region.
 *  @param in_memory_block_ptr            Memory block, bound to the specified region.
 *  @param in_memory_block_start_offset   Start offset relative to @param memory_block_ptr used for the update.
 *  @param in_memory_block_owned_by_image TODO
 *  @param in_n_plane                     Index of image plane the binding is to be made for. This must be 0 for non-YUV
 *                                        and joint YUV images. For disjoint YUV images, the value must be between 0 and 2.
 **/
void Anvil::Image::on_memory_backing_opaque_update(uint32_t            in_n_plane,
                                                   VkDeviceSize        in_resource_offset,
                                                   VkDeviceSize        in_size,
                                                   Anvil::MemoryBlock* in_memory_block_ptr,
                                                   VkDeviceSize        in_memory_block_start_offset,
                                                   bool                in_memory_block_owned_by_image)
{
    const bool is_unbinding = (in_memory_block_ptr == nullptr);

    /* Sanity checks */
    anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT) != 0);

    if (in_memory_block_ptr != nullptr)
    {
        anvil_assert(in_memory_block_ptr->get_create_info_ptr()->get_size() <= in_memory_block_start_offset + in_size);
    }

    if ((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) == 0)
    {
        /* Non-resident image: underlying memory is viewed as an opaque linear region. */
        PerPlaneMemoryProperties* plane_memory_reqs_ptr = &m_plane_index_to_memory_properties_map.at(in_n_plane);

        plane_memory_reqs_ptr->page_tracker_ptr->set_binding(in_memory_block_ptr,
                                                             in_memory_block_start_offset,
                                                             in_resource_offset,
                                                             in_size);
    }
    else
    {
        /* The following use cases are expected to make us reach this block:
         *
         * 1) Application is about to bind a memory backing to all tiles (of a single plane) OR unbind memory backing
         *    from all tiles forming all aspects at once.
         *
         * 2) Application wants to bind (or unbind) a memory backing to/from miptail tile(s).
         *
         * 3) Anvil::Image has requested to bind memory to the metadata aspect. At the moment, this can only
         *    be invoked from within the wrapper's code, and the memory block used for the operation is owned
         *    by the Image wrapper, so there's nothing we need to do here.
         */
        bool is_miptail_tile_binding_operation = false;
        auto metadata_aspect_iterator          = m_sparse_aspect_props.find(Anvil::ImageAspectFlagBits::METADATA_BIT);

        if (metadata_aspect_iterator != m_sparse_aspect_props.end() )
        {
            /* Handle case 3) */
            if (in_resource_offset == metadata_aspect_iterator->second.mip_tail_offset &&
                in_size            == metadata_aspect_iterator->second.mip_tail_size)
            {
                anvil_assert(m_metadata_memory_block_ptr == nullptr);
                anvil_assert(in_memory_block_owned_by_image);

                m_metadata_memory_block_ptr = MemoryBlockUniquePtr(in_memory_block_ptr,
                                                                   std::default_delete<MemoryBlock>() );

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
            Anvil::ImageAspectFlagBits         current_aspect       = aspect_iterator->first;
            const SparseImageAspectProperties& current_aspect_props = m_sparse_aspect_props.at(current_aspect);
            auto                               occupancy_data_ptr   = aspect_iterator->second;

            ANVIL_REDUNDANT_VARIABLE_CONST(current_aspect_props);

            if (in_size == current_aspect_props.mip_tail_size)
            {
                for (uint32_t n_layer = 0;
                              n_layer < m_create_info_ptr->get_n_layers();
                            ++n_layer)
                {
                    auto& current_layer = occupancy_data_ptr->layers.at(n_layer);

                    if (in_resource_offset == current_aspect_props.mip_tail_offset + current_aspect_props.mip_tail_stride * n_layer)
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

    if (in_memory_block_owned_by_image)
    {
        m_memory_blocks_owned.push_back(
            Anvil::MemoryBlockUniquePtr(in_memory_block_ptr,
                                       std::default_delete<MemoryBlock>() )
        );
    }
}

/** Updates tile-to-block mappings & tail page counteres, as per the specified opaque image update properties.
 *
 *  @param in_subresource               Subresource specified for the update.
 *  @param in_offset                    Offset specified for the update.
 *  @param in_extent                    Extent specified for the update.
 *  @param in_memory_block_ptr          Memory block, bound to the specified region.
 *  @param in_memory_block_start_offset Start offset relative to @param memory_block_ptr used for the update.
 **/
void Anvil::Image::on_memory_backing_update(const Anvil::ImageSubresource& in_subresource,
                                            VkOffset3D                     in_offset,
                                            VkExtent3D                     in_extent,
                                            Anvil::MemoryBlock*            in_memory_block_ptr,
                                            VkDeviceSize                   in_memory_block_start_offset,
                                            bool                           in_memory_block_owned_by_image)
{
    AspectPageOccupancyLayerData*                      aspect_layer_ptr;
    AspectPageOccupancyLayerMipData*                   aspect_layer_mip_ptr;
    decltype(m_sparse_aspect_page_occupancy)::iterator aspect_page_occupancy_iterator;
    decltype(m_sparse_aspect_props)::iterator          aspect_props_iterator;

    anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0);

    ANVIL_REDUNDANT_ARGUMENT(in_memory_block_start_offset);


    if (in_subresource.aspect_mask == Anvil::ImageAspectFlagBits::METADATA_BIT)
    {
        /* Metadata is not tracked since it needs to be completely bound in order for the sparse image to be usable. */
        anvil_assert_fail();

        return;
    }

    aspect_page_occupancy_iterator = m_sparse_aspect_page_occupancy.find(static_cast<Anvil::ImageAspectFlagBits>(in_subresource.aspect_mask.get_vk() ) );
    aspect_props_iterator          = m_sparse_aspect_props.find         (static_cast<Anvil::ImageAspectFlagBits>(in_subresource.aspect_mask.get_vk() ) );

    anvil_assert(aspect_page_occupancy_iterator != m_sparse_aspect_page_occupancy.end() );
    anvil_assert(aspect_props_iterator          != m_sparse_aspect_props.end() );

    anvil_assert((in_offset.x      % aspect_props_iterator->second.granularity.width)  == 0 &&
                 (in_offset.y      % aspect_props_iterator->second.granularity.height) == 0 &&
                 (in_offset.z      % aspect_props_iterator->second.granularity.depth)  == 0);
    anvil_assert((in_extent.width  % aspect_props_iterator->second.granularity.width)  == 0 &&
                 (in_extent.height % aspect_props_iterator->second.granularity.height) == 0 &&
                 (in_extent.depth  % aspect_props_iterator->second.granularity.depth)  == 0);

    anvil_assert(aspect_page_occupancy_iterator->second->layers.size() >= in_subresource.array_layer);
    aspect_layer_ptr = &aspect_page_occupancy_iterator->second->layers.at(in_subresource.array_layer);

    anvil_assert(aspect_layer_ptr->mips.size() > in_subresource.mip_level);
    aspect_layer_mip_ptr = &aspect_layer_ptr->mips.at(in_subresource.mip_level);

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

    if (in_memory_block_owned_by_image)
    {
        m_memory_blocks_owned.push_back(
            MemoryBlockUniquePtr(in_memory_block_ptr,
                                 std::default_delete<MemoryBlock>() )
        );
    }
}

/* Please see header for specification */
bool Anvil::Image::set_memory(MemoryBlockUniquePtr in_memory_block_ptr)
{
    return set_memory_internal(in_memory_block_ptr.release(),
                               true,     /* in_owned_by_image           */
                               0,        /* in_n_device_group_indices   */
                               nullptr,  /* in_device_group_indices_ptr */
                               0,        /* in_n_SFR_rects              */
                               nullptr); /* in_SFRs_ptr                 */
}

bool Anvil::Image::set_memory(Anvil::MemoryBlock* in_memory_block_ptr)
{
    return set_memory_internal(in_memory_block_ptr,
                               false,    /* in_owned_by_image           */
                               0,        /* in_n_device_group_indices   */
                               nullptr,  /* in_device_group_indices_ptr */
                               0,        /* in_n_SFR_rects              */
                               nullptr); /* in_SFRs_ptr                 */
}

/** Please see header for specification */
bool Anvil::Image::set_memory(MemoryBlockUniquePtr  in_memory_block_ptr,
                              uint32_t              in_n_device_group_indices,
                              const uint32_t*       in_device_group_indices_ptr)
{
    return set_memory_internal(in_memory_block_ptr.release(),
                               true, /* in_owned_by_image */
                               in_n_device_group_indices,
                               in_device_group_indices_ptr,
                               0,        /* in_n_SFR_rects */
                               nullptr); /* in_SFRs_ptr    */
}

bool Anvil::Image::set_memory(Anvil::MemoryBlock*   in_memory_block_ptr,
                              uint32_t              in_n_device_group_indices,
                              const uint32_t*       in_device_group_indices_ptr)
{
    return set_memory_internal(in_memory_block_ptr,
                               false, /* in_owned_by_image */
                               in_n_device_group_indices,
                               in_device_group_indices_ptr,
                               0,        /* in_n_SFR_rects */
                               nullptr); /* in_SFRs_ptr    */
}

/** Please see header for specification */
bool Anvil::Image::set_memory(MemoryBlockUniquePtr in_memory_block_ptr,
                              uint32_t             in_n_SFR_rects,
                              const VkRect2D*      in_SFRs_ptr)
{
    return set_memory_internal(in_memory_block_ptr.release(),
                               true,    /* in_owned_by_image           */
                               0,       /* in_n_device_group_indices   */
                               nullptr, /* in_device_group_indices_ptr */
                               in_n_SFR_rects,
                               in_SFRs_ptr);
}

bool Anvil::Image::set_memory(Anvil::MemoryBlock* in_memory_block_ptr,
                              uint32_t            in_n_SFR_rects,
                              const VkRect2D*     in_SFRs_ptr)
{
    return set_memory_internal(in_memory_block_ptr,
                               false,   /* in_owned_by_image           */
                               0,       /* in_n_device_group_indices   */
                               nullptr, /* in_device_group_indices_ptr */
                               in_n_SFR_rects,
                               in_SFRs_ptr);
}

bool Anvil::Image::set_memory_internal(Anvil::MemoryBlock*  in_memory_block_ptr,
                                       bool                 in_owned_by_image,
                                       uint32_t             in_n_device_group_indices,
                                       const uint32_t*      in_device_group_indices_ptr,
                                       uint32_t             in_n_SFR_rects,
                                       const VkRect2D*      in_SFRs_ptr)
{
    const auto& entrypoints          (m_device_ptr->get_extension_khr_bind_memory2_entrypoints() );
    const bool  is_disjoint_yuv_image((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::CREATE_DISJOINT_BIT) != 0);
    VkResult    result               (VK_ERROR_INITIALIZATION_FAILED);

    const uint32_t n_planes = (is_disjoint_yuv_image ? Anvil::Formats::get_format_n_planes(m_create_info_ptr->get_format() )
                                                     : 1);

    /* Sanity checks */
    anvil_assert(in_memory_block_ptr                                                                      != nullptr);
    anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT) == 0);
    anvil_assert(m_mipmap_props.size()                                                                    >  0);
    anvil_assert(m_memory_blocks_owned.size()                                                             == 0);
    anvil_assert(m_create_info_ptr->get_internal_type()                                                   != Anvil::ImageInternalType::SWAPCHAIN_WRAPPER);


    if (in_n_device_group_indices > 0)
    {
        if (!do_sanity_checks_for_physical_device_binding(in_memory_block_ptr,
                                                          in_n_device_group_indices) )
        {
            anvil_assert_fail();

            goto end;
        }
    }

    {
        std::vector<VkBindImageMemoryInfoKHR>                               bind_info_vec;
        std::vector<Anvil::StructChainUniquePtr<VkBindImageMemoryInfoKHR> > struct_chain_ptrs;

        bind_info_vec.resize    (n_planes);
        struct_chain_ptrs.resize(n_planes);

        for (uint32_t n_current_plane = 0;
                      n_current_plane < n_planes;
                    ++n_current_plane)
        {
            Anvil::StructChainUniquePtr<VkBindImageMemoryInfoKHR> struct_chain_ptr;
            Anvil::StructChainer<VkBindImageMemoryInfoKHR>        struct_chainer;

            {
                VkBindImageMemoryInfoKHR bind_info;

                bind_info.image            = m_image;
                bind_info.memory           = in_memory_block_ptr->get_memory      ();
                bind_info.memoryOffset     = in_memory_block_ptr->get_start_offset();
                bind_info.pNext            = nullptr;
                bind_info.sType            = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO_KHR;

                struct_chainer.append_struct(bind_info);
            }

            if (in_n_device_group_indices > 0)
            {
                VkBindImageMemoryDeviceGroupInfoKHR bind_info_dg;

                bind_info_dg.deviceIndexCount             = in_n_device_group_indices;
                bind_info_dg.pDeviceIndices               = in_device_group_indices_ptr;
                bind_info_dg.pSplitInstanceBindRegions    = in_SFRs_ptr;
                bind_info_dg.splitInstanceBindRegionCount = in_n_SFR_rects;
                bind_info_dg.pNext                        = nullptr;
                bind_info_dg.sType                        = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO_KHR;

                struct_chainer.append_struct(bind_info_dg);
            }

            if (is_disjoint_yuv_image)
            {
                VkBindImagePlaneMemoryInfoKHR plane_info;

                plane_info.planeAspect = (n_current_plane == 0) ? VK_IMAGE_ASPECT_PLANE_0_BIT
                                       : (n_current_plane == 1) ? VK_IMAGE_ASPECT_PLANE_1_BIT
                                                                : VK_IMAGE_ASPECT_PLANE_2_BIT;
                plane_info.pNext       = nullptr;
                plane_info.sType       = VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO_KHR;

                struct_chainer.append_struct(plane_info);

                anvil_assert(n_current_plane >= 0 &&
                             n_current_plane <= 2);
            }

            struct_chain_ptr = struct_chainer.create_chain();

            bind_info_vec.at    (n_current_plane) = *struct_chain_ptr->get_root_struct();
            struct_chain_ptrs.at(n_current_plane) = std::move                         (struct_chain_ptr);
        }

        anvil_assert(bind_info_vec.size() > 0);

        result = entrypoints.vkBindImageMemory2KHR(m_device_ptr->get_device_vk                   (),
                                                   static_cast<uint32_t>                         (bind_info_vec.size() ),
                                                   reinterpret_cast<const VkBindImageMemoryInfo*>(&bind_info_vec.at(0) ));
    }

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        if (in_owned_by_image)
        {
            m_memory_blocks_owned.push_back(
                MemoryBlockUniquePtr(in_memory_block_ptr,
                                     std::default_delete<MemoryBlock>() )
            );
        }

        {
            const auto&        mips_to_upload   = m_create_info_ptr->get_mipmaps_to_upload();
            const auto         tiling           = m_create_info_ptr->get_tiling           ();
            Anvil::ImageLayout src_image_layout = (tiling == Anvil::ImageTiling::LINEAR && mips_to_upload.size() > 0) ? Anvil::ImageLayout::PREINITIALIZED
                                                                                                                      : Anvil::ImageLayout::UNDEFINED;

            /* Fill the storage with mipmap contents, if mipmap data was specified at input */
            if (mips_to_upload.size() > 0)
            {
                upload_mipmaps(&mips_to_upload,
                               src_image_layout,
                              &src_image_layout);
            }

            if (m_create_info_ptr->get_post_alloc_image_layout() != m_create_info_ptr->get_post_create_image_layout() )
            {
                const uint32_t     n_mipmaps_to_upload = static_cast<uint32_t>(mips_to_upload.size());
                Anvil::AccessFlags src_access_mask;

                if (n_mipmaps_to_upload > 0)
                {
                    if (tiling == Anvil::ImageTiling::LINEAR)
                    {
                        src_access_mask = Anvil::AccessFlagBits::HOST_WRITE_BIT;
                    }
                    else
                    {
                        src_access_mask = Anvil::AccessFlagBits::TRANSFER_WRITE_BIT;
                    }
                }

                transition_to_post_alloc_image_layout(src_access_mask,
                                                      src_image_layout);
            }

            m_create_info_ptr->clear_mipmaps_to_upload();
        }

    }

end:
    return is_vk_call_successful(result);
}

bool Anvil::Image::set_swapchain_memory_internal(uint32_t        in_swapchain_image_index,
                                                 uint32_t        in_opt_n_SFR_rects,
                                                 const VkRect2D* in_opt_SFRs_ptr,
                                                 uint32_t        in_opt_n_device_indices,
                                                 const uint32_t* in_opt_device_indices)
{
    const auto&                                    entrypoints      (m_device_ptr->get_extension_khr_bind_memory2_entrypoints() );
    VkResult                                       result           (VK_ERROR_INITIALIZATION_FAILED);
    Anvil::StructChainer<VkBindImageMemoryInfoKHR> struct_chainer;

    /* Sanity checks */
    anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::CREATE_DISJOINT_BIT) == 0);
    anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT)  == 0);
    anvil_assert(m_mipmap_props.size                 ()                                                    == 1);
    anvil_assert(m_peer_device_indices.size          ()                                                    == 0);
    anvil_assert(m_peer_sfr_rects.size               ()                                                    == 0);
    anvil_assert(m_create_info_ptr->get_internal_type()                                                    == Anvil::ImageInternalType::PEER_NO_ALLOC     ||
                 m_create_info_ptr->get_internal_type()                                                    == Anvil::ImageInternalType::SWAPCHAIN_WRAPPER);
    anvil_assert(m_create_info_ptr->get_swapchain()->get_n_images()                                        >  in_swapchain_image_index);

    if (!m_device_ptr->is_extension_enabled(VK_KHR_DEVICE_GROUP_EXTENSION_NAME) )
    {
        anvil_assert(m_device_ptr->is_extension_enabled(VK_KHR_DEVICE_GROUP_EXTENSION_NAME) );

        goto end;
    }

    if (m_swapchain_memory_assigned)
    {
        anvil_assert(!m_swapchain_memory_assigned);

        goto end;
    }

    if (in_opt_n_SFR_rects != 0 && in_opt_n_device_indices != 0)
    {
        anvil_assert(!(in_opt_n_SFR_rects != 0 && in_opt_n_device_indices != 0) );

        goto end;
    }

    if (in_opt_n_SFR_rects > 0)
    {
        if (!do_sanity_checks_for_sfr_binding(in_opt_n_SFR_rects,
                                              in_opt_SFRs_ptr) )
        {
            anvil_assert_fail();

            goto end;
        }
    }

    if (in_opt_n_device_indices > 0)
    {
        const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );

        anvil_assert(mgpu_device_ptr != nullptr);

        if (in_opt_n_device_indices != mgpu_device_ptr->get_n_physical_devices() )
        {
            anvil_assert_fail();

            goto end;
        }
    }

    /* Bind the memory object to the image object */
    {
        VkBindImageMemoryInfoKHR bind_info;

        bind_info.image        = m_image;
        bind_info.memory       = VK_NULL_HANDLE;
        bind_info.memoryOffset = 0;
        bind_info.pNext        = nullptr;
        bind_info.sType        = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO_KHR;

        struct_chainer.append_struct(bind_info);
    }

    {
        VkBindImageMemoryDeviceGroupInfoKHR bind_info_dg;

        bind_info_dg.deviceIndexCount             = in_opt_n_device_indices;
        bind_info_dg.pDeviceIndices               = in_opt_device_indices;
        bind_info_dg.pSplitInstanceBindRegions    = in_opt_SFRs_ptr;
        bind_info_dg.splitInstanceBindRegionCount = in_opt_n_SFR_rects;
        bind_info_dg.pNext                        = nullptr;
        bind_info_dg.sType                        = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO_KHR;

        struct_chainer.append_struct(bind_info_dg);
    }

    {
        VkBindImageMemorySwapchainInfoKHR bind_swapchain_info;

        bind_swapchain_info.imageIndex = in_swapchain_image_index;
        bind_swapchain_info.pNext      = nullptr;
        bind_swapchain_info.sType      = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHR;
        bind_swapchain_info.swapchain  = m_create_info_ptr->get_swapchain()->get_swapchain_vk();

        struct_chainer.append_struct(bind_swapchain_info);
    }

    m_create_info_ptr->get_swapchain()->lock();
    {
        auto chain_ptr = struct_chainer.create_chain();

        result = entrypoints.vkBindImageMemory2KHR(m_device_ptr->get_device_vk(),
                                                   1, /* bindInfoCount */
                                                   chain_ptr->get_root_struct() );
    }
    m_create_info_ptr->get_swapchain()->unlock();

    anvil_assert_vk_call_succeeded(result);

    if (is_vk_call_successful(result) )
    {
        m_swapchain_memory_assigned = true;

        for (uint32_t n_SFR_rect = 0;
                      n_SFR_rect < in_opt_n_SFR_rects;
                    ++n_SFR_rect)
        {
            m_peer_sfr_rects.push_back(in_opt_SFRs_ptr[n_SFR_rect]);
        }

        for (uint32_t n_device_index = 0;
                      n_device_index < in_opt_n_device_indices;
                    ++n_device_index)
        {
            m_peer_device_indices.push_back(in_opt_device_indices[n_device_index]);
        }
    }
end:
    return is_vk_call_successful(result);
}

void Anvil::Image::transition_to_post_alloc_image_layout(Anvil::AccessFlags in_source_access_mask,
                                                         Anvil::ImageLayout in_src_layout)
{
    const auto post_alloc_layout = m_create_info_ptr->get_post_alloc_image_layout();

    anvil_assert(!m_has_transitioned_to_post_alloc_layout);

    change_image_layout(m_device_ptr->get_universal_queue(0),
                        in_source_access_mask,
                        in_src_layout,
                        Anvil::Utils::get_access_mask_from_image_layout(post_alloc_layout),
                        post_alloc_layout,
                        get_subresource_range() );

    m_has_transitioned_to_post_alloc_layout = true;
}

/** Please see header for specification */
void Anvil::Image::upload_mipmaps(const std::vector<MipmapRawData>* in_mipmaps_ptr,
                                  Anvil::ImageLayout                in_current_image_layout,
                                  Anvil::ImageLayout*               out_new_image_layout_ptr)
{
    std::map<Anvil::ImageAspectFlagBits, std::vector<const Anvil::MipmapRawData*> > image_aspect_to_mipmap_raw_data_map;
    Anvil::ImageAspectFlags                                                         image_aspects_touched;
    Anvil::ImageSubresourceRange                                                    image_subresource_range;
    Anvil::Queue*                                                                   universal_queue_ptr                (m_device_ptr->get_universal_queue(0) );

    /* Make sure image has been assigned at least one memory block before we go ahead with the upload process */
    get_memory_block();

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
    image_subresource_range.aspect_mask      = image_aspects_touched;
    image_subresource_range.base_array_layer = 0;
    image_subresource_range.base_mip_level   = 0;
    image_subresource_range.layer_count      = m_create_info_ptr->get_n_layers();
    image_subresource_range.level_count      = m_n_mipmaps;

    if (m_create_info_ptr->get_tiling() == Anvil::ImageTiling::LINEAR)
    {
        /* TODO: Transition the subresource ranges, if necessary. */
        anvil_assert(in_current_image_layout != Anvil::ImageLayout::UNDEFINED);

        for (auto   aspect_to_mipmap_data_iterator  = image_aspect_to_mipmap_raw_data_map.begin();
                    aspect_to_mipmap_data_iterator != image_aspect_to_mipmap_raw_data_map.end();
                  ++aspect_to_mipmap_data_iterator)
        {
            Anvil::ImageAspectFlagBits current_aspect         = aspect_to_mipmap_data_iterator->first;
            const uint32_t             current_plane_index    = (current_aspect == Anvil::ImageAspectFlagBits::PLANE_1_BIT) ? 1
                                                              : (current_aspect == Anvil::ImageAspectFlagBits::PLANE_2_BIT) ? 2
                                                                                                                            : 0;
            const auto&                current_plane_mem_reqs = m_plane_index_to_memory_properties_map.at(current_plane_index);
            const auto&                mipmap_raw_data_items  = aspect_to_mipmap_data_iterator->second;
            const VkDeviceSize         sparse_page_size       = (current_plane_mem_reqs.page_tracker_ptr != nullptr) ? current_plane_mem_reqs.page_tracker_ptr->get_page_size()
                                                                                                                     : 0;

            for (auto mipmap_raw_data_item_iterator  = mipmap_raw_data_items.begin();
                      mipmap_raw_data_item_iterator != mipmap_raw_data_items.end();
                    ++mipmap_raw_data_item_iterator)
            {
                const unsigned char*     current_mipmap_data_ptr          = nullptr;
                uint32_t                 current_mipmap_height            = 0;
                const auto&              current_mipmap_raw_data_item_ptr = *mipmap_raw_data_item_iterator;
                uint32_t                 current_mipmap_slices            = 0;
                uint32_t                 current_row_size                 = 0;
                uint32_t                 current_slice_size               = 0;
                VkDeviceSize             dst_slice_offset                 = 0;
                const unsigned char*     src_slice_ptr                    = nullptr;
                Anvil::ImageSubresource  image_subresource;
                Anvil::SubresourceLayout image_subresource_layout;

                current_mipmap_data_ptr = (current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_uchar_ptr     != nullptr) ? current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_uchar_ptr.get()
                                        : (current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_uchar_raw_ptr != nullptr) ? current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_uchar_raw_ptr
                                                                                                                                  : &(*current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_uchar_vec_ptr)[0];

                image_subresource.array_layer = current_mipmap_raw_data_item_ptr->n_layer;
                image_subresource.aspect_mask = current_aspect;
                image_subresource.mip_level   = current_mipmap_raw_data_item_ptr->n_mipmap;

                Anvil::Vulkan::vkGetImageSubresourceLayout(m_device_ptr->get_device_vk(),
                                                           m_image,
                                                           reinterpret_cast<const VkImageSubresource*>(&image_subresource),
                                                           reinterpret_cast<VkSubresourceLayout*>     (&image_subresource_layout) );

                /* Determine row size for the mipmap.
                 *
                 * NOTE: Current implementation can only handle power-of-two textures.
                 */
                const auto base_mip_height = m_create_info_ptr->get_base_mip_height();

                anvil_assert(base_mip_height == 1 || (base_mip_height % 2) == 0);

                current_mipmap_height = base_mip_height / (1 << current_mipmap_raw_data_item_ptr->n_mipmap);

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
                    dst_slice_offset = image_subresource_layout.offset                +
                                       image_subresource_layout.depth_pitch * n_slice;
                    src_slice_ptr    = current_mipmap_data_ptr                        +
                                       current_slice_size                   * n_slice;

                    for (unsigned int n_row = 0;
                                      n_row < current_mipmap_height;
                                    ++n_row, dst_slice_offset += image_subresource_layout.row_pitch)
                    {
                        Anvil::MemoryBlock* mem_block_ptr          = nullptr;
                        VkDeviceSize        write_dst_slice_offset = dst_slice_offset;

                        if ((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT) == 0)
                        {
                            anvil_assert(m_memory_blocks_owned.size() == 1);

                            mem_block_ptr = m_memory_blocks_owned.at(0).get();
                        }
                        else
                        if ((m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT)   != 0 &&
                            (m_create_info_ptr->get_create_flags() & Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT) == 0)
                        {
                            const VkDeviceSize dst_slice_offset_page_aligned = Anvil::Utils::round_down(dst_slice_offset,
                                                                                                        sparse_page_size);
                            VkDeviceSize       memory_region_start_offset    = UINT32_MAX;

                            anvil_assert(sparse_page_size                                     != 0);
                            anvil_assert(( dst_slice_offset_page_aligned % sparse_page_size)  == 0);

                            mem_block_ptr = current_plane_mem_reqs.page_tracker_ptr->get_memory_block(dst_slice_offset_page_aligned,
                                                                                                      sparse_page_size,
                                                                                                     &memory_region_start_offset);
                            anvil_assert(mem_block_ptr != nullptr);

                            if (dst_slice_offset + image_subresource_layout.row_pitch > dst_slice_offset_page_aligned + sparse_page_size)
                            {
                                VkDeviceSize dummy;
                                auto         mem_block2_ptr = current_plane_mem_reqs.page_tracker_ptr->get_memory_block(dst_slice_offset_page_aligned + sparse_page_size,
                                                                                                                        sparse_page_size,
                                                                                                                       &dummy);

                                ANVIL_REDUNDANT_VARIABLE(mem_block2_ptr);

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

        *out_new_image_layout_ptr = in_current_image_layout;
    }
    else
    {
        anvil_assert(m_create_info_ptr->get_tiling() == Anvil::ImageTiling::OPTIMAL);

        Anvil::BufferUniquePtr               temp_buffer_ptr;
        Anvil::PrimaryCommandBufferUniquePtr temp_cmdbuf_ptr;
        VkDeviceSize                         total_raw_mips_size = 0;

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
                total_raw_mips_size = Anvil::Utils::round_up(total_raw_mips_size,
                                                             static_cast<VkDeviceSize>(4) );
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
        const auto base_mip_height = m_create_info_ptr->get_base_mip_height();

        anvil_assert(base_mip_height < 2 || (base_mip_height % 2) == 0);

        for (auto mipmap_iterator  = in_mipmaps_ptr->cbegin();
                  mipmap_iterator != in_mipmaps_ptr->cend();
                ++mipmap_iterator)
        {
            const auto&          current_mipmap           = *mipmap_iterator;
            const unsigned char* current_mipmap_data_ptr;
            const auto           current_mipmap_data_size = current_mipmap.n_slices * current_mipmap.data_size;

            current_mipmap_data_ptr = (mipmap_iterator->linear_tightly_packed_data_uchar_ptr     != nullptr) ? mipmap_iterator->linear_tightly_packed_data_uchar_ptr.get()
                                    : (mipmap_iterator->linear_tightly_packed_data_uchar_raw_ptr != nullptr) ? mipmap_iterator->linear_tightly_packed_data_uchar_raw_ptr
                                                                                                             : &(*mipmap_iterator->linear_tightly_packed_data_uchar_vec_ptr)[0];

            mip_data_offsets.push_back(current_mip_offset);

            anvil_assert(current_mip_offset + current_mipmap_data_size <= total_raw_mips_size);

            memcpy(merged_mip_storage.get() + current_mip_offset,
                   current_mipmap_data_ptr,
                   current_mipmap_data_size);

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

        {
            auto create_info_ptr = Anvil::BufferCreateInfo::create_alloc(m_device_ptr,
                                                                         total_raw_mips_size,
                                                                         Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                         Anvil::SharingMode::EXCLUSIVE,
                                                                         Anvil::BufferCreateFlagBits::NONE,
                                                                         Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT,
                                                                         Anvil::MemoryFeatureFlagBits::NONE);

            create_info_ptr->set_client_data(merged_mip_storage.get() );
            create_info_ptr->set_mt_safety  (Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ));

            temp_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
        }

        merged_mip_storage.reset();

        /* Set up a command buffer we will use to copy the data to the image */
        temp_cmdbuf_ptr = m_device_ptr->get_command_pool_for_queue_family_index(universal_queue_ptr->get_queue_family_index() )->alloc_primary_level_command_buffer();
        anvil_assert(temp_cmdbuf_ptr != nullptr);

        temp_cmdbuf_ptr->start_recording(true, /* one_time_submit          */
                                         false /* simultaneous_use_allowed */);
        {
            std::vector<Anvil::BufferImageCopy> copy_regions;

            /* Transfer the image to the transfer_destination layout if not already in this or general layout */
            if (in_current_image_layout != Anvil::ImageLayout::GENERAL              &&
                in_current_image_layout != Anvil::ImageLayout::TRANSFER_DST_OPTIMAL)
            {
                const auto          sharing_mode   (m_create_info_ptr->get_sharing_mode() );
                const auto          queue_fam_index((sharing_mode == Anvil::SharingMode::EXCLUSIVE) ? universal_queue_ptr->get_queue_family_index() : VK_QUEUE_FAMILY_IGNORED);

                Anvil::ImageBarrier image_barrier  (Anvil::AccessFlagBits::NONE, /* source_access_mask */
                                                    Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,
                                                    in_current_image_layout,
                                                    Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,
                                                    queue_fam_index,
                                                    queue_fam_index,
                                                    this,
                                                    image_subresource_range);

                temp_cmdbuf_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::TOP_OF_PIPE_BIT,
                                                         Anvil::PipelineStageFlagBits::TRANSFER_BIT,
                                                         Anvil::DependencyFlagBits::NONE,
                                                         0,              /* in_memory_barrier_count        */
                                                         nullptr,        /* in_memory_barrier_ptrs         */
                                                         0,              /* in_buffer_memory_barrier_count */
                                                         nullptr,        /* in_buffer_memory_barrier_ptrs  */
                                                         1,              /* in_image_memory_barrier_count  */
                                                        &image_barrier);

                *out_new_image_layout_ptr = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;
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
                const auto             base_mip_width      = m_create_info_ptr->get_base_mip_width ();
                Anvil::BufferImageCopy current_copy_region;
                const auto&            current_mipmap      = *mipmap_iterator;

                current_copy_region.buffer_image_height                = std::max(base_mip_height / (1 << current_mipmap.n_mipmap), 1u);
                current_copy_region.buffer_offset                      = mip_data_offsets[static_cast<uint32_t>(mipmap_iterator - in_mipmaps_ptr->cbegin()) ];
                current_copy_region.buffer_row_length                  = 0;
                current_copy_region.image_offset.x                     = 0;
                current_copy_region.image_offset.y                     = 0;
                current_copy_region.image_offset.z                     = 0;
                current_copy_region.image_subresource.base_array_layer = current_mipmap.n_layer;
                current_copy_region.image_subresource.layer_count      = current_mipmap.n_layers;
                current_copy_region.image_subresource.aspect_mask      = current_mipmap.aspect;
                current_copy_region.image_subresource.mip_level        = current_mipmap.n_mipmap;
                current_copy_region.image_extent.depth                 = current_mipmap.n_slices;
                current_copy_region.image_extent.height                = std::max(base_mip_height / (1 << current_mipmap.n_mipmap), 1u);
                current_copy_region.image_extent.width                 = std::max(base_mip_width  / (1 << current_mipmap.n_mipmap), 1u);

                if (current_copy_region.image_extent.depth < 1)
                {
                    current_copy_region.image_extent.depth = 1;
                }

                if (current_copy_region.image_extent.height < 1)
                {
                    current_copy_region.image_extent.height = 1;
                }

                if (current_copy_region.image_extent.width < 1)
                {
                    current_copy_region.image_extent.width = 1;
                }

                copy_regions.push_back(current_copy_region);
            }

            /* Issue the copy ops. */
            const uint32_t        n_copy_regions                   = static_cast<uint32_t>(copy_regions.size() );
            static const uint32_t n_max_copy_regions_per_copy_call = 1024;

            for (uint32_t n_copy_region = 0;
                          n_copy_region < n_copy_regions;
                          n_copy_region += n_max_copy_regions_per_copy_call)
            {
                const uint32_t n_copy_regions_to_use = std::min(n_max_copy_regions_per_copy_call,
                                                                n_copy_regions - n_copy_region);

                temp_cmdbuf_ptr->record_copy_buffer_to_image(temp_buffer_ptr.get(),
                                                             this,
                                                             *out_new_image_layout_ptr,
                                                             n_copy_regions_to_use,
                                                            &copy_regions.at(n_copy_region) );
            }
        }
        temp_cmdbuf_ptr->stop_recording();

        /* Execute the command buffer */
        {
            Anvil::CommandBufferBase* cmd_buffer_raw_ptr = temp_cmdbuf_ptr.get();

            universal_queue_ptr->submit(
                Anvil::SubmitInfo::create_execute(&cmd_buffer_raw_ptr,
                                                  1,    /* in_n_cmd_buffers */
                                                  true) /* should_block     */
            );
        }
    }
}
