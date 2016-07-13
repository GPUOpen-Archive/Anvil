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

/** Implements a simple window wrapper for Windows and Linux (XCB) environments.
 *
 *  NOTE: This wrapper does not support scaling (yet).
 **/
#ifndef MISC_WINDOW_H
#define MISC_WINDOW_H

#include "misc/callbacks.h"
#include "misc/ref_counter.h"
#include "misc/types.h"
#include "misc/debug.h"

#ifdef _WIN32
    #include <Windows.h>
#endif

namespace Anvil
{
    /** Prototype of a function, which renders frame contents & presents it */
    typedef void (*PFNPRESENTCALLBACKPROC)(void* user_arg);

    /* Structure passed as a WINDOW_CALLBACK_ID_KEYPRESS_RELEASED call-back argument */
    typedef struct KeypressReleasedCallbackData
    {
        Window* window_ptr;
        KeyID   released_key_id;

        /** Constructor.
         *
         *  @param in_command_buffer_ptr  Command buffer instance the command is being recorded for.
         *  @param in_command_details_ptr Structure holding all arguments to be passed to the vkCmdBeginRenderPass() call.
         **/
        explicit KeypressReleasedCallbackData(Window* in_window_ptr,
                                              KeyID   in_released_key_id)
            :released_key_id(in_released_key_id),
             window_ptr     (in_window_ptr)
        {
            /* Stub */
        }
    } KeypressReleasedCallbackData;

    /* Enumerates available window call-back types.*/
    enum WindowCallbackID
    {
        /* Call-back issued when the user releases a pressed key.
         *
         * callback_arg: pointer to a KeypressReleasedCallbackData instance.
         **/
        WINDOW_CALLBACK_ID_KEYPRESS_RELEASED,

        /* Always last */
        WINDOW_CALLBACK_ID_COUNT
    };

    /* Enumerates available window call-back types.*/
    enum WindowPlatform
    {
    #ifdef _WIN32
        /* win32 */
        WINDOW_PLATFORM_SYSTEM,

    #else
        /* linux xcb */
        WINDOW_PLATFORM_XCB,

        /* linux xlib */
        WINDOW_PLATFORM_XLIB,

        /* linux xlib */
        WINDOW_PLATFORM_WAYLAND,
    #endif

        /* Always last */
        WINDOW_PLATFORM_COUNT
    };

    class Window : public CallbacksSupportProvider,
                   public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Creates a single Window instance and presents it.
         *
         *  When returned execution, the created window will not be functional. In order for it
         *  to become responsive, a dedicated thread should invoke run() to host the message pump.
         *
         *  @param title                          Name to use for the window's title bar.
         *  @param width                          Window's width. Note that this value should not exceed screen's width.
         *  @param height                         Window's height. Note that this value should not exceed screen's height.
         *  @param present_callback_func_ptr      Call-back to use in order to render & present the updated frame contents.
         *                                        Must not be nullptr.
         *  @param present_callback_func_user_arg Argument to pass to the @param present_callback_func_ptr call-back. May
         *                                        be nullptr.
         *
         **/
        Window(const std::string&     title,
               unsigned int           width,
               unsigned int           height,
               PFNPRESENTCALLBACKPROC present_callback_func_ptr,
               void*                  present_callback_func_user_arg);


        /** Closes the window and unblocks the thread executing the message pump. */
        virtual void close() { /* Stub */ }

        /** Returns system XCB connection, should be used by linux only */
        virtual void* get_connection() const { return nullptr; }

        /** Returns system window handle. */
        WindowHandle get_handle() const
        {
            return m_window;
        }

        /** Returns window's height */
        uint32_t get_height() const
        {
            return m_height;
        }

        /* Returns window's width */
        uint32_t get_width() const
        {
            return m_width;
        }

        /* Tells if it's a dummy window (offscreen rendering thus no WSI/swapchain involved) */
        virtual bool  is_dummy() = 0;

        /** Makes the window responsive to user's action and starts updating window contents.
         *
         *  This function will *block* the calling thread. To unblock it, call close().
         *
         *  This function can only be called once throughout Window instance's lifetime.
         *
         **/
        virtual void  run()       = 0;

    protected:
        /* protected variables */
        PFNPRESENTCALLBACKPROC m_present_callback_func_ptr;
        void*                  m_present_callback_func_user_arg;

        unsigned int m_height;
        std::string  m_title;
        unsigned int m_width;
        bool         m_window_should_close;

        /* This member should be used by linux only */
        void*        m_connection_ptr;

        /* Window handle */
        WindowHandle m_window;

        /* protected functions */
        virtual ~Window();

    private:
        /* Private functions */

        /* Private variables */

    };

    /** Delete functor. Useful if you need to wrap the event instance in an auto pointer */
    struct WindowDeleter
    {
        void operator()(Window* window_ptr) const
        {
            window_ptr->release();
        }
    };
}; /* namespace Anvil */

#endif /* MISC_WINDOW_H */