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

/*  Implements an utility which returns high-performance, monotonic time data. **/
#ifndef MISC_TIME_H
#define MISC_TIME_H

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <time.h>
#endif

#include <stdint.h>


namespace Anvil
{
    class Time
    {
    public:
        /* Public functions */
         Time();
        ~Time();

        uint64_t get_time_in_msec();

    private:
        /* Private fields */
        #ifdef _WIN32
            LARGE_INTEGER m_frequency;
            LARGE_INTEGER m_start_time;
        #else
            uint64_t m_start_time;
        #endif
    };
}; /* namespace Anvil */

#endif /* MISC_TIME_H */