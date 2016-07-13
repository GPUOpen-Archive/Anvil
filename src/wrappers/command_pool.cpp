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
#include "misc/types.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/device.h"
#include "wrappers/queue.h"
#include <algorithm>

/* Please see header for specification */
Anvil::CommandPool::CommandPool(Anvil::Device*         device_ptr,
                                bool                   transient_allocations_friendly,
                                bool                   support_per_cmdbuf_reset_ops,
                                Anvil::QueueFamilyType queue_family)

    :m_command_pool                     (VK_NULL_HANDLE),
     m_device_ptr                       (device_ptr),
     m_supports_per_cmdbuf_reset_ops    (support_per_cmdbuf_reset_ops),
     m_is_transient_allocations_friendly(transient_allocations_friendly)
{
    VkCommandPoolCreateInfo command_pool_create_info;
    uint32_t                queue_family_index = m_device_ptr->get_queue_family_index(queue_family);
    VkResult                result_vk;

    /* Go on and create the command pool */
    command_pool_create_info.flags            = ((transient_allocations_friendly) ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT            : 0) |
                                                ((support_per_cmdbuf_reset_ops)   ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0);
    command_pool_create_info.pNext            = nullptr;
    command_pool_create_info.queueFamilyIndex = queue_family_index;
    command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    result_vk = vkCreateCommandPool(m_device_ptr->get_device_vk(),
                                   &command_pool_create_info,
                                    nullptr, /* pAllocator */
                                   &m_command_pool);

    anvil_assert                  (queue_family_index != -1);
    anvil_assert_vk_call_succeeded(result_vk);

    /* Register the command pool instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_COMMAND_POOL,
                                                 this);
}

/* Please see header for specification */
Anvil::CommandPool::~CommandPool()
{
    /* Release the Vulkan command pool */
    if (m_command_pool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_device_ptr->get_device_vk(),
                             m_command_pool,
                             nullptr /* pAllocator */);

        m_command_pool = VK_NULL_HANDLE;
    }

    /* If there are any buffer wrapper instances still kicking at this point, force-release them. */
    for (auto alloc_iterator  = m_allocs_in_flight.begin();
              alloc_iterator != m_allocs_in_flight.end();
            ++alloc_iterator)
    {
        (*alloc_iterator)->on_parent_pool_released();
        (*alloc_iterator)->force_release();
    }

    m_allocs_in_flight.clear();

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_COMMAND_POOL,
                                                    this);
}

/* Please see header for specification */
Anvil::PrimaryCommandBuffer* Anvil::CommandPool::alloc_primary_level_command_buffer()
{
    PrimaryCommandBuffer* new_buffer_ptr = new PrimaryCommandBuffer(m_device_ptr,
                                                                    this /* parent_pool_ptr */);

    m_allocs_in_flight.push_back(static_cast<CommandBufferBase*>(new_buffer_ptr) );

    return new_buffer_ptr;
}

/* Please see header for specification */
Anvil::SecondaryCommandBuffer* Anvil::CommandPool::alloc_secondary_level_command_buffer()
{
    SecondaryCommandBuffer* new_buffer_ptr = new SecondaryCommandBuffer(m_device_ptr,
                                                                        this /* parent_command_pool_ptr */);

    m_allocs_in_flight.push_back(static_cast<CommandBufferBase*>(new_buffer_ptr) );

    return new_buffer_ptr;
}

/** Called back when a command buffer, which allocates its commands from this pool, is released.
 *
 *  @param command_buffer_ptr The released command buffer instance.
 **/
void Anvil::CommandPool::on_command_buffer_wrapper_destroyed(CommandBufferBase* command_buffer_ptr)
{
    /* The command buffer is being released back to the pool. No longer consider the buffer as allocated,
     * so that we do not accidentally delete the wrapper instance for the second time at tear-down time.
     */
    auto alloc_iterator = std::find(m_allocs_in_flight.begin(),
                                    m_allocs_in_flight.end(),
                                    command_buffer_ptr);

    anvil_assert(alloc_iterator != m_allocs_in_flight.end() );

    if (alloc_iterator != m_allocs_in_flight.end() )
    {
        m_allocs_in_flight.erase(alloc_iterator);
    }
}

/* Please see header for specification */
bool Anvil::CommandPool::reset(bool release_resources)
{
    VkResult result_vk;

    result_vk = vkResetCommandPool(m_device_ptr->get_device_vk(),
                                   m_command_pool,
                                   ((release_resources) ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0) );

    anvil_assert_vk_call_succeeded(result_vk);

    return is_vk_call_successful(result_vk);
}
