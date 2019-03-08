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

/** Implements a wrapper for a single Vulkan queue. Implemented in order to:
 *
 *  - encapsulate all state related to a single queue.
 *  - let ObjectTracker detect leaking queue wrapper instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_QUEUE_H
#define WRAPPERS_QUEUE_H

#include "misc/callbacks.h"
#include "misc/debug.h"
#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    /** TODO */
    typedef struct LocalModePresentationItem
    {
        /* Tells which physical device's swapchain image at index @param in_swapchain_image_index should be presented. */
        const Anvil::PhysicalDevice* physical_device_ptr;

        uint32_t          swapchain_image_index;
        Anvil::Swapchain* swapchain_ptr;

        LocalModePresentationItem()
        {
            swapchain_image_index = ~0u;
            swapchain_ptr         = nullptr;
        }
    } LocalModePresentationItem;

    /** TODO */
    typedef struct SumModePresentationItem
    {
        uint32_t                            n_physical_devices;
        const Anvil::PhysicalDevice* const* physical_devices_ptr;
        uint32_t                            swapchain_image_index;
        Anvil::Swapchain*                   swapchain_ptr;

        SumModePresentationItem()
        {
            n_physical_devices    = ~0u;
            physical_devices_ptr  = nullptr;
            swapchain_image_index = ~0u;
            swapchain_ptr         = nullptr;
        }
    } SumModePresentationItem;

    /** TODO */
    typedef SumModePresentationItem LocalMultiDeviceModePresentationItem;

    /** TODO */
    typedef LocalModePresentationItem RemoteModePresentationItem;

    typedef enum
    {
        /* Notification fired right after vkQueuePresentKHR() has been issued for a swapchain.
         *
         * callback_arg: Pointer to OnPresentRequestIssuedCallbackArgument instance.
         */
        QUEUE_CALLBACK_ID_PRESENT_REQUEST_ISSUED,

        QUEUE_CALLBACK_ID_COUNT
    } QueueCallbacKID;

    class Queue : public CallbacksSupportProvider,
                  public DebugMarkerSupportProvider<Queue>,
                  public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Initializes a new Vulkan queue instance.
         *
         *  NOTE: This function must only be used by Anvil::*Device!
         *
         *  @param in_device_ptr         Device to retrieve the queue from.
         *  @param in_queue_family_index Index of the queue family to retrieve the queue from.
         *  @param in_queue_index        Index of the queue to retrieve.
         *  @param in_mt_safe            True if queue submissions should be protected by a mutex, guaranteeing
         *                               no more than one thread at a time will ever submit a cmd buffer to the
         *                               same cmd queue.
         *  @param in_global_priority    Global priority of the new queue. Setting this value to anything else than Anvil::QueueGlobalPriority::MEDIUM_EXT
         *                               requires VK_EXT_queue_global_priority support.
         **/
        static std::unique_ptr<Anvil::Queue> create(const Anvil::BaseDevice*          in_device_ptr,
                                                    uint32_t                          in_queue_family_index,
                                                    uint32_t                          in_queue_index,
                                                    bool                              in_mt_safe,
                                                    const Anvil::QueueGlobalPriority& in_global_priority = Anvil::QueueGlobalPriority::MEDIUM_EXT);

        /** Destructor */
        virtual ~Queue();

        /** Starts a queue debug label region. App must call end_debug_utils_label() for the same queue instance
         *  at some point to declare the end of the label region.
         *
         *  Requires VK_EXT_debug_utils support. Otherwise, the call is moot.
         *
         *  @param in_label_name_ptr Meaning as per VkDebugUtilsLabelEXT::pLabelName. Must not be nullptr.
         *  @param in_color_vec4_ptr Meaning as per VkDebugUtilsLabelEXT::color. Must not be nullptr.
         */
        void begin_debug_utils_label(const char*  in_label_name_ptr,
                                     const float* in_color_vec4_ptr);

        /** Updates sparse resource memory bindings using this queue.
         *
         *  @param in_update Detailed information about the update to be carried out.
         **/
        bool bind_sparse_memory(Anvil::SparseMemoryBindingUpdateInfo& in_update);

        /** Ends a queue debug label region. Requires a preceding begin_debug_utils_label() call.
         *
         *  Requires VK_EXT_debug_utils support. Otherwise, the call is moot.
         *
         */
        void end_debug_utils_label();

        /** Retrieves parent device instance */
        const Anvil::BaseDevice* get_parent_device() const
        {
            return m_device_ptr;
        }

        /** Retrieves raw Vulkan queue handle. */
        const VkQueue& get_queue() const
        {
            return m_queue;
        }

        /** Retrieves family index of the queue */
        uint32_t get_queue_family_index() const
        {
            return m_queue_family_index;
        }

        /** Retrieves global priority used to create the queue.
         *
         *  Only meaningful if VK_EXT_queue_global_priority if supported.
         */
        const Anvil::QueueGlobalPriority& get_queue_global_priority() const
        {
            return m_queue_global_priority;
        }

        /** Retrieves index of the queue */
        uint32_t get_queue_index() const
        {
            return m_queue_index;
        }

        /** Inserts a single queue debug label.
         *
         *  Requires VK_EXT_debug_utils support. Otherwise, the call is moot.
         *
         *  @param in_label_name_ptr Meaning as per VkDebugUtilsLabelEXT::pLabelName. Must not be nullptr.
         *  @param in_color_vec4_ptr Meaning as per VkDebugUtilsLabelEXT::color. Must not be nullptr.
         */
        void insert_debug_utils_label(const char*  in_label_name_ptr,
                                      const float* in_color_vec4_ptr);

        /** Presents the specified swapchain image using this queue. This queue must support presentation
         *  for the swapchain's rendering surface in order for this call to succeed.
         *
         *  This function will only succeed if supports_presentation() returns true.
         *  This function will only succeed if used for a single-GPU device instance.
         *
         *  NOTE: If you are presenting to an off-screen window, make sure to transition
         *        the image to Anvil::ImageLayout::GENERAL, instead of Anvil::ImageLayout::PRESENT_SRC_KHR.
         *        In off-screen rendering mode, swapchain images are actually regular images, so
         *        presentable layout is not supported.
         *
         *  @param in_swapchain_ptr           Swapchain to use for the operation. Must not be NULL.
         *  @param in_swapchain_image_index   Index of the swapchain image to present.
         *  @param in_n_wait_semaphores       Number of semaphores defined under @param in_wait_semaphore_ptrs. These sems will
         *                                    be waited on before presentation occurs.
         *  @param in_wait_semaphore_ptrs_ptr Ptr to an array of @param in_n_wait_semaphores semaphore wrapper instances. May be nullptr
         *                                    if @param in_n_wait_semaphores is 0.
         *
         *  @return Vulkan result for the operation.
         **/
        bool present(Anvil::Swapchain*                   in_swapchain_ptr,
                     uint32_t                            in_swapchain_image_index,
                     uint32_t                            in_n_wait_semaphores,
                     Anvil::Semaphore* const*            in_wait_semaphore_ptrs_ptr,
                     Anvil::SwapchainOperationErrorCode* out_present_results_ptr);

        /** See present() documentation for general information about this function.
         *
         *  This function can be called for both single-GPU and multi-GPU devices.
         *
         *  @param in_n_local_mode_presentation_items TODO
         *  @param in_local_mode_presentation_items   TODO
         *  @param in_n_wait_semaphores               TODO
         *  @param in_wait_semaphore_ptrs_ptr         TODO
         *
         *  @return TODO
         **/
        bool present_in_local_presentation_mode(uint32_t                            in_n_local_mode_presentation_items,
                                                const LocalModePresentationItem*    in_local_mode_presentation_items,
                                                uint32_t                            in_n_wait_semaphores,
                                                Anvil::Semaphore* const*            in_wait_semaphore_ptrs_ptr,
                                                Anvil::SwapchainOperationErrorCode* out_present_results_ptr);

        /** See present() documentation for general information about this function.
         *
         *  This function can be called for both single-GPU and multi-GPU devices.
         *
         *  @param in_n_local_multi_device_mode_presentation_items TODO
         *  @param in_local_multi_device_mode_presentation_items   TODO
         *  @param in_n_wait_semaphores                            TODO
         *  @param in_wait_semaphore_ptrs_ptr                      TODO
         *
         *  @return TODO
         **/
        bool present_in_local_multi_device_presentation_mode(uint32_t                                    in_n_local_multi_device_mode_presentation_items,
                                                             const LocalMultiDeviceModePresentationItem* in_local_multi_device_mode_presentation_items,
                                                             uint32_t                                    in_n_wait_semaphores,
                                                             Anvil::Semaphore* const*                    in_wait_semaphore_ptrs_ptr,
                                                             Anvil::SwapchainOperationErrorCode*         out_present_results_ptr);

        /** See present() documentation for general information about this function.
         *
         *  This function can be called for multi-GPU devices.
         *
         *  @param in_n_remote_mode_presentation_items TODO
         *  @param in_remote_mode_presentation_items   TODO
         *  @param in_n_wait_semaphores                TODO
         *  @param in_wait_semaphore_ptrs_ptr          TODO
         *
         *  @return TODO
         **/
        bool present_in_remote_presentation_mode(uint32_t                            in_n_remote_mode_presentation_items,
                                                 const RemoteModePresentationItem*   in_remote_mode_presentation_items,
                                                 uint32_t                            in_n_wait_semaphores,
                                                 Anvil::Semaphore* const*            in_wait_semaphore_ptrs_ptr,
                                                 Anvil::SwapchainOperationErrorCode* out_present_results_ptr);

        /** See present() documentation for general information about this function.
         *
         *  This function can be called for multi-GPU devices.
         *
         *  @param in_n_sum_mode_presentation_items TODO
         *  @param in_sum_mode_presentation_items   TODO
         *  @param in_n_wait_semaphores             TODO
         *  @param in_wait_semaphore_ptrs_ptr       TODO
         *
         *  @return TODO
         **/
        bool present_in_sum_presentation_mode(uint32_t                            in_n_sum_mode_presentation_items,
                                              const SumModePresentationItem*      in_sum_mode_presentation_items,
                                              uint32_t                            in_n_wait_semaphores,
                                              Anvil::Semaphore* const*            in_wait_semaphore_ptrs_ptr,
                                              Anvil::SwapchainOperationErrorCode* out_present_results_ptr);

        bool submit(const SubmitInfo& in_submit_info);

        /** Tells whether the queue supports protected memory operations */
        bool supports_protected_memory_operations() const
        {
            return m_supports_protected_memory_operations;
        }

        /** Tells whether the queue supports sparse bindings */
        bool supports_sparse_bindings() const
        {
            return m_supports_sparse_bindings;
        }

        void wait_idle();

    private:
        /* Private functions */
        bool present_internal   (Anvil::DeviceGroupPresentModeFlagBits in_presentation_mode,
                                 uint32_t                              in_n_swapchains,
                                 Anvil::Swapchain* const*              in_swapchains,
                                 const uint32_t*                       in_swapchain_image_indices,
                                 const uint32_t*                       in_device_masks,
                                 uint32_t                              in_n_wait_semaphores,
                                 Anvil::Semaphore* const*              in_wait_semaphore_ptrs,
                                 Anvil::SwapchainOperationErrorCode*   out_present_results_ptr);
        void present_lock_unlock(uint32_t                              in_n_swapchains,
                                 const Anvil::Swapchain* const*        in_swapchains,
                                 uint32_t                              in_n_wait_semaphores,
                                 Anvil::Semaphore* const*              in_wait_semaphore_ptrs,
                                 bool                                  in_should_lock);

        void bind_sparse_memory_lock_unlock    (Anvil::SparseMemoryBindingUpdateInfo& in_update,
                                                bool                                  in_should_lock);
        void submit_command_buffers_lock_unlock(uint32_t                              in_n_command_buffers,
                                                Anvil::CommandBufferBase* const*      in_opt_cmd_buffer_ptrs_ptr,
                                                uint32_t                              in_n_semaphores_to_signal,
                                                Anvil::Semaphore* const*              in_opt_semaphore_to_signal_ptrs_ptr,
                                                uint32_t                              in_n_semaphores_to_wait_on,
                                                Anvil::Semaphore* const*              in_opt_semaphore_to_wait_on_ptrs_ptr,
                                                Anvil::Fence*                         in_opt_fence_ptr,
                                                bool                                  in_should_lock);
        void submit_command_buffers_lock_unlock(uint32_t                              in_n_command_buffer_submissions,
                                                const CommandBufferMGPUSubmission*    in_opt_command_buffer_submissions_ptr,
                                                uint32_t                              in_n_signal_semaphore_submissions,
                                                const SemaphoreMGPUSubmission*        in_opt_signal_semaphore_submissions_ptr,
                                                uint32_t                              in_n_wait_semaphore_submissions,
                                                const SemaphoreMGPUSubmission*        in_opt_wait_semaphore_submissions_ptr,
                                                Anvil::Fence*                         in_opt_fence_ptr,
                                                bool                                  in_should_lock);

        /* Constructor. Please see create() for specification */
        Queue(const Anvil::BaseDevice*          in_device_ptr,
              uint32_t                          in_queue_family_index,
              uint32_t                          in_queue_index,
              bool                              in_mt_safe,
              const Anvil::QueueGlobalPriority& in_global_priority);

        Queue          (const Queue&);
        Queue operator=(const Queue&);

        /* Private variables */
        const Anvil::BaseDevice*         m_device_ptr;
        uint32_t                         m_n_debug_label_regions_started;
        VkQueue                          m_queue;
        const uint32_t                   m_queue_family_index;
        const Anvil::QueueGlobalPriority m_queue_global_priority;
        const uint32_t                   m_queue_index;
        Anvil::FenceUniquePtr            m_submit_fence_ptr;
        bool                             m_supports_protected_memory_operations;
        bool                             m_supports_sparse_bindings;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_QUEUE_H */