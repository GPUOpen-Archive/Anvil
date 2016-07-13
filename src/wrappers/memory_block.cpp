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
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include "wrappers/physical_device.h"

/* Please see header for specification */
Anvil::MemoryBlock::MemoryBlock(Anvil::Device* device_ptr,
                                uint32_t       allowed_memory_bits,
                                VkDeviceSize   size,
                                bool           should_be_mappable,
                                bool           should_be_coherent)
    :m_allowed_memory_bits    (allowed_memory_bits),
     m_device_ptr             (device_ptr),
     m_gpu_data_ptr           (nullptr),
     m_gpu_data_user_mapped   (false),
     m_is_coherent            (should_be_coherent),
     m_is_mappable            (should_be_mappable),
     m_memory                 (VK_NULL_HANDLE),
     m_parent_memory_block_ptr(nullptr),
     m_size                   (size),
     m_start_offset           (0)
{
    VkMemoryAllocateInfo buffer_data_alloc_info;
    VkResult             result;

    buffer_data_alloc_info.allocationSize  = size;
    buffer_data_alloc_info.memoryTypeIndex = get_device_memory_type_index(allowed_memory_bits,
                                                                          should_be_mappable,  /* mappable_memory_required */
                                                                          should_be_coherent); /* coherent_memory_required */
    buffer_data_alloc_info.pNext           = nullptr;
    buffer_data_alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    result = vkAllocateMemory(m_device_ptr->get_device_vk(),
                             &buffer_data_alloc_info,
                              nullptr, /* pAllocator */
                             &m_memory);
    anvil_assert_vk_call_succeeded(result);

    m_memory_type_index = buffer_data_alloc_info.memoryTypeIndex;

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_MEMORY_BLOCK,
                                                  this);
}

/* Please see header for specification */
Anvil::MemoryBlock::MemoryBlock(MemoryBlock* parent_memory_block_ptr,
                                VkDeviceSize start_offset,
                                VkDeviceSize size)
{
    anvil_assert(parent_memory_block_ptr                                != nullptr);
    anvil_assert(parent_memory_block_ptr->m_gpu_data_ptr                == nullptr);
    anvil_assert(parent_memory_block_ptr->m_memory                      != nullptr);
    anvil_assert(parent_memory_block_ptr->m_parent_memory_block_ptr     == nullptr);
    anvil_assert(parent_memory_block_ptr->m_start_offset + start_offset <  parent_memory_block_ptr->m_size);
    anvil_assert(size                                                   <= parent_memory_block_ptr->m_size - start_offset - parent_memory_block_ptr->m_start_offset);

    m_allowed_memory_bits     = parent_memory_block_ptr->m_allowed_memory_bits;
    m_gpu_data_ptr            = nullptr;
    m_gpu_data_user_mapped    = false;
    m_is_coherent             = parent_memory_block_ptr->m_is_coherent;
    m_is_mappable             = parent_memory_block_ptr->m_is_mappable;
    m_memory                  = VK_NULL_HANDLE;
    m_memory_type_index       = -1;
    m_parent_memory_block_ptr = parent_memory_block_ptr;
    m_size                    = size;
    m_start_offset            = parent_memory_block_ptr->m_start_offset + start_offset;

    parent_memory_block_ptr->retain();

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_MEMORY_BLOCK,
                                                 this);
}

/** Releases the underlying Vulkan memory object. */
Anvil::MemoryBlock::~MemoryBlock()
{
    anvil_assert(!m_gpu_data_user_mapped);

    if (m_memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device_ptr->get_device_vk(),
                     m_memory,
                     nullptr /* pAllocator */);

        m_memory = VK_NULL_HANDLE;
    }

    if (m_parent_memory_block_ptr != nullptr)
    {
        m_parent_memory_block_ptr->release();

        m_parent_memory_block_ptr = nullptr;
    }

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_MEMORY_BLOCK,
                                                    this);
}

/** Finishes the memory mapping process, opened earlier with a open_gpu_memory_access() call. */
void Anvil::MemoryBlock::close_gpu_memory_access()
{
    VkDeviceMemory memory = VK_NULL_HANDLE;

    anvil_assert(m_gpu_data_ptr != nullptr);

    memory = (m_parent_memory_block_ptr != nullptr) ? m_parent_memory_block_ptr->m_memory
                                                    : m_memory;

    vkUnmapMemory(m_device_ptr->get_device_vk(),
                  memory);

    m_gpu_data_ptr = nullptr;
}

/** Returns index of a memory type which meets the specified requirements
 *
 *  NOTE: @param coherent_memory_required may only be true if @param mappable_memory_required is also true.
 *        It is valid for @param coherent_memory_required to be false in such case as well.
 *
 *  @param memory_type_bits         A bitfield specifying which memory type indices should be considered by
 *                                  the function. For instance: 0th bit lit indicates memory type at index 0 is OK,
 *                                  but if 1st bit is set to 0, 1st memory type is not acceptable.
 *  @param mappable_memory_required true if the returned memory type has to be mappable (that is: vkMapBuffer() calls
 *                                  issued against a buffer object, whose memory backing is of this type, must be valid);
 *                                  false if the caller intends never to map such buffer object into process space.
 *  @param coherent_memory_required true if the returned memory type should be coherent; false otherwise.
 *
 *  @return Result memory type index or -1, if none of the memory types supported by the implementation
 *          fits the specified conditions.
 **/
uint32_t Anvil::MemoryBlock::get_device_memory_type_index(uint32_t memory_type_bits,
                                                          bool     mappable_memory_required,
                                                          bool     coherent_memory_required)
{
    if (!mappable_memory_required)
    {
        anvil_assert(!coherent_memory_required);
    }

    const Anvil::MemoryTypes& memory_types   = m_device_ptr->get_physical_device()->get_memory_properties().types;
    const std::size_t         n_memory_types = memory_types.size();
    uint32_t                  result         = -1;

    for (uint32_t n_memory_type = 0;
                  n_memory_type < n_memory_types;
                ++n_memory_type)
    {
        const Anvil::MemoryType& current_memory_type = memory_types[n_memory_type];
        bool                     is_a_match          = false;

        if (( coherent_memory_required  && current_memory_type.flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ||
             !coherent_memory_required)                                                                     &&
              mappable_memory_required  && current_memory_type.flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  ||
             !mappable_memory_required)
        {
            if ( (memory_type_bits & (1 << n_memory_type)) != 0)
            {
                result = n_memory_type;

                break;
            }
        }
    }

    anvil_assert(result != -1);
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::map(VkDeviceSize start_offset,
                             VkDeviceSize size,
                             void**       opt_out_data_ptr)
{
    bool result = false;

    /* Sanity checks */
    if (m_gpu_data_user_mapped)
    {
        anvil_assert(!m_gpu_data_user_mapped);

        goto end;
    }

    result = open_gpu_memory_access(start_offset,
                                    size);

    if (!result)
    {
        goto end;
    }

    m_gpu_data_user_mapped       = true;
    m_gpu_data_user_size         = size;
    m_gpu_data_user_start_offset = start_offset;

    if (opt_out_data_ptr != nullptr)
    {
        *opt_out_data_ptr = m_gpu_data_ptr;
    }

    result = true;
end:
    return result;
}

/** Maps the specified region of the underlying memory object into process space and stores the
 *  pointer in m_gpu_data_ptr.
 *
 *  Once the mapped region is no longer needed, a close_gpu_memory_access() call must be made to
 *  unmap the object from process space.
 *
 *  @param start_offset Start offset of the region to be mapped. Must be smaller than the size
 *                      of the underlying buffer object, or else an assertion failure will occur.
 *  @param size         Size of the region to be mapped. (size + start_offset) must not be larger
 *                      than the size of the underlying buffer object, or else an assertion failure
 *                      will occur.
 *
 *  @return true if the call was successful, false otherwise.
 *  */
bool Anvil::MemoryBlock::open_gpu_memory_access(VkDeviceSize start_offset,
                                                VkDeviceSize size)
{
    VkDeviceMemory memory     = VK_NULL_HANDLE;
    bool           result     = false;
    VkResult       result_vk;

    /* Sanity checks */
    anvil_assert(m_parent_memory_block_ptr == nullptr);

    if (!m_is_mappable)
    {
        anvil_assert(m_is_mappable);

        goto end;
    }

    if (!(m_size >= start_offset + size))
    {
        anvil_assert(m_size >= start_offset + size);

        goto end;
    }

    /* Map the memory region into process space */
    memory = (m_parent_memory_block_ptr != nullptr) ? m_parent_memory_block_ptr->m_memory
                                                    : m_memory;

    result_vk = vkMapMemory(m_device_ptr->get_device_vk(),
                            memory,
                            m_start_offset + start_offset,
                            size,
                            0, /* flags */
                            (void**) &m_gpu_data_ptr);

    anvil_assert_vk_call_succeeded(result_vk);
    result = is_vk_call_successful(result_vk);

end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::read(VkDeviceSize start_offset,
                              VkDeviceSize size,
                              void*        out_result_ptr)
{
    VkDeviceSize memcpy_offset = 0;
    bool         result       = false;

    anvil_assert(size                >  0);
    anvil_assert(start_offset + size <= m_size);
    anvil_assert(out_result_ptr      != nullptr);

    if (m_parent_memory_block_ptr != nullptr)
    {
        result = m_parent_memory_block_ptr->read(m_start_offset + start_offset,
                                                 size,
                                                 out_result_ptr);
    }
    else
    {
        /* If a user's memory buffer mapping is active, make sure the requested region is completely covered
         * by the mapped region.
         * If not, just map the specified region, issue the read op, and then close the mapping.
         */
        if (m_gpu_data_user_mapped)
        {
            const bool no_full_overlap = !(start_offset + size < m_gpu_data_user_start_offset + m_gpu_data_user_size ||
                                           start_offset        > m_gpu_data_user_start_offset);

            if (no_full_overlap)
            {
                anvil_assert(!no_full_overlap);

                goto end;
            }

            anvil_assert(start_offset >= m_gpu_data_user_start_offset);

            memcpy_offset = start_offset - m_gpu_data_user_start_offset;
        }
        else
        if (!open_gpu_memory_access(start_offset,
                                    size) )
        {
            goto end;
        }

        if (!m_is_coherent)
        {
            VkMappedMemoryRange mapped_memory_range;
            VkResult            result_vk;

            mapped_memory_range.memory = m_memory;
            mapped_memory_range.offset = start_offset;
            mapped_memory_range.pNext  = nullptr;
            mapped_memory_range.size   = size;
            mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            result_vk = vkInvalidateMappedMemoryRanges(m_device_ptr->get_device_vk(),
                                                       1, /* memRangeCount */
                                                      &mapped_memory_range);
            anvil_assert_vk_call_succeeded(result_vk);
        }

        memcpy(out_result_ptr,
               (char*) m_gpu_data_ptr + memcpy_offset,
               size);

        if (!m_gpu_data_user_mapped)
        {
            close_gpu_memory_access();
        }

        result = true;
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::unmap()
{
    bool result = false;

    /* Sanity checks */
    if (!m_gpu_data_user_mapped)
    {
        anvil_assert(m_gpu_data_user_mapped);

        goto end;
    }

    close_gpu_memory_access();

    m_gpu_data_user_mapped       = false;
    m_gpu_data_user_size         = 0;
    m_gpu_data_user_start_offset = -1;

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::write(VkDeviceSize start_offset,
                               VkDeviceSize size,
                               const void*  data)
{
    VkDeviceSize memcpy_offset = 0;
    bool         result        = false;

    anvil_assert(size                >  0);
    anvil_assert(start_offset + size <= m_size);
    anvil_assert(data                != nullptr);

    if (m_parent_memory_block_ptr != nullptr)
    {
        result = m_parent_memory_block_ptr->write(m_start_offset + start_offset,
                                                  size,
                                                  data);
    }
    else
    {
        /* If a user's memory buffer mapping is active, make sure the requested region is completely covered
         * by the mapped region.
         * If not, just map the specified region, issue the read op, and then close the mapping.
         */
        if (m_gpu_data_user_mapped)
        {
            const bool no_full_overlap = !(start_offset + size <= m_gpu_data_user_start_offset + m_gpu_data_user_size ||
                                           start_offset        >  m_gpu_data_user_start_offset);

            if (no_full_overlap)
            {
                anvil_assert(!no_full_overlap);

                goto end;
            }

            anvil_assert(start_offset >= m_gpu_data_user_start_offset);

            memcpy_offset = start_offset - m_gpu_data_user_start_offset;
        }
        else
        if (!open_gpu_memory_access(start_offset,
                                   size) )
        {
            goto end;
        }

        memcpy((char*) m_gpu_data_ptr + memcpy_offset,
               data,
               size);

        if (!m_is_coherent)
        {
            VkMappedMemoryRange mapped_memory_range;
            VkResult            result_vk;

            mapped_memory_range.memory = m_memory;
            mapped_memory_range.offset = start_offset;
            mapped_memory_range.pNext  = nullptr;
            mapped_memory_range.size   = size;
            mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            result_vk = vkFlushMappedMemoryRanges(m_device_ptr->get_device_vk(),
                                                  1, /* memRangeCount */
                                                 &mapped_memory_range);
            anvil_assert_vk_call_succeeded(result_vk);
        }

        if (!m_gpu_data_user_mapped)
        {
            close_gpu_memory_access();
        }

        result = true;
    }

end:
    return result;
}