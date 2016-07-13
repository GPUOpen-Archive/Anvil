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

/** Implements a wrapper for a single Vulkan rendering surface. Implemented in order to:
 *
 *  - encapsulate all properties of a rendering surface and expose relevant getters.
 *  - simplify life-time management of rendering surfaces.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_RENDERING_SURFACE_H
#define WRAPPERS_RENDERING_SURFACE_H

#include "misc/types.h"
#include "wrappers/device.h"
#include "wrappers/physical_device.h"
#include "wrappers/queue.h"
#include <algorithm>

namespace Anvil
{
    /* Wrapper class for Vulkan rendering surfaces */
    class RenderingSurface : public RefCounterSupportProvider
    {
    public:
        /* Public type definitions */
        typedef struct RenderingSurfaceFormat
        {
            VkColorSpaceKHR color_space;
            VkFormat        format;

            /* Constructor. */
            RenderingSurfaceFormat(VkSurfaceFormatKHR& in_surface_format)
            {
                color_space = in_surface_format.colorSpace;
                format      = in_surface_format.format;
            }

            /** Comparison operator for _vulkan_surface_format and in_format types.
             *
             *  @param in_format Right-side value.
             *
             *  @return true if @param in_format matches _vulkan_surface_format::format value, false
             *          otherwise.
             **/
            bool operator==(const VkFormat& in_format) const
            {
                return (format == in_format);
            }
        } RenderingSurfaceFormat;

        /* Public functions */

        /** Constructor.
         *
         *  Creates a single Vulkan rendering surface instance and registers the object in
         *  Object Tracker.
         *
         */
        RenderingSurface(Anvil::Instance*                              instance_ptr,
                         Anvil::PhysicalDevice*                        physical_device_ptr,
                         Anvil::Window*                                window_ptr,
#ifdef _WIN32
                         PFN_vkCreateWin32SurfaceKHR                   pfn_create_win32_surface_khr_proc,
#else
                         PFN_vkCreateXcbSurfaceKHR                     pfn_create_xcb_surface_khr_proc,
#endif
                         PFN_vkDestroySurfaceKHR                       pfn_destroy_surface_khr_proc,
                         PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pfn_get_physical_device_surface_capabilities_khr_proc,
                         PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      pfn_get_physical_device_surface_formats_khr_proc,
                         PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pfn_get_physical_device_surface_present_modes_khr_proc,
                         PFN_vkGetPhysicalDeviceSurfaceSupportKHR      pfn_get_physical_device_surface_support_khr_proc);

        /** Returns rendering surface capabilities */
        const VkSurfaceCapabilitiesKHR& get_capabilities() const
        {
            return m_capabilities;
        }

        /** Returns rendering surface's height */
        uint32_t get_height() const
        {
            anvil_assert(m_height != 0);

            return m_height;
        }

        /** Returns physical device which was used to create this surface */
        const Anvil::PhysicalDevice* get_physical_device() const
        {
            return m_physical_device_ptr;
        }

        /** Returns composite alpha modes supported by the rendering surface */
        VkCompositeAlphaFlagsKHR get_supported_composite_alpha_flags() const
        {
            return static_cast<VkCompositeAlphaFlagsKHR>(m_supported_composite_alpha_flags);
        }

        /** Returns transformations supported by the rendering surface */
        VkSurfaceTransformFlagsKHR get_supported_transformations() const
        {
            return static_cast<VkSurfaceTransformFlagsKHR>(m_supported_transformations);
        }

        /** Returns flags corresponding to image usage supported by the rendering surface */
        VkImageUsageFlags get_supported_usages() const
        {
            return static_cast<VkImageUsageFlags>(m_supported_usages);
        }

        /** Retrieves a raw handle to the underlying Vulkan Rendering Surface */
        VkSurfaceKHR get_surface() const
        {
            return m_surface;
        }

        /** Retrieves a pointer to the raw handle to the underlying Vulkan Rendering Surface */
        const VkSurfaceKHR* get_surface_ptr() const
        {
            return &m_surface;
        }

        /** Returns rendering surface's width */
        uint32_t get_width() const
        {
            anvil_assert(m_width != 0);

            return m_width;
        }

        /* Tells whether the specified image format can be used for swapchain image initialization, using
         * this rendering surface. */
        bool is_compatible_with_image_format(VkFormat image_format) const
        {
            return std::find(m_supported_formats.begin(),
                             m_supported_formats.end(),
                             image_format) != m_supported_formats.end();
        }

        /* Tells whether the rendering surface can be presented using the specified queue */
        VkResult supports_presentation_for_queue(Anvil::Queue* queue_ptr,
                                                 VkBool32*     out_result_ptr) const
        {
            return m_vkGetPhysicalDeviceSurfaceSupportKHR(queue_ptr->get_parent_device()->get_physical_device()->get_physical_device(),
                                                          queue_ptr->get_queue_family_index(),
                                                          m_surface,
                                                          out_result_ptr);
        }

        /* Tells whether the specified presentation mode is supported by the rendering surface */
        bool supports_presentation_mode(VkPresentModeKHR presentation_mode) const
        {
            return std::find(m_supported_presentation_modes.begin(),
                             m_supported_presentation_modes.end(),
                             presentation_mode) != m_supported_presentation_modes.end();
        }

    private:
        /* Private functions */
        RenderingSurface           (const RenderingSurface&);
        RenderingSurface& operator=(const RenderingSurface&);

        /** Destructor
         *
         *  Releases the underlying Vulkan rendering surface instance and unregisters the
         *  object from the Object Tracker.
         **/
        virtual ~RenderingSurface();

        void cache_surface_properties(Anvil::Window* window_ptr);

        /* Private variables */
        Anvil::Instance*       m_instance_ptr;
        Anvil::PhysicalDevice* m_physical_device_ptr;

        VkSurfaceCapabilitiesKHR m_capabilities;
        VkSurfaceKHR             m_surface;

        std::vector<RenderingSurfaceFormat> m_supported_formats;
        std::vector<VkPresentModeKHR>       m_supported_presentation_modes;

        uint32_t                      m_height;
        VkCompositeAlphaFlagBitsKHR   m_supported_composite_alpha_flags;
        VkSurfaceTransformFlagBitsKHR m_supported_transformations;
        VkImageUsageFlagBits          m_supported_usages;
        uint32_t                      m_width;

        #ifdef _WIN32
            PFN_vkCreateWin32SurfaceKHR m_vkCreateWin32SurfaceKHR;
        #else
            PFN_vkCreateXcbSurfaceKHR   m_vkCreateXcbSurfaceKHR;
        #endif

        PFN_vkDestroySurfaceKHR                       m_vkDestroySurfaceKHR;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      m_vkGetPhysicalDeviceSurfaceFormatsKHR;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR m_vkGetPhysicalDeviceSurfacePresentModesKHR;
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR      m_vkGetPhysicalDeviceSurfaceSupportKHR;
    };

    /** Delete functor. Useful if you need to wrap the rendering surface instance in an auto pointer */
    struct RenderingSurfaceDeleter
    {
        void operator()(RenderingSurface* surface_ptr) const
        {
            surface_ptr->release();
        }
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_RENDERING_SURFACE_H */
