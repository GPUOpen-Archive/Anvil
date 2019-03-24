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
#include "misc/debug_messenger_create_info.h"
#include "misc/object_tracker.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"

static bool                    g_core_func_ptrs_inited     = false;
static bool                    g_instance_func_ptrs_inited = false;
static std::mutex              g_vk_func_ptr_init_mutex;
static Anvil::LibraryUniquePtr g_vk_library_ptr;


/** Please see header for specification */
Anvil::Instance::Instance(Anvil::InstanceCreateInfoUniquePtr in_create_info_ptr)
    :MTSafetySupportProvider(in_create_info_ptr->is_mt_safe() ),
     m_api_version          (APIVersion::UNKNOWN),
     m_debug_messenger_ptr  (Anvil::DebugMessengerUniquePtr(nullptr, std::default_delete<Anvil::DebugMessenger>() )),
     m_global_layer         (""),
     m_instance             (VK_NULL_HANDLE)
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::INSTANCE,
                                                  this);
}

/** Please see header for specification */
Anvil::Instance::~Instance()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::INSTANCE,
                                                    this);

    destroy();

    if (m_instance != VK_NULL_HANDLE)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyInstance(m_instance,
                                             nullptr /* pAllocator */);
        }
        unlock();

        m_instance = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
Anvil::InstanceUniquePtr Anvil::Instance::create(Anvil::InstanceCreateInfoUniquePtr in_create_info_ptr)
{
    InstanceUniquePtr new_instance_ptr(nullptr,
                                       std::default_delete<Anvil::Instance>() );

    new_instance_ptr.reset(
        new Instance(std::move(in_create_info_ptr) )
    );

    if (!new_instance_ptr->init() )
    {
        new_instance_ptr.reset();
    }

    return new_instance_ptr;
}

/** Entry-point for the Vulkan loader's debug callbacks.
 *
 *  For argument discussion, please see the extension specification
 **/
void Anvil::Instance::debug_callback_handler(const Anvil::DebugMessageSeverityFlagBits& in_severity,
                                             const char*                                in_message_ptr)
{
    get_create_info_ptr()->get_validation_callback()(in_severity,
                                                     in_message_ptr);
}

/** Please see header for specification */
void Anvil::Instance::destroy()
{
    m_debug_messenger_ptr.reset   ();
    m_physical_devices.clear      ();
    m_physical_device_groups.clear();

    #ifdef _DEBUG
    {
        /* Make sure no physical devices are still registered with Object Tracker at this point */
        auto object_manager_ptr = Anvil::ObjectTracker::get();

        if (object_manager_ptr->get_object_at_index(Anvil::ObjectType::INSTANCE,
                                                    0) == nullptr)
        {
            anvil_assert(object_manager_ptr->get_object_at_index(Anvil::ObjectType::PHYSICAL_DEVICE,
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
    result = Anvil::Vulkan::vkEnumerateInstanceLayerProperties(&n_layers,
                                                               nullptr); /* pProperties */
    anvil_assert_vk_call_succeeded(result);

    layer_props.resize(n_layers + 1 /* global layer */);

    result = Anvil::Vulkan::vkEnumerateInstanceLayerProperties(&n_layers,
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

    if (in_layer_ptr->name != "")
    {
        layer_name = in_layer_ptr->name.c_str();
    }

    result = Anvil::Vulkan::vkEnumerateInstanceExtensionProperties(layer_name,
                                                                  &n_extensions,
                                                                   nullptr); /* pProperties */

    anvil_assert_vk_call_succeeded(result);

    if (n_extensions > 0)
    {
        std::vector<VkExtensionProperties> extension_props;

        extension_props.resize(n_extensions);

        result = Anvil::Vulkan::vkEnumerateInstanceExtensionProperties(layer_name,
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

/** Enumerates and caches all available physical device groups. */
void Anvil::Instance::enumerate_physical_device_groups()
{
    uint32_t n_physical_device_groups = 0;
    VkResult result                   = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Enumerate available physical device groups and cache them for further use. */
    result = m_khr_device_group_creation_entrypoints.vkEnumeratePhysicalDeviceGroupsKHR(m_instance,
                                                                                        &n_physical_device_groups,
                                                                                        nullptr); /* pPhysicalDeviceGroupProperties */

    if (n_physical_device_groups > 0)
    {
        std::vector<VkPhysicalDeviceGroupPropertiesKHR> device_group_props(n_physical_device_groups);

        for (uint32_t n_physical_device_group = 0;
                      n_physical_device_group < n_physical_device_groups;
                    ++n_physical_device_group)
        {
            auto& current_props = device_group_props.at(n_physical_device_group);

            current_props.pNext = nullptr;
            current_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES_KHR;
        }

        result = m_khr_device_group_creation_entrypoints.vkEnumeratePhysicalDeviceGroupsKHR(m_instance,
                                                                                            &n_physical_device_groups,
                                                                                            &device_group_props[0]);
        anvil_assert_vk_call_succeeded(result);

        for (uint32_t n_group = 0;
                      n_group < n_physical_device_groups;
                    ++n_group)
        {
            Anvil::PhysicalDeviceGroup new_group;
            const auto&                this_group_props = device_group_props.at(n_group);

            anvil_assert(this_group_props.physicalDeviceCount > 0);

            for (uint32_t n_physical_device = 0;
                          n_physical_device < this_group_props.physicalDeviceCount;
                        ++n_physical_device)
            {
                auto physical_device_iterator = std::find(m_physical_devices.begin(),
                                                          m_physical_devices.end(),
                                                          this_group_props.physicalDevices[n_physical_device]);

                anvil_assert(physical_device_iterator != m_physical_devices.end() );
                if (physical_device_iterator != m_physical_devices.end() )
                {
                    new_group.physical_device_ptrs.push_back(physical_device_iterator->get() );

                    (*physical_device_iterator)->set_device_group_index       (n_group);
                    (*physical_device_iterator)->set_device_group_device_index(n_physical_device);
                }
            }

            new_group.supports_subset_allocations = (this_group_props.subsetAllocation == VK_TRUE);

            m_physical_device_groups.push_back(new_group);
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
    result = Anvil::Vulkan::vkEnumeratePhysicalDevices(m_instance,
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

    result = Anvil::Vulkan::vkEnumeratePhysicalDevices(m_instance,
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
const Anvil::ExtensionEXTDebugReportEntrypoints& Anvil::Instance::get_extension_ext_debug_report_entrypoints() const
{
    anvil_assert(m_enabled_extensions_info_ptr->get_instance_extension_info()->ext_debug_report() );

    return m_ext_debug_report_entrypoints;
}

/** Please see header for specification */
const Anvil::ExtensionEXTDebugUtilsEntrypoints& Anvil::Instance::get_extension_ext_debug_utils_entrypoints() const
{
    anvil_assert(m_enabled_extensions_info_ptr->get_instance_extension_info()->ext_debug_utils() );

    return m_ext_debug_utils_entrypoints;
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

/** Please see header for specification */
const Anvil::ExtensionKHRDeviceGroupCreationEntrypoints& Anvil::Instance::get_extension_khr_device_group_creation_entrypoints() const
{
    anvil_assert(m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_device_group_creation() );

    return m_khr_device_group_creation_entrypoints;
}

/** Initializes the wrapper. */
bool Anvil::Instance::init()
{
    uint32_t                    api_version_to_use                 = 0;
    std::vector<const char*>    enabled_extensions_raw;
    std::vector<const char*>    enabled_layers;
    std::map<std::string, bool> extension_enabled_status;
    bool                        is_device_group_creation_supported = true;
    size_t                      n_instance_layers                  = 0;
    bool                        result                             = false;
    VkResult                    result_vk                          = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    if (!init_vk_func_ptrs() )
    {
        anvil_assert_fail();

        goto end;
    }

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

    /* Determine API version to use */
    if (m_create_info_ptr->get_api_version() == Anvil::APIVersion::UNKNOWN)
    {
        /* Use the latest version available .. */
        uint32_t available_vk_version       = 0;
        uint32_t available_vk_version_major = 0;
        uint32_t available_vk_version_minor = 0;

        if (Anvil::Vulkan::vkEnumerateInstanceVersion != nullptr)
        {
            if (Anvil::Vulkan::vkEnumerateInstanceVersion(&available_vk_version) != VK_SUCCESS)
            {
                anvil_assert_fail();

                goto end;
            }
        }
        else
        {
            /* Assume VK 1.0 is the highest API version available. */
            available_vk_version = VK_MAKE_VERSION(1, 0, 0);
        }

        available_vk_version_major = VK_VERSION_MAJOR(available_vk_version);
        available_vk_version_minor = VK_VERSION_MINOR(available_vk_version);

        if ( available_vk_version_major >= 2                                    ||
            (available_vk_version_major == 1 && available_vk_version_minor >= 1)) 
        {
            api_version_to_use = VK_MAKE_VERSION(1, 1, 0);
            m_api_version      = APIVersion::_1_1;
        }
        else
        {
            api_version_to_use = VK_MAKE_VERSION(1, 0, 0);
            m_api_version      = APIVersion::_1_0;
        }
    }
    else
    {
        switch (m_create_info_ptr->get_api_version() )
        {
            case APIVersion::_1_0:
            {
                api_version_to_use = VK_MAKE_VERSION(1, 0, 0);
                m_api_version      = APIVersion::_1_0;

                break;
            }

            case APIVersion::_1_1:
            {
                api_version_to_use = VK_MAKE_VERSION(1, 1, 0);
                m_api_version      = APIVersion::_1_1;

                break;
            }

            default:
            {
                anvil_assert_fail();

                goto end;
            }
        }
    }

    /* Set up the create info descriptor */
    n_instance_layers = static_cast<uint32_t>(m_supported_layers.size() );

    for (size_t  n_instance_layer = 0;
                 n_instance_layer < n_instance_layers;
               ++n_instance_layer)
    {
        const std::string& layer_description = m_supported_layers[n_instance_layer].description;
        const std::string& layer_name        = m_supported_layers[n_instance_layer].name;

        /* If validation is enabled and this is a layer which issues debug call-backs, cache it, so that
         * we can request for it at vkCreateInstance() call time */
        if (get_create_info_ptr()->get_validation_callback()            != nullptr           &&
            layer_description.find                        ("alidation") != std::string::npos)
        {
            enabled_layers.push_back(layer_name.c_str() );
        }
    }

    {
        if (get_create_info_ptr()->get_validation_callback() != nullptr)
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
        if (is_instance_extension_supported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) )
        {
            extension_enabled_status[VK_EXT_DEBUG_UTILS_EXTENSION_NAME] = true;
        }

        if (is_instance_extension_supported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) )
        {
            extension_enabled_status[VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME] = true;
        }

        if (is_instance_extension_supported(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME) )
        {
            extension_enabled_status[VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME] = true;
        }
        else
        {
            is_device_group_creation_supported = false;
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
        for (const auto& current_extension_name : get_create_info_ptr()->get_disallowed_instance_level_extensions() )
        {
            auto ext_iterator = extension_enabled_status.find(current_extension_name);

            if (ext_iterator != extension_enabled_status.end() )
            {
                if (current_extension_name == VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME)
                {
                    is_device_group_creation_supported = false;
                }

                extension_enabled_status.erase(ext_iterator);
            }
        }

        m_enabled_extensions_info_ptr = Anvil::ExtensionInfo<bool>::create_instance_extension_info(extension_enabled_status,
                                                                                                   false); /* in_unspecified_extension_name_value */
    }

    /* We're ready to create a new Vulkan instance */
    for (auto& ext_name : extension_enabled_status)
    {
        enabled_extensions_raw.push_back(ext_name.first.c_str() );
    }

    {
        VkApplicationInfo    app_info;
        VkInstanceCreateInfo create_info;

        /* Set up the app info descriptor */
        app_info.apiVersion         = api_version_to_use;
        app_info.applicationVersion = m_create_info_ptr->get_app_version   ();
        app_info.engineVersion      = m_create_info_ptr->get_engine_version();
        app_info.pApplicationName   = m_create_info_ptr->get_app_name      ().c_str();
        app_info.pEngineName        = m_create_info_ptr->get_engine_name   ().c_str();
        app_info.pNext              = nullptr;
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;

        create_info.enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions_raw.size() );
        create_info.enabledLayerCount       = static_cast<uint32_t>(enabled_layers.size        () );
        create_info.flags                   = 0;
        create_info.pApplicationInfo        = &app_info;
        create_info.pNext                   = nullptr;
        create_info.ppEnabledExtensionNames = (enabled_extensions_raw.size() > 0) ? &enabled_extensions_raw.at(0) : nullptr;
        create_info.ppEnabledLayerNames     = (enabled_layers.size()         > 0) ? &enabled_layers.at        (0) : nullptr;
        create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        result_vk = Anvil::Vulkan::vkCreateInstance(&create_info,
                                                    nullptr, /* pAllocator */
                                                    &m_instance);

        anvil_assert_vk_call_succeeded(result_vk);
    }

    /* If this is a VK 1.1 instance, explicitly mark VK1.1 specific extensions as enabled. This is to provide backwards compatibility
     * with VK 1.0 applications which may not be aware that VK 1.0 extensions that were folded into VK 1.1 do not necessarily have to
     * be reported as supported.
     */
    if (m_api_version == APIVersion::_1_1)
    {
        extension_enabled_status[VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME]            = true;
        extension_enabled_status[VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME]      = true;
        extension_enabled_status[VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME]     = true;
        extension_enabled_status[VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME]  = true;
        extension_enabled_status[VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME] = true;

        m_enabled_extensions_info_ptr = Anvil::ExtensionInfo<bool>::create_instance_extension_info(extension_enabled_status,
                                                                                                   false); /* in_unspecified_extension_name_value */
    }

    /* Retrieve the remaining part of instance-dependent function pointers */
    if (!init_vk_func_ptrs() )
    {
        /* TODO: Return null instance in case of a failure */
        anvil_assert_fail();
    }

    init_func_pointers();

    if (m_create_info_ptr->get_validation_callback() != nullptr)
    {
        init_debug_callbacks();
    }

    enumerate_physical_devices();

    if (is_device_group_creation_supported)
    {
        enumerate_physical_device_groups();
    }

    result = true;
end:
    return result;
}

/** Initializes debug callback support. */
void Anvil::Instance::init_debug_callbacks()
{
    auto create_info_ptr = Anvil::DebugMessengerCreateInfo::create(this,
                                                                   Anvil::DebugMessageSeverityFlagBits::ERROR_BIT | Anvil::DebugMessageSeverityFlagBits::INFO_BIT | Anvil::DebugMessageSeverityFlagBits::WARNING_BIT,
                                                                   Anvil::DebugMessageTypeFlagBits::VALIDATION_BIT,
                                                                   std::bind(&Anvil::Instance::debug_callback_handler,
                                                                             this,
                                                                             std::placeholders::_1,    /* severity */
                                                                             std::placeholders::_5) ); /* message  */

    anvil_assert(create_info_ptr != nullptr);

    m_debug_messenger_ptr = Anvil::DebugMessenger::create(std::move(create_info_ptr) );
    anvil_assert(m_debug_messenger_ptr != nullptr);
}

/** Initializes all required instance-level function pointers. */
void Anvil::Instance::init_func_pointers()
{
    /* NOTE: Vulkan 1.1 instances do not need to report support for extensions considered core in 1.1. To address this, any func pointers corresponding to VK1.1 functionality
     *       need to be taken from core entrypoint pool.
     *
     *       Anvil reports support for extensions corresponding to wrapped core functionality, irrelevant of what extensions the implementation actually reports for.
     */
    const bool is_vk11_instance = (m_api_version == APIVersion::_1_1);

    #ifdef _WIN32
    {
        #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        {
            if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_win32_surface() )
            {
                m_khr_win32_surface_entrypoints.vkCreateWin32SurfaceKHR                        = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>                       (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                           "vkCreateWin32SurfaceKHR") );
                m_khr_win32_surface_entrypoints.vkGetPhysicalDeviceWin32PresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                           "vkGetPhysicalDeviceWin32PresentationSupportKHR") );

                anvil_assert(m_khr_win32_surface_entrypoints.vkCreateWin32SurfaceKHR                        != nullptr);
                anvil_assert(m_khr_win32_surface_entrypoints.vkGetPhysicalDeviceWin32PresentationSupportKHR != nullptr);
            }
        }
        #endif
    }
    #else
    {
        #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        {
            if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_xcb_surface() )
            {
                m_khr_xcb_surface_entrypoints.vkCreateXcbSurfaceKHR = reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                       "vkCreateXcbSurfaceKHR") );

                anvil_assert(m_khr_xcb_surface_entrypoints.vkCreateXcbSurfaceKHR != nullptr);
            }
        }
        #endif
    }
    #endif

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->ext_debug_report() )
    {
        m_ext_debug_report_entrypoints.vkCreateDebugReportCallbackEXT  = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT> (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                    "vkCreateDebugReportCallbackEXT") );
        m_ext_debug_report_entrypoints.vkDebugReportMessageEXT         = reinterpret_cast<PFN_vkDebugReportMessageEXT>        (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                    "vkDebugReportMessageEXT") );
        m_ext_debug_report_entrypoints.vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                    "vkDestroyDebugReportCallbackEXT") );

        anvil_assert(m_ext_debug_report_entrypoints.vkCreateDebugReportCallbackEXT  != nullptr);
        anvil_assert(m_ext_debug_report_entrypoints.vkDebugReportMessageEXT         != nullptr);
        anvil_assert(m_ext_debug_report_entrypoints.vkDestroyDebugReportCallbackEXT != nullptr);
    }

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->ext_debug_utils() )
    {
        m_ext_debug_utils_entrypoints.vkCmdBeginDebugUtilsLabelEXT    = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>   (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkCmdBeginDebugUtilsLabelEXT") );
        m_ext_debug_utils_entrypoints.vkCmdEndDebugUtilsLabelEXT      = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>     (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkCmdEndDebugUtilsLabelEXT") );
        m_ext_debug_utils_entrypoints.vkCmdInsertDebugUtilsLabelEXT   = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>  (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkCmdInsertDebugUtilsLabelEXT") );
        m_ext_debug_utils_entrypoints.vkCreateDebugUtilsMessengerEXT  = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT> (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkCreateDebugUtilsMessengerEXT") );
        m_ext_debug_utils_entrypoints.vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkDestroyDebugUtilsMessengerEXT") );
        m_ext_debug_utils_entrypoints.vkSetDebugUtilsObjectNameEXT    = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>   (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkSetDebugUtilsObjectNameEXT") );
        m_ext_debug_utils_entrypoints.vkSetDebugUtilsObjectTagEXT     = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>    (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkSetDebugUtilsObjectTagEXT") );
        m_ext_debug_utils_entrypoints.vkQueueBeginDebugUtilsLabelEXT  = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT> (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkQueueBeginDebugUtilsLabelEXT") );
        m_ext_debug_utils_entrypoints.vkQueueEndDebugUtilsLabelEXT    = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>   (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkQueueEndDebugUtilsLabelEXT") );
        m_ext_debug_utils_entrypoints.vkQueueInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueInsertDebugUtilsLabelEXT>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkQueueInsertDebugUtilsLabelEXT") );
        m_ext_debug_utils_entrypoints.vkSubmitDebugUtilsMessageEXT    = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>   (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                   "vkSubmitDebugUtilsMessageEXT") );

        anvil_assert(m_ext_debug_utils_entrypoints.vkCmdBeginDebugUtilsLabelEXT    != nullptr);
        anvil_assert(m_ext_debug_utils_entrypoints.vkCmdEndDebugUtilsLabelEXT      != nullptr);
        anvil_assert(m_ext_debug_utils_entrypoints.vkCmdInsertDebugUtilsLabelEXT   != nullptr);
        anvil_assert(m_ext_debug_utils_entrypoints.vkCreateDebugUtilsMessengerEXT  != nullptr);
        anvil_assert(m_ext_debug_utils_entrypoints.vkDestroyDebugUtilsMessengerEXT != nullptr);
        anvil_assert(m_ext_debug_utils_entrypoints.vkSetDebugUtilsObjectNameEXT    != nullptr);
        anvil_assert(m_ext_debug_utils_entrypoints.vkSetDebugUtilsObjectTagEXT     != nullptr);
        anvil_assert(m_ext_debug_utils_entrypoints.vkQueueBeginDebugUtilsLabelEXT  != nullptr);
        anvil_assert(m_ext_debug_utils_entrypoints.vkQueueEndDebugUtilsLabelEXT    != nullptr);
        anvil_assert(m_ext_debug_utils_entrypoints.vkQueueInsertDebugUtilsLabelEXT != nullptr);
        anvil_assert(m_ext_debug_utils_entrypoints.vkSubmitDebugUtilsMessageEXT    != nullptr);
    }

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_get_physical_device_properties2() ||
        is_vk11_instance)
    {
        const char* vk_get_physical_device_features_2_entrypoint_name                       = (is_vk11_instance) ? "vkGetPhysicalDeviceFeatures2"
                                                                                                                 : "vkGetPhysicalDeviceFeatures2KHR";
        const char* vk_get_physical_device_format_properties_2_entrypoint_name              = (is_vk11_instance) ? "vkGetPhysicalDeviceFormatProperties2"
                                                                                                                 : "vkGetPhysicalDeviceFormatProperties2KHR";
        const char* vk_get_physical_device_image_format_properties_2_entrypoint_name        = (is_vk11_instance) ? "vkGetPhysicalDeviceImageFormatProperties2"
                                                                                                                 : "vkGetPhysicalDeviceImageFormatProperties2KHR";
        const char* vk_get_physical_device_memory_properties_2_entrypoint_name              = (is_vk11_instance) ? "vkGetPhysicalDeviceMemoryProperties2"
                                                                                                                 : "vkGetPhysicalDeviceMemoryProperties2KHR";
        const char* vk_get_physical_device_properties_2_entrypoint_name                     = (is_vk11_instance) ? "vkGetPhysicalDeviceProperties2"
                                                                                                                 : "vkGetPhysicalDeviceProperties2KHR";
        const char* vk_get_physical_device_queue_family_properties_2_entrypoint_name        = (is_vk11_instance) ? "vkGetPhysicalDeviceQueueFamilyProperties2"
                                                                                                                 : "vkGetPhysicalDeviceQueueFamilyProperties2KHR";
        const char* vk_get_physical_device_sparse_image_format_properties_2_entrypoint_name = (is_vk11_instance) ? "vkGetPhysicalDeviceSparseImageFormatProperties2"
                                                                                                                 : "vkGetPhysicalDeviceSparseImageFormatProperties2KHR";

        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceFeatures2KHR                    = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>                   (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                                             vk_get_physical_device_features_2_entrypoint_name) );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceFormatProperties2KHR            = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties2KHR>           (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                                             vk_get_physical_device_format_properties_2_entrypoint_name) );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceImageFormatProperties2KHR       = reinterpret_cast<PFN_vkGetPhysicalDeviceImageFormatProperties2KHR>      (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                                             vk_get_physical_device_image_format_properties_2_entrypoint_name) );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceMemoryProperties2KHR            = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2KHR>           (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                                             vk_get_physical_device_memory_properties_2_entrypoint_name) );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceProperties2KHR                  = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>                 (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                                             vk_get_physical_device_properties_2_entrypoint_name) );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceQueueFamilyProperties2KHR       = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR>      (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                                             vk_get_physical_device_queue_family_properties_2_entrypoint_name) );
        m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceSparseImageFormatProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                                             vk_get_physical_device_sparse_image_format_properties_2_entrypoint_name) );

        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceFeatures2KHR                    != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceFormatProperties2KHR            != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceImageFormatProperties2KHR       != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceMemoryProperties2KHR            != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceProperties2KHR                  != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceQueueFamilyProperties2KHR       != nullptr);
        anvil_assert(m_khr_get_physical_device_properties2_entrypoints.vkGetPhysicalDeviceSparseImageFormatProperties2KHR != nullptr);
    }

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_device_group_creation() ||
        is_vk11_instance)
    {
        const char* vk_enumerate_physical_device_groups_entrypoint_name = (is_vk11_instance) ? "vkEnumeratePhysicalDeviceGroups"
                                                                                             : "vkEnumeratePhysicalDeviceGroupsKHR";

        m_khr_device_group_creation_entrypoints.vkEnumeratePhysicalDeviceGroupsKHR = reinterpret_cast<PFN_vkEnumeratePhysicalDeviceGroupsKHR>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                   vk_enumerate_physical_device_groups_entrypoint_name) );

        anvil_assert(m_khr_device_group_creation_entrypoints.vkEnumeratePhysicalDeviceGroupsKHR != nullptr);
    }

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_external_fence_capabilities() ||
        is_vk11_instance)
    {
        const char* vk_get_physical_device_external_fence_properties_entrypoint_name = (is_vk11_instance) ? "vkGetPhysicalDeviceExternalFenceProperties"
                                                                                                          : "vkGetPhysicalDeviceExternalFencePropertiesKHR";

        m_khr_external_fence_capabilities_entrypoints.vkGetPhysicalDeviceExternalFencePropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                               vk_get_physical_device_external_fence_properties_entrypoint_name) );

        anvil_assert(m_khr_external_fence_capabilities_entrypoints.vkGetPhysicalDeviceExternalFencePropertiesKHR != nullptr);
    }

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_external_memory_capabilities() ||
        is_vk11_instance)
    {
        const char* vk_get_physical_device_external_buffer_properties_entrypoint_name = (is_vk11_instance) ? "vkGetPhysicalDeviceExternalBufferProperties"
                                                                                                           : "vkGetPhysicalDeviceExternalBufferPropertiesKHR";

        m_khr_external_memory_capabilities_entrypoints.vkGetPhysicalDeviceExternalBufferPropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                                  vk_get_physical_device_external_buffer_properties_entrypoint_name) );

        anvil_assert(m_khr_external_memory_capabilities_entrypoints.vkGetPhysicalDeviceExternalBufferPropertiesKHR != nullptr);
    }

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_external_semaphore_capabilities() ||
        is_vk11_instance)
    {
        const char* vk_get_physical_device_external_semaphore_properties_entrypoint_name = (is_vk11_instance) ? "vkGetPhysicalDeviceExternalSemaphoreProperties"
                                                                                                              : "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR";

        m_khr_external_semaphore_capabilities_entrypoints.vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                                                           vk_get_physical_device_external_semaphore_properties_entrypoint_name) );

        anvil_assert(m_khr_external_semaphore_capabilities_entrypoints.vkGetPhysicalDeviceExternalSemaphorePropertiesKHR!= nullptr);
    }

    if (m_enabled_extensions_info_ptr->get_instance_extension_info()->khr_surface() )
    {
        m_khr_surface_entrypoints.vkDestroySurfaceKHR                       = reinterpret_cast<PFN_vkDestroySurfaceKHR>                      (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                   "vkDestroySurfaceKHR") );
        m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                   "vkGetPhysicalDeviceSurfaceCapabilitiesKHR") );
        m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceFormatsKHR      = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>     (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                   "vkGetPhysicalDeviceSurfaceFormatsKHR") );
        m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                   "vkGetPhysicalDeviceSurfacePresentModesKHR") );
        m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceSupportKHR      = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>     (Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                                                                                   "vkGetPhysicalDeviceSurfaceSupportKHR") );

        anvil_assert(m_khr_surface_entrypoints.vkDestroySurfaceKHR                       != nullptr);
        anvil_assert(m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceCapabilitiesKHR != nullptr);
        anvil_assert(m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceFormatsKHR      != nullptr);
        anvil_assert(m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfacePresentModesKHR != nullptr);
        anvil_assert(m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceSupportKHR      != nullptr);
    }
}

bool Anvil::Instance::init_vk_func_ptrs()
{
    std::lock_guard<std::mutex> lock  (g_vk_func_ptr_init_mutex);
    bool                        result(true);

    typedef struct
    {
        std::string func_name;
        bool        requires_getter_call;
        void**      result_func_ptr;
    } FunctionData;

    #if !defined(ANVIL_LINK_STATICALLY_WITH_VULKAN_LIB)
        FunctionData functions_vk10[] =
        {
            /* NOTE: All functions with @param requires_getter_call equal to false should come first! */
            {"vkCreateInstance",                               false, reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateInstance)},
            {"vkDestroyInstance",                              false, reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyInstance)},
            {"vkEnumeratePhysicalDevices",                     false, reinterpret_cast<void**>(&Anvil::Vulkan::vkEnumeratePhysicalDevices)},
            {"vkEnumerateInstanceExtensionProperties",         false, reinterpret_cast<void**>(&Anvil::Vulkan::vkEnumerateInstanceExtensionProperties)},
            {"vkEnumerateInstanceLayerProperties",             false, reinterpret_cast<void**>(&Anvil::Vulkan::vkEnumerateInstanceLayerProperties)},
            {"vkGetDeviceProcAddr",                            false, reinterpret_cast<void**>(&Anvil::Vulkan::vkGetDeviceProcAddr)},
            {"vkGetInstanceProcAddr",                          false, reinterpret_cast<void**>(&Anvil::Vulkan::vkGetInstanceProcAddr)},

            /* instance-dependent functions */
            {"vkGetPhysicalDeviceFeatures",                    true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceFeatures)},
            {"vkGetPhysicalDeviceFormatProperties",            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceFormatProperties)},
            {"vkGetPhysicalDeviceImageFormatProperties",       true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceImageFormatProperties)},
            {"vkGetPhysicalDeviceProperties",                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceProperties)},
            {"vkGetPhysicalDeviceQueueFamilyProperties",       true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceQueueFamilyProperties)},
            {"vkGetPhysicalDeviceMemoryProperties",            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceMemoryProperties)},
            {"vkCreateDevice",                                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateDevice)},
            {"vkDestroyDevice",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyDevice)},
            {"vkEnumerateDeviceExtensionProperties",           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkEnumerateDeviceExtensionProperties)},
            {"vkEnumerateDeviceLayerProperties",               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkEnumerateDeviceLayerProperties)},
            {"vkGetDeviceQueue",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetDeviceQueue)},
            {"vkQueueSubmit",                                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkQueueSubmit)},
            {"vkQueueWaitIdle",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkQueueWaitIdle)},
            {"vkDeviceWaitIdle",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDeviceWaitIdle)},
            {"vkAllocateMemory",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkAllocateMemory)},
            {"vkFreeMemory",                                   true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkFreeMemory)},
            {"vkMapMemory",                                    true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkMapMemory)},
            {"vkUnmapMemory",                                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkUnmapMemory)},
            {"vkFlushMappedMemoryRanges",                      true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkFlushMappedMemoryRanges)},
            {"vkInvalidateMappedMemoryRanges",                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkInvalidateMappedMemoryRanges)},
            {"vkGetDeviceMemoryCommitment",                    true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetDeviceMemoryCommitment)},
            {"vkBindBufferMemory",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkBindBufferMemory)},
            {"vkBindImageMemory",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkBindImageMemory)},
            {"vkGetBufferMemoryRequirements",                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetBufferMemoryRequirements)},
            {"vkGetImageMemoryRequirements",                   true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetImageMemoryRequirements)},
            {"vkGetImageSparseMemoryRequirements",             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetImageSparseMemoryRequirements)},
            {"vkGetPhysicalDeviceSparseImageFormatProperties", true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceSparseImageFormatProperties)},
            {"vkQueueBindSparse",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkQueueBindSparse)},
            {"vkCreateFence",                                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateFence)},
            {"vkDestroyFence",                                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyFence)},
            {"vkResetFences",                                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkResetFences)},
            {"vkGetFenceStatus",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetFenceStatus)},
            {"vkWaitForFences",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkWaitForFences)},
            {"vkCreateSemaphore",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateSemaphore)},
            {"vkDestroySemaphore",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroySemaphore)},
            {"vkCreateEvent",                                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateEvent)},
            {"vkDestroyEvent",                                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyEvent)},
            {"vkGetEventStatus",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetEventStatus)},
            {"vkSetEvent",                                     true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkSetEvent)},
            {"vkResetEvent",                                   true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkResetEvent)},
            {"vkCreateQueryPool",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateQueryPool)},
            {"vkDestroyQueryPool",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyQueryPool)},
            {"vkGetQueryPoolResults",                          true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetQueryPoolResults)},
            {"vkCreateBuffer",                                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateBuffer)},
            {"vkDestroyBuffer",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyBuffer)},
            {"vkCreateBufferView",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateBufferView)},
            {"vkDestroyBufferView",                            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyBufferView)},
            {"vkCreateImage",                                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateImage)},
            {"vkDestroyImage",                                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyImage)},
            {"vkGetImageSubresourceLayout",                    true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetImageSubresourceLayout)},
            {"vkCreateImageView",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateImageView)},
            {"vkDestroyImageView",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyImageView)},
            {"vkCreateShaderModule",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateShaderModule)},
            {"vkDestroyShaderModule",                          true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyShaderModule)},
            {"vkCreatePipelineCache",                          true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreatePipelineCache)},
            {"vkDestroyPipelineCache",                         true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyPipelineCache)},
            {"vkGetPipelineCacheData",                         true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPipelineCacheData)},
            {"vkMergePipelineCaches",                          true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkMergePipelineCaches)},
            {"vkCreateGraphicsPipelines",                      true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateGraphicsPipelines)},
            {"vkCreateComputePipelines",                       true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateComputePipelines)},
            {"vkDestroyPipeline",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyPipeline)},
            {"vkCreatePipelineLayout",                         true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreatePipelineLayout)},
            {"vkDestroyPipelineLayout",                        true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyPipelineLayout)},
            {"vkCreateSampler",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateSampler)},
            {"vkDestroySampler",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroySampler)},
            {"vkCreateDescriptorSetLayout",                    true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateDescriptorSetLayout)},
            {"vkDestroyDescriptorSetLayout",                   true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyDescriptorSetLayout)},
            {"vkCreateDescriptorPool",                         true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateDescriptorPool)},
            {"vkDestroyDescriptorPool",                        true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyDescriptorPool)},
            {"vkResetDescriptorPool",                          true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkResetDescriptorPool)},
            {"vkAllocateDescriptorSets",                       true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkAllocateDescriptorSets)},
            {"vkFreeDescriptorSets",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkFreeDescriptorSets)},
            {"vkUpdateDescriptorSets",                         true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkUpdateDescriptorSets)},
            {"vkCreateFramebuffer",                            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateFramebuffer)},
            {"vkDestroyFramebuffer",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyFramebuffer)},
            {"vkCreateRenderPass",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateRenderPass)},
            {"vkDestroyRenderPass",                            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyRenderPass)},
            {"vkGetRenderAreaGranularity",                     true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetRenderAreaGranularity)},
            {"vkCreateCommandPool",                            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateCommandPool)},
            {"vkDestroyCommandPool",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyCommandPool)},
            {"vkResetCommandPool",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkResetCommandPool)},
            {"vkAllocateCommandBuffers",                       true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkAllocateCommandBuffers)},
            {"vkFreeCommandBuffers",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkFreeCommandBuffers)},
            {"vkBeginCommandBuffer",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkBeginCommandBuffer)},
            {"vkEndCommandBuffer",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkEndCommandBuffer)},
            {"vkResetCommandBuffer",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkResetCommandBuffer)},
            {"vkCmdBindPipeline",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdBindPipeline)},
            {"vkCmdSetViewport",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetViewport)},
            {"vkCmdSetScissor",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetScissor)},
            {"vkCmdSetLineWidth",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetLineWidth)},
            {"vkCmdSetDepthBias",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetDepthBias)},
            {"vkCmdSetBlendConstants",                         true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetBlendConstants)},
            {"vkCmdSetDepthBounds",                            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetDepthBounds)},
            {"vkCmdSetStencilCompareMask",                     true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetStencilCompareMask)},
            {"vkCmdSetStencilWriteMask",                       true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetStencilWriteMask)},
            {"vkCmdSetStencilReference",                       true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetStencilReference)},
            {"vkCmdBindDescriptorSets",                        true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdBindDescriptorSets)},
            {"vkCmdBindIndexBuffer",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdBindIndexBuffer)},
            {"vkCmdBindVertexBuffers",                         true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdBindVertexBuffers)},
            {"vkCmdDraw",                                      true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdDraw)},
            {"vkCmdDrawIndexed",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdDrawIndexed)},
            {"vkCmdDrawIndirect",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdDrawIndirect)},
            {"vkCmdDrawIndexedIndirect",                       true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdDrawIndexedIndirect)},
            {"vkCmdDispatch",                                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdDispatch)},
            {"vkCmdDispatchIndirect",                          true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdDispatchIndirect)},
            {"vkCmdCopyBuffer",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdCopyBuffer)},
            {"vkCmdCopyImage",                                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdCopyImage)},
            {"vkCmdBlitImage",                                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdBlitImage)},
            {"vkCmdCopyBufferToImage",                         true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdCopyBufferToImage)},
            {"vkCmdCopyImageToBuffer",                         true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdCopyImageToBuffer)},
            {"vkCmdUpdateBuffer",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdUpdateBuffer)},
            {"vkCmdFillBuffer",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdFillBuffer)},
            {"vkCmdClearColorImage",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdClearColorImage)},
            {"vkCmdClearDepthStencilImage",                    true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdClearDepthStencilImage)},
            {"vkCmdClearAttachments",                          true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdClearAttachments)},
            {"vkCmdResolveImage",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdResolveImage)},
            {"vkCmdSetEvent",                                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetEvent)},
            {"vkCmdResetEvent",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdResetEvent)},
            {"vkCmdWaitEvents",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdWaitEvents)},
            {"vkCmdPipelineBarrier",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdPipelineBarrier)},
            {"vkCmdBeginQuery",                                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdBeginQuery)},
            {"vkCmdEndQuery",                                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdEndQuery)},
            {"vkCmdResetQueryPool",                            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdResetQueryPool)},
            {"vkCmdWriteTimestamp",                            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdWriteTimestamp)},
            {"vkCmdCopyQueryPoolResults",                      true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdCopyQueryPoolResults)},
            {"vkCmdPushConstants",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdPushConstants)},
            {"vkCmdBeginRenderPass",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdBeginRenderPass)},
            {"vkCmdNextSubpass",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdNextSubpass)},
            {"vkCmdEndRenderPass",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdEndRenderPass)},
            {"vkCmdExecuteCommands",                           true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdExecuteCommands)}
        };
    #endif

    FunctionData functions_vk11[] =
    {
        {"vkBindBufferMemory2",                             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkBindBufferMemory2)},
        {"vkCmdDispatchBase",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdDispatchBase)},
        {"vkCmdSetDeviceMask",                              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCmdSetDeviceMask)},
        {"vkCreateDescriptorUpdateTemplate",                true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateDescriptorUpdateTemplate)},
        {"vkCreateSamplerYcbcrConversion",                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkCreateSamplerYcbcrConversion)},
        {"vkDestroyDescriptorUpdateTemplate",               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroyDescriptorUpdateTemplate)},
        {"vkDestroySamplerYcbcrConversion",                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkDestroySamplerYcbcrConversion)},
        {"vkEnumerateInstanceVersion",                      false, reinterpret_cast<void**>(&Anvil::Vulkan::vkEnumerateInstanceVersion)},
        {"vkEnumeratePhysicalDeviceGroups",                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkEnumeratePhysicalDeviceGroups)},
        {"vkGetBufferMemoryRequirements2",                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetBufferMemoryRequirements2)},
        {"vkGetDescriptorSetLayoutSupport",                 true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetDescriptorSetLayoutSupport)},
        {"vkGetDeviceGroupPeerMemoryFeatures",              true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetDeviceGroupPeerMemoryFeatures)},
        {"vkGetDeviceQueue2",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetDeviceQueue2)},
        {"vkGetImageMemoryRequirements2",                   true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetImageMemoryRequirements2)},
        {"vkGetImageSparseMemoryRequirements2",             true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetImageSparseMemoryRequirements2)},
        {"vkGetPhysicalDeviceExternalBufferProperties",     true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceExternalBufferProperties)},
        {"vkGetPhysicalDeviceExternalFenceProperties",      true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceExternalFenceProperties)},
        {"vkGetPhysicalDeviceExternalSemaphoreProperties",  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceExternalSemaphoreProperties)},
        {"vkGetPhysicalDeviceFeatures2",                    true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceFeatures2)},
        {"vkGetPhysicalDeviceFormatProperties2",            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceFormatProperties2)},
        {"vkGetPhysicalDeviceImageFormatProperties2",       true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceImageFormatProperties2)},
        {"vkGetPhysicalDeviceMemoryProperties2",            true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceMemoryProperties2)},
        {"vkGetPhysicalDeviceProperties2",                  true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceProperties2)},
        {"vkGetPhysicalDeviceQueueFamilyProperties2",       true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceQueueFamilyProperties2)},
        {"vkGetPhysicalDeviceSparseImageFormatProperties2", true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkGetPhysicalDeviceSparseImageFormatProperties2)},
        {"vkTrimCommandPool",                               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkTrimCommandPool)},
        {"vkUpdateDescriptorSetWithTemplate",               true,  reinterpret_cast<void**>(&Anvil::Vulkan::vkUpdateDescriptorSetWithTemplate)},
    };

    if (g_core_func_ptrs_inited     &&
        g_instance_func_ptrs_inited)
    {
        result = true;

        goto end;
    }

    #if !defined(ANVIL_LINK_STATICALLY_WITH_VULKAN_LIB)
    {
        if (g_vk_library_ptr == nullptr)
        {
            g_vk_library_ptr = Anvil::Library::create(ANVIL_VULKAN_DYNAMIC_DLL);

            if (g_vk_library_ptr == nullptr)
            {
                anvil_assert(g_vk_library_ptr != nullptr);

                goto end;
            }
        }

        /* VK 1.0 func ptrgetters - all entrypoints must be present */
        for (const auto& current_func_data : functions_vk10)
        {
            if (!current_func_data.requires_getter_call &&
                !g_core_func_ptrs_inited)
            {
                *current_func_data.result_func_ptr = g_vk_library_ptr->get_proc_address(current_func_data.func_name.c_str() );

                if (*current_func_data.result_func_ptr == nullptr)
                {
                    anvil_assert(*current_func_data.result_func_ptr != nullptr);

                    goto end;
                }
            }

            if ((current_func_data.requires_getter_call)                   &&
                (g_core_func_ptrs_inited)                                  &&
                (m_instance                             != VK_NULL_HANDLE) )
            {
                *current_func_data.result_func_ptr = reinterpret_cast<void*>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                                  current_func_data.func_name.c_str() ));

                if (*current_func_data.result_func_ptr == nullptr)
                {
                    anvil_assert(*current_func_data.result_func_ptr != nullptr);

                    goto end;
                }
            }
        }
    }
    #endif

    /* VK 1.1 func ptr getters - all entrypoints may be missing on implementations that do not support the API */
    for (const auto& current_func_data : functions_vk11)
    {
        if (!current_func_data.requires_getter_call)
        {
            *current_func_data.result_func_ptr = reinterpret_cast<void*>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                              current_func_data.func_name.c_str() ));
        }

         if ((current_func_data.requires_getter_call)                   &&
             (m_instance                             != VK_NULL_HANDLE) )
        {
            *current_func_data.result_func_ptr = reinterpret_cast<void*>(Anvil::Vulkan::vkGetInstanceProcAddr(m_instance,
                                                                                                              current_func_data.func_name.c_str() ));
        }
    }

    /* Done */
    g_core_func_ptrs_inited = true;

    if (m_instance != VK_NULL_HANDLE)
    {
        g_instance_func_ptrs_inited = true;
    }

end:
    return result;
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
