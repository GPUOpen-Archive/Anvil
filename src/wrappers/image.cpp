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
                                             VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT),
     MTSafetySupportProvider                (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                             in_create_info_ptr->get_device   () )),
     m_alignment                            (UINT64_MAX),
     m_has_transitioned_to_post_alloc_layout(false),
     m_image                                (VK_NULL_HANDLE),
     m_memory_types                         (0),
     m_n_mipmaps                            (0),
     m_storage_size                         (0),
     m_swapchain_memory_assigned            (false)
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    /* Register this instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_IMAGE,
                                                 this);

}

/** Please see header for specification */
void Anvil::Image::change_image_layout(Anvil::Queue*                  in_queue_ptr,
                                       VkAccessFlags                  in_src_access_mask,
                                       VkImageLayout                  in_src_layout,
                                       VkAccessFlags                  in_dst_access_mask,
                                       VkImageLayout                  in_dst_layout,
                                       const VkImageSubresourceRange& in_subresource_range,
                                       const uint32_t                 in_opt_n_wait_semaphores,
                                       const VkPipelineStageFlags*    in_opt_wait_dst_stage_mask_ptrs,
                                       Anvil::Semaphore* const*       in_opt_wait_semaphore_ptrs,
                                       const uint32_t                 in_opt_n_set_semaphores,
                                       Anvil::Semaphore* const*       in_opt_set_semaphore_ptrs)
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
        const auto          queue_fam_index((sharing_mode == VK_SHARING_MODE_CONCURRENT) ? VK_QUEUE_FAMILY_IGNORED : in_queue_family_index);

        Anvil::ImageBarrier image_barrier  (in_src_access_mask,
                                            in_dst_access_mask,
                                            true, /* in_by_region_barrier */
                                            in_src_layout,
                                            in_dst_layout,
                                            queue_fam_index,
                                            queue_fam_index,
                                            this,
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

    if (device_type == Anvil::DEVICE_TYPE_SINGLE_GPU)
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
}

/** Please see header for specification */
Anvil::ImageUniquePtr Anvil::Image::create(Anvil::ImageCreateInfoUniquePtr in_create_info_ptr)
{
    ImageUniquePtr result_ptr(new Image(std::move(in_create_info_ptr) ),
                              std::default_delete<Image>() );

    if (result_ptr != nullptr)
    {
        const auto image_type = result_ptr->m_create_info_ptr->get_type();

        switch (image_type)
        {
            case Anvil::ImageType::NONSPARSE_ALLOC:
            case Anvil::ImageType::NONSPARSE_NO_ALLOC:
            case Anvil::ImageType::SPARSE_NO_ALLOC:
            {
                if (!result_ptr->init() )
                {
                    result_ptr.reset();
                }

                break;
            }

            case Anvil::ImageType::SWAPCHAIN_WRAPPER:
            {
                result_ptr->m_image        = result_ptr->m_create_info_ptr->get_swapchain_image();
                result_ptr->m_memory_types = 0;
                result_ptr->m_n_mipmaps    = 1;
                result_ptr->m_storage_size = 0;

                anvil_assert(result_ptr->m_image != VK_NULL_HANDLE);

                result_ptr->init_mipmap_props();

                break;
            }

            default:
            {
                anvil_assert_fail();

                result_ptr.reset();
            }
        }
    }

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

    anvil_assert(m_create_info_ptr->get_tiling() == VK_IMAGE_TILING_LINEAR);

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
Anvil::MemoryBlock* Anvil::Image::get_memory_block()
{
    bool       is_callback_needed = false;
    const auto is_sparse          = (m_create_info_ptr->get_type() == Anvil::ImageType::SPARSE_NO_ALLOC);

    if (is_sparse)
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

    if (is_sparse)
    {
        if (m_create_info_ptr->get_residency_scope() == Anvil::SPARSE_RESIDENCY_SCOPE_NONE)
        {
            anvil_assert(m_page_tracker_ptr != nullptr);

            return m_page_tracker_ptr->get_memory_block(0);
        }
        else
        {
            /* More than just one memory block may exist. You need to use page tracker manually. */
            return nullptr;
        }
    }
    else
    {
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
    std::vector<VkImageAspectFlags>         aspects_used;
    VkImageCreateFlags                      image_flags             = 0;
    Anvil::ImageFormatProperties            image_format_props;
    const auto                              memory_features         = m_create_info_ptr->get_memory_features();
    uint32_t                                n_queue_family_indices  = 0;
    uint32_t                                queue_family_indices[3];
    VkResult                                result                  = VK_ERROR_INITIALIZATION_FAILED;
    bool                                    result_bool             = true;
    Anvil::StructChainer<VkImageCreateInfo> struct_chainer;

    if ((memory_features & MEMORY_FEATURE_FLAG_MAPPABLE) == 0)
    {
        anvil_assert((memory_features & MEMORY_FEATURE_FLAG_HOST_COHERENT) == 0);
    }

    if (m_create_info_ptr->get_mipmaps_to_upload().size() > 0)
    {
        m_create_info_ptr->set_usage_flags(m_create_info_ptr->get_usage_flags() | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    }

    if (m_create_info_ptr->get_type()                                                                                                    == Anvil::ImageType::SWAPCHAIN_WRAPPER &&
        m_create_info_ptr->get_swapchain()->get_create_info_ptr()->get_flags() & VK_SWAPCHAIN_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR)
    {
        anvil_assert(!m_create_info_ptr->uses_full_mipmap_chain() );
        anvil_assert(m_create_info_ptr->get_n_layers           () == 1);
        anvil_assert(m_create_info_ptr->get_type_vk            () == VK_IMAGE_TYPE_2D);
        anvil_assert(m_create_info_ptr->get_tiling             () == VK_IMAGE_TILING_OPTIMAL);
    }

    /* Form the queue family array */
    Anvil::Utils::convert_queue_family_bits_to_family_indices(m_device_ptr,
                                                              m_create_info_ptr->get_queue_families(),
                                                              queue_family_indices,
                                                             &n_queue_family_indices);

    /* Is the requested texture size valid? */
    if (!m_device_ptr->get_physical_device_image_format_properties(Anvil::ImageFormatPropertiesQuery(m_create_info_ptr->get_format      (),
                                                                                                     m_create_info_ptr->get_type_vk     (),
                                                                                                     m_create_info_ptr->get_tiling      (),
                                                                                                     m_create_info_ptr->get_usage_flags (),
                                                                                                     m_create_info_ptr->get_create_flags() ),
                                                                  &image_format_props) )
    {
        anvil_assert_fail();

        goto end;
    }

    anvil_assert(m_create_info_ptr->get_base_mip_width() <= image_format_props.max_extent.width);

    if (m_create_info_ptr->get_base_mip_height() > 1)
    {
        anvil_assert(m_create_info_ptr->get_base_mip_height() <= image_format_props.max_extent.height);
    }

    if (m_create_info_ptr->get_base_mip_depth() > 1)
    {
        anvil_assert(m_create_info_ptr->get_base_mip_depth() <= image_format_props.max_extent.depth);
    }

    anvil_assert(m_create_info_ptr->get_n_layers() >= 1);

    if (m_create_info_ptr->get_n_layers() > 1)
    {
        anvil_assert(m_create_info_ptr->get_n_layers() <= image_format_props.n_max_array_layers);
    }

    /* If multisample image is requested, make sure the number of samples is supported. */
    anvil_assert(m_create_info_ptr->get_sample_count() >= 1);

    if (m_create_info_ptr->get_sample_count() > 1)
    {
        anvil_assert((image_format_props.sample_counts & m_create_info_ptr->get_sample_count() ) != 0);
    }

    /* Create the image object */
    if ( (m_create_info_ptr->get_create_flags() & Anvil::IMAGE_CREATE_FLAG_CUBE_COMPATIBLE_BIT) != 0)
    {
        anvil_assert(m_create_info_ptr->get_type_vk()        == VK_IMAGE_TYPE_2D);
        anvil_assert((m_create_info_ptr->get_n_layers() % 6) == 0);

        image_flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    if ( (m_create_info_ptr->get_create_flags() & Anvil::IMAGE_CREATE_FLAG_MUTABLE_FORMAT_BIT) != 0)
    {
        image_flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }

    if ( (m_create_info_ptr->get_create_flags() & Anvil::IMAGE_CREATE_FLAG_2D_ARRAY_COMPATIBLE_BIT) != 0)
    {
        anvil_assert(m_device_ptr->get_extension_info()->khr_maintenance1() );
        anvil_assert(m_create_info_ptr->get_type_vk() == VK_IMAGE_TYPE_3D);

        image_flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
    }

    if (m_create_info_ptr->get_type() == Anvil::ImageType::SWAPCHAIN_WRAPPER)
    {
        if (m_create_info_ptr->get_swapchain()->get_create_info_ptr()->get_flags() & VK_SWAPCHAIN_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR)
        {
            image_flags |= VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR;
        }
    }

    if (m_create_info_ptr->get_type() == Anvil::ImageType::SPARSE_NO_ALLOC)
    {
        /* Convert residency scope to Vulkan image create flags */
        switch (m_create_info_ptr->get_residency_scope() )
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

    {
        const auto max_dimension = std::max(std::max(m_create_info_ptr->get_base_mip_depth(),
                                                     m_create_info_ptr->get_base_mip_height() ),
                                            m_create_info_ptr->get_base_mip_width() );

        m_n_mipmaps = static_cast<uint32_t>((m_create_info_ptr->uses_full_mipmap_chain() ) ? (1 + log2(max_dimension))
                                                                                           : 1);
    }

    {
        VkImageCreateInfo image_create_info;

        image_create_info.arrayLayers           = m_create_info_ptr->get_n_layers       ();
        image_create_info.extent.depth          = m_create_info_ptr->get_base_mip_depth ();
        image_create_info.extent.height         = m_create_info_ptr->get_base_mip_height();
        image_create_info.extent.width          = m_create_info_ptr->get_base_mip_width ();
        image_create_info.flags                 = image_flags;
        image_create_info.format                = m_create_info_ptr->get_format                  ();
        image_create_info.imageType             = m_create_info_ptr->get_type_vk                 ();
        image_create_info.initialLayout         = m_create_info_ptr->get_post_create_image_layout();
        image_create_info.mipLevels             = m_n_mipmaps;
        image_create_info.pNext                 = nullptr;
        image_create_info.pQueueFamilyIndices   = queue_family_indices;
        image_create_info.queueFamilyIndexCount = n_queue_family_indices;
        image_create_info.samples               = static_cast<VkSampleCountFlagBits>(m_create_info_ptr->get_sample_count() );
        image_create_info.sharingMode           = m_create_info_ptr->get_sharing_mode();
        image_create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.tiling                = m_create_info_ptr->get_tiling     ();
        image_create_info.usage                 = m_create_info_ptr->get_usage_flags();

        if (m_create_info_ptr->get_external_memory_handle_types() != 0)
        {
            anvil_assert(image_create_info.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED);
        }

        struct_chainer.append_struct(image_create_info);
    }

    if (m_create_info_ptr->get_external_memory_handle_types() != 0)
    {
        VkExternalMemoryImageCreateInfoKHR external_memory_image_create_info;
        const auto                         handle_types                      = m_create_info_ptr->get_external_memory_handle_types();

        external_memory_image_create_info.handleTypes = Anvil::Utils::convert_external_memory_handle_type_bits_to_vk_external_memory_handle_type_flags(handle_types);
        external_memory_image_create_info.pNext       = nullptr;
        external_memory_image_create_info.sType       = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR;

        struct_chainer.append_struct(external_memory_image_create_info);
    }

    {
        auto struct_chain_ptr = struct_chainer.create_chain();

        result = vkCreateImage(m_device_ptr->get_device_vk(),
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

    set_vk_handle(m_image);

    if (m_create_info_ptr->get_type() != Anvil::ImageType::SWAPCHAIN_WRAPPER)
    {
        /* Extract various image properties we're going to need later */
        vkGetImageMemoryRequirements(m_device_ptr->get_device_vk(),
                                     m_image,
                                    &m_memory_reqs);

        m_alignment    = m_memory_reqs.alignment;
        m_memory_types = m_memory_reqs.memoryTypeBits;
        m_storage_size = m_memory_reqs.size;
    }

    /* Cache aspect subresource properties if we're dealing with a linear image */
    if (m_create_info_ptr->get_tiling() == VK_IMAGE_TILING_LINEAR)
    {
        const auto n_layers = m_create_info_ptr->get_n_layers();

        Anvil::Formats::get_format_aspects(m_create_info_ptr->get_format(),
                                          &aspects_used);

        for (const auto& current_aspect : aspects_used)
        {
            VkImageSubresource subresource;

            subresource.aspectMask = current_aspect;

            for (uint32_t n_layer = 0;
                          n_layer < n_layers;
                        ++n_layer)
            {
                subresource.arrayLayer = n_layer;

                for (uint32_t n_mip = 0;
                              n_mip < m_n_mipmaps;
                            ++n_mip)
                {
                    VkSubresourceLayout subresource_layout;

                    subresource.mipLevel = n_mip;

                    vkGetImageSubresourceLayout(m_device_ptr->get_device_vk(),
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

    if (m_create_info_ptr->get_residency_scope() == SPARSE_RESIDENCY_SCOPE_ALIASED    ||
        m_create_info_ptr->get_residency_scope() == SPARSE_RESIDENCY_SCOPE_NONALIASED)
    {
        uint32_t                                     n_reqs                  = 0;
        std::vector<VkSparseImageMemoryRequirements> sparse_image_memory_reqs;

        anvil_assert(m_create_info_ptr->get_type() != Anvil::ImageType::SWAPCHAIN_WRAPPER); /* TODO: can images, to which swapchains can be bound, be sparse? */

        /* Retrieve image aspect properties. Since Vulkan lets a single props structure to refer to more than
         * just a single aspect, we first cache the exposed info in a vec and then distribute the information to
         * a map, whose key is allowed to consist of a single bit ( = individual aspect) only */
        vkGetImageSparseMemoryRequirements(m_device_ptr->get_device_vk(),
                                           m_image,
                                          &n_reqs,
                                           nullptr);

        anvil_assert(n_reqs >= 1);

        sparse_image_memory_reqs.resize(n_reqs);

        vkGetImageSparseMemoryRequirements(m_device_ptr->get_device_vk(),
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
    }
    else
    if (m_create_info_ptr->get_residency_scope() == Anvil::SPARSE_RESIDENCY_SCOPE_NONE)
    {
        m_page_tracker_ptr.reset(
            new Anvil::PageTracker(m_storage_size,
                                   m_alignment)
        );
    }

    if (m_create_info_ptr->get_type() == Anvil::ImageType::NONSPARSE_ALLOC)
    {
        /* Allocate memory for the image */
        Anvil::MemoryBlockUniquePtr memory_block_ptr;

        {
            auto create_info_ptr = Anvil::MemoryBlockCreateInfo::create_regular(m_device_ptr,
                                                                                m_memory_reqs.memoryTypeBits,
                                                                                m_memory_reqs.size,
                                                                                m_create_info_ptr->get_memory_features() );

            create_info_ptr->set_mt_safety(Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe()) );

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

end:
    return result_bool;
}

/** Initializes page occupancy data. Should only be used for sparse images.
 *
 *  @param memory_reqs Image memory requirements.
 **/
void Anvil::Image::init_page_occupancy(const std::vector<VkSparseImageMemoryRequirements>& in_memory_reqs)
{
    anvil_assert(m_create_info_ptr->get_residency_scope() != Anvil::SPARSE_RESIDENCY_SCOPE_NONE       &&
                 m_create_info_ptr->get_residency_scope() != Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED);

    /* First, allocate space for an AspectPageOccupancyData instance for all aspects used by the image.
     *
     * Vulkan may interleave data of more than one aspect in one memory region, so we need to go the extra
     * mile to ensure the same AspectPageOccupancyData structure is assigned to such aspects.
     */
    for (const auto& memory_req : in_memory_reqs)
    {
        AspectPageOccupancyData* occupancy_data_ptr = nullptr;

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
            m_sparse_aspect_page_occupancy_data_items_owned.push_back(
                std::unique_ptr<AspectPageOccupancyData>(new AspectPageOccupancyData() )
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

            m_sparse_aspect_page_occupancy[current_aspect] = m_sparse_aspect_page_occupancy_data_items_owned.back().get();
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
        AspectPageOccupancyData*                        page_occupancy_ptr            = occupancy_iterator->second;

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
                      n_layer < m_create_info_ptr->get_n_layers();
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
    if (m_image                       != VK_NULL_HANDLE                      &&
        m_create_info_ptr->get_type() != Anvil::ImageType::SWAPCHAIN_WRAPPER)
    {
        lock();
        {
            vkDestroyImage(m_device_ptr->get_device_vk(),
                           m_image,
                           nullptr /* pAllocator */);
        }
        unlock();

        m_image = VK_NULL_HANDLE;
    }

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_IMAGE,
                                                   this);
}

/* Please see header for specification */
const VkImage& Anvil::Image::get_image()
{
    if (m_create_info_ptr->get_type() != Anvil::ImageType::SPARSE_NO_ALLOC)
    {
        if (m_memory_blocks_owned.size() == 0)
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

    if (m_create_info_ptr->get_type() != Anvil::ImageType::SPARSE_NO_ALLOC)
    {
        anvil_assert(m_create_info_ptr->get_type() == Anvil::ImageType::SPARSE_NO_ALLOC);

        goto end;
    }

    if (m_create_info_ptr->get_residency_scope() == Anvil::SPARSE_RESIDENCY_SCOPE_NONE)
    {
        anvil_assert(m_create_info_ptr->get_residency_scope() != Anvil::SPARSE_RESIDENCY_SCOPE_NONE);

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
VkImageCreateInfo Anvil::Image::get_create_info_for_swapchain(const Anvil::Swapchain* in_swapchain_ptr)
{
    VkImageCreateInfo result;

    in_swapchain_ptr->get_image(0)->get_image_mipmap_size(0, /* n_mipmap */
                                                         &result.extent.width,
                                                         &result.extent.height,
                                                         &result.arrayLayers);

    result.extent.depth          = 1;
    result.flags                 = in_swapchain_ptr->get_create_info_ptr()->get_flags ();
    result.format                = in_swapchain_ptr->get_create_info_ptr()->get_format();
    result.imageType             = VK_IMAGE_TYPE_2D;
    result.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    result.mipLevels             = 1;
    result.pNext                 = nullptr;
    result.pQueueFamilyIndices   = nullptr;
    result.queueFamilyIndexCount = UINT32_MAX;
    result.samples               = VK_SAMPLE_COUNT_1_BIT;
    result.sharingMode           = in_swapchain_ptr->get_image(0)->get_create_info_ptr()->get_sharing_mode();
    result.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    result.tiling                = VK_IMAGE_TILING_OPTIMAL;
    result.usage                 = in_swapchain_ptr->get_image(0)->get_create_info_ptr()->get_usage_flags();

    return result;
}

/** Please see header for specification */
VkImageSubresourceRange Anvil::Image::get_subresource_range() const
{
    VkImageSubresourceRange result;

    switch (m_create_info_ptr->get_format() )
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
    result.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    result.levelCount     = VK_REMAINING_MIP_LEVELS;

    return result;
}

/** Please see header for specification */
bool Anvil::Image::has_aspects(VkImageAspectFlags in_aspects) const
{
    VkImageAspectFlags checked_aspects = 0;
    bool               result          = true;

    if (m_create_info_ptr->get_residency_scope() == SPARSE_RESIDENCY_SCOPE_ALIASED    ||
        m_create_info_ptr->get_residency_scope() == SPARSE_RESIDENCY_SCOPE_NONALIASED)
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
        const Anvil::ComponentLayout component_layout       = Anvil::Formats::get_format_component_layout(m_create_info_ptr->get_format() );
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
        m_create_info_ptr->get_base_mip_width (),
        m_create_info_ptr->get_base_mip_height(),
        m_create_info_ptr->get_base_mip_depth ()
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

        if (m_create_info_ptr->get_type_vk() == VK_IMAGE_TYPE_3D)
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
    anvil_assert(m_create_info_ptr->get_residency_scope() == SPARSE_RESIDENCY_SCOPE_ALIASED     ||
                 m_create_info_ptr->get_residency_scope() == SPARSE_RESIDENCY_SCOPE_NONALIASED);

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
 **/
void Anvil::Image::on_memory_backing_opaque_update(VkDeviceSize        in_resource_offset,
                                                   VkDeviceSize        in_size,
                                                   Anvil::MemoryBlock* in_memory_block_ptr,
                                                   VkDeviceSize        in_memory_block_start_offset,
                                                   bool                in_memory_block_owned_by_image)
{
    const bool is_unbinding = (in_memory_block_ptr == nullptr);

    /* Sanity checks */
    anvil_assert(m_create_info_ptr->get_residency_scope() != Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED);

    if (in_memory_block_ptr != nullptr)
    {
        anvil_assert(in_memory_block_ptr->get_create_info_ptr()->get_size() <= in_memory_block_start_offset + in_size);
    }

    if (m_create_info_ptr->get_residency_scope() == Anvil::SPARSE_RESIDENCY_SCOPE_NONE)
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
                                  n_layer < m_create_info_ptr->get_n_layers();
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

    /* PTR_MANAGEMENT: Remove mem blocks no longer referenced by any page / til.e ! */
    if (in_memory_block_owned_by_image)
    {
        m_memory_blocks_owned.push_back(
            Anvil::MemoryBlockUniquePtr(in_memory_block_ptr,
                                       std::default_delete<MemoryBlock>() )
        );
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
void Anvil::Image::on_memory_backing_update(const VkImageSubresource& in_subresource,
                                            VkOffset3D                in_offset,
                                            VkExtent3D                in_extent,
                                            Anvil::MemoryBlock*       in_memory_block_ptr,
                                            VkDeviceSize              in_memory_block_start_offset,
                                            bool                      in_memory_block_owned_by_image)
{
    AspectPageOccupancyLayerData*                      aspect_layer_ptr;
    AspectPageOccupancyLayerMipData*                   aspect_layer_mip_ptr;
    decltype(m_sparse_aspect_page_occupancy)::iterator aspect_page_occupancy_iterator;
    decltype(m_sparse_aspect_props)::iterator          aspect_props_iterator;

    anvil_assert(m_create_info_ptr->get_residency_scope() == Anvil::SPARSE_RESIDENCY_SCOPE_ALIASED     ||
                 m_create_info_ptr->get_residency_scope() == Anvil::SPARSE_RESIDENCY_SCOPE_NONALIASED);

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
                               true); /* in_owned_by_image */
}

bool Anvil::Image::set_memory(Anvil::MemoryBlock* in_memory_block_ptr)
{
    return set_memory_internal(in_memory_block_ptr,
                               false); /* in_owned_by_image */
}

bool Anvil::Image::set_memory_internal(Anvil::MemoryBlock* in_memory_block_ptr,
                                       bool                in_owned_by_image)
{
    const Anvil::DeviceType device_type(m_device_ptr->get_type() );
    VkResult                result     (VK_ERROR_INITIALIZATION_FAILED);

    /* Sanity checks */
    anvil_assert(in_memory_block_ptr                      != nullptr);
    anvil_assert(m_create_info_ptr->get_residency_scope() == Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED);
    anvil_assert(m_mipmaps.size()                         >  0);
    anvil_assert(m_memory_blocks_owned.size()             == 0);
    anvil_assert(m_create_info_ptr->get_type()            != Anvil::ImageType::SWAPCHAIN_WRAPPER);

    /* Bind the memory object to the image object */
    if (device_type == Anvil::DEVICE_TYPE_SINGLE_GPU)
    {
        lock();
        {
            result = vkBindImageMemory(m_device_ptr->get_device_vk(),
                                       m_image,
                                       in_memory_block_ptr->get_memory(),
                                       in_memory_block_ptr->get_start_offset() );
        }
        unlock();
    }

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        const auto&   mips_to_upload   = m_create_info_ptr->get_mipmaps_to_upload();
        const auto    tiling           = m_create_info_ptr->get_tiling           ();

        VkImageLayout src_image_layout = (tiling == VK_IMAGE_TILING_LINEAR && mips_to_upload.size() > 0) ? VK_IMAGE_LAYOUT_PREINITIALIZED
                                                                                                         : VK_IMAGE_LAYOUT_UNDEFINED;

        if (in_owned_by_image)
        {
            m_memory_blocks_owned.push_back(
                MemoryBlockUniquePtr(in_memory_block_ptr,
                                     std::default_delete<MemoryBlock>() )
            );
        }

        /* Fill the storage with mipmap contents, if mipmap data was specified at input */
        if (mips_to_upload.size() > 0)
        {
            upload_mipmaps(&mips_to_upload,
                           src_image_layout,
                          &src_image_layout);
        }

        if (m_create_info_ptr->get_post_alloc_image_layout() != m_create_info_ptr->get_post_create_image_layout() )
        {
            const uint32_t n_mipmaps_to_upload = static_cast<uint32_t>(mips_to_upload.size());
            VkAccessFlags  src_access_mask     = 0;

            if (n_mipmaps_to_upload > 0)
            {
                if (tiling == VK_IMAGE_TILING_LINEAR)
                {
                    src_access_mask = VK_ACCESS_HOST_WRITE_BIT;
                }
                else
                {
                    src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
                }
            }

            transition_to_post_alloc_image_layout(src_access_mask,
                                                  src_image_layout);
        }

        m_create_info_ptr->clear_mipmaps_to_upload();
    }

    return is_vk_call_successful(result);
}

void Anvil::Image::transition_to_post_alloc_image_layout(VkAccessFlags in_source_access_mask,
                                                         VkImageLayout in_src_layout)
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
                                  VkImageLayout                     in_current_image_layout,
                                  VkImageLayout*                    out_new_image_layout_ptr)
{
    std::map<VkImageAspectFlagBits, std::vector<const Anvil::MipmapRawData*> > image_aspect_to_mipmap_raw_data_map;
    VkImageAspectFlags                                                         image_aspects_touched              (0);
    VkImageSubresourceRange                                                    image_subresource_range;
    const VkDeviceSize                                                         sparse_page_size                   ( (m_page_tracker_ptr != nullptr) ? m_page_tracker_ptr->get_page_size() : 0);
    Anvil::Queue*                                                              universal_queue_ptr                (m_device_ptr->get_universal_queue(0) );

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
    image_subresource_range.aspectMask     = image_aspects_touched;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.baseMipLevel   = 0;
    image_subresource_range.layerCount     = m_create_info_ptr->get_n_layers();
    image_subresource_range.levelCount     = m_n_mipmaps;

    if (m_create_info_ptr->get_tiling() == VK_IMAGE_TILING_LINEAR)
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

                vkGetImageSubresourceLayout(m_device_ptr->get_device_vk(),
                                            m_image,
                                           &image_subresource,
                                           &image_subresource_layout);

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
                    dst_slice_offset = image_subresource_layout.offset               +
                                       image_subresource_layout.depthPitch * n_slice;
                    src_slice_ptr    = current_mipmap_data_ptr                       +
                                       current_slice_size                  * n_slice;

                    for (unsigned int n_row = 0;
                                      n_row < current_mipmap_height;
                                    ++n_row, dst_slice_offset += image_subresource_layout.rowPitch)
                    {
                        Anvil::MemoryBlock* mem_block_ptr          = nullptr;
                        VkDeviceSize        write_dst_slice_offset = dst_slice_offset;

                        if (m_create_info_ptr->get_residency_scope() == Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED)
                        {
                            anvil_assert(m_memory_blocks_owned.size() == 1);

                            mem_block_ptr = m_memory_blocks_owned.at(0).get();
                        }
                        else
                        if (m_create_info_ptr->get_residency_scope() == SPARSE_RESIDENCY_SCOPE_NONE)
                        {
                            const VkDeviceSize dst_slice_offset_page_aligned = Anvil::Utils::round_down(dst_slice_offset,
                                                                                                        sparse_page_size);
                            VkDeviceSize       memory_region_start_offset    = UINT32_MAX;

                            anvil_assert(sparse_page_size                                     != 0);
                            anvil_assert(( dst_slice_offset_page_aligned % sparse_page_size)  == 0);

                            mem_block_ptr = m_page_tracker_ptr->get_memory_block(dst_slice_offset_page_aligned,
                                                                                 sparse_page_size,
                                                                                &memory_region_start_offset);
                            anvil_assert(mem_block_ptr != nullptr);

                            if (dst_slice_offset + image_subresource_layout.rowPitch > dst_slice_offset_page_aligned + sparse_page_size)
                            {
                                VkDeviceSize dummy;
                                auto         mem_block2_ptr = m_page_tracker_ptr->get_memory_block(dst_slice_offset_page_aligned + sparse_page_size,
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
        anvil_assert(m_create_info_ptr->get_tiling() == VK_IMAGE_TILING_OPTIMAL);

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
            auto create_info_ptr = Anvil::BufferCreateInfo::create_nonsparse_alloc(m_device_ptr,
                                                                                   total_raw_mips_size,
                                                                                   Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
                                                                                   VK_SHARING_MODE_EXCLUSIVE,
                                                                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                                   0); /* in_memory_features */

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
            std::vector<VkBufferImageCopy> copy_regions;

            /* Transfer the image to the transfer_destination layout if not already in this or general layout */
            if (in_current_image_layout != VK_IMAGE_LAYOUT_GENERAL              &&
                in_current_image_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                const auto          sharing_mode   (m_create_info_ptr->get_sharing_mode() );
                const auto          queue_fam_index((sharing_mode == VK_SHARING_MODE_EXCLUSIVE) ? universal_queue_ptr->get_queue_family_index() : VK_QUEUE_FAMILY_IGNORED);

                Anvil::ImageBarrier image_barrier  (0, /* source_access_mask */
                                                    VK_ACCESS_TRANSFER_WRITE_BIT,
                                                    false,
                                                    in_current_image_layout,
                                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                    queue_fam_index,
                                                    queue_fam_index,
                                                    this,
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
                const auto        base_mip_width      = m_create_info_ptr->get_base_mip_width ();
                VkBufferImageCopy current_copy_region;
                const auto&       current_mipmap      = *mipmap_iterator;

                current_copy_region.bufferImageHeight               = std::max(base_mip_height / (1 << current_mipmap.n_mipmap), 1u);
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
                current_copy_region.imageExtent.height              = std::max(base_mip_height / (1 << current_mipmap.n_mipmap), 1u);
                current_copy_region.imageExtent.width               = std::max(base_mip_width  / (1 << current_mipmap.n_mipmap), 1u);

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