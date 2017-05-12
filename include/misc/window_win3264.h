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

/** Implements a simple window wrapper for Windows environments.
 *
 *  NOTE: This wrapper does not support scaling (yet).
 **/
#ifndef WINDOW_WIN3264_H
#define WINDOW_WIN3264_H

#include "../misc/window.h"

namespace Anvil
{
    class WindowWin3264 : public Window
    {
    public:
        /* Public functions */

        /* Creates a window wrapper instance by opening a new system window.
         *
         * NOTE: This function resets that last system error assigned to the calling thread.
         *
         * @param title                          Title to use for the window.
         * @param width                          New window's width. Must not be 0.
         * @param height                         New window's height. Must not be 0.
         * @param present_callback_func_ptr      Func pointer to a function which is going to render frame contents to
         *                                       the swapchain image. Must not be null.
         * @param present_callback_func_user_arg User-specific argument to be passed with @param present_callback_func_ptr
         *                                       invocation. May be null.
         *
         * @return New Anvil::Window instance if successful, or null otherwise.
         */
        static std::shared_ptr<Anvil::Window> create(const std::string&     title,
                                                     unsigned int           width,
                                                     unsigned int           height,
                                                     PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                                                     void*                  present_callback_func_user_arg);

        /* Creates a window wrapper instance from an existing window handle.
         *
         * It is assumed that:
         * 1) the application is going to run the message pump on its own.
         * 2) the application is going to explicitly call the presentation callback function at expose/paint/etc. system requests.
         * 3) the application only needs the wrapper instance for interaction with other Anvil wrappers (such as swapchains).
         *
         *
         * @param window_handle Existing, valid window handle.
         *
         * @return New Anvil::Window instance if successful, or null otherwise.
         */
        static std::shared_ptr<Anvil::Window> create(HWND window_handle);

        virtual ~WindowWin3264(){ /* Stub */ }

        virtual void close();
        virtual void run();

        /* Returns window's platform */
        WindowPlatform get_platform() const
        {
            return WINDOW_PLATFORM_SYSTEM;
        }

        /* This function should never be called under Windows */
        virtual void* get_connection() const
        {
            anvil_assert(0);

            return nullptr;
        }

        /** Changes the window title.
         *
         *  @param new_title Null-terminated string, holding the new title. Must not be NULL.
         */
        void set_title(const char* new_title);

    private:
        /* Private functions */

        WindowWin3264(const std::string&     title,
                      unsigned int           width,
                      unsigned int           height,
                      PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                      void*                  present_callback_func_user_arg);
        WindowWin3264(HWND                   handle,
                      const std::string&     title,
                      unsigned int           width,
                      unsigned int           height,
                      PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                      void*                  present_callback_func_user_arg);

        /** Creates a new system window and prepares it for usage. */
        bool init();

        static LRESULT CALLBACK msg_callback_pfn_proc(HWND   window_handle,
                                                      UINT   message_id,
                                                      WPARAM param_wide,
                                                      LPARAM param_long);

        /* Private variables */
    };
}; /* namespace Anvil */

#endif /* WINDOW_WIN3264_H */

