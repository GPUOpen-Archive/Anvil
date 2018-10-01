//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef MISC_BUFFER_VIEW_CREATE_INFO_H
#define MISC_BUFFER_VIEW_CREATE_INFO_H

#include "misc/types.h"


namespace Anvil
{
    class BufferViewCreateInfo
    {
    public:
        /* Public functions */

        /* TODO.
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety: MTSafety::INHERIT_FROM_PARENT_DEVICE
         */
        static Anvil::BufferViewCreateInfoUniquePtr create(const Anvil::BaseDevice* in_device_ptr,
                                                           Anvil::Buffer*           in_buffer_ptr,
                                                           Anvil::Format            in_format,
                                                           VkDeviceSize             in_start_offset,
                                                           VkDeviceSize             in_size);

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        /** Returns format used by the buffer view */
        Anvil::Format get_format() const
        {
            return m_format;
        }

        const Anvil::MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }

        /** Returns pointer to the parent buffer instance */
        Anvil::Buffer* get_parent_buffer() const
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

        void set_device(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        void set_format(const Anvil::Format& in_format)
        {
            m_format = in_format;
        }

        void set_mt_safety(const Anvil::MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        /** Returns pointer to the parent buffer instance */
        void set_parent_buffer(Anvil::Buffer* in_buffer_ptr)
        {
            m_buffer_ptr = in_buffer_ptr;
        }

        void set_size(const VkDeviceSize& in_size)
        {
            m_size = in_size;
        }

        void set_start_offset(const VkDeviceSize& in_start_offset)
        {
            m_start_offset = in_start_offset;
        }

    private:
        /* Private functions */

        BufferViewCreateInfo(const Anvil::BaseDevice* in_device_ptr,
                             Anvil::Buffer*           in_buffer_ptr,
                             Anvil::Format            in_format,
                             VkDeviceSize             in_start_offset,
                             VkDeviceSize             in_size,
                             MTSafety                 in_mt_safety);


        /* Private variables */
        Anvil::Buffer*           m_buffer_ptr;
        const Anvil::BaseDevice* m_device_ptr;
        Anvil::Format            m_format;
        Anvil::MTSafety          m_mt_safety;
        VkDeviceSize             m_size;
        VkDeviceSize             m_start_offset;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(BufferViewCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(BufferViewCreateInfo);
    };
}; /* namespace Anvil */
#endif /* MISC_BUFFER_VIEW_CREATE_INFO_H */