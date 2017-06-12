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

/** Implements a wrapper for a single Vulkan buffer view. Implemented in order to:
 *
 *  - simplify debugging, life-time management and usage of buffer views.
 *  - let ObjectTracker detect leaking buffer view instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_BUFFER_VIEW_H
#define WRAPPERS_BUFFER_VIEW_H

#include "misc/types.h"

namespace Anvil
{
    /** Wrapper class for Vulkan buffer views */
    class BufferView : public DebugMarkerSupportProvider<BufferView>
    {
    public:
        /* Public functions */

        /** Creates a single Vulkan buffer view instance and registers the object in Object Tracker.
         *  For argument documentation, please see Vulkan API specification. */
        static std::shared_ptr<BufferView> create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                  std::shared_ptr<Anvil::Buffer>   in_buffer_ptr,
                                                  VkFormat                         in_format,
                                                  VkDeviceSize                     in_start_offset,
                                                  VkDeviceSize                     in_size);

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

        /** Returns format used by the buffer view */
        VkFormat get_format() const
        {
            return m_format;
        }

        /** Returns pointer to the parent buffer instance */
        std::shared_ptr<Anvil::Buffer> get_parent_buffer() const
        {
            return m_buffer_ptr;
        }

        /** Returns size of the encapsulated buffer memory region */
        VkDeviceSize get_size() const
        {
            return m_size;
        }

        /** Returns start offset of the encapsulated buffer memory region */
        VkDeviceSize get_start_offset() const
        {
            return m_start_offset;
        }

    private:
        /* Private functions */
        BufferView(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                   std::shared_ptr<Anvil::Buffer>   in_buffer_ptr,
                   VkFormat                         in_format,
                   VkDeviceSize                     in_start_offset,
                   VkDeviceSize                     in_size);

        BufferView           (const BufferView&);
        BufferView& operator=(const BufferView&);

        /* Private variables */
        std::shared_ptr<Anvil::Buffer>   m_buffer_ptr;
        VkBufferView                     m_buffer_view;
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        VkFormat                         m_format;
        VkDeviceSize                     m_size;
        VkDeviceSize                     m_start_offset;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_EVENT_H */