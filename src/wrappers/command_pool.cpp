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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "misc/types.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/device.h"
#include "wrappers/queue.h"
#include <algorithm>

/* Please see header for specification */
Anvil::CommandPool::CommandPool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                bool                             in_transient_allocations_friendly,
                                bool                             in_support_per_cmdbuf_reset_ops,
                                Anvil::QueueFamilyType           in_queue_family)

    :DebugMarkerSupportProvider         (in_device_ptr,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT),
     m_command_pool                     (VK_NULL_HANDLE),
     m_device_ptr                       (in_device_ptr),
     m_is_transient_allocations_friendly(in_transient_allocations_friendly),
     m_queue_family                     (in_queue_family),
     m_supports_per_cmdbuf_reset_ops    (in_support_per_cmdbuf_reset_ops)
{
    VkCommandPoolCreateInfo            command_pool_create_info;
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr       (in_device_ptr);
    uint32_t                           queue_family_index      (UINT32_MAX);
    VkResult                           result_vk               (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    queue_family_index = (device_locked_ptr->get_queue_family_index(in_queue_family) );
    anvil_assert(queue_family_index != UINT32_MAX);

    /* Go on and create the command pool */
    command_pool_create_info.flags            = ((in_transient_allocations_friendly) ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT            : 0u) |
                                                ((in_support_per_cmdbuf_reset_ops)   ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0u);
    command_pool_create_info.pNext            = nullptr;
    command_pool_create_info.queueFamilyIndex = queue_family_index;
    command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    result_vk = vkCreateCommandPool(device_locked_ptr->get_device_vk(),
                                   &command_pool_create_info,
                                    nullptr, /* pAllocator */
                                   &m_command_pool);

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk) )
    {
        set_vk_handle(m_command_pool);
    }

    /* Register the command pool instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_COMMAND_POOL,
                                                 this);
}

/* Please see header for specification */
Anvil::CommandPool::~CommandPool()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_COMMAND_POOL,
                                                   this);

    /* Release the Vulkan command pool */
    if (m_command_pool != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroyCommandPool(device_locked_ptr->get_device_vk(),
                             m_command_pool,
                             nullptr /* pAllocator */);

        m_command_pool = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
std::shared_ptr<Anvil::PrimaryCommandBuffer> Anvil::CommandPool::alloc_primary_level_command_buffer()
{
    std::shared_ptr<PrimaryCommandBuffer> new_buffer_ptr(new PrimaryCommandBuffer(m_device_ptr,
                                                                                  shared_from_this() ));

    return new_buffer_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::SecondaryCommandBuffer> Anvil::CommandPool::alloc_secondary_level_command_buffer()
{
    std::shared_ptr<SecondaryCommandBuffer> new_buffer_ptr(new SecondaryCommandBuffer(m_device_ptr,
                                                                                      shared_from_this() ));

    return new_buffer_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::CommandPool> Anvil::CommandPool::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                               bool                             in_transient_allocations_friendly,
                                                               bool                             in_support_per_cmdbuf_reset_ops,
                                                               Anvil::QueueFamilyType           in_queue_family)
{
    std::shared_ptr<Anvil::CommandPool> result_ptr;

    result_ptr.reset(
        new Anvil::CommandPool(in_device_ptr,
                               in_transient_allocations_friendly,
                               in_support_per_cmdbuf_reset_ops,
                               in_queue_family)
    );

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::CommandPool::reset(bool in_release_resources)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkResult                           result_vk;

    result_vk = vkResetCommandPool(device_locked_ptr->get_device_vk(),
                                   m_command_pool,
                                   ((in_release_resources) ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0u) );

    anvil_assert_vk_call_succeeded(result_vk);

    return is_vk_call_successful(result_vk);
}

/* Please see header for specification */
void Anvil::CommandPool::trim()
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

    if (device_locked_ptr->is_khr_maintenance1_extension_enabled() )
    {
        device_locked_ptr->get_extension_khr_maintenance1_entrypoints().vkTrimCommandPoolKHR(device_locked_ptr->get_device_vk(),
                                                                                             m_command_pool,
                                                                                             0); /* flags */
    }
    else
    {
        anvil_assert(device_locked_ptr->is_khr_maintenance1_extension_enabled() );
    }
}