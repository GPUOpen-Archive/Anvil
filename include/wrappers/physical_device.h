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

/** Implements a wrapper for a single Vulkan physical device. Implemented in order to:
 *
 *  - simplify life-time management of physical devices.
 *  - provide a simply way to cache & retrieve information about physical device capabilities.
 *  - track any physical device wrapper instance leaks via object tracker.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_PHYSICAL_DEVICE_H
#define WRAPPERS_PHYSICAL_DEVICE_H

#include "misc/debug.h"
#include "misc/extensions.h"
#include "misc/types.h"

namespace Anvil
{
    /* Wrapper class for Vulkan physical devices */
    class PhysicalDevice
    {
    public:
        /* Public functions */

        /** Creates a new PhysicalDevice instance.
         *
         *  Retrieves properties & capabilities of a physical device at user-specified index.
         *
         *  @param in_instance_ptr    Vulkan instance this object is being spawned for. Must
         *                            not be nullptr.
         *  @param in_index           Index of the physical device to initialize the wrapper for.
         *  @param in_physical_device Raw Vulkan physical device handle to encapsulate.
         */
        static std::unique_ptr<Anvil::PhysicalDevice> create(Anvil::Instance* in_instance_ptr,
                                                             uint32_t         in_index,
                                                             VkPhysicalDevice in_physical_device);

        /** Destructor */
        virtual ~PhysicalDevice();

        /** Returns a filled structure telling current consumption of memory on a per-heap basis.
         *
         *  Requires VK_EXT_memory_budget support.
         **/
        Anvil::MemoryBudget get_available_memory_budget() const;

        /* TODO
         *
         * Requires VK_KHR_external_memory_capabilities.
         */
        bool get_buffer_properties(const Anvil::BufferPropertiesQuery& in_query,
                                   Anvil::BufferProperties*            out_opt_result_ptr = nullptr) const;

        /** Retrieves features supported by the physical device */
        const Anvil::PhysicalDeviceFeatures& get_device_features() const
        {
            return m_features;
        }

        /** Retrieves device group index of this physical device. */
        uint32_t get_device_group_index() const
        {
            return m_device_group_index;
        }

        /** Retrieves index of the physical device within a device group. */
        uint32_t get_device_group_device_index() const
        {
            return m_device_group_device_index;
        }

        const Anvil::PhysicalDeviceProperties& get_device_properties() const
        {
            return m_properties;
        }

        /* TODO
         *
         * Requires VK_KHR_external_fence_capabiltiies.
         *
         */
        bool get_fence_properties(const Anvil::FencePropertiesQuery& in_query,
                                  Anvil::FenceProperties*            out_opt_result_ptr = nullptr) const;

        Anvil::FormatProperties get_format_properties(Anvil::Format in_format) const;

        /** Retrieves image format properties, as reported by the wrapped physical device.
         *
         *  For external memory handle capability queries, VK_KHR_external_memory_capabilities support is required.
         *
         *
         *  @param in_format Vulkan format to retrieve the filled structure for.
         *
         *  @return As per description.
         **/
        bool get_image_format_properties(const ImageFormatPropertiesQuery& in_query,
                                         Anvil::ImageFormatProperties*     out_opt_result_ptr = nullptr) const;

        /** Returns index of the physical device. */
        uint32_t get_index() const
        {
            return m_index;
        }

        /** Returns parent Vulkan instance wrapper. */
        const Anvil::Instance* get_instance() const
        {
            return m_instance_ptr;
        }

        Anvil::Instance* get_instance()
        {
            return m_instance_ptr;
        }

        /** Returns all layers, supported by the physical device */
        const Anvil::Layers& get_layers() const
        {
            return m_layers;
        }

        /** Returns a filled Anvil::MemoryProperties structure, describing the
        *  encapsulated physical device.
        **/
        const Anvil::MemoryProperties& get_memory_properties() const
        {
            return m_memory_properties;
        }

        /** Returns a raw Vulkan Physical Device handle. */
        const VkPhysicalDevice& get_physical_device() const
        {
            return m_physical_device;
        }

        /** Returns a filled QueueFamilyInfo vector, describing the wrapped physical
         *  device's capabilities.
         **/
        const QueueFamilyInfoItems& get_queue_families() const
        {
            return m_queue_families;
        }

        /* TODO
         *
         * Requires VK_KHR_external_semaphore_capabiltiies.
         *
         */
        bool get_semaphore_properties(const Anvil::SemaphorePropertiesQuery& in_query,
                                      Anvil::SemaphoreProperties*            out_opt_result_ptr = nullptr) const;

        /** Returns sparse image format properties for this physical device. See Vulkan spec
         *  for vkGetPhysicalDeviceSparseImageFormatProperties() function for more details.
         *
         *  @param in_format       As per Vulkan spec.
         *  @param in_type         As per Vulkan spec.
         *  @param in_sample_count As per Vulkan spec.
         *  @param in_usage        As per Vulkan spec.
         *  @param in_tiling       As per Vulkan spec.
         *  @param out_result      The retrieved information will be stored under this location, if
         *                         the function returns true.
         *
         *  @return true if successful, false otherwise
         **/
        bool get_sparse_image_format_properties(Anvil::Format                                    in_format,
                                                Anvil::ImageType                                 in_type,
                                                Anvil::SampleCountFlagBits                       in_sample_count,
                                                Anvil::ImageUsageFlags                           in_usage,
                                                Anvil::ImageTiling                               in_tiling,
                                                std::vector<Anvil::SparseImageFormatProperties>& out_result) const;

        /** Tells whether user-specified extension is supported by the physical device.
         *
         *  @param in_extension_name Name of the extension to use for the query. Must not be
         *                           nullptr.
         *
         *  @return As per description.
         **/
        bool is_device_extension_supported(const std::string& in_extension_name) const;

        /** Tells whether user-specified layer is supported by the physical device.
         *
         *  @param in_layer_name Name of the layer to use for the query. Must not be
         *                       nullptr.
         *
         *  @return As per description.
         **/
        bool is_layer_supported(const std::string& in_layer_name) const;

        /* Tells if the physical device supports VK 1.1 API (or newer) */
        bool supports_core_vk1_1() const;

    private:
        /* Private functions */

        /** Constructor. Retrieves properties & capabilities of a physical device at
         *  user-specified index.
         *
         *  @param in_instance_ptr    Vulkan instance this object is being spawned for. Must
         *                            not be nullptr.
         *  @param in_index           Index of the physical device to initialize the wrapper for.
         *  @param in_physical_device Raw Vulkan physical device handle to encapsulate.
         */
        explicit PhysicalDevice(Anvil::Instance* in_instance_ptr,
                                uint32_t         in_index,
                                VkPhysicalDevice in_physical_device)
            :m_device_group_device_index(0),
             m_device_group_index       (0),
             m_index                    (in_index),
             m_instance_ptr             (in_instance_ptr),
             m_physical_device          (in_physical_device)
        {
            anvil_assert(in_physical_device != VK_NULL_HANDLE);
        }

        PhysicalDevice           (const PhysicalDevice&);
        PhysicalDevice& operator=(const PhysicalDevice&);

        bool init                         ();
        void set_device_group_device_index(uint32_t in_new_device_group_device_index);
        void set_device_group_index       (uint32_t in_new_device_group_index);

        bool supports_core_vk1_1(const uint32_t& in_api_version) const;

        /* Private variables */
        uint32_t                                     m_device_group_device_index;
        uint32_t                                     m_device_group_index;
        std::unique_ptr<Anvil::ExtensionInfo<bool> > m_extension_info_ptr;
        uint32_t                                     m_index;
        Anvil::Instance*                             m_instance_ptr;
        Anvil::PhysicalDeviceFeatures                m_features;
        Anvil::Layers                                m_layers;
        MemoryProperties                             m_memory_properties;
        VkPhysicalDevice                             m_physical_device;
        QueueFamilyInfoItems                         m_queue_families;
        Anvil::PhysicalDeviceProperties              m_properties;

        std::unique_ptr<Anvil::AMDShaderCoreProperties>                                 m_amd_shader_core_properties_ptr;
        std::unique_ptr<Anvil::PhysicalDeviceFeaturesCoreVK10>                          m_core_features_vk10_ptr;
        std::unique_ptr<Anvil::PhysicalDeviceFeaturesCoreVK11>                          m_core_features_vk11_ptr;
        std::unique_ptr<Anvil::PhysicalDevicePropertiesCoreVK10>                        m_core_properties_vk10_ptr;
        std::unique_ptr<Anvil::PhysicalDevicePropertiesCoreVK11>                        m_core_properties_vk11_ptr;
        std::unique_ptr<Anvil::EXTConservativeRasterizationProperties>                  m_ext_conservative_rasterization_properties_ptr;
        std::unique_ptr<Anvil::EXTDepthClipEnableFeatures>                              m_ext_depth_clip_enable_features_ptr;
        std::unique_ptr<Anvil::EXTDescriptorIndexingFeatures>                           m_ext_descriptor_indexing_features_ptr;
        std::unique_ptr<Anvil::EXTDescriptorIndexingProperties>                         m_ext_descriptor_indexing_properties_ptr;
        std::unique_ptr<Anvil::EXTExternalMemoryHostProperties>                         m_ext_external_memory_host_properties_ptr;
        std::unique_ptr<Anvil::EXTInlineUniformBlockFeatures>                           m_ext_inline_uniform_block_features_ptr;
        std::unique_ptr<Anvil::EXTInlineUniformBlockProperties>                         m_ext_inline_uniform_block_properties_ptr;
        std::unique_ptr<Anvil::EXTPCIBusInfoProperties>                                 m_ext_pci_bus_info_ptr;
        std::unique_ptr<Anvil::EXTSampleLocationsProperties>                            m_ext_sample_locations_properties_ptr;
        std::unique_ptr<Anvil::EXTSamplerFilterMinmaxProperties>                        m_ext_sampler_filter_minmax_properties_ptr;
        std::unique_ptr<Anvil::EXTScalarBlockLayoutFeatures>                            m_ext_scalar_block_layout_features_ptr;
        std::unique_ptr<Anvil::EXTTransformFeedbackFeatures>                            m_ext_transform_feedback_features_ptr;
        std::unique_ptr<Anvil::EXTTransformFeedbackProperties>                          m_ext_transform_feedback_properties_ptr;
        std::unique_ptr<Anvil::EXTVertexAttributeDivisorProperties>                     m_ext_vertex_attribute_divisor_properties_ptr;
        std::unique_ptr<Anvil::EXTMemoryPriorityFeatures>                               m_ext_memory_priority_features_ptr;
        std::unique_ptr<Anvil::KHR16BitStorageFeatures>                                 m_khr_16_bit_storage_features_ptr;
        std::unique_ptr<Anvil::KHR8BitStorageFeatures>                                  m_khr_8_bit_storage_features_ptr;
        std::unique_ptr<Anvil::KHRDepthStencilResolveProperties>                        m_khr_depth_stencil_resolve_properties_ptr;
        std::unique_ptr<Anvil::KHRDriverPropertiesProperties>                           m_khr_driver_properties_properties_ptr;
        std::unique_ptr<Anvil::KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties> m_khr_external_memory_capabilities_physical_device_id_properties_ptr;
        std::unique_ptr<Anvil::KHRFloat16Int8Features>                                  m_khr_float16_int8_features_ptr;
        std::unique_ptr<Anvil::KHRMaintenance2PhysicalDevicePointClippingProperties>    m_khr_maintenance2_physical_device_point_clipping_properties_ptr;
        std::unique_ptr<Anvil::KHRMaintenance3Properties>                               m_khr_maintenance3_properties_ptr;
        std::unique_ptr<Anvil::KHRMultiviewFeatures>                                    m_khr_multiview_features_ptr;
        std::unique_ptr<Anvil::KHRMultiviewProperties>                                  m_khr_multiview_properties_ptr;
        std::unique_ptr<Anvil::KHRSamplerYCbCrConversionFeatures>                       m_khr_sampler_ycbcr_conversion_features_ptr;
        std::unique_ptr<Anvil::KHRShaderAtomicInt64Features>                            m_khr_shader_atomic_int64_features_ptr;
        std::unique_ptr<Anvil::KHRShaderFloatControlsProperties>                        m_khr_shader_float_controls_properties_ptr;
        std::unique_ptr<Anvil::KHRVariablePointerFeatures>                              m_khr_variable_pointer_features_ptr;
        std::unique_ptr<Anvil::KHRVulkanMemoryModelFeatures>                            m_khr_vulkan_memory_model_features_ptr;

        friend class Anvil::Instance;
    };

    /** Tells whether the specified Anvil PhysicalDevice wrapper encapsulates given Vulkan physical device.
     *
     *  @param in_physical_device_ptr Anvil physical device wrapper. Must not be null.
     *  @param in_physical_device_vk  Raw Vulkan physical device handle.
     *
     *  @return true if objects match, false otherwise.
     **/
    bool operator==(const std::unique_ptr<Anvil::PhysicalDevice>& in_physical_device_ptr,
                    const VkPhysicalDevice&                       in_physical_device_vk);
}; /* namespace Anvil */

#endif /* WRAPPERS_FENCE_H */
