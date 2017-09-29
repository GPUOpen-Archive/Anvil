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
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include "wrappers/physical_device.h"

/* Please see header for specification */
Anvil::MemoryBlock::MemoryBlock(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                                uint32_t                              in_allowed_memory_bits,
                                VkDeviceSize                          in_size,
                                Anvil::MemoryFeatureFlags             in_memory_features)
    :m_allowed_memory_bits               (in_allowed_memory_bits),
     m_device_ptr                        (in_device_ptr),
     m_destroy_memory_block_proc_user_arg(nullptr),
     m_gpu_data_ptr                      (nullptr),
     m_gpu_data_user_mapped              (false),
     m_memory                            (VK_NULL_HANDLE),
     m_memory_features                   (in_memory_features),
     m_parent_memory_block_ptr           (nullptr),
     m_pfn_destroy_memory_block_proc     (nullptr),
     m_size                              (in_size),
     m_start_offset                      (0)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_MEMORY_BLOCK,
                                                  this);
}

/* Please see header for specification */
Anvil::MemoryBlock::MemoryBlock(std::shared_ptr<MemoryBlock> in_parent_memory_block_ptr,
                                VkDeviceSize                 in_start_offset,
                                VkDeviceSize                 in_size)
{
    anvil_assert(in_parent_memory_block_ptr                 != nullptr);
    anvil_assert(in_parent_memory_block_ptr->m_gpu_data_ptr == nullptr);

    m_allowed_memory_bits                = in_parent_memory_block_ptr->m_allowed_memory_bits;
    m_destroy_memory_block_proc_user_arg = in_parent_memory_block_ptr->m_destroy_memory_block_proc_user_arg;
    m_device_ptr                         = in_parent_memory_block_ptr->m_device_ptr;
    m_gpu_data_ptr                       = nullptr;
    m_gpu_data_user_mapped               = false;
    m_memory_features                    = in_parent_memory_block_ptr->m_memory_features;
    m_memory                             = VK_NULL_HANDLE;
    m_memory_type_index                  = UINT32_MAX;
    m_parent_memory_block_ptr            = in_parent_memory_block_ptr;
    m_pfn_destroy_memory_block_proc      = in_parent_memory_block_ptr->m_pfn_destroy_memory_block_proc;
    m_size                               = in_size;
    m_start_offset                       = in_start_offset + m_parent_memory_block_ptr->m_start_offset;

    anvil_assert(m_start_offset          >= m_parent_memory_block_ptr->m_start_offset);
    anvil_assert(m_start_offset + m_size <= m_parent_memory_block_ptr->m_start_offset + m_parent_memory_block_ptr->m_size);

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_MEMORY_BLOCK,
                                                 this);
}

/* Please see header for specification */
Anvil::MemoryBlock::MemoryBlock(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                                VkDeviceMemory                        in_memory,
                                uint32_t                              in_allowed_memory_bits,
                                Anvil::MemoryFeatureFlags             in_memory_features,
                                uint32_t                              in_memory_type_index,
                                VkDeviceSize                          in_size,
                                VkDeviceSize                          in_start_offset,
                                PFNMEMORYBLOCKDESTRUCTIONCALLBACKPROC in_pfn_destroy_memory_block_proc,
                                void*                                 in_destroy_memory_block_proc_user_arg)
{
    anvil_assert(in_pfn_destroy_memory_block_proc != nullptr);

    m_allowed_memory_bits                = in_allowed_memory_bits;
    m_destroy_memory_block_proc_user_arg = in_destroy_memory_block_proc_user_arg;
    m_device_ptr                         = in_device_ptr;
    m_gpu_data_ptr                       = nullptr;
    m_gpu_data_user_mapped               = false;
    m_memory_features                    = in_memory_features;
    m_memory                             = in_memory;
    m_memory_type_index                  = in_memory_type_index;
    m_parent_memory_block_ptr            = nullptr;
    m_pfn_destroy_memory_block_proc      = in_pfn_destroy_memory_block_proc;
    m_size                               = in_size;
    m_start_offset                       = in_start_offset;

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_MEMORY_BLOCK,
                                                 this);
}

/** Releases the underlying Vulkan memory object. */
Anvil::MemoryBlock::~MemoryBlock()
{
    anvil_assert(!m_gpu_data_user_mapped);

    if (m_pfn_destroy_memory_block_proc != nullptr)
    {
        m_pfn_destroy_memory_block_proc(this,
                                        m_destroy_memory_block_proc_user_arg);
    }

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_MEMORY_BLOCK,
                                                   this);

    if (m_memory != VK_NULL_HANDLE)
    {
        if (m_pfn_destroy_memory_block_proc == nullptr)
        {
            std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

            vkFreeMemory(device_locked_ptr->get_device_vk(),
                         m_memory,
                         nullptr /* pAllocator */);
        }

        m_memory = VK_NULL_HANDLE;
    }
}

/** Finishes the memory mapping process, opened earlier with a open_gpu_memory_access() call. */
void Anvil::MemoryBlock::close_gpu_memory_access()
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkDeviceMemory                     memory           ((m_parent_memory_block_ptr != nullptr) ? m_parent_memory_block_ptr->m_memory : m_memory);

    anvil_assert(m_gpu_data_ptr != nullptr);

    vkUnmapMemory(device_locked_ptr->get_device_vk(),
                  memory);

    m_gpu_data_ptr = nullptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::MemoryBlock> Anvil::MemoryBlock::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                               uint32_t                         in_allowed_memory_bits,
                                                               VkDeviceSize                     in_size,
                                                               Anvil::MemoryFeatureFlags        in_memory_features)
{
    std::shared_ptr<Anvil::MemoryBlock> result_ptr;

    result_ptr.reset(
        new Anvil::MemoryBlock(in_device_ptr,
                               in_allowed_memory_bits,
                               in_size,
                               in_memory_features)
    );

    if (!result_ptr->init() )
    {
        result_ptr.reset();
    }

    return result_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::MemoryBlock> Anvil::MemoryBlock::create_derived(std::shared_ptr<MemoryBlock> in_parent_memory_block_ptr,
                                                                       VkDeviceSize                 in_start_offset,
                                                                       VkDeviceSize                 in_size)
{
    std::shared_ptr<Anvil::MemoryBlock> result_ptr;

    if (in_parent_memory_block_ptr == nullptr)
    {
        anvil_assert(in_parent_memory_block_ptr != nullptr);

        goto end;
    }

    result_ptr.reset(
        new Anvil::MemoryBlock(in_parent_memory_block_ptr,
                               in_start_offset,
                               in_size)
    );

end:
    return result_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::MemoryBlock> Anvil::MemoryBlock::create_derived_with_custom_delete_proc(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                                                                                               VkDeviceMemory                        in_memory,
                                                                                               uint32_t                              in_allowed_memory_bits,
                                                                                               Anvil::MemoryFeatureFlags             in_memory_features,
                                                                                               uint32_t                              in_memory_type_index,
                                                                                               VkDeviceSize                          in_size,
                                                                                               VkDeviceSize                          in_start_offset,
                                                                                               PFNMEMORYBLOCKDESTRUCTIONCALLBACKPROC in_pfn_destroy_memory_block_proc,
                                                                                               void*                                 in_destroy_memory_block_proc_user_arg)
{
    std::shared_ptr<Anvil::MemoryBlock> result_ptr;

    result_ptr.reset(
        new Anvil::MemoryBlock(in_device_ptr,
                               in_memory,
                               in_allowed_memory_bits,
                               in_memory_features,
                               in_memory_type_index,
                               in_size,
                               in_start_offset,
                               in_pfn_destroy_memory_block_proc,
                               in_destroy_memory_block_proc_user_arg)
    );

    return result_ptr;
}

/** Returns index of a memory type which meets the specified requirements
 *
 *  NOTE: @param coherent_memory_required may only be true if @param mappable_memory_required is also true.
 *        It is valid for @param coherent_memory_required to be false in such case as well.
 *
 *  @param in_memory_type_bits A bitfield specifying which memory type indices should be considered by
 *                             the function. For instance: 0th bit lit indicates memory type at index 0 is OK,
 *                             but if 1st bit is set to 0, 1st memory type is not acceptable.
 *  @param in_memory_features  Required memory features.
 *
 *  @return Result memory type index or UINT32_MAX, if none of the memory types supported by the implementation
 *          fits the specified conditions.
 **/
uint32_t Anvil::MemoryBlock::get_device_memory_type_index(uint32_t                  in_memory_type_bits,
                                                          Anvil::MemoryFeatureFlags in_memory_features)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr                  (m_device_ptr);
    const bool                         is_coherent_memory_required        ((in_memory_features & MEMORY_FEATURE_FLAG_HOST_COHERENT)    != 0);
    const bool                         is_device_local_memory_required    ((in_memory_features & MEMORY_FEATURE_FLAG_DEVICE_LOCAL)     != 0);
    const bool                         is_host_cached_memory_required     ((in_memory_features & MEMORY_FEATURE_FLAG_HOST_CACHED)      != 0);
    const bool                         is_lazily_allocated_memory_required((in_memory_features & MEMORY_FEATURE_FLAG_LAZILY_ALLOCATED) != 0);
    const bool                         is_mappable_memory_required        ((in_memory_features & MEMORY_FEATURE_FLAG_MAPPABLE)         != 0);
    const Anvil::MemoryTypes&          memory_types                       (device_locked_ptr->get_physical_device_memory_properties().types);

    if (!is_mappable_memory_required)
    {
        anvil_assert(!is_coherent_memory_required);
    }

    const std::size_t n_memory_types = memory_types.size();
    uint32_t          result         = UINT32_MAX;

    for (uint32_t n_memory_type = 0;
                  n_memory_type < n_memory_types;
                ++n_memory_type)
    {
        const Anvil::MemoryType& current_memory_type = memory_types[n_memory_type];

        if (((is_coherent_memory_required          && (current_memory_type.flags           & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))    ||
             !is_coherent_memory_required)                                                                                             &&
            ((is_device_local_memory_required      && (current_memory_type.flags           & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))     ||
             !is_device_local_memory_required)                                                                                         &&
            ((is_host_cached_memory_required       && (current_memory_type.flags           & VK_MEMORY_PROPERTY_HOST_CACHED_BIT))      ||
             !is_host_cached_memory_required)                                                                                          &&
            ((is_lazily_allocated_memory_required  && (current_memory_type.flags           & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)) ||
             !is_lazily_allocated_memory_required)                                                                                     &&
            ((is_mappable_memory_required          && (current_memory_type.flags           & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))     ||
             !is_mappable_memory_required) )
        {
            if ( (in_memory_type_bits & (1 << n_memory_type)) != 0)
            {
                result = n_memory_type;

                break;
            }
        }
    }

    anvil_assert(result != UINT32_MAX);
    return result;
}

/* Allocates actual memory and caches a number of properties used to spawn the memory block */
bool Anvil::MemoryBlock::init()
{
    VkMemoryAllocateInfo               buffer_data_alloc_info;
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr      (m_device_ptr);
    VkResult                           result;
    bool                               result_bool = false;

    buffer_data_alloc_info.allocationSize  = m_size;
    buffer_data_alloc_info.memoryTypeIndex = get_device_memory_type_index(m_allowed_memory_bits,
                                                                          m_memory_features);
    buffer_data_alloc_info.pNext           = nullptr;
    buffer_data_alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    result = vkAllocateMemory(device_locked_ptr->get_device_vk(),
                             &buffer_data_alloc_info,
                              nullptr, /* pAllocator */
                             &m_memory);

    anvil_assert_vk_call_succeeded(result);
    if (!is_vk_call_successful(result) )
    {
        goto end;
    }

    m_memory_type_index = buffer_data_alloc_info.memoryTypeIndex;

    /* All done */
    result_bool = true;
end:
    return result_bool;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::intersects(std::shared_ptr<const Anvil::MemoryBlock> in_memory_block_ptr) const
{
    bool result = (in_memory_block_ptr == shared_from_this() );

    if (result)
    {
        if (!(m_start_offset                               < in_memory_block_ptr->m_start_offset &&
              m_start_offset + m_size                      < in_memory_block_ptr->m_start_offset &&
              in_memory_block_ptr->m_start_offset          < m_start_offset                      &&
              in_memory_block_ptr->m_start_offset + m_size < m_start_offset) )
        {
            result = true;
        }
        else
        {
            result = false;
        }
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::map(VkDeviceSize in_start_offset,
                             VkDeviceSize in_size,
                             void**       out_opt_data_ptr)
{
    bool result = false;

    /* Sanity checks */
    if (m_gpu_data_user_mapped)
    {
        anvil_assert(!m_gpu_data_user_mapped);

        goto end;
    }

    result = open_gpu_memory_access(in_start_offset,
                                    in_size);

    if (!result)
    {
        goto end;
    }

    if ((m_memory_features & Anvil::MEMORY_FEATURE_FLAG_HOST_COHERENT) == 0)
    {
        /* Make sure the mapped region is invalidated before letting the user read from it */
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr  (m_device_ptr);
        VkMappedMemoryRange                mapped_memory_range;
        VkResult                           result_vk          (VK_ERROR_INITIALIZATION_FAILED);

        ANVIL_REDUNDANT_VARIABLE(result_vk);

        mapped_memory_range.memory = get_memory();
        mapped_memory_range.offset = m_start_offset + in_start_offset;
        mapped_memory_range.pNext  = nullptr;
        mapped_memory_range.size   = in_size;
        mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

        result_vk = vkInvalidateMappedMemoryRanges(device_locked_ptr->get_device_vk(),
                                                   1, /* memRangeCount */
                                                  &mapped_memory_range);
        anvil_assert_vk_call_succeeded(result_vk);
    }

    m_gpu_data_user_mapped       = true;
    m_gpu_data_user_size         = in_size;
    m_gpu_data_user_start_offset = in_start_offset;

    if (out_opt_data_ptr != nullptr)
    {
        *out_opt_data_ptr = m_gpu_data_ptr;
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
 *  @param in_start_offset Start offset of the region to be mapped. Must be smaller than the size
 *                         of the underlying buffer object, or else an assertion failure will occur.
 *  @param in_size         Size of the region to be mapped. (size + start_offset) must not be larger
 *                         than the size of the underlying buffer object, or else an assertion failure
 *                         will occur.
 *
 *  @return true if the call was successful, false otherwise.
 *  */
bool Anvil::MemoryBlock::open_gpu_memory_access(VkDeviceSize in_start_offset,
                                                VkDeviceSize in_size)
{
    std::shared_ptr<Anvil::BaseDevice>  device_locked_ptr(m_device_ptr);
    VkDeviceMemory                      memory           (VK_NULL_HANDLE);
    std::shared_ptr<Anvil::MemoryBlock> memory_block_ptr ((m_parent_memory_block_ptr != nullptr) ? m_parent_memory_block_ptr : shared_from_this() );
    bool                                result           (false);
    VkResult                            result_vk;

    /* Sanity checks */
    if ((m_memory_features & Anvil::MEMORY_FEATURE_FLAG_MAPPABLE) == 0)
    {
        anvil_assert((m_memory_features & Anvil::MEMORY_FEATURE_FLAG_MAPPABLE) != 0);

        goto end;
    }

    if (!(m_start_offset + m_size >= in_start_offset + in_size))
    {
        anvil_assert(m_start_offset + m_size >= in_start_offset + in_size);

        goto end;
    }

    /* Map the memory region into process space */
    memory = memory_block_ptr->m_memory;

    result_vk = vkMapMemory(device_locked_ptr->get_device_vk(),
                            memory,
                            in_start_offset,
                            in_size,
                            0, /* flags */
                            (void**) &m_gpu_data_ptr);

    anvil_assert_vk_call_succeeded(result_vk);
    result = is_vk_call_successful(result_vk);

end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::read(VkDeviceSize in_start_offset,
                              VkDeviceSize in_size,
                              void*        out_result_ptr)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkDeviceSize                       memcpy_offset = 0;
    bool                               result        = false;

    anvil_assert(in_size                   >  0);
    anvil_assert(in_start_offset + in_size <= m_size);
    anvil_assert(out_result_ptr           != nullptr);

    if (m_parent_memory_block_ptr != nullptr)
    {
        result = m_parent_memory_block_ptr->read(m_start_offset + in_start_offset,
                                                 in_size,
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
            const bool no_full_overlap = !(in_start_offset + in_size < m_gpu_data_user_start_offset + m_gpu_data_user_size ||
                                           in_start_offset           > m_gpu_data_user_start_offset);

            if (no_full_overlap)
            {
                anvil_assert(!no_full_overlap);

                goto end;
            }

            anvil_assert(in_start_offset >= m_gpu_data_user_start_offset);

            memcpy_offset = in_start_offset - m_gpu_data_user_start_offset;
        }
        else
        if (!open_gpu_memory_access(in_start_offset,
                                    in_size) )
        {
            goto end;
        }

        if ((m_memory_features & Anvil::MEMORY_FEATURE_FLAG_HOST_COHERENT) == 0)
        {
            VkMappedMemoryRange mapped_memory_range;
            VkResult            result_vk          (VK_ERROR_INITIALIZATION_FAILED);

            ANVIL_REDUNDANT_VARIABLE(result_vk);

            mapped_memory_range.memory = m_memory;
            mapped_memory_range.offset = in_start_offset;
            mapped_memory_range.pNext  = nullptr;
            mapped_memory_range.size   = in_size;
            mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            result_vk = vkInvalidateMappedMemoryRanges(device_locked_ptr->get_device_vk(),
                                                       1, /* memRangeCount */
                                                      &mapped_memory_range);
            anvil_assert_vk_call_succeeded(result_vk);
        }

        memcpy(out_result_ptr,
               (char*) m_gpu_data_ptr + static_cast<intptr_t>(memcpy_offset),
               static_cast<size_t>(in_size));

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
    m_gpu_data_user_start_offset = UINT64_MAX;

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::write(VkDeviceSize in_start_offset,
                               VkDeviceSize in_size,
                               const void*  in_data)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkDeviceSize                       memcpy_offset = 0;
    bool                               result        = false;

    anvil_assert(in_size                   >  0);
    anvil_assert(in_start_offset + in_size <= in_start_offset + m_size);
    anvil_assert(in_data                   != nullptr);

    if (m_parent_memory_block_ptr != nullptr)
    {
        result = m_parent_memory_block_ptr->write(m_start_offset + in_start_offset,
                                                  in_size,
                                                  in_data);
    }
    else
    {
        /* If a user's memory buffer mapping is active, make sure the requested region is completely covered
         * by the mapped region.
         * If not, just map the specified region, issue the read op, and then close the mapping.
         */
        if (m_gpu_data_user_mapped)
        {
            const bool no_full_overlap = !(in_start_offset + in_size <= m_gpu_data_user_start_offset + m_gpu_data_user_size ||
                                           in_start_offset           >  m_gpu_data_user_start_offset);

            if (no_full_overlap)
            {
                anvil_assert(!no_full_overlap);

                goto end;
            }

            anvil_assert(in_start_offset >= m_gpu_data_user_start_offset);

            memcpy_offset = in_start_offset - m_gpu_data_user_start_offset;
        }
        else
        if (!open_gpu_memory_access(in_start_offset,
                                    in_size) )
        {
            goto end;
        }

        memcpy((char*) m_gpu_data_ptr + static_cast<intptr_t>(memcpy_offset),
               in_data,
               static_cast<size_t>(in_size));

        if ((m_memory_features & Anvil::MEMORY_FEATURE_FLAG_HOST_COHERENT) == 0)
        {
            VkMappedMemoryRange mapped_memory_range;
            VkResult            result_vk          (VK_ERROR_INITIALIZATION_FAILED);

            ANVIL_REDUNDANT_VARIABLE(result_vk);

            mapped_memory_range.memory = m_memory;
            mapped_memory_range.offset = in_start_offset;
            mapped_memory_range.pNext  = nullptr;
            mapped_memory_range.size   = in_size;
            mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            result_vk = vkFlushMappedMemoryRanges(device_locked_ptr->get_device_vk(),
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