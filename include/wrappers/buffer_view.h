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

/** Implements a wrapper for a single Vulkan buffer view. Implemented in order to:
 *
 *  - simplify debugging, life-time management and usage of buffer views.
 *  - let ObjectTracker detect leaking buffer view instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_BUFFER_VIEW_H
#define WRAPPERS_BUFFER_VIEW_H

#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    /** Wrapper class for Vulkan buffer views */
    class BufferView : public DebugMarkerSupportProvider<BufferView>,
                       public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Creates a single Vulkan buffer view instance and registers the object in Object Tracker.
         *  For argument documentation, please see Vulkan API specification. */
        static BufferViewUniquePtr create(Anvil::BufferViewCreateInfoUniquePtr in_create_info_ptr);

        /** Destructor */
        virtual ~BufferView();

        /** Retrieves a raw Vulkan handle for the underlying VkBufferView instance. */
        VkBufferView get_buffer_view() const
        {
            return m_buffer_view;
        }

        /** Retrieves a pointer to the raw Vulkan handle for the underlying VkBufferView instance. */
        const VkBufferView* get_buffer_view_ptr() const
        {
            return &m_buffer_view;
        }

        const Anvil::BufferViewCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

    private:
        /* Private functions */
        BufferView(Anvil::BufferViewCreateInfoUniquePtr in_create_info_ptr);

        bool init();

        /* Private variables */
        VkBufferView                         m_buffer_view;
        Anvil::BufferViewCreateInfoUniquePtr m_create_info_ptr;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(BufferView);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(BufferView);
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_EVENT_H */