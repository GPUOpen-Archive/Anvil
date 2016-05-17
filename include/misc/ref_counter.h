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

#ifndef MISC_REF_COUNTER_H
#define MISC_REF_COUNTER_H

#include "misc/types.h"


namespace Anvil
{
    /* Provides reference counting support for inheriting classes */
    class RefCounterSupportProvider
    {
    public:
        /** Constructor. Sets the reference counter at 1. */
        RefCounterSupportProvider()
            :m_ref_counter(1)
        {
            /* Stub */
        }

        /** Resets the underlying reference counter and releases the object. */
        void force_release()
        {
            m_ref_counter = 0;

            delete this;
        }

        /** Returns the reference counter value. */
        uint32_t get_ref_counter() const
        {
            return m_ref_counter;
        }

        /** Decrements the internal reference counter. If the ref counter drops to 0,
         *  the object is deleted.
         *
         *  NOTE: This function is NOT thread-safe.
         **/
        void release()
        {
            --m_ref_counter;

            if (m_ref_counter == 0)
            {
                delete this;
            }
        }

        /** Increments the internal reference counter.
         *
         *  NOTE: This function is NOT thread-safe.
         **/
        void retain()
        {
            ++m_ref_counter;
        }


    protected:
        /* Private functions */

        /** Stub virtual destructor. */
        virtual ~RefCounterSupportProvider()
        {
            /* Stub */
        }

    private:
        /* Private variables */
        uint32_t m_ref_counter;
    };
}; /* namespace Anvil */

#endif /* MISC_REF_COUNTER_H */