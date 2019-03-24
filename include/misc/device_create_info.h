//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef MISC_DEVICE_CREATE_INFO_H
#define MISC_DEVICE_CREATE_INFO_H

#include "misc/types.h"
#include "misc/extensions.h"
#include <unordered_map>

namespace Anvil
{
    class DeviceCreateInfo
    {
    public:
        /* Public functions */

        /* TODO.
         *
         * Anvil creates one command pool per each queue family which apps can use at any time which is why the CommandPoolCreateFlags argument
         * is present.
         *
         * By default, the device will be created with API version equal to min(instance-level API version, physical device API version}.
         * This can be overridden by calling set_api_version().
         *
         * NOTE: If VK_EXT_global_queue_priority is supported, all queues are associated MEDIUM_EXT global priority by default.
         *       This can be changed on a per-queue basis by calling set_queue_global_priority() prior to passing the structure
         *       to Anvil::{M, S}GPUDevice for device instantiation.
         *
         * NOTE: By default, all queues are associated with a priority of 0.0 and no create flags. This can be adjusted by calling
         *       corresponding set_queue_..() functions.
         *
         *  @param in_physical_device_ptr                      Physical device to create this device from. Must not be nullptr.
         *  @param in_extension_configuration                  Tells which extensions must/should be specified at creation time.
         *  @param in_layers_to_enable                         A vector of layer names to be used when creating the device.
         *                                                     Can be empty.
         *  @param in_transient_command_buffer_allocs_only     True if the command pools, which are going to be initialized after
         *                                                     the device is created, should be set up for transient command buffer
         *                                                     support.
         *  @param in_support_resettable_command_buffer_allocs True if the command pools should be configured for resettable command
         *                                                     buffer support.
         *  @param in_enable_shader_module_cache               True if all spawned shader modules should be cached throughout instance lifetime.
         *                                                     False if they should be released as soon as all shared pointers go out of scope.
         *  @param in_mt_safe                                  True if command buffer creation and queue submissions should be automatically serialized.
         *                                                     Set to false if your app is never going to use more than one thread at a time for
         *                                                     command buffer creation or submission.
         *
         */
        static Anvil::DeviceCreateInfoUniquePtr create_mgpu(const std::vector<const Anvil::PhysicalDevice*>& in_physical_device_ptrs,
                                                            const bool&                                      in_enable_shader_module_cache,
                                                            const DeviceExtensionConfiguration&              in_extension_configuration,
                                                            const std::vector<std::string>&                  in_layers_to_enable,
                                                            const Anvil::CommandPoolCreateFlags&             in_helper_command_pool_create_flags,
                                                            const bool&                                      in_mt_safe);
        static Anvil::DeviceCreateInfoUniquePtr create_sgpu(const Anvil::PhysicalDevice*                     in_physical_device_ptr,
                                                            const bool&                                      in_enable_shader_module_cache,
                                                            const DeviceExtensionConfiguration&              in_extension_configuration,
                                                            const std::vector<std::string>&                  in_layers_to_enable,
                                                            const Anvil::CommandPoolCreateFlags&             in_helper_command_pool_create_flags,
                                                            const bool&                                      in_mt_safe);

        const DeviceExtensionConfiguration& get_extension_configuration() const
        {
            return m_extension_configuration;
        }

        const Anvil::CommandPoolCreateFlags& get_helper_command_pool_create_flags() const
        {
            return m_helper_command_pool_create_flags;
        }

        const std::vector<std::string>& get_layers_to_enable() const
        {
            return m_layers_to_enable;
        }

        const Anvil::MemoryOverallocationBehavior& get_memory_overallocation_behavior() const
        {
            return m_memory_overallocation_behavior;
        }

        const std::vector<const Anvil::PhysicalDevice*>& get_physical_device_ptrs() const
        {
            return m_physical_device_ptrs;
        }

        Anvil::QueueGlobalPriority get_queue_global_priority(const uint32_t& in_queue_family_index,
                                                             const uint32_t& in_queue_index) const
        {
            auto queue_fam_iterator = m_queue_properties.find(in_queue_family_index);
            auto result             = Anvil::QueueGlobalPriority::MEDIUM_EXT;

            if (queue_fam_iterator != m_queue_properties.end() )
            {
                auto queue_iterator = queue_fam_iterator->second.find(in_queue_index);

                if (queue_iterator != queue_fam_iterator->second.end() )
                {
                    result = queue_iterator->second.global_priority;
                }
            }

            return result;
        }

        bool get_queue_must_support_protected_memory_operations(const uint32_t& in_queue_family_index,
                                                                const uint32_t& in_queue_index) const
        {
            auto queue_fam_iterator = m_queue_properties.find(in_queue_family_index);
            auto result             = false;

            if (queue_fam_iterator != m_queue_properties.end() )
            {
                auto queue_iterator = queue_fam_iterator->second.find(in_queue_index);

                if (queue_iterator != queue_fam_iterator->second.end() )
                {
                    result = queue_iterator->second.is_protected_capable;
                }
            }

            return result;
        }

        float get_queue_priority(const uint32_t& in_queue_family_index,
                                 const uint32_t& in_queue_index) const
        {
            auto queue_fam_iterator = m_queue_properties.find(in_queue_family_index);
            auto result             = 0.0f;

            if (queue_fam_iterator != m_queue_properties.end() )
            {
                auto queue_iterator = queue_fam_iterator->second.find(in_queue_index);

                if (queue_iterator != queue_fam_iterator->second.end() )
                {
                    result = queue_iterator->second.priority;
                }
            }

            return result;
        }

        /* Sets memory overallocation behavior to request at device creation time.
         *
         * NOTE: Requires VK_AMD_memory_overallocation_behavior.
         *
         * @param in_behavior Required behavior to specify at device creation time.
         **/
        void set_memory_overallocation_behavior(const Anvil::MemoryOverallocationBehavior& in_behavior)
        {
            m_memory_overallocation_behavior = in_behavior;
        }

        /* Associates global priority information with a given <queue family index, queue index> pair.
         *
         * NOTE: Requires VK_EXT_global_queue_priority.
         *
         * @param in_queue_family_index    Index of the queue family to use for the association.
         * @param in_queue_index           Index of the queue belonging to queue family with index @param in_queue_family_index
         *                                 to use for the association.
         * @param in_queue_global_priority Global priority to use for the queue.
         */
        void set_queue_global_priority(const uint32_t&                   in_queue_family_index,
                                       const uint32_t&                   in_queue_index,
                                       const Anvil::QueueGlobalPriority& in_queue_global_priority)
        {
            m_queue_properties[in_queue_family_index][in_queue_index].global_priority = in_queue_global_priority;
        }

        /* Associates priority with a given <queue family index, queue index> pair.
         *
         * By default, all queues will be associated a priority of 0.0.
         *
         * NOTE: Apps are required to respect discreteQueuePriorities property of the physical device the device
         *       will be created from!
         *
         * @param in_queue_family_index Index of the queue family to use for the association.
         * @param in_queue_index        Index of the queue belonging to queue family with index @param in_queue_family_index
         *                              to use for the association.
         * @param in_queue_priority     Priority to use for the queue.
         */
        void set_queue_priority(const uint32_t& in_queue_family_index,
                                const uint32_t& in_queue_index,
                                const float&    in_queue_priority)
        {
            m_queue_properties[in_queue_family_index][in_queue_index].priority = in_queue_priority;
        }

        /* Call to specify whether or not VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT should be specified at queue creation time.
         *
         * NOTE: Apps can only request the bit for queues which are reported to support protected memory operations.
         * NOTE: This function is only supported for VK 1.1 devices or newer.
         *
         * @param in_queue_family_index Index of the queue family to use for the association.
         * @param in_queue_index        Index of the queue belonging to queue family with index @param in_queue_family_index
         *                              to use for the association.
         * @param in_should_enable      True to enable the functionality; false to disable.
         */
        void set_queue_must_support_protected_memory_operations(const uint32_t& in_queue_family_index,
                                                                const uint32_t& in_queue_index,
                                                                const bool&     in_should_enable)
        {
            m_queue_properties[in_queue_family_index][in_queue_index].is_protected_capable = in_should_enable;
        }

        const bool& should_be_mt_safe() const
        {
            return m_mt_safe;
        }

        const bool& should_enable_shader_module_cache() const
        {
            return m_should_enable_shader_module_cache;
        }

    private:
        /* Private type definitions */
        typedef struct QueueProperties
        {
            Anvil::QueueGlobalPriority global_priority;
            bool                       is_protected_capable;
            float                      priority;

            QueueProperties()
                :global_priority     (Anvil::QueueGlobalPriority::MEDIUM_EXT),
                 is_protected_capable(false),
                 priority            (0.0f)
            {
                /* Stub */
            }
        } QueueProperties;

        /* Private functions */
        DeviceCreateInfo(const std::vector<const Anvil::PhysicalDevice*>& in_physical_device_ptrs,
                         const bool&                                      in_enable_shader_module_cache,
                         const DeviceExtensionConfiguration&              in_extension_configuration,
                         const std::vector<std::string>&                  in_layers_to_enable,
                         const Anvil::CommandPoolCreateFlags&             in_helper_command_pool_create_flags,
                         const bool&                                      in_mt_safe);

        /* Private variables */
        DeviceExtensionConfiguration                                                 m_extension_configuration;
        Anvil::CommandPoolCreateFlags                                                m_helper_command_pool_create_flags;
        std::vector<std::string>                                                     m_layers_to_enable;
        Anvil::MemoryOverallocationBehavior                                          m_memory_overallocation_behavior;
        bool                                                                         m_mt_safe;
        std::vector<const Anvil::PhysicalDevice*>                                    m_physical_device_ptrs;
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, QueueProperties> > m_queue_properties;
        bool                                                                         m_should_enable_shader_module_cache;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(DeviceCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(DeviceCreateInfo);
    };

}; /* namespace Anvil */

#endif /* MISC_DEVICE_CREATE_INFO_H */