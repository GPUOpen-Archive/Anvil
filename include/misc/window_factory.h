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

/** Implements a simple window factory. **/
#ifndef WINDOW_FACTORY_H
#define WINDOW_FACTORY_H

#include "config.h"


#ifdef _WIN32
    #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        #include "misc/window_win3264.h"
    #endif
#else
    #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        #include "misc/window_xcb.h"
    #endif
#endif /* _WIN32 */

#include "misc/dummy_window.h"

namespace Anvil
{
    class WindowFactory
    {
    public:
        /* Public functions */

        /* Creates a Window wrapper instance by opening a new system window.
         *
         * @param in_platform                       Window platform to use. See WindowPlatform documentation
         *                                          for more details.
         * @param in_title                          Title to use for the new window.
         * @param in_width                          Width of the new window.
         * @param in_height                         Height of the new window.
         * @param in_present_callback_func_ptr      Callback function to use for rendering frame contents.
         * @param in_present_callback_func_user_arg User argument to pass with the callback function invocation.
         *
         * @return A new Window wrapper instance if successful, null otherwise.
         **/
        static std::shared_ptr<Window> create_window(WindowPlatform         in_platform,
                                                     const std::string&     in_title,
                                                     unsigned int           in_width,
                                                     unsigned int           in_height,
                                                     PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                                                     void*                  in_present_callback_func_user_arg);

        /* Creates a Window wrapper instance using app-managed window handle.
         *
         * NOTE: The following restrictions apply:
         *
         * 1) the application is going to run the message pump on its own.
         * 2) the application is going to explicitly call the presentation callback function at expose/paint/etc. system requests.
         * 3) the application only needs the wrapper instance for interaction with other Anvil wrappers (such as swapchains).
         *    This means that the app should not call any of the window wrapper functions if this platform is used.
         *
         * @param in_platform       Window platform to use. Must NOT be one of the dummy window platforms.
         * @param in_handle         Valid, existing window handle.
         * @param in_xcb_connection Only relevant for XCB window platform; Pointer to xcb_connection_t which owns the window.
         */
        static std::shared_ptr<Window> create_window(WindowPlatform in_platform,
                                                     WindowHandle   in_handle,
                                                     void*          in_xcb_connection_ptr = nullptr);

    };
}; /* namespace Anvil */

#endif /* WINDOW_FACTORY_H */