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

#include "misc/debug.h"
#include "misc/event_create_info.h"
#include "misc/object_tracker.h"
#include "wrappers/device.h"
#include "wrappers/event.h"

/* Please see header for specification */
Anvil::Event::Event(Anvil::EventCreateInfoUniquePtr in_create_info_ptr)
    :DebugMarkerSupportProvider(in_create_info_ptr->get_device(),
                                Anvil::ObjectType::EVENT),
     MTSafetySupportProvider   (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                in_create_info_ptr->get_device   () )),
     m_event                   (VK_NULL_HANDLE)
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    /* Register the event instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::EVENT,
                                                  this);
}

/** Destructor.
 *
 *  Releases the underlying Vulkan Event instance and signs the wrapper object out from
 *  the Object Tracker.
 **/
Anvil::Event::~Event()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::EVENT,
                                                    this);

    release_event();
}

/* Please see header for specification */
Anvil::EventUniquePtr Anvil::Event::create(Anvil::EventCreateInfoUniquePtr in_create_info_ptr)
{
    Anvil::EventUniquePtr result_ptr(nullptr,
                                     std::default_delete<Anvil::Event>() );

    result_ptr.reset(
        new Anvil::Event(std::move(in_create_info_ptr) )
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

bool Anvil::Event::init()
{
    VkEventCreateInfo event_create_info;
    VkResult          result           (VK_ERROR_INITIALIZATION_FAILED);

    /* Spawn a new event */
    event_create_info.flags = 0;
    event_create_info.pNext = nullptr;
    event_create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    result = Anvil::Vulkan::vkCreateEvent(m_device_ptr->get_device_vk(),
                                         &event_create_info,
                                          nullptr, /* pAllocator */
                                         &m_event);

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        set_vk_handle(m_event);
    }

    return is_vk_call_successful(result);
}

/* Please see header for specification */
bool Anvil::Event::is_set() const
{
    VkResult result;

    result = Anvil::Vulkan::vkGetEventStatus(m_device_ptr->get_device_vk(),
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
            Anvil::Vulkan::vkDestroyEvent(m_device_ptr->get_device_vk(),
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
        result = Anvil::Vulkan::vkResetEvent(m_device_ptr->get_device_vk(),
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
        result = Anvil::Vulkan::vkSetEvent(m_device_ptr->get_device_vk(),
                                           m_event);
    }
    unlock();

    anvil_assert_vk_call_succeeded(result);

    return (result == VK_SUCCESS);
}