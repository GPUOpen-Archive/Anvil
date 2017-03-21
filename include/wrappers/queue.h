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

/** Implements a wrapper for a single Vulkan queue. Implemented in order to:
 *
 *  - encapsulate all state related to a single queue.
 *  - let ObjectTracker detect leaking queue wrapper instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_QUEUE_H
#define WRAPPERS_QUEUE_H

#include "misc/debug.h"
#include "misc/types.h"

namespace Anvil
{
    class Queue
    {
    public:
        /* Public functions */

        /** Initializes a new Vulkan queue instance.
         *
         *  After construction, set_swapchain() should be called individually
         *  to assign a swapchain to the queue. This is necessary in order for
         *  present() calls to work correctly.
         *
         *  @param device_ptr         Device to retrieve the queue from.
         *  @param queue_family_index Index of the queue family to retrieve the queue from.
         *  @param queue_index        Index of the queue to retrieve.
         **/
        static std::shared_ptr<Anvil::Queue> create(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                    uint32_t                         queue_family_index,
                                                    uint32_t                         queue_index);

        /** Destructor */
        ~Queue();

        /** Updates sparse resource memory bindings using this queue.
         *
         *  @param update Detailed information about the update to be carried out.
         **/
        bool bind_sparse_memory(Anvil::Utils::SparseMemoryBindingUpdateInfo& update);

        /** Retrieves parent device instance */
        std::weak_ptr<Anvil::BaseDevice> get_parent_device() const
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

        /** Presents the specified swapchain image using this queue.
         *
         *  This function will only succeed if supports_presentation() returns true.
         *
         *  NOTE: If you are presenting to an off-screen window, make sure to transition
         *        the image to VK_IMAGE_LAYOUT_GENERAL, instead of VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
         *        In off-screen rendering mode, swapchain images are actually regular images, so
         *        presentable layout is not supported.
         *
         *  @param swapchain_ptr         Swapchain to use for the operation. Must not be NULL.
         *  @param swapchain_image_index Index of the swapchain image to present.
         *  @param n_wait_semaphores     Number of semaphores defined under @param wait_semaphore_ptrs. These sems will
         *                               be waited on before presentation occurs.
         *  @param wait_semaphore_pts    An array of @param n_wait_semaphores semaphore wrapper instances. May be nullptr
         *                               if @param n_wait_semaphores is 0.
         *
         *  @return Vulkan result for the operation.
         **/
        VkResult present(std::shared_ptr<Anvil::Swapchain>  swapchain_ptr,
                         uint32_t                           swapchain_image_index,
                         uint32_t                           n_wait_semaphores,
                         std::shared_ptr<Anvil::Semaphore>* wait_semaphore_ptrs);

        /** Waits on the semaphores specified by the user, executes user-defined command buffers,
         *  and then signals the semaphores passed by arguments. If a non-nullptr fence is specified,
         *  the function will also wait on the call to finish GPU-side before returning.
         *
         *  It is valid to specify 0 command buffers or signal/wait semaphores, in which case
         *  these steps will be skipped.
         *
         *  If @param should_block is true and @param opt_fence_ptr is nullptr, the function will create
         *  a new fence, wait on it, and then release it prior to leaving. This may come at a performance cost.
         *
         *  @param n_command_buffers                   Number of command buffers under @param opt_cmd_buffer_ptrs
         *                                             which should be executed. May be 0.
         *  @param opt_cmd_buffer_ptrs                 Array of command buffers to execute. Can be nullptr if
         *                                             @param n_command_buffers is 0.
         *  @param n_semaphores_to_signal              Number of semaphores to signal after command buffers finish
         *                                             executing. May be 0.
         *  @param opt_semaphore_to_signal_ptr_ptrs    Array of semaphores to signal after execution. May be nullptr if
         *                                             @param n_semaphores_to_signal is 0.
         *  @param n_semaphores_to_wait_on             Number of semaphores to wait on before executing command buffers.
         *                                             May be 0.
         *  @param opt_semaphore_to_wait_on_ptr_ptrs   Array of semaphores to wait on prior to execution. May be nullptr
         *                                             if @param n_semaphores_to_wait_on is 0.
         *  @param opt_dst_stage_masks_to_wait_on_ptrs Array of size @param n_semaphores_to_wait_on, specifying stages
         *                                             at which the wait ops should be performed. May be nullptr if
         *                                             @param n_semaphores_to_wait_on is 0.
         *  @param should_block                        true if the function should wait for the scheduled commands to
         *                                             finish executing, false otherwise.
         *  @param opt_fence_ptr                       Fence to use when submitting the comamnd buffers to the queue.
         *                                             The fence will be waited on if @param should_block is true.
         *                                             If @param should_block is false, the fence will be passed at
         *                                             submit call time, but will not be waited on.
         **/
        void submit_command_buffers(uint32_t                                         n_command_buffers,
                                    std::shared_ptr<Anvil::CommandBufferBase> const* opt_cmd_buffer_ptrs,
                                    uint32_t                                         n_semaphores_to_signal,
                                    std::shared_ptr<Anvil::Semaphore> const*         opt_semaphore_to_signal_ptr_ptrs,
                                    uint32_t                                         n_semaphores_to_wait_on,
                                    std::shared_ptr<Anvil::Semaphore> const*         opt_semaphore_to_wait_on_ptr_ptrs,
                                    const VkPipelineStageFlags*                      opt_dst_stage_masks_to_wait_on_ptrs,
                                    bool                                             should_block,
                                    std::shared_ptr<Anvil::Fence>                    opt_fence_ptr                = std::shared_ptr<Anvil::Fence>() );

        /** Submits specified command buffer to the queue. Can optionally wait until the execution finishes.
         *
         *  For argument discussion, please see submit_command_buffers() documentation
         **/
        void submit_command_buffer(std::shared_ptr<Anvil::CommandBufferBase> cmd_buffer_ptr,
                                   bool                                      should_block,
                                   std::shared_ptr<Anvil::Fence>             opt_fence_ptr = std::shared_ptr<Anvil::Fence>() )
        {
            submit_command_buffers(1,       /* n_command_buffers */
                                  &cmd_buffer_ptr,
                                   0,       /* n_semaphores_to_signal    */
                                   nullptr, /* semaphore_to_signal_ptrs  */
                                   0,       /* n_semaphoires_to_wait_on  */
                                   nullptr, /* semaphore_to_wait_on_ptrs */
                                   nullptr,
                                   should_block,
                                   opt_fence_ptr);
        }

        /** Submits specified command buffer to the queue. After the command buffer finishes executing
         *  GPU-side, user-specified semaphores will be signalled.
         *
         *  Can optionally wait until the execution finishes.
         *
         *  For argument discussion, please see submit_command_buffers() documentation
         **/
        void submit_command_buffer_with_signal_semaphores(std::shared_ptr<Anvil::CommandBufferBase> opt_cmd_buffer_ptr,
                                                          uint32_t                                  n_semaphores_to_signal,
                                                          std::shared_ptr<Anvil::Semaphore> const*  semaphore_to_signal_ptr_ptrs,
                                                          bool                                      should_block,
                                                          std::shared_ptr<Anvil::Fence>             opt_fence_ptr                = std::shared_ptr<Anvil::Fence>() )
        {
            submit_command_buffers((opt_cmd_buffer_ptr != nullptr) ? 1u : 0u,
                                  &opt_cmd_buffer_ptr,
                                   n_semaphores_to_signal,
                                   semaphore_to_signal_ptr_ptrs,
                                   0,                       /* n_semaphores_to_wait_on             */
                                   nullptr,                 /* semaphore_to_wait_on_ptr_ptrs       */
                                   nullptr,                 /* opt_dst_stage_masks_to_wait_on_ptrs */
                                   should_block,
                                   opt_fence_ptr);
        }

        /** Waits on the user-specified semaphores and submits specified command buffer to the queue.
         *  After the command buffer finishes executing GPU-side, user-specified semaphores will be
         *  signalled.
         *
         *  Can optionally wait until the execution finishes.
         *
         *  For argument discussion, please see submit_command_buffers() documentation
         **/
        void submit_command_buffer_with_signal_wait_semaphores(std::shared_ptr<Anvil::CommandBufferBase> opt_cmd_buffer_ptr,
                                                               uint32_t                                  n_semaphores_to_signal,
                                                               std::shared_ptr<Anvil::Semaphore> const*  semaphore_to_signal_ptr_ptrs,
                                                               uint32_t                                  n_semaphores_to_wait_on,
                                                               std::shared_ptr<Anvil::Semaphore> const*  semaphore_to_wait_on_ptr_ptrs,
                                                               const VkPipelineStageFlags*               dst_stage_masks_to_wait_on_ptrs,
                                                               bool                                      should_block,
                                                               std::shared_ptr<Anvil::Fence>             opt_fence_ptr                = std::shared_ptr<Anvil::Fence>() )
        {
            submit_command_buffers((opt_cmd_buffer_ptr != nullptr) ? 1u : 0u,
                                  &opt_cmd_buffer_ptr,
                                   n_semaphores_to_signal,
                                   semaphore_to_signal_ptr_ptrs,
                                   n_semaphores_to_wait_on,
                                   semaphore_to_wait_on_ptr_ptrs,
                                   dst_stage_masks_to_wait_on_ptrs,
                                   should_block,
                                   opt_fence_ptr);
        }

        /** Waits on the user-specified semaphores and submits specified command buffer to the queue.
         *
         *  Can optionally wait until the execution finishes.
         *
         *  For argument discussion, please see submit_command_buffers() documentation
         **/
        void submit_command_buffer_with_wait_semaphores(std::shared_ptr<Anvil::CommandBufferBase> cmd_buffer_ptr,
                                                        uint32_t                                  n_semaphores_to_wait_on,
                                                        std::shared_ptr<Anvil::Semaphore> const*  semaphore_to_wait_on_ptr_ptrs,
                                                        const VkPipelineStageFlags*               dst_stage_masks_to_wait_on_ptrs,
                                                        bool                                      should_block,
                                                        std::shared_ptr<Anvil::Fence>             opt_fence_ptr                = std::shared_ptr<Anvil::Fence>() )
        {
            submit_command_buffers((cmd_buffer_ptr != nullptr) ? 1u : 0u,
                                  &cmd_buffer_ptr,
                                   0u,                     /* n_semaphores_to_signal       */
                                   nullptr,                /* semaphore_to_signal_ptr_ptrs */
                                   n_semaphores_to_wait_on,
                                   semaphore_to_wait_on_ptr_ptrs,
                                   dst_stage_masks_to_wait_on_ptrs,
                                   should_block,
                                   opt_fence_ptr);
        }

        /** Tells whether the queue supports sparse bindings */
        bool supports_sparse_bindings() const
        {
            return m_supports_sparse_bindings;
        }

    private:
        /* Private functions */

        /* Constructor. Please see create() for specification */
        Queue(std::weak_ptr<Anvil::BaseDevice>        device_ptr,
              uint32_t                                queue_family_index,
              uint32_t                                queue_index);

        Queue          (const Queue&);
        Queue operator=(const Queue&);

        /* Private variables */
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        VkQueue                          m_queue;
        uint32_t                         m_queue_family_index;
        uint32_t                         m_queue_index;
        bool                             m_supports_sparse_bindings;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_QUEUE_H */