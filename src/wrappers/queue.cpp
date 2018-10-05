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

#include "misc/debug.h"
#include "misc/fence_create_info.h"
#include "misc/object_tracker.h"
#include "misc/struct_chainer.h"
#include "misc/swapchain_create_info.h"
#include "misc/window.h"
#include "wrappers/buffer.h"
#include "wrappers/command_buffer.h"
#include "wrappers/device.h"
#include "wrappers/fence.h"
#include "wrappers/instance.h"
#include "wrappers/queue.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/semaphore.h"
#include "wrappers/swapchain.h"

#define MAX_SWAPCHAINS (32)


/** Please see header for specification */
Anvil::Queue::Queue(const Anvil::BaseDevice* in_device_ptr,
                    uint32_t                 in_queue_family_index,
                    uint32_t                 in_queue_index,
                    bool                     in_mt_safe)

    :CallbacksSupportProvider  (QUEUE_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT),
     MTSafetySupportProvider   (in_mt_safe),
     m_device_ptr              (in_device_ptr),
     m_queue                   (VK_NULL_HANDLE),
     m_queue_family_index      (in_queue_family_index),
     m_queue_index             (in_queue_index)
{
    /* Retrieve the Vulkan handle */
    Anvil::Vulkan::vkGetDeviceQueue(m_device_ptr->get_device_vk(),
                                    in_queue_family_index,
                                    in_queue_index,
                                   &m_queue);

    anvil_assert(m_queue != VK_NULL_HANDLE);

    /* Determine whether the queue supports sparse bindings */
    m_supports_sparse_bindings = (m_device_ptr->get_queue_family_info(in_queue_family_index)->flags & Anvil::QueueFlagBits::SPARSE_BINDING_BIT) != 0;

    /* Cache a fence that may be optionally used for submissions */
    {
        auto create_info_ptr = Anvil::FenceCreateInfo::create(m_device_ptr,
                                                              false); /* create_signalled */

        create_info_ptr->set_mt_safety(Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe()) );

        m_submit_fence_ptr = Anvil::Fence::create(std::move(create_info_ptr) );
    }

    /* OK, register the wrapper instance and leave */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_QUEUE,
                                                  this);
}

/** Please see header for specification */
Anvil::Queue::~Queue()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_QUEUE,
                                                    this);
}

/** Please see header for specification */
bool Anvil::Queue::bind_sparse_memory(Anvil::SparseMemoryBindingUpdateInfo& in_update)
{
    const VkBindSparseInfo* bind_info_items   = nullptr;
    Anvil::Fence*           fence_ptr         = nullptr;
    const bool              mt_safe           = is_mt_safe();
    uint32_t                n_bind_info_items = 0;
    VkResult                result            = VK_ERROR_INITIALIZATION_FAILED;

    in_update.get_bind_sparse_call_args(&n_bind_info_items,
                                        &bind_info_items,
                                        &fence_ptr);

    /* If any of the bindings we are about to request requires non-zero memory or resource device indices,
     * make sure we're using a mGPU device */
    if (in_update.is_device_group_support_required() )
    {
        const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr));

        if (mgpu_device_ptr == nullptr)
        {
            anvil_assert(mgpu_device_ptr != nullptr);

            goto end;
        }

        if (!mgpu_device_ptr->is_extension_enabled(VK_KHR_DEVICE_GROUP_EXTENSION_NAME) )
        {
            anvil_assert(mgpu_device_ptr->is_extension_enabled(VK_KHR_DEVICE_GROUP_EXTENSION_NAME));

            goto end;
        }
    }

    if (mt_safe)
    {
        bind_sparse_memory_lock_unlock(in_update,
                                       true); /* in_should_lock */
    }
    {
        result = Anvil::Vulkan::vkQueueBindSparse(m_queue,
                                                  n_bind_info_items,
                                                  bind_info_items,
                                                  (fence_ptr != nullptr) ? fence_ptr->get_fence() : VK_NULL_HANDLE);
    }
    if (mt_safe)
    {
        bind_sparse_memory_lock_unlock(in_update,
                                       false); /* in_should_lock */
    }

    anvil_assert(result == VK_SUCCESS);

    for (uint32_t n_bind_info = 0;
                  n_bind_info < n_bind_info_items;
                ++n_bind_info)
    {
        uint32_t n_buffer_memory_updates       = 0;
        uint32_t n_image_memory_updates        = 0;
        uint32_t n_image_opaque_memory_updates = 0;

        in_update.get_bind_info_properties(n_bind_info,
                                          &n_buffer_memory_updates,
                                          &n_image_memory_updates,
                                          &n_image_opaque_memory_updates,
                                           nullptr,  /* out_opt_n_signal_semaphores_ptr   */
                                           nullptr,  /* out_opt_signal_semaphores_ptr_ptr */
                                           nullptr,  /* out_opt_n_wait_semaphores_ptr     */
                                           nullptr); /* out_opt_wait_semaphores_ptr_ptr   */

        for (uint32_t n_buffer_memory_update = 0;
                      n_buffer_memory_update < n_buffer_memory_updates;
                    ++n_buffer_memory_update)
        {
            VkDeviceSize        alloc_size                   = UINT64_MAX;
            VkDeviceSize        buffer_memory_start_offset   = UINT64_MAX;
            Anvil::Buffer*      buffer_ptr                   = nullptr;
            bool                memory_block_owned_by_buffer = false;
            Anvil::MemoryBlock* memory_block_ptr             = nullptr;
            VkDeviceSize        memory_block_start_offset;

            in_update.get_buffer_memory_update_properties(n_bind_info,
                                                          n_buffer_memory_update,
                                                         &buffer_ptr,
                                                         &buffer_memory_start_offset,
                                                         &memory_block_ptr,
                                                         &memory_block_start_offset,
                                                         &memory_block_owned_by_buffer,
                                                          &alloc_size);

            buffer_ptr->set_memory_sparse(memory_block_ptr,
                                          memory_block_owned_by_buffer,
                                          memory_block_start_offset,
                                          buffer_memory_start_offset,
                                          alloc_size);
        }

        for (uint32_t n_image_memory_update = 0;
                      n_image_memory_update < n_image_memory_updates;
                    ++n_image_memory_update)
        {
            Anvil::Image*                image_ptr                   = nullptr;
            VkExtent3D                   extent;
            Anvil::SparseMemoryBindFlags flags;
            bool                         memory_block_owned_by_image = false;
            Anvil::MemoryBlock*          memory_block_ptr            = nullptr;
            VkDeviceSize                 memory_block_start_offset;
            VkOffset3D                   offset;
            Anvil::ImageSubresource      subresource;

            in_update.get_image_memory_update_properties(n_bind_info,
                                                         n_image_memory_update,
                                                        &image_ptr,
                                                        &subresource,
                                                        &offset,
                                                        &extent,
                                                        &flags,
                                                        &memory_block_ptr,
                                                        &memory_block_start_offset,
                                                        &memory_block_owned_by_image);

            image_ptr->on_memory_backing_update(subresource,
                                                offset,
                                                extent,
                                                memory_block_ptr,
                                                memory_block_start_offset,
                                                memory_block_owned_by_image);
        }

        for (uint32_t n_image_opaque_memory_update = 0;
                      n_image_opaque_memory_update < n_image_opaque_memory_updates;
                    ++n_image_opaque_memory_update)
        {
            Anvil::SparseMemoryBindFlags flags;
            Anvil::Image*                image_ptr                   = nullptr;
            bool                         memory_block_owned_by_image = false;
            Anvil::MemoryBlock*          memory_block_ptr            = nullptr;
            VkDeviceSize                 memory_block_start_offset;
            VkDeviceSize                 resource_offset;
            VkDeviceSize                 size;

            in_update.get_image_opaque_memory_update_properties(n_bind_info,
                                                                n_image_opaque_memory_update,
                                                               &image_ptr,
                                                               &resource_offset,
                                                               &size,
                                                               &flags,
                                                               &memory_block_ptr,
                                                               &memory_block_start_offset,
                                                               &memory_block_owned_by_image);

            image_ptr->on_memory_backing_opaque_update(resource_offset,
                                                       size,
                                                       memory_block_ptr,
                                                       memory_block_start_offset,
                                                       memory_block_owned_by_image);
        }
    }

end:
    return (result == VK_SUCCESS);
}

void Anvil::Queue::bind_sparse_memory_lock_unlock(Anvil::SparseMemoryBindingUpdateInfo& in_update,
                                                  bool                                  in_should_lock)
{
    const VkBindSparseInfo* bind_info_items   = nullptr;
    Anvil::Fence*           fence_ptr         = nullptr;
    uint32_t                n_bind_info_items = 0;

    in_update.get_bind_sparse_call_args(&n_bind_info_items,
                                        &bind_info_items,
                                        &fence_ptr);

    if (in_should_lock)
    {
        lock();
    }
    else
    {
        unlock();
    }

    if (fence_ptr != nullptr)
    {
        if (in_should_lock)
        {
            fence_ptr->lock();
        }
        else
        {
            fence_ptr->unlock();
        }
    }

    for (uint32_t n_bind_info_item = 0;
                  n_bind_info_item < n_bind_info_items;
                ++n_bind_info_item)
    {
        uint32_t           n_buffer_memory_updates       = 0;
        uint32_t           n_image_memory_updates        = 0;
        uint32_t           n_image_opaque_memory_updates = 0;
        uint32_t           n_signal_sems                 = 0;
        uint32_t           n_wait_sems                   = 0;
        Anvil::Semaphore** signal_sem_ptrs               = nullptr;
        Anvil::Semaphore** wait_sem_ptrs                 = nullptr;

        in_update.get_bind_info_properties(n_bind_info_item,
                                          &n_buffer_memory_updates,
                                          &n_image_memory_updates,
                                          &n_image_opaque_memory_updates,
                                          &n_signal_sems,
                                          &signal_sem_ptrs,
                                          &n_wait_sems,
                                          &wait_sem_ptrs);

        for (uint32_t n_signal_sem = 0;
                      n_signal_sem < n_signal_sems;
                    ++n_signal_sem)
        {
            if (in_should_lock)
            {
                signal_sem_ptrs[n_signal_sem]->lock();
            }
            else
            {
                signal_sem_ptrs[n_signal_sem]->unlock();
            }
        }

        for (uint32_t n_wait_sem = 0;
                      n_wait_sem < n_wait_sems;
                    ++n_wait_sem)
        {
            if (in_should_lock)
            {
                wait_sem_ptrs[n_wait_sem]->lock();
            }
            else
            {
                wait_sem_ptrs[n_wait_sem]->unlock();
            }
        }

        for (uint32_t n_buffer_memory_update = 0;
                      n_buffer_memory_update < n_buffer_memory_updates;
                    ++n_buffer_memory_update)
        {
            Anvil::Buffer* buffer_ptr = nullptr;

            in_update.get_buffer_memory_update_properties(n_bind_info_item,
                                                          n_buffer_memory_update,
                                                         &buffer_ptr,
                                                          nullptr,  /* out_opt_buffer_memory_start_offset_ptr   */
                                                          nullptr,  /* out_opt_memory_block_ptr                 */
                                                          nullptr,  /* out_opt_memory_block_start_offset_ptr    */
                                                          nullptr,  /* out_opt_memory_block_owned_by_buffer_ptr */
                                                          nullptr); /* out_opt_size_ptr                         */

            if (in_should_lock)
            {
                buffer_ptr->lock();
            }
            else
            {
                buffer_ptr->unlock();
            }
        }

        for (uint32_t n_image_memory_update = 0;
                      n_image_memory_update < n_image_memory_updates;
                    ++n_image_memory_update)
        {
            Anvil::Image* image_ptr = nullptr;

            in_update.get_image_memory_update_properties(n_bind_info_item,
                                                         n_image_memory_update,
                                                        &image_ptr,
                                                         nullptr,  /* out_opt_subresource_ptr                 */
                                                         nullptr,  /* out_opt_offset_ptr                      */
                                                         nullptr,  /* out_opt_extent_ptr                      */
                                                         nullptr,  /* out_opt_flags_ptr                       */
                                                         nullptr,  /* out_opt_memory_block_ptr_ptr            */
                                                         nullptr,  /* out_opt_memory_block_start_offset_ptr   */
                                                         nullptr); /* out_opt_memory_block_owned_by_image_ptr */

            if (in_should_lock)
            {
                image_ptr->lock();
            }
            else
            {
                image_ptr->unlock();
            }
        }

        for (uint32_t n_opaque_image_memory_update = 0;
                      n_opaque_image_memory_update < n_image_opaque_memory_updates;
                    ++n_opaque_image_memory_update)
        {
            Anvil::Image* image_ptr = nullptr;

            in_update.get_image_opaque_memory_update_properties(n_bind_info_item,
                                                                n_opaque_image_memory_update,
                                                               &image_ptr,
                                                                nullptr,  /* out_opt_resource_offset_ptr             */
                                                                nullptr,  /* out_opt_size_ptr                        */
                                                                nullptr,  /* out_opt_flags_ptr                       */
                                                                nullptr,  /* out_opt_memory_block_ptr_ptr            */
                                                                nullptr,  /* out_opt_memory_block_start_offset_ptr   */
                                                                nullptr); /* out_opt_memory_block_owned_by_image_ptr */

            if (in_should_lock)
            {
                image_ptr->lock();
            }
            else
            {
                image_ptr->unlock();
            }
        }
    }
}

/** Please see header for specification */
std::unique_ptr<Anvil::Queue> Anvil::Queue::create(const Anvil::BaseDevice* in_device_ptr,
                                                   uint32_t                 in_queue_family_index,
                                                   uint32_t                 in_queue_index,
                                                   bool                     in_mt_safe)
{
    std::unique_ptr<Queue> result_ptr;

    result_ptr.reset(
        new Anvil::Queue(in_device_ptr,
                         in_queue_family_index,
                         in_queue_index,
                         in_mt_safe)
    );

    return result_ptr;
}

/** Please see header for specification */
VkResult Anvil::Queue::present(Anvil::Swapchain*        in_swapchain_ptr,
                               uint32_t                 in_swapchain_image_index,
                               uint32_t                 in_n_wait_semaphores,
                               Anvil::Semaphore* const* in_wait_semaphore_ptrs)
{
    static const uint32_t device_mask = 0x1;

    return present_internal(Anvil::DeviceGroupPresentModeFlagBits::LOCAL_BIT_KHR,
                            1, /* n_swapchain_image_indices */
                           &in_swapchain_ptr,
                           &in_swapchain_image_index,
                           &device_mask,
                            in_n_wait_semaphores,
                            in_wait_semaphore_ptrs);
}

/** Please see header for specification */
VkResult Anvil::Queue::present_in_local_presentation_mode(uint32_t                         in_n_local_mode_presentation_items,
                                                          const LocalModePresentationItem* in_local_mode_presentation_items,
                                                          uint32_t                         in_n_wait_semaphores,
                                                          Anvil::Semaphore* const*         in_wait_semaphore_ptrs)
{
    const Anvil::DeviceType device_type  (m_device_ptr->get_type() );
    uint32_t                n_swapchains;
    VkResult                result;

    uint32_t                device_masks           [MAX_SWAPCHAINS];
    uint32_t                swapchain_image_indices[MAX_SWAPCHAINS];
    Anvil::Swapchain*       swapchains             [MAX_SWAPCHAINS];

    if (device_type == Anvil::DeviceType::SINGLE_GPU)
    {
        const Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

        ANVIL_REDUNDANT_VARIABLE(sgpu_device_ptr);

        anvil_assert(in_n_local_mode_presentation_items                                             == 1);
        anvil_assert(in_local_mode_presentation_items[0].physical_device_ptr->get_physical_device() == sgpu_device_ptr->get_physical_device()->get_physical_device() );

        device_masks[0]            = 0x1;
        n_swapchains               = 1;
        swapchain_image_indices[0] = in_local_mode_presentation_items[0].swapchain_image_index;
        swapchains[0]              = in_local_mode_presentation_items[0].swapchain_ptr;
    }
    else
    {
        anvil_assert(device_type                        == Anvil::DeviceType::MULTI_GPU);
        anvil_assert(in_n_local_mode_presentation_items <  MAX_SWAPCHAINS);

        n_swapchains = in_n_local_mode_presentation_items;

        for (uint32_t n_item = 0;
                      n_item < in_n_local_mode_presentation_items;
                    ++n_item)
        {
            const auto current_item_ptr = in_local_mode_presentation_items + n_item;

            device_masks           [n_item] = (1u << current_item_ptr->physical_device_ptr->get_device_group_device_index());
            swapchains             [n_item] = current_item_ptr->swapchain_ptr;
            swapchain_image_indices[n_item] = current_item_ptr->swapchain_image_index;
        }
    }

    result = present_internal(Anvil::DeviceGroupPresentModeFlagBits::LOCAL_BIT_KHR,
                              n_swapchains,
                              swapchains,
                              swapchain_image_indices,
                              device_masks,
                              in_n_wait_semaphores,
                              in_wait_semaphore_ptrs);

    return result;
}

/** Please see header for specification */
VkResult Anvil::Queue::present_in_local_multi_device_presentation_mode(uint32_t                                           in_n_local_multi_device_mode_presentation_items,
                                                                       const Anvil::LocalMultiDeviceModePresentationItem* in_local_multi_device_mode_presentation_items,
                                                                       uint32_t                                           in_n_wait_semaphores,
                                                                       Anvil::Semaphore* const*                           in_wait_semaphore_ptrs)
{
    uint32_t          n_swapchains;
    VkResult          result;

    uint32_t          device_masks           [MAX_SWAPCHAINS];
    uint32_t          swapchain_image_indices[MAX_SWAPCHAINS];
    Anvil::Swapchain* swapchains             [MAX_SWAPCHAINS];

    anvil_assert(in_n_local_multi_device_mode_presentation_items <  MAX_SWAPCHAINS);

    n_swapchains = in_n_local_multi_device_mode_presentation_items;

    for (uint32_t n_item = 0;
                  n_item < in_n_local_multi_device_mode_presentation_items;
                ++n_item)
    {
        const auto current_item_ptr(in_local_multi_device_mode_presentation_items + n_item);
        uint32_t   device_mask     (0);

        for (uint32_t n_physical_device = 0;
                      n_physical_device < current_item_ptr->n_physical_devices;
                    ++n_physical_device)
        {
            const uint32_t device_index = current_item_ptr->physical_devices_ptr[n_physical_device]->get_device_group_device_index();

            device_mask |= 1 << device_index;
        }

        device_masks           [n_item] = device_mask;
        swapchains             [n_item] = current_item_ptr->swapchain_ptr;
        swapchain_image_indices[n_item] = current_item_ptr->swapchain_image_index;
    }

    result = present_internal(Anvil::DeviceGroupPresentModeFlagBits::LOCAL_MULTI_DEVICE_BIT_KHR,
                              n_swapchains,
                              swapchains,
                              swapchain_image_indices,
                              device_masks,
                              in_n_wait_semaphores,
                              in_wait_semaphore_ptrs);

    return result;
}

/** Please see header for specification */
VkResult Anvil::Queue::present_in_remote_presentation_mode(uint32_t                                 in_n_remote_mode_presentation_items,
                                                           const Anvil::RemoteModePresentationItem* in_remote_mode_presentation_items,
                                                           uint32_t                                 in_n_wait_semaphores,
                                                           Anvil::Semaphore* const*                 in_wait_semaphore_ptrs)
{
    const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr));
    uint32_t                 n_swapchains;
    VkResult                 result;

    uint32_t          device_masks           [MAX_SWAPCHAINS];
    uint32_t          swapchain_image_indices[MAX_SWAPCHAINS];
    Anvil::Swapchain* swapchains             [MAX_SWAPCHAINS];

    ANVIL_REDUNDANT_VARIABLE_CONST(mgpu_device_ptr);
    anvil_assert                  (mgpu_device_ptr                     != nullptr);
    anvil_assert                  (in_n_remote_mode_presentation_items <  MAX_SWAPCHAINS);

    n_swapchains = in_n_remote_mode_presentation_items;

    for (uint32_t n_item = 0;
                  n_item < in_n_remote_mode_presentation_items;
                ++n_item)
    {
        const auto     current_item_ptr = in_remote_mode_presentation_items + n_item;
        const uint32_t device_index     = current_item_ptr->physical_device_ptr->get_device_group_device_index();

        #if defined(_DEBUG)
        {
            const std::vector<const Anvil::PhysicalDevice*>* present_compatible_physical_devices_ptr;

            mgpu_device_ptr->get_present_compatible_physical_devices(device_index,
                                                                    &present_compatible_physical_devices_ptr);

            anvil_assert(present_compatible_physical_devices_ptr->size() > 0);
        }
        #endif

        device_masks           [n_item] = (1u << device_index);
        swapchains             [n_item] = current_item_ptr->swapchain_ptr;
        swapchain_image_indices[n_item] = current_item_ptr->swapchain_image_index;
    }

    result = present_internal(Anvil::DeviceGroupPresentModeFlagBits::REMOTE_BIT_KHR,
                              n_swapchains,
                              swapchains,
                              swapchain_image_indices,
                              device_masks,
                              in_n_wait_semaphores,
                              in_wait_semaphore_ptrs);

    return result;
}

/** Please see header for specification */
VkResult Anvil::Queue::present_in_sum_presentation_mode(uint32_t                              in_n_sum_mode_presentation_items,
                                                        const Anvil::SumModePresentationItem* in_sum_mode_presentation_items,
                                                        uint32_t                              in_n_wait_semaphores,
                                                        Anvil::Semaphore* const*              in_wait_semaphore_ptrs)
{
    const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr));
    uint32_t                 n_swapchains;
    VkResult                 result;

    uint32_t          device_masks           [MAX_SWAPCHAINS];
    uint32_t          swapchain_image_indices[MAX_SWAPCHAINS];
    Anvil::Swapchain* swapchains             [MAX_SWAPCHAINS];

    ANVIL_REDUNDANT_VARIABLE_CONST(mgpu_device_ptr);
    anvil_assert                  (mgpu_device_ptr                  != nullptr);
    anvil_assert                  (in_n_sum_mode_presentation_items <  MAX_SWAPCHAINS);

    n_swapchains = in_n_sum_mode_presentation_items;

    for (uint32_t n_item = 0;
                  n_item < in_n_sum_mode_presentation_items;
                ++n_item)
    {
        const auto current_item_ptr (in_sum_mode_presentation_items + n_item);
        uint32_t   device_mask      (0);

        #ifdef _DEBUG
            const uint32_t n_physical_devices = mgpu_device_ptr->get_n_physical_devices();
            bool           request_supported  = false;

            anvil_assert(mgpu_device_ptr->get_supported_present_modes_for_surface(current_item_ptr->swapchain_ptr->get_create_info_ptr()->get_rendering_surface() ) != Anvil::DeviceGroupPresentModeFlagBits::NONE);

            /* Make sure at least one physical device supports SUM presentation mode for all the specified physical devices */
            for (uint32_t n_physical_device = 0;
                          n_physical_device < n_physical_devices && !request_supported;
                        ++n_physical_device)
            {
                const std::vector<const Anvil::PhysicalDevice* >* compatible_physical_devices_ptr;

                if (!mgpu_device_ptr->get_present_compatible_physical_devices(n_physical_device,
                                                                             &compatible_physical_devices_ptr) )
                {
                    anvil_assert_fail();

                    continue;
                }

                request_supported = true;

                for (uint32_t n_checked_physical_device = 0;
                              n_checked_physical_device < current_item_ptr->n_physical_devices && request_supported;
                            ++n_checked_physical_device)
                {
                    const auto& physical_device_ptr = current_item_ptr->physical_devices_ptr[n_checked_physical_device];
                    bool        match_found         = false;

                    for (uint32_t n_compatible_physical_device = 0;
                                  n_compatible_physical_device < compatible_physical_devices_ptr->size() && !match_found;
                                ++n_compatible_physical_device)
                    {
                        auto compatible_physical_device_ptr = compatible_physical_devices_ptr->at(n_compatible_physical_device);

                        if (compatible_physical_device_ptr == physical_device_ptr)
                        {
                            match_found = true;
                        }
                    }

                    request_supported = match_found;
                }
            }

            anvil_assert(request_supported);
        #endif

        for (uint32_t n_physical_device = 0;
                      n_physical_device < current_item_ptr->n_physical_devices;
                    ++n_physical_device)
        {
            const uint32_t device_index = current_item_ptr->physical_devices_ptr[n_physical_device]->get_device_group_device_index();

            device_mask |= 1 << device_index;
        }

        device_masks           [n_item] = device_mask;
        swapchains             [n_item] = current_item_ptr->swapchain_ptr;
        swapchain_image_indices[n_item] = current_item_ptr->swapchain_image_index;
    }

    result = present_internal(Anvil::DeviceGroupPresentModeFlagBits::SUM_BIT_KHR,
                              n_swapchains,
                              swapchains,
                              swapchain_image_indices,
                              device_masks,
                              in_n_wait_semaphores,
                              in_wait_semaphore_ptrs);

    return result;
}

/** Please see header for specification */
VkResult Anvil::Queue::present_internal(DeviceGroupPresentModeFlagBits in_presentation_mode,
                                        uint32_t                       in_n_swapchains,
                                        Anvil::Swapchain* const*       in_swapchains,
                                        const uint32_t*                in_swapchain_image_indices,
                                        const uint32_t*                in_device_masks,
                                        uint32_t                       in_n_wait_semaphores,
                                        Anvil::Semaphore* const*       in_wait_semaphore_ptrs)
{
    const Anvil::DeviceType                 device_type            (m_device_ptr->get_type() );
    VkResult                                presentation_results   [MAX_SWAPCHAINS];
    VkResult                                result;
    Anvil::StructChainer<VkPresentInfoKHR>  struct_chainer;
    const ExtensionKHRSwapchainEntrypoints* swapchain_entrypoints_ptr(nullptr);
    VkSwapchainKHR                          swapchains_vk          [MAX_SWAPCHAINS];
    std::vector<VkSemaphore>                wait_semaphores_vk     (in_n_wait_semaphores);

    /* Sanity checks */
    anvil_assert(in_n_swapchains      <  MAX_SWAPCHAINS);
    anvil_assert(in_swapchains        != nullptr);

    if (device_type == Anvil::DeviceType::SINGLE_GPU)
    {
        anvil_assert(*in_device_masks     == 1);
        anvil_assert(in_n_swapchains      == 1);
        anvil_assert(in_presentation_mode == Anvil::DeviceGroupPresentModeFlagBits::LOCAL_BIT_KHR);
    }

    /* If the application is only interested in off-screen rendering, do *not* post the present request,
     * since the fake swapchain image is not presentable. We still have to wait on the user-specified
     * semaphores though. */
    if (in_swapchains[0] != nullptr)
    {
        Anvil::Window* window_ptr = nullptr;

        window_ptr = in_swapchains[0]->get_create_info_ptr()->get_window();

        if (window_ptr != nullptr)
        {
            const WindowPlatform window_platform = window_ptr->get_platform();

            if (window_platform == WINDOW_PLATFORM_DUMMY                    ||
                window_platform == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS)
            {
                static const Anvil::PipelineStageFlags dst_stage_mask(Anvil::PipelineStageFlagBits::TOP_OF_PIPE_BIT);

                m_device_ptr->get_universal_queue(0)->submit(
                    SubmitInfo::create_wait(in_n_wait_semaphores,
                                            in_wait_semaphore_ptrs,
                                           &dst_stage_mask)
                );

                for (uint32_t n_presentation = 0;
                              n_presentation < in_n_swapchains;
                            ++n_presentation)
                {
                    OnPresentRequestIssuedCallbackArgument callback_argument(in_swapchains[n_presentation]);

                    CallbacksSupportProvider::callback(QUEUE_CALLBACK_ID_PRESENT_REQUEST_ISSUED,
                                                      &callback_argument);
                }

                result = VK_SUCCESS;
                goto end;
            }
        }
    }

    /* Convert arrays of Anvil objects to raw Vulkan handle arrays */
    for (uint32_t n_swapchain = 0;
                  n_swapchain < in_n_swapchains;
                ++n_swapchain)
    {
        anvil_assert(in_swapchains[n_swapchain] != nullptr);

        swapchains_vk[n_swapchain] = in_swapchains[n_swapchain]->get_swapchain_vk();
    }

    for (uint32_t n_wait_semaphore = 0;
                  n_wait_semaphore < in_n_wait_semaphores;
                ++n_wait_semaphore)
    {
        wait_semaphores_vk[n_wait_semaphore] = in_wait_semaphore_ptrs[n_wait_semaphore]->get_semaphore();
    }

    {
        VkPresentInfoKHR image_presentation_info;

        image_presentation_info.pImageIndices      = in_swapchain_image_indices;
        image_presentation_info.pNext              = nullptr;
        image_presentation_info.pResults           = presentation_results;
        image_presentation_info.pSwapchains        = swapchains_vk;
        image_presentation_info.pWaitSemaphores    = (in_n_wait_semaphores != 0) ? &wait_semaphores_vk.at(0) : nullptr;
        image_presentation_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        image_presentation_info.swapchainCount     = in_n_swapchains;
        image_presentation_info.waitSemaphoreCount = in_n_wait_semaphores;

        struct_chainer.append_struct(image_presentation_info);
    }

    /* For multi-GPU support, we're likely going to need to attach the VkDeviceGroupPresentInfoKHR struct */
    if (device_type == Anvil::DeviceType::MULTI_GPU)
    {
        VkDeviceGroupPresentInfoKHR device_group_present_info;

        device_group_present_info.mode           = static_cast<VkDeviceGroupPresentModeFlagBitsKHR>(in_presentation_mode);
        device_group_present_info.pDeviceMasks   = in_device_masks;
        device_group_present_info.pNext          = nullptr;
        device_group_present_info.sType          = VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_INFO_KHR;
        device_group_present_info.swapchainCount = in_n_swapchains;

        struct_chainer.append_struct(device_group_present_info);
    }

    swapchain_entrypoints_ptr = &m_device_ptr->get_extension_khr_swapchain_entrypoints();

    present_lock_unlock(in_n_swapchains,
                        in_swapchains,
                        in_n_wait_semaphores,
                        in_wait_semaphore_ptrs,
                        true);
    {
        auto chain_ptr = struct_chainer.create_chain();

        result = swapchain_entrypoints_ptr->vkQueuePresentKHR(m_queue,
                                                              chain_ptr->get_root_struct() );
    }
    present_lock_unlock(in_n_swapchains,
                        in_swapchains,
                        in_n_wait_semaphores,
                        in_wait_semaphore_ptrs,
                        false);

    anvil_assert_vk_call_succeeded(result);

    if (is_vk_call_successful(result) )
    {
        for (uint32_t n_presentation = 0;
                      n_presentation < in_n_swapchains;
                    ++n_presentation)
        {
            anvil_assert(is_vk_call_successful(presentation_results[n_presentation]));

            /* Return the most important error code reported */
            if (result != VK_ERROR_DEVICE_LOST)
            {
                switch (presentation_results[n_presentation])
                {
                    case VK_ERROR_DEVICE_LOST:
                    {
                        result = VK_ERROR_DEVICE_LOST;

                        break;
                    }

                    case VK_ERROR_SURFACE_LOST_KHR:
                    {
                        if (result != VK_ERROR_DEVICE_LOST)
                        {
                            result = VK_ERROR_SURFACE_LOST_KHR;
                        }

                        break;
                    }

                    case VK_ERROR_OUT_OF_DATE_KHR:
                    {
                        if (result != VK_ERROR_DEVICE_LOST      &&
                            result != VK_ERROR_SURFACE_LOST_KHR)
                        {
                            result = VK_ERROR_OUT_OF_DATE_KHR;
                        }

                        break;
                    }

                    case VK_SUBOPTIMAL_KHR:
                    {
                        if (result != VK_ERROR_DEVICE_LOST      &&
                            result != VK_ERROR_SURFACE_LOST_KHR &&
                            result != VK_ERROR_OUT_OF_DATE_KHR)
                        {
                            result = VK_SUBOPTIMAL_KHR;
                        }

                        break;
                    }

                    default:
                    {
                        anvil_assert(presentation_results[n_presentation] == VK_SUCCESS);
                    }
                }
            }

            {
                OnPresentRequestIssuedCallbackArgument callback_argument(in_swapchains[n_presentation]);

                CallbacksSupportProvider::callback(QUEUE_CALLBACK_ID_PRESENT_REQUEST_ISSUED,
                                                  &callback_argument);
            }
        }
    }

end:
    return result;
}

/** Please see header for specification */
void Anvil::Queue::present_lock_unlock(uint32_t                       in_n_swapchains,
                                       const Anvil::Swapchain* const* in_swapchains,
                                       uint32_t                       in_n_wait_semaphores,
                                       Anvil::Semaphore* const*       in_wait_semaphore_ptrs,
                                       bool                           in_should_lock)
{
    if (in_should_lock)
    {
        lock();
    }
    else
    {
        unlock();
    }

    for (uint32_t n_semaphore = 0;
                  n_semaphore < in_n_wait_semaphores;
                ++n_semaphore)
    {
        if (in_should_lock)
        {
            in_wait_semaphore_ptrs[n_semaphore]->lock();
        }
        else
        {
            in_wait_semaphore_ptrs[n_semaphore]->unlock();
        }
    }

    for (uint32_t n_swapchain = 0;
                  n_swapchain < in_n_swapchains;
                ++n_swapchain)
    {
        if (in_should_lock)
        {
            in_swapchains[n_swapchain]->lock();
        }
        else
        {
            in_swapchains[n_swapchain]->unlock();
        }
    }
}

/** Please see header for specification */
bool Anvil::Queue::submit(const Anvil::SubmitInfo& in_submit_info)
{
    Anvil::Fence*                      fence_ptr        (in_submit_info.get_fence() );
    bool                               needs_fence_reset(false);
    VkResult                           result           (VK_ERROR_INITIALIZATION_FAILED);
    Anvil::StructChainer<VkSubmitInfo> struct_chainer;

    std::vector<VkCommandBuffer> cmd_buffers_vk      (in_submit_info.get_n_command_buffers  () );
    std::vector<VkSemaphore>     signal_semaphores_vk(in_submit_info.get_n_signal_semaphores() );
    std::vector<VkSemaphore>     wait_semaphores_vk  (in_submit_info.get_n_wait_semaphores  () );

    std::vector<uint32_t> cmd_buffer_device_masks         = std::vector<uint32_t>(in_submit_info.get_n_command_buffers() );
    std::vector<uint32_t> signal_semaphore_device_indices = std::vector<uint32_t>(in_submit_info.get_n_signal_semaphores() );
    std::vector<uint32_t> wait_semaphore_device_indices   = std::vector<uint32_t>(in_submit_info.get_n_wait_semaphores  () );

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Prepare for the submission */
    switch (in_submit_info.get_type() )
    {
        case SubmissionType::MGPU:
        {
            uint32_t n_cmd_buffers = 0;

            for (uint32_t n_command_buffer_submission = 0;
                          n_command_buffer_submission < in_submit_info.get_n_command_buffers();
                        ++n_command_buffer_submission)
            {
                const auto& current_submission = in_submit_info.get_command_buffers_mgpu()[n_command_buffer_submission];

                if (current_submission.cmd_buffer_ptr != nullptr)
                {
                    cmd_buffers_vk.at         (n_cmd_buffers) = current_submission.cmd_buffer_ptr->get_command_buffer();
                    cmd_buffer_device_masks.at(n_cmd_buffers) = current_submission.device_mask;

                    ++n_cmd_buffers;
                }
            }

            for (uint32_t n_signal_semaphore_submission = 0;
                          n_signal_semaphore_submission < in_submit_info.get_n_signal_semaphores();
                        ++n_signal_semaphore_submission)
            {
                const auto& current_submission = in_submit_info.get_signal_semaphores_mgpu()[n_signal_semaphore_submission];

                signal_semaphore_device_indices.at(n_signal_semaphore_submission) = current_submission.device_index;
                signal_semaphores_vk.at           (n_signal_semaphore_submission) = current_submission.semaphore_ptr->get_semaphore();
            }

            for (uint32_t n_wait_semaphore_submission = 0;
                          n_wait_semaphore_submission < in_submit_info.get_n_wait_semaphores();
                        ++n_wait_semaphore_submission)
            {
                const auto& current_submission = in_submit_info.get_wait_semaphores_mgpu()[n_wait_semaphore_submission];

                wait_semaphore_device_indices.at(n_wait_semaphore_submission) = current_submission.device_index;
                wait_semaphores_vk.at           (n_wait_semaphore_submission) = current_submission.semaphore_ptr->get_semaphore();
            }

            {
                VkSubmitInfo submit_info;

                submit_info.commandBufferCount   = in_submit_info.get_n_command_buffers();
                submit_info.pCommandBuffers      = (submit_info.commandBufferCount           != 0) ? &cmd_buffers_vk.at(0)       : nullptr;
                submit_info.pNext                = nullptr;
                submit_info.pSignalSemaphores    = (in_submit_info.get_n_signal_semaphores() != 0) ? &signal_semaphores_vk.at(0) : nullptr;
                submit_info.pWaitDstStageMask    = in_submit_info.get_destination_stage_wait_masks();
                submit_info.pWaitSemaphores      = (in_submit_info.get_n_wait_semaphores()   != 0) ? &wait_semaphores_vk.at(0)   : nullptr;
                submit_info.signalSemaphoreCount = in_submit_info.get_n_signal_semaphores();
                submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info.waitSemaphoreCount   = in_submit_info.get_n_wait_semaphores();

                struct_chainer.append_struct(submit_info);
            }

            {
                VkDeviceGroupSubmitInfoKHR submit_info_device_group;

                submit_info_device_group.commandBufferCount            = n_cmd_buffers;
                submit_info_device_group.pCommandBufferDeviceMasks     = (n_cmd_buffers != 0) ? &cmd_buffer_device_masks.at(0) : nullptr;
                submit_info_device_group.pNext                         = nullptr;
                submit_info_device_group.pSignalSemaphoreDeviceIndices = (in_submit_info.get_n_signal_semaphores() != 0) ? &signal_semaphore_device_indices.at(0) : nullptr;
                submit_info_device_group.pWaitSemaphoreDeviceIndices   = (in_submit_info.get_n_wait_semaphores  () != 0) ? &wait_semaphore_device_indices.at  (0) : nullptr;
                submit_info_device_group.signalSemaphoreCount          = in_submit_info.get_n_signal_semaphores();
                submit_info_device_group.sType                         = VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO_KHR;
                submit_info_device_group.waitSemaphoreCount            = in_submit_info.get_n_wait_semaphores();

                struct_chainer.append_struct(submit_info_device_group);
            }

            break;
        }

        case SubmissionType::SGPU:
        {
            VkSubmitInfo submit_info;

            for (uint32_t n_command_buffer = 0;
                          n_command_buffer < in_submit_info.get_n_command_buffers();
                        ++n_command_buffer)
            {
                cmd_buffers_vk.at(n_command_buffer) = in_submit_info.get_command_buffers_sgpu()[n_command_buffer]->get_command_buffer();
            }

            for (uint32_t n_signal_semaphore = 0;
                          n_signal_semaphore < in_submit_info.get_n_signal_semaphores();
                        ++n_signal_semaphore)
            {
                auto sem_ptr = in_submit_info.get_signal_semaphores_sgpu()[n_signal_semaphore];

                signal_semaphores_vk.at(n_signal_semaphore) = sem_ptr->get_semaphore();
            }

            for (uint32_t n_wait_semaphore = 0;
                          n_wait_semaphore < in_submit_info.get_n_wait_semaphores();
                        ++n_wait_semaphore)
            {
                wait_semaphores_vk.at(n_wait_semaphore) = in_submit_info.get_wait_semaphores_sgpu()[n_wait_semaphore]->get_semaphore();
            }

            submit_info.commandBufferCount   = in_submit_info.get_n_command_buffers ();
            submit_info.pCommandBuffers      = (in_submit_info.get_n_command_buffers()   != 0) ? &cmd_buffers_vk.at(0)       : nullptr;
            submit_info.pNext                = nullptr;
            submit_info.pSignalSemaphores    = (in_submit_info.get_n_signal_semaphores() != 0) ? &signal_semaphores_vk.at(0) : nullptr;
            submit_info.pWaitDstStageMask    = in_submit_info.get_destination_stage_wait_masks();
            submit_info.pWaitSemaphores      = (in_submit_info.get_n_wait_semaphores()   != 0) ? &wait_semaphores_vk.at(0)   : nullptr;
            submit_info.signalSemaphoreCount = in_submit_info.get_n_signal_semaphores();
            submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount   = in_submit_info.get_n_wait_semaphores();

            struct_chainer.append_struct(submit_info);

            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }

    /* Any additional structs to chain? */
    #if defined(_WIN32)
    {
        const uint64_t* d3d12_fence_signal_semaphore_values_ptr = nullptr;
        const uint64_t* d3d12_fence_wait_semaphore_values_ptr   = nullptr;

        if (in_submit_info.get_d3d12_fence_semaphore_values(&d3d12_fence_signal_semaphore_values_ptr,
                                                            &d3d12_fence_wait_semaphore_values_ptr) )
        {
            VkD3D12FenceSubmitInfoKHR fence_info;

            fence_info.pNext                      = nullptr;
            fence_info.pSignalSemaphoreValues     = d3d12_fence_signal_semaphore_values_ptr;
            fence_info.pWaitSemaphoreValues       = d3d12_fence_wait_semaphore_values_ptr;
            fence_info.signalSemaphoreValuesCount = in_submit_info.get_n_signal_semaphores();
            fence_info.sType                      = VK_STRUCTURE_TYPE_D3D12_FENCE_SUBMIT_INFO_KHR;
            fence_info.waitSemaphoreValuesCount   = in_submit_info.get_n_wait_semaphores();

            struct_chainer.append_struct(fence_info);
        }
    }
    #endif

    /* Go for it */
    if (fence_ptr                         == nullptr &&
        in_submit_info.get_should_block() )
    {
        fence_ptr         = m_submit_fence_ptr.get();
        needs_fence_reset = true;
    }

    switch (in_submit_info.get_type() )
    {
        case SubmissionType::MGPU:
        {
            submit_command_buffers_lock_unlock(in_submit_info.get_n_command_buffers     (),
                                               in_submit_info.get_command_buffers_mgpu  (),
                                               in_submit_info.get_n_signal_semaphores   (),
                                               in_submit_info.get_signal_semaphores_mgpu(),
                                               in_submit_info.get_n_wait_semaphores     (),
                                               in_submit_info.get_wait_semaphores_mgpu  (),
                                               fence_ptr,
                                               true);

            break;
        }

        case SubmissionType::SGPU:
        {
             submit_command_buffers_lock_unlock(in_submit_info.get_n_command_buffers     (),
                                                in_submit_info.get_command_buffers_sgpu  (),
                                                in_submit_info.get_n_signal_semaphores   (),
                                                in_submit_info.get_signal_semaphores_sgpu(),
                                                in_submit_info.get_n_wait_semaphores     (),
                                                in_submit_info.get_wait_semaphores_sgpu  (),
                                                fence_ptr,
                                                true); /* in_should_lock */

             break;
        }

        default:
        {
            anvil_assert_fail();
        }
     }

     {
        auto chain_ptr = struct_chainer.create_chain();

        if (needs_fence_reset)
        {
            m_submit_fence_ptr->reset();
        }

         result = Anvil::Vulkan::vkQueueSubmit(m_queue,
                                               1, /* submitCount */
                                               chain_ptr->get_root_struct(),
                                              (fence_ptr != nullptr) ? fence_ptr->get_fence()
                                                                     : VK_NULL_HANDLE);

        if (in_submit_info.get_should_block() )
        {
            /* Wait till initialization finishes GPU-side */
            result = Anvil::Vulkan::vkWaitForFences(m_device_ptr->get_device_vk(),
                                                    1, /* fenceCount */
                                                    fence_ptr->get_fence_ptr(),
                                                    VK_TRUE,     /* waitAll */
                                                    in_submit_info.get_timeout() );
        }
     }

     switch (in_submit_info.get_type() )
     {
         case SubmissionType::MGPU:
         {
             submit_command_buffers_lock_unlock(in_submit_info.get_n_command_buffers     (),
                                                in_submit_info.get_command_buffers_mgpu  (),
                                                in_submit_info.get_n_signal_semaphores   (),
                                                in_submit_info.get_signal_semaphores_mgpu(),
                                                in_submit_info.get_n_wait_semaphores     (),
                                                in_submit_info.get_wait_semaphores_mgpu  (),
                                                fence_ptr,
                                                false); /* in_should_lock */

             break;
         }

         case SubmissionType::SGPU:
         {
             submit_command_buffers_lock_unlock(in_submit_info.get_n_command_buffers     (),
                                                in_submit_info.get_command_buffers_sgpu  (),
                                                in_submit_info.get_n_signal_semaphores   (),
                                                in_submit_info.get_signal_semaphores_sgpu(),
                                                in_submit_info.get_n_wait_semaphores     (),
                                                in_submit_info.get_wait_semaphores_sgpu  (),
                                                fence_ptr,
                                                false); /* in_should_lock */

             break;
         }

         default:
         {
             anvil_assert_fail();
         }
     }

     return (result == VK_SUCCESS);
}

void Anvil::Queue::submit_command_buffers_lock_unlock(uint32_t                         in_n_command_buffers,
                                                      Anvil::CommandBufferBase* const* in_opt_cmd_buffer_ptrs,
                                                      uint32_t                         in_n_semaphores_to_signal,
                                                      Anvil::Semaphore* const*         in_opt_semaphore_to_signal_ptr_ptrs,
                                                      uint32_t                         in_n_semaphores_to_wait_on,
                                                      Anvil::Semaphore* const*         in_opt_semaphore_to_wait_on_ptr_ptrs,
                                                      Anvil::Fence*                    in_opt_fence_ptr,
                                                      bool                             in_should_lock)
{
    if (in_should_lock)
    {
        lock();
    }
    else
    {
        unlock();
    }

    for (uint32_t n_command_buffer = 0;
                  n_command_buffer < in_n_command_buffers;
                ++n_command_buffer)
    {
        if (in_should_lock)
        {
            in_opt_cmd_buffer_ptrs[n_command_buffer]->lock();
        }
        else
        {
            in_opt_cmd_buffer_ptrs[n_command_buffer]->unlock();
        }
    }

    for (uint32_t n_signal_semaphore = 0;
                  n_signal_semaphore < in_n_semaphores_to_signal;
                ++n_signal_semaphore)
    {
        if (in_should_lock)
        {
            in_opt_semaphore_to_signal_ptr_ptrs[n_signal_semaphore]->lock();
        }
        else
        {
            in_opt_semaphore_to_signal_ptr_ptrs[n_signal_semaphore]->unlock();
        }
    }

    for (uint32_t n_wait_semaphore = 0;
                  n_wait_semaphore < in_n_semaphores_to_wait_on;
                ++n_wait_semaphore)
    {
        if (in_should_lock)
        {
            in_opt_semaphore_to_wait_on_ptr_ptrs[n_wait_semaphore]->lock();
        }
        else
        {
            in_opt_semaphore_to_wait_on_ptr_ptrs[n_wait_semaphore]->unlock();
        }
    }

    if (in_opt_fence_ptr != nullptr)
    {
        if (in_should_lock)
        {
            in_opt_fence_ptr->lock();
        }
        else
        {
            in_opt_fence_ptr->unlock();
        }
    }
}

void Anvil::Queue::submit_command_buffers_lock_unlock(uint32_t                           in_n_command_buffer_submissions,
                                                      const CommandBufferMGPUSubmission* in_opt_command_buffer_submissions_ptr,
                                                      uint32_t                           in_n_signal_semaphore_submissions,
                                                      const SemaphoreMGPUSubmission*     in_opt_signal_semaphore_submissions_ptr,
                                                      uint32_t                           in_n_wait_semaphore_submissions,
                                                      const SemaphoreMGPUSubmission*     in_opt_wait_semaphore_submissions_ptr,
                                                      Anvil::Fence*                      in_opt_fence_ptr,
                                                      bool                               in_should_lock)
{
    if (in_should_lock)
    {
        lock();
    }
    else
    {
        unlock();
    }

    for (uint32_t n_command_buffer_submission = 0;
                  n_command_buffer_submission < in_n_command_buffer_submissions;
                ++n_command_buffer_submission)
    {
        const auto& current_submission = in_opt_command_buffer_submissions_ptr[n_command_buffer_submission];

        if (current_submission.cmd_buffer_ptr != nullptr)
        {
            if (in_should_lock)
            {
                current_submission.cmd_buffer_ptr->lock();
            }
            else
            {
                current_submission.cmd_buffer_ptr->unlock();
            }
        }
    }

    for (uint32_t n_signal_semaphore_submission = 0;
                  n_signal_semaphore_submission < in_n_signal_semaphore_submissions;
                ++n_signal_semaphore_submission)
    {
        if (in_should_lock)
        {
            in_opt_signal_semaphore_submissions_ptr[n_signal_semaphore_submission].semaphore_ptr->lock();
        }
        else
        {
            in_opt_signal_semaphore_submissions_ptr[n_signal_semaphore_submission].semaphore_ptr->unlock();
        }
    }

    for (uint32_t n_wait_semaphore_submission = 0;
                  n_wait_semaphore_submission < in_n_wait_semaphore_submissions;
                ++n_wait_semaphore_submission)
    {
        if (in_should_lock)
        {
            in_opt_wait_semaphore_submissions_ptr[n_wait_semaphore_submission].semaphore_ptr->lock();
        }
        else
        {
            in_opt_wait_semaphore_submissions_ptr[n_wait_semaphore_submission].semaphore_ptr->unlock();
        }
    }

    if (in_opt_fence_ptr != nullptr)
    {
        if (in_should_lock)
        {
            in_opt_fence_ptr->lock();
        }
        else
        {
            in_opt_fence_ptr->unlock();
        }
    }
}

void Anvil::Queue::wait_idle()
{
    lock();
    {
        Anvil::Vulkan::vkQueueWaitIdle(m_queue);
    }
    unlock();
}