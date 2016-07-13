//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
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
#include "wrappers/memory_block.h"
#include "wrappers/queue.h"

/* Please see header for specification */
Anvil::Buffer::Buffer(Anvil::Device*     device_ptr,
                      VkDeviceSize       size,
                      QueueFamilyBits    queue_families,
                      VkSharingMode      queue_sharing_mode,
                      VkBufferUsageFlags usage_flags)
    :m_buffer           (VK_NULL_HANDLE),
     m_buffer_size      (size),
     m_device_ptr       (device_ptr),
     m_memory_block_ptr (nullptr),
     m_parent_buffer_ptr(0),
     m_start_offset     (0),
     m_usage_flags      (static_cast<VkBufferUsageFlagBits>(usage_flags) )
{
    /* Create the buffer object */
    create_buffer(queue_families,
                  queue_sharing_mode,
                  size);

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_BUFFER,
                                                  this);
}

/* Please see header for specification */
Anvil::Buffer::Buffer(Anvil::Device*         device_ptr,
                      VkDeviceSize           size,
                      Anvil::QueueFamilyBits queue_families,
                      VkSharingMode          queue_sharing_mode,
                      VkBufferUsageFlags     usage_flags,
                      bool                   should_be_mappable,
                      bool                   should_be_coherent,
                      const void*            opt_client_data)
    :m_buffer           (VK_NULL_HANDLE),
     m_buffer_size      (size),
     m_device_ptr       (device_ptr),
     m_memory_block_ptr (nullptr),
     m_parent_buffer_ptr(0),
     m_start_offset     (0),
     m_usage_flags      (static_cast<VkBufferUsageFlagBits>(usage_flags) )
{
    /* Sanity checks */
    if (!should_be_mappable)
    {
        anvil_assert(!should_be_coherent);

        /* For host->gpu writes to work in this case, we will need the buffer to work as a target
         * for buffer->buffer copy operations.
         */
        m_usage_flags = static_cast<VkBufferUsageFlagBits>(m_usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    }

    /* Create the buffer object */
    create_buffer(queue_families,
                  queue_sharing_mode,
                  size);

    /* Create a memory object and preallocate as much space as we need */
    {
        Anvil::MemoryBlock* memory_block_ptr = nullptr;

        memory_block_ptr = new Anvil::MemoryBlock(m_device_ptr,
                                                  m_buffer_memory_reqs.memoryTypeBits,
                                                  m_buffer_memory_reqs.size,
                                                  should_be_mappable,
                                                  should_be_coherent);

        anvil_assert(memory_block_ptr != nullptr);

        set_memory(memory_block_ptr);

        if (opt_client_data != nullptr)
        {
            write(0,
                  size,
                  opt_client_data);
        }

        memory_block_ptr->release();
    }

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_BUFFER,
                                                  this);
}

/* Please see header for specification */
Anvil::Buffer::Buffer(Anvil::Buffer* parent_buffer_ptr,
                      VkDeviceSize   start_offset,
                      VkDeviceSize   size)
    :m_buffer           (VK_NULL_HANDLE),
     m_buffer_size      (size),
     m_memory_block_ptr (nullptr),
     m_parent_buffer_ptr(parent_buffer_ptr),
     m_start_offset     (start_offset),
     m_usage_flags      (static_cast<VkBufferUsageFlagBits>(0) )
{
    /* Sanity checks */
    anvil_assert(parent_buffer_ptr != nullptr);
    anvil_assert(start_offset      >= 0);
    anvil_assert(size              >  0);

    m_memory_block_ptr = new Anvil::MemoryBlock(parent_buffer_ptr->m_memory_block_ptr,
                                                start_offset,
                                                size);

    m_buffer = parent_buffer_ptr->m_buffer;
    m_parent_buffer_ptr->retain();

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_BUFFER,
                                                 this);
}


/** Releases a buffer object and a memory object associated with this Buffer instance. */
Anvil::Buffer::~Buffer()
{
    if (m_buffer            != VK_NULL_HANDLE &&
        m_parent_buffer_ptr == nullptr)
    {
        vkDestroyBuffer(m_device_ptr->get_device_vk(),
                        m_buffer,
                        nullptr /* pAllocator */);

        m_buffer = VK_NULL_HANDLE;
    }

    if (m_memory_block_ptr != nullptr)
    {
        m_memory_block_ptr->release();

        m_memory_block_ptr = nullptr;
    }

    if (m_parent_buffer_ptr != nullptr)
    {
        m_parent_buffer_ptr->release();

        m_parent_buffer_ptr = nullptr;
    }

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_BUFFER,
                                                   this);
}

/** Converts a Anvil::QueueFamilyBits bitfield value to an array of queue family indices.
 *
 *  @param queue_families                     Input value to convert from.
 *  @param out_opt_queue_family_indices_ptr   If not NULL, deref will be updated with @param *out_opt_n_queue_family_indices_ptr
 *                                            values, corresponding to queue family indices, as specified under @param queue_families.
 *  @param out_opt_n_queue_family_indices_ptr If not NULL, deref will be set to the number of items that would be or were written
 *                                            under @param out_opt_queue_family_indices_ptr.
 *
 **/
void Anvil::Buffer::convert_queue_family_bits_to_family_indices(Anvil::QueueFamilyBits queue_families,
                                                                uint32_t*              out_opt_queue_family_indices_ptr,
                                                                uint32_t*              out_opt_n_queue_family_indices_ptr) const
{
    uint32_t n_queue_family_indices = 0;

    if ((queue_families & QUEUE_FAMILY_COMPUTE_BIT) != 0)
    {
        anvil_assert(m_device_ptr->get_n_compute_queues() > 0);

        if (out_opt_queue_family_indices_ptr != nullptr)
        {
            out_opt_queue_family_indices_ptr[n_queue_family_indices] = m_device_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_COMPUTE);
        }

        ++n_queue_family_indices;
    }

    if ((queue_families & QUEUE_FAMILY_DMA_BIT) != 0)
    {
        anvil_assert(m_device_ptr->get_n_transfer_queues() > 0);

        if (out_opt_queue_family_indices_ptr != nullptr)
        {
            out_opt_queue_family_indices_ptr[n_queue_family_indices] = m_device_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_TRANSFER);
        }

        ++n_queue_family_indices;
    }

    if ((queue_families & QUEUE_FAMILY_GRAPHICS_BIT) != 0)
    {
        anvil_assert(m_device_ptr->get_n_universal_queues() > 0);

        if (out_opt_queue_family_indices_ptr != nullptr)
        {
            out_opt_queue_family_indices_ptr[n_queue_family_indices] = m_device_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL);
        }

        ++n_queue_family_indices;
    }

    if (out_opt_n_queue_family_indices_ptr != nullptr)
    {
        *out_opt_n_queue_family_indices_ptr = n_queue_family_indices;
    }
}

/* Creates a new Vulkan buffer object and caches memory requirements for the created buffer.
 *
 * @param queue_families Queue families the buffer needs to support.
 * @param sharing_mode   Sharing mode the buffer needs to support.
 * @param size           Size of the buffer.
 **/
void Anvil::Buffer::create_buffer(Anvil::QueueFamilyBits queue_families,
                                  VkSharingMode          sharing_mode,
                                  VkDeviceSize           size)
{
    VkBufferCreateInfo buffer_create_info;
    uint32_t           n_queue_family_indices;
    uint32_t           queue_family_indices[8];
    VkResult           result;

    /* Determine which queues the buffer should be available to. */
    convert_queue_family_bits_to_family_indices(queue_families,
                                                queue_family_indices,
                                               &n_queue_family_indices);

    anvil_assert(n_queue_family_indices > 0);
    anvil_assert(n_queue_family_indices < sizeof(queue_family_indices) / sizeof(queue_family_indices[0]) );

    /* Prepare the create info structure */
    buffer_create_info.flags                 = 0;
    buffer_create_info.pNext                 = nullptr;
    buffer_create_info.pQueueFamilyIndices   = queue_family_indices;
    buffer_create_info.queueFamilyIndexCount = n_queue_family_indices;
    buffer_create_info.sharingMode           = sharing_mode;
    buffer_create_info.size                  = size;
    buffer_create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.usage                 = m_usage_flags;

    /* Create the buffer object */
    result = vkCreateBuffer(m_device_ptr->get_device_vk(),
                           &buffer_create_info,
                            nullptr, /* pAllocator */
                           &m_buffer);
    anvil_assert_vk_call_succeeded(result);

    /* Cache buffer data memory requirements */
    vkGetBufferMemoryRequirements(m_device_ptr->get_device_vk(),
                                  m_buffer,
                                 &m_buffer_memory_reqs);
}

/* Please see header for specification */
Anvil::Buffer* Anvil::Buffer::get_base_buffer()
{
    Anvil::Buffer* parent_ptr = nullptr;
    Anvil::Buffer* result_ptr = this;

    if ( (parent_ptr = result_ptr->get_parent_buffer_ptr()) != nullptr)
    {
        result_ptr = parent_ptr;
    }

    return result_ptr;
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
    if (m_memory_block_ptr != nullptr)
    {
        return m_start_offset;
    }
    else
    {
        anvil_assert(false);

        return -1;
    }
}

/* Please see header for specification */
bool Anvil::Buffer::read(VkDeviceSize start_offset,
                         VkDeviceSize size,
                         void*        out_result_ptr)
{
    bool result = false;

    anvil_assert(m_memory_block_ptr != nullptr);

    if (m_memory_block_ptr->is_mappable() )
    {
        result = m_memory_block_ptr->read(start_offset,
                                          size,
                                          out_result_ptr);
    }
    else
    {
        /* The buffer memory is not mappable. We need to create a staging buffer,
         * do a non-mappable->mappable memory copy, and then read back data from the mappable buffer. */
        const uint32_t               n_transfer_queues             = m_device_ptr->get_n_transfer_queues();
        Anvil::Queue*                queue_ptr                     = (n_transfer_queues > 0) ? m_device_ptr->get_transfer_queue (0)
                                                                                             : m_device_ptr->get_universal_queue(0);
        Anvil::Buffer*               staging_buffer_ptr            = nullptr;
        const Anvil::QueueFamilyBits staging_buffer_queue_fam_bits = (n_transfer_queues > 0) ? Anvil::QUEUE_FAMILY_DMA_BIT       : Anvil::QUEUE_FAMILY_GRAPHICS_BIT;
        const Anvil::QueueFamilyType staging_buffer_queue_fam_type = (n_transfer_queues > 0) ? Anvil::QUEUE_FAMILY_TYPE_TRANSFER : Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL;

        Anvil::PrimaryCommandBuffer* copy_cmdbuf_ptr               = m_device_ptr->get_command_pool(staging_buffer_queue_fam_type)->alloc_primary_level_command_buffer();

        if (copy_cmdbuf_ptr == nullptr)
        {
            anvil_assert(copy_cmdbuf_ptr != nullptr);

            goto end;
        }

        staging_buffer_ptr = new Anvil::Buffer(m_device_ptr,
                                               size,
                                               staging_buffer_queue_fam_bits,
                                               VK_SHARING_MODE_EXCLUSIVE,
                                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                               true,  /* should_be_mappable */
                                               false, /* should_be_coherent */
                                               nullptr);

        if (staging_buffer_ptr == nullptr)
        {
            anvil_assert(staging_buffer_ptr != nullptr);

            copy_cmdbuf_ptr->release();
            goto end;
        }

        copy_cmdbuf_ptr->start_recording(true,   /* one_time_submit          */
                                         false); /* simultaneous_use_allowed */
        {
            VkBufferCopy copy_region;

            copy_region.dstOffset = 0;
            copy_region.size      = size;
            copy_region.srcOffset = start_offset;

            copy_cmdbuf_ptr->record_copy_buffer(this,
                                                staging_buffer_ptr,
                                                1, /* in_region_count */
                                               &copy_region);
        }
        copy_cmdbuf_ptr->stop_recording();

        queue_ptr->submit_command_buffer(copy_cmdbuf_ptr,
                                         true /* should_block */);

        result = staging_buffer_ptr->read(start_offset,
                                          size,
                                          out_result_ptr);
        copy_cmdbuf_ptr->release();
        staging_buffer_ptr->release();

        result = true;
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::Buffer::set_memory(Anvil::MemoryBlock* memory_block_ptr)
{
    bool     result = false;
    VkResult result_vk;

    if (memory_block_ptr == nullptr)
    {
        anvil_assert(!(memory_block_ptr == nullptr) );

        goto end;
    }

    if (m_memory_block_ptr != nullptr)
    {
        anvil_assert( (memory_block_ptr == nullptr) );

        goto end;
    }

    /* Bind the memory object to the buffer object */
    m_memory_block_ptr = memory_block_ptr;
    m_memory_block_ptr->retain();

    result_vk = vkBindBufferMemory(m_device_ptr->get_device_vk(),
                                   m_buffer,
                                   m_memory_block_ptr->get_memory(),
                                   memory_block_ptr->get_start_offset() );
    anvil_assert_vk_call_succeeded(result_vk);

    result = is_vk_call_successful(result_vk);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::Buffer::write(VkDeviceSize start_offset,
                          VkDeviceSize size,
                          const void*  data)
{
    bool result = false;

    anvil_assert(m_memory_block_ptr != nullptr);

    if (m_memory_block_ptr->is_mappable() )
    {
        result = m_memory_block_ptr->write(start_offset,
                                           size,
                                           data);
    }
    else
    {
        /* The buffer memory is not mappable. We need to create a staging memory,
         * upload user's data there, and then issue a copy op. */
        const uint32_t               n_transfer_queues             = m_device_ptr->get_n_transfer_queues();
        Anvil::Queue*                queue_ptr                     = (n_transfer_queues > 0) ? m_device_ptr->get_transfer_queue (0)
                                                                                             : m_device_ptr->get_universal_queue(0);
        Anvil::Buffer*               staging_buffer_ptr            = nullptr;
        const Anvil::QueueFamilyBits staging_buffer_queue_fam_bits = (n_transfer_queues > 0) ? Anvil::QUEUE_FAMILY_DMA_BIT       : Anvil::QUEUE_FAMILY_GRAPHICS_BIT;
        const Anvil::QueueFamilyType staging_buffer_queue_fam_type = (n_transfer_queues > 0) ? Anvil::QUEUE_FAMILY_TYPE_TRANSFER : Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL;

        Anvil::PrimaryCommandBuffer* copy_cmdbuf_ptr               = m_device_ptr->get_command_pool(staging_buffer_queue_fam_type)->alloc_primary_level_command_buffer();

        if (copy_cmdbuf_ptr == nullptr)
        {
            anvil_assert(copy_cmdbuf_ptr != nullptr);

            goto end;
        }

        staging_buffer_ptr = new Anvil::Buffer(m_device_ptr,
                                               size,
                                               staging_buffer_queue_fam_bits,
                                               VK_SHARING_MODE_EXCLUSIVE,
                                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                               true,  /* should_be_mappable */
                                               false, /* should_be_coherent */
                                               data);

        if (staging_buffer_ptr == nullptr)
        {
            anvil_assert(staging_buffer_ptr != nullptr);

            copy_cmdbuf_ptr->release();
            goto end;
        }

        copy_cmdbuf_ptr->start_recording(true,   /* one_time_submit          */
                                         false); /* simultaneous_use_allowed */
        {
            VkBufferCopy copy_region;

            copy_region.dstOffset = start_offset;
            copy_region.size      = size;
            copy_region.srcOffset = 0;

            copy_cmdbuf_ptr->record_copy_buffer(staging_buffer_ptr,
                                                this,
                                                1, /* in_region_count */
                                               &copy_region);
        }
        copy_cmdbuf_ptr->stop_recording();

        queue_ptr->submit_command_buffer(copy_cmdbuf_ptr,
                                         true /* should_block */);

        copy_cmdbuf_ptr->release();
        staging_buffer_ptr->release();

        result = true;
    }

end:
    return result;
}