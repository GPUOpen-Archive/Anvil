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

/** Implements a simple window wrapper for Linux (XCB) environments.
 *
 *  NOTE: This wrapper does not support scaling (yet).
 **/
#ifndef WINDOW_XCB_H
#define WINDOW_XCB_H

#include "misc/window.h"

namespace Anvil
{
    class WindowXcb : public Window
    {
    public:
        /* Public functions */
        static std::shared_ptr<Anvil::Window> create(const std::string&     in_title,
                                                     unsigned int           in_width,
                                                     unsigned int           in_height,
                                                     PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                                                     void*                  in_present_callback_func_user_arg);

        static std::shared_ptr<Anvil::Window> create(xcb_connection_t* in_connection_ptr,
                                                     WindowHandle      in_window_handle);

        virtual ~WindowXcb();

        virtual void close();
        virtual void run();

        /* Returns window's platform */
        WindowPlatform get_platform() const
        {
            return WINDOW_PLATFORM_XCB;
        }

        /* Tells if it's a dummy window (offscreen rendering thus no WSI/swapchain involved) */
        virtual bool is_dummy()
        {
            return false;
        }

        /** Returns system XCB connection, should be used by linux only */
        virtual void* get_connection() const
        {
            return m_connection_ptr;
        }

    private:
        WindowXcb(const std::string&     in_title,
                  unsigned int           in_width,
                  unsigned int           in_height,
                  PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                  void*                  in_present_callback_func_user_arg);
        WindowXcb(xcb_connection_t*      in_connection_ptr,
                  WindowHandle           in_window_handle);

        /** Creates a new system window and prepares it for usage. */
        bool init();
        bool init_connection();

        /* Private variables */
        xcb_intern_atom_reply_t* m_atom_wm_delete_window_ptr;
        xcb_connection_t*        m_connection_ptr;
        xcb_screen_t*            m_screen_ptr;
        xcb_key_symbols_t*       m_key_symbols;
        XCBLoader                m_xcb_loader;
    };
}; /* namespace Anvil */

#endif /* WINDOW_XCB_H */