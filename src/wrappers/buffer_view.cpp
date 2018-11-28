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

#include "misc/buffer_view_create_info.h"
#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/buffer.h"
#include "wrappers/buffer_view.h"
#include "wrappers/device.h"

/** Please see header for specification */
Anvil::BufferView::BufferView(Anvil::BufferViewCreateInfoUniquePtr in_create_info_ptr)
    :DebugMarkerSupportProvider<BufferView>(in_create_info_ptr->get_device(),
                                            Anvil::ObjectType::BUFFER_VIEW),
     MTSafetySupportProvider               (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                            in_create_info_ptr->get_device   () ))
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    /* Register the buffer view instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::BUFFER_VIEW,
                                                 this);
}

/** Destructor.
 *
 *  Releases the underlying Vulkan Buffer View instance and signs the wrapper object out from
 *  the Object Tracker.
 **/
Anvil::BufferView::~BufferView()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::BUFFER_VIEW,
                                                    this);

    lock();
    {
        Anvil::Vulkan::vkDestroyBufferView(m_device_ptr->get_device_vk(),
                                           m_buffer_view,
                                           nullptr /* pAllocator */);
    }
    unlock();

    m_buffer_view = VK_NULL_HANDLE;
}

Anvil::BufferViewUniquePtr Anvil::BufferView::create(Anvil::BufferViewCreateInfoUniquePtr in_create_info_ptr)
{
    Anvil::BufferViewUniquePtr result_ptr(nullptr,
                                          std::default_delete<Anvil::BufferView>() );

    result_ptr.reset(
        new Anvil::BufferView(std::move(in_create_info_ptr) )
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool Anvil::BufferView::init()
{
    VkBufferViewCreateInfo buffer_view_create_info;
    VkResult               result                 (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Spawn a new Vulkan buffer view */
    buffer_view_create_info.buffer = m_create_info_ptr->get_parent_buffer()->get_buffer();
    buffer_view_create_info.flags  = 0;
    buffer_view_create_info.format = static_cast<VkFormat>(m_create_info_ptr->get_format() );
    buffer_view_create_info.offset = m_create_info_ptr->get_start_offset();
    buffer_view_create_info.pNext  = nullptr;
    buffer_view_create_info.range  = m_create_info_ptr->get_size();
    buffer_view_create_info.sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;

    result = Anvil::Vulkan::vkCreateBufferView(m_create_info_ptr->get_device()->get_device_vk(),
                                              &buffer_view_create_info,
                                               nullptr, /* pAllocator */
                                              &m_buffer_view);

    if (is_vk_call_successful(result) )
    {
        anvil_assert_vk_call_succeeded(result);
        if (is_vk_call_successful(result) )
        {
            set_vk_handle(m_buffer_view);
        }
    }

    return is_vk_call_successful(result);
}
