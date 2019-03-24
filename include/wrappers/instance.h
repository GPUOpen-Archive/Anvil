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

/** Implements a wrapper for a single Vulkan instance. Implemented in order to:
 *
 *  - manage life-time of Vulkan instances.
 *  - encapsulate all logic required to manipulate instances and children objects.
 *  - let ObjectTracker detect leaking Vulkan instance wrapper instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_INSTANCE_H
#define WRAPPERS_INSTANCE_H

#include "misc/extensions.h"
#include "misc/instance_create_info.h"
#include "misc/library.h"
#include "misc/mt_safety.h"
#include "misc/types.h"
#include "wrappers/debug_messenger.h"

namespace Anvil
{
    class Instance : public Anvil::MTSafetySupportProvider
    {
    public:
        /** Destructor */
        virtual ~Instance();

        /** Creates a new Instance wrapper instance. **/
        static Anvil::InstanceUniquePtr create(Anvil::InstanceCreateInfoUniquePtr in_create_info_ptr);

        const Anvil::APIVersion& get_api_version() const
        {
            return m_api_version;
        }

        const Anvil::IExtensionInfoInstance<bool>* get_enabled_extensions_info() const
        {
            return m_enabled_extensions_info_ptr->get_instance_extension_info();
        }

        const ExtensionEXTDebugReportEntrypoints& get_extension_ext_debug_report_entrypoints() const;
        const ExtensionEXTDebugUtilsEntrypoints&  get_extension_ext_debug_utils_entrypoints () const;

        const ExtensionKHRExternalFenceCapabilitiesEntrypoints& get_extension_khr_external_fence_capabilities_entrypoints() const;

        const ExtensionKHRExternalMemoryCapabilitiesEntrypoints& get_extension_khr_external_memory_capabilities_entrypoints() const;

        const ExtensionKHRExternalSemaphoreCapabilitiesEntrypoints& get_extension_khr_external_semaphore_capabilities_entrypoints() const;

        /** Returns a container with entry-points to functions introduced by VK_KHR_get_physical_device_properties2.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionKHRGetPhysicalDeviceProperties2& get_extension_khr_get_physical_device_properties2_entrypoints() const;

        /** Returns a container with entry-points to functions introduced by VK_KHR_surface.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionKHRSurfaceEntrypoints& get_extension_khr_surface_entrypoints() const;

#if defined(_WIN32)
    #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        /** Returns a container with entry-points to functions introduced by VK_KHR_win32_surface.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionKHRWin32SurfaceEntrypoints& get_extension_khr_win32_surface_entrypoints() const;
    #endif
#else
    #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        /** Returns a container with entry-points to functions introduced by VK_KHR_xcb_surface.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionKHRXcbSurfaceEntrypoints& get_extension_khr_xcb_surface_entrypoints() const;
    #endif
#endif

        /** Returns a container with entry-points to functions introduced by VK_KHR_device_group_creation extension.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionKHRDeviceGroupCreationEntrypoints& get_extension_khr_device_group_creation_entrypoints() const;

        const Anvil::InstanceCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        /** Returns a raw wrapped VkInstance handle. */
        VkInstance get_instance_vk() const
        {
            return m_instance;
        }

        /** Returns information about @param in_n_physical_device_group -th physical device group, as reported
         *  for this instance.
         *
         *  @param in_n_physical_device_group Index of the physical device group to retrieve properties of.
         *                                    This value must NOT be equal or larger than the value reported by
         *                                    get_n_physical_device_groups().
         *
         ** @return As per description.
         **/
        const Anvil::PhysicalDeviceGroup& get_physical_device_group(uint32_t in_n_physical_device_group) const
        {
            anvil_assert(m_physical_device_groups.size() > in_n_physical_device_group);

            return m_physical_device_groups.at(in_n_physical_device_group);
        }

        /** Returns a PhysicalDevice wrapper for a physical device at index @param in_n_device.
         *
         *  @param in_n_device Index of the physical device to retrieve the wrapper instance for.
         *                     This value must NOT be equal or larger than the value reported by
         *                     get_n_physical_devices().
         *
         ** @return As per description.
         **/
        const Anvil::PhysicalDevice* get_physical_device(uint32_t in_n_device) const
        {
            return m_physical_devices.at(in_n_device).get();
        }

        /** Returns the total number of physical device groups supported on the running platform.
         *
         *  Will return 0 if VK_KHR_physical_device_group_creation is not supported.
         */
        uint32_t get_n_physical_device_groups() const
        {
            return static_cast<uint32_t>(m_physical_device_groups.size() );
        }

        /** Returns the total number of physical devices supported on the running platform. */
        uint32_t get_n_physical_devices() const
        {
            return static_cast<uint32_t>(m_physical_devices.size() );
        }

        bool is_instance_extension_enabled(const char*        in_extension_name) const;
        bool is_instance_extension_enabled(const std::string& in_extension_name) const
        {
            return is_instance_extension_enabled(in_extension_name.c_str() );
        }

        /** Tells whether the specified instance extension is supported.
         *
         *  @param in_extension_name Name of the extension to use for the query.
         *
         *  @return true if the extension was reported as supported, false otherwise.
         **/
        bool is_instance_extension_supported(const char*        in_extension_name) const;
        bool is_instance_extension_supported(const std::string& in_extension_name) const
        {
            return is_instance_extension_supported(in_extension_name.c_str() );
        }

        /** Tells if validation support has been requested for this Vulkan Instance wrapper */
        bool is_validation_enabled() const
        {
            return m_create_info_ptr->get_validation_callback() != nullptr;
        }

    private:
        /* Private functions */

        /** Private constructor. Please use create() function instead. */
        Instance(Anvil::InstanceCreateInfoUniquePtr in_create_info_ptr);

        Instance& operator=(const Instance&);
        Instance           (const Instance&);

        void destroy                         ();
        void enumerate_instance_layers       ();
        void enumerate_layer_extensions      (Anvil::Layer* layer_ptr);
        void enumerate_physical_device_groups();
        void enumerate_physical_devices      ();
        bool init                            ();
        void init_debug_callbacks            ();
        void init_func_pointers              ();

        bool init_vk_func_ptrs();

        void debug_callback_handler(const Anvil::DebugMessageSeverityFlagBits& in_severity,
                                    const char*                                in_message_ptr);

        /* Private variables */
        VkInstance m_instance;

        ExtensionEXTDebugReportEntrypoints                   m_ext_debug_report_entrypoints;
        ExtensionEXTDebugUtilsEntrypoints                    m_ext_debug_utils_entrypoints;
        ExtensionKHRDeviceGroupCreationEntrypoints           m_khr_device_group_creation_entrypoints;
        ExtensionKHRExternalFenceCapabilitiesEntrypoints     m_khr_external_fence_capabilities_entrypoints;
        ExtensionKHRExternalMemoryCapabilitiesEntrypoints    m_khr_external_memory_capabilities_entrypoints;
        ExtensionKHRExternalSemaphoreCapabilitiesEntrypoints m_khr_external_semaphore_capabilities_entrypoints;
        ExtensionKHRGetPhysicalDeviceProperties2             m_khr_get_physical_device_properties2_entrypoints;
        ExtensionKHRSurfaceEntrypoints                       m_khr_surface_entrypoints;

        #if defined(_WIN32)
        #endif

        #ifdef _WIN32
            #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
                ExtensionKHRWin32SurfaceEntrypoints m_khr_win32_surface_entrypoints;
            #endif
        #else
            #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
                ExtensionKHRXcbSurfaceEntrypoints m_khr_xcb_surface_entrypoints;
            #endif
        #endif

        Anvil::APIVersion                  m_api_version;
        Anvil::InstanceCreateInfoUniquePtr m_create_info_ptr;

        std::unique_ptr<Anvil::ExtensionInfo<bool> > m_enabled_extensions_info_ptr;
        std::unique_ptr<Anvil::ExtensionInfo<bool> > m_supported_extensions_info_ptr;

        Anvil::DebugMessengerUniquePtr                       m_debug_messenger_ptr;
        Anvil::Layer                                         m_global_layer;
        std::vector<Anvil::PhysicalDeviceGroup>              m_physical_device_groups;
        std::vector<std::unique_ptr<Anvil::PhysicalDevice> > m_physical_devices;
        std::vector<Anvil::Layer>                            m_supported_layers;

        friend struct InstanceDeleter;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_INSTANCE_H */