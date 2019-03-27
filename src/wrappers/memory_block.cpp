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
#include "misc/memory_allocator.h"
#include "misc/memory_block_create_info.h"
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
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::ANVIL_MEMORY_BLOCK,
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
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::ANVIL_MEMORY_BLOCK,
                                                   this);

    if (m_memory != VK_NULL_HANDLE)
    {
        if (on_release_callback_function == nullptr)
        {
            lock();
            {
                Anvil::Vulkan::vkFreeMemory(m_create_info_ptr->get_device()->get_device_vk(),
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
                    Anvil::Vulkan::vkUnmapMemory(m_create_info_ptr->get_device()->get_device_vk(),
                                                 m_memory);
                }
            }
            unlock();

            m_gpu_data_ptr = nullptr;
        }
    }
}

/* Please see header for specification */
Anvil::MemoryBlockUniquePtr Anvil::MemoryBlock::create(Anvil::MemoryBlockCreateInfoUniquePtr in_create_info_ptr,
                                                       VkResult*                             out_opt_result_ptr)
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
        if (type == Anvil::MemoryBlockType::REGULAR                 ||
            type == Anvil::MemoryBlockType::REGULAR_WITH_MEMORY_TYPE )
        {
            if (!result_ptr->init(out_opt_result_ptr) )
            {
                result_ptr.reset();
            }
        }
        else
        {
            result_ptr->m_memory_type_props_ptr = &result_ptr->m_create_info_ptr->get_device()->get_physical_device_memory_properties().types.at(result_ptr->m_create_info_ptr->get_memory_type_index() );

            if (type == Anvil::MemoryBlockType::DERIVED_WITH_CUSTOM_DELETE_PROC)
            {
                result_ptr->m_memory = result_ptr->m_create_info_ptr->get_memory();
            }

            if (out_opt_result_ptr != nullptr)
            {
                *out_opt_result_ptr = VkResult::VK_SUCCESS;
            }
        }
    }

    return result_ptr;
}

Anvil::ExternalHandleUniquePtr Anvil::MemoryBlock::export_to_external_memory_handle(const Anvil::ExternalMemoryHandleTypeFlagBits& in_memory_handle_type)
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

    auto                           memory                  = get_memory();
    MemoryBlock*                   parent_memory_block_ptr = nullptr;
    ExternalHandleType             result_handle           = invalid_handle;
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

    if (m_create_info_ptr->get_type() != Anvil::MemoryBlockType::REGULAR                 &&
        m_create_info_ptr->get_type() != Anvil::MemoryBlockType::REGULAR_WITH_MEMORY_TYPE )
    {
        parent_memory_block_ptr = m_create_info_ptr->get_parent_memory_block();

        while (parent_memory_block_ptr->get_create_info_ptr()->get_parent_memory_block() != nullptr)
        {
            parent_memory_block_ptr = parent_memory_block_ptr->get_create_info_ptr()->get_parent_memory_block();
        }

        anvil_assert(m_create_info_ptr->get_type        () == Anvil::MemoryBlockType::DERIVED                            &&
                     m_create_info_ptr->get_start_offset() == 0                                                          &&
                     m_create_info_ptr->get_size        () == parent_memory_block_ptr->get_create_info_ptr()->get_size() );
    }
    else
    {
        anvil_assert(m_create_info_ptr->get_parent_memory_block() == nullptr);

        parent_memory_block_ptr = this;
    }

    /* Should we try to return a handle which has already been created for this memory block? */
    if (only_one_handle_ever_permitted)
    {
        auto map_iterator = parent_memory_block_ptr->m_external_handle_type_to_external_handle.find(in_memory_handle_type);
        if (map_iterator != parent_memory_block_ptr->m_external_handle_type_to_external_handle.end() )
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

        anvil_assert(memory != VK_NULL_HANDLE);

        info.handleType = static_cast<VkExternalMemoryHandleTypeFlagBits>(in_memory_handle_type);
        info.memory     = memory;
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

        parent_memory_block_ptr->m_external_handle_type_to_external_handle[in_memory_handle_type] = std::move(result_owned_ptr);
    }
    else
    {
        result_returned_ptr = Anvil::ExternalHandle::create(result_handle,
                                                            is_autorelease_handle); /* in_close_at_destruction_time */
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
    uint32_t filtered_mem_types = 0;
    uint32_t result             = UINT32_MAX;

    if (!Anvil::MemoryAllocator::get_mem_types_supporting_mem_features(get_create_info_ptr()->get_device(),
                                                                       in_memory_type_bits,
                                                                       in_memory_features,
                                                                      &filtered_mem_types) )
    {
        goto end;
    }

    /* Simply pick the first lit bit */
    result = 0;

    while ((filtered_mem_types & (1 << 0)) == 0)
    {
        result             ++;
        filtered_mem_types >>= 1;
    }

end:
    anvil_assert(result != UINT32_MAX);
    return result;
}

/* Allocates actual memory and caches a number of properties used to spawn the memory block */
bool Anvil::MemoryBlock::init(VkResult* out_opt_result)
{
    VkResult                                   result;
    bool                                       result_bool = false;
    Anvil::StructChainer<VkMemoryAllocateInfo> struct_chainer;

    {
        VkMemoryAllocateInfo mem_alloc_info;

        mem_alloc_info.allocationSize  = m_create_info_ptr->get_size();
        mem_alloc_info.memoryTypeIndex = (m_create_info_ptr->get_imported_external_memory_handle_type() != Anvil::ExternalMemoryHandleTypeFlagBits::NONE) ? m_create_info_ptr->get_memory_type_index()
                                       : (m_create_info_ptr->get_type                                () != Anvil::MemoryBlockType::REGULAR)               ? m_create_info_ptr->get_memory_type_index()
                                                                                                                                                          : get_device_memory_type_index            (m_create_info_ptr->get_allowed_memory_bits(),
                                                                                                                                                                                                     m_create_info_ptr->get_memory_features    () );
        mem_alloc_info.pNext           = nullptr;
        mem_alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        m_memory_type_props_ptr = &m_create_info_ptr->get_device()->get_physical_device_memory_properties().types.at(mem_alloc_info.memoryTypeIndex);

        struct_chainer.append_struct(mem_alloc_info);

        /* Cache the memory type index in the create info struct. */
        m_create_info_ptr->set_memory_type_index(mem_alloc_info.memoryTypeIndex);
    }

    {
        Anvil::Buffer* dedicated_allocation_buffer_ptr = nullptr;
        Anvil::Image*  dedicated_allocation_image_ptr  = nullptr;
        bool           use_dedicated_allocation        = false;

        m_create_info_ptr->get_dedicated_allocation_properties(&use_dedicated_allocation,
                                                               &dedicated_allocation_buffer_ptr,
                                                               &dedicated_allocation_image_ptr);

        if (use_dedicated_allocation)
        {
            VkMemoryDedicatedAllocateInfoKHR alloc_info;

            alloc_info.buffer = (dedicated_allocation_buffer_ptr != nullptr) ? dedicated_allocation_buffer_ptr->get_buffer(false) : VK_NULL_HANDLE;
            alloc_info.image  = (dedicated_allocation_image_ptr  != nullptr) ? dedicated_allocation_image_ptr->get_image  (false) : VK_NULL_HANDLE;
            alloc_info.pNext  = nullptr;
            alloc_info.sType  = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;

            struct_chainer.append_struct(alloc_info);
        }
    }

    {
        const float& memory_priority = m_create_info_ptr->get_memory_priority();

        if (memory_priority != FLT_MAX)
        {
            anvil_assert(memory_priority >= 0.0f && memory_priority <= 1.0f);

            VkMemoryPriorityAllocateInfoEXT alloc_info;

            alloc_info.sType    = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
            alloc_info.pNext    = nullptr;
            alloc_info.priority = memory_priority;

            struct_chainer.append_struct(alloc_info);
        }
    }

    if (m_create_info_ptr->get_exportable_external_memory_handle_types() != Anvil::ExternalMemoryHandleTypeFlagBits::NONE)
    {
        VkExportMemoryAllocateInfoKHR export_memory_alloc_info;

        export_memory_alloc_info.handleTypes = m_create_info_ptr->get_exportable_external_memory_handle_types().get_vk();
        export_memory_alloc_info.pNext       = nullptr;
        export_memory_alloc_info.sType       = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;

        struct_chainer.append_struct(export_memory_alloc_info);

        #if defined(_WIN32)
        {
            const Anvil::ExternalNTHandleInfo* nt_handle_info_ptr = nullptr;

            if (m_create_info_ptr->get_exportable_nt_handle_info(&nt_handle_info_ptr) )
            {
                VkExportMemoryWin32HandleInfoKHR handle_info_khr;

                handle_info_khr.dwAccess    =  nt_handle_info_ptr->access;
                handle_info_khr.name        = (nt_handle_info_ptr->name.size() > 0) ? nt_handle_info_ptr->name.c_str()
                                                                                    : nullptr;
                handle_info_khr.pAttributes = nt_handle_info_ptr->attributes_ptr;
                handle_info_khr.pNext       = nullptr;
                handle_info_khr.sType       = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;

                struct_chainer.append_struct(handle_info_khr);
            }
        }
        #endif
    }

    {
        const Anvil::ExternalMemoryHandleImportInfo* handle_import_info_ptr = nullptr;

        if (m_create_info_ptr->get_external_handle_import_info(&handle_import_info_ptr) )
        {
            const auto imported_external_memory_handle_type = m_create_info_ptr->get_imported_external_memory_handle_type();

            if (imported_external_memory_handle_type == Anvil::ExternalMemoryHandleTypeFlagBits::HOST_ALLOCATION_BIT_EXT             ||
                imported_external_memory_handle_type == Anvil::ExternalMemoryHandleTypeFlagBits::HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT)
            {
                VkImportMemoryHostPointerInfoEXT handle_info_khr;

                anvil_assert(handle_import_info_ptr->host_ptr != nullptr);
                anvil_assert(handle_import_info_ptr->handle   == 0);

                handle_info_khr.handleType   = static_cast<VkExternalMemoryHandleTypeFlagBitsKHR>(imported_external_memory_handle_type);
                handle_info_khr.pHostPointer = handle_import_info_ptr->host_ptr;
                handle_info_khr.pNext        = nullptr;
                handle_info_khr.sType        = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT;

                struct_chainer.append_struct(handle_info_khr);
            }
            else
            {
                #if defined(_WIN32)
                    VkImportMemoryWin32HandleInfoKHR handle_info_khr;
                #else
                    VkImportMemoryFdInfoKHR handle_info_khr;
                #endif

                anvil_assert(handle_import_info_ptr->host_ptr == nullptr);

                #if defined(_WIN32)
                {
                    anvil_assert(handle_import_info_ptr->handle      != static_cast<Anvil::ExternalHandleType>(nullptr) ||
                                 handle_import_info_ptr->name.size() >  0);
                }
                #else
                {
                    anvil_assert(handle_import_info_ptr->handle != 0);
                }
                #endif

                handle_info_khr.handleType = static_cast<VkExternalMemoryHandleTypeFlagBitsKHR>(imported_external_memory_handle_type);
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
    }

    if (m_create_info_ptr->get_device_mask()        != 0                             &&
        m_create_info_ptr->get_device()->get_type() == Anvil::DeviceType::MULTI_GPU)
    {
        VkMemoryAllocateFlagsInfoKHR alloc_info_khr;

        alloc_info_khr.deviceMask = m_create_info_ptr->get_device_mask();

        /* NOTE: Host-mappable memory must not be multi-instance heap and must exist on only one device. */
        if (((m_create_info_ptr->get_memory_features()                          & Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) != 0) &&
             (Utils::count_set_bits                 (alloc_info_khr.deviceMask)                                               >  1) )
        {
            alloc_info_khr.flags = 0;
        }
        else
        {
            alloc_info_khr.flags = VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
        }

        alloc_info_khr.pNext      = nullptr;
        alloc_info_khr.sType      = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;

        struct_chainer.append_struct(alloc_info_khr);
    }

    {
        auto chain_ptr = struct_chainer.create_chain();

        result = Anvil::Vulkan::vkAllocateMemory(m_create_info_ptr->get_device()->get_device_vk(),
                                                 chain_ptr->get_root_struct(),
                                                 nullptr, /* pAllocator */
                                                &m_memory);
    }

    if (out_opt_result != nullptr)
    {
        *out_opt_result = result;
    }

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

        if ((memory_features & Anvil::MemoryFeatureFlagBits::HOST_COHERENT_BIT) == 0)
        {
            /* Make sure the mapped region is invalidated before letting the user read from it */
            VkMappedMemoryRange mapped_memory_range;
            const auto          non_coherent_atom_size(m_create_info_ptr->get_device()->get_physical_device_properties().core_vk1_0_properties_ptr->limits.non_coherent_atom_size);
            VkResult            result_vk             (VK_ERROR_INITIALIZATION_FAILED);

            ANVIL_REDUNDANT_VARIABLE(result_vk);

            mapped_memory_range.memory = get_memory();
            mapped_memory_range.offset = Anvil::Utils::round_down(in_start_offset,
                                                                  non_coherent_atom_size);
            mapped_memory_range.pNext  = nullptr;
            mapped_memory_range.size   = Anvil::Utils::round_up(in_size,
                                                                non_coherent_atom_size);
            mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            if (mapped_memory_range.size + mapped_memory_range.offset > m_create_info_ptr->get_size() )
            {
                mapped_memory_range.size = VK_WHOLE_SIZE;
            }

            result_vk = Anvil::Vulkan::vkInvalidateMappedMemoryRanges(m_create_info_ptr->get_device()->get_device_vk(),
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

    if ((memory_features & Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) == 0)
    {
        anvil_assert((memory_features & Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) != 0);

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
                                                                       m_create_info_ptr->get_start_offset(),
                                                                       m_create_info_ptr->get_size        (),
                                                                       static_cast<void**>(&m_gpu_data_ptr) );

                m_gpu_data_ptr = reinterpret_cast<uint8_t*>(m_gpu_data_ptr);
            }
            else
            {
                /* This block will be entered for memory blocks instantiated without a memory allocator
                 *
                 * TODO: A memory block may hold both buffer and image data. This means that the invocation below may trigger validation warnings telling
                 *       that image memory which is not in GENERAL and PREINITIALIZED layout must not be modified by the host. As long as the map request
                 *       is done specifically for a buffer, this warning is harmless.
                 *       Avoiding this warning is tricky, given the VK restriction where a memory allocation X must not be mapped more than once at a time.
                 *       Effectively, we would need to make the memory alloc backends create separate memory allocs for buffer and image objects, in order
                 *       to support cases where apps need to map >1 memory regions, coming from the same memory block, at once. It would also make the implementation
                 *       less readable. Might want to consider doing this nevertheless one day.
                 *
                 */
                result_vk = Anvil::Vulkan::vkMapMemory(m_create_info_ptr->get_device()->get_device_vk(),
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

        if ((m_create_info_ptr->get_memory_features() & Anvil::MemoryFeatureFlagBits::HOST_COHERENT_BIT) == 0)
        {
            VkMappedMemoryRange mapped_memory_range;
            const auto          mem_block_size         = m_create_info_ptr->get_size();
            const auto          non_coherent_atom_size = m_create_info_ptr->get_device()->get_physical_device_properties().core_vk1_0_properties_ptr->limits.non_coherent_atom_size;
            VkResult            result_vk              = VK_ERROR_INITIALIZATION_FAILED;

            anvil_assert            (m_start_offset == 0);
            ANVIL_REDUNDANT_VARIABLE(result_vk);

            mapped_memory_range.memory = m_memory;
            mapped_memory_range.offset = Anvil::Utils::round_down(in_start_offset,
                                                                  non_coherent_atom_size);
            mapped_memory_range.pNext  = nullptr;
            mapped_memory_range.size   = Anvil::Utils::round_up(in_size,
                                                                non_coherent_atom_size);
            mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            if (mapped_memory_range.size + mapped_memory_range.offset > mem_block_size)
            {
                mapped_memory_range.size = mem_block_size - mapped_memory_range.offset;
            }

            result_vk = Anvil::Vulkan::vkInvalidateMappedMemoryRanges(m_create_info_ptr->get_device()->get_device_vk(),
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

        if ((m_create_info_ptr->get_memory_features() & Anvil::MemoryFeatureFlagBits::HOST_COHERENT_BIT) == 0)
        {
            VkMappedMemoryRange mapped_memory_range;
            const auto          mem_block_size         = m_create_info_ptr->get_size();
            const auto          non_coherent_atom_size = m_create_info_ptr->get_device()->get_physical_device_properties().core_vk1_0_properties_ptr->limits.non_coherent_atom_size;
            auto                result_vk              = VkResult(VK_ERROR_INITIALIZATION_FAILED);

            ANVIL_REDUNDANT_VARIABLE(result_vk);

            mapped_memory_range.memory = m_memory;
            mapped_memory_range.offset = Anvil::Utils::round_down(in_start_offset,
                                                                  non_coherent_atom_size);
            mapped_memory_range.pNext  = nullptr;
            mapped_memory_range.size   = Anvil::Utils::round_up(in_size,
                                                                non_coherent_atom_size);
            mapped_memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            if (mapped_memory_range.size + mapped_memory_range.offset > mem_block_size)
            {
                mapped_memory_range.size = mem_block_size - mapped_memory_range.offset;
            }

            result_vk = Anvil::Vulkan::vkFlushMappedMemoryRanges(m_create_info_ptr->get_device()->get_device_vk(),
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
