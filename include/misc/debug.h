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

/** Defines debug-related macros and function prototypes used by Anvil */
#ifndef MISC_DEBUG_H
#define MISC_DEBUG_H

#include <functional>


#if defined(_DEBUG)
    #define anvil_assert(assertion)                  \
        if (!(assertion))                            \
        {                                            \
            Anvil::on_assertion_failed(__FILE__,     \
                                        __LINE__,    \
                                        #assertion); \
        }

    #define anvil_assert_fail()                                      \
        Anvil::on_assertion_failed(__FILE__,                         \
                                   __LINE__,                         \
                                   "Unexpected condition detected");
#else
    #define anvil_assert(assertion)
    #define anvil_assert_fail()
#endif


#define is_vk_call_successful(result) \
    (result == VK_SUCCESS || result == VK_ERROR_VALIDATION_FAILED_EXT || result == VK_INCOMPLETE)

#define anvil_assert_vk_call_succeeded(result) \
    anvil_assert( is_vk_call_successful(result) )


namespace Anvil
{
    /** Function prototype of an assertion failure handler.
     *
     *  @param in_filename File, from which the assertion failure originated.
     *  @param in_line     Line index
     *  @param in_message  Null-terminated text string holding the tokenized condition which failed.
     **/
    typedef std::function< void(const char*  in_filename,
                                unsigned int in_line,
                                const char*  in_message)> AssertionFailedCallbackFunction;

    /** Assertion failure interceptor.
     *
     *  Calls the default or the user-specified assertion failure handler (if one was defined
     *  by the app with a set_assertion_failure_handler() invocation).
     *
     *  @param in_filename File, from which the assertion failure originated.
     *  @param in_line     Line index
     *  @param in_message  Null-terminated text string holding the tokenized condition which failed.
     **/
    void on_assertion_failed(const char*  in_filename,
                             unsigned int in_line,
                             const char*  in_message);

    /** Modifies the assertion failure handler entry-point, which is going to be used by Anvil in case
     *  an assertion failure occurs.
     *
     *  @param in_new_callback_func New entry-point to use. Must not be nullptr.
     *
     **/
    void set_assertion_failure_handler(AssertionFailedCallbackFunction in_new_callback_func);
};

#endif /* MISC_DEBUG_H */
