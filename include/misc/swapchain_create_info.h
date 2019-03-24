//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef MISC_SWAPCHAIN_CREATE_INFO_H
#define MISC_SWAPCHAIN_CREATE_INFO_H

#include "misc/types.h"

namespace Anvil
{
    class SwapchainCreateInfo
    {
    public:
        /* Public functions */

        /* TODO.
         *
         * NOTE: By default, the following parameters take default values as below.
         *
         * - MGPU present mode flags: Anvil::DeviceGroupPresentModeFlagBits::LOCAL_BIT_KHR
         * - MT safety:               Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         * - Swapchain create flags:  None
         *
         * To modify these, use corresponding set_..() functions.
         */
        static SwapchainCreateInfoUniquePtr create(Anvil::BaseDevice*       in_device_ptr,
                                                   Anvil::RenderingSurface* in_parent_surface_ptr,
                                                   Anvil::Window*           in_window_ptr,
                                                   Anvil::Format            in_format,
                                                   Anvil::ColorSpaceKHR     in_color_space,
                                                   Anvil::PresentModeKHR    in_present_mode,
                                                   Anvil::ImageUsageFlags   in_usage_flags,
                                                   uint32_t                 in_n_images,
                                                   const bool&              in_clipped               = true,
                                                   const Anvil::Swapchain*  in_opt_old_swapchain_ptr = nullptr);

        const bool& get_clipped() const
        {
            return m_clipped;
        }

        Anvil::ColorSpaceKHR get_color_space() const
        {
            return m_color_space;
        }

        void get_view_format_list(const Anvil::Format** out_compatible_formats_ptr,
                                  uint32_t*             out_n_compatible_formats_ptr) const
        {
            anvil_assert(m_compatible_formats.size() > 0);

            *out_compatible_formats_ptr   = &m_compatible_formats.at(0);
            *out_n_compatible_formats_ptr = static_cast<uint32_t>   (m_compatible_formats.size() );
        }

        /** Returns device instance which has been used to create the swapchain */
        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        /** Returns flags used to create the swapchain. */
        const Anvil::SwapchainCreateFlags& get_flags() const
        {
            return m_flags;
        }

        /** Returns format used by swapchain image and image views */
        Anvil::Format get_format() const
        {
            return m_format;
        }

        Anvil::DeviceGroupPresentModeFlags get_mgpu_present_mode_flags() const
        {
            return m_mgpu_present_mode_flags;
        }

        const MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }

        /** Tells how many images the swap-chain encapsulates. */
        uint32_t get_n_images() const
        {
            return m_n_images;
        }

        const Anvil::Swapchain* get_old_swapchain() const
        {
            return m_old_swapchain_ptr;
        }

        Anvil::PresentModeKHR get_present_mode() const
        {
            return m_present_mode;
        }

        /** Retrieves parent rendering surface. */
        const Anvil::RenderingSurface* get_rendering_surface() const
        {
            return m_parent_surface_ptr;
        }

        const Anvil::ImageUsageFlags& get_usage_flags() const
        {
            return m_usage_flags;
        }

        /** Retrieves a window, to which the swapchain is bound. Note that under certain
         *  circumstances no window may be assigned. */
        Anvil::Window* get_window() const
        {
            return m_window_ptr;
        }

        void set_clipped(const bool& in_clipped)
        {
            m_clipped = in_clipped;
        }

        void set_color_space(const Anvil::ColorSpaceKHR& in_color_space)
        {
            m_color_space = in_color_space;
        }

        void set_device(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        /* NOTE: If @param in_flags includes SwapchainCreateFlagBits::CREATE_MUTABLE_FORMAT_BIT, you must
         *       also call set_view_format_list() in order to specify the list of compatible formats BEFORE
         *       passing the create info struct to swapchain create function.
         */
        void set_flags(const Anvil::SwapchainCreateFlags& in_flags)
        {
            m_flags = in_flags;
        }

        void set_format(const Anvil::Format& in_format)
        {
            m_format = in_format;
        }

        void set_mgpu_present_mode_flags(const Anvil::DeviceGroupPresentModeFlags& in_mgpu_present_mode_flags)
        {
            m_mgpu_present_mode_flags = in_mgpu_present_mode_flags;
        }

        void set_mt_safety(const MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        void set_n_images(const uint32_t& in_n_images)
        {
            m_n_images = in_n_images;
        }

        void set_old_swapchain(const Anvil::Swapchain* in_old_swapchain_ptr)
        {
            m_old_swapchain_ptr = in_old_swapchain_ptr;
        }

        void set_present_mode(const Anvil::PresentModeKHR& in_present_mode)
        {
            m_present_mode = in_present_mode;
        }

        void set_rendering_surface(Anvil::RenderingSurface* in_rendering_surface_ptr)
        {
            m_parent_surface_ptr = in_rendering_surface_ptr;
        }

        void set_usage_flags(Anvil::ImageUsageFlags in_new_usage_flags)
        {
            m_usage_flags = in_new_usage_flags;
        }

        /* Caches a list of image formats the created swapchain needs to be able to support.
         *
         * If SwapchainCreateFlagBits::CREATE_MUTABLE_FORMAT_BIT create flag has not been specified via set_flags(), it will
         * be force-set by the function.
         *
         * @param in_compatible_formats_ptr Array of formats swapchain images need to support for image view usage. Must not be nullptr.
         * @param in_n_compatible_formats   Size of @param in_compatible_Formats_ptr array. Must be larger than 0.
         *
         * Requires VK_KHR_swapchain_mutable_format extension support.
         */
        void set_view_format_list(const Anvil::Format* in_compatible_formats_ptr,
                                  const uint32_t&      in_n_compatible_formats);

        void set_window(Anvil::Window* in_window_ptr)
        {
            m_window_ptr = in_window_ptr;
        }


    private:
        /* Private functions */

        SwapchainCreateInfo(Anvil::BaseDevice*                 in_device_ptr,
                            Anvil::RenderingSurface*           in_parent_surface_ptr,
                            Anvil::Window*                     in_window_ptr,
                            Anvil::Format                      in_format,
                            Anvil::ColorSpaceKHR               in_color_space,
                            Anvil::PresentModeKHR              in_present_mode,
                            Anvil::ImageUsageFlags             in_usage_flags,
                            uint32_t                           in_n_images,
                            MTSafety                           in_mt_safety,
                            Anvil::SwapchainCreateFlags        in_flags,
                            Anvil::DeviceGroupPresentModeFlags in_mgpu_present_mode_flags,
                            const bool&                        in_clipped,
                            const Anvil::Swapchain*            in_opt_old_swapchain_ptr);

        /* Private variables */

        bool                               m_clipped;
        Anvil::ColorSpaceKHR               m_color_space;
        std::vector<Anvil::Format>         m_compatible_formats;
        const Anvil::BaseDevice*           m_device_ptr;
        Anvil::SwapchainCreateFlags        m_flags;
        Anvil::Format                      m_format;
        Anvil::DeviceGroupPresentModeFlags m_mgpu_present_mode_flags;
        Anvil::MTSafety                    m_mt_safety;
        uint32_t                           m_n_images;
        const Anvil::Swapchain*            m_old_swapchain_ptr;
        Anvil::RenderingSurface*           m_parent_surface_ptr;
        Anvil::PresentModeKHR              m_present_mode;
        Anvil::Window*                     m_window_ptr;

        Anvil::ImageUsageFlags m_usage_flags;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(SwapchainCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(SwapchainCreateInfo);
    };
}; /* namespace Anvil */

#endif /* MISC_SWAPCHAIN_CREATE_INFO_H */