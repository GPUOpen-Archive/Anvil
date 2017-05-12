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

#ifdef _WIN32
    #include "misc/window_win3264.h"
#else
    #include "misc/window_xcb.h"
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
         * @param platform                       Window platform to use. See WindowPlatform documentation
         *                                       for more details.
         * @param title                          Title to use for the new window.
         * @param width                          Width of the new window.
         * @param height                         Height of the new window.
         * @param present_callback_func_ptr      Callback function to use for rendering frame contents.
         * @param present_callback_func_user_arg User argument to pass with the callback function invocation.
         *
         * @return A new Window wrapper instance if successful, null otherwise.
         **/
        static std::shared_ptr<Window> create_window(WindowPlatform         platform,
                                                     const std::string&     title,
                                                     unsigned int           width,
                                                     unsigned int           height,
                                                     PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                                                     void*                  present_callback_func_user_arg);

        /* Creates a Window wrapper instance using app-managed window handle.
         *
         * NOTE: The following restrictions apply:
         *
         * 1) the application is going to run the message pump on its own.
         * 2) the application is going to explicitly call the presentation callback function at expose/paint/etc. system requests.
         * 3) the application only needs the wrapper instance for interaction with other Anvil wrappers (such as swapchains).
         *    This means that the app should not call any of the window wrapper functions if this platform is used.
         *
         * @param platform       Window platform to use. Must NOT be one of the dummy window platforms.
         * @param handle         Valid, existing window handle.
         * @param xcb_connection Only relevant for XCB window platform; Pointer to xcb_connection_t which owns the window.
         */
        static std::shared_ptr<Window> create_window(WindowPlatform platform,
                                                     WindowHandle   handle,
                                                     void*          xcb_connection_ptr = nullptr);

    };
}; /* namespace Anvil */

#endif /* WINDOW_FACTORY_H */