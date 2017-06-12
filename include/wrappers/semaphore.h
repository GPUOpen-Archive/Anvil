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

/** Implements a wrapper for a single Vulkan semaphore. Implemented in order to:
 *
 *  - simplify life-time management of semaphores.
 *  - simplify semaphore usage.
 *  - let ObjectTracker detect leaking semaphore instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_SEMAPHORE_H
#define WRAPPERS_SEMAPHORE_H

#include "misc/debug_marker.h"
#include "misc/types.h"

namespace Anvil
{
    /* Wrapper class for Vulkan semaphores */
    class Semaphore : public DebugMarkerSupportProvider<Semaphore>
    {
    public:
        /* Public functions */

        /** Creates a single Vulkan semaphore instance and registers the object in Object Tracker. */
        static std::shared_ptr<Anvil::Semaphore> create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr);

        /** Destructor.
         *
         *  Destroys the Vulkan counterpart and unregisters the wrapper instance from the Object Tracker.
         **/
        virtual ~Semaphore();

        /** Retrieves a raw handle to the underlying Vulkan semaphore instance  */
        VkSemaphore get_semaphore() const
        {
            return m_semaphore;
        }

        /** Retrieves a pointer to a raw handle of the underlying Vulkan semaphore instance */
        const VkSemaphore* get_semaphore_ptr() const
        {
            return &m_semaphore;
        }

        /** Releases the underlying Vulkan Semaphore instance and creates a new Vulkan object. */
        void reset();


    private:
        /* Private functions */

        /* Constructor. Please see create() for specification */
        Semaphore(std::weak_ptr<Anvil::BaseDevice> in_device_ptr);

        Semaphore           (const Semaphore&);
        Semaphore& operator=(const Semaphore&);

        void release_semaphore();

        /* Private variables */
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        VkSemaphore                      m_semaphore;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SEMAPHORE_H */