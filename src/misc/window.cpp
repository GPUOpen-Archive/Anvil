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

#include "misc/debug.h"
#include "misc/window.h"

/** Please see header for specification */
Anvil::Window::Window(const std::string&     title,
                      unsigned int           width,
                      unsigned int           height,
                      PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                      void*                  present_callback_func_user_arg)
    :CallbacksSupportProvider        (WINDOW_CALLBACK_ID_COUNT),
     m_height                        (height),
     m_present_callback_func_ptr     (present_callback_func_ptr),
     m_present_callback_func_user_arg(present_callback_func_user_arg),
     m_title                         (title),
     m_width                         (width),
     m_window_should_close           (false),
     m_connection_ptr                (nullptr)

{
    /* Stub */
}

/** Destructor */
Anvil::Window::~Window()
{
    /* Stub */
}

