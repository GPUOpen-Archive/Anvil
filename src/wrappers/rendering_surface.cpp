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

/* Please see header for specification */
Anvil::RenderingSurface::RenderingSurface(std::weak_ptr<Anvil::Instance>             instance_ptr,
                                          std::weak_ptr<Anvil::BaseDevice>           device_ptr,
                                          std::shared_ptr<Anvil::Window>             window_ptr,
                                          const ExtensionKHRSurfaceEntrypoints&      khr_surface_entrypoints,
#ifdef _WIN32
                                          const ExtensionKHRWin32SurfaceEntrypoints& khr_win32_surface_entrypoints,
#else
                                          const ExtensionKHRXcbSurfaceEntrypoints&   khr_xcb_surface_entrypoints,
#endif
                                          bool*                                      out_safe_to_use_ptr)
    :m_device_ptr                   (device_ptr),
     m_height                       (0),
     m_instance_ptr                 (instance_ptr),
     m_khr_surface_entrypoints      (khr_surface_entrypoints),
#ifdef _WIN32
     m_khr_win32_surface_entrypoints(khr_win32_surface_entrypoints),
#else
     m_khr_xcb_surface_entrypoints  (khr_xcb_surface_entrypoints),
#endif
     m_surface                      (VK_NULL_HANDLE),
     m_width                        (0)
{
    VkBool32             is_physical_device_supported(VK_FALSE);
    VkResult             result;
    const WindowPlatform window_platform             (window_ptr->get_platform() );

    if (window_platform != WINDOW_PLATFORM_DUMMY                     &&
        window_platform != WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS)
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

            result = m_khr_win32_surface_entrypoints.vkCreateWin32SurfaceKHR(instance_locked_ptr->get_instance_vk(),
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

            result = m_khr_xcb_surface_entrypoints.vkCreateXcbSurfaceKHR(instance_locked_ptr->get_instance_vk(),
                                                                        &surface_create_info,
                                                                         nullptr, /* pAllocator */
                                                                        &m_surface);
        }
        #endif

        /* Is there at least one queue fam that can be used together with at least one physical device associated with
         * the logical device to present using the surface we've just spawned and the physical device user has specified? */
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr     (m_device_ptr);
        uint32_t                           n_physical_devices    (1);
        const auto&                        queue_families        (device_locked_ptr->get_physical_device_queue_families() );
        std::shared_ptr<Anvil::SGPUDevice> sgpu_device_locked_ptr(std::dynamic_pointer_cast<Anvil::SGPUDevice>(device_locked_ptr) );

        for (uint32_t n_physical_device = 0;
                      n_physical_device < n_physical_devices && (is_physical_device_supported == VK_FALSE);
                    ++n_physical_device)
        {
            std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(sgpu_device_locked_ptr->get_physical_device());

            for (uint32_t n_queue_family = 0;
                          n_queue_family < static_cast<uint32_t>(queue_families.size() );
                        ++n_queue_family)
            {
                is_physical_device_supported = VK_FALSE;

                result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_locked_ptr->get_physical_device(),
                                                              n_queue_family,
                                                              m_surface,
                                                             &is_physical_device_supported);

                if (is_vk_call_successful(result)            &&
                    is_physical_device_supported  == VK_TRUE)
                {
                    break;
                }
            }
        }
    }
    else
    {
        /* offscreen rendering */
        is_physical_device_supported = true;
        result                       = VK_SUCCESS;
    }

    if (!is_physical_device_supported)
    {
        anvil_assert_vk_call_succeeded(is_physical_device_supported);

        *out_safe_to_use_ptr = false;
    }
    else
    {
        /* Retrieve Vulkan object capabilities and cache them */
        cache_surface_properties(window_ptr);

        *out_safe_to_use_ptr = true;
    }

    /* Register the instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_RENDERING_SURFACE,
                                                 this);
}

/* Please see header for specification */
Anvil::RenderingSurface::~RenderingSurface()
{
    if (m_surface != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::Instance> instance_locked_ptr(m_instance_ptr);

        m_khr_surface_entrypoints.vkDestroySurfaceKHR(instance_locked_ptr->get_instance_vk(),
                                                      m_surface,
                                                      nullptr /* pAllocator */);

        m_surface = VK_NULL_HANDLE;
    }

    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_RENDERING_SURFACE,
                                                    this);
}

/* Please see header for specification */
void Anvil::RenderingSurface::cache_surface_properties(std::shared_ptr<Anvil::Window> window_ptr)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr     (m_device_ptr);
    std::shared_ptr<Anvil::SGPUDevice> sgpu_device_locked_ptr(std::dynamic_pointer_cast<Anvil::SGPUDevice>(device_locked_ptr));

    const WindowPlatform window_platform                = (window_ptr->get_platform() );
    const bool           is_offscreen_rendering_enabled = (window_platform == WINDOW_PLATFORM_DUMMY                     ||
                                                           window_platform == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS);

    /* Retrieve general properties */
    uint32_t            n_supported_formats           (0);
    uint32_t            n_supported_presentation_modes(0);
    VkResult            result                        (VK_ERROR_INITIALIZATION_FAILED);
    VkSurfaceFormatKHR* supported_formats             (nullptr);

    ANVIL_REDUNDANT_VARIABLE(result);

    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(sgpu_device_locked_ptr->get_physical_device());

    auto& result_caps = m_physical_device_capabilities[0];

    if (is_offscreen_rendering_enabled)
    {
        m_height                                    = window_ptr->get_height();
        result_caps.supported_composite_alpha_flags = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        result_caps.supported_transformations       = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;
        result_caps.supported_usages                = static_cast<VkImageUsageFlagBits> (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT     |
                                                                                         VK_IMAGE_USAGE_TRANSFER_DST_BIT     |
                                                                                         VK_IMAGE_USAGE_STORAGE_BIT);
        m_width                                     = window_ptr->get_width();

        return;
    }

    const VkPhysicalDevice physical_device_vk = physical_device_locked_ptr->get_physical_device();

    result = m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_vk,
                                                                                 m_surface,
                                                                                &result_caps.capabilities);

    anvil_assert_vk_call_succeeded(result);

    m_height = result_caps.capabilities.currentExtent.height;
    m_width  = result_caps.capabilities.currentExtent.width;

    result_caps.supported_composite_alpha_flags = static_cast<VkCompositeAlphaFlagBitsKHR>  (result_caps.capabilities.supportedCompositeAlpha);
    result_caps.supported_transformations       = static_cast<VkSurfaceTransformFlagBitsKHR>(result_caps.capabilities.supportedTransforms);
    result_caps.supported_usages                = static_cast<VkImageUsageFlagBits>         (result_caps.capabilities.supportedUsageFlags);

    /* Retrieve a list of formats supported by the surface */
    result = m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_vk,
                                                                            m_surface,
                                                                           &n_supported_formats,
                                                                            nullptr /* pSurfaceFormats */);

    anvil_assert                  (n_supported_formats >  0);
    anvil_assert_vk_call_succeeded(result);

    supported_formats = new VkSurfaceFormatKHR[n_supported_formats];
    anvil_assert(supported_formats != nullptr);

    result = m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_vk,
                                                                            m_surface,
                                                                           &n_supported_formats,
                                                                            supported_formats);
    anvil_assert_vk_call_succeeded(result);

    for (unsigned int n_format = 0;
                      n_format < n_supported_formats;
                    ++n_format)
    {
        result_caps.supported_formats.push_back(RenderingSurfaceFormat(supported_formats[n_format]) );
    }

    delete [] supported_formats;
    supported_formats = nullptr;

    /* Retrieve a list of supported presentation modes */
    result = m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_vk,
                                                                                 m_surface,
                                                                                &n_supported_presentation_modes,
                                                                                 nullptr /* pPresentModes */);

    anvil_assert                  (n_supported_presentation_modes >  0);
    anvil_assert_vk_call_succeeded(result);

    result_caps.supported_presentation_modes.resize(n_supported_presentation_modes);

    result = m_khr_surface_entrypoints.vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_vk,
                                                                                 m_surface,
                                                                                &n_supported_presentation_modes,
                                                                                &result_caps.supported_presentation_modes[0]);
    anvil_assert_vk_call_succeeded(result);
}

/* Please see header for specification */
std::shared_ptr<Anvil::RenderingSurface> Anvil::RenderingSurface::create(std::weak_ptr<Anvil::Instance>             instance_ptr,
                                                                         std::weak_ptr<Anvil::BaseDevice>           device_ptr,
                                                                         std::shared_ptr<Anvil::Window>             window_ptr,
                                                                         const ExtensionKHRSurfaceEntrypoints&      khr_surface_entrypoints,
#ifdef _WIN32
                                                                         const ExtensionKHRWin32SurfaceEntrypoints& khr_win32_surface_entrypoints
#else
                                                                         const ExtensionKHRXcbSurfaceEntrypoints&   khr_xcb_surface_entrypoints
#endif
)
{
    std::shared_ptr<Anvil::RenderingSurface> result_ptr;
    bool                                     success    = false;

    result_ptr.reset(
        new Anvil::RenderingSurface(instance_ptr,
                                    device_ptr,
                                    window_ptr,
                                    khr_surface_entrypoints,
#ifdef _WIN32
                                    khr_win32_surface_entrypoints,
#else
                                    khr_xcb_surface_entrypoints,
#endif
                                   &success) );

    if (!success)
    {
        result_ptr.reset();
    }

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::get_capabilities(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr,
                                               VkSurfaceCapabilitiesKHR*            out_surface_caps_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(0);
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
bool Anvil::RenderingSurface::get_supported_composite_alpha_flags(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr,
                                                                  VkCompositeAlphaFlagsKHR*            out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(0);
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
bool Anvil::RenderingSurface::get_supported_transformations(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr,
                                                            VkSurfaceTransformFlagsKHR*          out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(0);
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
bool Anvil::RenderingSurface::get_supported_usages(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr,
                                                   VkImageUsageFlags*                   out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(0);
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
bool Anvil::RenderingSurface::is_compatible_with_image_format(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr,
                                                              VkFormat                             image_format,
                                                              bool*                                out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(0);
    bool result        = false;

    if (caps_iterator != m_physical_device_capabilities.end() )
    {
        *out_result_ptr = std::find(caps_iterator->second.supported_formats.begin(),
                                    caps_iterator->second.supported_formats.end(),
                                    image_format) != caps_iterator->second.supported_formats.end();
        result          = true;
    }

    return result;
}

/* Please see header for specification */
bool Anvil::RenderingSurface::supports_presentation_mode(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr,
                                                         VkPresentModeKHR                     presentation_mode,
                                                         bool*                                out_result_ptr) const
{
    auto caps_iterator = m_physical_device_capabilities.find(0);
    bool result        = false;

    if (caps_iterator != m_physical_device_capabilities.end() )
    {
        *out_result_ptr = std::find(caps_iterator->second.supported_presentation_modes.begin(),
                                    caps_iterator->second.supported_presentation_modes.end(),
                                    presentation_mode) != caps_iterator->second.supported_presentation_modes.end();
        result          = true;
    }

    return result;
}