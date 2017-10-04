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
#include "wrappers/buffer.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/device.h"
#include "wrappers/instance.h"
#include "wrappers/memory_block.h"
#include "wrappers/physical_device.h"
#include "wrappers/queue.h"

/* Please see header for specification */
Anvil::Buffer::Buffer(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                      VkDeviceSize                     in_size,
                      QueueFamilyBits                  in_queue_families,
                      VkSharingMode                    in_queue_sharing_mode,
                      VkBufferUsageFlags               in_usage_flags,
                      Anvil::SparseResidencyScope      in_residency_scope)
    :CallbacksSupportProvider          (BUFFER_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider<Buffer>(in_device_ptr,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT),
     m_buffer           (VK_NULL_HANDLE),
     m_buffer_size      (in_size),
     m_create_flags     (0),
     m_device_ptr       (in_device_ptr),
     m_is_sparse        (true),
     m_parent_buffer_ptr(0),
     m_queue_families   (in_queue_families),
     m_residency_scope  (in_residency_scope),
     m_sharing_mode     (in_queue_sharing_mode),
     m_start_offset     (0)
{
    switch (in_residency_scope)
    {
        case Anvil::SPARSE_RESIDENCY_SCOPE_ALIASED:    m_create_flags = VK_BUFFER_CREATE_SPARSE_ALIASED_BIT | VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT | VK_BUFFER_CREATE_SPARSE_BINDING_BIT; break;
        case Anvil::SPARSE_RESIDENCY_SCOPE_NONALIASED: m_create_flags =                                       VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT | VK_BUFFER_CREATE_SPARSE_BINDING_BIT; break;
        case Anvil::SPARSE_RESIDENCY_SCOPE_NONE:       m_create_flags =                                                                               VK_BUFFER_CREATE_SPARSE_BINDING_BIT; break;

        default:
        {
            anvil_assert_fail();
        }
    }

    /* Assume the user may try to bind memory from non-mappable memory heap, in which case
     * we're going to need to copy data from a staging buffer to this buffer if the user
     * ever uses write(), and vice versa. */
    m_usage_flags = static_cast<VkBufferUsageFlagBits>(in_usage_flags                    |
                                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT  |
                                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
}

/* Please see header for specification */
Anvil::Buffer::Buffer(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                      VkDeviceSize                     in_size,
                      Anvil::QueueFamilyBits           in_queue_families,
                      VkSharingMode                    in_queue_sharing_mode,
                      VkBufferUsageFlags               in_usage_flags,
                      Anvil::MemoryFeatureFlags        in_memory_features)
    :CallbacksSupportProvider          (BUFFER_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider<Buffer>(in_device_ptr,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT),
     m_buffer           (VK_NULL_HANDLE),
     m_buffer_size      (in_size),
     m_create_flags     (0),
     m_device_ptr       (in_device_ptr),
     m_is_sparse        (false),
     m_parent_buffer_ptr(0),
     m_queue_families   (in_queue_families),
     m_residency_scope  (Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED),
     m_sharing_mode     (in_queue_sharing_mode),
     m_start_offset     (0),
     m_usage_flags      (static_cast<VkBufferUsageFlagBits>(in_usage_flags) )
{
    /* Sanity checks */
    if ((in_memory_features & MEMORY_FEATURE_FLAG_MAPPABLE) == 0)
    {
        anvil_assert((in_memory_features & MEMORY_FEATURE_FLAG_HOST_COHERENT) == 0)

        /* For host->gpu writes to work in this case, we will need the buffer to work as a target
         * for buffer->buffer copy operations. Same goes for the other way around.
         */
        m_usage_flags |= static_cast<VkBufferUsageFlagBits>(m_usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT 
                                                                          | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    }
}

/* Please see header for specification */
Anvil::Buffer::Buffer(std::shared_ptr<Anvil::Buffer> in_parent_buffer_ptr,
                      VkDeviceSize                   in_start_offset,
                      VkDeviceSize                   in_size)
    :CallbacksSupportProvider          (BUFFER_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider<Buffer>(in_parent_buffer_ptr->m_device_ptr,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT),
     m_buffer           (VK_NULL_HANDLE),
     m_buffer_size      (in_size),
     m_create_flags     (0),
     m_is_sparse        (false),
     m_parent_buffer_ptr(in_parent_buffer_ptr),
     m_residency_scope  (Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED),
     m_sharing_mode     (VK_SHARING_MODE_MAX_ENUM),
     m_start_offset     (in_start_offset)
{
    /* Sanity checks */
    anvil_assert(in_parent_buffer_ptr != nullptr);
    anvil_assert(in_size              >  0);

    anvil_assert(!in_parent_buffer_ptr->is_sparse() );

    m_create_flags = in_parent_buffer_ptr->m_create_flags;
    m_usage_flags  = in_parent_buffer_ptr->m_usage_flags;
}


/** Releases a buffer object and a memory object associated with this Buffer instance. */
Anvil::Buffer::~Buffer()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_BUFFER,
                                                   this);

    if (m_buffer            != VK_NULL_HANDLE &&
        m_parent_buffer_ptr == nullptr)
    {
        std::shared_ptr<BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroyBuffer(device_locked_ptr->get_device_vk(),
                        m_buffer,
                        nullptr /* pAllocator */);

        m_buffer = VK_NULL_HANDLE;
    }

    m_parent_buffer_ptr = nullptr;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Buffer> Anvil::Buffer::create_nonsparse(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                               VkDeviceSize                     in_size,
                                                               QueueFamilyBits                  in_queue_families,
                                                               VkSharingMode                    in_queue_sharing_mode,
                                                               VkBufferUsageFlags               in_usage_flags)
{
    std::shared_ptr<Anvil::Buffer> new_buffer_ptr;

    new_buffer_ptr.reset(
        new Anvil::Buffer(in_device_ptr,
                          in_size,
                          in_queue_families,
                          in_queue_sharing_mode,
                          in_usage_flags,
                          0) /* in_memory_features */
    );

    /* Initialize */
    new_buffer_ptr->create_buffer(in_queue_families,
                                  in_queue_sharing_mode,
                                  in_size);

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_BUFFER,
                                                 new_buffer_ptr.get() );

    return new_buffer_ptr;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Buffer> Anvil::Buffer::create_nonsparse(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                               VkDeviceSize                     in_size,
                                                               QueueFamilyBits                  in_queue_families,
                                                               VkSharingMode                    in_queue_sharing_mode,
                                                               VkBufferUsageFlags               in_usage_flags,
                                                               Anvil::MemoryFeatureFlags        in_memory_features,
                                                               const void*                      in_opt_client_data)
{
    std::shared_ptr<Anvil::Buffer> new_buffer_ptr;

    new_buffer_ptr.reset(
        new Anvil::Buffer(in_device_ptr,
                          in_size,
                          in_queue_families,
                          in_queue_sharing_mode,
                          in_usage_flags,
                          in_memory_features)
    );

    /* Initialize */
    new_buffer_ptr->create_buffer(in_queue_families,
                                  in_queue_sharing_mode,
                                  in_size);

    /* Create a memory object and preallocate as much space as we need */
    {
        std::shared_ptr<Anvil::MemoryBlock> memory_block_ptr;

        memory_block_ptr = Anvil::MemoryBlock::create(in_device_ptr,
                                                      new_buffer_ptr->m_buffer_memory_reqs.memoryTypeBits,
                                                      new_buffer_ptr->m_buffer_memory_reqs.size,
                                                      in_memory_features);

        new_buffer_ptr->set_nonsparse_memory(memory_block_ptr);

        if (in_opt_client_data != nullptr)
        {
            new_buffer_ptr->write(0,
                                  in_size,
                                  in_opt_client_data);
        }
    }

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_BUFFER,
                                                 new_buffer_ptr.get() );

    return new_buffer_ptr;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Buffer> Anvil::Buffer::create_nonsparse(std::shared_ptr<Anvil::Buffer> in_parent_nonsparse_buffer_ptr,
                                                               VkDeviceSize                   in_start_offset,
                                                               VkDeviceSize                   in_size)
{
    std::shared_ptr<Anvil::Buffer>      new_buffer_ptr;
    std::shared_ptr<Anvil::MemoryBlock> new_mem_block_ptr;

    if (in_parent_nonsparse_buffer_ptr->is_sparse() )
    {
        anvil_assert(!in_parent_nonsparse_buffer_ptr->is_sparse() );

        goto end;
    }

    new_buffer_ptr.reset(
        new Anvil::Buffer(in_parent_nonsparse_buffer_ptr,
                          in_start_offset,
                          in_size)
    );

    /* Initialize */

    anvil_assert(in_parent_nonsparse_buffer_ptr->get_memory_block(0 /* in_n_memory_block */) != nullptr);

    new_buffer_ptr->m_buffer           = in_parent_nonsparse_buffer_ptr->m_buffer;
    new_buffer_ptr->m_memory_block_ptr = Anvil::MemoryBlock::create_derived                (in_parent_nonsparse_buffer_ptr->get_memory_block(0 /* in_n_memory_block */),
                                                                                            in_start_offset,
                                                                                            in_size);
    new_buffer_ptr->m_queue_families   = in_parent_nonsparse_buffer_ptr->get_queue_families();
    new_buffer_ptr->m_sharing_mode     = in_parent_nonsparse_buffer_ptr->get_sharing_mode  ();
    new_buffer_ptr->m_usage_flags      = in_parent_nonsparse_buffer_ptr->get_usage         ();

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_BUFFER,
                                                 new_buffer_ptr.get() );

end:
    return new_buffer_ptr;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Buffer> Anvil::Buffer::create_sparse(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                            VkDeviceSize                     in_size,
                                                            QueueFamilyBits                  in_queue_families,
                                                            VkSharingMode                    in_queue_sharing_mode,
                                                            VkBufferUsageFlags               in_usage_flags,
                                                            Anvil::SparseResidencyScope      in_residency_scope)
{
    std::shared_ptr<Anvil::BaseDevice>     device_locked_ptr         (in_device_ptr);
    const VkPhysicalDeviceFeatures&        physical_device_features  (device_locked_ptr->get_physical_device_features() );
    std::shared_ptr<Anvil::Buffer>         result_ptr;

    /* Sanity checks */
    if (!physical_device_features.sparseBinding)
    {
        anvil_assert(physical_device_features.sparseBinding);

        goto end;
    }

    if ((in_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_ALIASED     ||
         in_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_NONALIASED) &&
        !physical_device_features.sparseResidencyBuffer)
    {
        anvil_assert((in_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_ALIASED     ||
                      in_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_NONALIASED) &&
                     physical_device_features.sparseResidencyBuffer);

        goto end;
    }

    if ( in_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_ALIASED &&
        !physical_device_features.sparseResidencyAliased)
    {
        anvil_assert(in_residency_scope == Anvil::SPARSE_RESIDENCY_SCOPE_ALIASED &&
                     physical_device_features.sparseResidencyAliased);

        goto end;
    }

    result_ptr.reset(
        new Anvil::Buffer(in_device_ptr,
                          in_size,
                          in_queue_families,
                          in_queue_sharing_mode,
                          in_usage_flags,
                          in_residency_scope)
    );

    /* Initialize */
    result_ptr->create_buffer(in_queue_families,
                              in_queue_sharing_mode,
                              in_size);

    result_ptr->m_page_tracker_ptr.reset(
        new Anvil::PageTracker(Anvil::Utils::round_up(in_size,
                                                      result_ptr->m_buffer_memory_reqs.alignment),
                               result_ptr->m_buffer_memory_reqs.alignment)
    );

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_BUFFER,
                                                 result_ptr.get() );

end:
    return result_ptr;
}

/* Creates a new Vulkan buffer object and caches memory requirements for the created buffer.
 *
 * @param in_queue_families Queue families the buffer needs to support.
 * @param in_sharing_mode   Sharing mode the buffer needs to support.
 * @param in_size           Size of the buffer.
 **/
void Anvil::Buffer::create_buffer(Anvil::QueueFamilyBits in_queue_families,
                                  VkSharingMode          in_sharing_mode,
                                  VkDeviceSize           in_size)
{
    VkBufferCreateInfo          buffer_create_info;
    std::shared_ptr<BaseDevice> device_locked_ptr(m_device_ptr);
    uint32_t                    n_queue_family_indices;
    uint32_t                    queue_family_indices[8];
    VkResult                    result(VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Determine which queues the buffer should be available to. */
    Anvil::Utils::convert_queue_family_bits_to_family_indices(m_device_ptr,
                                                              in_queue_families,
                                                              queue_family_indices,
                                                             &n_queue_family_indices);

    anvil_assert(n_queue_family_indices > 0);
    anvil_assert(n_queue_family_indices < sizeof(queue_family_indices) / sizeof(queue_family_indices[0]) );

    /* Prepare the create info structure */
    buffer_create_info.flags                 = m_create_flags;
    buffer_create_info.pNext                 = nullptr;
    buffer_create_info.pQueueFamilyIndices   = queue_family_indices;
    buffer_create_info.queueFamilyIndexCount = n_queue_family_indices;
    buffer_create_info.sharingMode           = (n_queue_family_indices == 1) ? VK_SHARING_MODE_EXCLUSIVE : in_sharing_mode;
    buffer_create_info.size                  = in_size;
    buffer_create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.usage                 = m_usage_flags;

    /* Create the buffer object */
    result = vkCreateBuffer(device_locked_ptr->get_device_vk(),
                           &buffer_create_info,
                            nullptr, /* pAllocator */
                           &m_buffer);
    anvil_assert_vk_call_succeeded(result);

    if (is_vk_call_successful(result) )
    {
        set_vk_handle(m_buffer);

        /* Cache buffer data memory requirements */
        vkGetBufferMemoryRequirements(device_locked_ptr->get_device_vk(),
                                      m_buffer,
                                     &m_buffer_memory_reqs);
    }
}

/* Please see header for specification */
std::shared_ptr<Anvil::Buffer> Anvil::Buffer::get_base_buffer()
{
    std::shared_ptr<Anvil::Buffer> parent_ptr;
    std::shared_ptr<Anvil::Buffer> result_ptr = shared_from_this();

    if ( (parent_ptr = result_ptr->get_parent_buffer_ptr()) != nullptr)
    {
        result_ptr = parent_ptr;
    }

    return result_ptr;
}

/* Please see header for specification */
VkBuffer Anvil::Buffer::get_buffer()
{
    if (!m_is_sparse)
    {
        if (m_memory_block_ptr == nullptr)
        {
            get_memory_block(0 /* in_n_memory_block */);
        }
    }

    return m_buffer;
}

/* Please see header for specification */
std::shared_ptr<Anvil::MemoryBlock> Anvil::Buffer::get_memory_block(uint32_t in_n_memory_block)
{
    bool is_callback_needed = false;

    if (m_is_sparse)
    {
        BufferCallbackIsAllocPendingQueryData callback_arg(shared_from_this() );

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
        anvil_assert(m_parent_buffer_ptr == nullptr);

        callback_safe(BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED,
                      nullptr);
    }

    if (m_is_sparse)
    {
        return m_page_tracker_ptr->get_memory_block(in_n_memory_block);
    }
    else
    {
        return m_memory_block_ptr;
    }

}

/* Please see header for specification */
VkDeviceSize Anvil::Buffer::get_size() const
{
    anvil_assert(m_buffer_size != 0);

    return m_buffer_size;
}

/* Please see header for specification */
VkDeviceSize Anvil::Buffer::get_start_offset() const
{
    return m_start_offset;
}

/* Please see header for specification */
bool Anvil::Buffer::read(VkDeviceSize in_start_offset,
                         VkDeviceSize in_size,
                         void*        out_result_ptr)
{
    std::shared_ptr<BaseDevice> device_locked_ptr(m_device_ptr);
    auto                        memory_block_ptr (get_memory_block(0 /* in_n_memory_block */) );
    bool                        result           (false);

    /* TODO: Support for sparse buffers */
    anvil_assert(!is_sparse());

    if ((memory_block_ptr->get_memory_features() & MEMORY_FEATURE_FLAG_MAPPABLE) != 0)
    {
        result = memory_block_ptr->read(in_start_offset,
                                        in_size,
                                        out_result_ptr);
    }
    else
    {
        /* The buffer memory is not mappable. We need to create a staging buffer,
         * do a non-mappable->mappable memory copy, and then read back data from the mappable buffer. */
        const uint32_t                 n_transfer_queues             = device_locked_ptr->get_n_transfer_queues();
        std::shared_ptr<Anvil::Queue>  queue_ptr                     = (n_transfer_queues > 0) ? device_locked_ptr->get_transfer_queue (0)
                                                                                               : device_locked_ptr->get_universal_queue(0);
        std::shared_ptr<Anvil::Buffer> staging_buffer_ptr;
        const Anvil::QueueFamilyBits   staging_buffer_queue_fam_bits = (n_transfer_queues > 0) ? Anvil::QUEUE_FAMILY_DMA_BIT       : Anvil::QUEUE_FAMILY_GRAPHICS_BIT;
        const Anvil::QueueFamilyType   staging_buffer_queue_fam_type = (n_transfer_queues > 0) ? Anvil::QUEUE_FAMILY_TYPE_TRANSFER : Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL;

        std::shared_ptr<Anvil::PrimaryCommandBuffer> copy_cmdbuf_ptr = device_locked_ptr->get_command_pool(staging_buffer_queue_fam_type)->alloc_primary_level_command_buffer();

        if (copy_cmdbuf_ptr == nullptr)
        {
            anvil_assert(copy_cmdbuf_ptr != nullptr);

            goto end;
        }

        staging_buffer_ptr = Anvil::Buffer::create_nonsparse(
            m_device_ptr,
            in_size,
            staging_buffer_queue_fam_bits,
            VK_SHARING_MODE_EXCLUSIVE,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            MEMORY_FEATURE_FLAG_MAPPABLE,
            nullptr);

        if (staging_buffer_ptr == nullptr)
        {
            anvil_assert(staging_buffer_ptr != nullptr);

            goto end;
        }

        copy_cmdbuf_ptr->start_recording(true,   /* one_time_submit          */
                                         false); /* simultaneous_use_allowed */
        {
            Anvil::BufferBarrier buffer_barrier(VK_ACCESS_TRANSFER_WRITE_BIT,
                                                VK_ACCESS_HOST_READ_BIT,
                                                VK_QUEUE_FAMILY_IGNORED,
                                                VK_QUEUE_FAMILY_IGNORED,
                                                staging_buffer_ptr,
                                                0, /* in_offset */
                                                staging_buffer_ptr->get_size() );
            VkBufferCopy         copy_region;

            copy_region.dstOffset = 0;
            copy_region.size      = in_size;
            copy_region.srcOffset = in_start_offset;

            copy_cmdbuf_ptr->record_copy_buffer     (shared_from_this(),
                                                     staging_buffer_ptr,
                                                     1, /* in_region_count */
                                                    &copy_region);
            copy_cmdbuf_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                     VK_PIPELINE_STAGE_HOST_BIT,
                                                     VK_FALSE,        /* in_by_region                   */
                                                     0,               /* in_memory_barrier_count        */
                                                     nullptr,         /* in_memory_barriers_ptr         */
                                                     1,               /* in_buffer_memory_barrier_count */
                                                     &buffer_barrier,
                                                     0,               /* in_image_memory_barrier_count */
                                                     nullptr);        /* in_image_memory_barriers_ptr  */
        }
        copy_cmdbuf_ptr->stop_recording();

        queue_ptr->submit_command_buffer(copy_cmdbuf_ptr,
                                         true /* should_block */);

        result = staging_buffer_ptr->read(0,
                                          in_size,
                                          out_result_ptr);
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::Buffer::set_nonsparse_memory(std::shared_ptr<Anvil::MemoryBlock> in_memory_block_ptr)
{
    std::shared_ptr<BaseDevice>          device_locked_ptr(m_device_ptr);
    bool                                 result           (false);
    VkResult                             result_vk;
    std::shared_ptr<Anvil::SGPUDevice>   sgpu_device_locked_ptr;
    std::weak_ptr<Anvil::PhysicalDevice> sgpu_physical_device_ptr;

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

    if (m_is_sparse)
    {
        anvil_assert(!m_is_sparse);

        goto end;
    }

    sgpu_device_locked_ptr   = std::shared_ptr<Anvil::SGPUDevice>(std::dynamic_pointer_cast<Anvil::SGPUDevice>(device_locked_ptr) );
    sgpu_physical_device_ptr = sgpu_device_locked_ptr->get_physical_device();

    /* Bind the memory object to the buffer object */
    result_vk = vkBindBufferMemory(device_locked_ptr->get_device_vk(),
                                   m_buffer,
                                   in_memory_block_ptr->get_memory      (),
                                   in_memory_block_ptr->get_start_offset() );

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert_vk_call_succeeded(result_vk);

        goto end;
    }

    /* All done */
    m_memory_block_ptr = in_memory_block_ptr;
    result             = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::Buffer::set_memory_sparse(std::shared_ptr<MemoryBlock> in_memory_block_ptr,
                                      VkDeviceSize                 in_memory_block_start_offset,
                                      VkDeviceSize                 in_start_offset,
                                      VkDeviceSize                 in_size)
{
    anvil_assert(m_is_sparse);

    return m_page_tracker_ptr->set_binding(in_memory_block_ptr,
                                           in_memory_block_start_offset,
                                           in_start_offset,
                                           in_size);
}

/* Please see header for specification */
bool Anvil::Buffer::write(VkDeviceSize                  in_start_offset,
                          VkDeviceSize                  in_size,
                          const void*                   in_data,
                          std::shared_ptr<Anvil::Queue> in_opt_queue_ptr)
{
    std::shared_ptr<BaseDevice> base_device_locked_ptr(m_device_ptr);
    bool                        result                (false);

    /** TODO: Support for sparse-resident buffers whose n_memory_blocks > 1 */
    std::shared_ptr<Anvil::MemoryBlock> memory_block_ptr(get_memory_block(0) );

    if (m_is_sparse)
    {
        anvil_assert(m_page_tracker_ptr->get_n_memory_blocks() == 1);
    }

    anvil_assert(memory_block_ptr             != nullptr);
    anvil_assert(memory_block_ptr->get_size() >= in_size);

    if ((memory_block_ptr->get_memory_features() & MEMORY_FEATURE_FLAG_MAPPABLE) != 0)
    {
        result = memory_block_ptr->write(in_start_offset,
                                         in_size,
                                         in_data);
    }
    else
    {
        /* The buffer memory is not mappable. We need to create a staging memory,
         * upload user's data there, and then issue a copy op. */
        std::shared_ptr<Anvil::PrimaryCommandBuffer> copy_cmdbuf_ptr;
        std::shared_ptr<Anvil::Queue>                queue_ptr;
        std::shared_ptr<Anvil::Buffer>               staging_buffer_ptr;
        Anvil::QueueFamilyBits                       staging_buffer_queue_fam_bits = 0;
        Anvil::QueueFamilyType                       staging_buffer_queue_fam_type = Anvil::QUEUE_FAMILY_TYPE_UNDEFINED;

        if (m_sharing_mode == VK_SHARING_MODE_EXCLUSIVE)
        {
            /* We need to use a user-specified queue, since we can't tell which queue fam the buffer is currently configured to be compatible with.
             *
             * This can be worked around if the buffer can only be used with a specific queue family type.
             */
            if (Anvil::Utils::is_pow2(m_queue_families) )
            {
                switch (m_queue_families)
                {
                    case Anvil::QUEUE_FAMILY_COMPUTE_BIT:  queue_ptr = base_device_locked_ptr->get_compute_queue  (0); break;
                    case Anvil::QUEUE_FAMILY_DMA_BIT:      queue_ptr = base_device_locked_ptr->get_transfer_queue (0); break;
                    case Anvil::QUEUE_FAMILY_GRAPHICS_BIT: queue_ptr = base_device_locked_ptr->get_universal_queue(0); break;

                    default:
                    {
                        anvil_assert_fail();
                    }
                }
            }
            else
            {
                anvil_assert(in_opt_queue_ptr != nullptr);

                queue_ptr = in_opt_queue_ptr;
            }

            anvil_assert(queue_ptr != nullptr);
            staging_buffer_queue_fam_type = base_device_locked_ptr->get_queue_family_type(queue_ptr->get_queue_family_index() );

            switch (staging_buffer_queue_fam_type)
            {
                case Anvil::QUEUE_FAMILY_TYPE_COMPUTE:   staging_buffer_queue_fam_bits = Anvil::QUEUE_FAMILY_COMPUTE_BIT;  break;
                case Anvil::QUEUE_FAMILY_TYPE_TRANSFER:  staging_buffer_queue_fam_bits = Anvil::QUEUE_FAMILY_DMA_BIT;      break;
                case Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL: staging_buffer_queue_fam_bits = Anvil::QUEUE_FAMILY_GRAPHICS_BIT; break;

                default:
                {
                    anvil_assert_fail();
                }
            }
        }
        else
        {
            /* We can use any queue from the list of queue fams this buffer is compatible, in order to perform the copy op. */
            if ((m_queue_families & Anvil::QUEUE_FAMILY_DMA_BIT) != 0)
            {
                queue_ptr                     = base_device_locked_ptr->get_transfer_queue(0);
                staging_buffer_queue_fam_bits = Anvil::QUEUE_FAMILY_DMA_BIT;
                staging_buffer_queue_fam_type = Anvil::QUEUE_FAMILY_TYPE_TRANSFER;
            }
            else
            if ((m_queue_families & Anvil::QUEUE_FAMILY_GRAPHICS_BIT) != 0)
            {
                queue_ptr                     = base_device_locked_ptr->get_universal_queue(0);
                staging_buffer_queue_fam_bits = Anvil::QUEUE_FAMILY_GRAPHICS_BIT;
                staging_buffer_queue_fam_type = Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL;
            }
            else
            {
                anvil_assert((m_queue_families & Anvil::QUEUE_FAMILY_COMPUTE_BIT) != 0)

                queue_ptr                     = base_device_locked_ptr->get_compute_queue(0);
                staging_buffer_queue_fam_bits = Anvil::QUEUE_FAMILY_COMPUTE_BIT;
                staging_buffer_queue_fam_type = Anvil::QUEUE_FAMILY_TYPE_COMPUTE;
            }
        }

        copy_cmdbuf_ptr = base_device_locked_ptr->get_command_pool(staging_buffer_queue_fam_type)->alloc_primary_level_command_buffer();

        if (copy_cmdbuf_ptr == nullptr)
        {
            anvil_assert(copy_cmdbuf_ptr != nullptr);

            goto end;
        }

        staging_buffer_ptr = Anvil::Buffer::create_nonsparse(
            m_device_ptr,
            in_size,
            staging_buffer_queue_fam_bits,
            VK_SHARING_MODE_EXCLUSIVE,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            Anvil::MEMORY_FEATURE_FLAG_MAPPABLE,
            in_data);

        if (staging_buffer_ptr == nullptr)
        {
            anvil_assert(staging_buffer_ptr != nullptr);

            goto end;
        }

        copy_cmdbuf_ptr->start_recording(true,   /* one_time_submit          */
                                         false); /* simultaneous_use_allowed */
        {
            VkBufferCopy copy_region;

            copy_region.dstOffset = in_start_offset;
            copy_region.size      = in_size;
            copy_region.srcOffset = 0;

            copy_cmdbuf_ptr->record_copy_buffer(staging_buffer_ptr,
                                                shared_from_this(),
                                                1, /* in_region_count */
                                               &copy_region);
        }
        copy_cmdbuf_ptr->stop_recording();

        queue_ptr->submit_command_buffer(copy_cmdbuf_ptr,
                                         true /* should_block */);

        result = true;
    }

end:
    return result;
}