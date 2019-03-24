//
// Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/rendering_surface_create_info.h"
#include "misc/window.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/queue.h"
#include "config.h"

/* Please see header for specification */
Anvil::RenderingSurface::RenderingSurface(Anvil::RenderingSurfaceCreateInfoUniquePtr in_create_info_ptr)
    :DebugMarkerSupportProvider(in_create_info_ptr->get_device_ptr(),
                                Anvil::ObjectType::RENDERING_SURFACE),
     MTSafetySupportProvider   (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety (),
                                                                                in_create_info_ptr->get_device_ptr() )),
     m_height                  (0),
     m_surface                 (VK_NULL_HANDLE),
     m_width                   (0)
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    /* Register the instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::RENDERING_SURFACE,
                                                 this);
}

/* Please see header for specification */
Anvil::RenderingSurface::~RenderingSurface()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::RENDERING_SURFACE,
                                                    this);

    if (m_surface != VK_NULL_HANDLE)
    {
        lock();
        {
            auto instance_ptr = m_create_info_ptr->get_instance_ptr();

            instance_ptr->get_extension_khr_surface_entrypoints().vkDestroySurfaceKHR(instance_ptr->get_instance_vk(),
                                                                                      m_surface,
                                                                                      nullptr /* pAllocator */);
        }
        unlock();

        m_surface = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
void Anvil::RenderingSurface::cache_surface_properties()
{
    const Anvil::DeviceType&             device_type                   (m_device_ptr->get_type() );
    bool                                 is_offscreen_rendering_enabled(true);
    auto                                 khr_surface_entrypoints       (m_create_info_ptr->get_instance_ptr()->get_extension_khr_surface_entrypoints() );
    const Anvil::MGPUDevice*             mgpu_device_ptr               (dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr));
    uint32_t                             n_physical_devices            (0);
    const Anvil::SGPUDevice*             sgpu_device_ptr               (dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr));
    std::vector<Anvil::SurfaceFormatKHR> supported_formats;
    auto                                 window_ptr                    (m_create_info_ptr->get_window_ptr() );

    if (window_ptr != nullptr)
    {
        const WindowPlatform window_platform(window_ptr->get_platform() );

        is_offscreen_rendering_enabled = (window_platform == WINDOW_PLATFORM_DUMMY                     ||
                                          window_platform == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS);

        if (is_offscreen_rendering_enabled)
        {
            m_height = window_ptr->get_height_at_creation_time();
            m_width  = window_ptr->get_width_at_creation_time ();
        }
        else
        {
            /* In this case, width & height may change at run-time */
        }
    }
    else
    {
        /* In this case, width & height may change at run-time */
    }

    switch (device_type)
    {
        case Anvil::DeviceType::MULTI_GPU:  n_physical_devices = mgpu_device_ptr->get_n_physical_devices(); break;
        case Anvil::DeviceType::SINGLE_GPU: n_physical_devices = 1;                                         break;

        default:
        {
            anvil_assert_fail();
        }
    }

    /* Retrieve general properties */
    uint32_t n_supported_formats           (0);
    uint32_t n_supported_presentation_modes(0);
    VkResult result                        (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result);

    for (uint32_t n_physical_device = 0;
                  n_physical_device < n_physical_devices;
                ++n_physical_device)
    {
        const Anvil::PhysicalDevice* physical_device_ptr = nullptr;

        switch (device_type)
        {
            case Anvil::DeviceType::MULTI_GPU:  physical_device_ptr = mgpu_device_ptr->get_physical_device(n_physical_device); break;
            case Anvil::DeviceType::SINGLE_GPU: physical_device_ptr = sgpu_device_ptr->get_physical_device();                  break;

            default:
            {
                anvil_assert_fail();
            }
        }

        auto& result_caps = m_physical_device_capabilities[physical_device_ptr->get_device_group_device_index()];

        if (m_surface == VK_NULL_HANDLE)
        {
            result_caps.supported_composite_alpha_flags = Anvil::CompositeAlphaFlagBits::INHERIT_BIT_KHR;
            result_caps.supported_transformations       = Anvil::SurfaceTransformFlagBits::INHERIT_BIT_KHR;
            result_caps.supported_usages                = static_cast<Anvil::ImageUsageFlags> (Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT |
                                                                                               Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT     |
                                                                                               Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT     |
                                                                                               Anvil::ImageUsageFlagBits::STORAGE_BIT);

            result_caps.supported_presentation_modes.push_back(Anvil::PresentModeKHR::IMMEDIATE_KHR);

            continue;
        }

        const VkPhysicalDevice physical_device_vk = physical_device_ptr->get_physical_device();

        result = khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_vk,
                                                                                   m_surface,
                                                                                   reinterpret_cast<VkSurfaceCapabilitiesKHR*>(&result_caps.capabilities) );

        anvil_assert_vk_call_succeeded(result);

        if (n_physical_device == 0)
        {
            m_height = result_caps.capabilities.current_extent.height;
            m_width  = result_caps.capabilities.current_extent.width;
        }
        else
        {
            anvil_assert(m_height == result_caps.capabilities.current_extent.height);
            anvil_assert(m_width  == result_caps.capabilities.current_extent.width);
        }

        result_caps.supported_composite_alpha_flags = result_caps.capabilities.supported_composite_alpha;
        result_caps.supported_transformations       = result_caps.capabilities.supported_transforms;
        result_caps.supported_usages                = result_caps.capabilities.supported_usage_flags;

        /* Retrieve a list of formats supported by the surface */
        result = khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_vk,
                                                                              m_surface,
                                                                             &n_supported_formats,
                                                                              nullptr /* pSurfaceFormats */);

        anvil_assert                  (n_supported_formats >  0);
        anvil_assert_vk_call_succeeded(result);

        supported_formats.resize(n_supported_formats);

        result = khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_vk,
                                                                              m_surface,
                                                                             &n_supported_formats,
                                                                              reinterpret_cast<VkSurfaceFormatKHR*>(&supported_formats.at(0) ));
        anvil_assert_vk_call_succeeded(result);

        for (unsigned int n_format = 0;
                          n_format < n_supported_formats;
                        ++n_format)
        {
            result_caps.supported_formats.push_back(RenderingSurfaceFormat(supported_formats[n_format]) );
        }

        /* Retrieve a list of supported presentation modes
         *
         * NOTE: In case of mGPU devices, n_supported_presentation_modes may actually be 0 here for slave devices.
         */
        result = khr_surface_entrypoints.vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_vk,
                                                                                   m_surface,
                                                                                  &n_supported_presentation_modes,
                                                                                   nullptr /* pPresentModes */);

        anvil_assert_vk_call_succeeded(result);

        if (n_supported_presentation_modes > 0)
        {
            std::vector<VkPresentModeKHR> temp_storage(n_supported_presentation_modes);

            result_caps.supported_presentation_modes.resize(n_supported_presentation_modes);

            result = khr_surface_entrypoints.vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_vk,
                                                                                       m_surface,
                                                                                      &n_supported_presentation_modes,
                                                                                      &temp_storage.at(0) );
            anvil_assert_vk_call_succeeded(result);

            for (uint32_t n_presentation_mode = 0;
                          n_presentation_mode < static_cast<uint32_t>(temp_storage.size() );
                        ++n_presentation_mode)
            {
                result_caps.supported_presentation_modes.at(n_presentation_mode) = static_cast<Anvil::PresentModeKHR>(temp_storage.at(n_presentation_mode) );
            }
        }
    }
}

/* Please see header for specification */
Anvil::RenderingSurfaceUniquePtr Anvil::RenderingSurface::create(Anvil::RenderingSurfaceCreateInfoUniquePtr in_create_info_ptr)
{
    RenderingSurfaceUniquePtr result_ptr(nullptr,
                                         std::default_delete<RenderingSurface>() );

    result_ptr.reset(
        new Anvil::RenderingSurface(
            std::move(in_create_info_ptr)
        )
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_capabilities(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                               Anvil::SurfaceCapabilities*  out_surface_caps_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(in_physical_device_ptr->get_device_group_device_index());
    bool result        = false;

    if (caps_iterator != m_physical_device_capabilities.end() )
    {
        *out_surface_caps_ptr = caps_iterator->second.capabilities;
        result                = true;
    }

    /* All done */
    return result;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_queue_families_with_present_support(const Anvil::PhysicalDevice*  in_physical_device_ptr,
                                                                      const std::vector<uint32_t>** out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(in_physical_device_ptr->get_device_group_device_index());
    bool result        = false;

    if (caps_iterator != m_physical_device_capabilities.end() )
    {
        *out_result_ptr = &caps_iterator->second.present_capable_queue_fams;
        result          = true;
    }

    /* All done */
    return result;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_supported_composite_alpha_flags(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                                                  Anvil::CompositeAlphaFlags*  out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(in_physical_device_ptr->get_device_group_device_index());
    bool result        = false;

    if (caps_iterator != m_physical_device_capabilities.end() )
    {
        *out_result_ptr = caps_iterator->second.supported_composite_alpha_flags;
        result          = true;
    }

    /* All done */
    return result;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_supported_transformations(const Anvil::PhysicalDevice*  in_physical_device_ptr,
                                                            Anvil::SurfaceTransformFlags* out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(in_physical_device_ptr->get_device_group_device_index());
    bool result        = false;

    if (caps_iterator != m_physical_device_capabilities.end() )
    {
        *out_result_ptr = caps_iterator->second.supported_transformations;
        result          = true;
    }

    /* All done */
    return result;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_supported_usages(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                                   Anvil::ImageUsageFlags*      out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(in_physical_device_ptr->get_device_group_device_index());
    bool result        = false;

    if (caps_iterator != m_physical_device_capabilities.end() )
    {
        *out_result_ptr = caps_iterator->second.supported_usages;
        result          = true;
    }

    /* All done */
    return result;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::init()
{
    const Anvil::DeviceType& device_type       (m_device_ptr->get_type() );
    bool                     init_successful   (false);
    auto                     instance_ptr      (m_create_info_ptr->get_instance_ptr() );
    uint32_t                 n_physical_devices(0);
    VkResult                 result            (VK_SUCCESS);
    const WindowPlatform     window_platform   (m_create_info_ptr->get_window_ptr()->get_platform());

    const bool               is_dummy_window_platform(window_platform == WINDOW_PLATFORM_DUMMY                     ||
                                                      window_platform == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS);


    switch (device_type)
    {
        case Anvil::DeviceType::MULTI_GPU:
        {
            const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );

            n_physical_devices = mgpu_device_ptr->get_n_physical_devices();

            break;
        }

        case Anvil::DeviceType::SINGLE_GPU:
        {
            n_physical_devices = 1;

            break;
        }

        default:
        {
            anvil_assert_fail();

            goto end;
        }
    }


    if (!is_dummy_window_platform)
    {
        auto window_ptr = m_create_info_ptr->get_window_ptr();

        #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT) && defined(_WIN32)
        {
            VkWin32SurfaceCreateInfoKHR surface_create_info;

            surface_create_info.flags     = 0;
            surface_create_info.hinstance = GetModuleHandle(nullptr);
            surface_create_info.hwnd      = window_ptr->get_handle();
            surface_create_info.pNext     = nullptr;
            surface_create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;

            result = instance_ptr->get_extension_khr_win32_surface_entrypoints().vkCreateWin32SurfaceKHR(instance_ptr->get_instance_vk(),
                                                                                                        &surface_create_info,
                                                                                                         nullptr, /* pAllocator */
                                                                                                        &m_surface);
        }
        #endif
        #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT) && !defined(_WIN32)
        {
            VkXcbSurfaceCreateInfoKHR surface_create_info;

            surface_create_info.flags       = 0;
            surface_create_info.window      = window_ptr->get_handle();
            surface_create_info.connection  = static_cast<xcb_connection_t*>(window_ptr->get_connection());
            surface_create_info.pNext       = nullptr;
            surface_create_info.sType       = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;

            result = instance_ptr->get_extension_khr_xcb_surface_entrypoints().vkCreateXcbSurfaceKHR(instance_ptr->get_instance_vk(),
                                                                                                    &surface_create_info,
                                                                                                     nullptr, /* pAllocator */
                                                                                                    &m_surface);
            }
        #endif

        anvil_assert_vk_call_succeeded(result);
        if (is_vk_call_successful(result) )
        {
            set_vk_handle(m_surface);
        }
    }
    else
    {
        anvil_assert(window_platform != WINDOW_PLATFORM_UNKNOWN);
    }

    if (is_dummy_window_platform == false)
    {
        /* Is there at least one queue fam that can be used together with at least one physical device associated with
         * the logical device to present using the surface we've just spawned and the physical device user has specified? */
        const auto& queue_families(m_device_ptr->get_physical_device_queue_families() );

        for (uint32_t n_physical_device = 0;
                      n_physical_device < n_physical_devices;
                    ++n_physical_device)
        {
            Anvil::RenderingSurface::PhysicalDeviceCapabilities* physical_device_caps_ptr = nullptr;
            const Anvil::PhysicalDevice*                         physical_device_ptr      = nullptr;

            switch (device_type)
            {
                case Anvil::DeviceType::MULTI_GPU:
                {
                    const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );

                    physical_device_ptr      = mgpu_device_ptr->get_physical_device(n_physical_device);
                    physical_device_caps_ptr = &m_physical_device_capabilities[physical_device_ptr->get_device_group_device_index()];

                    break;
                }

                case Anvil::DeviceType::SINGLE_GPU:
                {
                    const Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

                    physical_device_ptr      = sgpu_device_ptr->get_physical_device();
                    physical_device_caps_ptr = &m_physical_device_capabilities[physical_device_ptr->get_device_group_device_index()];

                    break;
                }

                default:
                {
                    anvil_assert_fail();

                    goto end;
                }
            }

            for (uint32_t n_queue_family = 0;
                          n_queue_family < static_cast<uint32_t>(queue_families.size() );
                        ++n_queue_family)
            {
                VkBool32 is_presentation_supported = VK_FALSE;

                {
                    const auto& khr_surface_entrypoints = instance_ptr->get_extension_khr_surface_entrypoints();

                    result = khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_ptr->get_physical_device(),
                                                                                          n_queue_family,
                                                                                          m_surface,
                                                                                         &is_presentation_supported);
                }

                if (is_vk_call_successful(result)         &&
                    is_presentation_supported == VK_TRUE)
                {
                    physical_device_caps_ptr->present_capable_queue_fams.push_back(n_queue_family);
                }
            }
        }
    }
    else
    {
        /* offscreen rendering. Any physical device that offers universal queue can be used to "present" */
        for (uint32_t n_physical_device = 0;
                      n_physical_device < n_physical_devices;
                    ++n_physical_device)
        {
            switch (device_type)
            {
                case Anvil::DeviceType::MULTI_GPU:
                {
                    const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );

                    if (mgpu_device_ptr->get_n_universal_queues() > 0)
                    {
                        const Anvil::PhysicalDevice* physical_device_ptr = mgpu_device_ptr->get_physical_device(n_physical_device);
                        auto&                        result_caps         = m_physical_device_capabilities[physical_device_ptr->get_device_group_device_index()];

                        result_caps.present_capable_queue_fams.push_back(mgpu_device_ptr->get_universal_queue(0)->get_queue_family_index() );
                    }

                    break;
                }

                case Anvil::DeviceType::SINGLE_GPU:
                {
                    const Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

                    if (sgpu_device_ptr->get_n_universal_queues() > 0)
                    {
                        const Anvil::PhysicalDevice* physical_device_ptr = sgpu_device_ptr->get_physical_device();
                        auto&                        result_caps         = m_physical_device_capabilities[physical_device_ptr->get_device_group_device_index()];

                        result_caps.present_capable_queue_fams.push_back(sgpu_device_ptr->get_universal_queue(0)->get_queue_family_index() );
                    }

                    break;
                }

                default:
                {
                    anvil_assert_fail();

                    goto end;
                }
            }
        }

        result = VK_SUCCESS;
    }

    if (!is_vk_call_successful(result) )
    {
        anvil_assert_vk_call_succeeded(result);

        init_successful = false;
    }
    else
    {
        /* Retrieve Vulkan object capabilities and cache them */
        cache_surface_properties();

        init_successful = true;
    }

end:
    return init_successful;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::is_compatible_with_image_format(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                                              Anvil::Format                in_image_format,
                                                              bool*                        out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(in_physical_device_ptr->get_device_group_device_index());
    bool result        = false;

    if (caps_iterator != m_physical_device_capabilities.end() )
    {
        *out_result_ptr = std::find(caps_iterator->second.supported_formats.begin(),
                                    caps_iterator->second.supported_formats.end(),
                                    in_image_format) != caps_iterator->second.supported_formats.end();
        result          = true;
    }

    return result;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::supports_presentation_mode(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                                         Anvil::PresentModeKHR        in_presentation_mode,
                                                         bool*                        out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(in_physical_device_ptr->get_device_group_device_index());
    bool result        = false;

    if (caps_iterator != m_physical_device_capabilities.end() )
    {
        *out_result_ptr = std::find(caps_iterator->second.supported_presentation_modes.begin(),
                                    caps_iterator->second.supported_presentation_modes.end(),
                                    in_presentation_mode) != caps_iterator->second.supported_presentation_modes.end();
        result          = true;
    }

    return result;
}

void Anvil::RenderingSurface::update_surface_extents() const
{
    const Anvil::DeviceType& device_type                   (m_device_ptr->get_type                             () );
    auto                     instance_ptr                  (m_create_info_ptr->get_instance_ptr                () );
    auto                     khr_surface_entrypoints       (instance_ptr->get_extension_khr_surface_entrypoints() );
    const Anvil::MGPUDevice* mgpu_device_ptr               (dynamic_cast<const Anvil::MGPUDevice*>             (m_device_ptr));
    uint32_t                 n_physical_devices            (0);
    const Anvil::SGPUDevice* sgpu_device_ptr               (dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr));
    auto                     window_ptr                    (m_create_info_ptr->get_window_ptr     () );

    if (window_ptr != nullptr)
    {
        const WindowPlatform window_platform(window_ptr->get_platform() );

        if (window_platform == WINDOW_PLATFORM_DUMMY                     ||
            window_platform == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS)
        {
            /* Nothing to update - off-screen rendering is active. */
            goto end;
        }
        else
        {
            /* In this case, width & height may change at run-time */
        }
    }
    else
    {
        /* In this case, width & height may change at run-time */
    }

    switch (device_type)
    {
        case Anvil::DeviceType::MULTI_GPU:  n_physical_devices = mgpu_device_ptr->get_n_physical_devices(); break;
        case Anvil::DeviceType::SINGLE_GPU: n_physical_devices = 1;                                         break;

        default:
        {
            anvil_assert_fail();
        }
    }

    /* Retrieve general properties */
    for (uint32_t n_physical_device = 0;
                  n_physical_device < n_physical_devices;
                ++n_physical_device)
    {
        const Anvil::PhysicalDevice* physical_device_ptr = nullptr;
        VkResult                     result_vk;
        Anvil::SurfaceCapabilities   surface_caps;

        ANVIL_REDUNDANT_VARIABLE_CONST(result_vk);

        switch (device_type)
        {
            case Anvil::DeviceType::MULTI_GPU:  physical_device_ptr = mgpu_device_ptr->get_physical_device(n_physical_device); break;
            case Anvil::DeviceType::SINGLE_GPU: physical_device_ptr = sgpu_device_ptr->get_physical_device();                  break;

            default:
            {
                anvil_assert_fail();
            }
        }

        if (m_surface == VK_NULL_HANDLE)
        {
            /* Nothing to update */
            goto end;
        }

        const VkPhysicalDevice physical_device_vk = physical_device_ptr->get_physical_device();

        result_vk = khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_vk,
                                                                                      m_surface,
                                                                                      reinterpret_cast<VkSurfaceCapabilitiesKHR*>(&surface_caps) );

        anvil_assert_vk_call_succeeded(result_vk);

        if (n_physical_device == 0)
        {
            m_height = surface_caps.current_extent.height;
            m_width  = surface_caps.current_extent.width;
        }
        else
        {
            anvil_assert(m_height == surface_caps.current_extent.height);
            anvil_assert(m_width  == surface_caps.current_extent.width);
        }
    }

end:
    ;
}
