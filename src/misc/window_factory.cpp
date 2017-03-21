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

#include "misc/window_factory.h"

std::shared_ptr<Anvil::Window> Anvil::WindowFactory::create_window(WindowPlatform         platform,
                                                                   const std::string&     title,
                                                                   unsigned int           width,
                                                                   unsigned int           height,
                                                                   PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                                                                   void*                  present_callback_func_user_arg)
{
    std::shared_ptr<Anvil::Window> result;

    switch (platform)
    {
        case WINDOW_PLATFORM_DUMMY:
        {
            result.reset(new Anvil::DummyWindow(title,
                                                width,
                                                height,
                                                present_callback_func_ptr,
                                                present_callback_func_user_arg));

            break;
        }

        case WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS:
        {
            result.reset(new Anvil::DummyWindowWithPNGSnapshots(title,
                                                                width,
                                                                height,
                                                                present_callback_func_ptr,
                                                                present_callback_func_user_arg));

            break;
        }

#ifdef _WIN32

        case WINDOW_PLATFORM_SYSTEM:
        {
            anvil_assert(WINDOW_PLATFORM_SYSTEM == platform);

            result.reset(new Anvil::WindowWin3264(title,
                                                  width,
                                                  height,
                                                  present_callback_func_ptr,
                                                  present_callback_func_user_arg));

            break;
        }

#else
        case WINDOW_PLATFORM_XCB:
        {
            result.reset(new Anvil::WindowXcb(title,
                                              width,
                                              height,
                                              present_callback_func_ptr,
                                              present_callback_func_user_arg));

            break;
        }

        case WINDOW_PLATFORM_XLIB:
        case WINDOW_PLATFORM_WAYLAND:
            /* Fall-back - TODO */

#endif /* !_WIN32 */

        default:
        {
            /* TODO */
            anvil_assert(0);
        }
    }

    return result;
}