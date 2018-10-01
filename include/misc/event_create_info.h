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
#ifndef MISC_EVENT_CREATE_INFO_H
#define MISC_EVENT_CREATE_INFO_H

#include "misc/types.h"

namespace Anvil
{
    class EventCreateInfo
    {
    public:
        /* Public functions */

        /* TODO.
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety: Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         */
        static Anvil::EventCreateInfoUniquePtr create(const Anvil::BaseDevice* in_device_ptr);

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        const MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }


        void set_device(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        void set_mt_safety(const MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

    private:
        /* Private functions */
        EventCreateInfo(const Anvil::BaseDevice* in_device_ptr,
                        MTSafety                 in_mt_safety);

        /* Private variables */
        const Anvil::BaseDevice* m_device_ptr;
        Anvil::MTSafety          m_mt_safety;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(EventCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(EventCreateInfo);
    };

}; /* namespace Anvil */

#endif /* MISC_EVENT_CREATE_INFO_H */