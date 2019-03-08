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

/** Implements a wrapper for a Vulkan device. Implemented in order to:
 *
 *  - manage life-time of device instances.
 *  - encapsulate all logic required to manipulate devices.
 *  - let ObjectTracker detect leaking device instances.
 *
 *  The wrapper is thread-safe (on an opt-in basis).
 **/
#ifndef WRAPPERS_DEVICE_H
#define WRAPPERS_DEVICE_H

#include "misc/debug.h"
#include "misc/device_create_info.h"
#include "misc/extensions.h"
#include "misc/mt_safety.h"
#include "misc/struct_chainer.h"
#include "misc/types.h"
#include <algorithm>

namespace Anvil
{
    class BaseDevice : public Anvil::MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /* Constructor */
        BaseDevice(Anvil::DeviceCreateInfoUniquePtr in_create_info_ptr);

        virtual ~BaseDevice();

        /** Retrieves a command pool, created for the specified queue family index.
         *
         *  @param in_vk_queue_family_index Vulkan index of the queue family to return the command pool for.
         *
         *  @return As per description
         **/
        Anvil::CommandPool* get_command_pool_for_queue_family_index(uint32_t in_vk_queue_family_index) const
        {
            return m_command_pool_ptr_per_vk_queue_fam.at(in_vk_queue_family_index).get();
        }

        /** Retrieves a compute pipeline manager, created for this device instance.
         *
         *  @return As per description
         **/
        Anvil::ComputePipelineManager* get_compute_pipeline_manager() const
        {
            return m_compute_pipeline_manager_ptr.get();
        }

        /** Returns a Queue instance, corresponding to a compute queue at index @param in_n_queue
         *
         *  @param in_n_queue Index of the compute queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_compute_queue(uint32_t in_n_queue) const
        {
            Anvil::Queue* result_ptr = nullptr;

            if (m_compute_queues.size() > in_n_queue)
            {
                result_ptr = m_compute_queues.at(in_n_queue);
            }

            return result_ptr;
        }

        const Anvil::DeviceCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        Anvil::DescriptorSetLayoutManager* get_descriptor_set_layout_manager() const
        {
            return m_descriptor_set_layout_manager_ptr.get();
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
        const Anvil::DescriptorSet* get_dummy_descriptor_set() const;

        /** Retrieves a DescriptorSetLayout instance, which encapsulates a single descriptor set layout,
         *  which holds 1 descriptor set holding 0 descriptors.
         *
         *  Do NOT release. This object is owned by Device and will be released at object tear-down time.
         **/
        Anvil::DescriptorSetLayout* get_dummy_descriptor_set_layout() const;

        /** Returns a container with entry-points to functions introduced by VK_AMD_buffer_marker extension.
         *
         *  Will fire an assertion failure if the extension was not requested at device creation time.
         **/
        const ExtensionAMDBufferMarkerEntrypoints& get_extension_amd_buffer_marker_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->amd_buffer_marker() );

            return m_amd_buffer_marker_extension_entrypoints;
        }


        /** Returns a container with entry-points to functions introduced by VK_AMD_draw_indirect_count  extension.
         *
         *  Will fire an assertion failure if the extension was not requested at device creation time.
         **/
        const ExtensionAMDDrawIndirectCountEntrypoints& get_extension_amd_draw_indirect_count_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->amd_draw_indirect_count() );

            return m_amd_draw_indirect_count_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_AMD_shader_info extension.
        *
        *  Will fire an assertion failure if the extension was not requested at device creation time.
        **/
        const ExtensionAMDShaderInfoEntrypoints& get_extension_amd_shader_info_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->amd_shader_info() );

            return m_amd_shader_info_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_EXT_debug_marker extension.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionEXTDebugMarkerEntrypoints& get_extension_ext_debug_marker_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->ext_debug_marker() );

            return m_ext_debug_marker_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_EXT_external_memory_host extension.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionEXTExternalMemoryHostEntrypoints& get_extension_ext_external_memory_host_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->ext_external_memory_host() );

            return m_ext_external_memory_host_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_EXT_hdr_metadata extension.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionEXTHdrMetadataEntrypoints& get_extension_ext_hdr_metadata_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->ext_hdr_metadata() );

            return m_ext_hdr_metadata_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_EXT_sample_locations extension.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionEXTSampleLocationsEntrypoints& get_extension_ext_sample_locations_entrypoints() const;

        /** Returns a container with entry-points to functions introduced by VK_EXT_transform_feedback extension.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionEXTTransformFeedbackEntrypoints& get_extension_ext_transform_feedback_entrypoints() const;

        /** Returns a container with entry-points to functions introduced by VK_KHR_bind_memory2 extension. **/
        const ExtensionKHRBindMemory2Entrypoints& get_extension_khr_bind_memory2_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_bind_memory2() );

            return m_khr_bind_memory2_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_create_renderpass2 extension. **/
        const ExtensionKHRCreateRenderpass2Entrypoints& get_extension_khr_create_renderpass2_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_create_renderpass2() );

            return m_khr_create_renderpass2_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_descriptor_update_template extension. **/
        const ExtensionKHRDescriptorUpdateTemplateEntrypoints& get_extension_khr_descriptor_update_template_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_descriptor_update_template() );

            return m_khr_descriptor_update_template_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_device_group extension. **/
        const ExtensionKHRDeviceGroupEntrypoints& get_extension_khr_device_group_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_device_group() );

            return m_khr_device_group_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_draw_indirect_count extension.
         *
         *  Will fire an assertion failure if the extension was not requested at device creation time.
         **/
        const ExtensionKHRDrawIndirectCountEntrypoints& get_extension_khr_draw_indirect_count_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_draw_indirect_count() );

            return m_khr_draw_indirect_count_extension_entrypoints;
        }

        #if defined(_WIN32)
            const ExtensionKHRExternalFenceWin32Entrypoints& get_extension_khr_external_fence_win32_entrypoints() const
            {
                anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_fence_win32() );

                return m_khr_external_fence_win32_extension_entrypoints;
            }

            const ExtensionKHRExternalMemoryWin32Entrypoints& get_extension_khr_external_memory_win32_entrypoints() const
            {
                anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_memory_win32() );

                return m_khr_external_memory_win32_extension_entrypoints;
            }

            const ExtensionKHRExternalSemaphoreWin32Entrypoints& get_extension_khr_external_semaphore_win32_entrypoints() const
            {
                anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_semaphore_win32() );

                return m_khr_external_semaphore_win32_extension_entrypoints;
            }
        #else
            const ExtensionKHRExternalFenceFdEntrypoints& get_extension_khr_external_fence_fd_entrypoints() const
            {
                anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_fence_fd() );

                return m_khr_external_fence_fd_extension_entrypoints;
            }

            const ExtensionKHRExternalMemoryFdEntrypoints& get_extension_khr_external_memory_fd_entrypoints() const
            {
                anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_memory_fd() );

                return m_khr_external_memory_fd_extension_entrypoints;
            }

            const ExtensionKHRExternalSemaphoreFdEntrypoints& get_extension_khr_external_semaphore_fd_entrypoints() const
            {
                anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_semaphore_fd() );

                return m_khr_external_semaphore_fd_extension_entrypoints;
            }
        #endif

        /** Returns a container with entry-points to functions introduced by VK_KHR_get_memory_requirements2 extension. **/
        const ExtensionKHRGetMemoryRequirements2Entrypoints& get_extension_khr_get_memory_requirements2_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_get_memory_requirements2() );

            return m_khr_get_memory_requirements2_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_maintenance1 extension. **/
        const ExtensionKHRMaintenance1Entrypoints& get_extension_khr_maintenance1_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_maintenance1() );

            return m_khr_maintenance1_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_maintenance3 extension. **/
        const ExtensionKHRMaintenance3Entrypoints& get_extension_khr_maintenance3_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_maintenance3() );

            return m_khr_maintenance3_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_sampler_ycbcr_conversion extension. **/
        const ExtensionKHRSamplerYCbCrConversionEntrypoints& get_extension_khr_sampler_ycbcr_conversion_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_sampler_ycbcr_conversion() );

            return m_khr_sampler_ycbcr_conversion_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_swapchain extension.
         *
         *  Will fire an assertion failure if the extension was not requested at device creation time.
         **/
        const ExtensionKHRSwapchainEntrypoints& get_extension_khr_swapchain_entrypoints() const
        {
            anvil_assert(m_extension_enabled_info_ptr->get_device_extension_info()->khr_swapchain() );

            return m_khr_swapchain_extension_entrypoints;
        }

        /** Retrieves a graphics pipeline manager, created for this device instance.
         *
         *  @return As per description
         **/
        Anvil::GraphicsPipelineManager* get_graphics_pipeline_manager() const
        {
            return m_graphics_pipeline_manager_ptr.get();
        }

        /** Returns the number of compute queues supported by this device.
         *
         *  @return As per description
         **/
        uint32_t get_n_compute_queues() const
        {
            return static_cast<uint32_t>(m_compute_queues.size() );
        }

        /** Returns the number of queues available for the specified queue family index */
        uint32_t get_n_queues(uint32_t in_n_queue_family) const;

        /** Returns the number of queues available for the specified queue family type
         *
         *  @param in_family_type Queue family type to use for the query.
         *
         *  @return As per description.
         **/
        uint32_t get_n_queues(QueueFamilyType in_family_type) const
        {
            uint32_t result = 0;

            switch (in_family_type)
            {
                case Anvil::QueueFamilyType::COMPUTE:   result = static_cast<uint32_t>(m_compute_queues.size() );   break;
                case Anvil::QueueFamilyType::TRANSFER:  result = static_cast<uint32_t>(m_transfer_queues.size() );  break;
                case Anvil::QueueFamilyType::UNIVERSAL: result = static_cast<uint32_t>(m_universal_queues.size() ); break;

                default:
                {
                    /* Invalid queue family type */
                    anvil_assert_fail();
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
        const Anvil::Instance* get_parent_instance() const;

        virtual bool get_physical_device_buffer_properties(const BufferPropertiesQuery& in_query,
                                                           Anvil::BufferProperties*     out_opt_result_ptr = nullptr) const = 0;

        /** Returns features supported by physical device(s) which have been used to instantiate
         *  this logical device instance.
         **/
        virtual const Anvil::PhysicalDeviceFeatures& get_physical_device_features() const = 0;

        virtual bool get_physical_device_fence_properties(const FencePropertiesQuery& in_query,
                                                          Anvil::FenceProperties*     out_opt_result_ptr = nullptr) const = 0;

        virtual Anvil::FormatProperties get_physical_device_format_properties(Anvil::Format in_format) const = 0;

        virtual bool get_physical_device_image_format_properties(const ImageFormatPropertiesQuery& in_query,
                                                                 Anvil::ImageFormatProperties*     out_opt_result_ptr = nullptr) const = 0;

        /** Returns memory properties, as reported by physical device(s) which have been used
         *  to instantiate this logical device instance.
         **/
        virtual const Anvil::MemoryProperties& get_physical_device_memory_properties() const = 0;

        /** Returns multisample properties as reported for physical device(s) which have been used
         *  to instantiate this logical device instance.
         *
         * Requires VK_EXT_sample_locations
         */
        virtual Anvil::MultisamplePropertiesEXT get_physical_device_multisample_properties(const Anvil::SampleCountFlagBits& in_samples) const = 0;

        /** Returns general physical device properties, as reported by physical device(s) which have been used
         *  to instantiate this logical device instance.
         **/
        virtual const Anvil::PhysicalDeviceProperties& get_physical_device_properties() const = 0;

        /** Returns queue families available for the physical device(s) used to instantiate this
         *  logical device instance.
         **/
        virtual const QueueFamilyInfoItems& get_physical_device_queue_families() const = 0;

        virtual bool get_physical_device_semaphore_properties(const SemaphorePropertiesQuery& in_query,
                                                              Anvil::SemaphoreProperties*     out_opt_result_ptr = nullptr) const = 0;

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
        virtual bool get_physical_device_sparse_image_format_properties(Anvil::Format                                    in_format,
                                                                        Anvil::ImageType                                 in_type,
                                                                        Anvil::SampleCountFlagBits                       in_sample_count,
                                                                        ImageUsageFlags                                  in_usage,
                                                                        Anvil::ImageTiling                               in_tiling,
                                                                        std::vector<Anvil::SparseImageFormatProperties>& out_result) const = 0;


        /** Returns surface capabilities, as reported for physical device(s) which have been used to instantiate
         *  this logical device instance.
         **/
        virtual bool get_physical_device_surface_capabilities(Anvil::RenderingSurface*    in_surface_ptr,
                                                              Anvil::SurfaceCapabilities* out_result_ptr) const = 0;

        /** Returns a pipeline cache, created specifically for this device.
         *
         *  @return As per description
         **/
        Anvil::PipelineCache* get_pipeline_cache() const
        {
            return m_pipeline_cache_ptr.get();
        }

        /** Returns a pipeline layout manager, created specifically for this device.
         *
         *  @return As per description
         **/
        Anvil::PipelineLayoutManager* get_pipeline_layout_manager() const
        {
            return m_pipeline_layout_manager_ptr.get();
        }

        /** Calls the device-specific implementaton of vkGetDeviceProcAddr(), using this device
         *  instance and user-specified arguments.
         *
         *  @param in_name Func name to use for the query. Must not be nullptr.
         *
         *  @return As per description
         **/
        PFN_vkVoidFunction get_proc_address(const char* in_name) const
        {
            return Anvil::Vulkan::vkGetDeviceProcAddr(m_device,
                                                      in_name);
        }

        PFN_vkVoidFunction get_proc_address(const std::string& in_name) const
        {
            return get_proc_address(in_name.c_str() );
        }

        /** TODO */
        Anvil::Queue* get_queue(const Anvil::QueueFamilyType& in_queue_family_type,
                                uint32_t                      in_n_queue) const;

        /* Returns a Queue instance representing a Vulkan queue from queue family @param in_n_queue_family
         * at index @param in_n_queue.
         *
         * @param in_n_queue_family See @brief.
         * @param in_n_queue        See @brief.
         *
         * @return Requested Queue instance OR nullptr if either of the parameters is invalid.
         */
        Anvil::Queue* get_queue_for_queue_family_index(uint32_t in_n_queue_family,
                                                       uint32_t in_n_queue) const;

        /** TODO
         *
         *  NOTE: A single Anvil queue family MAY map onto more than one Vulkan queue family type. The same is not true the other way around.
         */
        bool get_queue_family_indices_for_queue_family_type(const Anvil::QueueFamilyType& in_queue_family_type,
                                                            uint32_t*                     out_opt_n_queue_family_indices_ptr,
                                                            const uint32_t**              out_opt_queue_family_indices_ptr_ptr) const;

        /** TODO.
         *
         *  NOTE: Anvil's queue family may map onto more than one Vulkan queue family. The same is not true the other way around.
         */
        Anvil::QueueFamilyType get_queue_family_type(uint32_t in_queue_family_index) const;

        /** Returns detailed queue family information for a queue family at index @param in_queue_family_index . */
        virtual const Anvil::QueueFamilyInfo* get_queue_family_info(uint32_t in_queue_family_index) const = 0;

        /** Returns sample locations used by the physical device for the specified sample count.
         *
         *  NOTE: This function will only report success if the physical device supports standard sample locations.
         *
         *  @param in_sample_count Sample count to use for the query.
         *  @param out_result_ptr  Must not be null. Deref will be cleared and filled with sample locations
         *                         for the specified sample count.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_sample_locations(Anvil::SampleCountFlagBits          in_sample_count,
                                  std::vector<Anvil::SampleLocation>* out_result_ptr) const;

        /** Returns shader module cache instance */
        Anvil::ShaderModuleCache* get_shader_module_cache() const
        {
            return m_shader_module_cache_ptr.get();
        }

        /** Returns a Queue instance, corresponding to a sparse binding-capable queue at index @param in_n_queue,
         *  which supports queue family capabilities specified with @param opt_required_queue_flags.
         *
         *  @param in_n_queue                  Index of the queue to retrieve the wrapper instance for.
         *  @param in_opt_required_queue_flags Additional queue family bits the returned queue needs to support.
         *                                     0 by default.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_sparse_binding_queue(uint32_t          in_n_queue,
                                               Anvil::QueueFlags in_opt_required_queue_flags = Anvil::QueueFlags() ) const;

        /* Tells which memory types can be specified when creating an external memory handle for a Win32 handle @param in_handle
         *
         * For all external memory handle types EXCEPT host pointers:
         * + Requires VK_KHR_external_memory_Fd    under Linux.
         * + Requires VK_KHR_external_memory_win32 under Windows
         *
         * For host pointers:
         * + Requires VK_EXT_external_memory_host.
         *
         * @return true if successful, false otherwise.
         *
         *
         * @param in_external_handle_type            (Host pointers) Must be Anvil::ExternalMemoryHandleTypeFlagBits::HOST_ALLOCATION_BIT_EXT or
         *                                                           Anvil::ExternalMemoryHandleTypeFlagBits::HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT.
         *
         *                                           (Other) (Windows) must be either Anvil::ExternalMemoryHandleTypeFlagBits::OPAQUE_WIN32_BIT or
         *                                                             Anvil::ExternalMemoryHandleTypeFlagBits::WIN32_KMT_BIT.
         *                                                   (Linux)   must be Anvil::ExternalMemoryHandleTypeFlagBits::OPAQUE_FD_BIT.
         *
         * @param out_supported_memory_type_bits_ptr Deref will be set to a set of bits where each index corresponds to support status
         *                                           of a memory type with corresponding index. Must not be null.
         */
        bool get_memory_types_supported_for_external_handle(const Anvil::ExternalMemoryHandleTypeFlagBits& in_external_handle_type,
                                                            ExternalHandleType                             in_handle,
                                                            uint32_t*                                      out_supported_memory_type_bits) const;

        /** Returns a Queue instance, corresponding to a transfer queue at index @param in_n_queue
         *
         *  @param in_n_queue Index of the transfer queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_transfer_queue(uint32_t in_n_queue) const
        {
            Anvil::Queue* result_ptr = nullptr;

            if (m_transfer_queues.size() > in_n_queue)
            {
                result_ptr = m_transfer_queues.at(in_n_queue);
            }

            return result_ptr;
        }

        /** Returns a Queue instance, corresponding to a universal queue at index @param in_n_queue
         *
         *  @param in_n_queue Index of the universal queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_universal_queue(uint32_t in_n_queue) const
        {
            Anvil::Queue* result_ptr = nullptr;

            if (m_universal_queues.size() > in_n_queue)
            {
                result_ptr = m_universal_queues.at(in_n_queue);
            }

            return result_ptr;
        }

        /* Tells what type this device instance is */
        virtual DeviceType get_type() const = 0;

        bool is_compute_queue_family_index(const uint32_t& in_queue_family_index) const;

        const Anvil::IExtensionInfoDevice<bool>* get_extension_info() const
        {
            return m_extension_enabled_info_ptr->get_device_extension_info();
        }

        /* Tells whether the device has been created with the specified extension enabled.
         *
         * @param in_extension_name Null-terminated name of the extension. Must not be null.
         *
         * @return true if the device has been created with the extension enabled, false otherwise.
         **/
        bool is_extension_enabled(const std::string& in_extension_name) const
        {
            anvil_assert(m_extension_enabled_info_ptr != nullptr);

            return m_extension_enabled_info_ptr->get_device_extension_info()->by_name(in_extension_name);
        }

        bool is_transfer_queue_family_index(const uint32_t& in_queue_family_index) const;

        bool is_universal_queue_family_index(const uint32_t& in_queue_family_index) const;

        bool wait_idle() const;

    protected:
        /* Protected type definitions */

        typedef struct DeviceQueueFamilyMemberInfo
        {
            uint32_t family_index;
            uint32_t n_queues;

            DeviceQueueFamilyMemberInfo()
                :family_index(UINT32_MAX),
                 n_queues    (UINT32_MAX)
            {
                /* Stub */
            }

            explicit DeviceQueueFamilyMemberInfo(uint32_t in_family_index,
                                                 uint32_t in_n_queues)
                :family_index(in_family_index),
                 n_queues    (in_n_queues)
            {
                /* Stub */
            }

            bool operator==(const uint32_t& in_family_index) const
            {
                return family_index == in_family_index;
            }
        } DeviceQueueFamilyMemberInfo;

        typedef struct
        {
            uint32_t                                                                    n_total_queues_per_family[static_cast<uint32_t>(Anvil::QueueFamilyType::COUNT)];
            std::map<Anvil::QueueFamilyType, std::vector<DeviceQueueFamilyMemberInfo> > queue_families;
        } DeviceQueueFamilyInfo;

        /* Protected functions */

        void add_physical_device_features_to_chainer(Anvil::StructChainer<VkDeviceCreateInfo>* in_struct_chainer) const;

        void create_device(const std::vector<const char*>& in_extensions,
                           const std::vector<const char*>& in_layers,
                           DeviceQueueFamilyInfo*          out_queue_families_ptr);

        std::vector<float> get_queue_priorities(const QueueFamilyInfo* in_queue_family_info_ptr) const;
        bool               init                ();

        BaseDevice& operator=(const BaseDevice&);
        BaseDevice           (const BaseDevice&);

        /** Retrieves family indices of compute, DMA, graphics, transfer queue families for the specified physical device.
         *
         *  @param in_physical_device_ptr           Physical device to use for the query.
         *  @param out_device_queue_family_info_ptr Deref will be used to store the result info. Must not be null.
         *
         **/
        virtual void get_queue_family_indices_for_physical_device(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                                                  DeviceQueueFamilyInfo*       out_device_queue_family_info_ptr) const;

        virtual void init_device                           ()                                                             = 0;
        virtual bool is_layer_supported                    (const std::string&              in_layer_name)          const = 0;
        virtual bool is_physical_device_extension_supported(const std::string&              in_extension_name)      const = 0;

        /* Protected variables */
        Anvil::DeviceCreateInfoUniquePtr m_create_info_ptr;

        std::vector<Anvil::Queue*> m_compute_queues;
        DeviceQueueFamilyInfo      m_device_queue_families;
        std::vector<Anvil::Queue*> m_sparse_binding_queues;
        std::vector<Anvil::Queue*> m_transfer_queues;
        std::vector<Anvil::Queue*> m_universal_queues;

        std::vector<std::unique_ptr<Anvil::Queue> > m_owned_queues;

        std::map<uint32_t, std::vector<Anvil::QueueFamilyType> >                        m_queue_family_index_to_types;
        std::map<Anvil::QueueFamilyType, std::vector<uint32_t> >                        m_queue_family_type_to_queue_family_indices;
        std::map<uint32_t /* Vulkan queue family index */, std::vector<Anvil::Queue*> > m_queue_ptrs_per_vk_queue_fam;

        /* Protected variables */
        VkDevice m_device;

        ExtensionAMDBufferMarkerEntrypoints               m_amd_buffer_marker_extension_entrypoints;
        ExtensionAMDDrawIndirectCountEntrypoints          m_amd_draw_indirect_count_extension_entrypoints;
        ExtensionAMDShaderInfoEntrypoints                 m_amd_shader_info_extension_entrypoints;
        ExtensionEXTDebugMarkerEntrypoints                m_ext_debug_marker_extension_entrypoints;
        ExtensionEXTExternalMemoryHostEntrypoints         m_ext_external_memory_host_extension_entrypoints;
        ExtensionEXTHdrMetadataEntrypoints                m_ext_hdr_metadata_extension_entrypoints;
        ExtensionEXTSampleLocationsEntrypoints            m_ext_sample_locations_extension_entrypoints;
        ExtensionEXTTransformFeedbackEntrypoints          m_ext_transform_feedback_extension_entrypoints;
        ExtensionKHRBindMemory2Entrypoints                m_khr_bind_memory2_extension_entrypoints;
        ExtensionKHRCreateRenderpass2Entrypoints          m_khr_create_renderpass2_extension_entrypoints;
        ExtensionKHRDescriptorUpdateTemplateEntrypoints   m_khr_descriptor_update_template_extension_entrypoints;
        ExtensionKHRDeviceGroupEntrypoints                m_khr_device_group_extension_entrypoints;
        ExtensionKHRDrawIndirectCountEntrypoints          m_khr_draw_indirect_count_extension_entrypoints;
        ExtensionKHRGetMemoryRequirements2Entrypoints     m_khr_get_memory_requirements2_extension_entrypoints;
        ExtensionKHRMaintenance1Entrypoints               m_khr_maintenance1_extension_entrypoints;
        ExtensionKHRMaintenance3Entrypoints               m_khr_maintenance3_extension_entrypoints;
        ExtensionKHRSamplerYCbCrConversionEntrypoints     m_khr_sampler_ycbcr_conversion_extension_entrypoints;
        ExtensionKHRSurfaceEntrypoints                    m_khr_surface_extension_entrypoints;
        ExtensionKHRSwapchainEntrypoints                  m_khr_swapchain_extension_entrypoints;

        #if defined(_WIN32)
            ExtensionKHRExternalFenceWin32Entrypoints     m_khr_external_fence_win32_extension_entrypoints;
            ExtensionKHRExternalMemoryWin32Entrypoints    m_khr_external_memory_win32_extension_entrypoints;
            ExtensionKHRExternalSemaphoreWin32Entrypoints m_khr_external_semaphore_win32_extension_entrypoints;
        #else
            ExtensionKHRExternalFenceFdEntrypoints        m_khr_external_fence_fd_extension_entrypoints;
            ExtensionKHRExternalMemoryFdEntrypoints       m_khr_external_memory_fd_extension_entrypoints;
            ExtensionKHRExternalSemaphoreFdEntrypoints    m_khr_external_semaphore_fd_extension_entrypoints;
        #endif

    private:
        /* Private functions */
        bool init_dummy_dsg          () const;
        bool init_extension_func_ptrs();

        /* Private variables */


        std::unique_ptr<Anvil::ComputePipelineManager>   m_compute_pipeline_manager_ptr;
        DescriptorSetLayoutManagerUniquePtr              m_descriptor_set_layout_manager_ptr;
        mutable Anvil::DescriptorSetGroupUniquePtr       m_dummy_dsg_ptr;
        mutable std::mutex                               m_dummy_dsg_mutex;
        std::unique_ptr<Anvil::ExtensionInfo<bool> >     m_extension_enabled_info_ptr;
        GraphicsPipelineManagerUniquePtr                 m_graphics_pipeline_manager_ptr;
        PipelineCacheUniquePtr                           m_pipeline_cache_ptr;
        PipelineLayoutManagerUniquePtr                   m_pipeline_layout_manager_ptr;
        Anvil::ShaderModuleCacheUniquePtr                m_shader_module_cache_ptr;

        std::vector<CommandPoolUniquePtr> m_command_pool_ptr_per_vk_queue_fam;

        friend struct DeviceDeleter;
    };

    /* Implements a logical device wrapper, created from a single physical device */
    class SGPUDevice : public BaseDevice
    {
    public:
        /* Public functions */

        virtual ~SGPUDevice();

        /** Creates a new Vulkan Device instance.
         *
         *  @param in_create_info_ptr Create info structure. Must not be nullptr.
         *
         *  @return A new Device instance.
         **/
        static BaseDeviceUniquePtr create(Anvil::DeviceCreateInfoUniquePtr in_create_info_ptr);

        /** Creates a new swapchain instance for the device.
         *
         *  @param in_parent_surface_ptr Rendering surface to create the swapchain for. Must not be nullptr.
         *  @param in_window_ptr         current window to create the swapchain for. Must not be nullptr.
         *  @param in_image_format       Format which the swap-chain should use.
         *  @param in_color_space        TODO
         *  @param in_present_mode       Presentation mode which the swap-chain should use.
         *  @param in_usage              Image usage flags describing how the swap-chain is going to be used.
         *  @param in_n_swapchain_images Number of images the swap-chain should use.
         *
         *  @return A new Swapchain instance.
         **/
        Anvil::SwapchainUniquePtr create_swapchain(Anvil::RenderingSurface* in_parent_surface_ptr,
                                                   Anvil::Window*           in_window_ptr,
                                                   Anvil::Format            in_image_format,
                                                   Anvil::ColorSpaceKHR     in_color_space,
                                                   Anvil::PresentModeKHR    in_present_mode,
                                                   ImageUsageFlags          in_usage,
                                                   uint32_t                 in_n_swapchain_images);

        /** Retrieves a PhysicalDevice instance, from which this device instance was created.
         *
         *  @return As per description
         **/
        const Anvil::PhysicalDevice* get_physical_device() const
        {
            return m_create_info_ptr->get_physical_device_ptrs().at(0);
        }

        bool get_physical_device_buffer_properties(const BufferPropertiesQuery& in_query,
                                                   Anvil::BufferProperties*     out_opt_result_ptr = nullptr) const override;

        /** See documentation in BaseDevice for more details */
        const Anvil::PhysicalDeviceFeatures& get_physical_device_features() const override;

        bool get_physical_device_fence_properties(const FencePropertiesQuery& in_query,
                                                  Anvil::FenceProperties*     out_opt_result_ptr = nullptr) const override;

        bool get_physical_device_semaphore_properties(const SemaphorePropertiesQuery& in_query,
                                                      Anvil::SemaphoreProperties*     out_opt_result_ptr = nullptr) const override;

        Anvil::FormatProperties get_physical_device_format_properties(Anvil::Format in_format) const override;

        bool get_physical_device_image_format_properties(const ImageFormatPropertiesQuery& in_query,
                                                         Anvil::ImageFormatProperties*     out_opt_result_ptr = nullptr) const override;

        /** See documentation in BaseDevice for more details */
        Anvil::MultisamplePropertiesEXT get_physical_device_multisample_properties(const Anvil::SampleCountFlagBits& in_samples) const override;

        /** See documentation in BaseDevice for more details */
        const Anvil::MemoryProperties& get_physical_device_memory_properties() const override;

        /** See documentation in BaseDevice for more details */
        const Anvil::PhysicalDeviceProperties& get_physical_device_properties() const override;

        /** See documentation in BaseDevice for more details */
        const QueueFamilyInfoItems& get_physical_device_queue_families() const override;

        /** See documentation in BaseDevice for more details */
        bool get_physical_device_sparse_image_format_properties(Anvil::Format                                    in_format,
                                                                Anvil::ImageType                                 in_type,
                                                                Anvil::SampleCountFlagBits                       in_sample_count,
                                                                ImageUsageFlags                                  in_usage,
                                                                Anvil::ImageTiling                               in_tiling,
                                                                std::vector<Anvil::SparseImageFormatProperties>& out_result) const override;

        /** See documentation in BaseDevice for more details */
        bool get_physical_device_surface_capabilities(Anvil::RenderingSurface*    in_surface_ptr,
                                                      Anvil::SurfaceCapabilities* out_result_ptr) const override;

        /** See documentation in BaseDevice for more details */
        const Anvil::QueueFamilyInfo* get_queue_family_info(uint32_t in_queue_family_index) const override;

        /* Tells what type this device instance is */
        DeviceType get_type() const override
        {
            return Anvil::DeviceType::SINGLE_GPU;
        }

    protected:
        /* Protected functions */
        void get_queue_family_indices              (DeviceQueueFamilyInfo*          out_device_queue_family_info_ptr) const;
        void init_device                           () override;
        bool is_layer_supported                    (const std::string&              in_layer_name)     const override;
        bool is_physical_device_extension_supported(const std::string&              in_extension_name) const override;

    private:
        /* Private type definitions */

        /* Private functions */

        /** Private constructor. Please use create() instead. */
        SGPUDevice(Anvil::DeviceCreateInfoUniquePtr in_create_info_ptr);

        /* Private variables */
    };

    /* TODO */
    class MGPUDevice : public BaseDevice
    {
    public:
        /* Public functions */

        virtual ~MGPUDevice();

        /** TODO */
        static Anvil::BaseDeviceUniquePtr create(Anvil::DeviceCreateInfoUniquePtr in_create_info_ptr);

        /** TODO */
        Anvil::SwapchainUniquePtr create_swapchain(Anvil::RenderingSurface*           in_parent_surface_ptr,
                                                   Anvil::Window*                     in_window_ptr,
                                                   Anvil::Format                      in_image_format,
                                                   Anvil::ColorSpaceKHR               in_color_space,
                                                   Anvil::PresentModeKHR              in_present_mode,
                                                   ImageUsageFlags                    in_usage,
                                                   uint32_t                           in_n_swapchain_images,
                                                   bool                               in_support_SFR,
                                                   Anvil::DeviceGroupPresentModeFlags in_presentation_modes_to_support = Anvil::DeviceGroupPresentModeFlagBits::LOCAL_BIT_KHR);

        /** TODO
         *
         *  This function does NOT call the driver to retrieve the requested information. Instead, it returns
         *  information cached at mGPU device creation time.
         */
        bool get_peer_memory_features(const Anvil::PhysicalDevice* in_local_physical_device_ptr,
                                      const Anvil::PhysicalDevice* in_remote_physical_device_ptr,
                                      uint32_t                     in_memory_heap_index,
                                      PeerMemoryFeatureFlags*      out_result_ptr) const;

        /** TODO */
        uint32_t get_n_physical_devices() const
        {
            return static_cast<uint32_t>(m_parent_physical_devices.size() );
        }

        /** TODO */
        const Anvil::PhysicalDevice* get_physical_device(uint32_t in_n_physical_device) const;

        bool get_physical_device_buffer_properties(const BufferPropertiesQuery& in_query,
                                                   Anvil::BufferProperties*     out_opt_result_ptr = nullptr) const override;

        /** TODO */
        const Anvil::PhysicalDevice* const* get_physical_devices() const
        {
            return &m_parent_physical_devices_vec.at(0);
        }

        /** TODO */
        const Anvil::PhysicalDeviceFeatures& get_physical_device_features() const override;

        bool get_physical_device_fence_properties(const FencePropertiesQuery& in_query,
                                                  Anvil::FenceProperties*     out_opt_result_ptr = nullptr) const override;

        Anvil::FormatProperties get_physical_device_format_properties(Anvil::Format in_format) const override;

        bool get_physical_device_image_format_properties(const ImageFormatPropertiesQuery& in_query,
                                                         Anvil::ImageFormatProperties*     out_opt_result_ptr = nullptr) const override;

        /** TODO */
        const Anvil::MemoryProperties& get_physical_device_memory_properties() const override;

        /** See documentation in BaseDevice for more details */
        Anvil::MultisamplePropertiesEXT get_physical_device_multisample_properties(const Anvil::SampleCountFlagBits& in_samples) const override;

        /** TODO */
        const Anvil::PhysicalDeviceProperties& get_physical_device_properties() const override;

        /** TODO */
        const QueueFamilyInfoItems& get_physical_device_queue_families() const override;

        bool get_physical_device_semaphore_properties(const SemaphorePropertiesQuery& in_query,
                                                      Anvil::SemaphoreProperties*     out_opt_result_ptr = nullptr) const override;

        /** TODO */
        bool get_physical_device_sparse_image_format_properties(Anvil::Format                                    in_format,
                                                                Anvil::ImageType                                 in_type,
                                                                Anvil::SampleCountFlagBits                       in_sample_count,
                                                                ImageUsageFlags                                  in_usage,
                                                                Anvil::ImageTiling                               in_tiling,
                                                                std::vector<Anvil::SparseImageFormatProperties>& out_result) const override;

        /** TODO */
        bool get_physical_device_surface_capabilities(Anvil::RenderingSurface*    in_surface_ptr,
                                                      Anvil::SurfaceCapabilities* out_result_ptr) const override;

        /** Tells which physical devices can be parent to swapchain images that a physical device at device index @param in_device_index
         *  can present.
         *
         *  By VK_KHR_device_group, *out_result_ptr is guaranteed to at least refer to the device with index @param in_device_index,
         *  as long as the physical device has a presentation engine and can be used for presentation purposes.
         *
         *  This function is not a part of Anvil::PhysicalDevice, because the spec does not guarantee this data to be invariant between
         *  device instances. In cases where the app creates a sGPU device and a mGPU device, where both share the same physical device,
         *  the former device could be incorrectly assumed to be compatible with physical devices from an external device group.
         *
         *  @param in_device_index TODO
         *  @param out_result_ptr  TODO. Must not be NULL.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_present_compatible_physical_devices(uint32_t                                                in_device_index,
                                                     const std::vector<const Anvil::PhysicalDevice*>** out_result_ptr) const;

        /** TODO
         *
         *  Note: The value returned by this function is NOT guaranteed to be invariant.
         **/
        bool get_present_rectangles(uint32_t                       in_device_index,
                                    const Anvil::RenderingSurface* in_rendering_surface_ptr,
                                    std::vector<VkRect2D>*         out_result_ptr) const;

        /** TODO */
        const Anvil::QueueFamilyInfo* get_queue_family_info(uint32_t in_queue_family_index) const override;

        /** TODO */
        Anvil::DeviceGroupPresentModeFlags get_supported_present_modes() const
        {
            return m_supported_present_modes;
        }

        /** TODO
         *
         *  Note: The value returned by this function is NOT guaranteed to be invariant.
         *
         */
        Anvil::DeviceGroupPresentModeFlags get_supported_present_modes_for_surface(const Anvil::RenderingSurface* in_surface_ptr) const;

        /* Tells what type this device instance is */
        DeviceType get_type() const override
        {
            return DeviceType::MULTI_GPU;
        }

        /* Tells whether this logical device is a part of a device group which supports subset allocations. */
        bool supports_subset_allocations() const
        {
            return m_supports_subset_allocations;
        }

    protected:
        /* Protected functions */
        void get_queue_family_indices              (DeviceQueueFamilyInfo*          out_device_queue_family_info_ptr) const;
        void init_device                           () override;
        bool is_layer_supported                    (const std::string&              in_layer_name)     const override;
        bool is_physical_device_extension_supported(const std::string&              in_extension_name) const override;

    private:
        /* Private type definitions */

        struct ParentPhysicalDeviceProperties
        {
            typedef uint32_t                                    HeapIndex;
            typedef std::map<HeapIndex, PeerMemoryFeatureFlags> HeapIndexToPeerMemoryFeaturesMap;
            typedef uint32_t                                    RemotePhysicalDeviceGroupIndex;

            const Anvil::PhysicalDevice*                                               physical_device_ptr;
            std::vector<const Anvil::PhysicalDevice*>                                  presentation_compatible_physical_devices; /* may be empty if the physical device does not have a presentation engine! */
            std::map<RemotePhysicalDeviceGroupIndex, HeapIndexToPeerMemoryFeaturesMap> peer_memory_features;
        };

        /* Private functions */

        /** Private constructor. Please use create() instead. */
        MGPUDevice(Anvil::DeviceCreateInfoUniquePtr in_create_info_ptr);

        /* Private variables */
        std::map<uint32_t, const ParentPhysicalDeviceProperties*> m_device_index_to_physical_device_props;
        std::vector<ParentPhysicalDeviceProperties>               m_parent_physical_devices;
        std::vector<const Anvil::PhysicalDevice*>                 m_parent_physical_devices_vec;
        bool                                                      m_supports_subset_allocations;

        Anvil::DeviceGroupPresentModeFlags m_supported_present_modes;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_DEVICE_H */
