//
// Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
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
Anvil::CommandPool::CommandPool(Anvil::BaseDevice*                   in_device_ptr,
                                const Anvil::CommandPoolCreateFlags& in_create_flags,
                                uint32_t                             in_queue_family_index,
                                bool                                 in_mt_safe)

    :DebugMarkerSupportProvider(in_device_ptr,
                                Anvil::ObjectType::COMMAND_POOL),
     MTSafetySupportProvider   (in_mt_safe),
     m_command_pool            (VK_NULL_HANDLE),
     m_create_flags            (in_create_flags),
     m_device_ptr              (in_device_ptr),
     m_queue_family_index      (in_queue_family_index)
{
    VkCommandPoolCreateInfo command_pool_create_info;
    VkResult                result_vk               (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    /* Go on and create the command pool */
    command_pool_create_info.flags            = in_create_flags.get_vk();
    command_pool_create_info.pNext            = nullptr;
    command_pool_create_info.queueFamilyIndex = in_queue_family_index;
    command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    result_vk = Anvil::Vulkan::vkCreateCommandPool(in_device_ptr->get_device_vk(),
                                                  &command_pool_create_info,
                                                   nullptr, /* pAllocator */
                                                  &m_command_pool);

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk) )
    {
        set_vk_handle(m_command_pool);
    }

    /* Register the command pool instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::COMMAND_POOL,
                                                 this);
}

/* Please see header for specification */
Anvil::CommandPool::~CommandPool()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::COMMAND_POOL,
                                                   this);

    /* Release the Vulkan command pool */
    if (m_command_pool != VK_NULL_HANDLE)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyCommandPool(m_device_ptr->get_device_vk(),
                                                m_command_pool,
                                                nullptr /* pAllocator */);
        }
        unlock();

        m_command_pool = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
Anvil::PrimaryCommandBufferUniquePtr Anvil::CommandPool::alloc_primary_level_command_buffer()
{
    Anvil::PrimaryCommandBufferUniquePtr new_buffer_ptr(nullptr,
                                                        std::default_delete<PrimaryCommandBuffer>() );

    new_buffer_ptr.reset(
        new PrimaryCommandBuffer(m_device_ptr,
                                 this,
                                 is_mt_safe() )
    );

    return new_buffer_ptr;
}

/* Please see header for specification */
Anvil::SecondaryCommandBufferUniquePtr Anvil::CommandPool::alloc_secondary_level_command_buffer()
{
    Anvil::SecondaryCommandBufferUniquePtr new_buffer_ptr(nullptr,
                                                          std::default_delete<Anvil::SecondaryCommandBuffer>() );

    new_buffer_ptr.reset(
        new SecondaryCommandBuffer(m_device_ptr,
                                   this,
                                   is_mt_safe() )
    );

    return new_buffer_ptr;
}

/* Please see header for specification */
Anvil::CommandPoolUniquePtr Anvil::CommandPool::create(Anvil::BaseDevice*                   in_device_ptr,
                                                       const Anvil::CommandPoolCreateFlags& in_create_flags,
                                                       uint32_t                             in_queue_family_index,
                                                       MTSafety                             in_mt_safety)
{
    const bool                  is_mt_safe = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                             in_device_ptr);
    Anvil::CommandPoolUniquePtr result_ptr(nullptr,
                                           std::default_delete<Anvil::CommandPool>() );

    result_ptr.reset(
        new Anvil::CommandPool(in_device_ptr,
                               in_create_flags,
                               in_queue_family_index,
                               is_mt_safe)
    );

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::CommandPool::reset(bool in_release_resources)
{
    std::unique_lock<std::mutex> mutex_lock;
    VkResult                     result_vk;

    lock();
    {
        result_vk = Anvil::Vulkan::vkResetCommandPool(m_device_ptr->get_device_vk(),
                                                      m_command_pool,
                                                      ((in_release_resources) ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0u) );
    }
    unlock();

    anvil_assert_vk_call_succeeded(result_vk);

    return is_vk_call_successful(result_vk);
}

/* Please see header for specification */
void Anvil::CommandPool::trim()
{
    if (m_device_ptr->get_extension_info()->khr_maintenance1() )
    {
        lock();
        {
            m_device_ptr->get_extension_khr_maintenance1_entrypoints().vkTrimCommandPoolKHR(m_device_ptr->get_device_vk(),
                                                                                            m_command_pool,
                                                                                            0); /* flags */
        }
        unlock();
    }
    else
    {
        anvil_assert(m_device_ptr->get_extension_info()->khr_maintenance1() );
    }
}