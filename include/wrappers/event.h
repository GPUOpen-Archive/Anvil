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

/** Implements a wrapper for a single Vulkan event. Implemented in order to:
 *
 *  - simplify life-time management of events.
 *  - simplify event usage.
 *  - let ObjectTracker detect leaking event instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_EVENT_H
#define WRAPPERS_EVENT_H

#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
    /** Wrapper class for Vulkan events */
    class Event : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  Creates a single Vulkan event instance and registers the object in Object Tracker.
         *
         *  @param device_ptr Device to use.
         */
        Event(Anvil::Device* device_ptr);

        /** Retrieves a raw Vulkan handle for the underlying VkEvent instance. */
        VkEvent get_event() const
        {
            return m_event;
        }

        /** Retrieves a pointer to the raw Vulkan handle for the underlying VkEvent instance. */
        const VkEvent* get_event_ptr() const
        {
            return &m_event;
        }

        /** Tells whether the event is signalled at the time of the call.
         *
         *  @return true if the event is set, false otherwise.
         **/
        bool is_set() const;

        /** Resets the Vulkan Event, if set. If the event is not set, this function is a nop.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool reset();

        /** Sets the Vulkan Event.
         *
         *  @return true if the function executed successfully, false otherwise. */
        bool set();

    private:
        /* Private functions */
        Event           (const Event&);
        Event& operator=(const Event&);

        virtual ~Event();

        void release_event();

        /* Private variables */
        Anvil::Device* m_device_ptr;
        VkEvent        m_event;
    };

    /** Delete functor. Useful if you need to wrap the event instance in an auto pointer */
    struct EventDeleter
    {
        void operator()(Event* event_ptr) const
        {
            event_ptr->release();
        }
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_EVENT_H */