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
#include "wrappers/buffer.h"
#include "wrappers/device.h"


Anvil::BufferCreateInfoUniquePtr Anvil::BufferCreateInfo::create_alloc(const Anvil::BaseDevice*  in_device_ptr,
                                                                       VkDeviceSize              in_size,
                                                                       Anvil::QueueFamilyFlags   in_queue_families,
                                                                       Anvil::SharingMode        in_sharing_mode,
                                                                       Anvil::BufferCreateFlags  in_create_flags,
                                                                       Anvil::BufferUsageFlags   in_usage_flags,
                                                                       Anvil::MemoryFeatureFlags in_memory_features)
{

    Anvil::BufferCreateInfoUniquePtr result_ptr(nullptr,
                                                std::default_delete<Anvil::BufferCreateInfo>() );

    if ((in_create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT) != 0)
    {
        anvil_assert((in_create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT) == 0);

        goto end;
    }

    if ((in_create_flags & Anvil::BufferCreateFlagBits::SPARSE_BINDING_BIT) != 0)
    {
        anvil_assert((in_create_flags & Anvil::BufferCreateFlagBits::SPARSE_BINDING_BIT) == 0);

        goto end;
    }

    if ((in_create_flags & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0)
    {
        anvil_assert((in_create_flags & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) == 0);

        goto end;
    }

    result_ptr.reset(
        new Anvil::BufferCreateInfo(BufferType::ALLOC,
                                    in_device_ptr,
                                    in_size,
                                    in_queue_families,
                                    in_sharing_mode,
                                    in_create_flags,
                                    in_usage_flags,
                                    in_memory_features,
                                    Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE,
                                    Anvil::ExternalMemoryHandleTypeFlagBits::NONE, /* in_external_memory_handle_types */
                                    nullptr)                                       /* in_opt_client_data_ptr          */
    );

end:
    return result_ptr;
}

Anvil::BufferCreateInfoUniquePtr Anvil::BufferCreateInfo::create_no_alloc(const Anvil::BaseDevice* in_device_ptr,
                                                                          VkDeviceSize             in_size,
                                                                          Anvil::QueueFamilyFlags  in_queue_families,
                                                                          Anvil::SharingMode       in_sharing_mode,
                                                                          Anvil::BufferCreateFlags in_create_flags,
                                                                          Anvil::BufferUsageFlags  in_usage_flags)
{
    Anvil::BufferCreateInfoUniquePtr result_ptr(nullptr,
                                                std::default_delete<Anvil::BufferCreateInfo>() );

    result_ptr.reset(
        new Anvil::BufferCreateInfo(BufferType::NO_ALLOC,
                                    in_device_ptr,
                                    in_size,
                                    in_queue_families,
                                    in_sharing_mode,
                                    in_create_flags,
                                    in_usage_flags,
                                    Anvil::MemoryFeatureFlagBits::NONE,
                                    Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE,
                                    Anvil::ExternalMemoryHandleTypeFlagBits::NONE, /* in_external_memory_handle_types */
                                    nullptr)                                       /* in_opt_client_data_ptr          */
    );

    return result_ptr;
}

Anvil::BufferCreateInfoUniquePtr Anvil::BufferCreateInfo::create_no_alloc_child(Anvil::Buffer* in_parent_nonsparse_buffer_ptr,
                                                                                VkDeviceSize   in_start_offset,
                                                                                VkDeviceSize   in_size)
{
    Anvil::BufferCreateInfoUniquePtr result_ptr(nullptr,
                                                std::default_delete<Anvil::BufferCreateInfo>() );

    result_ptr.reset(
        new Anvil::BufferCreateInfo(in_parent_nonsparse_buffer_ptr,
                                    in_start_offset,
                                    in_size)
    );

    return result_ptr;
}

Anvil::BufferCreateInfo::BufferCreateInfo(const Anvil::BaseDevice*             in_device_ptr,
                                          VkDeviceSize                         in_size,
                                          Anvil::QueueFamilyFlags              in_queue_families,
                                          Anvil::SharingMode                   in_sharing_mode,
                                          Anvil::BufferCreateFlags             in_create_flags,
                                          Anvil::BufferUsageFlags              in_usage_flags,
                                          MTSafety                             in_mt_safety,
                                          Anvil::ExternalMemoryHandleTypeFlags in_exportable_external_memory_handle_types)
    :m_client_data_ptr                        (nullptr),
     m_create_flags                           (in_create_flags),
     m_device_ptr                             (in_device_ptr),
     m_exportable_external_memory_handle_types(in_exportable_external_memory_handle_types),
     m_mt_safety                              (in_mt_safety),
     m_parent_buffer_ptr                      (nullptr),
     m_queue_families                         (in_queue_families),
     m_sharing_mode                           (in_sharing_mode),
     m_size                                   (in_size),
     m_start_offset                           (0),
     m_type                                   (BufferType::NO_ALLOC),
     m_usage_flags                            (in_usage_flags)
{
    /* Sanity checks */
    const auto& physical_device_features(in_device_ptr->get_physical_device_features() );

    if (!physical_device_features.core_vk1_0_features_ptr->sparse_binding)
    {
        anvil_assert(physical_device_features.core_vk1_0_features_ptr->sparse_binding);
    }

    if ((((m_create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   != 0)  ||
         ((m_create_flags & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0)) &&
        !physical_device_features.core_vk1_0_features_ptr->sparse_residency_buffer)
    {
        anvil_assert((((m_create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   != 0)  ||
                      ((m_create_flags & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) != 0)) &&
                      physical_device_features.core_vk1_0_features_ptr->sparse_residency_buffer)
    }

    if ( ((m_create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   != 0) &&
        !physical_device_features.core_vk1_0_features_ptr->sparse_residency_aliased)
    {
        anvil_assert(((m_create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   != 0) &&
                     physical_device_features.core_vk1_0_features_ptr->sparse_residency_aliased);
    }
}

Anvil::BufferCreateInfo::BufferCreateInfo(const Anvil::BufferType&             in_buffer_type,
                                          const Anvil::BaseDevice*             in_device_ptr,
                                          VkDeviceSize                         in_size,
                                          Anvil::QueueFamilyFlags              in_queue_families,
                                          Anvil::SharingMode                   in_sharing_mode,
                                          Anvil::BufferCreateFlags             in_create_flags,
                                          Anvil::BufferUsageFlags              in_usage_flags,
                                          MemoryFeatureFlags                   in_memory_features,
                                          MTSafety                             in_mt_safety,
                                          Anvil::ExternalMemoryHandleTypeFlags in_exportable_external_memory_handle_types,
                                          const void*                          in_opt_client_data_ptr)
    :m_client_data_ptr                        (in_opt_client_data_ptr),
     m_create_flags                           (in_create_flags),
     m_device_ptr                             (in_device_ptr),
     m_exportable_external_memory_handle_types(in_exportable_external_memory_handle_types),
     m_memory_features                        (in_memory_features),
     m_mt_safety                              (in_mt_safety),
     m_parent_buffer_ptr                      (nullptr),
     m_queue_families                         (in_queue_families),
     m_sharing_mode                           (in_sharing_mode),
     m_size                                   (in_size),
     m_start_offset                           (0),
     m_type                                   (in_buffer_type),
     m_usage_flags                            (in_usage_flags)
{
    if ((in_memory_features & Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) == 0)
    {
        anvil_assert((in_memory_features & Anvil::MemoryFeatureFlagBits::HOST_COHERENT_BIT) == 0)
    }
}

Anvil::BufferCreateInfo::BufferCreateInfo(Anvil::Buffer* in_parent_buffer_ptr,
                                          VkDeviceSize   in_start_offset,
                                          VkDeviceSize   in_size)
    :m_client_data_ptr                        (nullptr),
     m_create_flags                           (in_parent_buffer_ptr->get_create_info_ptr()->m_create_flags),
     m_device_ptr                             (in_parent_buffer_ptr->get_create_info_ptr()->m_device_ptr),
     m_exportable_external_memory_handle_types(in_parent_buffer_ptr->get_create_info_ptr()->m_exportable_external_memory_handle_types),
     m_memory_features                        (in_parent_buffer_ptr->get_create_info_ptr()->m_memory_features),
     m_mt_safety                              (in_parent_buffer_ptr->get_create_info_ptr()->m_mt_safety),
     m_parent_buffer_ptr                      (in_parent_buffer_ptr),
     m_queue_families                         (in_parent_buffer_ptr->get_create_info_ptr()->m_queue_families),
     m_sharing_mode                           (in_parent_buffer_ptr->get_create_info_ptr()->m_sharing_mode),
     m_size                                   (in_size),
     m_start_offset                           (in_start_offset),
     m_type                                   (BufferType::NO_ALLOC_CHILD),
     m_usage_flags                            (in_parent_buffer_ptr->get_create_info_ptr()->m_usage_flags)
{
    /* Sanity checks */
    anvil_assert(in_parent_buffer_ptr != nullptr);
    anvil_assert(in_size              >  0);

    anvil_assert((in_parent_buffer_ptr->get_create_info_ptr()->m_create_flags & Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT)   == 0);
    anvil_assert((in_parent_buffer_ptr->get_create_info_ptr()->m_create_flags & Anvil::BufferCreateFlagBits::SPARSE_BINDING_BIT)   == 0);
    anvil_assert((in_parent_buffer_ptr->get_create_info_ptr()->m_create_flags & Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT) == 0);
    anvil_assert( in_parent_buffer_ptr->get_memory_block    (0 /* in_n_memory_block */)                                            != nullptr);
}
