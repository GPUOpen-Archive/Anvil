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

std::shared_ptr<Anvil::Window> Anvil::WindowFactory::create_window(WindowPlatform         in_platform,
                                                                   const std::string&     in_title,
                                                                   unsigned int           in_width,
                                                                   unsigned int           in_height,
                                                                   PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                                                                   void*                  in_present_callback_func_user_arg)
{
    std::shared_ptr<Anvil::Window> result_ptr;

    switch (in_platform)
    {
        case WINDOW_PLATFORM_DUMMY:
        {
            result_ptr = Anvil::DummyWindow::create(in_title,
                                                    in_width,
                                                    in_height,
                                                    in_present_callback_func_ptr,
                                                    in_present_callback_func_user_arg);

            break;
        }

        case WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS:
        {
            result_ptr = Anvil::DummyWindowWithPNGSnapshots::create(in_title,
                                                                    in_width,
                                                                    in_height,
                                                                    in_present_callback_func_ptr,
                                                                    in_present_callback_func_user_arg);

            break;
        }

#ifdef _WIN32

        #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
            case WINDOW_PLATFORM_SYSTEM:
            {
                result_ptr = Anvil::WindowWin3264::create(in_title,
                                                          in_width,
                                                          in_height,
                                                          in_present_callback_func_ptr,
                                                          in_present_callback_func_user_arg);

                break;
            }
        #endif
#else
        #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
            case WINDOW_PLATFORM_XCB:
            {
                result_ptr = Anvil::WindowXcb::create(in_title,
                                                      in_width,
                                                      in_height,
                                                      in_present_callback_func_ptr,
                                                      in_present_callback_func_user_arg);

                break;
            }
        #endif

        case WINDOW_PLATFORM_XLIB:
        case WINDOW_PLATFORM_WAYLAND:
            /* Fall-back - TODO */

#endif /* !_WIN32 */

        default:
        {
            /* TODO */
            anvil_assert_fail();
        }
    }

    return result_ptr;
}

std::shared_ptr<Anvil::Window> Anvil::WindowFactory::create_window(WindowPlatform in_platform,
                                                                   WindowHandle   in_handle,
                                                                   void*          in_xcb_connection_ptr)
{
    std::shared_ptr<Anvil::Window> result_ptr;

    /* NOTE: These arguments may not be used at all, depending on ANVIL_INCLUDE_*_WINDOW_SYSTEM_SUPPORT configuration */
    ANVIL_REDUNDANT_ARGUMENT(in_handle);
    ANVIL_REDUNDANT_ARGUMENT(in_xcb_connection_ptr);

    #ifdef _WIN32
    {
        ANVIL_REDUNDANT_ARGUMENT(in_xcb_connection_ptr);
    }
    #endif

    switch (in_platform)
    {
#ifdef _WIN32

        #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
            case WINDOW_PLATFORM_SYSTEM:
            {
                result_ptr = Anvil::WindowWin3264::create(in_handle);

                break;
            }
        #endif

#else
        #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
            case WINDOW_PLATFORM_XCB:
            {
                result_ptr = Anvil::WindowXcb::create(static_cast<xcb_connection_t*>(in_xcb_connection_ptr),
                                                      in_handle);

                break;
            }
        #endif

        case WINDOW_PLATFORM_XLIB:
        case WINDOW_PLATFORM_WAYLAND:
            /* Fall-back - TODO */

#endif /* !_WIN32 */

        case WINDOW_PLATFORM_DUMMY:
        case WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS:
        default:
        {
            anvil_assert_fail();
        }
    }

    return result_ptr;
}