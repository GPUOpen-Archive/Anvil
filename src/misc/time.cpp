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

#include "misc/time.h"


/** Please see header for specification */
Anvil::Time::Time()
{
    #ifdef _WIN32
    {
        QueryPerformanceFrequency(&m_frequency);
        QueryPerformanceCounter  (&m_start_time);
    }
    #else
    {
        struct timespec current_timespec;

        clock_gettime(CLOCK_MONOTONIC, &current_timespec);

        m_start_time = static_cast<uint64_t>(1000LL /* SEC_TO_MSEC */ * current_timespec.tv_sec + current_timespec.tv_nsec / 1000000LL /* MSEC_TO_NSEC */);
    }
    #endif
}

/** Please see header for specification */
Anvil::Time::~Time()
{
    /* Stub */
}

/** Please see header for specification */
uint64_t Anvil::Time::get_time_in_msec()
{
    uint64_t result = 0;

    #ifdef _WIN32
    {
        LARGE_INTEGER current_time;

        QueryPerformanceCounter(&current_time);

        result = ((current_time.QuadPart - m_start_time.QuadPart) * 1000 /* ms in s */ / m_frequency.QuadPart);
    }
    #else
    {
        struct timespec current_timespec;

        clock_gettime(CLOCK_MONOTONIC, &current_timespec);

        result = 1000LL /* SEC_TO_MSEC */ * current_timespec.tv_sec + current_timespec.tv_nsec / 1000000LL /* MSEC_TO_NSEC */ - m_start_time;
    }
    #endif

    return result;
}
