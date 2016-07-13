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
#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
    /* Wrapper class for Vulkan physical devices */
    class PhysicalDevice : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor. Retrieves properties & capabilities of a physical device at
         *  user-specified index.
         *
         *  @param instance_ptr    Vulkan instance this object is being spawned for. Must
         *                         not be nullptr.
         *  @param index           Index of the physical device to initialize the wrapper for.
         *  @param physical_device Raw Vulkan physical device handle to encapsulate.
         */
        explicit PhysicalDevice(Anvil::Instance* instance_ptr,
                                uint32_t         index,
                                VkPhysicalDevice physical_device);

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
                static FormatProperties dummy;

                anvil_assert(false);

                return dummy;
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
        PhysicalDevice           (const PhysicalDevice&);
        PhysicalDevice& operator=(const PhysicalDevice&);

        virtual ~PhysicalDevice();

        void init();

        /* Private variables */
        VkPhysicalDevice m_physical_device;

        Anvil::Extensions          m_extensions;
        uint32_t                   m_index;
        Anvil::Instance*           m_instance_ptr;
        VkPhysicalDeviceFeatures   m_features;
        FormatPropertiesMap        m_format_properties;
        Anvil::Layers              m_layers;
        MemoryProperties           m_memory_properties;
        QueueFamilyInfoItems       m_queue_families;
        VkPhysicalDeviceProperties m_properties;
    };

    /* Delete functor. Useful for wrapping Physical Device instances in auto pointers. */
    struct PhysicalDeviceDeleter
    {
        void operator()(PhysicalDevice* physical_device_ptr)
        {
            physical_device_ptr->release();
        }
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_FENCE_H */