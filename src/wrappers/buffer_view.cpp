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
#include "wrappers/buffer.h"
#include "wrappers/buffer_view.h"
#include "wrappers/device.h"

/** Please see header for specification */
Anvil::BufferView::BufferView(Anvil::Device* device_ptr,
                              Anvil::Buffer* buffer_ptr,
                              VkFormat       format,
                              VkDeviceSize   start_offset,
                              VkDeviceSize   size)
    :m_buffer_ptr  (buffer_ptr),
     m_device_ptr  (device_ptr),
     m_format      (format),
     m_size        (size),
     m_start_offset(start_offset)
{
    VkBufferViewCreateInfo buffer_view_create_info;
    VkResult               result;

    /* Spawn a new event */
    buffer_view_create_info.buffer = buffer_ptr->get_buffer();
    buffer_view_create_info.flags  = 0;
    buffer_view_create_info.format = format;
    buffer_view_create_info.offset = start_offset;
    buffer_view_create_info.pNext  = nullptr;
    buffer_view_create_info.range  = size;
    buffer_view_create_info.sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;

    result = vkCreateBufferView(m_device_ptr->get_device_vk(),
                               &buffer_view_create_info,
                                nullptr, /* pAllocator */
                               &m_buffer_view);

    anvil_assert_vk_call_succeeded(result);

    /* Register the buffer view instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_BUFFER_VIEW,
                                                  this);
}

/** Destructor.
 *
 *  Releases the underlying Vulkan Buffer View instance and signs the wrapper object out from
 *  the Object Tracker.
 **/
Anvil::BufferView::~BufferView()
{
    vkDestroyBufferView(m_device_ptr->get_device_vk(),
                        m_buffer_view,
                        nullptr /* pAllocator */);

    m_buffer_view = VK_NULL_HANDLE;

    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_BUFFER_VIEW,
                                                    this);
}

