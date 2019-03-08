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
#include "misc/object_tracker.h"
#include "misc/struct_chainer.h"
#include "wrappers/buffer.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/device.h"
#include "wrappers/instance.h"
#include "wrappers/memory_block.h"
#include "wrappers/physical_device.h"
#include "wrappers/queue.h"

Anvil::Buffer::Buffer(Anvil::BufferCreateInfoUniquePtr in_create_info_ptr)
    :CallbacksSupportProvider          (BUFFER_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider<Buffer>(in_create_info_ptr->get_device(),
                                        Anvil::ObjectType::BUFFER),
     MTSafetySupportProvider           (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                        in_create_info_ptr->get_device   () )),
     m_buffer                          (VK_NULL_HANDLE),
     m_memory_block_ptr                (nullptr),
     m_prefers_dedicated_allocation    (false),
     m_requires_dedicated_allocation   (false),
     m_staging_buffer_queue_ptr        (nullptr)
{
    if (in_create_info_ptr->get_type() == BufferType::NO_ALLOC)
    {
        if (((in_create_info_ptr->get_memory_features() & Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT)        == 0) ||
            ((in_create_info_ptr->get_create_flags   () & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   == 0) ||
            ((in_create_info_ptr->get_create_flags   () & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) == 0))
        {
            /* For host->gpu writes to work in this case, we will need the buffer to work as a target
             * for buffer->buffer copy operations. Same goes for the other way around.
             */
            auto usage_flags = in_create_info_ptr->get_usage_flags();

            usage_flags |= Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT | Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT;

            in_create_info_ptr->set_usage_flags(usage_flags);
        }
    }

    m_create_info_ptr = std::move(in_create_info_ptr);
}


/** Releases a buffer object and a memory object associated with this Buffer instance. */
Anvil::Buffer::~Buffer()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::BUFFER,
                                                   this);

    if (m_buffer                                   != VK_NULL_HANDLE &&
        m_create_info_ptr->get_parent_buffer_ptr() == nullptr)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyBuffer(m_device_ptr->get_device_vk(),
                                           m_buffer,
                                           nullptr /* pAllocator */);
        }
        unlock();

        m_buffer = VK_NULL_HANDLE;
    }
}

Anvil::BufferUniquePtr Anvil::Buffer::create(Anvil::BufferCreateInfoUniquePtr in_create_info_ptr)
{
    Anvil::BufferUniquePtr new_buffer_ptr(nullptr,
                                          std::default_delete<Anvil::Buffer>() );

    new_buffer_ptr.reset(
        new Anvil::Buffer(std::move(in_create_info_ptr) )
    );

    if (new_buffer_ptr != nullptr)
    {
        if (!new_buffer_ptr->init() )
        {
            new_buffer_ptr.reset();
        }
    }

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::BUFFER,
                                                 new_buffer_ptr.get() );

    return new_buffer_ptr;
}

/* Please see header for specification */
const Anvil::Buffer* Anvil::Buffer::get_base_buffer()
{
    const Anvil::Buffer* result_ptr = this;
    Anvil::Buffer*       parent_ptr = nullptr;

    if ( (parent_ptr = result_ptr->get_create_info_ptr()->get_parent_buffer_ptr()) != nullptr)
    {
        result_ptr = parent_ptr;
    }

    return result_ptr;
}

/* Please see header for specification */
VkBuffer Anvil::Buffer::get_buffer(const bool& in_bake_memory_if_necessary)
{
    const auto& create_flags = get_create_info_ptr()->get_create_flags();
    const bool  is_sparse    = ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   != 0) ||
                               ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_BINDING_BIT)   != 0) ||
                               ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0);

    if (!is_sparse)
    {
        if (in_bake_memory_if_necessary            &&
            m_memory_block_ptr          == nullptr)
        {
            get_memory_block(0 /* in_n_memory_block */);
        }
    }

    return m_buffer;
}

/* Please see header for specification */
Anvil::MemoryBlock* Anvil::Buffer::get_memory_block(uint32_t in_n_memory_block)
{
    const auto& create_flags       = get_create_info_ptr()->get_create_flags();
    bool        is_callback_needed = false;
    const bool  is_sparse          = ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   != 0) ||
                                     ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_BINDING_BIT)   != 0) ||
                                     ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0);

    if (is_sparse)
    {
        IsBufferMemoryAllocPendingQueryCallbackArgument callback_arg(this);

        callback(BUFFER_CALLBACK_ID_IS_ALLOC_PENDING,
                &callback_arg);

        is_callback_needed = callback_arg.result;
    }
    else
    {
        is_callback_needed = (m_memory_block_ptr == nullptr);
    }

    if (is_callback_needed)
    {
        OnMemoryBlockNeededForBufferCallbackArgument callback_argument(this);

        anvil_assert(m_create_info_ptr->get_parent_buffer_ptr() == nullptr);

        callback_safe(BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED,
                     &callback_argument);
    }

    if (is_sparse)
    {
        return m_page_tracker_ptr->get_memory_block(in_n_memory_block);
    }
    else
    {
        return m_memory_block_ptr;
    }
}

VkMemoryRequirements Anvil::Buffer::get_memory_requirements() const
{
    auto parent_buffer_ptr = m_create_info_ptr->get_parent_buffer_ptr();

    if (parent_buffer_ptr != nullptr)
    {
        return parent_buffer_ptr->get_memory_requirements();
    }
    else
    {
        return m_buffer_memory_reqs;
    }
}

uint32_t Anvil::Buffer::get_n_memory_blocks() const
{
    const auto& create_flags = get_create_info_ptr()->get_create_flags();
    const bool  is_sparse    = ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   != 0) ||
                               ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0);

    if (is_sparse)
    {
        return m_page_tracker_ptr->get_n_memory_blocks();
    }
    else
    {
        return 1;
    }
}

bool Anvil::Buffer::init()
{
    uint32_t                                 n_queue_family_indices;
    uint32_t                                 queue_family_indices   [8];
    VkResult                                 result                  (VK_ERROR_INITIALIZATION_FAILED);
    Anvil::StructChainer<VkBufferCreateInfo> struct_chainer;
    bool                                     use_dedicated_allocation(false);

    if ( m_create_info_ptr->get_client_data    ()                                               != nullptr &&
        (m_create_info_ptr->get_memory_features() & Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) == 0)
    {
        m_create_info_ptr->set_usage_flags(m_create_info_ptr->get_usage_flags() | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT);
    }

    if (m_create_info_ptr->get_type() != BufferType::NO_ALLOC_CHILD)
    {
        /* Determine which queues the buffer should be available to. */
        Anvil::Utils::convert_queue_family_bits_to_family_indices(m_device_ptr,
                                                                  m_create_info_ptr->get_queue_families(),
                                                                  queue_family_indices,
                                                                 &n_queue_family_indices);

        anvil_assert(n_queue_family_indices > 0);
        anvil_assert(n_queue_family_indices < sizeof(queue_family_indices) / sizeof(queue_family_indices[0]) );

        /* Prepare the create info structure */
        {
            VkBufferCreateInfo buffer_create_info;

            buffer_create_info.flags                 = m_create_info_ptr->get_create_flags().get_vk();
            buffer_create_info.pNext                 = nullptr;
            buffer_create_info.pQueueFamilyIndices   = queue_family_indices;
            buffer_create_info.queueFamilyIndexCount = n_queue_family_indices;
            buffer_create_info.sharingMode           = static_cast<VkSharingMode>(m_create_info_ptr->get_sharing_mode() );
            buffer_create_info.size                  = m_create_info_ptr->get_size();
            buffer_create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_create_info.usage                 = m_create_info_ptr->get_usage_flags().get_vk();

            struct_chainer.append_struct(buffer_create_info);
        }

        {
            const auto& external_memory_handle_types = m_create_info_ptr->get_exportable_external_memory_handle_types();

            if (external_memory_handle_types != 0)
            {
                VkExternalMemoryBufferCreateInfoKHR external_memory_buffer_create_info;

                external_memory_buffer_create_info.handleTypes = external_memory_handle_types.get_vk();
                external_memory_buffer_create_info.pNext       = nullptr;
                external_memory_buffer_create_info.sType       = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR;

                struct_chainer.append_struct(external_memory_buffer_create_info);
            }
        }

        /* Create the buffer object */
        {
            auto struct_chain_ptr = struct_chainer.create_chain();

            result = Anvil::Vulkan::vkCreateBuffer(m_device_ptr->get_device_vk(),
                                                   struct_chain_ptr->get_root_struct(),
                                                   nullptr, /* pAllocator */
                                                  &m_buffer);
        }

        anvil_assert_vk_call_succeeded(result);
        if (is_vk_call_successful(result) )
        {
            set_vk_handle(m_buffer);

            /* Cache buffer data memory requirements.
             *
             * Prefer facility exposed by VK_KHR_get_memory_requirements2, unless the extension is unavailable.
             */
            if (m_device_ptr->get_extension_info()->khr_get_memory_requirements2() )
            {
                Anvil::StructID                                       dedicated_reqs_struct_id           = static_cast<Anvil::StructID>(UINT32_MAX);
                const auto                                            gmr2_entrypoints                   = m_device_ptr->get_extension_khr_get_memory_requirements2_entrypoints();
                VkBufferMemoryRequirementsInfo2KHR                    info;
                const bool                                            khr_dedicated_allocation_available = m_device_ptr->get_extension_info()->khr_dedicated_allocation();
                Anvil::StructChainUniquePtr<VkMemoryRequirements2KHR> result_reqs_chain_ptr;
                VkMemoryRequirements2KHR*                             result_reqs_chain_raw_ptr          = nullptr;
                Anvil::StructChainer<VkMemoryRequirements2KHR>        result_reqs_chainer;

                info.buffer = m_buffer;
                info.pNext  = nullptr;
                info.sType  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2_KHR;

                {
                    VkMemoryRequirements2KHR reqs;

                    reqs.pNext = nullptr;
                    reqs.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;

                    result_reqs_chainer.append_struct(reqs);
                }

                if (khr_dedicated_allocation_available)
                {
                    VkMemoryDedicatedRequirementsKHR dedicated_reqs;

                    dedicated_reqs.pNext                       = nullptr;
                    dedicated_reqs.prefersDedicatedAllocation  = VK_FALSE;
                    dedicated_reqs.requiresDedicatedAllocation = VK_FALSE;
                    dedicated_reqs.sType                       = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;

                    dedicated_reqs_struct_id = result_reqs_chainer.append_struct(dedicated_reqs);
                }

                result_reqs_chain_ptr = result_reqs_chainer.create_chain();
                anvil_assert(result_reqs_chain_ptr != nullptr);

                result_reqs_chain_raw_ptr = result_reqs_chain_ptr->get_root_struct();
                anvil_assert(result_reqs_chain_raw_ptr != nullptr);

                gmr2_entrypoints.vkGetBufferMemoryRequirements2KHR(m_device_ptr->get_device_vk(),
                                                                  &info,
                                                                   result_reqs_chain_raw_ptr);

                m_buffer_memory_reqs = result_reqs_chain_raw_ptr->memoryRequirements;

                if (khr_dedicated_allocation_available)
                {
                    const auto dedicated_alloc_info_ptr = result_reqs_chain_ptr->get_struct_with_id<VkMemoryDedicatedRequirementsKHR>(dedicated_reqs_struct_id);

                    m_prefers_dedicated_allocation  = (dedicated_alloc_info_ptr->prefersDedicatedAllocation  == VK_TRUE);
                    m_requires_dedicated_allocation = (dedicated_alloc_info_ptr->requiresDedicatedAllocation == VK_TRUE);

                    use_dedicated_allocation = m_requires_dedicated_allocation;
                }
            }
            else
            {
                Anvil::Vulkan::vkGetBufferMemoryRequirements(m_device_ptr->get_device_vk(),
                                                             m_buffer,
                                                            &m_buffer_memory_reqs);
            }
        }
    }
    else
    {
        m_buffer = m_create_info_ptr->get_parent_buffer_ptr()->m_buffer;

        anvil_assert(m_buffer != VK_NULL_HANDLE);
        if (m_buffer != VK_NULL_HANDLE)
        {
            result = VK_SUCCESS;
        }
    }

    switch (m_create_info_ptr->get_type() )
    {
        case Anvil::BufferType::ALLOC:
        {
            /* Create a memory object and preallocate as much space as we need */
            auto                        client_data_ptr  = m_create_info_ptr->get_client_data();
            Anvil::MemoryBlockUniquePtr memory_block_ptr;

            {
                auto create_info_ptr = Anvil::MemoryBlockCreateInfo::create_regular(m_create_info_ptr->get_device(),
                                                                                    m_buffer_memory_reqs.memoryTypeBits,
                                                                                    m_buffer_memory_reqs.size,
                                                                                    m_create_info_ptr->get_memory_features() );

                create_info_ptr->set_mt_safety(m_create_info_ptr->get_mt_safety() );

                if (use_dedicated_allocation)
                {
                    create_info_ptr->use_dedicated_allocation(this,
                                                              nullptr); /* in_opt_image_ptr */
                }

                memory_block_ptr = Anvil::MemoryBlock::create(std::move(create_info_ptr) );
            }

            if (!set_nonsparse_memory( std::move(memory_block_ptr) ))
            {
                anvil_assert_fail();

                result = VK_ERROR_INITIALIZATION_FAILED;

                goto end;
            }

            if (client_data_ptr != nullptr)
            {
                if (!write(0, /* in_start_offset */
                           m_create_info_ptr->get_size(),
                           client_data_ptr) )
                {
                    anvil_assert_fail();

                    result = VK_ERROR_INITIALIZATION_FAILED;

                    goto end;
                }
            }

            break;
        }

        case Anvil::BufferType::NO_ALLOC_CHILD:
        {
            Anvil::MemoryBlockUniquePtr mem_block_ptr;

            {
                auto create_info_ptr = Anvil::MemoryBlockCreateInfo::create_derived(m_create_info_ptr->get_parent_buffer_ptr()->get_memory_block(0 /* in_n_memory_block */),
                                                                                    m_create_info_ptr->get_start_offset(),
                                                                                    m_create_info_ptr->get_size() );

                mem_block_ptr = Anvil::MemoryBlock::create(std::move(create_info_ptr) );

                if (use_dedicated_allocation)
                {
                    create_info_ptr->use_dedicated_allocation(this,
                                                              nullptr); /* in_opt_image_ptr */
                }
            }

            anvil_assert(!is_memory_block_owned(mem_block_ptr.get()));

            m_owned_memory_blocks.push_back(std::move(mem_block_ptr) );

            m_memory_block_ptr = m_owned_memory_blocks.back().get();
            anvil_assert(m_memory_block_ptr != nullptr);

            break;
        }

        case BufferType::NO_ALLOC:
        {
            const auto& create_flags = get_create_info_ptr()->get_create_flags();
            const bool  is_sparse    = ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   != 0) ||
                                       ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_BINDING_BIT)   != 0) ||
                                       ((create_flags & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0);

            if (is_sparse)
            {
                m_page_tracker_ptr.reset(
                    new Anvil::PageTracker(Anvil::Utils::round_up(m_create_info_ptr->get_size(),
                                                                  m_buffer_memory_reqs.alignment),
                                           m_buffer_memory_reqs.alignment)
                );
            }

            /* No special action needed */
            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }

end:
    return is_vk_call_successful(result);
}

/* TODO */
bool Anvil::Buffer::init_staging_buffer(const VkDeviceSize& in_size,
                                        Anvil::Queue*       in_opt_queue_ptr)
{
    Anvil::PrimaryCommandBufferUniquePtr      copy_cmdbuf_ptr;
    std::vector<const Anvil::PhysicalDevice*> helper_vec;
    const auto                                queue_fams                    = m_create_info_ptr->get_queue_families();
    Anvil::QueueFamilyFlagBits                staging_buffer_queue_fam_bits = Anvil::QueueFamilyFlagBits::NONE;

    m_staging_buffer_ptr.reset();
    m_staging_buffer_queue_ptr = nullptr;

    if (m_create_info_ptr->get_sharing_mode() == Anvil::SharingMode::EXCLUSIVE)
    {
        /* We need to use a user-specified queue, since we can't tell which queue fam the buffer is currently configured to be compatible with.
         *
         * This can be worked around if the buffer can only be used with a specific queue family type.
         */
        if (Anvil::Utils::is_pow2(queue_fams.get_vk() ) )
        {
            switch (queue_fams.get_vk() )
            {
                case static_cast<uint32_t>(Anvil::QueueFamilyFlagBits::COMPUTE_BIT):  m_staging_buffer_queue_ptr = m_device_ptr->get_compute_queue  (0); break;
                case static_cast<uint32_t>(Anvil::QueueFamilyFlagBits::DMA_BIT):      m_staging_buffer_queue_ptr = m_device_ptr->get_transfer_queue (0); break;
                case static_cast<uint32_t>(Anvil::QueueFamilyFlagBits::GRAPHICS_BIT): m_staging_buffer_queue_ptr = m_device_ptr->get_universal_queue(0); break;

                default:
                {
                    anvil_assert_fail();
                }
            }
        }
        else
        {
            anvil_assert(in_opt_queue_ptr != nullptr);

            m_staging_buffer_queue_ptr = in_opt_queue_ptr;
        }

        anvil_assert(m_staging_buffer_queue_ptr != nullptr);

        switch (m_device_ptr->get_queue_family_type(m_staging_buffer_queue_ptr->get_queue_family_index() ) )
        {
            case Anvil::QueueFamilyType::COMPUTE:   staging_buffer_queue_fam_bits = Anvil::QueueFamilyFlagBits::COMPUTE_BIT;  break;
            case Anvil::QueueFamilyType::TRANSFER:  staging_buffer_queue_fam_bits = Anvil::QueueFamilyFlagBits::DMA_BIT;      break;
            case Anvil::QueueFamilyType::UNIVERSAL: staging_buffer_queue_fam_bits = Anvil::QueueFamilyFlagBits::GRAPHICS_BIT; break;

            default:
            {
                anvil_assert_fail();
            }
        }
    }
    else
    {
        /* We can use any queue from the list of queue fams this buffer is compatible with, in order to perform the copy op. */
        if ((queue_fams & Anvil::QueueFamilyFlagBits::GRAPHICS_BIT) != 0)
        {
            m_staging_buffer_queue_ptr    = m_device_ptr->get_universal_queue(0);
            staging_buffer_queue_fam_bits = Anvil::QueueFamilyFlagBits::GRAPHICS_BIT;
        }
        else
        if ((queue_fams & Anvil::QueueFamilyFlagBits::DMA_BIT) != 0)
        {
            m_staging_buffer_queue_ptr    = m_device_ptr->get_transfer_queue(0);
            staging_buffer_queue_fam_bits = Anvil::QueueFamilyFlagBits::DMA_BIT;
        }
        else
        {
            anvil_assert((queue_fams & Anvil::QueueFamilyFlagBits::COMPUTE_BIT) != 0)

            m_staging_buffer_queue_ptr    = m_device_ptr->get_compute_queue(0);
            staging_buffer_queue_fam_bits = Anvil::QueueFamilyFlagBits::COMPUTE_BIT;
        }
    }

    if (m_staging_buffer_ptr == nullptr                                   ||
        m_staging_buffer_ptr->get_create_info_ptr()->get_size() < in_size)
    {
        {
            const auto sharing_mode    = Anvil::Utils::is_pow2(static_cast<uint32_t>(staging_buffer_queue_fam_bits) ) ? Anvil::SharingMode::EXCLUSIVE
                                                                                                                      : Anvil::SharingMode::CONCURRENT;

            auto       create_info_ptr = Anvil::BufferCreateInfo::create_alloc(m_device_ptr,
                                                                               in_size,
                                                                               staging_buffer_queue_fam_bits,
                                                                               sharing_mode,
                                                                               Anvil::BufferCreateFlagBits::NONE,
                                                                               Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT | Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT,
                                                                               Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT);

            create_info_ptr->set_mt_safety  (Anvil::MTSafety::DISABLED);

            m_staging_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
        }

        if (m_staging_buffer_ptr == nullptr)
        {
            anvil_assert(m_staging_buffer_ptr != nullptr);
        }
    }

    return (m_staging_buffer_ptr != nullptr);
}

/* Please see header for specification */
bool Anvil::Buffer::read(VkDeviceSize in_start_offset,
                         VkDeviceSize in_size,
                         void*        out_result_ptr)
{
    return read(in_start_offset,
                in_size,
                UINT32_MAX, /* in_device_mask */
                out_result_ptr);
}

/* Please see header for specification */
bool Anvil::Buffer::read(VkDeviceSize in_start_offset,
                         VkDeviceSize in_size,
                         uint32_t     in_device_mask,
                         void*        out_result_ptr)
{
    const Anvil::DeviceType device_type      (m_device_ptr->get_type() );
    auto                    memory_block_ptr (get_memory_block(0 /* in_n_memory_block */) );
    bool                    result           (false);

    /* TODO: Complete support for sparsely-bound & sparse-resident buffers */
    anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) == 0);

    if ((m_create_info_ptr->get_create_flags() & Anvil::BufferCreateFlagBits::SPARSE_BINDING_BIT) != 0)
    {
        anvil_assert(m_page_tracker_ptr->get_n_memory_blocks            () == 1);
        anvil_assert(m_page_tracker_ptr->get_n_pages_with_memory_backing() == m_page_tracker_ptr->get_n_pages() );
    }


    if ((memory_block_ptr->get_create_info_ptr()->get_memory_features() & Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) != 0)
    {
        result = memory_block_ptr->read(in_start_offset,
                                        in_size,
                                        out_result_ptr);
    }
    else
    {
        /* The buffer memory is not mappable. We need to create a staging buffer,
         * do a non-mappable->mappable memory copy, and then read back data from the mappable buffer. */
        Anvil::PrimaryCommandBufferUniquePtr copy_cmdbuf_ptr;

        if (m_staging_buffer_ptr                                    == nullptr ||
            m_staging_buffer_ptr->get_create_info_ptr()->get_size() <  in_size)
        {
            if (!init_staging_buffer(in_size,
                                     nullptr) ) /* in_opt_queue_ptr */
            {
                result = false;

                goto end;
            }
        }

        if (m_staging_buffer_ptr == nullptr)
        {
            anvil_assert(m_staging_buffer_ptr != nullptr);

            goto end;
        }

        copy_cmdbuf_ptr = m_device_ptr->get_command_pool_for_queue_family_index(m_staging_buffer_queue_ptr->get_queue_family_index() )->alloc_primary_level_command_buffer();

        if (copy_cmdbuf_ptr == nullptr)
        {
            anvil_assert(copy_cmdbuf_ptr != nullptr);

            goto end;

        }
        if (device_type == Anvil::DeviceType::SINGLE_GPU)
        {
            copy_cmdbuf_ptr->start_recording(true,   /* one_time_submit          */
                                             false); /* simultaneous_use_allowed */
        }
        else
        {
            anvil_assert(device_type == Anvil::DeviceType::MULTI_GPU);
            anvil_assert(Utils::count_set_bits(in_device_mask) == 1);

            copy_cmdbuf_ptr->start_recording(true,  /* one_time_submit          */
                                             false, /* simultaneous_use_allowed */
                                             in_device_mask); /* in_opt_device_mask */
        }
        {
            Anvil::BufferBarrier buffer_barrier(Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,
                                                Anvil::AccessFlagBits::HOST_READ_BIT,
                                                VK_QUEUE_FAMILY_IGNORED,
                                                VK_QUEUE_FAMILY_IGNORED,
                                                m_staging_buffer_ptr.get(),
                                                0, /* in_offset */
                                                in_size);
            Anvil::BufferCopy    copy_region;
            Anvil::MemoryBarrier pre_copy_barrier(Anvil::AccessFlagBits::TRANSFER_READ_BIT, /* in_destination_access_mask */
                                                  Anvil::AccessFlagBits::HOST_WRITE_BIT | Anvil::AccessFlagBits::MEMORY_WRITE_BIT | Anvil::AccessFlagBits::SHADER_WRITE_BIT | Anvil::AccessFlagBits::TRANSFER_WRITE_BIT);

            copy_region.dst_offset = 0;
            copy_region.size       = in_size;
            copy_region.src_offset = in_start_offset;

            copy_cmdbuf_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT, /* in_src_stage_mask */
                                                     Anvil::PipelineStageFlagBits::TRANSFER_BIT,     /* in_dst_stage_mask */
                                                     Anvil::DependencyFlagBits::NONE,
                                                     1, /* in_memory_barrier_count */
                                                    &pre_copy_barrier,
                                                     0,        /* in_buffer_memory_barrier_count */
                                                     nullptr,  /* in_buffer_memory_barriers_ptr  */
                                                     0,        /* in_image_memory_barrier_count  */
                                                     nullptr); /* in_iamge_memory_barriers_ptr   */

            copy_cmdbuf_ptr->record_copy_buffer     (this,
                                                     m_staging_buffer_ptr.get(),
                                                     1, /* in_region_count */
                                                    &copy_region);
            copy_cmdbuf_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::TRANSFER_BIT,
                                                     Anvil::PipelineStageFlagBits::HOST_BIT,
                                                     Anvil::DependencyFlagBits::NONE,
                                                     0,               /* in_memory_barrier_count        */
                                                     nullptr,         /* in_memory_barriers_ptr         */
                                                     1,               /* in_buffer_memory_barrier_count */
                                                     &buffer_barrier,
                                                     0,               /* in_image_memory_barrier_count */
                                                     nullptr);        /* in_image_memory_barriers_ptr  */
        }
        copy_cmdbuf_ptr->stop_recording();

        if (device_type == Anvil::DeviceType::SINGLE_GPU)
        {
            m_staging_buffer_queue_ptr->submit(
                Anvil::SubmitInfo::create_execute(copy_cmdbuf_ptr.get(),
                                                  true /* should_block */)
            );
        }
        else
        {
            Anvil::CommandBufferMGPUSubmission cmd_buffer_submission;

            anvil_assert((in_device_mask != 0) && (in_device_mask != UINT32_MAX));

            cmd_buffer_submission.cmd_buffer_ptr = copy_cmdbuf_ptr.get();
            cmd_buffer_submission.device_mask    = in_device_mask;

            m_staging_buffer_queue_ptr->submit(
                Anvil::SubmitInfo::create_execute(&cmd_buffer_submission,
                                                  1, /* in_n_command_buffer_submissions */
                                                  true /* should_block */)
            );
        }

        result = m_staging_buffer_ptr->read(0,
                                            in_size,
                                            out_result_ptr);
    }

end:
    return result;
}

bool Anvil::Buffer::set_memory_nonsparse_internal(MemoryBlockUniquePtr  in_memory_block_ptr,
                                                  uint32_t              in_n_device_group_indices,
                                                  const uint32_t*       in_device_group_indices_ptr)
{
    const Anvil::DeviceType      device_type             (m_device_ptr->get_type() );
    bool                         result                  (false);
    VkResult                     result_vk;

    if (in_memory_block_ptr == nullptr)
    {
        anvil_assert(!(in_memory_block_ptr == nullptr) );

        goto end;
    }

    if (m_memory_block_ptr != nullptr)
    {
        anvil_assert(m_memory_block_ptr == nullptr);

        goto end;
    }

    if (((m_create_info_ptr->get_create_flags() & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   != 0) ||
        ((m_create_info_ptr->get_create_flags() & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0) )
    {
        anvil_assert(((m_create_info_ptr->get_create_flags() & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   == 0) &&
                     ((m_create_info_ptr->get_create_flags() & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) == 0) );

        goto end;
    }

    /* Bind the memory object to the buffer object */
    if (device_type == Anvil::DeviceType::SINGLE_GPU)
    {
        lock();
        {
            result_vk = Anvil::Vulkan::vkBindBufferMemory(m_device_ptr->get_device_vk(),
                                                          m_buffer,
                                                          in_memory_block_ptr->get_memory      (),
                                                          in_memory_block_ptr->get_start_offset() );
        }
        unlock();
    }
    else
    {
        StructChainUniquePtr<VkBindBufferMemoryInfoKHR> bind_info_struct_chain_ptr;
        Anvil::StructChainer<VkBindBufferMemoryInfoKHR> bind_info_struct_chainer;

        const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );
        const auto&              entrypoints    (mgpu_device_ptr->get_extension_khr_bind_memory2_entrypoints() );

        anvil_assert(device_type == Anvil::DeviceType::MULTI_GPU);

        {
            VkBindBufferMemoryInfoKHR            bind_info;
            VkBindBufferMemoryDeviceGroupInfoKHR bind_info_dg;

            bind_info_dg.deviceIndexCount = in_n_device_group_indices;
            bind_info_dg.pDeviceIndices   = in_device_group_indices_ptr;
            bind_info_dg.pNext            = nullptr;
            bind_info_dg.sType            = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO_KHR;

            bind_info.buffer        = m_buffer;
            bind_info.memory        = in_memory_block_ptr->get_memory      ();
            bind_info.memoryOffset  = in_memory_block_ptr->get_start_offset();
            bind_info.pNext         = nullptr;
            bind_info.sType         = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO_KHR;

            bind_info_struct_chainer.append_struct(bind_info);
            bind_info_struct_chainer.append_struct(bind_info_dg);
        }

        bind_info_struct_chain_ptr = bind_info_struct_chainer.create_chain();

        result_vk = entrypoints.vkBindBufferMemory2KHR(m_device_ptr->get_device_vk(),
                                                       1, /* bindInfoCount */
                                                       bind_info_struct_chain_ptr ->get_root_struct() );
    }

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert_vk_call_succeeded(result_vk);

        goto end;
    }

    /* All done */
    m_memory_block_ptr = in_memory_block_ptr.get();
    result             = true;

    anvil_assert(!is_memory_block_owned(in_memory_block_ptr.get()));

    m_owned_memory_blocks.push_back(
        std::move(in_memory_block_ptr)
    );

end:
    return result;
}

/* Please see header for specification */
bool Anvil::Buffer::set_memory_sparse(MemoryBlock* in_memory_block_ptr,
                                      bool         in_memory_block_owned_by_buffer,
                                      VkDeviceSize in_memory_block_start_offset,
                                      VkDeviceSize in_start_offset,
                                      VkDeviceSize in_size)
{
    anvil_assert((m_create_info_ptr->get_create_flags() & Anvil::BufferCreateFlagBits::SPARSE_BINDING_BIT) != 0);

    if (m_page_tracker_ptr->set_binding(in_memory_block_ptr,
                                        in_memory_block_start_offset,
                                        in_start_offset,
                                        in_size) )
    {
        if (in_memory_block_owned_by_buffer && !is_memory_block_owned(in_memory_block_ptr))
        {
            MemoryBlockUniquePtr mem_block_ptr(in_memory_block_ptr,
                                               std::default_delete<MemoryBlock>() );

            m_owned_memory_blocks.push_back(
                std::move(mem_block_ptr)
            );
        }

        return true;
    }
    else
    {
        return false;
    }
}

/* Please see header for specification */
bool Anvil::Buffer::set_nonsparse_memory(MemoryBlockUniquePtr in_memory_block_ptr)
{
    MemoryBlock* memory_block_raw_ptr = in_memory_block_ptr.release();

    return set_nonsparse_memory(memory_block_raw_ptr,
                                true);
}

bool Anvil::Buffer::set_nonsparse_memory(MemoryBlock* in_memory_block_ptr,
                                         bool         in_memory_block_owned_by_buffer)
{
    MemoryBlockUniquePtr memory_block_ptr;

    if (in_memory_block_owned_by_buffer)
    {
        memory_block_ptr = MemoryBlockUniquePtr(in_memory_block_ptr,
                                                std::default_delete<Anvil::MemoryBlock>() );
    }
    else
    {
        memory_block_ptr = MemoryBlockUniquePtr(in_memory_block_ptr,
                                                [](MemoryBlock*)
                                                {
                                                    /* Stub */
                                                });
    }

    return set_memory_nonsparse_internal(std::move(memory_block_ptr),
                                         0,        /* n_physical_devices   */
                                         nullptr); /* physical_devices_ptr */
}

/* Please see header for specification */
bool Anvil::Buffer::set_nonsparse_memory(MemoryBlockUniquePtr   in_memory_block_ptr,
                                         uint32_t               in_n_device_group_indices,
                                         const uint32_t*        in_device_group_indices_ptr)
{
    MemoryBlockUniquePtr memory_block_ptr = MemoryBlockUniquePtr(in_memory_block_ptr.release(),
                                                                 std::default_delete<Anvil::MemoryBlock>() );

    return set_memory_nonsparse_internal(std::move(memory_block_ptr),
                                         in_n_device_group_indices,
                                         in_device_group_indices_ptr);
}

/* Please see header for specification */
bool Anvil::Buffer::set_nonsparse_memory(MemoryBlock*       in_memory_block_ptr,
                                         bool               in_memory_block_owned_by_buffer,
                                         uint32_t           in_n_device_group_indices,
                                         const uint32_t*    in_device_group_indices_ptr)
{
    MemoryBlockUniquePtr memory_block_ptr;

    if (in_memory_block_owned_by_buffer)
    {
        memory_block_ptr = MemoryBlockUniquePtr(in_memory_block_ptr,
                                                std::default_delete<Anvil::MemoryBlock>() );
    }
    else
    {
        memory_block_ptr = MemoryBlockUniquePtr(in_memory_block_ptr,
                                                [](MemoryBlock*)
                                                {
                                                    /* Stub */
                                                });
    }

    return set_memory_nonsparse_internal(std::move(memory_block_ptr),
                                         in_n_device_group_indices,
                                         in_device_group_indices_ptr);
}

/* Please see header for specification */
bool Anvil::Buffer::set_nonsparse_memory_multi(uint32_t                   in_n_buffer_memory_binding_updates,
                                               BufferMemoryBindingUpdate* in_updates_ptr)
{
    StructChainVector<VkBindBufferMemoryInfoKHR> bind_info_struct_chains;
    const Anvil::BaseDevice*                     device_ptr              (nullptr);
    uint32_t                                     n_total_physical_devices(0);
    bool                                         result                  (false);

    /* Sanity checks */
    if (in_n_buffer_memory_binding_updates == 0)
    {
        anvil_assert(in_n_buffer_memory_binding_updates != 0);

        goto end;
    }

    /* Convert input structure into VkBindBufferMemoryInfoKHR structures */
    for (uint32_t n_update = 0;
                  n_update < in_n_buffer_memory_binding_updates;
                ++n_update)
    {
        const auto&              current_update (in_updates_ptr[n_update]);
        const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(current_update.buffer_ptr->m_device_ptr) );

        if (current_update.physical_devices.size() != mgpu_device_ptr->get_n_physical_devices() )
        {
            anvil_assert(current_update.physical_devices.size() == mgpu_device_ptr->get_n_physical_devices() ||
                         current_update.physical_devices.size() == 0);

            goto end;
        }

        n_total_physical_devices += static_cast<uint32_t>(current_update.physical_devices.size() );

        if (n_update == 0)
        {
            device_ptr = current_update.buffer_ptr->m_device_ptr;
        }
        else
        {
            anvil_assert(device_ptr == current_update.buffer_ptr->m_device_ptr);
        }
    }

    {
        std::vector<uint32_t> device_indices           (n_total_physical_devices);
        uint32_t              n_current_physical_device(0);
        VkResult              result_vk;
        const auto&           entrypoints              (device_ptr->get_extension_khr_bind_memory2_entrypoints() );

        for (uint32_t n_update = 0;
                      n_update < in_n_buffer_memory_binding_updates;
                    ++n_update)
        {
            const auto& current_update = in_updates_ptr[n_update];

            for (uint32_t n_physical_device = 0;
                          n_physical_device < current_update.physical_devices.size();
                        ++n_physical_device, ++n_current_physical_device)
            {
                device_indices.at(n_current_physical_device) = current_update.physical_devices.at(n_physical_device)->get_device_group_device_index();
            }
        }

        n_current_physical_device = 0;

        for (uint32_t n_update = 0;
                      n_update < in_n_buffer_memory_binding_updates;
                    ++n_update)
        {
            VkBindBufferMemoryInfoKHR                current_bind_info;
            VkBindBufferMemoryDeviceGroupInfoKHR     current_bind_info_device_group;
            StructChainer<VkBindBufferMemoryInfoKHR> current_bind_info_chain;
            const auto&                              current_update                      = in_updates_ptr[n_update];

            current_bind_info_device_group.sType            = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO_KHR;
            current_bind_info_device_group.deviceIndexCount = static_cast<uint32_t>(current_update.physical_devices.size());
            current_bind_info_device_group.pDeviceIndices   = &device_indices.at(n_current_physical_device);

            current_bind_info.buffer        = current_update.buffer_ptr->get_buffer            ();
            current_bind_info.memory        = current_update.memory_block_ptr->get_memory      ();
            current_bind_info.memoryOffset  = current_update.memory_block_ptr->get_start_offset();
            current_bind_info.pNext         = nullptr;
            current_bind_info.sType         = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO_KHR;

            current_bind_info_chain.append_struct(current_bind_info);
            current_bind_info_chain.append_struct(current_bind_info_device_group);

            bind_info_struct_chains.append_struct_chain(current_bind_info_chain.create_chain() );

            n_current_physical_device += static_cast<uint32_t>(current_update.physical_devices.size());
        }

        result_vk = entrypoints.vkBindBufferMemory2KHR(device_ptr->get_device_vk(),
                                                       in_n_buffer_memory_binding_updates,
                                                       bind_info_struct_chains.get_root_structs() );

        if (!is_vk_call_successful(result_vk) )
        {
            anvil_assert(is_vk_call_successful(result_vk) );

            goto end;
        }

        for (uint32_t n_update = 0;
                      n_update < in_n_buffer_memory_binding_updates;
                    ++n_update)
        {
            auto& current_update = in_updates_ptr[n_update];

            anvil_assert(current_update.buffer_ptr->m_memory_block_ptr == nullptr);

            anvil_assert(std::find_if(current_update.buffer_ptr->m_owned_memory_blocks.begin(),
                                      current_update.buffer_ptr->m_owned_memory_blocks.end  (),
                                      [=](const MemoryBlockUniquePtr& in_memory_block_ptr)
                                      {
                                          return (in_memory_block_ptr.get() == current_update.memory_block_ptr);
                                      }) == current_update.buffer_ptr->m_owned_memory_blocks.end() );

            current_update.buffer_ptr->m_memory_block_ptr = current_update.memory_block_ptr;

            if (current_update.memory_block_owned_by_buffer)
            {
                MemoryBlockUniquePtr memory_block_ptr(current_update.memory_block_ptr,
                                                      std::default_delete<MemoryBlock>() );

                anvil_assert(!current_update.buffer_ptr->is_memory_block_owned(current_update.memory_block_ptr));

                current_update.buffer_ptr->m_owned_memory_blocks.push_back(
                    std::move(memory_block_ptr)
                );
            }
        }
    }

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::Buffer::write(VkDeviceSize  in_start_offset,
                          VkDeviceSize  in_size,
                          const void*   in_data,
                          Anvil::Queue* in_opt_queue_ptr)
{
    return write(in_start_offset,
                 in_size,
                 in_data,
                 UINT32_MAX, /* in_device_mask */
                 in_opt_queue_ptr);
}

/* Please see header for specification */
bool Anvil::Buffer::write(VkDeviceSize  in_start_offset,
                          VkDeviceSize  in_size,
                          const void*   in_data,
                          uint32_t      in_device_mask,
                          Anvil::Queue* in_opt_queue_ptr)
{
    const Anvil::DeviceType device_type(m_device_ptr->get_type() );
    bool                    result     (false);

    /** TODO: Support for sparse-resident buffers whose n_memory_blocks > 1 */
    Anvil::MemoryBlock* memory_block_ptr(get_memory_block(0) );

    if ( m_create_info_ptr->get_type()                                                              == Anvil::BufferType::NO_ALLOC &&
        (m_create_info_ptr->get_create_flags() & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0)
    {
        anvil_assert(m_page_tracker_ptr->get_n_memory_blocks() == 1);
    }

    anvil_assert(memory_block_ptr                                    != nullptr);
    anvil_assert(memory_block_ptr->get_create_info_ptr()->get_size() >= in_size);

    if ((memory_block_ptr->get_create_info_ptr()->get_memory_features() & Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) != 0)
    {
        anvil_assert((memory_block_ptr->get_create_info_ptr()->get_memory_features() & Anvil::MemoryFeatureFlagBits::MULTI_INSTANCE_BIT) == 0);

        result = memory_block_ptr->write(in_start_offset,
                                         in_size,
                                         in_data);
    }
    else
    {
        /* The buffer memory is not mappable. We need to create a staging memory,
         * upload user's data there, and then issue a copy op. */
        Anvil::PrimaryCommandBufferUniquePtr      copy_cmdbuf_ptr;

        if (m_staging_buffer_ptr == nullptr                                    ||
            m_staging_buffer_ptr->get_create_info_ptr()->get_size() < in_size)
        {
            if (!init_staging_buffer(in_size,
                                     in_opt_queue_ptr) )
            {
                result = false;

                goto end;
            }

            anvil_assert(m_staging_buffer_ptr != nullptr);
        }

        m_staging_buffer_ptr->write(0, /* in_start_offset */
                                    in_size,
                                    in_data);

        copy_cmdbuf_ptr = m_device_ptr->get_command_pool_for_queue_family_index(m_staging_buffer_queue_ptr->get_queue_family_index() )->alloc_primary_level_command_buffer();

        if (copy_cmdbuf_ptr == nullptr)
        {
            anvil_assert(copy_cmdbuf_ptr != nullptr);

            goto end;
        }

        if (device_type == Anvil::DeviceType::SINGLE_GPU)
        {
            copy_cmdbuf_ptr->start_recording(true,   /* one_time_submit          */
                                             false); /* simultaneous_use_allowed */
        }
        else
        {
            anvil_assert(device_type == Anvil::DeviceType::MULTI_GPU);

            copy_cmdbuf_ptr->start_recording(true,   /* one_time_submit          */
                                             false,  /* simultaneous_use_allowed */
                                             in_device_mask);
        }
        {
            BufferBarrier        buffer_barrier(Anvil::AccessFlagBits::HOST_WRITE_BIT, /* in_source_access_mask */
                                                (Anvil::AccessFlagBits::HOST_READ_BIT  | Anvil::AccessFlagBits::MEMORY_READ_BIT  | Anvil::AccessFlagBits::SHADER_READ_BIT  | Anvil::AccessFlagBits::TRANSFER_READ_BIT   |
                                                 Anvil::AccessFlagBits::HOST_WRITE_BIT | Anvil::AccessFlagBits::MEMORY_WRITE_BIT | Anvil::AccessFlagBits::SHADER_WRITE_BIT | Anvil::AccessFlagBits::TRANSFER_WRITE_BIT),
                                                VK_QUEUE_FAMILY_IGNORED,
                                                VK_QUEUE_FAMILY_IGNORED,
                                                m_staging_buffer_ptr.get(),
                                                0, /* in_offset */
                                                in_size);
            Anvil::BufferCopy    copy_region;

            copy_region.dst_offset = in_start_offset;
            copy_region.size       = in_size;
            copy_region.src_offset = 0;


            copy_cmdbuf_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::HOST_BIT,
                                                     Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                     Anvil::DependencyFlagBits::NONE,
                                                     0,               /* in_memory_barrier_count        */
                                                     nullptr,         /* in_memory_barriers_ptr         */
                                                     1,               /* in_buffer_memory_barrier_count */
                                                     &buffer_barrier,
                                                     0,               /* in_image_memory_barrier_count */
                                                     nullptr);        /* in_image_memory_barriers_ptr  */
            copy_cmdbuf_ptr->record_copy_buffer     (m_staging_buffer_ptr.get(),
                                                     this,
                                                     1, /* in_region_count */
                                                    &copy_region);
        }
        copy_cmdbuf_ptr->stop_recording();

        if (device_type == Anvil::DeviceType::SINGLE_GPU)
        {
            m_staging_buffer_queue_ptr->submit(
                Anvil::SubmitInfo::create_execute(copy_cmdbuf_ptr.get(),
                                                  true /* should_block */)
            );
        }
        else
        {
            Anvil::CommandBufferMGPUSubmission copy_cmdbuf_submission;

            copy_cmdbuf_submission.cmd_buffer_ptr = copy_cmdbuf_ptr.get();
            copy_cmdbuf_submission.device_mask    = in_device_mask;

            /* Need to update all memory instances */
            if ((memory_block_ptr->get_create_info_ptr()->get_memory_features() & Anvil::MemoryFeatureFlagBits::MULTI_INSTANCE_BIT) != 0)
            {
                auto                     device_mask     = memory_block_ptr->get_create_info_ptr()->get_device_mask();
                const Anvil::MGPUDevice* mgpu_device_ptr = dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr);

                if (device_mask == 0)
                {
                    device_mask = (1 << mgpu_device_ptr->get_n_physical_devices()) - 1;
                }

                copy_cmdbuf_submission.device_mask = device_mask;
            }

            m_staging_buffer_queue_ptr->submit(
                Anvil::SubmitInfo::create_execute(&copy_cmdbuf_submission,
                                                  1, /* in_n_command_buffer_submissions */
                                                  true /* should_block */)
            );
        }

        result = true;
    }

end:
    return result;
}

bool Anvil::Buffer::is_memory_block_owned(const MemoryBlock* in_memory_block_ptr) const
{
    for (auto memory_block_ptr_iter  = m_owned_memory_blocks.begin();
              memory_block_ptr_iter != m_owned_memory_blocks.end();
            ++memory_block_ptr_iter)
    {
        if (in_memory_block_ptr == memory_block_ptr_iter->get())
        {
            return true;
        }
    }

    return false;
}
