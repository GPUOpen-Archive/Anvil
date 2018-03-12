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
#include "wrappers/event.h"

/* Please see header for specification */
Anvil::Event::Event(const Anvil::BaseDevice* in_device_ptr,
                    bool                     in_mt_safe)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT),
     MTSafetySupportProvider   (in_mt_safe),
     m_device_ptr              (in_device_ptr),
     m_event                   (VK_NULL_HANDLE)
{
    VkEventCreateInfo event_create_info;
    VkResult          result           (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Spawn a new event */
    event_create_info.flags = 0;
    event_create_info.pNext = nullptr;
    event_create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    result = vkCreateEvent(m_device_ptr->get_device_vk(),
                          &event_create_info,
                           nullptr, /* pAllocator */
                          &m_event);

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        set_vk_handle(m_event);
    }

    /* Register the event instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_EVENT,
                                                  this);
}

/** Destructor.
 *
 *  Releases the underlying Vulkan Event instance and signs the wrapper object out from
 *  the Object Tracker.
 **/
Anvil::Event::~Event()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_EVENT,
                                                    this);

    release_event();
}

/* Please see header for specification */
Anvil::EventUniquePtr Anvil::Event::create(const Anvil::BaseDevice* in_device_ptr,
                                           MTSafety                 in_mt_safety)
{
    const bool            mt_safe    = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                       in_device_ptr);
    Anvil::EventUniquePtr result_ptr(nullptr,
                                     std::default_delete<Anvil::Event>() );

    result_ptr.reset(
        new Anvil::Event(in_device_ptr,
                         mt_safe)
    );

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::Event::is_set() const
{
    VkResult result;

    result = vkGetEventStatus(m_device_ptr->get_device_vk(),
                              m_event);

    anvil_assert(result == VK_EVENT_RESET ||
                 result == VK_EVENT_SET);

    return (result == VK_EVENT_SET);
}

/** Destroys the underlying Vulkan Event instance. */
void Anvil::Event::release_event()
{
    if (m_event != VK_NULL_HANDLE)
    {
        lock();
        {
            vkDestroyEvent(m_device_ptr->get_device_vk(),
                           m_event,
                           nullptr /* pAllocator */);
        }
        unlock();

        m_event = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
bool Anvil::Event::reset()
{
    VkResult result;

    lock();
    {
        result = vkResetEvent(m_device_ptr->get_device_vk(),
                              m_event);
    }
    unlock();

    anvil_assert_vk_call_succeeded(result);

    return (result == VK_SUCCESS);
}

/* Please see header for specification */
bool Anvil::Event::set()
{
    VkResult result;

    lock();
    {
        result = vkSetEvent(m_device_ptr->get_device_vk(),
                            m_event);
    }
    unlock();

    anvil_assert_vk_call_succeeded(result);

    return (result == VK_SUCCESS);
}