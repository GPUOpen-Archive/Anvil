//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/memory_block_create_info.h"
#include "wrappers/device.h"
#include "wrappers/memory_block.h"


Anvil::MemoryBlockCreateInfoUniquePtr Anvil::MemoryBlockCreateInfo::create_derived(MemoryBlock* in_parent_memory_block_ptr,
                                                                                   VkDeviceSize in_start_offset,
                                                                                   VkDeviceSize in_size)
{
    const uint32_t n_physical_devices = static_cast<uint32_t>(in_parent_memory_block_ptr->get_create_info_ptr()->m_physical_devices.size() );
    auto           result_ptr         = Anvil::MemoryBlockCreateInfoUniquePtr(nullptr,
                                                                              std::default_delete<Anvil::MemoryBlockCreateInfo>() );

    anvil_assert(in_start_offset + in_size <= in_parent_memory_block_ptr->get_create_info_ptr()->get_start_offset() + in_parent_memory_block_ptr->get_create_info_ptr()->get_size() );

    result_ptr.reset(
        new Anvil::MemoryBlockCreateInfo(Anvil::MemoryBlockType::DERIVED,
                                         in_parent_memory_block_ptr->get_create_info_ptr()->m_allowed_memory_bits,
                                         const_cast<Anvil::BaseDevice*>(in_parent_memory_block_ptr->get_create_info_ptr()->m_device_ptr),
                                         VK_NULL_HANDLE, /* in_memory */
                                         in_parent_memory_block_ptr->get_create_info_ptr()->m_memory_features,
                                         UINT32_MAX, /* in_memory_type_index */
                                         n_physical_devices,
                                         Anvil::OnMemoryBlockReleaseCallbackFunction(),
                                         (n_physical_devices != 0) ? &in_parent_memory_block_ptr->get_create_info_ptr()->m_physical_devices.at(0)
                                                                   : nullptr,
                                         in_parent_memory_block_ptr,
                                         in_size,
                                         in_start_offset)
    );

    return result_ptr;
}

Anvil::MemoryBlockCreateInfoUniquePtr Anvil::MemoryBlockCreateInfo::create_derived_with_custom_delete_proc(const Anvil::BaseDevice*                    in_device_ptr,
                                                                                                           VkDeviceMemory                              in_memory,
                                                                                                           uint32_t                                    in_allowed_memory_bits,
                                                                                                           Anvil::MemoryFeatureFlags                   in_memory_features,
                                                                                                           uint32_t                                    in_memory_type_index,
                                                                                                           VkDeviceSize                                in_size,
                                                                                                           VkDeviceSize                                in_start_offset,
                                                                                                           Anvil::OnMemoryBlockReleaseCallbackFunction in_on_release_callback_function)
{
    auto result_ptr = Anvil::MemoryBlockCreateInfoUniquePtr(nullptr,
                                                            std::default_delete<Anvil::MemoryBlockCreateInfo>() );

    anvil_assert(in_device_ptr->get_type() == Anvil::DEVICE_TYPE_SINGLE_GPU); /* todo: pass physical device array below accordingly */

    result_ptr.reset(
        new Anvil::MemoryBlockCreateInfo(Anvil::MemoryBlockType::DERIVED_WITH_CUSTOM_DELETE_PROC,
                                         in_allowed_memory_bits,
                                         in_device_ptr,
                                         in_memory,
                                         in_memory_features,
                                         in_memory_type_index,
                                         0, /* in_n_physical_devices */
                                         in_on_release_callback_function,
                                         nullptr, /* in_physical_device_ptr_ptr */
                                         nullptr, /* in_parent_memory_block_ptr */
                                         in_size,
                                         in_start_offset)
    );

    return result_ptr;
}

Anvil::MemoryBlockCreateInfoUniquePtr Anvil::MemoryBlockCreateInfo::create_regular(const Anvil::BaseDevice*  in_device_ptr,
                                                                                   uint32_t                  in_allowed_memory_bits,
                                                                                   VkDeviceSize              in_size,
                                                                                   Anvil::MemoryFeatureFlags in_memory_features)
{
    auto result_ptr = Anvil::MemoryBlockCreateInfoUniquePtr(nullptr,
                                                            std::default_delete<Anvil::MemoryBlockCreateInfo>() );

    anvil_assert(in_device_ptr->get_type() == Anvil::DEVICE_TYPE_SINGLE_GPU); /* todo: pass physical device array below accordingly */

    result_ptr.reset(
        new Anvil::MemoryBlockCreateInfo(Anvil::MemoryBlockType::REGULAR,
                                         in_allowed_memory_bits,
                                         in_device_ptr,
                                         VK_NULL_HANDLE, /* in_memory */
                                         in_memory_features,
                                         0, /* in_memory_type_index  */
                                         0, /* in_n_physical_devices */
                                         OnMemoryBlockReleaseCallbackFunction(),
                                         nullptr, /* in_physical_device_ptr_ptr */
                                         nullptr, /* in_parent_memory_block_ptr */
                                         in_size,
                                         0)       /* in_start_offset */
    );

    return result_ptr;
}

Anvil::MemoryBlockCreateInfo::MemoryBlockCreateInfo(const Anvil::MemoryBlockType&                      in_type,
                                                    const uint32_t&                                    in_allowed_memory_bits,
                                                    const Anvil::BaseDevice*                           in_device_ptr,
                                                    VkDeviceMemory                                     in_memory,
                                                    const Anvil::MemoryFeatureFlags&                   in_memory_features,
                                                    const uint32_t&                                    in_memory_type_index,
                                                    const uint32_t&                                    in_n_physical_devices,
                                                    const Anvil::OnMemoryBlockReleaseCallbackFunction& in_on_release_callback_function,
                                                    const Anvil::PhysicalDevice* const*                in_opt_physical_device_ptr_ptr,
                                                    Anvil::MemoryBlock*                                in_parent_memory_block_ptr,
                                                    const VkDeviceSize&                                in_size,
                                                    const VkDeviceSize&                                in_start_offset)
    :m_allowed_memory_bits                     (in_allowed_memory_bits),
     m_device_ptr                              (in_device_ptr),
     m_exportable_external_memory_handle_types (Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE),
     m_external_handle_import_info_specified   (false),
#ifdef _WIN32
     m_external_nt_handle_import_info_specified(false),
#endif
     m_imported_external_memory_handle_type    (Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE),
     m_memory                                  (in_memory),
     m_memory_features                         (in_memory_features),
     m_memory_type_index                       (in_memory_type_index),
     m_mt_safety                               (Anvil::MT_SAFETY_INHERIT_FROM_PARENT_DEVICE),
     m_on_release_callback_function            (in_on_release_callback_function),
     m_parent_memory_block_ptr                 (in_parent_memory_block_ptr),
     m_size                                    (in_size),
     m_start_offset                            (in_start_offset),
     m_type                                    (in_type)
{
    if (in_n_physical_devices != 0)
    {
        anvil_assert(in_opt_physical_device_ptr_ptr != nullptr);

        m_physical_devices.resize(in_n_physical_devices);

        for (uint32_t n_physical_device = 0;
                      n_physical_device < in_n_physical_devices;
                    ++n_physical_device)
        {
            m_physical_devices.at(n_physical_device) = in_opt_physical_device_ptr_ptr[n_physical_device];
        }
    }
}

Anvil::MemoryFeatureFlags Anvil::MemoryBlockCreateInfo::get_memory_features() const
{
    if (m_parent_memory_block_ptr != nullptr)
    {
        return m_parent_memory_block_ptr->get_create_info_ptr()->get_memory_features();
    }
    else
    {
        return m_memory_features;
    }
}

/** Returns the memory type index the memory block was allocated from */
uint32_t Anvil::MemoryBlockCreateInfo::get_memory_type_index() const
{
    if (m_parent_memory_block_ptr != nullptr)
    {
        return m_parent_memory_block_ptr->get_create_info_ptr()->get_memory_type_index();
    }
    else
    {
        return m_memory_type_index;
    }
}
