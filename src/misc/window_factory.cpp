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

#include "misc/window_factory.h"

Anvil::Window* Anvil::WindowFactory::create_window(WindowPlatform         platform,
                                                   const std::string&     title,
                                                   unsigned int           width,
                                                   unsigned int           height,
                                                   PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                                                   void*                  present_callback_func_user_arg,
                                                   bool                   is_dummy)
{
    if (is_dummy)
    {
        return new Anvil::DummyWindow(title,
                                      width,
                                      height,
                                      present_callback_func_ptr,
                                      present_callback_func_user_arg);
    }
    else
    #ifdef _WIN32
    {
        anvil_assert(WINDOW_PLATFORM_SYSTEM == platform);

        return new Anvil::WindowWin3264(title,
                                        width,
                                        height,
                                        present_callback_func_ptr,
                                        present_callback_func_user_arg);
    }
    #else
    {
        switch (platform)
        {
            case WINDOW_PLATFORM_XCB:
            {
                return new Anvil::WindowXcb(title,
                                            width,
                                            height,
                                            present_callback_func_ptr,
                                            present_callback_func_user_arg);

                break;
            }

            case WINDOW_PLATFORM_XLIB:
            case WINDOW_PLATFORM_WAYLAND:
            default:
            {
                anvil_assert(0);
            }
        }

        return nullptr;
    }
    #endif /* _WIN32 */
}