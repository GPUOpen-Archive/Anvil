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
         * - MT safety:               MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         * - Swapchain create flags:  0
         *
         * To modify these, use corresponding set_..() functions.
         */
        static SwapchainCreateInfoUniquePtr create(Anvil::BaseDevice*       in_device_ptr,
                                                   Anvil::RenderingSurface* in_parent_surface_ptr,
                                                   Anvil::Window*           in_window_ptr,
                                                   VkFormat                 in_format,
                                                   VkPresentModeKHR         in_present_mode,
                                                   VkImageUsageFlags        in_usage_flags,
                                                   uint32_t                 in_n_images);

        /** Returns device instance which has been used to create the swapchain */
        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        /** Returns flags used to create the swapchain. */
        const VkSwapchainCreateFlagsKHR& get_flags() const
        {
            return m_flags;
        }

        /** Returns format used by swapchain image and image views */
        VkFormat get_format() const
        {
            return m_format;
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

        VkPresentModeKHR get_present_mode() const
        {
            return m_present_mode;
        }

        /** Retrieves parent rendering surface. */
        const Anvil::RenderingSurface* get_rendering_surface() const
        {
            return m_parent_surface_ptr;
        }

        const VkImageUsageFlags& get_usage_flags() const
        {
            return m_usage_flags;
        }

        /** Retrieves a window, to which the swapchain is bound. Note that under certain
         *  circumstances no window may be assigned. */
        Anvil::Window* get_window() const
        {
            return m_window_ptr;
        }

        void set_device(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        void set_flags(const VkSwapchainCreateFlagsKHR& in_flags)
        {
            m_flags = in_flags;
        }

        void set_format(const VkFormat& in_format)
        {
            m_format = in_format;
        }

        void set_mt_safety(const MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        void set_n_images(const uint32_t& in_n_images)
        {
            m_n_images = in_n_images;
        }

        void set_present_mode(const VkPresentModeKHR& in_present_mode)
        {
            m_present_mode = in_present_mode;
        }

        void set_rendering_surface(Anvil::RenderingSurface* in_rendering_surface_ptr)
        {
            m_parent_surface_ptr = in_rendering_surface_ptr;
        }

        void set_usage_flags(VkImageUsageFlags in_new_usage_flags)
        {
            m_usage_flags = in_new_usage_flags;
        }

        void set_window(Anvil::Window* in_window_ptr)
        {
            m_window_ptr = in_window_ptr;
        }


    private:
        /* Private functions */

        SwapchainCreateInfo(Anvil::BaseDevice*        in_device_ptr,
                            Anvil::RenderingSurface*  in_parent_surface_ptr,
                            Anvil::Window*            in_window_ptr,
                            VkFormat                  in_format,
                            VkPresentModeKHR          in_present_mode,
                            VkImageUsageFlags         in_usage_flags,
                            uint32_t                  in_n_images,
                            MTSafety                  in_mt_safety,
                            VkSwapchainCreateFlagsKHR in_flags);

        /* Private variables */

        const Anvil::BaseDevice*  m_device_ptr;
        VkSwapchainCreateFlagsKHR m_flags;
        VkFormat                  m_format;
        Anvil::MTSafety           m_mt_safety;
        uint32_t                  m_n_images;
        Anvil::RenderingSurface*  m_parent_surface_ptr;
        VkPresentModeKHR          m_present_mode;
        Anvil::Window*            m_window_ptr;

        VkImageUsageFlagsVariable(m_usage_flags);

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(SwapchainCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(SwapchainCreateInfo);
    };
}; /* namespace Anvil */

#endif /* MISC_SWAPCHAIN_CREATE_INFO_H */