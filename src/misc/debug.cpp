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

#include "misc/debug.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void default_assertion_failure_handler(const char*  filename,
                                              unsigned int line,
                                              const char*  message);


Anvil::PFNASSERTIONFAILEDCALLBACKPROC g_anvil_assertion_check_failed_func_ptr = default_assertion_failure_handler;


/** Please see header for specification */
void default_assertion_failure_handler(const char*  filename,
                                       unsigned int line,
                                       const char*  message)
{
    fprintf(stderr,
             "Assertion failed in [%s:%d]: %s",
             filename,
             line,
             message);
    fflush  (stderr);

    exit(-1);
}

/** Please see header for specification */
void Anvil::on_assertion_failed(const char*  filename,
                                unsigned int line,
                                const char*  message)
{
    #if defined(_WIN32) && defined(_DEBUG)
    {
        __debugbreak();
    }
    #endif

    g_anvil_assertion_check_failed_func_ptr(filename,
                                            line,
                                            message);
}

/** Please see header for specification */
void Anvil::set_assertion_failure_handler(Anvil::PFNASSERTIONFAILEDCALLBACKPROC new_callback_func_ptr)
{
    g_anvil_assertion_check_failed_func_ptr = new_callback_func_ptr;
}