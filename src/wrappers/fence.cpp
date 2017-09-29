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
#include "wrappers/device.h"
#include "wrappers/fence.h"
#include <algorithm>

/* Please see header for specification */
Anvil::Fence::Fence(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                    bool                             in_create_signalled)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT),
     m_device_ptr              (in_device_ptr),
     m_fence                   (VK_NULL_HANDLE),
     m_possibly_set            (false)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkFenceCreateInfo                  fence_create_info;
    VkResult                           result           (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Spawn a new fence */
    fence_create_info.flags = (in_create_signalled) ? VK_FENCE_CREATE_SIGNALED_BIT
                                                    : 0u;
    fence_create_info.pNext = nullptr;
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    result = vkCreateFence(device_locked_ptr->get_device_vk(),
                          &fence_create_info,
                           nullptr, /* pAllocator */
                          &m_fence);

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        set_vk_handle(m_fence);
    }

    /* Register the event instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_FENCE,
                                                  this);
}

/* Please see header for specification */
Anvil::Fence::~Fence()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_FENCE,
                                                    this);

    release_fence();
}

/* Please see header for specification */
std::shared_ptr<Anvil::Fence> Anvil::Fence::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                   bool                             in_create_signalled)
{
    std::shared_ptr<Anvil::Fence> result_ptr;

    result_ptr.reset(
        new Anvil::Fence(in_device_ptr,
                         in_create_signalled)
    );

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::Fence::is_set() const
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkResult                           result;

    result = vkGetFenceStatus(device_locked_ptr->get_device_vk(),
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
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroyFence(device_locked_ptr->get_device_vk(),
                       m_fence,
                       nullptr /* pAllocator */);

        m_fence = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
bool Anvil::Fence::reset()
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkResult                           result;

    result = vkResetFences(device_locked_ptr->get_device_vk(),
                           1, /* fenceCount */
                          &m_fence);
    anvil_assert_vk_call_succeeded(result);

    m_possibly_set = false;

    return (result == VK_SUCCESS);
}

/* Please see header for specification */
bool Anvil::Fence::reset_fences(const uint32_t in_n_fences,
                                Fence*         in_fences)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr;
    VkFence                            fence_cache[32];
    static const uint32_t              fence_cache_capacity = sizeof(fence_cache) / sizeof(fence_cache[0]);
    bool                               result               = true;
    VkResult                           result_vk;

    if (in_n_fences == 0)
    {
        goto end;
    }

    for (uint32_t n_fence_batch = 0;
                  n_fence_batch < 1 + in_n_fences / fence_cache_capacity;
                ++n_fence_batch)
    {
        const uint32_t n_fences_remaining = in_n_fences - n_fence_batch * fence_cache_capacity;

        for (uint32_t n_fence = 0;
                      n_fence < n_fences_remaining;
                    ++n_fence)
        {
            Anvil::Fence& current_fence = in_fences[n_fence_batch * fence_cache_capacity + n_fence];

            anvil_assert(device_locked_ptr == nullptr                                          ||
                         device_locked_ptr != nullptr && !current_fence.m_device_ptr.expired());

            device_locked_ptr    = current_fence.m_device_ptr.lock();
            fence_cache[n_fence] = current_fence.m_fence;

            current_fence.m_possibly_set = false;
        }

        result_vk = vkResetFences(device_locked_ptr->get_device_vk(),
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