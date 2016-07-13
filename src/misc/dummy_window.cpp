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

#include "misc/dummy_window.h"

/** Please see header for specification */
Anvil::DummyWindow::DummyWindow(const std::string&     title,
                                unsigned int           width,
                                unsigned int           height,
                                PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                                void*                  present_callback_func_user_arg)
    : Window(title,
             width,
             height,
             present_callback_func_ptr,
             present_callback_func_user_arg)
{
    init();
}

/** Please see header for specification */
void Anvil::DummyWindow::close()
{
    if (!m_window_should_close)
    {
        m_window_should_close = true;
    }
}

/** Creates a new system window and prepares it for usage. */
void Anvil::DummyWindow::init()
{
    /* Stub */
}

/* Please see header for specification */
void Anvil::DummyWindow::run()
{
    bool running = true;

    while (running && !m_window_should_close)
    {
        m_present_callback_func_ptr(m_present_callback_func_user_arg);

        running = !m_window_should_close;
    }
}

