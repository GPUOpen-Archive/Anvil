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

/** Implements a simple window (but without X) wrapper for Linux (XCB) environments.
 *
 *  NOTE: This wrapper does not support scaling (yet).
 **/
#ifndef DUMMY_WINDOW_H
#define DUMMY_WINDOW_H

#include "misc/window.h"

namespace Anvil
{
    class DummyWindow : public Window
    {
    public:
        /* Public functions */
        DummyWindow(const std::string&     title,
                    unsigned int           width,
                    unsigned int           height,
                    PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                    void*                  present_callback_func_user_arg);

        virtual void    close();
        virtual void    run();

        /* Tells if it's a dummy window (offscreen rendering thus no WSI/swapchain involved) */
        virtual bool    is_dummy()
        {
            return true;
        }

        /** Returns system XCB connection, should be used by linux only */
        virtual void*   get_connection() const
        {
            return nullptr;
        }

    private:
        /* Private functions */
        virtual        ~DummyWindow(){ /* Stub */ }

        /** Creates a new system window and prepares it for usage. */
        void            init();
    };
}; /* namespace Anvil */

#endif /* DUMMY_WINDOW_H */