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

/** Implements a simple window wrapper for Windows and Linux (XCB) environments.
 *
 *  NOTE: This wrapper does not support scaling (yet).
 **/
#ifndef MISC_WINDOW_H
#define MISC_WINDOW_H

#include "misc/callbacks.h"
#include "misc/io.h"
#include "misc/ref_counter.h"
#include "misc/types.h"
#include "misc/debug.h"

namespace Anvil
{
    /** Prototype of a function, which renders frame contents & presents it */
    typedef void (*PFNPRESENTCALLBACKPROC)(void* in_user_arg);

    /* Structure passed as a WINDOW_CALLBACK_ID_KEYPRESS_RELEASED call-back argument */
    typedef struct KeypressReleasedCallbackData
    {
        KeyID   released_key_id;
        Window* window_ptr;

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
        /* Call-back issued right before OS is requested to close the window.
         *
         * callback_arg: pointer to the Window instance
         */
        WINDOW_CALLBACK_ID_ABOUT_TO_CLOSE,

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
        /* Stub window implementation - useful for off-screen rendering */
        WINDOW_PLATFORM_DUMMY,

        /* Stub window implementation - useful for off-screen rendering.
         *
         * This dummy window saves each "presented" frame in a PNG file. For that process to be successful,
         * the application MUST ensure the swapchain image is transitioned to VK_IMAGE_LAYOUT_GENERAL before
         * it is presented.
         **/
        WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS,

    #ifdef _WIN32
        /* win32 */
        #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
            WINDOW_PLATFORM_SYSTEM,
        #endif

    #else
        #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
            /* linux xcb */
            WINDOW_PLATFORM_XCB,
        #endif

        /* linux xlib */
        WINDOW_PLATFORM_XLIB,

        /* linux xlib */
        WINDOW_PLATFORM_WAYLAND,
    #endif

        /* Always last */
        WINDOW_PLATFORM_COUNT,
        WINDOW_PLATFORM_UNKNOWN = WINDOW_PLATFORM_COUNT
    };

    class Window : public CallbacksSupportProvider
    {
    public:
        /* Public functions */

        /** Creates a single Window instance and presents it.
         *
         *  When returned execution, the created window will not be functional. In order for it
         *  to become responsive, a dedicated thread should invoke run() to host the message pump.
         *
         *  A window instance should be created in the same thread, from which the run() function
         *  is going to be invoked.
         *
         *  @param in_title                          Name to use for the window's title bar.
         *  @param in_width                          Window's width. Note that this value should not exceed screen's width.
         *  @param in_height                         Window's height. Note that this value should not exceed screen's height.
         *  @param in_present_callback_func_ptr      Call-back to use in order to render & present the updated frame contents.
         *                                           Must not be nullptr.
         *  @param in_present_callback_func_user_arg Argument to pass to the @param in_present_callback_func_ptr call-back. May
         *                                           be nullptr.
         *
         **/
        Window(const std::string&     in_title,
               unsigned int           in_width,
               unsigned int           in_height,
               PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
               void*                  in_present_callback_func_user_arg);

        /** Closes the window and unblocks the thread executing the message pump. */
        virtual void close() { /* Stub */ }

        /** Returns system XCB connection, should be used by linux only */
        virtual void* get_connection() const { return nullptr; }

        /** Returns system window handle. */
        WindowHandle get_handle() const
        {
            return m_window;
        }

        /** Returns window's height, as specified at creation time */
        uint32_t get_height_at_creation_time() const
        {
            return m_height;
        }

        /* Returns window's platform */
        virtual WindowPlatform get_platform() const = 0;

        /* Returns window's width */
        uint32_t get_width_at_creation_time() const
        {
            return m_width;
        }

        /** Makes the window responsive to user's action and starts updating window contents.
         *
         *  This function will *block* the calling thread. To unblock it, call close().
         *
         *  This function can only be called once throughout Window instance's lifetime.
         *  This function can only be called for window instances which have opened a system window.
         *
         **/
        virtual void run() = 0;

        /** Changes the window title.
         *
         *  @param new_title Null-terminated string, holding the new title. Must not be NULL.
         */
        virtual void set_title(const std::string& in_new_title)
        {
            ANVIL_REDUNDANT_ARGUMENT_CONST(in_new_title);

            /* Nop by default */
        }

        /* Tells if the window closure process has finished */
        bool is_window_close_finished()
        {
            return m_window_close_finished;
        }

    protected:
        /* protected variables */
        PFNPRESENTCALLBACKPROC m_present_callback_func_ptr;
        void*                  m_present_callback_func_user_arg;

        unsigned int m_height;
        std::string  m_title;
        unsigned int m_width;
        bool         m_window_should_close;
        bool         m_window_close_finished;

        /* Window handle */
        WindowHandle m_window;
        bool         m_window_owned;

        /* protected functions */
        virtual ~Window();

    private:
        /* Private functions */

    /* Private variables */

    };
}; /* namespace Anvil */

#endif /* MISC_WINDOW_H */