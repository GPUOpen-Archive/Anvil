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

#include "misc/swapchain_create_info.h"


Anvil::SwapchainCreateInfoUniquePtr Anvil::SwapchainCreateInfo::create(Anvil::BaseDevice*       in_device_ptr,
                                                                       Anvil::RenderingSurface* in_parent_surface_ptr,
                                                                       Anvil::Window*           in_window_ptr,
                                                                       Anvil::Format            in_format,
                                                                       Anvil::ColorSpaceKHR     in_color_space,
                                                                       Anvil::PresentModeKHR    in_present_mode,
                                                                       Anvil::ImageUsageFlags   in_usage_flags,
                                                                       uint32_t                 in_n_images,
                                                                       const bool&              in_clipped,
                                                                       const Anvil::Swapchain*  in_opt_old_swapchain_ptr)
{
    SwapchainCreateInfoUniquePtr result_ptr = SwapchainCreateInfoUniquePtr(nullptr,
                                                                           std::default_delete<Anvil::SwapchainCreateInfo>() );

    result_ptr.reset(
        new SwapchainCreateInfo(in_device_ptr,
                                in_parent_surface_ptr,
                                in_window_ptr,
                                in_format,
                                in_color_space,
                                in_present_mode,
                                in_usage_flags,
                                in_n_images,
                                Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE,
                                Anvil::SwapchainCreateFlagBits::NONE,
                                Anvil::DeviceGroupPresentModeFlagBits::LOCAL_BIT_KHR, /* in_mgpu_present_mode_flags */
                                in_clipped,
                                in_opt_old_swapchain_ptr)
    );

    return result_ptr;
}

Anvil::SwapchainCreateInfo::SwapchainCreateInfo(Anvil::BaseDevice*                 in_device_ptr,
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
                                                const Anvil::Swapchain*            in_opt_old_swapchain_ptr)
    :m_clipped                (in_clipped),
     m_color_space            (in_color_space),
     m_device_ptr             (in_device_ptr),
     m_flags                  (in_flags),
     m_format                 (in_format),
     m_mgpu_present_mode_flags(in_mgpu_present_mode_flags),
     m_mt_safety              (in_mt_safety),
     m_n_images               (in_n_images),
     m_old_swapchain_ptr      (in_opt_old_swapchain_ptr),
     m_parent_surface_ptr     (in_parent_surface_ptr),
     m_present_mode           (in_present_mode),
     m_usage_flags            (in_usage_flags),
     m_window_ptr             (in_window_ptr)
{
    anvil_assert(in_n_images           >  0);
    anvil_assert(in_parent_surface_ptr != nullptr);
    anvil_assert(in_usage_flags        != 0);
}

void Anvil::SwapchainCreateInfo::set_view_format_list(const Anvil::Format* in_compatible_formats_ptr,
                                                      const uint32_t&      in_n_compatible_formats)
{
    anvil_assert(in_n_compatible_formats > 0);

    m_flags |= Anvil::SwapchainCreateFlagBits::CREATE_MUTABLE_FORMAT_BIT;

    m_compatible_formats.resize(in_n_compatible_formats);

    memcpy(&m_compatible_formats.at(0),
           in_compatible_formats_ptr,
           sizeof(Anvil::Format) * in_n_compatible_formats);
}