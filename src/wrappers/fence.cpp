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
#include "wrappers/device.h"
#include "wrappers/fence.h"
#include <algorithm>

/* Please see header for specification */
Anvil::Fence::Fence(Anvil::Device* device_ptr,
                    bool           create_signalled)
    :m_device_ptr  (device_ptr),
     m_fence       (VK_NULL_HANDLE),
     m_possibly_set(false)
{
    VkFenceCreateInfo fence_create_info;
    VkResult          result;

   
    /* Spawn a new fence */
    fence_create_info.flags = (create_signalled) ? VK_FENCE_CREATE_SIGNALED_BIT
                                                 : 0;
    fence_create_info.pNext = nullptr;
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    result = vkCreateFence(m_device_ptr->get_device_vk(),
                          &fence_create_info,
                           nullptr, /* pAllocator */
                          &m_fence);
    anvil_assert_vk_call_succeeded(result);

    /* Register the event instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_FENCE,
                                                  this);
}

/* Please see header for specification */
Anvil::Fence::~Fence()
{
    release_fence();

    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_FENCE,
                                                    this);
}

/* Please see header for specification */
bool Anvil::Fence::is_set() const
{
    VkResult result;

    result = vkGetFenceStatus(m_device_ptr->get_device_vk(),
                              m_fence);

    anvil_assert(result == VK_SUCCESS  ||
                 result == VK_NOT_READY);

    return (result == VK_SUCCESS);
}

/** Destroys the underlying Vulkan Fence instance. */
void Anvil::Fence::release_fence()
{
    if (m_fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(m_device_ptr->get_device_vk(),
                       m_fence,
                       nullptr /* pAllocator */);

        m_fence = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
bool Anvil::Fence::reset()
{
    VkResult result;

    result = vkResetFences(m_device_ptr->get_device_vk(),
                           1, /* fenceCount */
                          &m_fence);
    anvil_assert_vk_call_succeeded(result);

    m_possibly_set = false;

    return (result == VK_SUCCESS);
}

/* Please see header for specification */
bool Anvil::Fence::reset_fences(const uint32_t n_fences,
                                Fence*         fences)
{
    Anvil::Device*       device_ptr           = nullptr;
    VkFence               fence_cache[32];
    static const uint32_t fence_cache_capacity = sizeof(fence_cache) / sizeof(fence_cache[0]);
    bool                  result               = true;
    VkResult              result_vk;

    if (n_fences == 0)
    {
        goto end;
    }

    for (uint32_t n_fence_batch = 0;
                  n_fence_batch < 1 + n_fences / fence_cache_capacity;
                ++n_fence_batch)
    {
        const uint32_t n_fences_remaining = n_fences - n_fence_batch * fence_cache_capacity;

        for (uint32_t n_fence = 0;
                      n_fence < n_fences_remaining;
                    ++n_fence)
        {
            Anvil::Fence& current_fence = fences[n_fence_batch * fence_cache_capacity + n_fence];

            anvil_assert(device_ptr == nullptr                            ||
                        device_ptr != nullptr && current_fence.m_device_ptr);

            device_ptr           = current_fence.m_device_ptr;
            fence_cache[n_fence] = current_fence.m_fence;

            current_fence.m_possibly_set = false;
        }

        result_vk = vkResetFences(device_ptr->get_device_vk(),
                                  n_fences_remaining,
                                  fence_cache);
        anvil_assert_vk_call_succeeded(result_vk);

        if (!is_vk_call_successful(result_vk) )
        {
            result = false;
        }
    }

end:
    return result;
}