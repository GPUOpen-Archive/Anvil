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

#include "misc/debug.h"
#include "misc/external_handle.h"
#include "misc/object_tracker.h"
#include "misc/struct_chainer.h"
#include "wrappers/buffer.h"
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include "wrappers/physical_device.h"

/* Please see header for specification */
Anvil::MemoryBlock::MemoryBlock(Anvil::MemoryBlockCreateInfoUniquePtr in_create_info_ptr)
    :MTSafetySupportProvider              (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                           in_create_info_ptr->get_device   () )),
     m_backend_object                     (nullptr),
     m_gpu_data_map_count                 (0),
     m_gpu_data_ptr                       (nullptr),
     m_memory                             (VK_NULL_HANDLE),
     m_memory_type_index                  (in_create_info_ptr->get_memory_type_index() ),
     m_parent_memory_allocator_backend_ptr(nullptr)
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    if (m_create_info_ptr->get_parent_memory_block() != nullptr)
    {
        m_start_offset = m_create_info_ptr->get_start_offset() + m_create_info_ptr->get_parent_memory_block()->m_start_offset;
    }
    else
    {
        m_start_offset = m_create_info_ptr->get_start_offset();
    }

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_MEMORY_BLOCK,
                                                  this);
}

/** Releases the underlying Vulkan memory object. */
Anvil::MemoryBlock::~MemoryBlock()
{
    auto on_release_callback_function = m_create_info_ptr->get_on_release_callback_function();

    #ifdef _DEBUG
    {
        auto parent_memory_block_ptr = m_create_info_ptr->get_parent_memory_block();

        if (parent_memory_block_ptr == nullptr)
        {
            anvil_assert(m_gpu_data_map_count.load() == 0);
        }
    }
    #endif

    if (on_release_callback_function != nullptr)
    {
        on_release_callback_function(this);
    }

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_MEMORY_BLOCK,
                                                   this);

    if (m_memory != VK_NULL_HANDLE)
    {
        if (on_release_callback_function == nullptr)
        {
            lock();
            {
                vkFreeMemory(m_create_info_ptr->get_device()->get_device_vk(),
                             m_memory,
                             nullptr /* pAllocator */);
            }
            unlock();
        }

        m_memory = VK_NULL_HANDLE;
    }
}

/** Finishes the memory mapping process, opened earlier with a open_gpu_memory_access() call. */
void Anvil::MemoryBlock::close_gpu_memory_access()
{
    auto parent_memory_block_ptr = m_create_info_ptr->get_parent_memory_block();

    if (parent_memory_block_ptr != nullptr)
    {
        parent_memory_block_ptr->close_gpu_memory_access();
    }
    else
    {
        anvil_assert(m_gpu_data_ptr != nullptr);

        if (m_gpu_data_map_count.fetch_sub(1) == 1)
        {
            lock();
            {
                if (m_parent_memory_allocator_backend_ptr != nullptr)
                {
                    m_parent_memory_allocator_backend_ptr->unmap(m_backend_object);
                }
                else
                {
                    /* This block will be entered for memory blocks instantiated without a memory allocator */
                    vkUnmapMemory(m_create_info_ptr->get_device()->get_device_vk(),
                                  m_memory);
                }
            }
            unlock();

            m_gpu_data_ptr = nullptr;
        }
    }
}

/* Please see header for specification */
Anvil::MemoryBlockUniquePtr Anvil::MemoryBlock::create(Anvil::MemoryBlockCreateInfoUniquePtr in_create_info_ptr)
{
    MemoryBlockUniquePtr result_ptr(nullptr,
                                    std::default_delete<MemoryBlock>() );
    const auto           type      (in_create_info_ptr->get_type() );

    result_ptr.reset(
        new Anvil::MemoryBlock(
            std::move(in_create_info_ptr)
        )
    );

    if (result_ptr != nullptr)
    {
        if (type == Anvil::MemoryBlockType::REGULAR)
        {
            if (!result_ptr->init() )
            {
                result_ptr.reset();
            }
        }
        else
        if (type == Anvil::MemoryBlockType::DERIVED_WITH_CUSTOM_DELETE_PROC)
        {
            result_ptr->m_memory = result_ptr->m_create_info_ptr->get_memory();
        }
    }

    return result_ptr;
}

Anvil::ExternalHandleUniquePtr Anvil::MemoryBlock::export_to_external_memory_handle(const Anvil::ExternalMemoryHandleTypeBit& in_memory_handle_type)
{
    #if defined(_WIN32)
        const auto invalid_handle                 = nullptr;
        const bool is_autorelease_handle          = Anvil::Utils::is_nt_handle(in_memory_handle_type);
        const bool only_one_handle_ever_permitted = Anvil::Utils::is_nt_handle(in_memory_handle_type);
    #else
        const auto invalid_handle                 = -1;
        const bool is_autorelease_handle          = true;
        const bool only_one_handle_ever_permitted = false;
    #endif

    ExternalHandleType             result_handle       = invalid_handle;
    Anvil::ExternalHandleUniquePtr result_returned_ptr;

    /* Sanity checks */
    #if defined(_WIN32)
    {
        if (!m_create_info_ptr->get_device()->get_extension_info()->khr_external_memory_win32() )
        {
            anvil_assert(m_create_info_ptr->get_device()->get_extension_info()->khr_external_memory_win32() );

            goto end;
        }
    }
    #else
    {
        if (!m_create_info_ptr->get_device()->get_extension_info()->khr_external_memory_fd() )
        {
            anvil_assert(m_create_info_ptr->get_device()->get_extension_info()->khr_external_memory_fd() );

            goto end;
        }
    }
    #endif

    if (m_create_info_ptr->get_type() != Anvil::MemoryBlockType::REGULAR)
    {
        anvil_assert(m_create_info_ptr->get_type() == Anvil::MemoryBlockType::REGULAR);

        goto end;
    }

    anvil_assert(m_create_info_ptr->get_parent_memory_block() == nullptr);

    /* Should we try to return a handle which has already been created for this memory block? */
    if (only_one_handle_ever_permitted)
    {
        auto map_iterator = m_external_handle_type_to_external_handle.find(in_memory_handle_type);
        if (map_iterator != m_external_handle_type_to_external_handle.end() )
        {
            result_returned_ptr = Anvil::ExternalHandleUniquePtr(map_iterator->second.get(),
                                                                 [](Anvil::ExternalHandle*){} );

            goto end;
        }
    }

    /* Apparently not, go and try to open the new handle then. */
    {
        #if defined(_WIN32)
            const auto                    entrypoints_ptr = &m_create_info_ptr->get_device()->get_extension_khr_external_memory_win32_entrypoints();
            VkMemoryGetWin32HandleInfoKHR info;
        #else
            const auto           entrypoints_ptr = &m_create_info_ptr->get_device()->get_extension_khr_external_memory_fd_entrypoints();
            VkMemoryGetFdInfoKHR info;
        #endif

        anvil_assert(m_memory != VK_NULL_HANDLE);

        info.handleType = static_cast<VkExternalMemoryHandleTypeFlagBits>(Anvil::Utils::convert_external_memory_handle_type_bits_to_vk_external_memory_handle_type_flags(in_memory_handle_type) );
        info.memory     = m_memory;
        info.pNext      = nullptr;

        #if defined(_WIN32)
        {
            info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        }
        #else
        {
            info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        }
        #endif

        #if defined(_WIN32)
            if (entrypoints_ptr->vkGetMemoryWin32HandleKHR(m_create_info_ptr->get_device()->get_device_vk(),
                                                          &info,
                                                          &result_handle) != VK_SUCCESS)
        #else
            if (entrypoints_ptr->vkGetMemoryFdKHR(m_create_info_ptr->get_device()->get_device_vk(),
                                                 &info,
                                                 &result_handle) != VK_SUCCESS)
        #endif
        {
            goto end;
        }
    }

    /* Cache the newly created handle if it's a NT handle  */
    if (only_one_handle_ever_permitted)
    {
        Anvil::ExternalHandleUniquePtr result_owned_ptr;

        result_owned_ptr    = Anvil::ExternalHandle::create (result_handle,
                                                             is_autorelease_handle); /* in_close_at_destruction_time */
        result_returned_ptr = Anvil::ExternalHandleUniquePtr(result_owned_ptr.get(),
                                                             [](Anvil::ExternalHandle*){} );

        m_external_handle_type_to_external_handle[in_memory_handle_type] = std::move(result_owned_ptr);
    }
    else
    {
        result_returned_ptr = Anvil::ExternalHandle::create(result_handle,
                                                            false); /* in_close_at_destruction_time */
    }

    anvil_assert(result_returned_ptr != nullptr);

end:
    return result_returned_ptr;
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
    const bool                is_coherent_memory_required        ((in_memory_features & MEMORY_FEATURE_FLAG_HOST_COHERENT)    != 0);
    const bool                is_device_local_memory_required    ((in_memory_features & MEMORY_FEATURE_FLAG_DEVICE_LOCAL)     != 0);
    const bool                is_host_cached_memory_required     ((in_memory_features & MEMORY_FEATURE_FLAG_HOST_CACHED)      != 0);
    const bool                is_lazily_allocated_memory_required((in_memory_features & MEMORY_FEATURE_FLAG_LAZILY_ALLOCATED) != 0);
    const bool                is_mappable_memory_required        ((in_memory_features & MEMORY_FEATURE_FLAG_MAPPABLE)         != 0);
    const Anvil::MemoryTypes& memory_types                       (get_create_info_ptr()->get_device()->get_physical_device_memory_properties().types);

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
    VkResult                                   result;
    bool                                       result_bool = false;
    Anvil::StructChainer<VkMemoryAllocateInfo> struct_chainer;

    {
        VkMemoryAllocateInfo buffer_data_alloc_info;

        buffer_data_alloc_info.allocationSize  = m_create_info_ptr->get_size();
        buffer_data_alloc_info.memoryTypeIndex = get_device_memory_type_index(m_create_info_ptr->get_allowed_memory_bits(),
                                                                              m_create_info_ptr->get_memory_features    () );
        buffer_data_alloc_info.pNext           = nullptr;
        buffer_data_alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        m_memory_type_index = buffer_data_alloc_info.memoryTypeIndex;

        struct_chainer.append_struct(buffer_data_alloc_info);
    }

    if (m_create_info_ptr->get_exportable_external_memory_handle_types() != Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE)
    {
        VkExportMemoryAllocateInfoKHR export_memory_alloc_info;

        export_memory_alloc_info.handleTypes = Anvil::Utils::convert_external_memory_handle_type_bits_to_vk_external_memory_handle_type_flags(m_create_info_ptr->get_exportable_external_memory_handle_types() );
        export_memory_alloc_info.pNext       = nullptr;
        export_memory_alloc_info.sType       = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;

        struct_chainer.append_struct(export_memory_alloc_info);

        {
            const Anvil::ExternalMemoryHandleImportInfo* handle_import_info_ptr = nullptr;

            if (m_create_info_ptr->get_external_handle_import_info(&handle_import_info_ptr) )
            {
                #if defined(_WIN32)
                    VkImportMemoryWin32HandleInfoKHR handle_info_khr;
                #else
                    VkImportMemoryFdInfoKHR handle_info_khr;
                #endif

                handle_info_khr.handleType = static_cast<VkExternalMemoryHandleTypeFlagBitsKHR>(Anvil::Utils::convert_external_memory_handle_type_bits_to_vk_external_memory_handle_type_flags(static_cast<Anvil::ExternalMemoryHandleTypeBits>(m_create_info_ptr->get_imported_external_memory_handle_type() )) );
                handle_info_khr.pNext      = nullptr;

                #if defined(_WIN32)
                    handle_info_khr.handle = handle_import_info_ptr->handle;
                    handle_info_khr.name   = (handle_import_info_ptr->name.size() > 0) ? handle_import_info_ptr->name.c_str()
                                                                                       : nullptr;
                    handle_info_khr.sType  = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
                #else
                    handle_info_khr.fd     = handle_import_info_ptr->handle;
                    handle_info_khr.sType  = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
                #endif

                struct_chainer.append_struct(handle_info_khr);
            }
        }

        #if defined(_WIN32)
        {
            const Anvil::ExternalNTHandleInfo* nt_handle_import_info_ptr = nullptr;

            if (m_create_info_ptr->get_external_nt_handle_import_info(&nt_handle_import_info_ptr) )
            {
                VkExportMemoryWin32HandleInfoKHR handle_info_khr;

                handle_info_khr.dwAccess    = nt_handle_import_info_ptr->access;
                handle_info_khr.name        = (nt_handle_import_info_ptr->name.size() > 0) ? nt_handle_import_info_ptr->name.c_str()
                                                                                           : nullptr;
                handle_info_khr.pAttributes = nt_handle_import_info_ptr->attributes_ptr;
                handle_info_khr.pNext       = nullptr;
                handle_info_khr.sType       = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;

                struct_chainer.append_struct(handle_info_khr);
            }
        }
        #endif
    }

    {
        auto chain_ptr = struct_chainer.create_chain();

        result = vkAllocateMemory(m_create_info_ptr->get_device()->get_device_vk(),
                                  chain_ptr->get_root_struct(),
                                  nullptr, /* pAllocator */
                                 &m_memory);
    }

    anvil_assert_vk_call_succeeded(result);
    if (!is_vk_call_successful(result) )
    {
        goto end;
    }

    /* All done */
    result_bool = true;
end:
    return result_bool;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::intersects(const Anvil::MemoryBlock* in_memory_block_ptr) const
{
    bool result = (m_memory == in_memory_block_ptr->m_memory);

    if (result)
    {
        const auto size = m_create_info_ptr->get_size();

        if (!(m_start_offset                             < in_memory_block_ptr->m_start_offset &&
              m_start_offset + size                      < in_memory_block_ptr->m_start_offset &&
              in_memory_block_ptr->m_start_offset        < m_start_offset                      &&
              in_memory_block_ptr->m_start_offset + size < m_start_offset) )
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
    const auto memory_features = m_create_info_ptr->get_memory_features();
    bool       result          = false;

    /* Sanity checks */
    if (m_create_info_ptr->get_parent_memory_block() != nullptr)
    {
        result = m_create_info_ptr->get_parent_memory_block()->map(m_start_offset + in_start_offset,
                                                                   in_size,
                                                                   out_opt_data_ptr);
    }
    else
    {
        /* Map whole memory block into process space. Necessary since deriving memory blocks may
         * need to carve out subregions. */
        if (!open_gpu_memory_access() )
        {
            goto end;
        }

        if ((memory_features & Anvil::MEMORY_FEATURE_FLAG_HOST_COHERENT) == 0)
        {
            /* Make sure the mapped region is invalidated before letting the user read from it */
            VkMappedMemoryRange mapped_memory_range;
            VkResult            result_vk          (VK_ERROR_INITIALIZATION_FAILED);

            ANVIL_REDUNDANT_VARIABLE(result_vk);

            mapped_memory_range.memory = get_memory();
            mapped_memory_range.offset = m_start_offset + in_start_offset;
            mapped_memory_range.pNext  = nullptr;
            mapped_memory_range.size   = in_size;
            mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            result_vk = vkInvalidateMappedMemoryRanges(m_create_info_ptr->get_device()->get_device_vk(),
                                                       1, /* memRangeCount */
                                                      &mapped_memory_range);
            anvil_assert_vk_call_succeeded(result_vk);
        }

        if (out_opt_data_ptr != nullptr)
        {
            *out_opt_data_ptr = static_cast<char*>(m_gpu_data_ptr) + m_start_offset + in_start_offset;
        }

        result = true;
    }
end:
    return result;
}

/** Maps the specified region of the underlying memory object into process space and stores the
 *  pointer in m_gpu_data_ptr.
 *
 *  Once the mapped region is no longer needed, a close_gpu_memory_access() call must be made to
 *  unmap the object from process space.
 *
 *  @return true if the call was successful, false otherwise.
 **/
bool Anvil::MemoryBlock::open_gpu_memory_access()
{
    const auto memory_features(m_create_info_ptr->get_memory_features() );
    bool       result         (false);
    VkResult   result_vk;

    /* Sanity checks */
    anvil_assert(m_create_info_ptr->get_parent_memory_block()  == nullptr);

    if ((memory_features & Anvil::MEMORY_FEATURE_FLAG_MAPPABLE) == 0)
    {
        anvil_assert((memory_features & Anvil::MEMORY_FEATURE_FLAG_MAPPABLE) != 0);

        goto end;
    }

    if (m_gpu_data_map_count.fetch_add(1) == 0)
    {
        /* Map the memory region into process space */
        lock();
        {
            if (m_parent_memory_allocator_backend_ptr != nullptr)
            {
                result_vk = m_parent_memory_allocator_backend_ptr->map(m_backend_object,
                                                                       0, /* in_start_offset */
                                                                       m_create_info_ptr->get_size(),
                                                                       static_cast<void**>(&m_gpu_data_ptr) );
            }
            else
            {
                /* This block will be entered for memory blocks instantiated without a memory allocator */
                result_vk = vkMapMemory(m_create_info_ptr->get_device()->get_device_vk(),
                                        m_memory,
                                        0, /* offset */
                                        m_create_info_ptr->get_size(),
                                        0, /* flags */
                                        static_cast<void**>(&m_gpu_data_ptr) );
            }
        }
        unlock();

        anvil_assert_vk_call_succeeded(result_vk);
        result = is_vk_call_successful(result_vk);
    }
    else
    {
        result = true;
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::read(VkDeviceSize in_start_offset,
                              VkDeviceSize in_size,
                              void*        out_result_ptr)
{
    bool result(false);

    anvil_assert(in_size                   >  0);
    anvil_assert(in_start_offset + in_size <= m_create_info_ptr->get_size() );
    anvil_assert(out_result_ptr            != nullptr);

    if (m_create_info_ptr->get_parent_memory_block() != nullptr)
    {
        result = m_create_info_ptr->get_parent_memory_block()->read(m_start_offset + in_start_offset,
                                                                    in_size,
                                                                    out_result_ptr);
    }
    else
    {
        if (!open_gpu_memory_access() )
        {
            anvil_assert_fail();

            goto end;
        }

        if ((m_create_info_ptr->get_memory_features() & Anvil::MEMORY_FEATURE_FLAG_HOST_COHERENT) == 0)
        {
            VkMappedMemoryRange mapped_memory_range;
            VkResult            result_vk          (VK_ERROR_INITIALIZATION_FAILED);

            anvil_assert            (m_start_offset == 0);
            ANVIL_REDUNDANT_VARIABLE(result_vk);

            mapped_memory_range.memory = m_memory;
            mapped_memory_range.offset = in_start_offset;
            mapped_memory_range.pNext  = nullptr;
            mapped_memory_range.size   = in_size;
            mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            result_vk = vkInvalidateMappedMemoryRanges(m_create_info_ptr->get_device()->get_device_vk(),
                                                       1, /* memRangeCount */
                                                      &mapped_memory_range);
            anvil_assert_vk_call_succeeded(result_vk);
        }

        memcpy(out_result_ptr,
               static_cast<char*>(m_gpu_data_ptr) + static_cast<intptr_t>(in_start_offset),
               static_cast<size_t>(in_size));

        close_gpu_memory_access();

        result = true;
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::unmap()
{
    bool result = false;

    if (m_create_info_ptr->get_parent_memory_block() != nullptr)
    {
        result = m_create_info_ptr->get_parent_memory_block()->unmap();

        goto end;
    }
    else
    {
        /* Sanity checks */
        if (m_gpu_data_ptr == nullptr)
        {
            anvil_assert(m_gpu_data_ptr != nullptr);

            goto end;
        }

        close_gpu_memory_access();

        result = true;
    }
end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryBlock::write(VkDeviceSize in_start_offset,
                               VkDeviceSize in_size,
                               const void*  in_data)
{
    bool result(false);

    anvil_assert(in_size                   >  0);
    anvil_assert(in_start_offset + in_size <= in_start_offset + m_create_info_ptr->get_size() );
    anvil_assert(in_data                   != nullptr);

    if (m_create_info_ptr->get_parent_memory_block() != nullptr)
    {
        result = m_create_info_ptr->get_parent_memory_block()->write(m_start_offset + in_start_offset,
                                                                     in_size,
                                                                     in_data);
    }
    else
    {
        if (!open_gpu_memory_access() )
        {
            goto end;
        }

        memcpy(static_cast<char*>(m_gpu_data_ptr) + static_cast<intptr_t>(in_start_offset),
               in_data,
               static_cast<size_t>(in_size));

        if ((m_create_info_ptr->get_memory_features() & Anvil::MEMORY_FEATURE_FLAG_HOST_COHERENT) == 0)
        {
            VkMappedMemoryRange mapped_memory_range;
            VkResult            result_vk          (VK_ERROR_INITIALIZATION_FAILED);

            ANVIL_REDUNDANT_VARIABLE(result_vk);

            mapped_memory_range.memory = m_memory;
            mapped_memory_range.offset = in_start_offset;
            mapped_memory_range.pNext  = nullptr;
            mapped_memory_range.size   = in_size;
            mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            result_vk = vkFlushMappedMemoryRanges(m_create_info_ptr->get_device()->get_device_vk(),
                                                  1, /* memRangeCount */
                                                 &mapped_memory_range);
            anvil_assert_vk_call_succeeded(result_vk);
        }

        close_gpu_memory_access();

        result = true;
    }

end:
    return result;
}