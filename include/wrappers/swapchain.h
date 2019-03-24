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
#include "misc/mt_safety.h"
#include "misc/types.h"
#include "wrappers/device.h"
#include "wrappers/fence.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/rendering_surface.h"

namespace Anvil
{
    /* Wrapper class for a Vulkan Swapchain */
    class Swapchain : public DebugMarkerSupportProvider<Swapchain>,
                      public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        static Anvil::SwapchainUniquePtr create(Anvil::SwapchainCreateInfoUniquePtr in_create_info_ptr);

        /** Destructor.
         *
         *  Destroys the Vulkan counterpart and unregisters the wrapper instance from the Object Tracker.
         **/
        virtual ~Swapchain();

        /** Acquires a new swapchain image.
         *
         *  Can be used for both SGPUDevice and mGPU swapchains.
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
        SwapchainOperationErrorCode acquire_image(Anvil::Semaphore*                   in_opt_semaphore_ptr,
                                                  uint32_t*                           out_result_index_ptr,
                                                  bool                                in_should_block = false);
        SwapchainOperationErrorCode acquire_image(Anvil::Semaphore*                   in_opt_semaphore_ptr,
                                                  uint32_t                            in_n_mgpu_physical_devices,
                                                  const Anvil::PhysicalDevice* const* in_mgpu_physical_device_ptrs,
                                                  uint32_t*                           out_result_index_ptr,
                                                  bool                                in_should_block = false);

        const SwapchainCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        /** Returns height of the swapchain, as specified at creation time */
        uint32_t get_height() const
        {
            return m_size.height;
        }

        /** Return the actual number of swapchain images created. */
        uint32_t get_n_images() const
        {
            return m_n_images;
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
        Anvil::Image* get_image(uint32_t in_n_swapchain_image) const;

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
        Anvil::ImageView* get_image_view(uint32_t in_n_swapchain_image) const;

        /* Returns index of the most recently acquired swapchain image.
         *
         * @return UINT32_MAX if no image was acquired successfully, otherwise as per description.
         **/
        uint32_t get_last_acquired_image_index() const
        {
            return m_last_acquired_image_index;
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

        /** Returns width of the swapchain. */
        uint32_t get_width() const
        {
            return m_size.width;
        }

        /* Associates HDR metadata with one or more swapchains.
         *
         * Requires VK_EXT_hdr_metadata.
         *
         * @param in_n_swapchains     Number of swapchains to update.
         * @param in_swapchains_ptr   At least @param in_n_swapchains swapchains to set HDR metadata for. All defined swapchains must have
         *                            been created for the same Anvil::BaseDevice instance.
         * @param in_hdr_metadata_ptr An array of @param in_n_swapchains HDR metadata descriptors to use. Must not be nullptr.
         */
        static void set_hdr_metadata(const uint32_t&              in_n_swapchains,
                                     Anvil::Swapchain**           in_swapchains_ptr_ptr,
                                     const Anvil::HdrMetadataEXT* in_metadata_items_ptr);

        void set_hdr_metadata(const Anvil::HdrMetadataEXT* in_metadata_ptr)
        {
            Anvil::Swapchain* this_ptr = this;

            return Anvil::Swapchain::set_hdr_metadata(1, /* in_n_swapchains */
                                                     &this_ptr,
                                                      in_metadata_ptr);
        }

        /* By default, swapchain instance will transparently destroy the underlying Vulkan swapchain handle, right before
         * the window is closed.
         *
         * There are certain use cases where we want the order to be reversed (ie. the swapchain handle should be destroyed only after
         * the window is closed). This behavior can be enabled by calling this function with @param in_value set to false.
         */
        void set_should_destroy_swapchain_before_parent_window_closes(const bool& in_value)
        {
            m_destroy_swapchain_before_parent_window_closes = in_value;
        }

        const bool& should_destroy_swapchain_before_parent_window_closes()
        {
            return m_destroy_swapchain_before_parent_window_closes;
        }

    private:
        /* Private functions */

        /* Constructor. Please see create() for specification */
        Swapchain(Anvil::SwapchainCreateInfoUniquePtr in_create_info_ptr);

        Swapchain           (const Swapchain&);
        Swapchain& operator=(const Swapchain&);

        void destroy_swapchain();
        bool init             ();

        void on_parent_window_about_to_close();
        void on_present_request_issued      (Anvil::CallbackArgument* in_callback_raw_ptr);

        /* Private variables */
        Anvil::SwapchainCreateInfoUniquePtr  m_create_info_ptr;
        Anvil::FenceUniquePtr                m_image_available_fence_ptr;
        uint32_t                             m_n_images;  /* number of images created in the swapchain. */
        std::vector<ImageUniquePtr>          m_image_ptrs;
        std::vector<ImageViewUniquePtr>      m_image_view_ptrs;
        uint32_t                             m_last_acquired_image_index;
        VkExtent2D                           m_size;
        VkSwapchainKHR                       m_swapchain;

        bool m_destroy_swapchain_before_parent_window_closes;

        volatile uint64_t m_n_acquire_counter;
        volatile uint32_t m_n_acquire_counter_rounded;
        volatile uint64_t m_n_present_counter;

        std::vector<Anvil::Queue*> m_observed_queues;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SWAPCHAIN_H */