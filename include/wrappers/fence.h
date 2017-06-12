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

/** Implements a wrapper for a single Vulkan fence. Implemented in order to:
 *
 *  - simplify life-time management of fences.
 *  - simplify fence usage.
 *  - let ObjectTracker detect leaking fence instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_FENCE_H
#define WRAPPERS_FENCE_H

#include "misc/debug_marker.h"
#include "misc/types.h"

namespace Anvil
{
    /* Wrapper class for Vulkan fences */
    class Fence : public DebugMarkerSupportProvider<Fence>
    {
    public:
        /* Public functions */

        /** Destructor.
         *
         *  Destroys the Vulkan counterpart and unregister the wrapper instance from the object tracker.
         **/
        virtual ~Fence();

        /** Creates a new Fence instance.
         *
         *  Creates a single Vulkan fence instance and registers the object in Object Tracker.
         *
         *  @param in_device_ptr       Device to use.
         *  @param in_create_signalled true if the fence should be created as a signalled entity.
         *                             False to make it unsignalled at creation time.
         */
        static std::shared_ptr<Fence> create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                      bool                                    in_create_signalled);

        /** Retrieves a raw handle to the underlying Vulkan fence instance */
        VkFence get_fence() const
        {
            return m_fence;
        }

        /** Retrieves a pointer to the raw handle to the underlying Vulkan fence instance */
        const VkFence* get_fence_ptr()
        {
            m_possibly_set = true;

            return &m_fence;
        }

        /** Tells whether the fence is signalled at the time of the call.
         *
         *  @return true if the fence is set, false otherwise.
         **/
        bool is_set() const;

        /** Resets the specified Vulkan Fence, if set. If the fence is not set, this function is a nop.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool reset();

        /** Resets the specified number of Vulkan fences.
         *
         *  This function is expected to be more efficient than calling reset() for @param in_n_fences
         *  times, assuming @param in_n_fences is larger than 1.
         *
         *  @param in_n_fences Number of Fence instances accessible under @param in_fences.
         *  @param in_fences   An array of @param in_n_fences Fence instances to reset. Must not be nullptr,
         *                     unless @param in_n_fences is 0.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        static bool reset_fences(const uint32_t in_n_fences,
                                 Fence*         in_fences);

    private:
        /* Private functions */

        /** Constructor.
         *
         *  Please see documentation of create() for specification
         */
        Fence(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
              bool                             in_create_signalled);

        Fence           (const Fence&);
        Fence& operator=(const Fence&);

        void release_fence();

        /* Private variables */
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        VkFence                          m_fence;
        bool                             m_possibly_set;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_FENCE_H */