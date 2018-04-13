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

        /* Returns physical device's LUID.
         *
         * Requires VK_KHR_external_memory support, as well as VkPhysicalDeviceIDPropertiesKHR::deviceLUIDValid
         * to have been set to true by the driver owning the physical device, in order to report success.
         *
         * @param out_result_ptr Exactly VK_LUID_SIZE_KHR bytes will be copied under *out_result_ptr if
         *                       function returns true. Must not be nullptr.
         *
         * @return true if successful, false otherwise.
         **/
        bool get_device_LUID(uint8_t* out_result_ptr) const
        {
            bool result = false;

            if (m_device_LUID_available)
            {
                static_assert(sizeof(m_device_LUID) == VK_LUID_SIZE_KHR, "");

                memcpy(out_result_ptr,
                       m_device_LUID,
                       VK_LUID_SIZE_KHR);

                result = true;
            }

            return result;
        }

        /* Returns physical device's UUID.
         *
         * Requires VK_KHR_external_memory support to return successfully.
         *
         * @param out_result_ptr Exactly VK_UUID_SIZE bytes will be copied under *out_result_ptr if
         *                       function returns true. Must not be nullptr.
         *
         * @return true if successful, false otherwise.
         **/
        bool get_device_UUID(uint8_t* out_result_ptr) const
        {
            bool result = false;

            if (m_device_UUID_available)
            {
                static_assert(sizeof(m_device_UUID) == VK_UUID_SIZE, "");

                memcpy(out_result_ptr,
                       m_device_UUID,
                       VK_UUID_SIZE);

                result = true;
            }

            return result;
        }

        /* Returns UUID of the driver owning the physical device.
         *
         * Requires VK_KHR_external_memory support to return successfully.
         *
         * @param out_result_ptr Exactly VK_UUID_SIZE bytes will be copied under *out_result_ptr if
         *                       function returns true. Must not be nullptr.
         *
         * @return true if successful, false otherwise.
         **/
        bool get_driver_UUID(uint8_t* out_result_ptr) const
        {
            bool result = false;

            if (m_driver_UUID_available)
            {
                static_assert(sizeof(m_driver_UUID) == VK_UUID_SIZE, "");

                memcpy(out_result_ptr,
                       m_driver_UUID,
                       VK_UUID_SIZE);

                result = true;
            }

            return result;
        }

        /** Retrieves features supported by the physical device */
        const Anvil::PhysicalDeviceFeatures& get_device_features() const
        {
            return m_features;
        }

        const Anvil::PhysicalDeviceProperties& get_device_properties() const
        {
            return m_properties;
        }

        /** Retrieves format properties, as reported by the wrapped physical device.
         *
         *  @param in_format Vulkan format to retrieve the filled structure for.
         *
         *  @return As per description.
         **/
        const FormatProperties& get_format_properties(VkFormat in_format) const
        {
            auto format_props_iterator = m_format_properties.find(in_format);

            if (format_props_iterator != m_format_properties.end() )
            {
                return format_props_iterator->second;
            }
            else
            {
                anvil_assert_fail();

                return m_dummy;
            }
        }

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
        bool get_sparse_image_format_properties(VkFormat                                    in_format,
                                                VkImageType                                 in_type,
                                                VkSampleCountFlagBits                       in_sample_count,
                                                VkImageUsageFlags                           in_usage,
                                                VkImageTiling                               in_tiling,
                                                std::vector<VkSparseImageFormatProperties>& out_result) const;

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

    private:
        /* Private type definitions */
        typedef std::map<VkFormat, FormatProperties> FormatPropertiesMap;

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
            :m_device_LUID_available(false),
             m_device_UUID_available(false),
             m_index                (in_index),
             m_instance_ptr         (in_instance_ptr),
             m_physical_device      (in_physical_device)
        {
            anvil_assert(in_physical_device != VK_NULL_HANDLE);
        }

        PhysicalDevice           (const PhysicalDevice&);
        PhysicalDevice& operator=(const PhysicalDevice&);

        bool init();

        /* Private variables */
        FormatProperties                             m_dummy;
        std::unique_ptr<Anvil::ExtensionInfo<bool> > m_extension_info_ptr;
        uint32_t                                     m_index;
        Anvil::Instance*                             m_instance_ptr;
        Anvil::PhysicalDeviceFeatures                m_features;
        FormatPropertiesMap                          m_format_properties;
        Anvil::Layers                                m_layers;
        MemoryProperties                             m_memory_properties;
        VkPhysicalDevice                             m_physical_device;
        QueueFamilyInfoItems                         m_queue_families;
        Anvil::PhysicalDeviceProperties              m_properties;

        std::unique_ptr<Anvil::AMDShaderCoreProperties>          m_amd_shader_core_properties_ptr;
        std::unique_ptr<Anvil::PhysicalDeviceFeaturesCoreVK10>   m_core_features_vk10_ptr;
        std::unique_ptr<Anvil::PhysicalDevicePropertiesCoreVK10> m_core_properties_vk10_ptr;
        std::unique_ptr<Anvil::EXTDescriptorIndexingFeatures>    m_ext_descriptor_indexing_features_ptr;
        std::unique_ptr<Anvil::EXTDescriptorIndexingProperties>  m_ext_descriptor_indexing_properties_ptr;
        std::unique_ptr<Anvil::KHR16BitStorageFeatures>          m_khr_16_bit_storage_features_ptr;
        std::unique_ptr<Anvil::KHRMaintenance3Properties>        m_khr_maintenance3_properties_ptr;

        bool    m_device_LUID_available;
        uint8_t m_device_LUID[VK_LUID_SIZE_KHR];
        bool    m_device_UUID_available;
        uint8_t m_device_UUID[VK_UUID_SIZE];
        uint8_t m_driver_UUID[VK_UUID_SIZE];
        bool    m_driver_UUID_available;

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
