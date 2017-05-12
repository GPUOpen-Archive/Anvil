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

/** Implements a wrapper for a Vulkan device. Implemented in order to:
 *
 *  - manage life-time of device instances.
 *  - encapsulate all logic required to manipulate devices.
 *  - let ObjectTracker detect leaking device instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_DEVICE_H
#define WRAPPERS_DEVICE_H

#include "../misc/debug.h"
#include "../misc/types.h"
#include <algorithm>

namespace Anvil
{
    class BaseDevice : public std::enable_shared_from_this<BaseDevice>
    {
    public:
        /* Public functions */

        /* Constructor */
        BaseDevice(std::weak_ptr<Anvil::Instance> in_parent_instance_ptr);

        /** Releases all children queues and unregisters itself from the owning physical device. */
        virtual void destroy();

        /** Retrieves a command pool, created for the specified queue family type.
         *
         *  @param queue_family_type Queue family to retrieve the command pool for.
         *
         *  @return As per description
         **/
        std::shared_ptr<Anvil::CommandPool> get_command_pool(Anvil::QueueFamilyType queue_family_type) const
        {
            return m_command_pool_ptrs[queue_family_type];
        }

        /** Retrieves a compute pipeline manager, created for this device instance.
         *
         *  @return As per description
         **/
        std::shared_ptr<Anvil::ComputePipelineManager> get_compute_pipeline_manager() const
        {
            return m_compute_pipeline_manager_ptr;
        }

        /** Returns a Queue instance, corresponding to a compute queue at index @param n_queue
         *
         *  @param n_queue Index of the compute queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        std::shared_ptr<Anvil::Queue> get_compute_queue(uint32_t n_queue) const
        {
            std::shared_ptr<Anvil::Queue> result_ptr;

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
        std::shared_ptr<Anvil::DescriptorSet> get_dummy_descriptor_set() const;

        /** Retrieves a DescriptorSetLayout instance, which encapsulates a single descriptor set layout,
         *  which holds 1 descriptor set holding 0 descriptors.
         *
         *  Do NOT release. This object is owned by Device and will be released at object tear-down time.
         **/
        std::shared_ptr<Anvil::DescriptorSetLayout> get_dummy_descriptor_set_layout() const;

        /** Returns a container with entry-points to functions introduced by VK_AMD_draw_indirect_count extension.
         *
         *  Will fire an assertion failure if the extension was not requested at device creation time.
         **/
        const ExtensionAMDDrawIndirectCountEntrypoints& get_extension_amd_draw_indirect_count_entrypoints() const;

        /** Returns a container with entry-points to functions introduced by VK_KHR_swapchain extension.
         *
         *  Will fire an assertion failure if the extension was not requested at device creation time.
         **/
        const ExtensionKHRSwapchainEntrypoints& get_extension_khr_swapchain_entrypoints() const;

        /** Retrieves a graphics pipeline manager, created for this device instance.
         *
         *  @return As per description
         **/
        std::shared_ptr<Anvil::GraphicsPipelineManager> get_graphics_pipeline_manager() const
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
                case QUEUE_FAMILY_TYPE_COMPUTE:           result = static_cast<uint32_t>(m_compute_queues.size() );           break;
                case QUEUE_FAMILY_TYPE_TRANSFER:          result = static_cast<uint32_t>(m_transfer_queues.size() );          break;
                case QUEUE_FAMILY_TYPE_UNIVERSAL:         result = static_cast<uint32_t>(m_universal_queues.size() );         break;

                default:
                {
                    /* Invalid queue family type */
                    anvil_assert(false);
                }
            }

            return result;
        }

        /** Returns the number of sparse binding queues supported by this device.
         *
         *  @return As per description
         **/
        uint32_t get_n_sparse_binding_queues() const
        {
            return static_cast<uint32_t>(m_sparse_binding_queues.size() );
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

        /** Returns Vulkan instance wrapper used to create this device. */
        std::weak_ptr<Anvil::Instance> get_parent_instance() const
        {
            return m_parent_instance_ptr;
        }

        /** Returns features supported by physical device(s) which have been used to instantiate
         *  this logical device instance.
         **/
        virtual const VkPhysicalDeviceFeatures& get_physical_device_features() const = 0;

        /** Returns format properties, as reported by physical device(s) which have been used to
         *  instantiate this logical device instance.
         *
         *  @param in_format Format to use for the query.
         */
        virtual const Anvil::FormatProperties& get_physical_device_format_properties(VkFormat in_format) const = 0;

        /** Returns image format properties, as reported by physical device(s) which have been used
         *  to instantiate this logical device instance.
         *
         *  @param in_format  Meaning as per Vulkan spec.
         *  @param in_type    Meaning as per Vulkan spec.
         *  @param in_tiling  Meaning as per Vulkan spec.
         *  @param in_usage   Meaning as per Vulkan spec.
         *  @param in_flags   Meaning as per Vulkan spec.
         *  @param out_result If the function returns true, the requested information will be stored
         *                    at this location.
         *
         *  @return true if successful, false otherwise.
         **/
        virtual bool get_physical_device_image_format_properties(VkFormat                 in_format,
                                                                 VkImageType              in_type,
                                                                 VkImageTiling            in_tiling,
                                                                 VkImageUsageFlags        in_usage,
                                                                 VkImageCreateFlags       in_flags,
                                                                 VkImageFormatProperties& out_result) const = 0;

        /** Returns memory properties, as reported by physical device(s) which have been used
         *  to instantiate this logical device instance.
         **/
        virtual const Anvil::MemoryProperties& get_physical_device_memory_properties() const = 0;

        /** Returns general physical device properties, as reported by physical device(s) which have been used
         *  to instantiate this logical device instance.
         **/
        virtual const VkPhysicalDeviceProperties& get_physical_device_properties() const = 0;

        /** Returns queue families available for the physical device(s) used to instantiate this
         *  logical device instance.
         **/
        virtual const QueueFamilyInfoItems& get_physical_device_queue_families() const = 0;

        /** Returns sparse image format properties, as reported by physical device(s) which have been
         *  used to instantiate this logical device instance.
         *
         *  @param in_format       Meaning as per Vulkan spec.
         *  @param in_type         Meaning as per Vulkan spec.
         *  @param in_sample_count Meaning as per Vulkan spec.
         *  @param in_usage        Meaning as per Vulkan spec.
         *  @param in_tiling       Meaning as per Vulkan spec.
         *  @param out_result      If the function returns true, the requested information will be stored
         *                         at this location.
         *
         *  @return true if successful, false otherwise.
         **/
        virtual bool get_physical_device_sparse_image_format_properties(VkFormat                                    in_format,
                                                                        VkImageType                                 in_type,
                                                                        VkSampleCountFlagBits                       in_sample_count,
                                                                        VkImageUsageFlags                           in_usage,
                                                                        VkImageTiling                               in_tiling,
                                                                        std::vector<VkSparseImageFormatProperties>& out_result) const = 0;

        /** Returns a pipeline cache, created specifically for this device.
         *
         *  @return As per description
         **/
        std::shared_ptr<Anvil::PipelineCache> get_pipeline_cache() const
        {
            return m_pipeline_cache_ptr;
        }

        /** Returns a pipeline layout manager, created specifically for this device.
         *
         *  @return As per description
         **/
        std::shared_ptr<Anvil::PipelineLayoutManager> get_pipeline_layout_manager() const
        {
            return m_pipeline_layout_manager_ptr;
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
        uint32_t get_queue_family_index(Anvil::QueueFamilyType family_type) const
        {
            return m_device_queue_families.family_index[family_type];
        }

        /** Returns detailed queue family information for a queue family at index @param queue_family_index . */
        virtual const Anvil::QueueFamilyInfo* get_queue_family_info(uint32_t queue_family_index) const = 0;

        /** Returns a Queue instance, corresponding to a sparse binding-capable queue at index @param n_queue,
         *  which supports queue family capabilities specified with @param opt_required_queue_flags.
         *
         *  @param n_queue                  Index of the queue to retrieve the wrapper instance for.
         *  @param opt_required_queue_flags Additional queue family bits the returned queue needs to support.
         *                                  0 by default.
         *
         *  @return As per description
         **/
        std::shared_ptr<Anvil::Queue> get_sparse_binding_queue(uint32_t     n_queue,
                                                               VkQueueFlags opt_required_queue_flags = 0) const;

        /** Returns a Queue instance, corresponding to a transfer queue at index @param n_queue
         *
         *  @param n_queue Index of the transfer queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        std::shared_ptr<Anvil::Queue> get_transfer_queue(uint32_t n_queue) const
        {
            std::shared_ptr<Anvil::Queue> result_ptr;

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
        std::shared_ptr<Anvil::Queue> get_universal_queue(uint32_t n_queue) const
        {
            std::shared_ptr<Anvil::Queue> result_ptr;

            if (m_universal_queues.size() > n_queue)
            {
                result_ptr = m_universal_queues[n_queue];
            }

            return result_ptr;
        }

        /* Tells what type this device instance is */
        virtual DeviceType get_type() const = 0;

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

    protected:
        /* Protected type definitions */
        typedef struct
        {
            uint32_t family_index[Anvil::QUEUE_FAMILY_TYPE_COUNT];
            uint32_t n_queues    [Anvil::QUEUE_FAMILY_TYPE_COUNT];
        } DeviceQueueFamilyInfo;

        /* Protected functions */
        virtual ~BaseDevice();

        std::vector<float> get_queue_priorities(const DeviceQueueFamilyInfo*      device_queue_families_ptr);
        void               init                (const std::vector<const char*>&   extensions,
                                                const std::vector<const char*>&   layers,
                                                bool                              transient_command_buffer_allocs_only,
                                                bool                              support_resettable_command_buffer_allocs);

        BaseDevice& operator=(const BaseDevice&);
        BaseDevice           (const BaseDevice&);

        /** Retrieves family indices of compute, DMA, graphics, transfer queue families for the specified physical device.
         *
         *  @param physical_device_ptr              Physical device to use for the query.
         *  @param out_device_queue_family_info_ptr Deref will be used to store the result info. Must not be null.
         *
         **/
        void get_queue_family_indices_for_physical_device(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr,
                                                          DeviceQueueFamilyInfo*               out_device_queue_family_info_ptr) const;

        virtual void create_device                         (const std::vector<const char*>& extensions,
                                                            const std::vector<const char*>& layers,
                                                            const VkPhysicalDeviceFeatures& features,
                                                            DeviceQueueFamilyInfo*          out_queue_families_ptr)       = 0;
        virtual void init_device                           ()                                                             = 0;
        virtual bool is_layer_supported                    (const char*                     layer_name)             const = 0;
        virtual bool is_physical_device_extension_supported(const char*                     extension_name)         const = 0;

        /* Protected variables */
        std::vector<std::shared_ptr<Anvil::Queue> > m_compute_queues;
        DeviceQueueFamilyInfo                       m_device_queue_families;
        std::vector<std::shared_ptr<Anvil::Queue> > m_sparse_binding_queues;
        std::vector<std::shared_ptr<Anvil::Queue> > m_transfer_queues;
        std::vector<std::shared_ptr<Anvil::Queue> > m_universal_queues;

        /* Protected variables */
        bool     m_destroyed;
        VkDevice m_device;

        ExtensionAMDDrawIndirectCountEntrypoints m_amd_draw_indirect_count_extension_entrypoints;
        ExtensionKHRSurfaceEntrypoints           m_khr_surface_entrypoints;
        ExtensionKHRSwapchainEntrypoints         m_khr_swapchain_entrypoints;

    private:
        /* Private variables */
        std::shared_ptr<Anvil::ComputePipelineManager>  m_compute_pipeline_manager_ptr;
        std::shared_ptr<Anvil::DescriptorSetGroup>      m_dummy_dsg_ptr;
        std::vector<std::string>                        m_enabled_extensions;
        std::shared_ptr<Anvil::GraphicsPipelineManager> m_graphics_pipeline_manager_ptr;
        std::weak_ptr<Anvil::Instance>                  m_parent_instance_ptr;
        std::shared_ptr<Anvil::PipelineCache>           m_pipeline_cache_ptr;
        std::shared_ptr<Anvil::PipelineLayoutManager>   m_pipeline_layout_manager_ptr;
        uint32_t                                        m_queue_family_index[Anvil::QUEUE_FAMILY_TYPE_COUNT];

        std::shared_ptr<Anvil::CommandPool> m_command_pool_ptrs[Anvil::QUEUE_FAMILY_TYPE_COUNT];

        friend struct DeviceDeleter;
    };

    /* Implements a logical device wrapper, created from a single physical device */
    class SGPUDevice : public BaseDevice
    {
    public:
        /* Public functions */

        /** Creates a new Vulkan Device instance using a user-specified physical device instance.
         *
         *  To release a device instance, please call destroy().
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
        static std::weak_ptr<Anvil::SGPUDevice> create(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr,
                                                       const std::vector<const char*>&      extensions,
                                                       const std::vector<const char*>&      layers,
                                                       bool                                 transient_command_buffer_allocs_only,
                                                       bool                                 support_resettable_command_buffer_allocs);

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
        std::shared_ptr<Anvil::Swapchain> create_swapchain(std::shared_ptr<Anvil::RenderingSurface> parent_surface_ptr,
                                                           std::shared_ptr<Anvil::Window>           window_ptr,
                                                           VkFormat                                 image_format,
                                                           VkPresentModeKHR                         present_mode,
                                                           VkImageUsageFlags                        usage,
                                                           uint32_t                                 n_swapchain_images);

        /** Releases all children queues and unregisters itself from the owning physical device. */
        virtual void destroy();

        /** Retrieves a PhysicalDevice instance, from which this device instance was created.
         *
         *  @return As per description
         **/
        std::weak_ptr<Anvil::PhysicalDevice> get_physical_device() const
        {
            return m_parent_physical_device_ptr;
        }

        /** See documentation in BaseDevice for more details */
        const VkPhysicalDeviceFeatures& get_physical_device_features() const;

        /** See documentation in BaseDevice for more details */
        const Anvil::FormatProperties& get_physical_device_format_properties(VkFormat in_format) const;

        /** See documentation in BaseDevice for more details */
        bool get_physical_device_image_format_properties(VkFormat                 in_format,
                                                         VkImageType              in_type,
                                                         VkImageTiling            in_tiling,
                                                         VkImageUsageFlags        in_usage,
                                                         VkImageCreateFlags       in_flags,
                                                         VkImageFormatProperties& out_result) const;

        /** See documentation in BaseDevice for more details */
        const Anvil::MemoryProperties& get_physical_device_memory_properties() const;

        /** See documentation in BaseDevice for more details */
        const VkPhysicalDeviceProperties& get_physical_device_properties() const;

        /** See documentation in BaseDevice for more details */
        const QueueFamilyInfoItems& get_physical_device_queue_families() const;

        /** See documentation in BaseDevice for more details */
        bool get_physical_device_sparse_image_format_properties(VkFormat                                    in_format,
                                                                VkImageType                                 in_type,
                                                                VkSampleCountFlagBits                       in_sample_count,
                                                                VkImageUsageFlags                           in_usage,
                                                                VkImageTiling                               in_tiling,
                                                                std::vector<VkSparseImageFormatProperties>& out_result) const;

        /** See documentation in BaseDevice for more details */
        const Anvil::QueueFamilyInfo* get_queue_family_info(uint32_t queue_family_index) const;

        /* Tells what type this device instance is */
        DeviceType get_type() const
        {
            return DEVICE_TYPE_SINGLE_GPU;
        }

    protected:
        /* Protected functions */
        void create_device                         (const std::vector<const char*>& extensions,
                                                    const std::vector<const char*>& layers,
                                                    const VkPhysicalDeviceFeatures& features,
                                                    DeviceQueueFamilyInfo*          out_queue_families_ptr);
        void get_queue_family_indices              (DeviceQueueFamilyInfo*          out_device_queue_family_info_ptr) const;
        void init_device                           ();
        bool is_layer_supported                    (const char*                     layer_name)     const;
        bool is_physical_device_extension_supported(const char*                     extension_name) const;

    private:
        /* Private type definitions */
        struct SGPUDeviceDeleter
        {
            void operator()(SGPUDevice* device_ptr)
            {
                delete device_ptr;
            }
        };

        /* Private functions */
        virtual ~SGPUDevice();

        /** Private constructor. Please use create() instead. */
        SGPUDevice(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr);

        /* Private variables */
        std::weak_ptr<Anvil::PhysicalDevice> m_parent_physical_device_ptr;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_DEVICE_H */