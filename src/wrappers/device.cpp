//
// Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/shader_module_cache.h"
#include "misc/struct_chainer.h"
#include "misc/swapchain_create_info.h"
#include "wrappers/command_pool.h"
#include "wrappers/compute_pipeline_manager.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/descriptor_set_layout_manager.h"
#include "wrappers/device.h"
#include "wrappers/graphics_pipeline_manager.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"
#include "wrappers/pipeline_cache.h"
#include "wrappers/pipeline_layout_manager.h"
#include "wrappers/queue.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/swapchain.h"

#ifdef max
#undef max
#endif

/* Please see header for specification */
Anvil::BaseDevice::BaseDevice(Anvil::DeviceCreateInfoUniquePtr in_create_info_ptr)
    :MTSafetySupportProvider(in_create_info_ptr->should_be_mt_safe() ),
     m_create_info_ptr      (std::move(in_create_info_ptr) ),
     m_device               (VK_NULL_HANDLE)
{
    m_khr_surface_extension_entrypoints = m_create_info_ptr->get_physical_device_ptrs().at(0)->get_instance()->get_extension_khr_surface_entrypoints();

    /* Register the instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::DEVICE,
                                                 this);
}

Anvil::BaseDevice::~BaseDevice()
{
    /* Unregister the instance. Tihs needs to happen before actual Vulkan object destruction, as there might
     * be observers who postpone their destruction until the device is about to go down.
     */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::DEVICE,
                                                    this);

    if (m_device != VK_NULL_HANDLE)
    {
        wait_idle();
    }

    m_command_pool_ptr_per_vk_queue_fam.clear();
    m_compute_pipeline_manager_ptr.reset     ();
    m_dummy_dsg_ptr.reset                    ();
    m_graphics_pipeline_manager_ptr.reset    ();
    m_descriptor_set_layout_manager_ptr.reset();
    m_pipeline_cache_ptr.reset               ();
    m_pipeline_layout_manager_ptr.reset      ();
    m_owned_queues.clear                     ();

    if (m_device != VK_NULL_HANDLE)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyDevice(m_device,
                                           nullptr); /* pAllocator */
        }
        unlock();

        m_device = nullptr;
    }
}

/** TODO */
void Anvil::BaseDevice::create_device(const std::vector<const char*>& in_extensions,
                                      const std::vector<const char*>& in_layers,
                                      DeviceQueueFamilyInfo*          out_queue_families_ptr)
{
    std::vector<float>                       device_queue_priorities;
    const auto&                              physical_device_ptrs             (m_create_info_ptr->get_physical_device_ptrs() );
    std::vector<VkPhysicalDevice>            physical_devices                 (physical_device_ptrs.size                  () );
    VkResult                                 result                           (VK_ERROR_INITIALIZATION_FAILED);
    Anvil::StructChainer<VkDeviceCreateInfo> struct_chainer;
    const Anvil::PhysicalDevice*             zeroth_physical_device_ptr       (physical_device_ptrs.at                       (0) );
    const auto&                              zeroth_physical_device_queue_fams(zeroth_physical_device_ptr->get_queue_families()  );

    ANVIL_REDUNDANT_VARIABLE(result);

    for (uint32_t n_physical_device = 0;
                  n_physical_device < static_cast<uint32_t>(physical_devices.size() );
                ++n_physical_device)
    {
        physical_devices.at(n_physical_device) = physical_device_ptrs.at(n_physical_device)->get_physical_device();
    }

    {
        Anvil::StructID root_create_info_struct_id = 0;

        /* Root structure */
        {
            VkDeviceCreateInfo create_info;

            create_info.enabledExtensionCount   = static_cast<uint32_t>(in_extensions.size() );
            create_info.enabledLayerCount       = static_cast<uint32_t>(in_layers.size    () );
            create_info.flags                   = 0;
            create_info.pEnabledFeatures        = nullptr; /* chained */
            create_info.pNext                   = nullptr;
            create_info.ppEnabledExtensionNames = (in_extensions.size() > 0) ? &in_extensions[0] : nullptr;
            create_info.ppEnabledLayerNames     = (in_layers.size    () > 0) ? &in_layers    [0] : nullptr;
            create_info.pQueueCreateInfos       = nullptr; /* chained later */
            create_info.queueCreateInfoCount    = 0;       /* filled later  */
            create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

            root_create_info_struct_id = struct_chainer.append_struct(create_info);
        }

        /* Queue create info items */
        {
            std::vector<VkDeviceQueueCreateInfo> device_queue_create_info_items;
            const auto                           n_queue_fams                   = static_cast<uint32_t>(zeroth_physical_device_queue_fams.size() );

            /* For any queue that uses non-medium global priority, we're going to need a dedicated device queue create info struct. This is
             * to ensure the global priority does not propagate to other queues.
             *
             * Creation requests for all other queues can be merged into one struct, one per each queue family index.
             */
            static VkDeviceQueueGlobalPriorityCreateInfoEXT low_priority_create_info =
            {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_EXT,
                nullptr, /* pNext */
                VK_QUEUE_GLOBAL_PRIORITY_LOW_EXT
            };
            static VkDeviceQueueGlobalPriorityCreateInfoEXT high_priority_create_info =
            {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_EXT,
                nullptr, /* pNext */
                VK_QUEUE_GLOBAL_PRIORITY_HIGH_EXT
            };
            static VkDeviceQueueGlobalPriorityCreateInfoEXT realtime_priority_create_info =
            {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_EXT,
                nullptr, /* pNext */
                VK_QUEUE_GLOBAL_PRIORITY_REALTIME_EXT
            };

            /* Prepare a vec of queue priorities we're going to stick into DeviceQueueCreateInfo structure */
            {
                uint32_t n_queues_used_so_far = 0;
                uint32_t n_total_queues       = 0;

                for (uint32_t n_queue_fam = 0;
                              n_queue_fam < n_queue_fams;
                            ++n_queue_fam)
                {
                    const auto& n_queues = zeroth_physical_device_queue_fams.at(n_queue_fam).n_queues;

                    n_total_queues += n_queues;
                }

                device_queue_priorities.resize(n_total_queues,
                                               0.0f);

                for (uint32_t n_queue_fam = 0;
                              n_queue_fam < n_queue_fams;
                            ++n_queue_fam)
                {
                    const auto& n_queues = zeroth_physical_device_queue_fams.at(n_queue_fam).n_queues;

                    for (uint32_t n_queue = 0;
                                  n_queue < n_queues;
                                ++n_queue, ++n_queues_used_so_far)
                    {
                        device_queue_priorities.at(n_queues_used_so_far) = m_create_info_ptr->get_queue_priority(n_queue_fam,
                                                                                                                 n_queue);
                    }
                }
            }

            /* Prepare device queue create info structs */
            {
                uint32_t n_queues_defined_so_far = 0;

                for (uint32_t n_queue_fam = 0;
                              n_queue_fam < n_queue_fams;
                            ++n_queue_fam)
                {
                    const auto& current_queue_fam(zeroth_physical_device_queue_fams.at(n_queue_fam) );

                    if (current_queue_fam.n_queues > 0)
                    {
                        int32_t n_queue_consumed_last = -1;

                        for (uint32_t n_queue = 0;
                                      n_queue < current_queue_fam.n_queues;
                                    ++n_queue)
                        {
                            const auto& current_queue_global_priority                   = m_create_info_ptr->get_queue_global_priority                         (n_queue_fam,
                                                                                                                                                                n_queue);
                            const bool  current_queue_must_support_protected_memory_ops = m_create_info_ptr->get_queue_must_support_protected_memory_operations(n_queue_fam,
                                                                                                                                                                n_queue);

                            if (current_queue_must_support_protected_memory_ops                                                          ||
                                current_queue_global_priority                   != Anvil::QueueGlobalPriority::MEDIUM_EXT                ||
                                n_queue                                         == static_cast<uint32_t>(current_queue_fam.n_queues - 1) )
                            {
                                VkDeviceQueueCreateInfo queue_create_info;

                                queue_create_info.flags            = 0;
                                queue_create_info.pNext            = nullptr;
                                queue_create_info.pQueuePriorities = &device_queue_priorities.at(n_queues_defined_so_far);
                                queue_create_info.queueCount       = (n_queue - n_queue_consumed_last);
                                queue_create_info.queueFamilyIndex = n_queue_fam;
                                queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

                                if (current_queue_must_support_protected_memory_ops)
                                {
                                    queue_create_info.flags |= VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT;
                                }

                                if (current_queue_global_priority == Anvil::QueueGlobalPriority::HIGH_EXT     ||
                                    current_queue_global_priority == Anvil::QueueGlobalPriority::LOW_EXT      ||
                                    current_queue_global_priority == Anvil::QueueGlobalPriority::REALTIME_EXT)
                                {
                                    queue_create_info.pNext = (current_queue_global_priority == Anvil::QueueGlobalPriority::HIGH_EXT) ? &high_priority_create_info
                                                            : (current_queue_global_priority == Anvil::QueueGlobalPriority::LOW_EXT)  ? &low_priority_create_info
                                                                                                                                      : &realtime_priority_create_info;
                                }
                                else
                                {
                                    anvil_assert(current_queue_global_priority == Anvil::QueueGlobalPriority::MEDIUM_EXT);
                                }

                                device_queue_create_info_items.push_back(queue_create_info);

                                n_queue_consumed_last    = static_cast<int32_t>(n_queue);
                                n_queues_defined_so_far += (n_queue - n_queue_consumed_last);
                            }
                        }

                    }
                }
            }

            struct_chainer.get_root_struct()->queueCreateInfoCount = static_cast<uint32_t>(device_queue_create_info_items.size() );

            struct_chainer.store_helper_structure_vector(device_queue_create_info_items,
                                                         root_create_info_struct_id,
                                                         offsetof(VkDeviceCreateInfo, pQueueCreateInfos) );
        }
    }

    {
        const auto& mem_overallocation_behavior = m_create_info_ptr->get_memory_overallocation_behavior();

        if (mem_overallocation_behavior != Anvil::MemoryOverallocationBehavior::DEFAULT)
        {
            VkDeviceMemoryOverallocationCreateInfoAMD overallocation_info;

            overallocation_info.overallocationBehavior = static_cast<VkMemoryOverallocationBehaviorAMD>(mem_overallocation_behavior);
            overallocation_info.pNext                  = nullptr;
            overallocation_info.sType                  = VK_STRUCTURE_TYPE_DEVICE_MEMORY_OVERALLOCATION_CREATE_INFO_AMD;

            anvil_assert(is_extension_enabled(VK_AMD_MEMORY_OVERALLOCATION_BEHAVIOR_EXTENSION_NAME) );

            struct_chainer.append_struct(overallocation_info);
        }
    }

    if (physical_device_ptrs.size() > 1)
    {
        VkDeviceGroupDeviceCreateInfoKHR device_group_device_create_info;

        device_group_device_create_info.physicalDeviceCount = static_cast<uint32_t>(physical_device_ptrs.size() );
        device_group_device_create_info.pPhysicalDevices    = &physical_devices[0];
        device_group_device_create_info.pNext               = nullptr;
        device_group_device_create_info.sType               = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO_KHR;

        struct_chainer.append_struct(device_group_device_create_info);
    }

    /* Enable all features available. */
    add_physical_device_features_to_chainer(&struct_chainer);

    /* Issue the request */
    {
        auto struct_chain_ptr = struct_chainer.create_chain();

        result = Anvil::Vulkan::vkCreateDevice(physical_device_ptrs.at(0)->get_physical_device(),
                                               struct_chain_ptr->get_root_struct(),
                                               nullptr, /* pAllocator */
                                              &m_device);

        anvil_assert_vk_call_succeeded(result);
    }

    /* Now that all queues are available, assign them to queue families Anvil recognizes. */
    get_queue_family_indices_for_physical_device(physical_device_ptrs.at(0),
                                                 out_queue_families_ptr);
}

/** Please see header for specification */
const Anvil::DescriptorSet* Anvil::BaseDevice::get_dummy_descriptor_set() const
{
    std::unique_lock<std::mutex> lock(m_dummy_dsg_mutex);

    if (m_dummy_dsg_ptr == nullptr)
    {
        init_dummy_dsg();
    }

    return m_dummy_dsg_ptr->get_descriptor_set(0);
}

/** Please see header for specification */
Anvil::DescriptorSetLayout* Anvil::BaseDevice::get_dummy_descriptor_set_layout() const
{
    std::unique_lock<std::mutex> lock(m_dummy_dsg_mutex);

    if (m_dummy_dsg_ptr == nullptr)
    {
        init_dummy_dsg();
    }

    return m_dummy_dsg_ptr->get_descriptor_set_layout(0);
}

const Anvil::ExtensionEXTSampleLocationsEntrypoints& Anvil::BaseDevice::get_extension_ext_sample_locations_entrypoints() const
{
    anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->ext_sample_locations() );

    return m_ext_sample_locations_extension_entrypoints;
}

const Anvil::ExtensionEXTTransformFeedbackEntrypoints& Anvil::BaseDevice::get_extension_ext_transform_feedback_entrypoints() const
{
    anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->ext_transform_feedback() );

    return m_ext_transform_feedback_extension_entrypoints;
}

bool Anvil::BaseDevice::get_memory_types_supported_for_external_handle(const Anvil::ExternalMemoryHandleTypeFlagBits& in_external_handle_type,
                                                                       ExternalHandleType                             in_handle,
                                                                       uint32_t*                                      out_supported_memory_type_bits) const
{
    bool result = false;

    if (in_external_handle_type != Anvil::ExternalMemoryHandleTypeFlagBits::HOST_ALLOCATION_BIT_EXT             &&
        in_external_handle_type != Anvil::ExternalMemoryHandleTypeFlagBits::HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT)
    {
        /* Sanity checks */
        #if defined(_WIN32)
        {
            if (!m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_memory_win32() )
            {
                anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_memory_win32() );

                goto end;
            }

            if (in_external_handle_type != Anvil::ExternalMemoryHandleTypeFlagBits::OPAQUE_WIN32_BIT     &&
                in_external_handle_type != Anvil::ExternalMemoryHandleTypeFlagBits::OPAQUE_WIN32_KMT_BIT)
            {
                anvil_assert(in_external_handle_type == Anvil::ExternalMemoryHandleTypeFlagBits::OPAQUE_WIN32_BIT        ||
                             in_external_handle_type == Anvil::ExternalMemoryHandleTypeFlagBits::OPAQUE_WIN32_KMT_BIT);

                goto end;
            }
        }
        #else
        {
            if (!m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_memory_fd() )
            {
                anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_memory_fd() );

                goto end;
            }

            if (in_external_handle_type != Anvil::ExternalMemoryHandleTypeFlagBits::OPAQUE_FD_BIT)
            {
                anvil_assert(in_external_handle_type == Anvil::ExternalMemoryHandleTypeFlagBits::OPAQUE_FD_BIT);

                goto end;
            }
        }
        #endif

        /* Go ahead with the query */
        #if defined(_WIN32)
            VkMemoryWin32HandlePropertiesKHR result_props;

            result_props.pNext = nullptr;
            result_props.sType = VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR;
        #else
            VkMemoryFdPropertiesKHR result_props;

            result_props.pNext = nullptr;
            result_props.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR;
        #endif

        #if defined(_WIN32)
            if (m_khr_external_memory_win32_extension_entrypoints.vkGetMemoryWin32HandlePropertiesKHR(m_device,
                                                                                                      static_cast<VkExternalMemoryHandleTypeFlagBits>(in_external_handle_type),
                                                                                                      in_handle,
                                                                                                     &result_props) != VK_SUCCESS)
        #else
            if (m_khr_external_memory_fd_extension_entrypoints.vkGetMemoryFdPropertiesKHR(m_device,
                                                                                          static_cast<VkExternalMemoryHandleTypeFlagBits>(in_external_handle_type),
                                                                                          in_handle,
                                                                                         &result_props) != VK_SUCCESS)
        #endif
        {
            anvil_assert_fail();

            goto end;
        }

        *out_supported_memory_type_bits = result_props.memoryTypeBits;
    }
    else
    {
        /* Sanity checks */
        if (!m_extension_enabled_info_ptr->get_device_extension_info()->ext_external_memory_host() )
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->ext_external_memory_host() );

            goto end;
        }

        /* Go ahead with the query */
        VkMemoryHostPointerPropertiesEXT result_props;

        result_props.pNext = nullptr;
        result_props.sType = VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT;

        if (m_ext_external_memory_host_extension_entrypoints.vkGetMemoryHostPointerPropertiesEXT(m_device,
                                                                                                 static_cast<VkExternalMemoryHandleTypeFlagBits>(in_external_handle_type),
                                                                                                 reinterpret_cast<const void*>(in_handle),
                                                                                                &result_props) != VK_SUCCESS)
        {
            anvil_assert_fail();

            goto end;
        }

        *out_supported_memory_type_bits = result_props.memoryTypeBits;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
uint32_t Anvil::BaseDevice::get_n_queues(uint32_t in_n_queue_family) const
{
    auto     map_iterator = m_queue_ptrs_per_vk_queue_fam.find(in_n_queue_family);
    uint32_t result       = 0;

    if (map_iterator != m_queue_ptrs_per_vk_queue_fam.end())
    {
        result = static_cast<uint32_t>(map_iterator->second.size() );
    }

    return result;
}

/* Please see header for specification */
void Anvil::BaseDevice::add_physical_device_features_to_chainer(Anvil::StructChainer<VkDeviceCreateInfo>* in_struct_chainer_ptr) const
{
    const auto& features                   = get_physical_device_features();
    const bool  should_add_features_struct = m_create_info_ptr->get_physical_device_ptrs().at(0)->get_instance()->get_enabled_extensions_info()->khr_get_physical_device_properties2();

    anvil_assert(in_struct_chainer_ptr != nullptr);

    if (should_add_features_struct)
    {
        VkPhysicalDeviceFeatures2KHR features_khr;

        features_khr.features = features.core_vk1_0_features_ptr->get_vk_physical_device_features();
        features_khr.pNext    = nullptr;
        features_khr.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;

        in_struct_chainer_ptr->append_struct(features_khr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_depth_clip_enable() )
    {
        in_struct_chainer_ptr->append_struct(features.ext_depth_clip_enable_features_ptr->get_vk_physical_device_depth_clip_enable_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_descriptor_indexing() )
    {
        in_struct_chainer_ptr->append_struct(features.ext_descriptor_indexing_features_ptr->get_vk_physical_device_descriptor_indexing_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_inline_uniform_block() )
    {
        in_struct_chainer_ptr->append_struct(features.ext_inline_uniform_block_features_ptr->get_vk_physical_device_inline_uniform_block_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_memory_priority() )
    {
        in_struct_chainer_ptr->append_struct(features.ext_memory_priority_features_ptr->get_vk_physical_device_memory_priority_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_scalar_block_layout() )
    {
        in_struct_chainer_ptr->append_struct(features.ext_scalar_block_layout_features_ptr->get_vk_physical_device_scalar_block_layout_features_ext() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_transform_feedback() )
    {
        in_struct_chainer_ptr->append_struct(features.ext_transform_feedback_features_ptr->get_vk_physical_device_transform_feedback_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_16bit_storage() )
    {
        in_struct_chainer_ptr->append_struct(features.khr_16bit_storage_features_ptr->get_vk_physical_device_16_bit_storage_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_8bit_storage() )
    {
        in_struct_chainer_ptr->append_struct(features.khr_8bit_storage_features_ptr->get_vk_physical_device_8_bit_storage_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_multiview() )
    {
        in_struct_chainer_ptr->append_struct(features.khr_multiview_features_ptr->get_vk_physical_device_multiview_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_sampler_ycbcr_conversion() )
    {
        in_struct_chainer_ptr->append_struct(features.khr_sampler_ycbcr_conversion_features_ptr->get_vk_physical_device_sampler_ycbcr_conversion_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_shader_atomic_int64() )
    {
        in_struct_chainer_ptr->append_struct(features.khr_shader_atomic_int64_features_ptr->get_vk_physical_device_shader_atomic_int64_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_shader_float16_int8() )
    {
        in_struct_chainer_ptr->append_struct(features.khr_float16_int8_features_ptr->get_vk_physical_device_float16_int8_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_variable_pointers() )
    {
        in_struct_chainer_ptr->append_struct(features.khr_variable_pointer_features_ptr->get_vk_physical_device_variable_pointer_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_vulkan_memory_model() )
    {
        in_struct_chainer_ptr->append_struct(features.khr_vulkan_memory_model_features_ptr->get_vk_physical_device_vulkan_memory_model_features() );
    }
}

/* Please see header for specification */
const Anvil::Instance* Anvil::BaseDevice::get_parent_instance() const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->get_instance();
}

/* Please see header for specification */

Anvil::Queue* Anvil::BaseDevice::get_queue(const Anvil::QueueFamilyType& in_queue_family_type,
                                           uint32_t                      in_n_queue) const
{
    Anvil::Queue* result_ptr = nullptr;

    switch (in_queue_family_type)
    {
        case Anvil::QueueFamilyType::COMPUTE:   result_ptr = get_compute_queue  (in_n_queue); break;
        case Anvil::QueueFamilyType::TRANSFER:  result_ptr = get_transfer_queue (in_n_queue); break;
        case Anvil::QueueFamilyType::UNIVERSAL: result_ptr = get_universal_queue(in_n_queue); break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result_ptr;
}

/* Please see header for specification */
void Anvil::BaseDevice::get_queue_family_indices_for_physical_device(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                                                     DeviceQueueFamilyInfo*       out_device_queue_family_info_ptr) const
{
    const auto                               n_queue_families                 = static_cast<uint32_t>(in_physical_device_ptr->get_queue_families().size() );
    std::vector<DeviceQueueFamilyMemberInfo> result_compute_queue_families;
    std::vector<DeviceQueueFamilyMemberInfo> result_transfer_queue_families;
    std::vector<DeviceQueueFamilyMemberInfo> result_universal_queue_families;

    for (uint32_t n_iteration = 0;
                  n_iteration < 3;
                ++n_iteration)
    {
        for (uint32_t n_queue_family_index = 0;
                      n_queue_family_index < n_queue_families;
                    ++n_queue_family_index)
        {
            const Anvil::QueueFamilyInfo& current_queue_family = in_physical_device_ptr->get_queue_families()[n_queue_family_index];

            if (n_iteration == 0)
            {
                if ( ((current_queue_family.flags & Anvil::QueueFlagBits::COMPUTE_BIT)  != 0) &&
                     ((current_queue_family.flags & Anvil::QueueFlagBits::GRAPHICS_BIT) == 0) )
                {
                    result_compute_queue_families.push_back(
                        DeviceQueueFamilyMemberInfo(n_queue_family_index,
                                                    current_queue_family.n_queues)
                    );
                }
            }
            else
            if (n_iteration == 1)
            {
                if ((current_queue_family.flags & Anvil::QueueFlagBits::GRAPHICS_BIT) != 0)
                {
                    if (std::find(result_compute_queue_families.begin(),
                                  result_compute_queue_families.end  (),
                                  n_queue_family_index) == result_compute_queue_families.end() )
                    {
                        result_universal_queue_families.push_back(
                            DeviceQueueFamilyMemberInfo(n_queue_family_index,
                                                        current_queue_family.n_queues)
                        );
                    }
                }
            }
            else
            if (n_iteration == 2)
            {
                if ((current_queue_family.flags & Anvil::QueueFlagBits::TRANSFER_BIT) != 0)
                {
                    if (std::find(result_compute_queue_families.begin(),
                                  result_compute_queue_families.end  (),
                                  n_queue_family_index) == result_compute_queue_families.end()   &&
                        std::find(result_universal_queue_families.begin(),
                                  result_universal_queue_families.end  (),
                                  n_queue_family_index) == result_universal_queue_families.end() )
                    {
                        result_transfer_queue_families.push_back(
                            DeviceQueueFamilyMemberInfo(n_queue_family_index,
                                                        current_queue_family.n_queues)
                        );
                    }
                }
            }
        }
    }

    /* NOTE: Vulkan API only guarantees universal queue family's availability */
    anvil_assert(result_universal_queue_families.size() > 0);

    out_device_queue_family_info_ptr->queue_families[Anvil::QueueFamilyType::COMPUTE]   = result_compute_queue_families;
    out_device_queue_family_info_ptr->queue_families[Anvil::QueueFamilyType::TRANSFER]  = result_transfer_queue_families;
    out_device_queue_family_info_ptr->queue_families[Anvil::QueueFamilyType::UNIVERSAL] = result_universal_queue_families;

    for (Anvil::QueueFamilyType current_queue_family_type  = Anvil::QueueFamilyType::FIRST;
                                current_queue_family_type != Anvil::QueueFamilyType::COUNT;
                                current_queue_family_type  = static_cast<Anvil::QueueFamilyType>(static_cast<uint32_t>(current_queue_family_type) + 1) )
    {
        uint32_t n_total_queues = 0;

        for (const auto& current_queue_fam : out_device_queue_family_info_ptr->queue_families[current_queue_family_type])
        {
            n_total_queues += current_queue_fam.n_queues;
        }

        out_device_queue_family_info_ptr->n_total_queues_per_family[static_cast<uint32_t>(current_queue_family_type)] = n_total_queues;
    }
}

/** Please see header for specification */
bool Anvil::BaseDevice::get_queue_family_indices_for_queue_family_type(const Anvil::QueueFamilyType& in_queue_family_type,
                                                                       uint32_t*                     out_opt_n_queue_family_indices_ptr,
                                                                       const uint32_t**              out_opt_queue_family_indices_ptr_ptr) const
{
    bool            result                          = false;
    uint32_t        result_n_queue_family_indices   = 0;
    const uint32_t* result_queue_family_indices_ptr = nullptr;

    auto map_iterator = m_queue_family_type_to_queue_family_indices.find(in_queue_family_type);

    if (map_iterator != m_queue_family_type_to_queue_family_indices.end() )
    {
        result                          = true;
        result_n_queue_family_indices   = static_cast<uint32_t>(map_iterator->second.size() );
        result_queue_family_indices_ptr = (map_iterator->second.size() > 0) ? &map_iterator->second.at(0)
                                                                            : nullptr;
    }
    else
    {
        result = true;
    }

    if (out_opt_n_queue_family_indices_ptr != nullptr)
    {
        *out_opt_n_queue_family_indices_ptr = result_n_queue_family_indices;
    }

    if (out_opt_queue_family_indices_ptr_ptr != nullptr)
    {
        *out_opt_queue_family_indices_ptr_ptr = result_queue_family_indices_ptr;
    }

    return result;
}

/** Please see header for specification */
Anvil::QueueFamilyType Anvil::BaseDevice::get_queue_family_type(uint32_t in_queue_family_index) const
{
    return m_queue_family_index_to_types.at(in_queue_family_index).at(0);
}

/** Please see header for specification */
Anvil::Queue* Anvil::BaseDevice::get_queue_for_queue_family_index(uint32_t in_n_queue_family,
                                                                  uint32_t in_n_queue) const
{
    auto          map_iterator = m_queue_ptrs_per_vk_queue_fam.find(in_n_queue_family);
    Anvil::Queue* result_ptr   = nullptr;

    if (map_iterator != m_queue_ptrs_per_vk_queue_fam.end())
    {
        if (map_iterator->second.size() > in_n_queue)
        {
            result_ptr = map_iterator->second.at(in_n_queue);
        }
    }

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::BaseDevice::get_sample_locations(Anvil::SampleCountFlagBits   in_sample_count,
                                             std::vector<SampleLocation>* out_result_ptr) const
{
    bool                        result                            = true;
    std::vector<SampleLocation> sample_locations;
    bool                        standard_sample_locations_support = false;

    switch (get_type() )
    {
        case Anvil::DeviceType::MULTI_GPU:
        {
            /* Sample locations must not differ for all devices within a given device group, so it technically
             * doesn't matter which physical device we choose to check for standard sample locations support.
             */
            const Anvil::MGPUDevice* mgpu_device_ptr = reinterpret_cast<const Anvil::MGPUDevice*>(this);
            anvil_assert(mgpu_device_ptr != nullptr);

            standard_sample_locations_support = (mgpu_device_ptr->get_physical_device_properties().core_vk1_0_properties_ptr->limits.standard_sample_locations);
            break;
        }

        case Anvil::DeviceType::SINGLE_GPU:
        {
            const Anvil::SGPUDevice* sgpu_device_ptr = reinterpret_cast<const Anvil::SGPUDevice*>(this);
            anvil_assert(sgpu_device_ptr != nullptr);

            standard_sample_locations_support = (sgpu_device_ptr->get_physical_device_properties().core_vk1_0_properties_ptr->limits.standard_sample_locations);
            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }

    if (!standard_sample_locations_support)
    {
        result = false;

        goto end;
    }

    switch (in_sample_count)
    {
        case Anvil::SampleCountFlagBits::_1_BIT:
        {
            out_result_ptr->clear();

            out_result_ptr->push_back(SampleLocation(0.5f, 0.5f) );

            break;
        }

        case Anvil::SampleCountFlagBits::_2_BIT:
        {
            out_result_ptr->clear();

            out_result_ptr->push_back(SampleLocation(0.25f, 0.25f) );
            out_result_ptr->push_back(SampleLocation(0.75f, 0.75f) );

            break;

        }

        case Anvil::SampleCountFlagBits::_4_BIT:
        {
            out_result_ptr->clear();

            out_result_ptr->push_back(SampleLocation(0.375f, 0.125f) );
            out_result_ptr->push_back(SampleLocation(0.875f, 0.375f) );
            out_result_ptr->push_back(SampleLocation(0.125f, 0.625f) );
            out_result_ptr->push_back(SampleLocation(0.625f, 0.875f) );

            break;
        }

        case Anvil::SampleCountFlagBits::_8_BIT:
        {
            out_result_ptr->clear();

            out_result_ptr->push_back(SampleLocation(0.5625f, 0.3125f) );
            out_result_ptr->push_back(SampleLocation(0.4375f, 0.6875f) );
            out_result_ptr->push_back(SampleLocation(0.8125f, 0.5625f) );
            out_result_ptr->push_back(SampleLocation(0.3125f, 0.1875f) );
            out_result_ptr->push_back(SampleLocation(0.1875f, 0.8125f) );
            out_result_ptr->push_back(SampleLocation(0.0625f, 0.4375f) );
            out_result_ptr->push_back(SampleLocation(0.6875f, 0.9375f) );
            out_result_ptr->push_back(SampleLocation(0.9375f, 0.0625f) );

            break;
        }

        case Anvil::SampleCountFlagBits::_16_BIT:
        {
            out_result_ptr->clear();

            out_result_ptr->push_back(SampleLocation(0.5625f,  0.5625f) );
            out_result_ptr->push_back(SampleLocation(0.4375f,  0.3125f) );
            out_result_ptr->push_back(SampleLocation(0.3125f,  0.625f)  );
            out_result_ptr->push_back(SampleLocation(0.75f,    0.4375f) );
            out_result_ptr->push_back(SampleLocation(0.1875f,  0.375f)  );
            out_result_ptr->push_back(SampleLocation(0.625f,   0.8125f) );
            out_result_ptr->push_back(SampleLocation(0.8125f,  0.6875f) );
            out_result_ptr->push_back(SampleLocation(0.6875f,  0.1875f) );
            out_result_ptr->push_back(SampleLocation(0.375f,   0.875f)  );
            out_result_ptr->push_back(SampleLocation(0.5f,     0.0625f) );
            out_result_ptr->push_back(SampleLocation(0.25f,    0.125f)  );
            out_result_ptr->push_back(SampleLocation(0.125f,   0.75f)   );
            out_result_ptr->push_back(SampleLocation(0.0f,     0.5f)    );
            out_result_ptr->push_back(SampleLocation(0.9375f,  0.25f)   );
            out_result_ptr->push_back(SampleLocation(0.875f,   0.9375f) );
            out_result_ptr->push_back(SampleLocation(0.0625f,  0.0f)    );

            break;
        }

        default:
        {
            anvil_assert_fail();

            goto end;
        }
    }

end:
    return result;
}

/* Please see header for specification */
Anvil::Queue* Anvil::BaseDevice::get_sparse_binding_queue(uint32_t          in_n_queue,
                                                          Anvil::QueueFlags in_opt_required_queue_flags) const
{
    uint32_t      n_queues_found = 0;
    Anvil::Queue* result_ptr     = nullptr;

    for (auto queue_ptr : m_sparse_binding_queues)
    {
        const uint32_t queue_family_index    = queue_ptr->get_queue_family_index();
        const auto&    queue_family_info_ptr = get_queue_family_info            (queue_family_index);

        if ((queue_family_info_ptr->flags & in_opt_required_queue_flags) == in_opt_required_queue_flags)
        {
            if (n_queues_found == in_n_queue)
            {
                result_ptr = queue_ptr;
                break;
            }
            else
            {
                ++n_queues_found;
            }
        }
    }

    return result_ptr;
}

/* Initializes a new Device instance */
bool Anvil::BaseDevice::init()
{
    const auto parent_instance_ptr(m_create_info_ptr->get_physical_device_ptrs().at(0)->get_instance() );

    std::map<std::string, bool> extensions_final_enabled_status;
    const bool                  is_validation_enabled(parent_instance_ptr->is_validation_enabled() );
    std::vector<const char*>    layers_final;
    const auto                  mt_safety            (Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe()) );
    bool                        result               (false);

    /* If validation is enabled, retrieve names of all suported validation layers and
     * append them to the list of layers the user has alreaedy specified. **/
    for (auto current_layer : m_create_info_ptr->get_layers_to_enable() )
    {
        layers_final.push_back(current_layer.c_str() );
    }

    if (is_validation_enabled)
    {
        if (is_layer_supported("VK_LAYER_LUNARG_standard_validation") )
        {
            layers_final.push_back("VK_LAYER_LUNARG_standard_validation");
        }
        else
        {
            anvil_assert(is_layer_supported("VK_LAYER_LUNARG_core_validation") );

            layers_final.push_back("VK_LAYER_LUNARG_core_validation");
        }
    }

    /* Go through the extension struct, verify availability of the requested extensions,
     * and cache the ones that have been requested and which are available in a vector. */
    {
        bool is_amd_negative_viewport_height_defined = false;
        bool is_khr_maintenance1_defined             = false;

        for (const auto& current_extension : m_create_info_ptr->get_extension_configuration().extension_status)
        {
            const bool is_ext_supported = is_physical_device_extension_supported(current_extension.first);

            is_amd_negative_viewport_height_defined |= (current_extension.first  == VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME &&
                                                        current_extension.second != Anvil::ExtensionAvailability::IGNORE);
            is_khr_maintenance1_defined             |= (current_extension.first  == VK_KHR_MAINTENANCE1_EXTENSION_NAME              &&
                                                        current_extension.second != Anvil::ExtensionAvailability::IGNORE);

            switch (current_extension.second)
            {
                case Anvil::ExtensionAvailability::ENABLE_IF_AVAILABLE:
                {
                    extensions_final_enabled_status[current_extension.first] = is_ext_supported;

                    break;
                }

                case Anvil::ExtensionAvailability::IGNORE:
                {
                    extensions_final_enabled_status[current_extension.first] = false;

                    break;
                }

                case Anvil::ExtensionAvailability::REQUIRE:
                {
                    if (!is_ext_supported)
                    {
                        char temp[1024];

                        if (snprintf(temp,
                                     sizeof(temp),
                                     "Device extension [%s] is unsupported",
                                     current_extension.first.c_str() ) > 0)
                        {
                            fprintf(stderr,
                                    "%s",
                                    temp);
                        }

                        anvil_assert_fail();
                    }

                    extensions_final_enabled_status[current_extension.first.c_str()] = is_ext_supported;

                    break;
                }

                default:
                {
                    anvil_assert_fail();
                }
            }
        }

        if (is_amd_negative_viewport_height_defined &&
            is_khr_maintenance1_defined)
        {
            /* VK_AMD_negative_viewport_height and VK_KHR_maintenance1 extensions are mutually exclusive. */
            anvil_assert_fail();

            goto end;
        }

        /* Instantiate the device. Actual behavior behind this is implemented by the overriding class. */
        {
            m_extension_enabled_info_ptr = Anvil::ExtensionInfo<bool>::create_device_extension_info(extensions_final_enabled_status,
                                                                                                    true); /* in_unspecified_extension_name_value */

            anvil_assert(m_device == VK_NULL_HANDLE);
            {
                std::vector<const char*> extension_name_raw_ptrs;

                extension_name_raw_ptrs.reserve(extensions_final_enabled_status.size() );

                for (const auto& current_extension_data : extensions_final_enabled_status)
                {
                    if (current_extension_data.second)
                    {
                        extension_name_raw_ptrs.push_back(current_extension_data.first.c_str() );
                    }
                }

                create_device(extension_name_raw_ptrs,
                              layers_final,
                             &m_device_queue_families);
            }
            anvil_assert(m_device != VK_NULL_HANDLE);
        }

        /* Re-create the "extension enabled info" variable, this time taking into account contexts newer than 1.0.
         *
         * This is important for applications that use VK 1.1 contexts (or newer) and do not take into account that Vulkan does not
         * require implementations to report support for extensions that have been folded into core.
         */
        if (m_create_info_ptr->get_physical_device_ptrs().at(0)->supports_core_vk1_1() )
        {
            extensions_final_enabled_status[VK_KHR_16BIT_STORAGE_EXTENSION_NAME]                = true;
            extensions_final_enabled_status[VK_KHR_BIND_MEMORY_2_EXTENSION_NAME]                = true;
            extensions_final_enabled_status[VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME]         = true;
            extensions_final_enabled_status[VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME]   = true;
            extensions_final_enabled_status[VK_KHR_DEVICE_GROUP_EXTENSION_NAME]                 = true;
            extensions_final_enabled_status[VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME]               = true;
            extensions_final_enabled_status[VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME]              = true;
            extensions_final_enabled_status[VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME]           = true;
            extensions_final_enabled_status[VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME]    = true;
            extensions_final_enabled_status[VK_KHR_MAINTENANCE1_EXTENSION_NAME]                 = true;
            extensions_final_enabled_status[VK_KHR_MAINTENANCE2_EXTENSION_NAME]                 = true;
            extensions_final_enabled_status[VK_KHR_MAINTENANCE3_EXTENSION_NAME]                 = true;
            extensions_final_enabled_status[VK_KHR_MULTIVIEW_EXTENSION_NAME]                    = true;
            extensions_final_enabled_status[VK_KHR_RELAXED_BLOCK_LAYOUT_EXTENSION_NAME]         = true;
            extensions_final_enabled_status[VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME]     = true;
            extensions_final_enabled_status[VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME]       = true;
            extensions_final_enabled_status[VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME] = true;
            extensions_final_enabled_status[VK_KHR_VARIABLE_POINTERS_EXTENSION_NAME]            = true;

            m_extension_enabled_info_ptr = Anvil::ExtensionInfo<bool>::create_device_extension_info(extensions_final_enabled_status,
                                                                                                    true); /* in_unspecified_extension_name_value */
        }
    }

    /* Retrieve device-specific func pointers */
    if (!init_extension_func_ptrs() )
    {
        anvil_assert_fail();

        goto end;
    }

    /* Spawn queue wrappers */
    for (Anvil::QueueFamilyType queue_family_type = Anvil::QueueFamilyType::FIRST;
                                queue_family_type < Anvil::QueueFamilyType::COUNT;
                                queue_family_type = static_cast<Anvil::QueueFamilyType>(static_cast<uint32_t>(queue_family_type) + 1))
    {
        const uint32_t              n_queues       = m_device_queue_families.n_total_queues_per_family[static_cast<uint32_t>(queue_family_type)];
        std::vector<Anvil::Queue*>* out_queues_ptr = nullptr;

        if (n_queues == 0)
        {
            continue;
        }

        switch (queue_family_type)
        {
            case Anvil::QueueFamilyType::COMPUTE:   out_queues_ptr = &m_compute_queues;   break;
            case Anvil::QueueFamilyType::TRANSFER:  out_queues_ptr = &m_transfer_queues;  break;
            case Anvil::QueueFamilyType::UNIVERSAL: out_queues_ptr = &m_universal_queues; break;

            default:
            {
                anvil_assert_fail();
            }
        }

        for (const auto& current_queue_fam : m_device_queue_families.queue_families[queue_family_type])
        {
            auto queue_ptr_storage_ptr = &m_queue_ptrs_per_vk_queue_fam[current_queue_fam.family_index];

            anvil_assert(std::find(m_queue_family_type_to_queue_family_indices[queue_family_type].begin(),
                                   m_queue_family_type_to_queue_family_indices[queue_family_type].end(),
                                   current_queue_fam.family_index)                                         == m_queue_family_type_to_queue_family_indices[queue_family_type].end() );

            m_queue_family_index_to_types              [current_queue_fam.family_index].push_back(queue_family_type);
            m_queue_family_type_to_queue_family_indices[queue_family_type].push_back             (current_queue_fam.family_index);

            for (uint32_t n_queue = 0;
                          n_queue < current_queue_fam.n_queues;
                        ++n_queue)
            {
                const auto                    new_queue_global_priority = m_create_info_ptr->get_queue_global_priority(current_queue_fam.family_index,
                                                                                                                       n_queue);
                std::unique_ptr<Anvil::Queue> new_queue_ptr             = Anvil::Queue::create                        (this,
                                                                                                                       current_queue_fam.family_index,
                                                                                                                       n_queue,
                                                                                                                       is_mt_safe(),
                                                                                                                       new_queue_global_priority);

                {
                    anvil_assert(out_queues_ptr != nullptr);

                    out_queues_ptr->push_back(new_queue_ptr.get() );
                }

                /* If this queue supports sparse binding ops, cache it in a separate vector as well */
                if (new_queue_ptr->supports_sparse_bindings() )
                {
                    m_sparse_binding_queues.push_back(new_queue_ptr.get() );
                }

                /* Cache the queue in general-purpose storage as well */
                if (std::find(queue_ptr_storage_ptr->cbegin(),
                              queue_ptr_storage_ptr->cend  (),
                              new_queue_ptr.get() ) == queue_ptr_storage_ptr->cend() )
                {
                    queue_ptr_storage_ptr->push_back(new_queue_ptr.get() );
                }

                m_owned_queues.push_back(
                    std::move(new_queue_ptr)
                );
            }
        }
    }

    /* Instantiate per-queue family command pools */
    m_command_pool_ptr_per_vk_queue_fam.resize(m_device_queue_families.queue_families.size() );

    for (const auto& current_queue_fam : m_device_queue_families.queue_families)
    {
        for (const auto& current_queue_fam_queue : current_queue_fam.second)
        {
            if (m_command_pool_ptr_per_vk_queue_fam.size() <= current_queue_fam_queue.family_index)
            {
                m_command_pool_ptr_per_vk_queue_fam.resize(current_queue_fam_queue.family_index + 1);
            }

            if (m_command_pool_ptr_per_vk_queue_fam[current_queue_fam_queue.family_index] == nullptr)
            {
                m_command_pool_ptr_per_vk_queue_fam[current_queue_fam_queue.family_index] =
                    Anvil::CommandPool::create(this,
                                               m_create_info_ptr->get_helper_command_pool_create_flags(),
                                               current_queue_fam_queue.family_index,
                                               mt_safety);
            }
        }
    }

    /* Set up shader module cache, if one was requested. */
    if (m_create_info_ptr->should_enable_shader_module_cache() )
    {
        m_shader_module_cache_ptr = Anvil::ShaderModuleCache::create();
    }

    /* Set up the pipeline cache */
    m_pipeline_cache_ptr = Anvil::PipelineCache::create(this,
                                                        is_mt_safe() );

    /* Cache a pipeline layout manager instance. */
    m_pipeline_layout_manager_ptr = Anvil::PipelineLayoutManager::create(this,
                                                                         is_mt_safe() );

    /* Cache a descriptor set layout manager. */
    m_descriptor_set_layout_manager_ptr = Anvil::DescriptorSetLayoutManager::create(this,
                                                                                    is_mt_safe() );

    /* Initialize compute & graphics pipeline managers */
    m_compute_pipeline_manager_ptr  = Anvil::ComputePipelineManager::create (this,
                                                                             is_mt_safe() ,
                                                                             true /* use_pipeline_cache */,
                                                                             m_pipeline_cache_ptr.get() );
    m_graphics_pipeline_manager_ptr = Anvil::GraphicsPipelineManager::create(this,
                                                                             is_mt_safe() ,
                                                                             true /* use_pipeline_cache */,
                                                                             m_pipeline_cache_ptr.get() );

    /* Continue with specialized initialization */
    init_device();

    result = true;
end:
    return result;
}

bool Anvil::BaseDevice::init_dummy_dsg() const
{
    bool result = false;

    /* Initialize a dummy descriptor set group */
    std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> dummy_ds_create_info_ptrs(1);
    std::vector<Anvil::OverheadAllocation>               dummy_overhead_allocs;

    dummy_overhead_allocs.push_back(
        Anvil::OverheadAllocation(Anvil::DescriptorType::SAMPLER,
                                  1) /* in_n_overhead_allocs */
    );

    dummy_ds_create_info_ptrs[0] = Anvil::DescriptorSetCreateInfo::create();
    dummy_ds_create_info_ptrs[0]->add_binding(0, /* n_binding */
                                             Anvil::DescriptorType::UNKNOWN,
                                             0,  /* n_elements  */
                                             Anvil::ShaderStageFlagBits::NONE); /* stage_flags */

    m_dummy_dsg_ptr = Anvil::DescriptorSetGroup::create(this,
                                                        dummy_ds_create_info_ptrs,
                                                        false, /* releaseable_sets */
                                                        Anvil::MTSafety::DISABLED,
                                                        dummy_overhead_allocs);

    if (m_dummy_dsg_ptr == nullptr)
    {
        anvil_assert(m_dummy_dsg_ptr != nullptr);

        goto end;
    }

    m_dummy_dsg_ptr->get_descriptor_set(0)->update();

    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::BaseDevice::init_extension_func_ptrs()
{
    const bool is_core_vk11_device(m_create_info_ptr->get_physical_device_ptrs().at(0)->supports_core_vk1_1() );

    if (m_extension_enabled_info_ptr->get_device_extension_info()->amd_buffer_marker() )
    {
        m_amd_buffer_marker_extension_entrypoints.vkCmdWriteBufferMarkerAMD = reinterpret_cast<PFN_vkCmdWriteBufferMarkerAMD>(get_proc_address("vkCmdWriteBufferMarkerAMD") );

        anvil_assert(m_amd_buffer_marker_extension_entrypoints.vkCmdWriteBufferMarkerAMD != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->amd_draw_indirect_count() )
    {
        m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndexedIndirectCountAMD = reinterpret_cast<PFN_vkCmdDrawIndexedIndirectCountAMD>(get_proc_address("vkCmdDrawIndexedIndirectCountAMD") );
        m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndirectCountAMD        = reinterpret_cast<PFN_vkCmdDrawIndirectCountAMD>       (get_proc_address("vkCmdDrawIndirectCountAMD") );

        anvil_assert(m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndexedIndirectCountAMD != nullptr);
        anvil_assert(m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndirectCountAMD        != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->amd_shader_info() )
    {
        m_amd_shader_info_extension_entrypoints.vkGetShaderInfoAMD = reinterpret_cast<PFN_vkGetShaderInfoAMD>(get_proc_address("vkGetShaderInfoAMD") );

        anvil_assert(m_amd_shader_info_extension_entrypoints.vkGetShaderInfoAMD != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_debug_marker() )
    {
        m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerBeginEXT      = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>     (get_proc_address("vkCmdDebugMarkerBeginEXT") );
        m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerEndEXT        = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>       (get_proc_address("vkCmdDebugMarkerEndEXT") );
        m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerInsertEXT     = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>    (get_proc_address("vkCmdDebugMarkerInsertEXT") );
        m_ext_debug_marker_extension_entrypoints.vkDebugMarkerSetObjectNameEXT = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(get_proc_address("vkDebugMarkerSetObjectNameEXT") );
        m_ext_debug_marker_extension_entrypoints.vkDebugMarkerSetObjectTagEXT  = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT> (get_proc_address("vkDebugMarkerSetObjectTagEXT") );

        anvil_assert(m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerBeginEXT      != nullptr);
        anvil_assert(m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerEndEXT        != nullptr);
        anvil_assert(m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerInsertEXT     != nullptr);
        anvil_assert(m_ext_debug_marker_extension_entrypoints.vkDebugMarkerSetObjectNameEXT != nullptr);
        anvil_assert(m_ext_debug_marker_extension_entrypoints.vkDebugMarkerSetObjectTagEXT  != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_external_memory_host() )
    {
        m_ext_external_memory_host_extension_entrypoints.vkGetMemoryHostPointerPropertiesEXT = reinterpret_cast<PFN_vkGetMemoryHostPointerPropertiesEXT>(get_proc_address("vkGetMemoryHostPointerPropertiesEXT") );

        anvil_assert(m_ext_external_memory_host_extension_entrypoints.vkGetMemoryHostPointerPropertiesEXT != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_hdr_metadata() )
    {
        m_ext_hdr_metadata_extension_entrypoints.vkSetHdrMetadataEXT = reinterpret_cast<PFN_vkSetHdrMetadataEXT>(get_proc_address("vkSetHdrMetadataEXT") );

        anvil_assert(m_ext_hdr_metadata_extension_entrypoints.vkSetHdrMetadataEXT != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_sample_locations() )
    {
        m_ext_sample_locations_extension_entrypoints.vkCmdSetSampleLocationsEXT                  = reinterpret_cast<PFN_vkCmdSetSampleLocationsEXT>                 (get_proc_address("vkCmdSetSampleLocationsEXT") );
        m_ext_sample_locations_extension_entrypoints.vkGetPhysicalDeviceMultisamplePropertiesEXT = reinterpret_cast<PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT>(get_proc_address("vkGetPhysicalDeviceMultisamplePropertiesEXT") );

        anvil_assert(m_ext_sample_locations_extension_entrypoints.vkCmdSetSampleLocationsEXT                  != nullptr);
        anvil_assert(m_ext_sample_locations_extension_entrypoints.vkGetPhysicalDeviceMultisamplePropertiesEXT != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_transform_feedback() )
    {
        m_ext_transform_feedback_extension_entrypoints.vkCmdBeginQueryIndexedEXT            = reinterpret_cast<PFN_vkCmdBeginQueryIndexedEXT>           (get_proc_address("vkCmdBeginQueryIndexedEXT") );
        m_ext_transform_feedback_extension_entrypoints.vkCmdBeginTransformFeedbackEXT       = reinterpret_cast<PFN_vkCmdBeginTransformFeedbackEXT>      (get_proc_address("vkCmdBeginTransformFeedbackEXT") );
        m_ext_transform_feedback_extension_entrypoints.vkCmdBindTransformFeedbackBuffersEXT = reinterpret_cast<PFN_vkCmdBindTransformFeedbackBuffersEXT>(get_proc_address("vkCmdBindTransformFeedbackBuffersEXT") );
        m_ext_transform_feedback_extension_entrypoints.vkCmdDrawIndirectByteCountEXT        = reinterpret_cast<PFN_vkCmdDrawIndirectByteCountEXT>       (get_proc_address("vkCmdDrawIndirectByteCountEXT") );
        m_ext_transform_feedback_extension_entrypoints.vkCmdEndQueryIndexedEXT              = reinterpret_cast<PFN_vkCmdEndQueryIndexedEXT>             (get_proc_address("vkCmdEndQueryIndexedEXT") );
        m_ext_transform_feedback_extension_entrypoints.vkCmdEndTransformFeedbackEXT         = reinterpret_cast<PFN_vkCmdEndTransformFeedbackEXT>        (get_proc_address("vkCmdEndTransformFeedbackEXT") );

        anvil_assert(m_ext_transform_feedback_extension_entrypoints.vkCmdBeginQueryIndexedEXT            != nullptr);
        anvil_assert(m_ext_transform_feedback_extension_entrypoints.vkCmdBeginTransformFeedbackEXT       != nullptr);
        anvil_assert(m_ext_transform_feedback_extension_entrypoints.vkCmdBindTransformFeedbackBuffersEXT != nullptr);
        anvil_assert(m_ext_transform_feedback_extension_entrypoints.vkCmdDrawIndirectByteCountEXT        != nullptr);
        anvil_assert(m_ext_transform_feedback_extension_entrypoints.vkCmdEndQueryIndexedEXT              != nullptr);
        anvil_assert(m_ext_transform_feedback_extension_entrypoints.vkCmdEndTransformFeedbackEXT         != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_descriptor_update_template() ||
        is_core_vk11_device)
    {
        const auto vk_create_descriptor_update_template_entrypoint_name  = (is_core_vk11_device) ? "vkCreateDescriptorUpdateTemplate"
                                                                                                 : "vkCreateDescriptorUpdateTemplateKHR";
        const auto vk_destroy_descriptor_update_template_entrypoint_name = (is_core_vk11_device) ? "vkDestroyDescriptorUpdateTemplate"
                                                                                                 : "vkDestroyDescriptorUpdateTemplateKHR";
        const auto vk_update_descriptor_update_template_entrypoint_name  = (is_core_vk11_device) ? "vkUpdateDescriptorSetWithTemplate"
                                                                                                 : "vkUpdateDescriptorSetWithTemplateKHR";

        m_khr_descriptor_update_template_extension_entrypoints.vkCreateDescriptorUpdateTemplateKHR  = reinterpret_cast<PFN_vkCreateDescriptorUpdateTemplateKHR> (get_proc_address(vk_create_descriptor_update_template_entrypoint_name) );
        m_khr_descriptor_update_template_extension_entrypoints.vkDestroyDescriptorUpdateTemplateKHR = reinterpret_cast<PFN_vkDestroyDescriptorUpdateTemplateKHR>(get_proc_address(vk_destroy_descriptor_update_template_entrypoint_name));
        m_khr_descriptor_update_template_extension_entrypoints.vkUpdateDescriptorSetWithTemplateKHR = reinterpret_cast<PFN_vkUpdateDescriptorSetWithTemplateKHR>(get_proc_address(vk_update_descriptor_update_template_entrypoint_name) );

        anvil_assert(m_khr_descriptor_update_template_extension_entrypoints.vkCreateDescriptorUpdateTemplateKHR  != nullptr);
        anvil_assert(m_khr_descriptor_update_template_extension_entrypoints.vkDestroyDescriptorUpdateTemplateKHR != nullptr);
        anvil_assert(m_khr_descriptor_update_template_extension_entrypoints.vkUpdateDescriptorSetWithTemplateKHR != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_device_group() ||
        is_core_vk11_device)
    {
        const auto vk_cmd_dispatch_base_entrypoint_name                     = (is_core_vk11_device) ? "vkCmdDispatchBase"
                                                                                                    : "vkCmdDispatchBaseKHR";
        const auto vk_cmd_set_device_mask_entrypoint_name                   = (is_core_vk11_device) ? "vkCmdSetDeviceMask"
                                                                                                    : "vkCmdSetDeviceMaskKHR";
        const auto vk_get_device_group_peer_memory_features_entrypoint_name = (is_core_vk11_device) ? "vkGetDeviceGroupPeerMemoryFeatures"
                                                                                                    : "vkGetDeviceGroupPeerMemoryFeaturesKHR";

        m_khr_device_group_extension_entrypoints.vkCmdDispatchBaseKHR                    = reinterpret_cast<PFN_vkCmdDispatchBaseKHR>                   (get_proc_address(vk_cmd_dispatch_base_entrypoint_name) );
        m_khr_device_group_extension_entrypoints.vkCmdSetDeviceMaskKHR                   = reinterpret_cast<PFN_vkCmdSetDeviceMaskKHR>                  (get_proc_address(vk_cmd_set_device_mask_entrypoint_name) );
        m_khr_device_group_extension_entrypoints.vkGetDeviceGroupPeerMemoryFeaturesKHR   = reinterpret_cast<PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR>  (get_proc_address(vk_get_device_group_peer_memory_features_entrypoint_name) );

        anvil_assert(m_khr_device_group_extension_entrypoints.vkCmdDispatchBaseKHR                  != nullptr);
        anvil_assert(m_khr_device_group_extension_entrypoints.vkCmdSetDeviceMaskKHR                 != nullptr);
        anvil_assert(m_khr_device_group_extension_entrypoints.vkGetDeviceGroupPeerMemoryFeaturesKHR != nullptr);

        {
            /* NOTE: Certain device group entrypoints are only available if KHR_swapchain is also enabled */
            if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_swapchain() )
            {
                m_khr_device_group_extension_entrypoints.vkAcquireNextImage2KHR                  = reinterpret_cast<PFN_vkAcquireNextImage2KHR>                 (get_proc_address("vkAcquireNextImage2KHR") );
                m_khr_device_group_extension_entrypoints.vkGetPhysicalDevicePresentRectanglesKHR = reinterpret_cast<PFN_vkGetPhysicalDevicePresentRectanglesKHR>(get_proc_address("vkGetPhysicalDevicePresentRectanglesKHR") );
                m_khr_device_group_extension_entrypoints.vkGetDeviceGroupSurfacePresentModesKHR  = reinterpret_cast<PFN_vkGetDeviceGroupSurfacePresentModesKHR> (get_proc_address("vkGetDeviceGroupSurfacePresentModesKHR") );
                m_khr_device_group_extension_entrypoints.vkGetDeviceGroupPresentCapabilitiesKHR  = reinterpret_cast<PFN_vkGetDeviceGroupPresentCapabilitiesKHR> (get_proc_address("vkGetDeviceGroupPresentCapabilitiesKHR") );

                anvil_assert(m_khr_device_group_extension_entrypoints.vkAcquireNextImage2KHR                  != nullptr);
                anvil_assert(m_khr_device_group_extension_entrypoints.vkGetDeviceGroupPresentCapabilitiesKHR  != nullptr);
                anvil_assert(m_khr_device_group_extension_entrypoints.vkGetDeviceGroupSurfacePresentModesKHR  != nullptr);
                anvil_assert(m_khr_device_group_extension_entrypoints.vkGetPhysicalDevicePresentRectanglesKHR != nullptr);
            }
        }
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_bind_memory2() ||
        is_core_vk11_device)
    {
        const char* vk_bind_buffer_memory_2_entrypoint_name = (is_core_vk11_device) ? "vkBindBufferMemory2"
                                                                                    : "vkBindBufferMemory2KHR";
        const char* vk_bind_image_memory_2_entrypoint_name  = (is_core_vk11_device) ? "vkBindImageMemory2"
                                                                                    : "vkBindImageMemory2KHR";

        m_khr_bind_memory2_extension_entrypoints.vkBindBufferMemory2KHR = reinterpret_cast<PFN_vkBindBufferMemory2KHR>(get_proc_address(vk_bind_buffer_memory_2_entrypoint_name));
        m_khr_bind_memory2_extension_entrypoints.vkBindImageMemory2KHR  = reinterpret_cast<PFN_vkBindImageMemory2KHR> (get_proc_address(vk_bind_image_memory_2_entrypoint_name));

        anvil_assert(m_khr_bind_memory2_extension_entrypoints.vkBindBufferMemory2KHR != nullptr);
        anvil_assert(m_khr_bind_memory2_extension_entrypoints.vkBindImageMemory2KHR  != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_create_renderpass2() )
    {
        m_khr_create_renderpass2_extension_entrypoints.vkCmdBeginRenderPass2KHR = reinterpret_cast<PFN_vkCmdBeginRenderPass2KHR>(get_proc_address("vkCmdBeginRenderPass2KHR"));
        m_khr_create_renderpass2_extension_entrypoints.vkCmdEndRenderPass2KHR   = reinterpret_cast<PFN_vkCmdEndRenderPass2KHR>  (get_proc_address("vkCmdEndRenderPass2KHR"));
        m_khr_create_renderpass2_extension_entrypoints.vkCmdNextSubpass2KHR     = reinterpret_cast<PFN_vkCmdNextSubpass2KHR>    (get_proc_address("vkCmdNextSubpass2KHR"));
        m_khr_create_renderpass2_extension_entrypoints.vkCreateRenderPass2KHR   = reinterpret_cast<PFN_vkCreateRenderPass2KHR>  (get_proc_address("vkCreateRenderPass2KHR"));

        anvil_assert(m_khr_create_renderpass2_extension_entrypoints.vkCmdBeginRenderPass2KHR != nullptr);
        anvil_assert(m_khr_create_renderpass2_extension_entrypoints.vkCmdEndRenderPass2KHR   != nullptr);
        anvil_assert(m_khr_create_renderpass2_extension_entrypoints.vkCmdNextSubpass2KHR     != nullptr);
        anvil_assert(m_khr_create_renderpass2_extension_entrypoints.vkCreateRenderPass2KHR   != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_draw_indirect_count() )
    {
        m_khr_draw_indirect_count_extension_entrypoints.vkCmdDrawIndexedIndirectCountKHR = reinterpret_cast<PFN_vkCmdDrawIndexedIndirectCountKHR>(get_proc_address("vkCmdDrawIndexedIndirectCountKHR") );
        m_khr_draw_indirect_count_extension_entrypoints.vkCmdDrawIndirectCountKHR        = reinterpret_cast<PFN_vkCmdDrawIndirectCountKHR>       (get_proc_address("vkCmdDrawIndirectCountKHR") );

        anvil_assert(m_khr_draw_indirect_count_extension_entrypoints.vkCmdDrawIndexedIndirectCountKHR != nullptr);
        anvil_assert(m_khr_draw_indirect_count_extension_entrypoints.vkCmdDrawIndirectCountKHR        != nullptr);
    }

    #if defined(_WIN32)
    {
        if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_fence_win32() )
        {
            m_khr_external_fence_win32_extension_entrypoints.vkGetFenceWin32HandleKHR    = reinterpret_cast<PFN_vkGetFenceWin32HandleKHR>   (get_proc_address("vkGetFenceWin32HandleKHR") );
            m_khr_external_fence_win32_extension_entrypoints.vkImportFenceWin32HandleKHR = reinterpret_cast<PFN_vkImportFenceWin32HandleKHR>(get_proc_address("vkImportFenceWin32HandleKHR") );

            anvil_assert(m_khr_external_fence_win32_extension_entrypoints.vkGetFenceWin32HandleKHR    != nullptr);
            anvil_assert(m_khr_external_fence_win32_extension_entrypoints.vkImportFenceWin32HandleKHR != nullptr);
        }

        if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_memory_win32() )
        {
            m_khr_external_memory_win32_extension_entrypoints.vkGetMemoryWin32HandleKHR           = reinterpret_cast<PFN_vkGetMemoryWin32HandleKHR>          (get_proc_address("vkGetMemoryWin32HandleKHR"));
            m_khr_external_memory_win32_extension_entrypoints.vkGetMemoryWin32HandlePropertiesKHR = reinterpret_cast<PFN_vkGetMemoryWin32HandlePropertiesKHR>(get_proc_address("vkGetMemoryWin32HandlePropertiesKHR"));

            anvil_assert(m_khr_external_memory_win32_extension_entrypoints.vkGetMemoryWin32HandleKHR           != nullptr);
            anvil_assert(m_khr_external_memory_win32_extension_entrypoints.vkGetMemoryWin32HandlePropertiesKHR != nullptr);
        }

        if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_semaphore_win32() )
        {
            m_khr_external_semaphore_win32_extension_entrypoints.vkGetSemaphoreWin32HandleKHR    = reinterpret_cast<PFN_vkGetSemaphoreWin32HandleKHR>   (get_proc_address("vkGetSemaphoreWin32HandleKHR"));
            m_khr_external_semaphore_win32_extension_entrypoints.vkImportSemaphoreWin32HandleKHR = reinterpret_cast<PFN_vkImportSemaphoreWin32HandleKHR>(get_proc_address("vkImportSemaphoreWin32HandleKHR"));

            anvil_assert(m_khr_external_semaphore_win32_extension_entrypoints.vkGetSemaphoreWin32HandleKHR    != nullptr);
            anvil_assert(m_khr_external_semaphore_win32_extension_entrypoints.vkImportSemaphoreWin32HandleKHR != nullptr);
        }
    }
    #else
    {
        if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_fence_fd() )
        {
            m_khr_external_fence_fd_extension_entrypoints.vkGetFenceFdKHR    = reinterpret_cast<PFN_vkGetFenceFdKHR>   (get_proc_address("vkGetFenceFdKHR") );
            m_khr_external_fence_fd_extension_entrypoints.vkImportFenceFdKHR = reinterpret_cast<PFN_vkImportFenceFdKHR>(get_proc_address("vkImportFenceFdKHR") );

            anvil_assert(m_khr_external_fence_fd_extension_entrypoints.vkGetFenceFdKHR    != nullptr);
            anvil_assert(m_khr_external_fence_fd_extension_entrypoints.vkImportFenceFdKHR != nullptr);
        }

        if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_memory_fd() )
        {
            m_khr_external_memory_fd_extension_entrypoints.vkGetMemoryFdKHR           = reinterpret_cast<PFN_vkGetMemoryFdKHR>          (get_proc_address("vkGetMemoryFdKHR"));
            m_khr_external_memory_fd_extension_entrypoints.vkGetMemoryFdPropertiesKHR = reinterpret_cast<PFN_vkGetMemoryFdPropertiesKHR>(get_proc_address("vkGetMemoryFdPropertiesKHR"));

            anvil_assert(m_khr_external_memory_fd_extension_entrypoints.vkGetMemoryFdKHR           != nullptr);
            anvil_assert(m_khr_external_memory_fd_extension_entrypoints.vkGetMemoryFdPropertiesKHR != nullptr);
        }

        if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_semaphore_fd() )
        {
            m_khr_external_semaphore_fd_extension_entrypoints.vkGetSemaphoreFdKHR    = reinterpret_cast<PFN_vkGetSemaphoreFdKHR>   (get_proc_address("vkGetSemaphoreFdKHR"));
            m_khr_external_semaphore_fd_extension_entrypoints.vkImportSemaphoreFdKHR = reinterpret_cast<PFN_vkImportSemaphoreFdKHR>(get_proc_address("vkImportSemaphoreFdKHR"));

            anvil_assert(m_khr_external_semaphore_fd_extension_entrypoints.vkGetSemaphoreFdKHR    != nullptr);
            anvil_assert(m_khr_external_semaphore_fd_extension_entrypoints.vkImportSemaphoreFdKHR != nullptr);
        }
    }
    #endif

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_get_memory_requirements2() ||
        is_core_vk11_device)
    {
        const char* vk_get_buffer_memory_requirements_2_entrypoint_name       = (is_core_vk11_device) ? "vkGetBufferMemoryRequirements2"
                                                                                                      : "vkGetBufferMemoryRequirements2KHR";
        const char* vk_get_image_memory_requirements_2_entrypoint_name        = (is_core_vk11_device) ? "vkGetImageMemoryRequirements2"
                                                                                                      : "vkGetImageMemoryRequirements2KHR";
        const char* vk_get_image_sparse_memory_requirements_2_entrypoint_name = (is_core_vk11_device) ? "vkGetImageSparseMemoryRequirements2"
                                                                                                      : "vkGetImageSparseMemoryRequirements2KHR";

        m_khr_get_memory_requirements2_extension_entrypoints.vkGetBufferMemoryRequirements2KHR      = reinterpret_cast<PFN_vkGetBufferMemoryRequirements2KHR>     (get_proc_address(vk_get_buffer_memory_requirements_2_entrypoint_name) );
        m_khr_get_memory_requirements2_extension_entrypoints.vkGetImageMemoryRequirements2KHR       = reinterpret_cast<PFN_vkGetImageMemoryRequirements2KHR>      (get_proc_address(vk_get_image_memory_requirements_2_entrypoint_name) );
        m_khr_get_memory_requirements2_extension_entrypoints.vkGetImageSparseMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetImageSparseMemoryRequirements2KHR>(get_proc_address(vk_get_image_sparse_memory_requirements_2_entrypoint_name) );

        anvil_assert(m_khr_get_memory_requirements2_extension_entrypoints.vkGetBufferMemoryRequirements2KHR      != nullptr);
        anvil_assert(m_khr_get_memory_requirements2_extension_entrypoints.vkGetImageMemoryRequirements2KHR       != nullptr);
        anvil_assert(m_khr_get_memory_requirements2_extension_entrypoints.vkGetImageSparseMemoryRequirements2KHR != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_maintenance1() ||
        is_core_vk11_device)
    {
        const char* vk_trim_command_pool_entrypoint_name = (is_core_vk11_device) ? "vkTrimCommandPool"
                                                                                 : "vkTrimCommandPoolKHR";

        m_khr_maintenance1_extension_entrypoints.vkTrimCommandPoolKHR = reinterpret_cast<PFN_vkTrimCommandPoolKHR>(get_proc_address(vk_trim_command_pool_entrypoint_name) );

        anvil_assert(m_khr_maintenance1_extension_entrypoints.vkTrimCommandPoolKHR != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_maintenance3() ||
        is_core_vk11_device)
    {
        const char* vk_get_descriptor_set_layout_support_entrypoint_name = (is_core_vk11_device) ? "vkGetDescriptorSetLayoutSupport"
                                                                                                 : "vkGetDescriptorSetLayoutSupportKHR";

        m_khr_maintenance3_extension_entrypoints.vkGetDescriptorSetLayoutSupportKHR = reinterpret_cast<PFN_vkGetDescriptorSetLayoutSupportKHR>(get_proc_address(vk_get_descriptor_set_layout_support_entrypoint_name) );

        anvil_assert(m_khr_maintenance3_extension_entrypoints.vkGetDescriptorSetLayoutSupportKHR != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_sampler_ycbcr_conversion() ||
        is_core_vk11_device)
    {
        const char* vk_create_sampler_ycbcr_conversion_entrypoint_name  = (is_core_vk11_device) ? "vkCreateSamplerYcbcrConversion"
                                                                                                : "vkCreateSamplerYcbcrConversionKHR";
        const char* vk_destroy_sampler_ycbcr_conversion_entrypoint_name = (is_core_vk11_device) ? "vkDestroySamplerYcbcrConversion"
                                                                                                : "vkDestroySamplerYcbcrConversionKHR";

        m_khr_sampler_ycbcr_conversion_extension_entrypoints.vkCreateSamplerYcbcrConversionKHR  = reinterpret_cast<PFN_vkCreateSamplerYcbcrConversionKHR> (get_proc_address(vk_create_sampler_ycbcr_conversion_entrypoint_name) );
        m_khr_sampler_ycbcr_conversion_extension_entrypoints.vkDestroySamplerYcbcrConversionKHR = reinterpret_cast<PFN_vkDestroySamplerYcbcrConversionKHR>(get_proc_address(vk_destroy_sampler_ycbcr_conversion_entrypoint_name) );

        anvil_assert(m_khr_sampler_ycbcr_conversion_extension_entrypoints.vkCreateSamplerYcbcrConversionKHR  != nullptr);
        anvil_assert(m_khr_sampler_ycbcr_conversion_extension_entrypoints.vkDestroySamplerYcbcrConversionKHR != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_swapchain() )
    {
        m_khr_swapchain_extension_entrypoints.vkAcquireNextImageKHR   = reinterpret_cast<PFN_vkAcquireNextImageKHR>  (get_proc_address("vkAcquireNextImageKHR") );
        m_khr_swapchain_extension_entrypoints.vkCreateSwapchainKHR    = reinterpret_cast<PFN_vkCreateSwapchainKHR>   (get_proc_address("vkCreateSwapchainKHR") );
        m_khr_swapchain_extension_entrypoints.vkDestroySwapchainKHR   = reinterpret_cast<PFN_vkDestroySwapchainKHR>  (get_proc_address("vkDestroySwapchainKHR") );
        m_khr_swapchain_extension_entrypoints.vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(get_proc_address("vkGetSwapchainImagesKHR") );
        m_khr_swapchain_extension_entrypoints.vkQueuePresentKHR       = reinterpret_cast<PFN_vkQueuePresentKHR>      (get_proc_address("vkQueuePresentKHR") );

        anvil_assert(m_khr_swapchain_extension_entrypoints.vkAcquireNextImageKHR   != nullptr);
        anvil_assert(m_khr_swapchain_extension_entrypoints.vkCreateSwapchainKHR    != nullptr);
        anvil_assert(m_khr_swapchain_extension_entrypoints.vkDestroySwapchainKHR   != nullptr);
        anvil_assert(m_khr_swapchain_extension_entrypoints.vkGetSwapchainImagesKHR != nullptr);
        anvil_assert(m_khr_swapchain_extension_entrypoints.vkQueuePresentKHR       != nullptr);
    }

    return true;
}

/** Please see header for specification */
bool Anvil::BaseDevice::is_compute_queue_family_index(const uint32_t& in_queue_family_index) const
{
    return (m_queue_family_index_to_types.find(in_queue_family_index)       != m_queue_family_index_to_types.end()) &&
           (m_queue_family_index_to_types.at  (in_queue_family_index).at(0) == Anvil::QueueFamilyType::COMPUTE);
}

/** Please see header for specification */
bool Anvil::BaseDevice::is_transfer_queue_family_index(const uint32_t& in_queue_family_index) const
{
    return (m_queue_family_index_to_types.find(in_queue_family_index)       != m_queue_family_index_to_types.end()) &&
           (m_queue_family_index_to_types.at  (in_queue_family_index).at(0) == Anvil::QueueFamilyType::TRANSFER);
}

/** Please see header for specification */
bool Anvil::BaseDevice::is_universal_queue_family_index(const uint32_t& in_queue_family_index) const
{
    return (m_queue_family_index_to_types.find(in_queue_family_index)       != m_queue_family_index_to_types.end() ) &&
           (m_queue_family_index_to_types.at  (in_queue_family_index).at(0) == Anvil::QueueFamilyType::UNIVERSAL);
}

/* Please see header for specification */
bool Anvil::BaseDevice::wait_idle() const
{
    const bool mt_safe = is_mt_safe();

    VkResult result_vk;

    if (mt_safe)
    {
        for (const auto& queue_fam : m_queue_ptrs_per_vk_queue_fam)
        {
            for (const auto& queue_ptr : queue_fam.second)
            {
                queue_ptr->lock();
            }
        }
    }

    result_vk = Anvil::Vulkan::vkDeviceWaitIdle(m_device);

    if (mt_safe)
    {
        for (const auto& queue_fam : m_queue_ptrs_per_vk_queue_fam)
        {
            for (const auto& queue_ptr : queue_fam.second)
            {
                queue_ptr->unlock();
            }
        }
    }

    return is_vk_call_successful(result_vk);
}


/* Please see header for specification */
Anvil::MGPUDevice::MGPUDevice(Anvil::DeviceCreateInfoUniquePtr in_create_info_ptr)
    :BaseDevice               (std::move(in_create_info_ptr) ),
     m_supported_present_modes(static_cast<Anvil::DeviceGroupPresentModeFlagBits>(0) )
{
    const auto&    physical_device_ptrs   = m_create_info_ptr->get_physical_device_ptrs();
    const uint32_t n_physical_device_ptrs = static_cast<uint32_t>(physical_device_ptrs.size() );

    ANVIL_REDUNDANT_VARIABLE_CONST(n_physical_device_ptrs);

    /* MGPUDevice instance should not be used for cases where SGPUDevice would do */
    anvil_assert(n_physical_device_ptrs > 1);

    /* Sanity checks */
    #ifdef _DEBUG
    {
        for (uint32_t n_physical_device = 0;
                      n_physical_device < n_physical_device_ptrs - 1;
                    ++n_physical_device)
        {
            auto next_device_ptr(physical_device_ptrs[n_physical_device + 1]);
            auto this_device_ptr(physical_device_ptrs[n_physical_device]);

            /* Physical devices must come from the same Vulkan instance */
            anvil_assert(this_device_ptr->get_instance() == next_device_ptr->get_instance() );

            /* Physical devices must not repeat */
            anvil_assert(this_device_ptr != next_device_ptr);

            /* All physical devices must come from the same device group */
            anvil_assert(this_device_ptr->get_device_group_index() == next_device_ptr->get_device_group_index() );

            /* ..but must be ed unique device indices */
            anvil_assert(this_device_ptr->get_device_group_device_index() != next_device_ptr->get_device_group_device_index() );

            /* Extension spec is a bit vague about this restriction, but our implementation assumes that all physical devices
             * in a device group offer exactly the same set of queues and queue families. Their ordering also must match.
             */
            anvil_assert(this_device_ptr->get_queue_families() == next_device_ptr->get_queue_families() );
        }
    }
    #endif

    for (auto physical_device_ptr : physical_device_ptrs)
    {
        ParentPhysicalDeviceProperties device_props;

        device_props.physical_device_ptr = physical_device_ptr;

        m_parent_physical_devices.push_back    (device_props);
        m_parent_physical_devices_vec.push_back(physical_device_ptr);
    }

    for (uint32_t n_physical_device = 0;
                  n_physical_device < static_cast<uint32_t>(m_parent_physical_devices.size() );
                ++n_physical_device)
    {
        const auto& physical_device_info = m_parent_physical_devices.at(n_physical_device);

        anvil_assert(m_device_index_to_physical_device_props.find(n_physical_device) == m_device_index_to_physical_device_props.end() );
        m_device_index_to_physical_device_props[n_physical_device] = &physical_device_info;
    }
}

/* Please see header for specification */
Anvil::MGPUDevice::~MGPUDevice()
{
    /* Stub */
}

/* Please see header for specification */
Anvil::BaseDeviceUniquePtr Anvil::MGPUDevice::create(Anvil::DeviceCreateInfoUniquePtr in_create_info_ptr)
{
    Anvil::BaseDeviceUniquePtr result_ptr;

    /* Make sure the caller has requested the VK_KHR_device_group and VK_KHR_bind_memory2 extensions */
    #ifdef _DEBUG
    {
        const auto& extension_configuration = in_create_info_ptr->get_extension_configuration();

        anvil_assert(extension_configuration.extension_status.at(VK_KHR_DEVICE_GROUP_EXTENSION_NAME)  == Anvil::ExtensionAvailability::REQUIRE);
        anvil_assert(extension_configuration.extension_status.at(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) == Anvil::ExtensionAvailability::REQUIRE);
    }
    #endif

    result_ptr = Anvil::BaseDeviceUniquePtr(
        new Anvil::MGPUDevice(std::move(in_create_info_ptr) ),
        std::default_delete<Anvil::BaseDevice>()
    );

    if (result_ptr != nullptr)
    {
        dynamic_cast<Anvil::MGPUDevice*>(result_ptr.get() )->init();
    }

    return result_ptr;
}

/** Please see header for specification */
Anvil::SwapchainUniquePtr Anvil::MGPUDevice::create_swapchain(Anvil::RenderingSurface*           in_parent_surface_ptr,
                                                              Anvil::Window*                     in_window_ptr,
                                                              Anvil::Format                      in_image_format,
                                                              Anvil::ColorSpaceKHR               in_color_space,
                                                              Anvil::PresentModeKHR              in_present_mode,
                                                              Anvil::ImageUsageFlags             in_usage,
                                                              uint32_t                           in_n_swapchain_images,
                                                              bool                               in_support_SFR,
                                                              Anvil::DeviceGroupPresentModeFlags in_presentation_modes_to_support)
{
    SwapchainUniquePtr result_ptr(nullptr,
                                  std::default_delete<Anvil::Swapchain>() );

    {
        auto swapchain_create_info_ptr = Anvil::SwapchainCreateInfo::create(this,
                                                                            in_parent_surface_ptr,
                                                                            in_window_ptr,
                                                                            in_image_format,
                                                                            in_color_space,
                                                                            in_present_mode,
                                                                            in_usage,
                                                                            in_n_swapchain_images);

        swapchain_create_info_ptr->set_mt_safety              (Anvil::MTSafety::ENABLED);
        swapchain_create_info_ptr->set_mgpu_present_mode_flags(in_presentation_modes_to_support);

        if (in_support_SFR)
        {
            swapchain_create_info_ptr->set_flags(Anvil::SwapchainCreateFlagBits::SPLIT_INSTANCE_BIND_REGIONS_BIT);
        }

        result_ptr = Anvil::Swapchain::create(std::move(swapchain_create_info_ptr) );
    }

    anvil_assert(result_ptr != nullptr);

    return result_ptr;
}

/** Please see header for specification */
bool Anvil::MGPUDevice::get_peer_memory_features(const Anvil::PhysicalDevice* in_local_physical_device_ptr,
                                                 const Anvil::PhysicalDevice* in_remote_physical_device_ptr,
                                                 uint32_t                     in_memory_heap_index,
                                                 PeerMemoryFeatureFlags*      out_result_ptr) const
{
    const uint32_t&                                                                  local_physical_device_index          = in_local_physical_device_ptr->get_device_group_device_index();
    decltype(m_device_index_to_physical_device_props)::const_iterator                local_physical_device_props_iterator;
    ParentPhysicalDeviceProperties::HeapIndexToPeerMemoryFeaturesMap::const_iterator memory_heap_iterator;
    const uint32_t&                                                                  remote_physical_device_index         = in_remote_physical_device_ptr->get_device_group_device_index();
    decltype(ParentPhysicalDeviceProperties::peer_memory_features)::const_iterator   remote_physical_device_iterator;
    bool                                                                             result                               = false;

    if (in_local_physical_device_ptr == in_remote_physical_device_ptr)
    {
        anvil_assert(in_local_physical_device_ptr != in_remote_physical_device_ptr);

        goto end;
    }

    /* Retrieve cached peer memory features data for the local phys dev + remote phys dev + heap index tuple */
    local_physical_device_props_iterator = m_device_index_to_physical_device_props.find(local_physical_device_index);

    if (local_physical_device_props_iterator == m_device_index_to_physical_device_props.end() )
    {
        /* The specified local physical device is not a part of this logical device? */
        anvil_assert(local_physical_device_props_iterator != m_device_index_to_physical_device_props.end());

        goto end;
    }


    remote_physical_device_iterator = local_physical_device_props_iterator->second->peer_memory_features.find(remote_physical_device_index);

    if (remote_physical_device_iterator == local_physical_device_props_iterator->second->peer_memory_features.end() )
    {
        /* The specified remote physical device is not a part of this logical device? */
        anvil_assert(remote_physical_device_iterator != local_physical_device_props_iterator->second->peer_memory_features.end() );

        goto end;
    }


    memory_heap_iterator = remote_physical_device_iterator->second.find(in_memory_heap_index);

    if (memory_heap_iterator == remote_physical_device_iterator->second.end() )
    {
        anvil_assert(memory_heap_iterator != remote_physical_device_iterator->second.end());

        goto end;
    }

    /* All done */
    *out_result_ptr = memory_heap_iterator->second;
    result          = true;
end:
    return result;
}

/** Please see header for specification */
const Anvil::PhysicalDevice* Anvil::MGPUDevice::get_physical_device(uint32_t in_n_physical_device) const
{
    anvil_assert(m_parent_physical_devices.size() > in_n_physical_device);

    return m_parent_physical_devices.at(in_n_physical_device).physical_device_ptr;
}

bool Anvil::MGPUDevice::get_physical_device_buffer_properties(const BufferPropertiesQuery& in_query,
                                                              Anvil::BufferProperties*     out_opt_result_ptr) const
{
    /* NOTE: All physical devices within a device group are assumed to report the same format properties */
    return m_parent_physical_devices.at(0).physical_device_ptr->get_buffer_properties(in_query,
                                                                                      out_opt_result_ptr);
}

/** Please see header for specification */
const Anvil::PhysicalDeviceFeatures& Anvil::MGPUDevice::get_physical_device_features() const
{
    /* NOTE: We're assuming here all physical devices within a device group offer the same set of features.
     *
     *       An assertion check to double-check this holds is implemented in the init function.
     */
    return m_parent_physical_devices.at(0).physical_device_ptr->get_device_features();
}

bool Anvil::MGPUDevice::get_physical_device_fence_properties(const FencePropertiesQuery& in_query,
                                                             Anvil::FenceProperties*     out_opt_result_ptr) const
{
    /* NOTE: All physical devices within a device group are assumed to report the same fence properties */
    return m_parent_physical_devices.at(0).physical_device_ptr->get_fence_properties(in_query,
                                                                                     out_opt_result_ptr);
}

/** Please see header for specification */
Anvil::FormatProperties Anvil::MGPUDevice::get_physical_device_format_properties(Anvil::Format in_format) const
{
    /* NOTE: All physical devices within a device group are assumed to report the same format properties */
    return m_parent_physical_devices.at(0).physical_device_ptr->get_format_properties(in_format);
}


/** Please see header for specification */
bool Anvil::MGPUDevice::get_physical_device_image_format_properties(const ImageFormatPropertiesQuery& in_query,
                                                                    Anvil::ImageFormatProperties*     out_opt_result_ptr) const
{
    /* NOTE: All physical devices within a device group are assumed to report the same image format properties */
    return m_parent_physical_devices.at(0).physical_device_ptr->get_image_format_properties(in_query,
                                                                                            out_opt_result_ptr);
}

/** Please see header for specification */
const Anvil::MemoryProperties& Anvil::MGPUDevice::get_physical_device_memory_properties() const
{
    /* NOTE: We're assuming here all physical devices within a device group have the same memory caps.
     *
     *       An assertion check to double-check this holds is implemented in the init function.
     */
    return m_parent_physical_devices.at(0).physical_device_ptr->get_memory_properties();
}

Anvil::MultisamplePropertiesEXT Anvil::MGPUDevice::get_physical_device_multisample_properties(const Anvil::SampleCountFlagBits& in_samples) const
{
    /* NOTE: We're assuming here all physical devices within a device group have the same memory caps. */
    VkMultisamplePropertiesEXT result_vk;

    anvil_assert(is_extension_enabled(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME) );

    result_vk.maxSampleLocationGridSize.height = 0;
    result_vk.maxSampleLocationGridSize.width  = 0;
    result_vk.pNext                            = nullptr;
    result_vk.sType                            = VK_STRUCTURE_TYPE_MULTISAMPLE_PROPERTIES_EXT;

    m_ext_sample_locations_extension_entrypoints.vkGetPhysicalDeviceMultisamplePropertiesEXT(m_parent_physical_devices.at(0).physical_device_ptr->get_physical_device(),
                                                                                             static_cast<VkSampleCountFlagBits>(in_samples),
                                                                                            &result_vk);

    return Anvil::MultisamplePropertiesEXT(result_vk);
}

/* Please see header for specification */
const Anvil::PhysicalDeviceProperties& Anvil::MGPUDevice::get_physical_device_properties() const
{
    /* NOTE: We're assuming here all physical devices within a device group have the same properties.
     *
     *       An assertion check to double-check this holds is implemented in the init function.
     */
    return m_parent_physical_devices.at(0).physical_device_ptr->get_device_properties();
}

/* Please see header for specification */
const Anvil::QueueFamilyInfoItems& Anvil::MGPUDevice::get_physical_device_queue_families() const
{
    /* NOTE: We're assuming here all physical devices within a device group offer the same set of queue families.
     *
     *       An assertion check to double-check this holds is implemented in the init function.
     */
    return m_parent_physical_devices.at(0).physical_device_ptr->get_queue_families();
}

bool Anvil::MGPUDevice::get_physical_device_semaphore_properties(const SemaphorePropertiesQuery& in_query,
                                                                 Anvil::SemaphoreProperties*     out_opt_result_ptr) const
{
    /* NOTE: All physical devices within a device group are assumed to report the same semaphore properties */
    return m_parent_physical_devices.at(0).physical_device_ptr->get_semaphore_properties(in_query,
                                                                                         out_opt_result_ptr);
}

/** Please see header for specification */
bool Anvil::MGPUDevice::get_physical_device_sparse_image_format_properties(Anvil::Format                                    in_format,
                                                                           Anvil::ImageType                                 in_type,
                                                                           Anvil::SampleCountFlagBits                       in_sample_count,
                                                                           Anvil::ImageUsageFlags                           in_usage,
                                                                           Anvil::ImageTiling                               in_tiling,
                                                                           std::vector<Anvil::SparseImageFormatProperties>& out_result) const
{
    /* NOTE: All physical devices within a device group are assumed to report the same sparse image format properties */
    uint32_t n_props = 0;

    Anvil::Vulkan::vkGetPhysicalDeviceSparseImageFormatProperties(m_parent_physical_devices.at(0).physical_device_ptr->get_physical_device(),
                                                                  static_cast<VkFormat>             (in_format),
                                                                  static_cast<VkImageType>          (in_type),
                                                                  static_cast<VkSampleCountFlagBits>(in_sample_count),
                                                                  in_usage.get_vk(),
                                                                  static_cast<VkImageTiling>(in_tiling),
                                                                 &n_props,
                                                                  nullptr); /* pProperties */

    if (n_props > 0)
    {
        out_result.resize(n_props);

        Anvil::Vulkan::vkGetPhysicalDeviceSparseImageFormatProperties(m_parent_physical_devices.at(0).physical_device_ptr->get_physical_device(),
                                                                      static_cast<VkFormat>             (in_format),
                                                                      static_cast<VkImageType>          (in_type),
                                                                      static_cast<VkSampleCountFlagBits>(in_sample_count),
                                                                      in_usage.get_vk(),
                                                                      static_cast<VkImageTiling>(in_tiling),
                                                                     &n_props,
                                                                      reinterpret_cast<VkSparseImageFormatProperties*>(&out_result[0] ));
    }

    return true;
}

/* Please see header for specification */
bool Anvil::MGPUDevice::get_physical_device_surface_capabilities(Anvil::RenderingSurface*    in_surface_ptr,
                                                                 Anvil::SurfaceCapabilities* out_result_ptr) const
{
    /* NOTE: All physical devices within a device group are assumed to report the same sparse image format properties */
    return (m_khr_surface_extension_entrypoints.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_parent_physical_devices.at(0).physical_device_ptr->get_physical_device(),
                                                                                          in_surface_ptr->get_surface(),
                                                                                          reinterpret_cast<VkSurfaceCapabilitiesKHR*>(out_result_ptr) ) == VK_SUCCESS);
}

/* Please see header for specification */
bool Anvil::MGPUDevice::get_present_compatible_physical_devices(uint32_t                                          in_device_index,
                                                                const std::vector<const Anvil::PhysicalDevice*>** out_result_ptr) const
{
    bool result = false;

    if (m_parent_physical_devices.size() > in_device_index)
    {
        result = true;

        *out_result_ptr = &m_parent_physical_devices.at(in_device_index).presentation_compatible_physical_devices;
    }
    else
    {
        anvil_assert(m_parent_physical_devices.size() > in_device_index);
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MGPUDevice::get_present_rectangles(uint32_t                       in_device_index,
                                               const Anvil::RenderingSurface* in_rendering_surface_ptr,
                                               std::vector<VkRect2D>*         out_result_ptr) const
{
    uint32_t                     n_rectangles        = 0;
    const Anvil::PhysicalDevice* physical_device_ptr = nullptr;
    bool                         result              = false;
    VkResult                     result_vk;

    if (m_parent_physical_devices.size() <= in_device_index)
    {
        anvil_assert(!(m_parent_physical_devices.size() <= in_device_index) );

        goto end;
    }
    else
    {
        physical_device_ptr = m_parent_physical_devices.at(in_device_index).physical_device_ptr;
    }

    in_rendering_surface_ptr->lock();
    {
        result_vk = m_khr_device_group_extension_entrypoints.vkGetPhysicalDevicePresentRectanglesKHR(physical_device_ptr->get_physical_device(),
                                                                                                     in_rendering_surface_ptr->get_surface(),
                                                                                                    &n_rectangles,
                                                                                                     nullptr); /* pRects */

        if (is_vk_call_successful(result_vk) )
        {
            out_result_ptr->resize(n_rectangles);

            result_vk = m_khr_device_group_extension_entrypoints.vkGetPhysicalDevicePresentRectanglesKHR(physical_device_ptr->get_physical_device(),
                                                                                                         in_rendering_surface_ptr->get_surface(),
                                                                                                        &n_rectangles,
                                                                                                        &((*out_result_ptr)[0])); /* pRects */
        }
    }
    in_rendering_surface_ptr->unlock();

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert(is_vk_call_successful(result_vk));

        goto end;
    }

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
void Anvil::MGPUDevice::get_queue_family_indices(DeviceQueueFamilyInfo* out_device_queue_family_info_ptr) const
{
    /* Assuming all physical devices within a device group offer the same set of queue families and queues,
     * and that the ordering of queue families matches across all those physical devices, we're good to
     * specify any physical device we're going to use to set up a multi-GPU VkDevice instance.
     */
    get_queue_family_indices_for_physical_device(m_parent_physical_devices.at(0).physical_device_ptr,
                                                 out_device_queue_family_info_ptr);
}

/** Please see header for specification */
const Anvil::QueueFamilyInfo* Anvil::MGPUDevice::get_queue_family_info(uint32_t in_queue_family_index) const
{
    /* NOTE: It is assumed here that all physical devices within a device group have a matching set of queue families */
    const auto                    physical_device_ptr(m_parent_physical_devices.at(0).physical_device_ptr);
    const auto&                   queue_fams         (physical_device_ptr->get_queue_families() );
    const Anvil::QueueFamilyInfo* result_ptr         (nullptr);

    if (queue_fams.size() > in_queue_family_index)
    {
        result_ptr = &queue_fams.at(in_queue_family_index);
    }

    return result_ptr;
}

/** Please see header for specification */
Anvil::DeviceGroupPresentModeFlags Anvil::MGPUDevice::get_supported_present_modes_for_surface(const Anvil::RenderingSurface* in_surface_ptr) const
{
    VkDeviceGroupPresentModeFlagsKHR result_flags = 0;
    VkResult                         result_vk    = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    result_vk = m_khr_device_group_extension_entrypoints.vkGetDeviceGroupSurfacePresentModesKHR(m_device,
                                                                                                in_surface_ptr->get_surface(),
                                                                                               &result_flags);
    anvil_assert_vk_call_succeeded(result_vk);

    return Anvil::DeviceGroupPresentModeFlags(static_cast<Anvil::DeviceGroupPresentModeFlagBits>(result_flags) );
}

/** Please see header for specification */
void Anvil::MGPUDevice::init_device()
{
    VkResult result(VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Extract present capabilities of each physical device in the device group */
    VkDeviceGroupPresentCapabilitiesKHR present_caps;

    present_caps.pNext = nullptr;
    present_caps.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_CAPABILITIES_KHR;

    result = m_khr_device_group_extension_entrypoints.vkGetDeviceGroupPresentCapabilitiesKHR(m_device,
                                                                                            &present_caps);

    anvil_assert_vk_call_succeeded(result);
    anvil_assert                  ((present_caps.modes & static_cast<uint32_t>(Anvil::DeviceGroupPresentModeFlagBits::LOCAL_BIT_KHR) ));

    m_supported_present_modes = Anvil::DeviceGroupPresentModeFlags(static_cast<Anvil::DeviceGroupPresentModeFlagBits>(present_caps.modes) );

    for (uint32_t n_physical_device = 0;
                  n_physical_device < static_cast<uint32_t>(m_parent_physical_devices.size() );
                ++n_physical_device)
    {
        auto& physical_device_props = m_parent_physical_devices.at(n_physical_device);

        anvil_assert( present_caps.presentMask[n_physical_device] == 0                                                                                    ||
                     (present_caps.presentMask[n_physical_device] != 0 && (present_caps.presentMask[n_physical_device] & (1 << n_physical_device) ) != 0));

        for (uint32_t n_sub_physical_device = 0;
                      n_sub_physical_device < static_cast<uint32_t>(m_parent_physical_devices.size() );
                    ++n_sub_physical_device)
        {
            if ((present_caps.presentMask[n_physical_device] & (1 << n_sub_physical_device) ) != 0)
            {
                const Anvil::PhysicalDevice* sub_physical_device_ptr(m_parent_physical_devices.at(n_sub_physical_device).physical_device_ptr);

                physical_device_props.presentation_compatible_physical_devices.push_back(sub_physical_device_ptr);
            }
        }
    }

    /* Extract & cache peer memory features information */
    for (uint32_t n_src_physical_device = 0;
                  n_src_physical_device < static_cast<uint32_t>(m_parent_physical_devices.size() );
                ++n_src_physical_device)
    {
        auto&          src_physical_device_info = m_parent_physical_devices.at(n_src_physical_device);
        const auto&    src_physical_device_ptr  = src_physical_device_info.physical_device_ptr;
        const uint32_t n_src_memory_heaps       = src_physical_device_ptr->get_memory_properties().n_heaps;

        for (uint32_t n_dst_physical_device = 0;
                      n_dst_physical_device < static_cast<uint32_t>(m_parent_physical_devices.size() );
                    ++n_dst_physical_device)
        {
            const auto&    dst_physical_device_ptr = m_parent_physical_devices.at(n_dst_physical_device).physical_device_ptr;
            const uint32_t n_dst_memory_heaps      = dst_physical_device_ptr->get_memory_properties().n_heaps;

            ANVIL_REDUNDANT_VARIABLE_CONST(n_dst_memory_heaps);

            if (n_src_physical_device == n_dst_physical_device)
            {
                continue;
            }

            anvil_assert(n_src_memory_heaps == n_dst_memory_heaps);

            for (uint32_t n_heap = 0;
                          n_heap < n_src_memory_heaps;
                        ++n_heap)
            {
                const uint32_t              dst_physical_device_index = dst_physical_device_ptr->get_device_group_device_index();
                VkPeerMemoryFeatureFlagsKHR memory_features           = 0;
                const uint32_t              src_physical_device_index = src_physical_device_ptr->get_device_group_device_index();

                m_khr_device_group_extension_entrypoints.vkGetDeviceGroupPeerMemoryFeaturesKHR(m_device,
                                                                                               n_heap,
                                                                                               src_physical_device_index,
                                                                                               dst_physical_device_index,
                                                                                              &memory_features);

                anvil_assert( (memory_features & VK_PEER_MEMORY_FEATURE_COPY_DST_BIT_KHR) != 0); /* As per spec */

                src_physical_device_info.peer_memory_features[dst_physical_device_index][n_heap] = static_cast<Anvil::PeerMemoryFeatureFlagBits>(memory_features);
            }
        }
    }

    /* Cache info whether this logical device supports subset allocations or not */
    const Anvil::PhysicalDevice* ref_physical_device_ptr = m_parent_physical_devices.at(0).physical_device_ptr;
    const Anvil::Instance*       instance_ptr            = ref_physical_device_ptr->get_instance();
    const uint32_t               device_group_index      = ref_physical_device_ptr->get_device_group_index();

    m_supports_subset_allocations = instance_ptr->get_physical_device_group(device_group_index).supports_subset_allocations;
}

/** Please see header for specification */
bool Anvil::MGPUDevice::is_layer_supported(const std::string& in_layer_name) const
{
    /* Note: All physical devices within a device group must support exactly the same layers. */
    return m_parent_physical_devices.at(0).physical_device_ptr->is_layer_supported(in_layer_name);
}

/** Please see header for specification */
bool Anvil::MGPUDevice::is_physical_device_extension_supported(const std::string& in_extension_name) const
{
    /* Note: All physical devices within a device group must support exactly the same device extensions. */
    return m_parent_physical_devices.at(0).physical_device_ptr->is_device_extension_supported(in_extension_name);
}


/* Please see header for specification */
Anvil::SGPUDevice::SGPUDevice(Anvil::DeviceCreateInfoUniquePtr in_create_info_ptr)
    :BaseDevice(std::move(in_create_info_ptr) )
{
    /* Stub */
}

/* Please see header for specification */
Anvil::SGPUDevice::~SGPUDevice()
{
    /* Stub */
}


/* Please see header for specification */
Anvil::BaseDeviceUniquePtr Anvil::SGPUDevice::create(Anvil::DeviceCreateInfoUniquePtr in_create_info_ptr)
{
    BaseDeviceUniquePtr result_ptr(nullptr,
                                   std::default_delete<Anvil::BaseDevice>() );

    result_ptr = std::unique_ptr<Anvil::BaseDevice>(
        new Anvil::SGPUDevice(std::move(in_create_info_ptr) )
    );

    dynamic_cast<Anvil::SGPUDevice*>(result_ptr.get() )->init();

    return result_ptr;
}

/** Please see header for specification */
Anvil::SwapchainUniquePtr Anvil::SGPUDevice::create_swapchain(Anvil::RenderingSurface* in_parent_surface_ptr,
                                                              Anvil::Window*           in_window_ptr,
                                                              Anvil::Format            in_image_format,
                                                              Anvil::ColorSpaceKHR     in_color_space,
                                                              Anvil::PresentModeKHR    in_present_mode,
                                                              Anvil::ImageUsageFlags   in_usage,
                                                              uint32_t                 in_n_swapchain_images)
{
    SwapchainUniquePtr result_ptr(nullptr,
                                  std::default_delete<Anvil::Swapchain>() );

    {
        auto create_info_ptr = Anvil::SwapchainCreateInfo::create(this,
                                                                  in_parent_surface_ptr,
                                                                  in_window_ptr,
                                                                  in_image_format,
                                                                  in_color_space,
                                                                  in_present_mode,
                                                                  in_usage,
                                                                  in_n_swapchain_images);

        create_info_ptr->set_mt_safety(Anvil::MTSafety::ENABLED);

        result_ptr = Anvil::Swapchain::create(std::move(create_info_ptr) );
    }

    return result_ptr;
}

bool Anvil::SGPUDevice::get_physical_device_buffer_properties(const BufferPropertiesQuery& in_query,
                                                              Anvil::BufferProperties*     out_opt_result_ptr) const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->get_buffer_properties(in_query,
                                                                                      out_opt_result_ptr);
}

/** Please see header for specification */
const Anvil::PhysicalDeviceFeatures& Anvil::SGPUDevice::get_physical_device_features() const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->get_device_features();
}

/** Please see header for specification */
bool Anvil::SGPUDevice::get_physical_device_fence_properties(const FencePropertiesQuery& in_query,
                                                             Anvil::FenceProperties*     out_opt_result_ptr) const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->get_fence_properties(in_query,
                                                                                     out_opt_result_ptr);
}

/** Please see header for specification */
Anvil::FormatProperties Anvil::SGPUDevice::get_physical_device_format_properties(Anvil::Format in_format) const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->get_format_properties(in_format);
}

/** Please see header for specification */
bool Anvil::SGPUDevice::get_physical_device_image_format_properties(const ImageFormatPropertiesQuery& in_query,
                                                                    Anvil::ImageFormatProperties*     out_opt_result_ptr) const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->get_image_format_properties(in_query,
                                                                                            out_opt_result_ptr);
}

/** Please see header for specification */
Anvil::MultisamplePropertiesEXT Anvil::SGPUDevice::get_physical_device_multisample_properties(const Anvil::SampleCountFlagBits& in_samples) const
{
    VkMultisamplePropertiesEXT result_vk;

    anvil_assert(is_extension_enabled(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME) );

    result_vk.maxSampleLocationGridSize.height = 0;
    result_vk.maxSampleLocationGridSize.width  = 0;
    result_vk.pNext                            = nullptr;
    result_vk.sType                            = VK_STRUCTURE_TYPE_MULTISAMPLE_PROPERTIES_EXT;

    m_ext_sample_locations_extension_entrypoints.vkGetPhysicalDeviceMultisamplePropertiesEXT(m_create_info_ptr->get_physical_device_ptrs().at(0)->get_physical_device(),
                                                                                             static_cast<VkSampleCountFlagBits>(in_samples),
                                                                                            &result_vk);

    return Anvil::MultisamplePropertiesEXT(result_vk);
}

/** Please see header for specification */
const Anvil::MemoryProperties& Anvil::SGPUDevice::get_physical_device_memory_properties() const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->get_memory_properties();
}

/** Please see header for specification */
const Anvil::PhysicalDeviceProperties& Anvil::SGPUDevice::get_physical_device_properties() const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->get_device_properties();
}

/** Please see header for specification */
const Anvil::QueueFamilyInfoItems& Anvil::SGPUDevice::get_physical_device_queue_families() const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->get_queue_families();
}

/** Please see header for specification */
bool Anvil::SGPUDevice::get_physical_device_semaphore_properties(const SemaphorePropertiesQuery& in_query,
                                                                 Anvil::SemaphoreProperties*     out_opt_result_ptr) const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->get_semaphore_properties(in_query,
                                                                                         out_opt_result_ptr);
}

/** Please see header for specification */
bool Anvil::SGPUDevice::get_physical_device_sparse_image_format_properties(Anvil::Format                                    in_format,
                                                                           Anvil::ImageType                                 in_type,
                                                                           Anvil::SampleCountFlagBits                       in_sample_count,
                                                                           Anvil::ImageUsageFlags                           in_usage,
                                                                           Anvil::ImageTiling                               in_tiling,
                                                                           std::vector<Anvil::SparseImageFormatProperties>& out_result) const
{
    uint32_t n_props = 0;

    Anvil::Vulkan::vkGetPhysicalDeviceSparseImageFormatProperties(m_create_info_ptr->get_physical_device_ptrs().at(0)->get_physical_device(),
                                                                  static_cast<VkFormat>             (in_format),
                                                                  static_cast<VkImageType>          (in_type),
                                                                  static_cast<VkSampleCountFlagBits>(in_sample_count),
                                                                  in_usage.get_vk(),
                                                                  static_cast<VkImageTiling>(in_tiling),
                                                                 &n_props,
                                                                  nullptr); /* pProperties */

    if (n_props > 0)
    {
        out_result.resize(n_props);

        Anvil::Vulkan::vkGetPhysicalDeviceSparseImageFormatProperties(m_create_info_ptr->get_physical_device_ptrs().at(0)->get_physical_device(),
                                                                      static_cast<VkFormat>             (in_format),
                                                                      static_cast<VkImageType>          (in_type),
                                                                      static_cast<VkSampleCountFlagBits>(in_sample_count),
                                                                      in_usage.get_vk(),
                                                                      static_cast<VkImageTiling>(in_tiling),
                                                                     &n_props,
                                                                      reinterpret_cast<VkSparseImageFormatProperties*>(&out_result[0]) );
    }

    return true;
}

/* Please see header for specification */
bool Anvil::SGPUDevice::get_physical_device_surface_capabilities(Anvil::RenderingSurface*    in_surface_ptr,
                                                                 Anvil::SurfaceCapabilities* out_result_ptr) const
{
    return (m_khr_surface_extension_entrypoints.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_create_info_ptr->get_physical_device_ptrs().at(0)->get_physical_device(),
                                                                                          in_surface_ptr->get_surface(),
                                                                                          reinterpret_cast<VkSurfaceCapabilitiesKHR*>(out_result_ptr) ) == VK_SUCCESS);
}

/* Please see header for specification */
void Anvil::SGPUDevice::get_queue_family_indices(DeviceQueueFamilyInfo* out_device_queue_family_info_ptr) const
{
    get_queue_family_indices_for_physical_device(m_create_info_ptr->get_physical_device_ptrs().at(0),
                                                 out_device_queue_family_info_ptr);
}

/** Please see header for specification */
const Anvil::QueueFamilyInfo* Anvil::SGPUDevice::get_queue_family_info(uint32_t in_queue_family_index) const
{
    const auto&                   queue_fams(m_create_info_ptr->get_physical_device_ptrs().at(0)->get_queue_families() );
    const Anvil::QueueFamilyInfo* result_ptr(nullptr);

    if (queue_fams.size() > in_queue_family_index)
    {
        result_ptr = &queue_fams.at(in_queue_family_index);
    }

    return result_ptr;
}

void Anvil::SGPUDevice::init_device()
{
    /* Stub */
}

/** Please see header for specification */
bool Anvil::SGPUDevice::is_layer_supported(const std::string& in_layer_name) const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->is_layer_supported(in_layer_name);
}

/** Please see header for specification */
bool Anvil::SGPUDevice::is_physical_device_extension_supported(const std::string& in_extension_name) const
{
    return m_create_info_ptr->get_physical_device_ptrs().at(0)->is_device_extension_supported(in_extension_name);
}
