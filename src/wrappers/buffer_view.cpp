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
#include "wrappers/buffer.h"
#include "wrappers/buffer_view.h"
#include "wrappers/device.h"

/** Please see header for specification */
Anvil::BufferView::BufferView(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                              std::shared_ptr<Anvil::Buffer>   in_buffer_ptr,
                              VkFormat                         in_format,
                              VkDeviceSize                     in_start_offset,
                              VkDeviceSize                     in_size)
    :DebugMarkerSupportProvider<BufferView>(in_device_ptr,
                                            VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT),
     m_buffer_ptr  (in_buffer_ptr),
     m_device_ptr  (in_device_ptr),
     m_format      (in_format),
     m_size        (in_size),
     m_start_offset(in_start_offset)
{
    VkBufferViewCreateInfo             buffer_view_create_info;
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(in_device_ptr);
    VkResult                           result           (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Spawn a new Vulkan buffer view */
    buffer_view_create_info.buffer = in_buffer_ptr->get_buffer();
    buffer_view_create_info.flags  = 0;
    buffer_view_create_info.format = in_format;
    buffer_view_create_info.offset = in_start_offset;
    buffer_view_create_info.pNext  = nullptr;
    buffer_view_create_info.range  = in_size;
    buffer_view_create_info.sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;

    result = vkCreateBufferView(device_locked_ptr->get_device_vk(),
                               &buffer_view_create_info,
                                nullptr, /* pAllocator */
                               &m_buffer_view);

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        set_vk_handle(m_buffer_view);
    }
}

/** Destructor.
 *
 *  Releases the underlying Vulkan Buffer View instance and signs the wrapper object out from
 *  the Object Tracker.
 **/
Anvil::BufferView::~BufferView()
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);


    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_BUFFER_VIEW,
                                                    this);

    vkDestroyBufferView(device_locked_ptr->get_device_vk(),
                        m_buffer_view,
                        nullptr /* pAllocator */);

    m_buffer_view = VK_NULL_HANDLE;
}

/** Please see header for specification */
std::shared_ptr<Anvil::BufferView> Anvil::BufferView::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                             std::shared_ptr<Anvil::Buffer>   in_buffer_ptr,
                                                             VkFormat                         in_format,
                                                             VkDeviceSize                     in_start_offset,
                                                             VkDeviceSize                     in_size)
{
    std::shared_ptr<Anvil::BufferView> result_ptr;

    /* Instantiate the object */
    result_ptr.reset(
        new BufferView(
            in_device_ptr,
            in_buffer_ptr,
            in_format,
            in_start_offset,
            in_size)
    );

    /* Register the buffer view instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_BUFFER_VIEW,
                                                 result_ptr.get() );

    return result_ptr;
}