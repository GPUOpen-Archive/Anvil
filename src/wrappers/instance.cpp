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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/instance.h"

/** Please see header for specification */
Anvil::Instance::Instance(const char*                  app_name,
                          const char*                  engine_name,
                          PFNINSTANCEDEBUGCALLBACKPROC opt_pfn_validation_callback_proc,
                          void*                        validation_proc_user_arg)
    :m_app_name                                      (app_name),
     m_debug_callback_data                           (0),
     m_engine_name                                   (engine_name),
     m_pfn_validation_callback_proc                  (opt_pfn_validation_callback_proc),
     m_validation_proc_user_arg                      (validation_proc_user_arg),
     m_vkCreateDebugReportCallbackEXT                (nullptr),
#ifdef _WIN32
     m_vkCreateWin32SurfaceKHR                       (nullptr),
#else
     m_vkCreateXcbSurfaceKHR                         (nullptr),
#endif
     m_vkDestroyDebugReportCallbackEXT               (nullptr),
     m_vkDestroySurfaceKHR                           (nullptr),
     m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR     (nullptr),
     m_vkGetPhysicalDeviceSurfaceFormatsKHR          (nullptr),
     m_vkGetPhysicalDeviceSurfacePresentModesKHR     (nullptr),
     m_vkGetPhysicalDeviceSurfaceSupportKHR          (nullptr)
#ifdef _WIN32
    ,m_vkGetPhysicalDeviceWin32PresentationSupportKHR(nullptr)
#endif
{
    anvil_assert(app_name    != nullptr);
    anvil_assert(engine_name != nullptr);

    init();

    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_INSTANCE,
                                                  this);
}

/** Please see header for specification */
Anvil::Instance::~Instance()
{
    if (m_debug_callback_data != VK_NULL_HANDLE)
    {
        m_vkDestroyDebugReportCallbackEXT(m_instance,
                                          m_debug_callback_data,
                                          nullptr /* pAllocator */);

        m_debug_callback_data = VK_NULL_HANDLE;
    }

    while (m_physical_devices.size() > 0)
    {
        m_physical_devices.back()->release();

        m_physical_devices.pop_back();
    }

    if (m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_instance,
                          nullptr /* pAllocator */);

        m_instance = VK_NULL_HANDLE;
    }

    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_INSTANCE,
                                                    this);
}

/** Entry-point for the Vulkan loader's debug callbacks.
 *
 *  For argument discussion, please see the extension specification
 **/
VkBool32 VKAPI_PTR Anvil::Instance::debug_callback_pfn_proc(VkDebugReportFlagsEXT      message_flags,
                                                            VkDebugReportObjectTypeEXT object_type,
                                                            uint64_t                   src_object,
                                                            size_t                     location,
                                                            int32_t                    msg_code,
                                                            const char*                layer_prefix_ptr,
                                                            const char*                message_ptr,
                                                            void*                      user_data)
{
    Anvil::Instance* instance_ptr = static_cast<Anvil::Instance*>(user_data);

    return instance_ptr->m_pfn_validation_callback_proc(message_flags,
                                                        object_type,
                                                        layer_prefix_ptr,
                                                        message_ptr,
                                                        instance_ptr->m_validation_proc_user_arg);
}

/** Enumerates and caches all layers supported by the Vulkan Instance. */
void Anvil::Instance::enumerate_instance_layers()
{
    VkLayerProperties* layer_props_ptr = nullptr;
    uint32_t           n_layers        = 0;
    VkResult           result;

    /* Retrieve layer data */
    result = vkEnumerateInstanceLayerProperties(&n_layers,
                                                nullptr); /* pProperties */
    anvil_assert_vk_call_succeeded(result);

    layer_props_ptr = new VkLayerProperties[n_layers + 1 /* global layer */];
    anvil_assert(layer_props_ptr != nullptr);

    result = vkEnumerateInstanceLayerProperties(&n_layers,
                                                 layer_props_ptr);

    anvil_assert_vk_call_succeeded(result);

    /* Convert raw layer props data to internal descriptors */
    for (uint32_t n_layer = 0;
                  n_layer < n_layers;
                ++n_layer)
    {
        Anvil::Layer* layer_ptr = nullptr;

        if (n_layer < n_layers)
        {
            m_supported_layers.push_back(Anvil::Layer(layer_props_ptr[n_layer]) );

            layer_ptr = &m_supported_layers[n_layer];
        }

        enumerate_layer_extensions(layer_ptr);
    }

    /* Release the raw layer data */
    delete [] layer_props_ptr;
    layer_props_ptr = nullptr;
}

/** Enumerates all available layer extensions. The enumerated extensions will be stored
 *  in the specified _vulkan_layer descriptor.
 *
 *  @param layer_ptr Layer to enumerate the extensions for. If nullptr, device extensions
 *                   will be retrieved instead.
 **/
void Anvil::Instance::enumerate_layer_extensions(Anvil::Layer* layer_ptr)
{
    VkExtensionProperties* extension_props_ptr = nullptr;
    uint32_t               n_extensions        = 0;
    VkResult               result;

    /* Check if the layer supports any extensions  at all*/
    const char* layer_name = (layer_ptr != nullptr) ? layer_ptr->name.c_str()
                                                 : nullptr;

    result = vkEnumerateInstanceExtensionProperties(layer_name,
                                                   &n_extensions,
                                                    nullptr); /* pProperties */
    anvil_assert_vk_call_succeeded(result);

    if (n_extensions > 0)
    {
        extension_props_ptr = new VkExtensionProperties[n_extensions];
        anvil_assert(extension_props_ptr != nullptr);

        result = vkEnumerateInstanceExtensionProperties(layer_name,
                                                       &n_extensions,
                                                        extension_props_ptr);

        anvil_assert_vk_call_succeeded(result);

        /* Convert raw extension props data to internal descriptors */
        for (uint32_t n_extension = 0;
                      n_extension < n_extensions;
                    ++n_extension)
        {
            layer_ptr->extensions.push_back(Anvil::Extension(extension_props_ptr[n_extension]) );
        }

        /* Release the raw layer data */
        delete [] extension_props_ptr;
        extension_props_ptr = nullptr;
    }
}

/** Enumerates and caches all available physical devices. */
void Anvil::Instance::enumerate_physical_devices()
{
    VkPhysicalDevice* devices            = nullptr;
    uint32_t          n_physical_devices = 0;
    VkResult          result;

    /* Retrieve physical device handles */
    result = vkEnumeratePhysicalDevices(m_instance,
                                       &n_physical_devices,
                                        nullptr); /* pPhysicalDevices */
    anvil_assert_vk_call_succeeded(result);

    if (n_physical_devices == 0)
    {
#ifdef _WIN32
        MessageBox(HWND_DESKTOP,
                   "No physical devices reported for the Vulkan instance",
                   "Error",
                   MB_OK | MB_ICONERROR);

        exit(1);
#else
        fprintf(stderr,"No physical devices reported for the Vulkan instance");
        fflush(stderr);
        exit(1);
#endif
    }

    devices = new VkPhysicalDevice[n_physical_devices];
    anvil_assert(devices != nullptr);

    result = vkEnumeratePhysicalDevices(m_instance,
                                       &n_physical_devices,
                                        devices);
    anvil_assert_vk_call_succeeded(result);

    /* Fill out internal physical device descriptors */
    for (unsigned int n_physical_device = 0;
                      n_physical_device < n_physical_devices;
                    ++n_physical_device)
    {
        m_physical_devices.push_back(new Anvil::PhysicalDevice(this,
                                                                n_physical_device,
                                                                devices[n_physical_device]) );
    }

    /* Clean up */
    delete [] devices;
    devices = nullptr;
}

/** Initializes the wrapper. */
void Anvil::Instance::init()
{
    VkApplicationInfo    app_info;
    VkInstanceCreateInfo create_info;
    const char**         enabled_layer_name_ptrs = nullptr;
    uint32_t             n_required_extensions   = 0;
    VkResult             result;
    const char**         required_extensions     = nullptr;

    /* Determine what extensions we need to request at instance creation time */
    static const char* required_extensions_with_validation[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,

        #ifdef _WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        #else
            VK_KHR_XCB_SURFACE_EXTENSION_NAME,
        #endif

        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };
    static const char* required_extensions_without_validation[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,

        #ifdef _WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        #else
            VK_KHR_XCB_SURFACE_EXTENSION_NAME,
        #endif
    };

    if (m_pfn_validation_callback_proc != nullptr)
    {
        n_required_extensions = sizeof(required_extensions_with_validation) /
                                sizeof(required_extensions_with_validation[0]);
        required_extensions   = required_extensions_with_validation;
    }
    else
    {
        n_required_extensions = sizeof(required_extensions_without_validation) /
                                sizeof(required_extensions_without_validation[0]);
        required_extensions   = required_extensions_without_validation;
    }

    /* Set up the app info descriptor **/
    app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 0);
    app_info.applicationVersion = 0;
    app_info.engineVersion      = 0;
    app_info.pApplicationName   = m_app_name;
    app_info.pEngineName        = m_engine_name;
    app_info.pNext              = nullptr;
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    /* Set up the create info descriptor */
    memset(&create_info,
           0,
           sizeof(create_info) );

    create_info.enabledExtensionCount = n_required_extensions;

    if (m_pfn_validation_callback_proc != nullptr)
    {
        size_t   n_instance_layers = 0;
        uint32_t n_reported_layers = 0;

        /* Also set up the layer name array */
        enumerate_instance_layers();

        n_instance_layers = static_cast<uint32_t>(m_supported_layers.size() );

        if (n_instance_layers > 0)
        {
            enabled_layer_name_ptrs = new const char*[n_instance_layers];
            anvil_assert(enabled_layer_name_ptrs != nullptr);

            for (size_t  n_instance_layer = 0;
                         n_instance_layer < n_instance_layers;
                       ++n_instance_layer)
            {
                const std::vector<Anvil::Extension>& layer_extensions = m_supported_layers[n_instance_layer].extensions;
                const std::string&                    layer_name       = m_supported_layers[n_instance_layer].name;

                /* Is this really a validation layer? */
                if (std::find(layer_extensions.begin(),
                              layer_extensions.end(),
                              "VK_EXT_debug_report") != layer_extensions.end() )
                {
                    enabled_layer_name_ptrs[n_reported_layers] = layer_name.c_str();

                    n_reported_layers++;
                }
            }
        }

        create_info.enabledLayerCount   = (uint32_t) n_reported_layers;
        create_info.ppEnabledLayerNames = enabled_layer_name_ptrs;
    }

    create_info.flags                   = 0;
    create_info.pApplicationInfo        = &app_info;
    create_info.pNext                   = nullptr;
    create_info.ppEnabledExtensionNames = required_extensions;
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    /* Create a new Vulkan instance */
    result = vkCreateInstance(&create_info,
                              nullptr, /* pAllocator */
                              &m_instance);
    anvil_assert_vk_call_succeeded(result);

    /* Continue initializing */
    enumerate_physical_devices();
    init_func_pointers        ();

    if (m_pfn_validation_callback_proc != nullptr)
    {
        init_debug_callbacks();
    }

    /* Clean up */
    if (enabled_layer_name_ptrs != nullptr)
    {
        delete [] enabled_layer_name_ptrs;

        enabled_layer_name_ptrs = nullptr;
    }
}

/** Initializes debug callback support. */
void Anvil::Instance::init_debug_callbacks()
{
    VkResult result;

    /* Set up the debug call-backs, while we're at it */
    VkDebugReportCallbackCreateInfoEXT debug_report_callback_create_info;

    debug_report_callback_create_info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debug_report_callback_create_info.pfnCallback = debug_callback_pfn_proc;
    debug_report_callback_create_info.pNext       = nullptr;
    debug_report_callback_create_info.pUserData   = this;
    debug_report_callback_create_info.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

    result = m_vkCreateDebugReportCallbackEXT(m_instance,
                                             &debug_report_callback_create_info,
                                              nullptr, /* pAllocator */
                                             &m_debug_callback_data);
    anvil_assert_vk_call_succeeded(result);
}

/** Initializes all required instance-level function pointers. */
void Anvil::Instance::init_func_pointers()
{
    m_vkDestroySurfaceKHR                       = reinterpret_cast<PFN_vkDestroySurfaceKHR>                      (vkGetInstanceProcAddr(m_instance,
                                                                                                                                        "vkDestroySurfaceKHR") );
    m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                        "vkGetPhysicalDeviceSurfaceCapabilitiesKHR") );
    m_vkGetPhysicalDeviceSurfaceFormatsKHR      = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>     (vkGetInstanceProcAddr(m_instance,
                                                                                                                                        "vkGetPhysicalDeviceSurfaceFormatsKHR") );
    m_vkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                        "vkGetPhysicalDeviceSurfacePresentModesKHR") );
    m_vkGetPhysicalDeviceSurfaceSupportKHR      = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>     (vkGetInstanceProcAddr(m_instance,
                                                                                                                                        "vkGetPhysicalDeviceSurfaceSupportKHR") );

    m_vkCreateDebugReportCallbackEXT  = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT> (vkGetInstanceProcAddr(m_instance,
                                                                                                                    "vkCreateDebugReportCallbackEXT") );
    m_vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_instance,
                                                                                                                    "vkDestroyDebugReportCallbackEXT") );

    anvil_assert(m_vkDestroySurfaceKHR                       != nullptr);
    anvil_assert(m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR != nullptr);
    anvil_assert(m_vkGetPhysicalDeviceSurfaceFormatsKHR      != nullptr);
    anvil_assert(m_vkGetPhysicalDeviceSurfacePresentModesKHR != nullptr);
    anvil_assert(m_vkGetPhysicalDeviceSurfaceSupportKHR      != nullptr);

    #ifdef _WIN32
    {
        m_vkCreateWin32SurfaceKHR                        = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>                       (vkGetInstanceProcAddr(m_instance,
                                                                                                                                                      "vkCreateWin32SurfaceKHR") );
        m_vkGetPhysicalDeviceWin32PresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                                                                      "vkGetPhysicalDeviceWin32PresentationSupportKHR") );

        anvil_assert(m_vkCreateWin32SurfaceKHR                        != nullptr);
        anvil_assert(m_vkGetPhysicalDeviceWin32PresentationSupportKHR != nullptr);
    }
    #else
    {
        m_vkCreateXcbSurfaceKHR = reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(vkGetInstanceProcAddr(m_instance,
                                                                                                    "vkCreateXcbSurfaceKHR") );

        anvil_assert(m_vkCreateXcbSurfaceKHR != nullptr);
    }
    #endif
}

/** Please see header for specification */
bool Anvil::Instance::is_instance_extension_supported(const char* extension_name) const
{
    return std::find(m_extensions.begin(),
                     m_extensions.end(),
                     extension_name) != m_extensions.end();
}
