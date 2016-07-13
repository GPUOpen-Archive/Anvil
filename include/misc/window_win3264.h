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

/** Implements a simple window wrapper for Windows environments.
 *
 *  NOTE: This wrapper does not support scaling (yet).
 **/
#ifndef WINDOW_WIN3264_H
#define WINDOW_WIN3264_H

#include "misc/window.h"

namespace Anvil
{
    class WindowWin3264 : public Window
    {
    public:
        /* Public functions */
        WindowWin3264(const std::string&     title,
                      unsigned int           width,
                      unsigned int           height,
                      PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                      void*                  present_callback_func_user_arg);

        virtual void            close();
        virtual void            run();

        /* Tells if it's a dummy window (offscreen rendering thus no WSI/swapchain involved) */
        virtual bool            is_dummy()
        {
            return false;
        }

        /* This function should never be called under windows */
        virtual void*           get_connection() const
        {
            anvil_assert(0);

            return nullptr;
        }

    private:
        /* Private functions */
        virtual                ~WindowWin3264(){ /* Stub */ }

        /** Creates a new system window and prepares it for usage. */
        void                    init();

        static LRESULT CALLBACK msg_callback_pfn_proc(HWND   window_handle,
                                                      UINT   message_id,
                                                      WPARAM param_wide,
                                                      LPARAM param_long);

        /* Private variables */
    };
}; /* namespace Anvil */

#endif /* WINDOW_WIN3264_H */

