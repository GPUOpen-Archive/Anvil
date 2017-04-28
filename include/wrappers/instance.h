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

#include "misc/types.h"
#include "wrappers/rendering_surface.h"

namespace Anvil
{
    /** Debug call-back function prototype */
    typedef VkBool32 (*PFNINSTANCEDEBUGCALLBACKPROC)(VkDebugReportFlagsEXT      message_flags,
                                                     VkDebugReportObjectTypeEXT object_type,
                                                     const char*                layer_prefix,
                                                     const char*                message,
                                                     void*                      user_arg);

    class Instance : public std::enable_shared_from_this<Instance>
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
         *  @param app_name                 Name of the application, to be passed in VkCreateInstanceInfo
         *                                  structure.
         *  @param engine_name              Name of the engine, to be passed in VkCreateInstanceInfo
         *                                  structure.
         *  @param opt_pfn_validation_proc  If not nullptr, the specified handled will be called whenever
         *                                  a call-back from any of the validation layers is received.
         *                                  Ignored otherwise.
         *  @param validation_proc_user_arg If @param opt_pfn_validation_proc is not nullptr, this argument
         *                                  will be passed to @param opt_pfn_validation_proc every time
         *                                  a debug callback is received. Ignored otherwise.
         **/
        static std::shared_ptr<Anvil::Instance> create(const char*                  app_name,
                                                       const char*                  engine_name,
                                                       PFNINSTANCEDEBUGCALLBACKPROC opt_pfn_validation_callback_proc,
                                                       void*                        validation_proc_user_arg);

        void destroy();

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

#ifdef _WIN32
        /** Returns a container with entry-points to functions introduced by VK_KHR_win32_surface.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionKHRWin32SurfaceEntrypoints& get_extension_khr_win32_surface_entrypoints() const;
#else
        /** Returns a container with entry-points to functions introduced by VK_KHR_xcb_surface.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionKHRXcbSurfaceEntrypoints& get_extension_khr_xcb_surface_entrypoints() const;
#endif

        /** Returns a raw wrapped VkInstance handle. */
        VkInstance get_instance_vk() const
        {
            return m_instance;
        }

        /** Returns a PhysicalDevice wrapper for a physical device at index @param n_device.
         *
         *  @param n_device Index of the physical device to retrieve the wrapper instance for.
         *                  This value must NOT be equal or larger than the value reported by
         *                  get_n_physical_devices().
         *
         ** @return As per description.
         **/
        std::weak_ptr<Anvil::PhysicalDevice> get_physical_device(uint32_t n_device) const
        {
            return m_physical_devices[n_device];
        }

        /** Returns the total number of physical devices supported on the running platform. */
        uint32_t get_n_physical_devices() const
        {
            return static_cast<uint32_t>(m_physical_devices.size() );
        }

        /** Tells whether the specified instance extension is supported.
         *
         *  @param extension_name Name of the extension to use for the query.
         *
         *  @return true if the extension was reported as supported, false otherwise.
         **/
        bool is_instance_extension_supported(const char* extension_name) const;

        /** Tells if validation support has been requested for this Vulkan Instance wrapper */
        bool is_validation_enabled() const
        {
            return m_pfn_validation_callback_proc != nullptr;
        }

    private:
        /* Private functions */

        /** Private constructor. Please use create() function instead. */
        Instance(const char*                  app_name,
                 const char*                  engine_name,
                 PFNINSTANCEDEBUGCALLBACKPROC opt_pfn_validation_callback_proc,
                 void*                        validation_proc_user_arg);

        Instance& operator=(const Instance&);
        Instance           (const Instance&);

        void register_physical_device  (std::shared_ptr<Anvil::PhysicalDevice> physical_device_ptr);
        void unregister_physical_device(std::shared_ptr<Anvil::PhysicalDevice> physical_device_ptr);

        void enumerate_instance_layers ();
        void enumerate_layer_extensions(Anvil::Layer* layer_ptr);
        void enumerate_physical_devices();
        void init                      ();
        void init_debug_callbacks      ();
        void init_func_pointers        ();

        static VkBool32 VKAPI_PTR debug_callback_pfn_proc(VkDebugReportFlagsEXT      message_flags,
                                                          VkDebugReportObjectTypeEXT object_type,
                                                          uint64_t                   src_object,
                                                          size_t                     location,
                                                          int32_t                    msg_code,
                                                          const char*                layer_prefix_ptr,
                                                          const char*                message_ptr,
                                                          void*                      user_data);

        /* Private variables */
        VkInstance m_instance;

        /* DebugReport extension function pointers and data */
        VkDebugReportCallbackEXT m_debug_callback_data;

        ExtensionEXTDebugReportEntrypoints       m_ext_debug_report_entrypoints;
        ExtensionKHRGetPhysicalDeviceProperties2 m_khr_get_physical_device_properties2_entrypoints;
        ExtensionKHRSurfaceEntrypoints           m_khr_surface_entrypoints;

        #ifdef _WIN32
            ExtensionKHRWin32SurfaceEntrypoints m_khr_win32_surface_entrypoints;
        #else
            ExtensionKHRXcbSurfaceEntrypoints m_khr_xcb_surface_entrypoints;
        #endif

        const char*                  m_app_name;
        const char*                  m_engine_name;
        PFNINSTANCEDEBUGCALLBACKPROC m_pfn_validation_callback_proc;
        void*                        m_validation_proc_user_arg;

        std::vector<std::string>                             m_enabled_extensions;
        Anvil::Layer                                         m_global_layer;
        std::vector<std::shared_ptr<Anvil::PhysicalDevice> > m_physical_devices;
        std::vector<Anvil::Layer>                            m_supported_layers;

        friend class  Anvil::PhysicalDevice;
        friend struct InstanceDeleter;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_INSTANCE_H */