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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "misc/shader_module_cache.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"

/** Please see header for specification */
Anvil::Instance::Instance(const std::string&           in_app_name,
                          const std::string&           in_engine_name,
                          PFNINSTANCEDEBUGCALLBACKPROC in_opt_pfn_validation_callback_proc,
                          void*                        in_validation_proc_user_arg)
    :m_app_name                    (in_app_name),
     m_debug_callback_data         (0),
     m_engine_name                 (in_engine_name),
     m_global_layer                (""),
     m_pfn_validation_callback_proc(in_opt_pfn_validation_callback_proc),
     m_validation_proc_user_arg    (in_validation_proc_user_arg)
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
        vkDestroyInstance(m_instance,
                          nullptr /* pAllocator */);

        m_instance = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
std::shared_ptr<Anvil::Instance> Anvil::Instance::create(const std::string&           in_app_name,
                                                         const std::string&           in_engine_name,
                                                         PFNINSTANCEDEBUGCALLBACKPROC in_opt_pfn_validation_callback_proc,
                                                         void*                        in_validation_proc_user_arg)
{
    std::shared_ptr<Anvil::Instance> new_instance_ptr;

    new_instance_ptr = std::shared_ptr<Anvil::Instance>(
        new Instance(
            in_app_name,
            in_engine_name,
            in_opt_pfn_validation_callback_proc,
            in_validation_proc_user_arg)
    );

    new_instance_ptr->init();

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

    return instance_ptr->m_pfn_validation_callback_proc(in_message_flags,
                                                        in_object_type,
                                                        in_layer_prefix_ptr,
                                                        in_message_ptr,
                                                        instance_ptr->m_validation_proc_user_arg);
}

/** Please see header for specification */
void Anvil::Instance::destroy()
{
    if (m_debug_callback_data != VK_NULL_HANDLE)
    {
        m_ext_debug_report_entrypoints.vkDestroyDebugReportCallbackEXT(m_instance,
                                                                       m_debug_callback_data,
                                                                       nullptr /* pAllocator */);

        m_debug_callback_data = VK_NULL_HANDLE;
    }

    while (m_physical_devices.size() > 0)
    {
        m_physical_devices.back()->destroy();
    }
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
            in_layer_ptr->extensions.push_back(Anvil::Extension(extension_props[n_extension]) );
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
        std::shared_ptr<Anvil::PhysicalDevice> new_physical_device_ptr;

        Anvil::PhysicalDevice::create(shared_from_this(),
                                      n_physical_device,
                                      devices[n_physical_device]);
    }
}

/** Please see header for specification */
const Anvil::ExtensionKHRGetPhysicalDeviceProperties2& Anvil::Instance::get_extension_khr_get_physical_device_properties2_entrypoints() const
{
    anvil_assert(std::find(m_enabled_extensions.begin(),
                           m_enabled_extensions.end(),
                           VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) != m_enabled_extensions.end() );

    return m_khr_get_physical_device_properties2_entrypoints;
}

/** Please see header for specification */
const Anvil::ExtensionKHRSurfaceEntrypoints& Anvil::Instance::get_extension_khr_surface_entrypoints() const
{
    anvil_assert(std::find(m_enabled_extensions.begin(),
                           m_enabled_extensions.end(),
                           VK_KHR_SURFACE_EXTENSION_NAME) != m_enabled_extensions.end() );

    return m_khr_surface_entrypoints;
}

#ifdef _WIN32
    #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        /** Please see header for specification */
        const Anvil::ExtensionKHRWin32SurfaceEntrypoints& Anvil::Instance::get_extension_khr_win32_surface_entrypoints() const
        {
            anvil_assert(std::find(m_enabled_extensions.begin(),
                                   m_enabled_extensions.end(),
                                   VK_KHR_WIN32_SURFACE_EXTENSION_NAME) != m_enabled_extensions.end() );

            return m_khr_win32_surface_entrypoints;
        }
    #endif
#else
    #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        /** Please see header for specification */
        const Anvil::ExtensionKHRXcbSurfaceEntrypoints& Anvil::Instance::get_extension_khr_xcb_surface_entrypoints() const
        {
            anvil_assert(std::find(m_enabled_extensions.begin(),
                                   m_enabled_extensions.end(),
                                   VK_KHR_XCB_SURFACE_EXTENSION_NAME) != m_enabled_extensions.end() );

            return m_khr_xcb_surface_entrypoints;
        }
    #endif
#endif

/** Initializes the wrapper. */
void Anvil::Instance::init()
{
    VkApplicationInfo        app_info;
    VkInstanceCreateInfo     create_info;
    std::vector<const char*> enabled_layers;
    size_t                   n_instance_layers = 0;
    VkResult                 result            = VK_ERROR_INITIALIZATION_FAILED;

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

    if (m_pfn_validation_callback_proc != nullptr)
    {
        for (uint32_t n_extension = 0;
                      n_extension < sizeof(desired_extensions_with_validation) / sizeof(desired_extensions_with_validation[0]);
                    ++n_extension)
        {
            if (is_instance_extension_supported(desired_extensions_with_validation[n_extension]))
            {
                m_enabled_extensions.push_back(desired_extensions_with_validation[n_extension]);
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
                m_enabled_extensions.push_back(desired_extensions_without_validation[n_extension]);
            }
        }
    }

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
        const std::vector<Anvil::Extension>& layer_extensions = m_supported_layers[n_instance_layer].extensions;
        const std::string&                   layer_name       = m_supported_layers[n_instance_layer].name;

        /* If validation is enabled and this is a layer which issues debug call-backs, cache it, so that
         * we can request for it at vkCreateInstance() call time */
        if (m_pfn_validation_callback_proc != nullptr                  &&
            std::find(layer_extensions.begin(),
                      layer_extensions.end(),
                      VK_EXT_DEBUG_REPORT_EXTENSION_NAME) != layer_extensions.end() )
        {
            enabled_layers.push_back(layer_name.c_str() );
        }
    }

    /* Enable known instance-level extensions by default */
    if (is_instance_extension_supported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) )
    {
        m_enabled_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    /* We're ready to create a new Vulkan instance */
    std::vector<const char*> enabled_extensions_raw;

    for (auto& ext_name : m_enabled_extensions)
    {
        enabled_extensions_raw.push_back(ext_name.c_str() );
    }

    create_info.enabledExtensionCount   = static_cast<uint32_t>(m_enabled_extensions.size() );
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

    if (m_pfn_validation_callback_proc != nullptr)
    {
        init_debug_callbacks();
    }

    enumerate_physical_devices();

    m_shader_module_cache_ptr = Anvil::ShaderModuleCache::create();
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

    if (std::find(m_enabled_extensions.begin(),
                  m_enabled_extensions.end(),
                  VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) != m_enabled_extensions.end() )
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
}

/** Please see header for specification */
bool Anvil::Instance::is_instance_extension_supported(const char* in_extension_name) const
{
    return std::find(m_global_layer.extensions.begin(),
                     m_global_layer.extensions.end(),
                     in_extension_name) != m_global_layer.extensions.end();
}

/** Please see header for specification */
void Anvil::Instance::register_physical_device(std::shared_ptr<Anvil::PhysicalDevice> in_physical_device_ptr)
{
    auto iterator = std::find(m_physical_devices.begin(),
                              m_physical_devices.end(),
                              in_physical_device_ptr);

    anvil_assert(iterator == m_physical_devices.end() );

    if (iterator == m_physical_devices.end() )
    {
        m_physical_devices.push_back(in_physical_device_ptr);
    }
}

/** Please see header for specification */
void Anvil::Instance::unregister_physical_device(std::shared_ptr<Anvil::PhysicalDevice> in_physical_device_ptr)
{
    auto iterator = std::find(m_physical_devices.begin(),
                              m_physical_devices.end(),
                              in_physical_device_ptr);

    anvil_assert(iterator != m_physical_devices.end() );

    if (iterator != m_physical_devices.end() )
    {
        m_physical_devices.erase(iterator);
    }
}