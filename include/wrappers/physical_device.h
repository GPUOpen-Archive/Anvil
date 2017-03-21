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
#include "misc/types.h"

namespace Anvil
{
    /* Wrapper class for Vulkan physical devices */
    class PhysicalDevice : public std::enable_shared_from_this<PhysicalDevice>
    {
    public:
        /* Public functions */

        /** Creates a new PhysicalDevice instance.
         *
         *  Retrieves properties & capabilities of a physical device at user-specified index.
         *
         *  @param instance_ptr    Vulkan instance this object is being spawned for. Must
         *                         not be nullptr.
         *  @param index           Index of the physical device to initialize the wrapper for.
         *  @param physical_device Raw Vulkan physical device handle to encapsulate.
         */
        static std::weak_ptr<Anvil::PhysicalDevice> create(std::shared_ptr<Anvil::Instance> instance_ptr,
                                                           uint32_t                         index,
                                                           VkPhysicalDevice                 physical_device);

        /** Destructor */
        virtual ~PhysicalDevice();

        /** Retrieves features supported by the physical device */
        const VkPhysicalDeviceFeatures& get_device_features() const
        {
            return m_features;
        }

        /** Retrieves a filled VkPhysicalDeviceProperties structure, holding properties of
         *  the wrapped physical device.
         **/
        const VkPhysicalDeviceProperties& get_device_properties() const
        {
            return m_properties;
        }

        /** Retrieves format properties, as reported by the wrapped physical device.
         *
         *  @param format Vulkan format to retrieve the filled structure for.
         *
         *  @return As per description.
         **/
        const FormatProperties& get_format_properties(VkFormat format) const
        {
            auto format_props_iterator = m_format_properties.find(format);

            if (format_props_iterator != m_format_properties.end() )
            {
                return format_props_iterator->second;
            }
            else
            {
                anvil_assert(false);

                return m_dummy;
            }
        }

        /** Returns index of the physical device. */
        uint32_t get_index() const
        {
            return m_index;
        }

        /** Returns parent Vulkan instance wrapper. */
        std::weak_ptr<Anvil::Instance> get_instance() const
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

        /** Returns sprase image format properties for this physical device. See Vulkan spec
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
         *  @param extension_name Name of the extension to use for the query. Must not be
         *                        nullptr.
         *
         *  @return As per description.
         **/
        bool is_device_extension_supported(const char* extension_name) const;

        /** Tells whether user-specified layer is supported by the physical device.
         *
         *  @param layer_name Name of the layer to use for the query. Must not be
         *                    nullptr.
         *
         *  @return As per description.
         **/
        bool is_layer_supported(const char* layer_name) const;

    private:
        /* Private type definitions */
        typedef std::map<VkFormat, FormatProperties> FormatPropertiesMap;

        /* Private functions */

        /** Constructor. Retrieves properties & capabilities of a physical device at
         *  user-specified index.
         *
         *  @param instance_ptr    Vulkan instance this object is being spawned for. Must
         *                         not be nullptr.
         *  @param index           Index of the physical device to initialize the wrapper for.
         *  @param physical_device Raw Vulkan physical device handle to encapsulate.
         */
        explicit PhysicalDevice(std::weak_ptr<Anvil::Instance> instance_ptr,
                                uint32_t                       index,
                                VkPhysicalDevice               physical_device)
            :m_destroyed      (false),
             m_index          (index),
             m_instance_ptr   (instance_ptr),
             m_physical_device(physical_device)
        {
            anvil_assert(physical_device != VK_NULL_HANDLE);
        }

        PhysicalDevice           (const PhysicalDevice&);
        PhysicalDevice& operator=(const PhysicalDevice&);

        void destroy          ();
        void init             ();
        void register_device  (std::shared_ptr<Anvil::BaseDevice> device_ptr);
        void unregister_device(std::shared_ptr<Anvil::BaseDevice> device_ptr);

        /* Private variables */
        bool             m_destroyed;
        FormatProperties m_dummy;

        Anvil::Extensions                m_extensions;
        uint32_t                         m_index;
        std::shared_ptr<Anvil::Instance> m_instance_ptr;
        VkPhysicalDeviceFeatures         m_features;
        FormatPropertiesMap              m_format_properties;
        Anvil::Layers                    m_layers;
        MemoryProperties                 m_memory_properties;
        VkPhysicalDevice                 m_physical_device;
        QueueFamilyInfoItems             m_queue_families;
        VkPhysicalDeviceProperties       m_properties;

        std::vector<std::shared_ptr<Anvil::BaseDevice> > m_cached_devices;

        friend class Anvil::Instance;
        friend class Anvil::SGPUDevice;
    };

    /** Tells whether the specified Anvil PhysicalDevice wrapper encapsulates given Vulkan physical device.
     *
     *  @param physical_device_ptr Anvil physical device wrapper. Must not be null.
     *  @param physical_device_vk  Raw Vulkan physical device handle.
     *
     *  @return true if objects match, false otherwise.
     **/
    bool operator==(const std::shared_ptr<Anvil::PhysicalDevice>& physical_device_ptr,
                    const VkPhysicalDevice&                       physical_device_vk);

}; /* namespace Anvil */

#endif /* WRAPPERS_FENCE_H */