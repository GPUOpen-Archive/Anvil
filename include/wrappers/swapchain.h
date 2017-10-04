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

#include "misc/debug_marker.h"
#include "misc/types.h"
#include "wrappers/device.h"
#include "wrappers/fence.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/rendering_surface.h"

namespace Anvil
{
    /* Wrapper class for a Vulkan Swapchain */
    class Swapchain : public DebugMarkerSupportProvider<Swapchain>
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  @param in_device_ptr                Device to initialize the swapchain for.
         *  @param in_parent_surface_ptr        Rendering surface the swapchain is to be created for. Must
         *                                      not be nullptr.
         *  @param in_window_ptr                current window to create the swapchain for. Must not be nullptr.
         *  @param in_format                    Format to use for the swapchain image.
         *  @param in_present_mode              Presentation mode to use for the swapchain.
         *  @param in_usage_flags               Image usage flags to use for the swapchain.
         *  @param in_n_images                  Number of swapchain images to use for the swapchain.
         *  @param in_khr_swapchain_entrypoints VK_KHR_swapchain entrypoint container.
         *  @param in_flags                     Swapchain create flags to pass, when creating the swapchain.
         *
         */

        static std::shared_ptr<Swapchain> create(std::weak_ptr<Anvil::BaseDevice>         in_device_ptr,
                                                 std::shared_ptr<Anvil::RenderingSurface> in_parent_surface_ptr,
                                                 std::shared_ptr<Anvil::Window>           in_window_ptr,
                                                 VkFormat                                 in_format,
                                                 VkPresentModeKHR                         in_present_mode,
                                                 VkImageUsageFlags                        in_usage_flags,
                                                 uint32_t                                 in_n_images,
                                                 const ExtensionKHRSwapchainEntrypoints&  in_khr_swapchain_entrypoints,
                                                 VkSwapchainCreateFlagsKHR                in_flags                     = 0);

        /** Destructor.
         *
         *  Destroys the Vulkan counterpart and unregisters the wrapper instance from the Object Tracker.
         **/
        virtual ~Swapchain();

        /** Acquires a new swapchain image.
         *
         *  @param in_semaphore_ptr Semaphore to set upon frame acquisition. May be nullptr (assuming the implications are understood!)
         *  @param in_should_block  Set to true, if you want to wait on a fence set by vkAcquireNextImage*KHR() functions
         *                          which are called by this function. The wrapper instantiates a unique fence for each allotted swapchain
         *                          image, in order to defer the CPU-side wait in time.
         *                          MUST be true if you need a CPU/GPU sync point (examples include doing CPU writes to memory which is going
         *                          to be accessed by the GPU while rendering the frame).
         *
         *  @return Index of the swapchain image that the commands should be submitted against.
         **/
        uint32_t acquire_image(std::shared_ptr<Anvil::Semaphore> in_opt_semaphore_ptr,
                               bool                              in_should_block = false);

        /* By default, swapchain instance will transparently destroy the underlying Vulkan swapchain handle, right before
         * the window is closed.
         *
         * There are certain use cases where we want the order to be reversed (ie. the swapchain handle should be destroyed only after
         * the window is closed). This function can be used to enable this behavior.
         *
         */
        void disable_destroy_swapchain_before_parent_window_closes_behavior();

        /** Returns device instance which has been used to create the swapchain */
        std::weak_ptr<Anvil::BaseDevice> get_device() const
        {
            return m_device_ptr;
        }

        /** Returns flags used to create the swapchain. */
        const VkSwapchainCreateFlagsKHR& get_flags() const
        {
            return m_flags;
        }

        /** Returns height of the swapchain, as specified at creation time */
        uint32_t get_height() const
        {
            return m_size.height;
        }

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
         *  @param in_n_swapchain_image.
         *
         *  Format of all swapchain images can be retrieved by caslling get_image_format().
         *
         *  @param in_n_swapchain_image Swapchain image index to use for the call.
         *                              Must be smaller than @param in_n_images specified at construction
         *                              time.
         *
         *  @return As per summary.
         **/
        std::shared_ptr<Anvil::Image> get_image(uint32_t in_n_swapchain_image) const;

        /** Retrieves an Image View instance associated with a swapchain image at index
         *  @param n_swapchain_image.
         *
         *  Format of all swapchain image views can be retrieved by caslling get_image_view_format().
         *
         *  @param in_n_swapchain_image Swapchain image index to use for the call.
         *                              Must be smaller than @param in_n_images specified at construction
         *                              time.
         *
         *  @return As per summary.
         **/
        std::shared_ptr<Anvil::ImageView> get_image_view(uint32_t in_n_swapchain_image) const;

        /* Returns index of the most recently acquired swapchain image.
         *
         * @return UINT32_MAX if no image was acquired successfully, otherwise as per description.
         **/
        uint32_t get_last_acquired_image_index() const
        {
            return m_last_acquired_image_index;
        }

        /** Tells how many images the swap-chain encapsulates. */
        uint32_t get_n_images() const
        {
            return m_n_swapchain_images;
        }

        /** Retrieves parent rendering surface. */
        std::shared_ptr<const Anvil::RenderingSurface> get_rendering_surface() const
        {
            return m_parent_surface_ptr;
        }

        /** Retrieves a pointer to the raw Vulkan swapchain handle.  */
        const VkSwapchainKHR* get_swapchain_ptr() const
        {
            return &m_swapchain;
        }

        VkSwapchainKHR get_swapchain_vk() const
        {
            return m_swapchain;
        }

        /** Returns width of the swapchain, as specified at creation time */
        uint32_t get_width() const
        {
            return m_size.width;
        }

        /** Retrieves a window, to which the swapchain is bound. Note that under certain
         *  circumstances no window may be assigned. */
        std::shared_ptr<Anvil::Window> get_window() const
        {
            return m_window_ptr;
        }

    private:
        /* Private functions */

        /* Constructor. Please see create() for specification */
        Swapchain(std::weak_ptr<Anvil::BaseDevice>         in_device_ptr,
                  std::shared_ptr<Anvil::RenderingSurface> in_parent_surface_ptr,
                  std::shared_ptr<Anvil::Window>           in_window_ptr,
                  VkFormat                                 in_format,
                  VkPresentModeKHR                         in_present_mode,
                  VkImageUsageFlags                        in_usage_flags,
                  uint32_t                                 in_n_images,
                  VkSwapchainCreateFlagsKHR                in_flags,
                  const ExtensionKHRSwapchainEntrypoints&  in_khr_swapchain_entrypoints);

        Swapchain           (const Swapchain&);
        Swapchain& operator=(const Swapchain&);

        void destroy_swapchain();
        void init             ();

        static void on_parent_window_about_to_close(void* in_window_ptr,
                                                    void* in_swapchain_raw_ptr);
        static void on_present_request_issued      (void* in_queue_raw_ptr,
                                                    void* in_swapchain_raw_ptr);

        /* Private variables */
        bool                                            m_destroy_swapchain_before_parent_window_closes;
        std::weak_ptr<Anvil::BaseDevice>                m_device_ptr;
        VkSwapchainCreateFlagsKHR                       m_flags;
        std::vector<std::shared_ptr<Anvil::Fence> >     m_image_available_fence_ptrs;
        VkFormat                                        m_image_format;
        std::vector<std::shared_ptr<Anvil::Image> >     m_image_ptrs;
        VkFormat                                        m_image_view_format;
        std::vector<std::shared_ptr<Anvil::ImageView> > m_image_view_ptrs;
        uint32_t                                        m_last_acquired_image_index;
        uint32_t                                        m_n_swapchain_images;
        std::shared_ptr<Anvil::RenderingSurface>        m_parent_surface_ptr;
        VkPresentModeKHR                                m_present_mode;
        VkSwapchainKHR                                  m_swapchain;
        std::shared_ptr<Anvil::Window>                  m_window_ptr;

        VkExtent2D m_size;

        VkImageUsageFlagsVariable(m_usage_flags);

        const ExtensionKHRSwapchainEntrypoints m_khr_swapchain_entrypoints;

        volatile uint64_t m_n_acquire_counter;
        volatile uint32_t m_n_acquire_counter_rounded;
        volatile uint64_t m_n_present_counter;

        std::vector<std::shared_ptr<Anvil::Queue> > m_observed_queues;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SWAPCHAIN_H */