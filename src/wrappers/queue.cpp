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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/command_buffer.h"
#include "wrappers/device.h"
#include "wrappers/fence.h"
#include "wrappers/queue.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/semaphore.h"
#include "wrappers/swapchain.h"

/** Please see header for specification */
Anvil::Queue::Queue(Anvil::Device*        device_ptr,
                    uint32_t              queue_family_index,
                    uint32_t              queue_index,
                    PFN_vkQueuePresentKHR pfn_queue_present_proc)

    :m_device_ptr           (device_ptr),
     m_queue                (VK_NULL_HANDLE),
     m_queue_family_index   (queue_family_index),
     m_queue_index          (queue_index),
     m_supports_presentation(false),
     m_swapchain_ptr        (nullptr),
     m_vkQueuePresentKHR    (pfn_queue_present_proc)
{
    /* Retrieve the Vulkan handle */
    vkGetDeviceQueue(device_ptr->get_device_vk(),
                     queue_family_index,
                     queue_index,
                    &m_queue);

    anvil_assert(m_queue != VK_NULL_HANDLE);

    /* OK, register the wrapper instance and leave */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_QUEUE,
                                                  this);
}

/** Please see header for specification */
Anvil::Queue::~Queue()
{
    /* Queues are indestructible. Nothing to do here. */

    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_QUEUE,
                                                    this);
}

/** Please see header for specification */
void Anvil::Queue::present(uint32_t          swapchain_image_index,
                           uint32_t          n_wait_semaphores,
                           Anvil::Semaphore* wait_semaphore_ptrs)
{
    VkPresentInfoKHR image_presentation_info;
    VkResult         presentation_result;
    VkResult         result;
    VkSemaphore      wait_semaphores_vk[8];

    anvil_assert(n_wait_semaphores < sizeof(wait_semaphores_vk) / sizeof(wait_semaphores_vk[0]) );
    anvil_assert(m_supports_presentation);

    for (uint32_t n_wait_semaphore = 0;
                  n_wait_semaphore < n_wait_semaphores;
                ++n_wait_semaphore)
    {
        wait_semaphores_vk[n_wait_semaphore] = wait_semaphore_ptrs->get_semaphore();
    }

    image_presentation_info.pImageIndices      = &swapchain_image_index;
    image_presentation_info.pNext              = nullptr;
    image_presentation_info.pResults           = &presentation_result;
    image_presentation_info.pSwapchains        = m_swapchain_ptr->get_swapchain_ptr();
    image_presentation_info.pWaitSemaphores    = wait_semaphores_vk;
    image_presentation_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    image_presentation_info.swapchainCount     = 1;
    image_presentation_info.waitSemaphoreCount = n_wait_semaphores;

    result = m_vkQueuePresentKHR(m_queue,
                                &image_presentation_info);

    anvil_assert_vk_call_succeeded(result);
    anvil_assert                  (presentation_result == result);
}

/** Please see header for specification */
void Anvil::Queue::set_swapchain(Anvil::Swapchain* swapchain_ptr)
{
    anvil_assert(m_swapchain_ptr == nullptr);
    anvil_assert(swapchain_ptr   != nullptr);

    m_swapchain_ptr = swapchain_ptr;

    /* Does this queue support presentation for the specified rendering surface? */
    const Anvil::RenderingSurface* rendering_surface_ptr = swapchain_ptr->get_rendering_surface();
    VkBool32                        result_vk;

    rendering_surface_ptr->supports_presentation_for_queue(this,
                                                          &result_vk);

    m_supports_presentation = (result_vk == VK_TRUE);
}

/** Please see header for specification */
void Anvil::Queue::submit_command_buffers(uint32_t                               n_command_buffers,
                                          const Anvil::CommandBufferBase* const* opt_cmd_buffer_ptrs,
                                          uint32_t                               n_semaphores_to_signal,
                                          const Anvil::Semaphore* const*         opt_semaphore_to_signal_ptr_ptrs,
                                          uint32_t                               n_semaphores_to_wait_on,
                                          const Anvil::Semaphore* const*         opt_semaphore_to_wait_on_ptr_ptrs,
                                          const VkPipelineStageFlags*            opt_dst_stage_masks_to_wait_on_ptrs,
                                          bool                                   should_block,
                                          Anvil::Fence*                          opt_fence_ptr)
{
    VkResult     result;
    VkSubmitInfo submit_info;
    bool         uses_custom_fence = false;

    VkCommandBuffer cmd_buffers_vk      [64];
    VkSemaphore     signal_semaphores_vk[64];
    VkSemaphore     wait_semaphores_vk  [64];

    anvil_assert(n_command_buffers       < sizeof(cmd_buffers_vk)       / sizeof(cmd_buffers_vk      [0]) );
    anvil_assert(n_semaphores_to_signal  < sizeof(signal_semaphores_vk) / sizeof(signal_semaphores_vk[0]) );
    anvil_assert(n_semaphores_to_wait_on < sizeof(wait_semaphores_vk)   / sizeof(wait_semaphores_vk  [0]) );

    if (opt_fence_ptr == nullptr && should_block)
    {
        opt_fence_ptr     = new Anvil::Fence(m_device_ptr,
                                              false /* create_signalled */);
        uses_custom_fence = true;
    }

    for (uint32_t n_command_buffer = 0;
                  n_command_buffer < n_command_buffers;
                ++n_command_buffer)
    {
        cmd_buffers_vk[n_command_buffer] = opt_cmd_buffer_ptrs[n_command_buffer]->get_command_buffer();
    }

    for (uint32_t n_signal_semaphore = 0;
                  n_signal_semaphore < n_semaphores_to_signal;
                ++n_signal_semaphore)
    {
        signal_semaphores_vk[n_signal_semaphore] = opt_semaphore_to_signal_ptr_ptrs[n_signal_semaphore]->get_semaphore();
    }

    for (uint32_t n_wait_semaphore = 0;
                  n_wait_semaphore < n_semaphores_to_wait_on;
                ++n_wait_semaphore)
    {
        wait_semaphores_vk[n_wait_semaphore] = opt_semaphore_to_wait_on_ptr_ptrs[n_wait_semaphore]->get_semaphore();
    }

    submit_info.commandBufferCount   = n_command_buffers;
    submit_info.pCommandBuffers      = cmd_buffers_vk;
    submit_info.pNext                = nullptr;
    submit_info.pSignalSemaphores    = signal_semaphores_vk;
    submit_info.pWaitDstStageMask    = opt_dst_stage_masks_to_wait_on_ptrs;
    submit_info.pWaitSemaphores      = wait_semaphores_vk;
    submit_info.signalSemaphoreCount = n_semaphores_to_signal;
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = n_semaphores_to_wait_on;

    result = vkQueueSubmit(m_queue,
                           1, /* submitCount */
                          &submit_info,
                          (opt_fence_ptr != nullptr) ? opt_fence_ptr->get_fence() 
                                                     : VK_NULL_HANDLE);
    anvil_assert_vk_call_succeeded(result);

    if (should_block)
    {
        /* Wait till initialization finishes GPU-side */
        result = vkWaitForFences(m_device_ptr->get_device_vk(),
                                 1, /* fenceCount */
                                 opt_fence_ptr->get_fence_ptr(),
                                 VK_TRUE,     /* waitAll */
                                 UINT64_MAX); /* timeout */
        anvil_assert_vk_call_succeeded(result);
    }

    if (uses_custom_fence)
    {
        opt_fence_ptr->release();
    }
}