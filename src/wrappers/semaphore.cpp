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
#include "wrappers/semaphore.h"

/* Please see header for specification */
Anvil::Semaphore::Semaphore(Anvil::Device* device_ptr)
    :m_device_ptr(device_ptr),
     m_semaphore (VK_NULL_HANDLE)
{
    reset();

    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_SEMAPHORE,
                                                  this);
}

/* Please see header for specification */
Anvil::Semaphore::~Semaphore()
{
    release_semaphore();

    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_SEMAPHORE,
                                                    this);
}

/** Destroys the underlying Vulkan Semaphore instance. */
void Anvil::Semaphore::release_semaphore()
{
    if (m_semaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_device_ptr->get_device_vk(),
                           m_semaphore,
                           nullptr /* pAllocator */);

        m_semaphore = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
void Anvil::Semaphore::reset()
{
    VkResult              result;
    VkSemaphoreCreateInfo semaphore_create_info;

    release_semaphore();

    /* Spawn a new semaphore */
    semaphore_create_info.flags = 0;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    result = vkCreateSemaphore(m_device_ptr->get_device_vk(),
                              &semaphore_create_info,
                               nullptr, /* pAllocator */
                              &m_semaphore);
    anvil_assert_vk_call_succeeded(result);
}