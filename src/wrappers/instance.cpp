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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"

/** Please see header for specification */
Anvil::Instance::Instance(const std::string&    in_app_name,
                          const std::string&    in_engine_name,
                          DebugCallbackFunction in_opt_validation_callback_function,
                          bool                  in_mt_safe)
    :MTSafetySupportProvider       (in_mt_safe),
     m_app_name                    (in_app_name),
     m_debug_callback_data         (0),
     m_engine_name                 (in_engine_name),
     m_global_layer                (""),
     m_validation_callback_function(in_opt_validation_callback_function)
{
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_INSTANCE,
                                                  this);
}

/** Please see header for specification */
Anvil::Instance::~Instance()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_INSTANCE,
                                                    this);

    destroy();

    if (m_instance != VK_NULL_HANDLE)
    {
        lock();
        {
            vkDestroyInstance(m_instance,
                              nullptr /* pAllocator */);
        }
        unlock();

        m_instance = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
Anvil::InstanceUniquePtr Anvil::Instance::create(const std::string&              in_app_name,
                                                 const std::string&              in_engine_name,
                                                 DebugCallbackFunction           in_opt_validation_callback_proc,
                                                 bool                            in_mt_safe,
                                                 const std::vector<std::string>& in_opt_disallowed_instance_level_extensions)
{
    InstanceUniquePtr new_instance_ptr(nullptr,
                                       std::default_delete<Anvil::Instance>() );

    new_instance_ptr.reset(
        new Instance(
            in_app_name,
            in_engine_name,
            in_opt_validation_callback_proc,
            in_mt_safe)
    );

    new_instance_ptr->init(in_opt_disallowed_instance_level_extensions);

    return new_instance_ptr;
}

/** Entry-point for the Vulkan loader's debug callbacks.
 *
 *  For argument discussion, please see the extension specification
 **/
VkBool32 VKAPI_PTR Anvil::Instance::debug_callback_pfn_proc(VkDebugReportFlagsEXT      in_message_flags,
                                                            VkDebugReportObjectTypeEXT in_object_type,
                                                            uint64_t                   in_src_object,
                                                            size_t                     in_location,
                                                            int32_t                    in_msg_code,
                                                            const char*                in_layer_prefix_ptr,
                                                            const char*                in_message_ptr,
                                                            void*                      in_user_data)
{
    Anvil::Instance* instance_ptr = static_cast<Anvil::Instance*>(in_user_data);

    ANVIL_REDUNDANT_ARGUMENT(in_src_object);
    ANVIL_REDUNDANT_ARGUMENT(in_location);
    ANVIL_REDUNDANT_ARGUMENT(in_msg_code);
    ANVIL_REDUNDANT_ARGUMENT(in_user_data);

    return instance_ptr->m_validation_callback_function(in_message_flags,
                                                        in_object_type,
                                                        in_layer_prefix_ptr,
                                                        in_message_ptr);
}

/** Please see header for specification */
void Anvil::Instance::destroy()
{
    if (m_debug_callback_data != VK_NULL_HANDLE)
    {
        lock();
        {
            m_ext_debug_report_entrypoints.vkDestroyDebugReportCallbackEXT(m_instance,
                                                                           m_debug_callback_data,
                                                                           nullptr /* pAllocator */);
        }
        unlock();

        m_debug_callback_data = VK_NULL_HANDLE;
    }

    m_physical_devices.clear();

    #ifdef _DEBUG
    {
        /* Make sure no physical devices are still registered with Object Tracker at this point */
        auto object_manager_ptr = Anvil::ObjectTracker::get();

        if (object_manager_ptr->get_object_at_index(Anvil::OBJECT_TYPE_INSTANCE,
                                                    0) == nullptr)
        {
            anvil_assert(object_manager_ptr->get_object_at_index(Anvil::OBJECT_TYPE_PHYSICAL_DEVICE,
                                                                 0) == nullptr);
        }
    }
    #endif
}

/** Enumerates and caches all layers supported by the Vulkan Instance. */
void Anvil::Instance::enumerate_instance_layers()
{
    std::vector<VkLayerProperties> layer_props;
    uint32_t                       n_layers    = 0;
    VkResult                       result      = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Retrieve layer data */
    result = vkEnumerateInstanceLayerProperties(&n_layers,
                                                nullptr); /* pProperties */
    anvil_assert_vk_call_succeeded(result);

    layer_props.resize(n_layers + 1 /* global layer */);

    result = vkEnumerateInstanceLayerProperties(&n_layers,
                                               &layer_props[0]);

    anvil_assert_vk_call_succeeded(result);

    /* Convert raw layer props data to internal descriptors */
    for (uint32_t n_layer = 0;
                  n_layer < n_layers + 1;
                ++n_layer)
    {
        Anvil::Layer* layer_ptr = nullptr;

        if (n_layer < n_layers)
        {
            m_supported_layers.push_back(Anvil::Layer(layer_props[n_layer]) );

            layer_ptr = &m_supported_layers[n_layer];
        }

        enumerate_layer_extensions(layer_ptr);
    }
}

/** Enumerates all available layer extensions. The enumerated extensions will be stored
 *  in the specified _vulkan_layer descriptor.
 *
 *  @param in_layer_ptr Layer to enumerate the extensions for. If nullptr, device extensions
 *                      will be retrieved instead.
 **/
void Anvil::Instance::enumerate_layer_extensions(Anvil::Layer* in_layer_ptr)
{
    uint32_t n_extensions = 0;
    VkResult result       = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Check if the layer supports any extensions at all */
    const char* layer_name = nullptr;

    if (in_layer_ptr == nullptr)
    {
        in_layer_ptr = &m_global_layer;
    }

    layer_name = in_layer_ptr->name.c_str();
    result     = vkEnumerateInstanceExtensionProperties(layer_name,
                                                       &n_extensions,
                                                        nullptr); /* pProperties */

    anvil_assert_vk_call_succeeded(result);

    if (n_extensions > 0)
    {
        std::vector<VkExtensionProperties> extension_props;

        extension_props.resize(n_extensions);

        result = vkEnumerateInstanceExtensionProperties(layer_name,
                                                       &n_extensions,
                                                       &extension_props[0]);

        anvil_assert_vk_call_succeeded(result);

        /* Convert raw extension props data to internal descriptors */
        for (uint32_t n_extension = 0;
                      n_extension < n_extensions;
                    ++n_extension)
        {
            in_layer_ptr->extensions.push_back(extension_props[n_extension].extensionName);
        }
    }
}

/** Enumerates and caches all available physical devices. */
void Anvil::Instance::enumerate_physical_devices()
{
    std::vector<VkPhysicalDevice> devices;
    uint32_t                      n_physical_devices = 0;
    VkResult                      result             = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Retrieve physical device handles */
    result = vkEnumeratePhysicalDevices(m_instance,
                                       &n_physical_devices,
                                        nullptr); /* pPhysicalDevices */
    anvil_assert_vk_call_succeeded(result);

    if (n_physical_devices == 0)
    {
        fprintf(stderr,"No physical devices reported for the Vulkan instance");
        fflush (stderr);

        anvil_assert_fail();
    }

    devices.resize(n_physical_devices);

    result = vkEnumeratePhysicalDevices(m_instance,
                                       &n_physical_devices,
                                       &devices[0]);
    anvil_assert_vk_call_succeeded(result);

    /* Fill out internal physical device descriptors */
    for (unsigned int n_physical_device = 0;
                      n_physical_device < n_physical_devices;
                    ++n_physical_device)
    {
        std::unique_ptr<Anvil::PhysicalDevice> new_physical_device_ptr;

        new_physical_device_ptr = Anvil::PhysicalDevice::create(this,
                                      n_physical_device,
                                      devices[n_physical_device]);

        m_physical_devices.push_back(
            std::move(new_physical_device_ptr)
        );
    }
}

/** Please see header for specification */
const Anvil::ExtensionKHRExternalFenceCapabilitiesEntrypoints& Anvil::Instance::get_extension_khr_external_fence_capabilities_entrypoints() const
{
    anvil_assert(m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_external_fence_capabilities() );

    return m_khr_external_fence_capabilities_entrypoints;
}

/** Please see header for specification */
const Anvil::ExtensionKHRExternalMemoryCapabilitiesEntrypoints& Anvil::Instance::get_extension_khr_external_memory_capabilities_entrypoints() const
{
    anvil_assert(m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_external_memory_capabilities() );

    return m_khr_external_memory_capabilities_entrypoints;
}

/** Please see header for specification */
const Anvil::ExtensionKHRExternalSemaphoreCapabilitiesEntrypoints& Anvil::Instance::get_extension_khr_external_semaphore_capabilities_entrypoints() const
{
    anvil_assert(m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_external_semaphore_capabilities() );

    return m_khr_external_semaphore_capabilities_entrypoints;
}

/** Please see header for specification */
const Anvil::ExtensionKHRGetPhysicalDeviceProperties2& Anvil::Instance::get_extension_khr_get_physical_device_properties2_entrypoints() const
{
    anvil_assert(m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_get_physical_device_properties2() );

    return m_khr_get_physical_device_properties2_entrypoints;
}

/** Please see header for specification */
const Anvil::ExtensionKHRSurfaceEntrypoints& Anvil::Instance::get_extension_khr_surface_entrypoints() const
{
    anvil_assert(m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_surface() );

    return m_khr_surface_entrypoints;
}

#ifdef _WIN32
    #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        /** Please see header for specification */
        const Anvil::ExtensionKHRWin32SurfaceEntrypoints& Anvil::Instance::get_extension_khr_win32_surface_entrypoints() const
        {
            anvil_assert(m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_win32_surface() );

            return m_khr_win32_surface_entrypoints;
        }
    #endif
#else
    #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        /** Please see header for specification */
        const Anvil::ExtensionKHRXcbSurfaceEntrypoints& Anvil::Instance::get_extension_khr_xcb_surface_entrypoints() const
        {
            anvil_assert(m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_xcb_surface() );

            return m_khr_xcb_surface_entrypoints;
        }
    #endif
#endif

/** Initializes the wrapper. */
void Anvil::Instance::init(const std::vector<std::string>& in_disallowed_instance_level_extensions)
{
    VkApplicationInfo           app_info;
    VkInstanceCreateInfo        create_info;
    std::vector<const char*>    enabled_layers;
    std::map<std::string, bool> extension_enabled_status;
    size_t                      n_instance_layers        = 0;
    VkResult                    result                   = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Enumerate available layers */
    enumerate_instance_layers();

    /* Determine what extensions we need to request at instance creation time */
    static const char* desired_extensions_with_validation[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,

        #ifdef _WIN32
            #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
            #endif
        #else
            #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
                VK_KHR_XCB_SURFACE_EXTENSION_NAME,
            #endif
        #endif

        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };
    static const char* desired_extensions_without_validation[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,

        #ifdef _WIN32
            #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
            #endif
        #else
            #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
                VK_KHR_XCB_SURFACE_EXTENSION_NAME,
            #endif
        #endif
    };

    /* Set up the app info descriptor **/
    app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 0);
    app_info.applicationVersion = 0;
    app_info.engineVersion      = 0;
    app_info.pApplicationName   = m_app_name.c_str();
    app_info.pEngineName        = m_engine_name.c_str();
    app_info.pNext              = nullptr;
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    /* Set up the create info descriptor */
    memset(&create_info,
           0,
           sizeof(create_info) );

    n_instance_layers = static_cast<uint32_t>(m_supported_layers.size() );

    for (size_t  n_instance_layer = 0;
                 n_instance_layer < n_instance_layers;
               ++n_instance_layer)
    {
        const std::string& layer_description = m_supported_layers[n_instance_layer].description;
        const std::string& layer_name        = m_supported_layers[n_instance_layer].name;

        /* If validation is enabled and this is a layer which issues debug call-backs, cache it, so that
         * we can request for it at vkCreateInstance() call time */
        if (m_validation_callback_function       != nullptr          &&
            layer_description.find("Validation") != std::string::npos)
        {
            enabled_layers.push_back(layer_name.c_str() );
        }
    }

    {
        if (m_validation_callback_function != nullptr)
        {
            for (uint32_t n_extension = 0;
                          n_extension < sizeof(desired_extensions_with_validation) / sizeof(desired_extensions_with_validation[0]);
                        ++n_extension)
            {
                if (is_instance_extension_supported(desired_extensions_with_validation[n_extension]))
                {
                    extension_enabled_status[desired_extensions_with_validation[n_extension] ] = true;
                }
            }
        }
        else
        {
            for (uint32_t n_extension = 0;
                          n_extension < sizeof(desired_extensions_without_validation) / sizeof(desired_extensions_without_validation[0]);
                        ++n_extension)
            {
                if (is_instance_extension_supported(desired_extensions_without_validation[n_extension]))
                {
                    extension_enabled_status[desired_extensions_without_validation[n_extension] ] = true;
                }
            }
        }

        /* Enable known instance-level extensions by default */
        if (is_instance_extension_supported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) )
        {
            extension_enabled_status[VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME] = true;
        }

        if (is_instance_extension_supported(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME) )
        {
            extension_enabled_status[VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME] = true;
        }

        if (is_instance_extension_supported(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) )
        {
            extension_enabled_status[VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME] = true;
        }

        if (is_instance_extension_supported(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME) )
        {
            extension_enabled_status[VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME] = true;
        }

        /* Filter out undesired extensions */
        for (const auto& current_extension_name : in_disallowed_instance_level_extensions)
        {
            auto ext_iterator = extension_enabled_status.find(current_extension_name);

            if (ext_iterator != extension_enabled_status.end() )
            {
                extension_enabled_status.erase(ext_iterator);
            }
        }

        m_enabled_extensions_info_ptr = Anvil::ExtensionInfo<bool>::create_instance_extension_info(extension_enabled_status,
                                                                                                   false); /* in_unspecified_extension_name_value */
    }

    /* We're ready to create a new Vulkan instance */
    std::vector<const char*> enabled_extensions_raw;

    for (auto& ext_name : extension_enabled_status)
    {
        enabled_extensions_raw.push_back(ext_name.first.c_str() );
    }

    create_info.enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions_raw.size() );
    create_info.enabledLayerCount       = static_cast<uint32_t>(enabled_layers.size() );
    create_info.flags                   = 0;
    create_info.pApplicationInfo        = &app_info;
    create_info.pNext                   = nullptr;
    create_info.ppEnabledExtensionNames = (enabled_extensions_raw.size() > 0) ? &enabled_extensions_raw[0] : nullptr;
    create_info.ppEnabledLayerNames     = (enabled_layers.size()         > 0) ? &enabled_layers        [0] : nullptr;
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    result = vkCreateInstance(&create_info,
                              nullptr, /* pAllocator */
                              &m_instance);

    anvil_assert_vk_call_succeeded(result);

    /* Continue initializing */
    init_func_pointers();

    if (m_validation_callback_function != nullptr)
    {
        init_debug_callbacks();
    }

    enumerate_physical_devices();
}

/** Initializes debug callback support. */
void Anvil::Instance::init_debug_callbacks()
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Set up the debug call-backs, while we're at it */
    VkDebugReportCallbackCreateInfoEXT debug_report_callback_create_info;

    debug_report_callback_create_info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debug_report_callback_create_info.pfnCallback = debug_callback_pfn_proc;
    debug_report_callback_create_info.pNext       = nullptr;
    debug_report_callback_create_info.pUserData   = this;
    debug_report_callback_create_info.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

    result = m_ext_debug_report_entrypoints.vkCreateDebugReportCallbackEXT(m_instance,
                                                                          &debug_report_callback_create_info,
                                                                           nullptr, /* pAllocator */
                                                                          &m_debug_callback_data);
    anvil_assert_vk_call_succeeded(result);
}

/** Initializes all required instance-level function pointers. */
void Anvil::Instance::init_func_pointers()
{
    m_khr_surface_entrypoints.vkDestroySurfaceKHR                       = reinterpret_cast<PFN_vkDestroySurfaceKHR>                      (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                "vkDestroySurfaceKHR") );
    m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                "vkGetPhysicalDeviceSurfaceCapabilitiesKHR") );
    m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceFormatsKHR      = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>     (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                "vkGetPhysicalDeviceSurfaceFormatsKHR") );
    m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                "vkGetPhysicalDeviceSurfacePresentModesKHR") );
    m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceSupportKHR      = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>     (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                "vkGetPhysicalDeviceSurfaceSupportKHR") );

    m_ext_debug_report_entrypoints.vkCreateDebugReportCallbackEXT  = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT> (vkGetInstanceProcAddr(m_instance,
                                                                                                                           "vkCreateDebugReportCallbackEXT") );
    m_ext_debug_report_entrypoints.vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_instance,
                                                                                                                           "vkDestroyDebugReportCallbackEXT") );

    anvil_assert(m_khr_surface_entrypoints.vkDestroySurfaceKHR                       != nullptr);
    anvil_assert(m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceCapabilitiesKHR != nullptr);
    anvil_assert(m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceFormatsKHR      != nullptr);
    anvil_assert(m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfacePresentModesKHR != nullptr);
    anvil_assert(m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceSupportKHR      != nullptr);

    #ifdef _WIN32
    {
        #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        {
            m_khr_win32_surface_entrypoints.vkCreateWin32SurfaceKHR                        = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>                       (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                  "vkCreateWin32SurfaceKHR") );
            m_khr_win32_surface_entrypoints.vkGetPhysicalDeviceWin32PresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                  "vkGetPhysicalDeviceWin32PresentationSupportKHR") );

            anvil_assert(m_khr_win32_surface_entrypoints.vkCreateWin32SurfaceKHR                        != nullptr);
            anvil_assert(m_khr_win32_surface_entrypoints.vkGetPhysicalDeviceWin32PresentationSupportKHR != nullptr);
        }
        #endif
    }
    #else
    {
        #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        {
            m_khr_xcb_surface_entrypoints.vkCreateXcbSurfaceKHR = reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                    "vkCreateXcbSurfaceKHR") );

            anvil_assert(m_khr_xcb_surface_entrypoints.vkCreateXcbSurfaceKHR != nullptr);
        }
        #endif
    }
    #endif

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_get_physical_device_properties2() )
    {
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceFeatures2KHR                    = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>                   (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                              "vkGetPhysicalDeviceFeatures2KHR") );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceFormatProperties2KHR            = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties2KHR>           (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                              "vkGetPhysicalDeviceFormatProperties2KHR") );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceImageFormatProperties2KHR       = reinterpret_cast<PFN_vkGetPhysicalDeviceImageFormatProperties2KHR>      (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                              "vkGetPhysicalDeviceImageFormatProperties2KHR") );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceMemoryProperties2KHR            = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2KHR>           (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                              "vkGetPhysicalDeviceMemoryProperties2KHR") );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceProperties2KHR                  = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>                 (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                              "vkGetPhysicalDeviceProperties2KHR") );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceQueueFamilyProperties2KHR       = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR>      (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                              "vkGetPhysicalDeviceQueueFamilyProperties2KHR") );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceSparseImageFormatProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                              "vkGetPhysicalDeviceSparseImageFormatProperties2KHR") );

        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceFeatures2KHR                    != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceFormatProperties2KHR            != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceImageFormatProperties2KHR       != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceMemoryProperties2KHR            != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceProperties2KHR                  != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceQueueFamilyProperties2KHR       != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceSparseImageFormatProperties2KHR != nullptr);
    }

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_external_fence_capabilities() )
    {
        m_khr_external_fence_capabilities_entrypoints.vkGetPhysicalDeviceExternalFencePropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                "vkGetPhysicalDeviceExternalFencePropertiesKHR") );

        anvil_assert(m_khr_external_fence_capabilities_entrypoints.vkGetPhysicalDeviceExternalFencePropertiesKHR!= nullptr);
    }

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_external_memory_capabilities() )
    {
        m_khr_external_memory_capabilities_entrypoints.vkGetPhysicalDeviceExternalBufferPropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                   "vkGetPhysicalDeviceExternalBufferPropertiesKHR") );

        anvil_assert(m_khr_external_memory_capabilities_entrypoints.vkGetPhysicalDeviceExternalBufferPropertiesKHR != nullptr);
    }

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_external_semaphore_capabilities() )
    {
        m_khr_external_semaphore_capabilities_entrypoints.vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                            "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR") );

        anvil_assert(m_khr_external_semaphore_capabilities_entrypoints.vkGetPhysicalDeviceExternalSemaphorePropertiesKHR!= nullptr);
    }
}

/** Please see header for specification */
bool Anvil::Instance::is_instance_extension_enabled(const char* in_extension_name) const
{
    return m_enabled_extensions_info_ptr->get_instance_extension_info()->by_name(in_extension_name);
}

/** Please see header for specification */
bool Anvil::Instance::is_instance_extension_supported(const char* in_extension_name) const
{
    return std::find(m_global_layer.extensions.begin(),
                     m_global_layer.extensions.end(),
                     in_extension_name) != m_global_layer.extensions.end();
}
