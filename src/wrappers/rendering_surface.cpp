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
#include "misc/window.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/queue.h"
#include "config.h"

/* Please see header for specification */
Anvil::RenderingSurface::RenderingSurface(Anvil::Instance*         in_instance_ptr,
                                          const Anvil::BaseDevice* in_device_ptr,
                                          const Anvil::Window*     in_window_ptr,
                                          bool                     in_mt_safe,
                                          bool*                    out_safe_to_use_ptr)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT),
     MTSafetySupportProvider   (in_mt_safe),
     m_device_ptr              (in_device_ptr),
     m_height                  (0),
     m_instance_ptr            (in_instance_ptr),
     m_surface                 (VK_NULL_HANDLE),
     m_width                   (0),
     m_window_ptr              (in_window_ptr)
{
    *out_safe_to_use_ptr = init();

    /* Register the instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_RENDERING_SURFACE,
                                                 this);
}

/* Please see header for specification */
Anvil::RenderingSurface::~RenderingSurface()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_RENDERING_SURFACE,
                                                    this);

    if (m_surface != VK_NULL_HANDLE)
    {
        lock();
        {
            m_instance_ptr->get_extension_khr_surface_entrypoints().vkDestroySurfaceKHR(m_instance_ptr->get_instance_vk(),
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
    const Anvil::DeviceType&        device_type                   (m_device_ptr->get_type() );
    bool                            is_offscreen_rendering_enabled(true);
    auto                            khr_surface_entrypoints       (m_instance_ptr->get_extension_khr_surface_entrypoints() );
    uint32_t                        n_physical_devices            (0);
    const Anvil::SGPUDevice*        sgpu_device_ptr               (dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr));
    std::vector<VkSurfaceFormatKHR> supported_formats;

    if (m_window_ptr != nullptr)
    {
        const WindowPlatform window_platform(m_window_ptr->get_platform() );

        is_offscreen_rendering_enabled = (window_platform == WINDOW_PLATFORM_DUMMY                     ||
                                          window_platform == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS);

        if (is_offscreen_rendering_enabled)
        {
            m_height = m_window_ptr->get_height_at_creation_time();
            m_width  = m_window_ptr->get_width_at_creation_time ();
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
        case Anvil::DEVICE_TYPE_SINGLE_GPU: n_physical_devices = 1;break;

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
            case Anvil::DEVICE_TYPE_SINGLE_GPU: physical_device_ptr = sgpu_device_ptr->get_physical_device(); break;

            default:
            {
                anvil_assert_fail();
            }
        }

        auto& result_caps = m_physical_device_capabilities;

        if (m_surface == VK_NULL_HANDLE)
        {
            result_caps.supported_composite_alpha_flags = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
            result_caps.supported_transformations       = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;
            result_caps.supported_usages                = static_cast<VkImageUsageFlagBits> (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                                                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT     |
                                                                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT     |
                                                                                             VK_IMAGE_USAGE_STORAGE_BIT);

            result_caps.supported_presentation_modes.push_back(VK_PRESENT_MODE_IMMEDIATE_KHR);

            continue;
        }

        const VkPhysicalDevice physical_device_vk = physical_device_ptr->get_physical_device();

        result = khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_vk,
                                                                                   m_surface,
                                                                                  &result_caps.capabilities);

        anvil_assert_vk_call_succeeded(result);

        if (n_physical_device == 0)
        {
            m_height = result_caps.capabilities.currentExtent.height;
            m_width  = result_caps.capabilities.currentExtent.width;
        }
        else
        {
            anvil_assert(m_height == result_caps.capabilities.currentExtent.height);
            anvil_assert(m_width  == result_caps.capabilities.currentExtent.width);
        }

        result_caps.supported_composite_alpha_flags = static_cast<VkCompositeAlphaFlagBitsKHR>  (result_caps.capabilities.supportedCompositeAlpha);
        result_caps.supported_transformations       = static_cast<VkSurfaceTransformFlagBitsKHR>(result_caps.capabilities.supportedTransforms);
        result_caps.supported_usages                = static_cast<VkImageUsageFlagBits>         (result_caps.capabilities.supportedUsageFlags);

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
                                                                             &supported_formats.at(0) );
        anvil_assert_vk_call_succeeded(result);

        for (unsigned int n_format = 0;
                          n_format < n_supported_formats;
                        ++n_format)
        {
            result_caps.supported_formats.push_back(RenderingSurfaceFormat(supported_formats[n_format]) );
        }

        /* Retrieve a list of supported presentation modes */
        result = khr_surface_entrypoints.vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_vk,
                                                                                   m_surface,
                                                                                  &n_supported_presentation_modes,
                                                                                   nullptr /* pPresentModes */);

        anvil_assert_vk_call_succeeded(result);

        if (n_supported_presentation_modes > 0)
        {
            result_caps.supported_presentation_modes.resize(n_supported_presentation_modes);

            result = khr_surface_entrypoints.vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_vk,
                                                                                       m_surface,
                                                                                      &n_supported_presentation_modes,
                                                                                      &result_caps.supported_presentation_modes.at(0));
            anvil_assert_vk_call_succeeded(result);
        }
    }
}

/* Please see header for specification */
Anvil::RenderingSurfaceUniquePtr Anvil::RenderingSurface::create(Anvil::Instance*         in_instance_ptr,
                                                                 const Anvil::BaseDevice* in_device_ptr,
                                                                 const Anvil::Window*     in_window_ptr,
                                                                 MTSafety                 in_mt_safety)
{
    const bool                mt_safe    = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                           in_device_ptr);
    RenderingSurfaceUniquePtr result_ptr(nullptr,
                                         std::default_delete<RenderingSurface>() );
    bool                      success    = false;

    result_ptr.reset(
        new Anvil::RenderingSurface(in_instance_ptr,
                                    in_device_ptr,
                                    in_window_ptr,
                                    mt_safe,
                                   &success) );

    if (!success)
    {
        result_ptr.reset();
    }

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_capabilities(VkSurfaceCapabilitiesKHR* out_surface_caps_ptr) const
{
    *out_surface_caps_ptr = m_physical_device_capabilities.capabilities;

    return true;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_queue_families_with_present_support(const std::vector<uint32_t>** out_result_ptr) const
{
    *out_result_ptr = &m_physical_device_capabilities.present_capable_queue_fams;

    /* All done */
    return true;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_supported_composite_alpha_flags(VkCompositeAlphaFlagsKHR* out_result_ptr) const
{
    *out_result_ptr = m_physical_device_capabilities.supported_composite_alpha_flags;

    /* All done */
    return true;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_supported_transformations(VkSurfaceTransformFlagsKHR* out_result_ptr) const
{
    *out_result_ptr = m_physical_device_capabilities.supported_transformations;

    /* All done */
    return true;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_supported_usages(VkImageUsageFlags* out_result_ptr) const
{
    *out_result_ptr = m_physical_device_capabilities.supported_usages;

    /* All done */
    return true;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::init()
{
    const Anvil::DeviceType& device_type       (m_device_ptr->get_type() );
    bool                     init_successful   (false);
    uint32_t                 n_physical_devices(0);
    VkResult                 result            (VK_SUCCESS);
    const WindowPlatform     window_platform   (m_window_ptr->get_platform());

    const bool               is_dummy_window_platform(window_platform == WINDOW_PLATFORM_DUMMY                     ||
                                                      window_platform == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS);


    switch (device_type)
    {
        case Anvil::DEVICE_TYPE_SINGLE_GPU:
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
        #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT) && defined(_WIN32)
        {
            VkWin32SurfaceCreateInfoKHR surface_create_info;

            surface_create_info.flags     = 0;
            surface_create_info.hinstance = GetModuleHandle(nullptr);
            surface_create_info.hwnd      = m_window_ptr->get_handle();
            surface_create_info.pNext     = nullptr;
            surface_create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;

            result = m_instance_ptr->get_extension_khr_win32_surface_entrypoints().vkCreateWin32SurfaceKHR(m_instance_ptr->get_instance_vk(),
                                                                                                          &surface_create_info,
                                                                                                           nullptr, /* pAllocator */
                                                                                                          &m_surface);
        }
        #endif
        #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT) && !defined(_WIN32)
        {
            VkXcbSurfaceCreateInfoKHR surface_create_info;

            surface_create_info.flags       = 0;
            surface_create_info.window      = m_window_ptr->get_handle();
            surface_create_info.connection  = static_cast<xcb_connection_t*>(m_window_ptr->get_connection());
            surface_create_info.pNext       = nullptr;
            surface_create_info.sType       = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;

            result = m_instance_ptr->get_extension_khr_xcb_surface_entrypoints().vkCreateXcbSurfaceKHR(m_instance_ptr->get_instance_vk(),
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
                case Anvil::DEVICE_TYPE_SINGLE_GPU:
                {
                    const Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

                    physical_device_ptr      = sgpu_device_ptr->get_physical_device();
                    physical_device_caps_ptr = &m_physical_device_capabilities;

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
                    result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_ptr->get_physical_device(),
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
                case Anvil::DEVICE_TYPE_SINGLE_GPU:
                {
                    const Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

                    if (sgpu_device_ptr->get_n_universal_queues() > 0)
                    {
                        auto& result_caps = m_physical_device_capabilities;

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
bool Anvil::RenderingSurface::is_compatible_with_image_format(VkFormat in_image_format,
                                                              bool*    out_result_ptr) const
{
    *out_result_ptr = std::find(m_physical_device_capabilities.supported_formats.begin(),
                                m_physical_device_capabilities.supported_formats.end(),
                                in_image_format) != m_physical_device_capabilities.supported_formats.end();

    return true;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::supports_presentation_mode(VkPresentModeKHR in_presentation_mode,
                                                         bool*            out_result_ptr) const
{
    *out_result_ptr = std::find(m_physical_device_capabilities.supported_presentation_modes.begin(),
                                m_physical_device_capabilities.supported_presentation_modes.end(),
                                in_presentation_mode) != m_physical_device_capabilities.supported_presentation_modes.end();

    return true;
}