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

/** Implements a wrapper for a single Vulkan device. Implemented in order to:
 *
 *  - manage life-time of device instances.
 *  - encapsulate all logic required to manipulate devices.
 *  - let ObjectTracker detect leaking device instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_DEVICE_H
#define WRAPPERS_DEVICE_H

#include "misc/ref_counter.h"
#include "misc/types.h"
#include <algorithm>

namespace Anvil
{
    class Device : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Creates a new Vulkan Device instance, as well as:
         *
         *  - a command pool for compute, transfer and universal command queues;
         *  - a compute & a graphics pipeline manager;
         *  - a pipeline cache;
         *  - a Queue instance for each command queue supported by the driver;
         *
         *  @param physical_device_ptr                      Physical device to create this device from. Must not be nullptr.
         *  @param extensions                               A vector of extension names to be used when creating the device.
         *                                                  Can be empty.
         *  @param layers                                   A vector of layer names to be used when creating the device.
         *                                                  Can be empty.
         *  @param transient_command_buffer_allocs_only     True if the command pools, which are going to be initialized after
         *                                                  the device is created, should be set up for transient command buffer
         *                                                  support.
         *  @param support_resettable_command_buffer_allocs True if the command pools should be configured for resettable command
         *                                                  buffer support.
         *
         *  @return A new Device instance.
         **/
        Device(Anvil::PhysicalDevice*          physical_device_ptr,
               const std::vector<const char*>& extensions,
               const std::vector<const char*>& layers,
               bool                            transient_command_buffer_allocs_only,
               bool                            support_resettable_command_buffer_allocs);

        /** Creates a new swapchain instance for the device.
         *
         *  @param parent_surface_ptr Rendering surface to create the swapchain for. Must not be nullptr.
         *  @param window_ptr         current window to create the swapchain for. Must not be nullptr.
         *  @param image_format       Format which the swap-chain should use.
         *  @param present_mode       Presentation mode which the swap-chain should use.
         *  @param usage              Image usage flags describing how the swap-chain is going to be used.
         *  @param n_swapchain_images Number of images the swap-chain should use.
         *
         *  @return A new Swapchain instance.
         **/
        Anvil::Swapchain* create_swapchain(Anvil::RenderingSurface* parent_surface_ptr,
                                           Anvil::Window*           window_ptr,
                                           VkFormat                 image_format,
                                           VkPresentModeKHR         present_mode,
                                           VkImageUsageFlags        usage,
                                           uint32_t                 n_swapchain_images);

        /** Retrieves a command pool, created for the specified queue family type.
         *
         *  @param queue_family_type Queue family to retrieve the command pool for.
         *
         *  @return As per description
         **/
        Anvil::CommandPool* get_command_pool(Anvil::QueueFamilyType queue_family_type) const
        {
            return m_command_pool_ptrs[queue_family_type];
        }

        /** Retrieves a compute pipeline manager, created for this device instance.
         *
         *  @return As per description
         **/
        Anvil::ComputePipelineManager* get_compute_pipeline_manager() const
        {
            return m_compute_pipeline_manager_ptr;
        }

        /** Returns a Queue instance, corresponding to a compute queue at index @param n_queue
         *
         *  @param n_queue Index of the compute queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_compute_queue(uint32_t n_queue) const
        {
            Anvil::Queue* result_ptr = nullptr;

            if (m_compute_queues.size() > n_queue)
            {
                result_ptr = m_compute_queues[n_queue];
            }

            return result_ptr;
        }

        /** Retrieves a raw Vulkan handle for this device.
         *
         *  @return As per description
         **/
        VkDevice get_device_vk() const
        {
            return m_device;
        }

        /** Retrieves a DescriptorSet instance which defines 0 descriptors.
         *
         *  Do NOT release. This object is owned by Device and will be released at object tear-down time.
         **/
        Anvil::DescriptorSet* get_dummy_descriptor_set() const;

        /** Retrieves a DescriptorSetLayout instance, which encapsulates a single descriptor set layout,
         *  which holds 1 descriptor set holding 0 descriptors.
         *
         *  Do NOT release. This object is owned by Device and will be released at object tear-down time.
         **/
        Anvil::DescriptorSetLayout* get_dummy_descriptor_set_layout() const;

        /** Retrieves a graphics pipeline manager, created for this device instance.
         *
         *  @return As per description
         **/
        Anvil::GraphicsPipelineManager* get_graphics_pipeline_manager() const
        {
            return m_graphics_pipeline_manager_ptr;
        }

        /** Returns the number of compute queues supported by this device.
         *
         *  @return As per description
         **/
        uint32_t get_n_compute_queues() const
        {
            return static_cast<uint32_t>(m_compute_queues.size() );
        }

        /** Returns the number of queues available for the specified queue family type
         *
         *  @param queue_family_type Queue family type to use for the query.
         *
         *  @return As per description.
         **/
        uint32_t get_n_queues(QueueFamilyType family_type) const
        {
            uint32_t result = 0;

            switch (family_type)
            {
                case QUEUE_FAMILY_TYPE_COMPUTE:   result = static_cast<uint32_t>(m_compute_queues.size() );   break;
                case QUEUE_FAMILY_TYPE_TRANSFER:  result = static_cast<uint32_t>(m_transfer_queues.size() );  break;
                case QUEUE_FAMILY_TYPE_UNIVERSAL: result = static_cast<uint32_t>(m_universal_queues.size() ); break;

                default:
                {
                    /* Invalid queue family type */
                    anvil_assert(false);
                }
            }

            return result;
        }

        /** Returns the number of transfer queues supported by this device.
         *
         *  @return As per description
         **/
        uint32_t get_n_transfer_queues() const
        {
            return static_cast<uint32_t>(m_transfer_queues.size() );
        }

        /** Returns the number of universal queues supported by this device.
         *
         *  @return As per description
         **/
        uint32_t get_n_universal_queues() const
        {
            return static_cast<uint32_t>(m_universal_queues.size() );
        }

        /** Retrieves a PhysicalDevice instance, from which this device instance was created.
         *
         *  @return As per description
         **/
        Anvil::PhysicalDevice* get_physical_device() const
        {
            return m_parent_physical_device_ptr;
        }

        /** Returns a pipeline cache, created specifically for this device.
         *
         *  @return As per description
         **/
        Anvil::PipelineCache* get_pipeline_cache() const
        {
            return m_pipeline_cache_ptr;
        }

        /** Calls the device-specific implementaton of vkGetDeviceProcAddr(), using this device
         *  instance and user-specified arguments.
         *
         *  @param name Func name to use for the query. Must not be nullptr.
         *
         *  @return As per description
         **/
        PFN_vkVoidFunction get_proc_address(const char* name) const
        {
            return vkGetDeviceProcAddr(m_device,
                                       name);
        }

        /** Returns a index of the specified queue family type.
         *
         *  @param queue_family_type
         *
         *  @return The requested queue family index, or -1 if the specified queue family type is
         *          not supported on this device.
         **/
        uint32_t get_queue_family_index(Anvil::QueueFamilyType family_type) const;

        /** Returns a Queue instance, corresponding to a transfer queue at index @param n_queue
         *
         *  @param n_queue Index of the transfer queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_transfer_queue(uint32_t n_queue) const
        {
            Anvil::Queue* result_ptr = nullptr;

            if (m_transfer_queues.size() > n_queue)
            {
                result_ptr = m_transfer_queues[n_queue];
            }

            return result_ptr;
        }

        /** Returns a Queue instance, corresponding to a universal queue at index @param n_queue
         *
         *  @param n_queue Index of the universal queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_universal_queue(uint32_t n_queue) const
        {
            Anvil::Queue* result_ptr = nullptr;

            if (m_universal_queues.size() > n_queue)
            {
                result_ptr = m_universal_queues[n_queue];
            }

            return result_ptr;
        }

        /* Tells whether the device has been created with the specified extension enabled.
         *
         * @param extension_name Null-terminated name of the extension. Must not be null.
         *
         * @return true if the device has been created with the extension enabled, false otherwise.
         **/
        bool is_extension_enabled(const char* extension_name) const
        {
            return std::find(m_enabled_extensions.begin(),
                             m_enabled_extensions.end(),
                             extension_name) != m_enabled_extensions.end();
        }

    private:
        /* Private functions */
        virtual ~Device();

        void get_queue_family_indices(uint32_t* out_compute_queue_family_index_ptr,
                                      uint32_t* out_n_compute_queues_available_ptr,
                                      uint32_t* out_universal_queue_family_index_ptr,
                                      uint32_t* out_n_universal_queues_available_ptr,
                                      uint32_t* out_dma_queue_family_index_ptr,
                                      uint32_t* out_n_dma_queues_available_ptr) const;

        Device& operator=(const Device&);
        Device           (const Device&);

        /* Private variables */
        Anvil::ComputePipelineManager*  m_compute_pipeline_manager_ptr;
        VkDevice                        m_device;
        Anvil::DescriptorSetGroup*      m_dummy_dsg_ptr;
        std::vector<std::string>        m_enabled_extensions;
        Anvil::GraphicsPipelineManager* m_graphics_pipeline_manager_ptr;
        Anvil::PhysicalDevice*          m_parent_physical_device_ptr;
        Anvil::PipelineCache*           m_pipeline_cache_ptr;

        Anvil::CommandPool* m_command_pool_ptrs[Anvil::QUEUE_FAMILY_TYPE_COUNT];

        std::vector<Anvil::Queue*> m_compute_queues;
        std::vector<Anvil::Queue*> m_transfer_queues;
        std::vector<Anvil::Queue*> m_universal_queues;

        /* VK_KHR_device_swapchain function pointers */
        PFN_vkAcquireNextImageKHR   m_vkAcquireNextImageKHR;
        PFN_vkCreateSwapchainKHR    m_vkCreateSwapchainKHR;
        PFN_vkDestroySwapchainKHR   m_vkDestroySwapchainKHR;
        PFN_vkGetSwapchainImagesKHR m_vkGetSwapchainImagesKHR;
        PFN_vkQueuePresentKHR       m_vkQueuePresentKHR;
    };

    /** Delete functor. Useful if you need to wrap the device instance in an auto pointer */
    struct DeviceDeleter
    {
        void operator()(Device* device_ptr) const
        {
            device_ptr->release();
        }
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_DEVICE_H */