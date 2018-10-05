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
#include "misc/library.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    /** Debug call-back function prototype */
    typedef std::function< VkBool32(VkDebugReportFlagsEXT      in_message_flags,
                                    VkDebugReportObjectTypeEXT in_object_type,
                                    const char*                in_layer_prefix,
                                    const char*                in_message)>     DebugCallbackFunction;

    class Instance : public Anvil::MTSafetySupportProvider
    {
    public:
        /** Destructor */
        virtual ~Instance();

        /** Creates a new Instance wrapper instance. This process is executed in the following steps:
         *
         *  1. If @param opt_pfn_validation_callback_proc is specified, available instance layers are
         *     enumerated. Layers which support VK_EXT_debug_report extension, are cached and used
         *     in step 2.
         *  2. A new Vulkan instance is created.
         *  3. Available physical devices are enumerated.
         *  4. Instance-level function pointers are extracted.
         *
         *  Only one Instance wrapper instance should be created during application's life-time.
         *
         *  NOTE: You MUST call destroy() for this object in order for all dependent objects to be
         *        destroyed correctly.
         *
         *
         *  @param in_app_name                                 Name of the application, to be passed in VkCreateInstanceInfo
         *                                                     structure.
         *  @param in_engine_name                              Name of the engine, to be passed in VkCreateInstanceInfo
         *                                                     structure.
         *  @param in_opt_validation_callback_function         If not nullptr, the specified function will be called whenever
         *                                                     a call-back from any of the validation layers is received.
         *                                                     Ignored otherwise.
         *  @param in_mt_safe                                  True if all instance-based operations where external host synchronization
         *                                                     is required should be automatically synchronized by Anvil.
         *  @param in_opt_disallowed_instance_level_extensions Optional vector holding instance-level extension names that must NOT be
         *                                                     requested at creation time. 
         **/
        static Anvil::InstanceUniquePtr create(const std::string&              in_app_name,
                                               const std::string&              in_engine_name,
                                               DebugCallbackFunction           in_opt_validation_callback_proc,
                                               bool                            in_mt_safe,
                                               const std::vector<std::string>& in_opt_disallowed_instance_level_extensions = std::vector<std::string>() );

        const Anvil::IExtensionInfoInstance<bool>* get_enabled_extensions_info() const
        {
            return m_enabled_extensions_info_ptr->get_instance_extension_info();
        }

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
            return m_validation_callback_function != nullptr;
        }

    private:
        /* Private functions */

        /** Private constructor. Please use create() function instead. */
        Instance(const std::string&    in_app_name,
                 const std::string&    in_engine_name,
                 DebugCallbackFunction in_opt_validation_callback_function,
                 bool                  in_mt_safe);

        Instance& operator=(const Instance&);
        Instance           (const Instance&);

        void destroy                         ();
        void enumerate_instance_layers       ();
        void enumerate_layer_extensions      (Anvil::Layer* layer_ptr);
        void enumerate_physical_device_groups();
        void enumerate_physical_devices      ();
        void init                            (const std::vector<std::string>& in_disallowed_instance_level_extensions);
        void init_debug_callbacks            ();
        void init_func_pointers              ();

        #if !defined(ANVIL_LINK_STATICALLY_WITH_VULKAN_LIB)
            bool init_vk10_func_ptrs();
        #endif

        static VkBool32 VKAPI_PTR debug_callback_pfn_proc(VkDebugReportFlagsEXT      in_message_flags,
                                                          VkDebugReportObjectTypeEXT in_object_type,
                                                          uint64_t                   in_src_object,
                                                          size_t                     in_location,
                                                          int32_t                    in_msg_code,
                                                          const char*                in_layer_prefix_ptr,
                                                          const char*                in_message_ptr,
                                                          void*                      in_user_data);

        /* Private variables */
        VkInstance m_instance;

        /* DebugReport extension function pointers and data */
        VkDebugReportCallbackEXT m_debug_callback_data;

        ExtensionEXTDebugReportEntrypoints                   m_ext_debug_report_entrypoints;
        ExtensionKHRDeviceGroupCreationEntrypoints           m_khr_device_group_creation_entrypoints;
        ExtensionKHRExternalFenceCapabilitiesEntrypoints     m_khr_external_fence_capabilities_entrypoints;
        ExtensionKHRExternalMemoryCapabilitiesEntrypoints    m_khr_external_memory_capabilities_entrypoints;
        ExtensionKHRExternalSemaphoreCapabilitiesEntrypoints m_khr_external_semaphore_capabilities_entrypoints;
        ExtensionKHRGetPhysicalDeviceProperties2             m_khr_get_physical_device_properties2_entrypoints;
        ExtensionKHRSurfaceEntrypoints                       m_khr_surface_entrypoints;

        #ifdef _WIN32
            #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
                ExtensionKHRWin32SurfaceEntrypoints m_khr_win32_surface_entrypoints;
            #endif
        #else
            #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
                ExtensionKHRXcbSurfaceEntrypoints m_khr_xcb_surface_entrypoints;
            #endif
        #endif

        const std::string     m_app_name;
        const std::string     m_engine_name;
        DebugCallbackFunction m_validation_callback_function;

        std::unique_ptr<Anvil::ExtensionInfo<bool> > m_enabled_extensions_info_ptr;
        std::unique_ptr<Anvil::ExtensionInfo<bool> > m_supported_extensions_info_ptr;

        Anvil::Layer                                         m_global_layer;
        std::vector<Anvil::PhysicalDeviceGroup>              m_physical_device_groups;
        std::vector<std::unique_ptr<Anvil::PhysicalDevice> > m_physical_devices;
        std::vector<Anvil::Layer>                            m_supported_layers;

        friend struct InstanceDeleter;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_INSTANCE_H */