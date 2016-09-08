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
#include "misc/window.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/queue.h"

/* Please see header for specification */
Anvil::RenderingSurface::RenderingSurface(std::weak_ptr<Anvil::Instance>                instance_ptr,
                                          std::weak_ptr<Anvil::PhysicalDevice>          physical_device_ptr,
                                          std::shared_ptr<Anvil::Window>                window_ptr,
#ifdef _WIN32
                                          PFN_vkCreateWin32SurfaceKHR                   pfn_create_win32_surface_khr_proc,
#else
                                          PFN_vkCreateXcbSurfaceKHR                     pfn_create_xcb_surface_khr_proc,
#endif
                                          PFN_vkDestroySurfaceKHR                       pfn_destroy_surface_khr_proc,
                                          PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pfn_get_physical_device_surface_capabilities_khr_proc,
                                          PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      pfn_get_physical_device_surface_formats_khr_proc,
                                          PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pfn_get_physical_device_surface_present_modes_khr_proc,
                                          PFN_vkGetPhysicalDeviceSurfaceSupportKHR      pfn_get_physical_device_surface_support_khr_proc)

    :m_height                                   (0),
     m_instance_ptr                             (instance_ptr),
     m_physical_device_ptr                      (physical_device_ptr),
     m_surface                                  (VK_NULL_HANDLE),
#ifdef _WIN32
     m_vkCreateWin32SurfaceKHR                  (pfn_create_win32_surface_khr_proc),
#else
     m_vkCreateXcbSurfaceKHR                    (pfn_create_xcb_surface_khr_proc),
#endif
     m_vkDestroySurfaceKHR                      (pfn_destroy_surface_khr_proc),
     m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pfn_get_physical_device_surface_capabilities_khr_proc),
     m_vkGetPhysicalDeviceSurfaceFormatsKHR     (pfn_get_physical_device_surface_formats_khr_proc),
     m_vkGetPhysicalDeviceSurfacePresentModesKHR(pfn_get_physical_device_surface_present_modes_khr_proc),
     m_vkGetPhysicalDeviceSurfaceSupportKHR     (pfn_get_physical_device_surface_support_khr_proc),
     m_width                                    (0)
{
    VkResult result;

    if (!window_ptr->is_dummy())
    {
        std::shared_ptr<Anvil::Instance> instance_locked_ptr(m_instance_ptr);

        #ifdef _WIN32
        {
            VkWin32SurfaceCreateInfoKHR surface_create_info;

            surface_create_info.flags     = 0;
            surface_create_info.hinstance = GetModuleHandle(nullptr);
            surface_create_info.hwnd      = window_ptr->get_handle();
            surface_create_info.pNext     = nullptr;
            surface_create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;

            result = m_vkCreateWin32SurfaceKHR(instance_locked_ptr->get_instance_vk(),
                                              &surface_create_info,
                                               nullptr, /* pAllocator */
                                              &m_surface);
        }
        #else
        {
            VkXcbSurfaceCreateInfoKHR surface_create_info;

            surface_create_info.flags       = 0;
            surface_create_info.window      = window_ptr->get_handle();
            surface_create_info.connection  = static_cast<xcb_connection_t*>(window_ptr->get_connection());
            surface_create_info.pNext       = nullptr;
            surface_create_info.sType       = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;

            result = m_vkCreateXcbSurfaceKHR(instance_locked_ptr->get_instance_vk(),
                                            &surface_create_info,
                                             nullptr, /* pAllocator */
                                            &m_surface);
        }
        #endif
    }
    else
    {
        /* offscreen rendering */
        result = VK_SUCCESS;
    }

    anvil_assert_vk_call_succeeded(result);

    /* Retrieve Vulkan object capabilities and cache them */
    cache_surface_properties(window_ptr);

    /* Register the event instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_RENDERING_SURFACE,
                                                  this);
}

/* Please see header for specification */
Anvil::RenderingSurface::~RenderingSurface()
{
    if (m_surface != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::Instance> instance_locked_ptr(m_instance_ptr);

        m_vkDestroySurfaceKHR(instance_locked_ptr->get_instance_vk(),
                              m_surface,
                              nullptr /* pAllocator */);

        m_surface = VK_NULL_HANDLE;
    }

    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_RENDERING_SURFACE,
                                                    this);
}

/* Please see header for specification */
void Anvil::RenderingSurface::cache_surface_properties(std::shared_ptr<Anvil::Window> window_ptr)
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_physical_device_ptr);

    uint32_t               n_supported_formats            = 0;
    uint32_t               n_supported_presentation_modes = 0;
    const VkPhysicalDevice physical_device_vk             = physical_device_locked_ptr->get_physical_device();
    VkResult               result;
    VkSurfaceFormatKHR*    supported_formats              = nullptr;
    bool                   is_offscreen_rendering_enabled = window_ptr->is_dummy();

    if (is_offscreen_rendering_enabled)
    {
        m_height                          = window_ptr->get_height();
        m_supported_composite_alpha_flags = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        m_supported_transformations       = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;
        m_supported_usages                = static_cast<VkImageUsageFlagBits> (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                                               VK_IMAGE_USAGE_TRANSFER_SRC_BIT     |
                                                                               VK_IMAGE_USAGE_TRANSFER_DST_BIT     |
                                                                               VK_IMAGE_USAGE_STORAGE_BIT);
        m_width                           = window_ptr->get_width();

        return;
    }

    /* not offscreen rendering */

    /* Retrieve general properties */
    result = m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_vk,
                                                         m_surface,
                                                        &m_capabilities);

    anvil_assert_vk_call_succeeded(result);

    m_height                          = m_capabilities.currentExtent.height;
    m_supported_composite_alpha_flags = static_cast<VkCompositeAlphaFlagBitsKHR>  (m_capabilities.supportedCompositeAlpha);
    m_supported_transformations       = static_cast<VkSurfaceTransformFlagBitsKHR>(m_capabilities.supportedTransforms);
    m_supported_usages                = static_cast<VkImageUsageFlagBits>         (m_capabilities.supportedUsageFlags);
    m_width                           = m_capabilities.currentExtent.width;

    /* Retrieve a list of formats supported by the surface */
    result = m_vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_vk,
                                                    m_surface,
                                                   &n_supported_formats,
                                                    nullptr /* pSurfaceFormats */);

    anvil_assert                  (n_supported_formats >  0);
    anvil_assert_vk_call_succeeded(result);

    supported_formats = new VkSurfaceFormatKHR[n_supported_formats];
    anvil_assert(supported_formats != nullptr);

    result = m_vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_vk,
                                                    m_surface,
                                                   &n_supported_formats,
                                                    supported_formats);
    anvil_assert_vk_call_succeeded(result);

    for (unsigned int n_format = 0;
                      n_format < n_supported_formats;
                    ++n_format)
    {
        m_supported_formats.push_back(RenderingSurfaceFormat(supported_formats[n_format]) );
    }

    delete [] supported_formats;
    supported_formats = nullptr;

    /* Retrieve a list of supported presentation modes */
    result = m_vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_vk,
                                                         m_surface,
                                                        &n_supported_presentation_modes,
                                                         nullptr /* pPresentModes */);

    anvil_assert                  (n_supported_presentation_modes >  0);
    anvil_assert_vk_call_succeeded(result);

    m_supported_presentation_modes.resize(n_supported_presentation_modes);

    result = m_vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_vk,
                                                         m_surface,
                                                        &n_supported_presentation_modes,
                                                        &m_supported_presentation_modes[0]);
    anvil_assert_vk_call_succeeded(result);
}

/* Please see header for specification */
std::shared_ptr<Anvil::RenderingSurface> Anvil::RenderingSurface::create(std::weak_ptr<Anvil::Instance>                instance_ptr,
                                                                         std::weak_ptr<Anvil::PhysicalDevice>          physical_device_ptr,
                                                                         std::shared_ptr<Anvil::Window>                window_ptr,
#ifdef _WIN32
                                                                         PFN_vkCreateWin32SurfaceKHR                   pfn_create_win32_surface_khr_proc,
#else
                                                                         PFN_vkCreateXcbSurfaceKHR                     pfn_create_xcb_surface_khr_proc,
#endif
                                                                         PFN_vkDestroySurfaceKHR                       pfn_destroy_surface_khr_proc,
                                                                         PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pfn_get_physical_device_surface_capabilities_khr_proc,
                                                                         PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      pfn_get_physical_device_surface_formats_khr_proc,
                                                                         PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pfn_get_physical_device_surface_present_modes_khr_proc,
                                                                         PFN_vkGetPhysicalDeviceSurfaceSupportKHR      pfn_get_physical_device_surface_support_khr_proc)
{
    std::shared_ptr<Anvil::RenderingSurface> result_ptr;

    result_ptr.reset(
        new Anvil::RenderingSurface(instance_ptr,
                                    physical_device_ptr,
                                    window_ptr,
#ifdef _WIN32
                                    pfn_create_win32_surface_khr_proc,
#else
                                    pfn_create_xcb_surface_khr_proc,
#endif
                                    pfn_destroy_surface_khr_proc,
                                    pfn_get_physical_device_surface_capabilities_khr_proc,
                                    pfn_get_physical_device_surface_formats_khr_proc,
                                    pfn_get_physical_device_surface_present_modes_khr_proc,
                                    pfn_get_physical_device_surface_support_khr_proc)
    );

    return result_ptr;
}