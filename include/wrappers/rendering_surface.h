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

/** Implements a wrapper for a single Vulkan rendering surface. Implemented in order to:
 *
 *  - encapsulate all properties of a rendering surface and expose relevant getters.
 *  - simplify life-time management of rendering surfaces.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_RENDERING_SURFACE_H
#define WRAPPERS_RENDERING_SURFACE_H

#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"
#include "wrappers/device.h"
#include "wrappers/physical_device.h"
#include "wrappers/queue.h"
#include <algorithm>

namespace Anvil
{
    /* Wrapper class for Vulkan rendering surfaces */
    class RenderingSurface : public DebugMarkerSupportProvider<RenderingSurface>,
                             public MTSafetySupportProvider
    {
    public:
        /* Public type definitions */

        /* Public functions */

        /** Creates a single Vulkan rendering surface instance and registers the object in
         *  Object Tracker. */
        static Anvil::RenderingSurfaceUniquePtr create(Anvil::RenderingSurfaceCreateInfoUniquePtr in_create_info_ptr);

        /** Destructor
         *
         *  Releases the underlying Vulkan rendering surface instance and unregisters the
         *  object from the Object Tracker.
         **/
        virtual ~RenderingSurface();

        /** Returns rendering surface capabilities */
        bool get_capabilities(const Anvil::PhysicalDevice* in_physical_device_ptr,
                              Anvil::SurfaceCapabilities*  out_surface_caps_ptr) const;

        /** Returns rendering surface's height */
        uint32_t get_height() const
        {
            anvil_assert(m_height != 0);

            return m_height;
        }

        /** Returns logical device which was used to create this surface */
        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        /** Returns queue family indices which support presentation on a given physical device */
        bool get_queue_families_with_present_support(const Anvil::PhysicalDevice*  in_physical_device_ptr,
                                                     const std::vector<uint32_t>** out_result_ptr) const;

        /** Returns composite alpha modes supported by the rendering surface */
        bool get_supported_composite_alpha_flags(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                                 Anvil::CompositeAlphaFlags*  out_result_ptr) const;

        /** Returns transformations supported by the rendering surface */
        bool get_supported_transformations(const Anvil::PhysicalDevice*  in_physical_device_ptr,
                                           Anvil::SurfaceTransformFlags* out_result_ptr) const;

        /** Returns flags corresponding to image usage supported by the rendering surface */
        bool get_supported_usages(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                  ImageUsageFlags*             out_result_ptr) const;

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
        bool is_compatible_with_image_format(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                             Anvil::Format                in_image_format,
                                             bool*                        out_result_ptr) const;

        /* Tells whether the specified presentation mode is supported by the rendering surface */
        bool supports_presentation_mode(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                        Anvil::PresentModeKHR        in_presentation_mode,
                                        bool*                        out_result_ptr) const;

        void update_surface_extents() const;

    private:
        /* Private type definitions */
        typedef uint32_t DeviceGroupIndex;

        typedef struct RenderingSurfaceFormat
        {
            Anvil::ColorSpaceKHR color_space;
            Anvil::Format        format;

            /* Constructor. */
            RenderingSurfaceFormat(Anvil::SurfaceFormatKHR& in_surface_format)
            {
                color_space = in_surface_format.color_space;
                format      = static_cast<Anvil::Format>(in_surface_format.format);
            }

            /** Comparison operator for _vulkan_surface_format and in_format types.
             *
             *  @param in_format Right-side value.
             *
             *  @return true if @param in_format matches _vulkan_surface_format::format value, false
             *          otherwise.
             **/
            bool operator==(const Anvil::Format& in_format) const
            {
                return (format == in_format);
            }
        } RenderingSurfaceFormat;

        typedef struct PhysicalDeviceCapabilities
        {
            Anvil::SurfaceCapabilities          capabilities;
            std::vector<RenderingSurfaceFormat> supported_formats;
            std::vector<Anvil::PresentModeKHR>  supported_presentation_modes;
            Anvil::SurfaceTransformFlags        supported_transformations;
            ImageUsageFlags                     supported_usages;

            std::vector<uint32_t>               present_capable_queue_fams;

            Anvil::CompositeAlphaFlags          supported_composite_alpha_flags;

            PhysicalDeviceCapabilities()
            {
                /* Stub */
            }
        } PhysicalDeviceCapabilities;

        /* Private functions */

        /* Constructor. Please see create() for specification */
        RenderingSurface(Anvil::RenderingSurfaceCreateInfoUniquePtr in_create_info_ptr);

        RenderingSurface           (const RenderingSurface&);
        RenderingSurface& operator=(const RenderingSurface&);

        void cache_surface_properties();
        bool init                    ();

        /* Private variables */
        Anvil::RenderingSurfaceCreateInfoUniquePtr m_create_info_ptr;

        mutable uint32_t                                       m_height;
        std::map<DeviceGroupIndex, PhysicalDeviceCapabilities> m_physical_device_capabilities;
        VkSurfaceKHR                                           m_surface;
        mutable uint32_t                                       m_width;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_RENDERING_SURFACE_H */
