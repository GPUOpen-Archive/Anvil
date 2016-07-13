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

/** Implements device-wide swapchain wrapper. Implemented in order to:
 *
 *  - encapsulate all objects useful when manipulating the swapchain.
 *  - let ObjectTracker detect swapchain leaks.
 *
 *  Swapchain images are transferred to PRESENT_SOURCE image layout at creation time.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_SWAPCHAIN_H
#define WRAPPERS_SWAPCHAIN_H

#include "misc/ref_counter.h"
#include "misc/types.h"
#include "wrappers/device.h"
#include "wrappers/fence.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/rendering_surface.h"

namespace Anvil
{
    /* Wrapper class for a Vulkan Swapchain */
    class Swapchain : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  @param device_ptr                    Device to initialize the swapchain for.
         *  @param parent_surface_ptr            Rendering surface the swapchain is to be created for. Must
         *                                       not be nullptr.
         *  @param window_ptr                    current window to create the swapchain for. Must not be nullptr.
         *  @param format                        Format to use for the swapchain image.
         *  @param present_mode                  Presentation mode to use for the swapchain.
         *  @param usage_flags                   Image usage flags to use for the swapchain.
         *  @param n_images                      Number of swapchain images to use for the swapchain.
         *  @param pfn_acquire_next_image_proc   Func pointer to device-specific vkAcquireNextImageKHR() implementation.
         *                                       Must not be nullptr.
         *  @param pfn_create_swapchain_proc     Func pointer to device-specific vkCreateSwapchainKHR() implementation.
         *                                       Must not be nullptr.
         *  @param pfn_destroy_swapchain_proc    Func pointer to device-specific vkDestroySwapchainKHR() implementation.
         *                                       Must not be nullptr.
         *  @param pfn_get_swapchain_images_proc Func pointer to device-specific vkGetSwapchainImages() implementation.
         *                                       Must not be nullptr.
         *
         */
        Swapchain(Anvil::Device*              device_ptr,
                  Anvil::RenderingSurface*    parent_surface_ptr,
                  Anvil::Window*              window_ptr,
                  VkFormat                    format,
                  VkPresentModeKHR            present_mode,
                  VkImageUsageFlags           usage_flags,
                  uint32_t                    n_images,
                  PFN_vkAcquireNextImageKHR   pfn_acquire_next_image_proc,
                  PFN_vkCreateSwapchainKHR    pfn_create_swapchain_proc,
                  PFN_vkDestroySwapchainKHR   pfn_destroy_swapchain_proc,
                  PFN_vkGetSwapchainImagesKHR pfn_get_swapchain_images_proc);

        /** Acquires a new swapchain image and waits until it becomes available before returning
         *  control to the caller.
         *
         *  @return Index of the swapchain image that has been acquired.
         */
        uint32_t acquire_image_by_blocking();

        /** Acquires a new swapchain image. Does NOT block until the image becomes available, but instead
         *  sets the specified semaphore.
         *
         *  @param semaphore_ptr Semaphore to set. Must NOT be nullptr.
         *
         *  @return Index of the swapchain image that the commands should be submitted against.
         **/
        uint32_t acquire_image_by_setting_semaphore(Anvil::Semaphore* semaphore_ptr);

        /** Returns format used by swapchain images */
        VkFormat get_image_format() const
        {
            return m_image_format;
        }

        /** Returns format used by swapchain image views. */
        VkFormat get_image_view_format() const
        {
            return m_image_view_format;
        }

        /** Retrieves an Image instance associated with a swapchain image at index
         *  @param n_swapchain_image.
         *
         *  Format of all swapchain images can be retrieved by caslling get_image_format().
         *
         *  @param n_swapchain_image Swapchain image index to use for the call.
         *                           Must be smaller than @param n_images specified at construction
         *                           time.
         *
         *  @return As per summary.
         **/
        Anvil::Image* get_image(uint32_t n_swapchain_image) const;

        /** Retrieves an Image View instance associated with a swapchain image at index
         *  @param n_swapchain_image.
         *
         *  Format of all swapchain image views can be retrieved by caslling get_image_view_format().
         *
         *  @param n_swapchain_image Swapchain image index to use for the call.
         *                           Must be smaller than @param n_images specified at construction
         *                           time.
         *
         *  @return As per summary.
         **/
        Anvil::ImageView* get_image_view(uint32_t n_swapchain_image) const;

        /** Tells how many images the swap-chain encapsulates. */
        uint32_t get_n_images() const
        {
            return m_n_swapchain_images;
        }

        /** Retrieves parent rendering surface. */
        const Anvil::RenderingSurface* get_rendering_surface() const
        {
            return m_parent_surface_ptr;
        }

        /** Retrieves a pointer to the raw Vulkan swapchain handle.  */
        const VkSwapchainKHR* get_swapchain_ptr() const
        {
            return &m_swapchain;
        }

        /** Deletes all instantiated image views and re-creates a new set with user-specified
         *  format.
         *
         *  @param new_view_format New format to use for the image views. Must not be the same
         *                         as currently used image view format.
         **/
        void set_image_view_format(VkFormat new_view_format);

    private:
        /* Private functions */
        Swapchain           (const Swapchain&);
        Swapchain& operator=(const Swapchain&);

        virtual ~Swapchain();

        void     init     ();


        /* Private variables */
        Anvil::Device*           m_device_ptr;
        Anvil::Window*           m_window_ptr;
        Anvil::Fence**           m_image_available_fence_ptrs;
        VkFormat                 m_image_format;
        Anvil::Image**           m_image_ptrs;
        VkFormat                 m_image_view_format;
        Anvil::ImageView**       m_image_view_ptrs;
        uint32_t                 m_n_swapchain_images;
        Anvil::RenderingSurface* m_parent_surface_ptr;
        VkPresentModeKHR         m_present_mode;
        VkSwapchainKHR           m_swapchain;
        VkImageUsageFlagBits     m_usage_flags;

        PFN_vkAcquireNextImageKHR   m_vkAcquireNextImageKHR;
        PFN_vkCreateSwapchainKHR    m_vkCreateSwapchainKHR;
        PFN_vkDestroySwapchainKHR   m_vkDestroySwapchainKHR;
        PFN_vkGetSwapchainImagesKHR m_vkGetSwapchainImagesKHR;

        uint32_t m_n_acquire_counter;
    };

    /* Delete functor. Useful for wrapping Swapchain instances in auto pointers. */
    struct SwapchainDeleter
    {
        void operator()(Swapchain* swapchain_ptr)
        {
            swapchain_ptr->release();
        }
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SWAPCHAIN_H */