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

#include "misc/window_win3264.h"

#define WM_DESTROY_WINDOW (WM_USER + 1)


/* See create() for documentation */
Anvil::WindowWin3264::WindowWin3264(const std::string&     in_title,
                                    unsigned int           in_width,
                                    unsigned int           in_height,
                                    PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                                    void*                  in_present_callback_func_user_arg)
    :Window(in_title,
            in_width,
            in_height,
            in_present_callback_func_ptr,
            in_present_callback_func_user_arg)
{
    m_window_owned = true;
}

/* See create() for documentation */
Anvil::WindowWin3264::WindowWin3264(HWND                   in_handle,
                                    const std::string&     in_title,
                                    unsigned int           in_width,
                                    unsigned int           in_height,
                                    PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                                    void*                  in_present_callback_func_user_arg)
    :Window(in_title,
            in_width,
            in_height,
            in_present_callback_func_ptr,
            in_present_callback_func_user_arg)
{
    m_window       = in_handle;
    m_window_owned = false;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Window> Anvil::WindowWin3264::create(const std::string&     in_title,
                                                            unsigned int           in_width,
                                                            unsigned int           in_height,
                                                            PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                                                            void*                  in_present_callback_func_user_arg)
{
    std::shared_ptr<Anvil::WindowWin3264> result_ptr(
        new Anvil::WindowWin3264(in_title,
                                 in_width,
                                 in_height,
                                 in_present_callback_func_ptr,
                                 in_present_callback_func_user_arg)
    );

    if (result_ptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Window> Anvil::WindowWin3264::create(HWND in_window_handle)
{
    std::shared_ptr<Anvil::WindowWin3264> result_ptr;
    RECT                                  window_rect;
    uint32_t                              window_size[2] = {0};
    std::vector<char>                     window_title;
    uint32_t                              window_title_length = 0;

    /* The window has already been spawned by the user. Gather all the info we need in order to instantiate
     * the wrapper instance.
     */
    if (::IsWindow(in_window_handle) == 0)
    {
        anvil_assert_fail();

        goto end;
    }

    if (::GetClientRect(in_window_handle,
                       &window_rect) == 0)
    {
        anvil_assert_fail();

        goto end;
    }

    window_size[0] = static_cast<uint32_t>(window_rect.right  - window_rect.left);
    window_size[1] = static_cast<uint32_t>(window_rect.bottom - window_rect.top);

    window_title_length = static_cast<uint32_t>(::GetWindowTextLength(in_window_handle) );

    if (window_title_length != 0)
    {
        window_title.resize(window_title_length);

        ::GetWindowText(in_window_handle,
                        static_cast<LPSTR>(&window_title.at(0) ),
                        static_cast<int>  (window_title_length) );
    }

    /* Go ahead and create the window wrapper instance */
    result_ptr.reset(
        new Anvil::WindowWin3264(in_window_handle,
                                 std::string(&window_title.at(0) ),
                                 window_size[0],
                                 window_size[1],
                                 nullptr, /* present_callback_func_ptr      */
                                 nullptr) /* present_callback_func_user_arg */
    );

    if (result_ptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

end:
    return result_ptr;
}

/** Please see header for specification */
void Anvil::WindowWin3264::close()
{
    anvil_assert(m_window_owned);

    if (!m_window_should_close)
    {
        m_window_should_close = true;

        /* NOTE: When the call below leaves, the window is guaranteed to be gone */
        ::SendMessage(m_window,
                      WM_DESTROY_WINDOW,
                      0,  /* wParam */
                      0); /* lParam */
    }
}

/** Creates a new system window and prepares it for usage. */
bool Anvil::WindowWin3264::init()
{
    bool result = false;

    if (m_window_owned)
    {
        const uint32_t window_size[2] =
        {
            m_width,
            m_height
        };

        static const char* class_name  = "Anvil window class";
        HINSTANCE          instance    = ::GetModuleHandle(nullptr);
        WNDCLASSEX         window_class;
        RECT               window_rect;

        // Initialize the window class structure:
        window_class.cbSize        = sizeof(WNDCLASSEX);
        window_class.style         = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc   = msg_callback_pfn_proc;
        window_class.cbClsExtra    = 0;
        window_class.cbWndExtra    = 0;
        window_class.hbrBackground = static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH) );
        window_class.hInstance     = instance;
        window_class.hIcon         = ::LoadIcon  (nullptr,  /* hInstance */
                                                  IDI_APPLICATION);
        window_class.hCursor       = ::LoadCursor(nullptr,  /* hInstance */
                                                  IDC_ARROW);
        window_class.lpszMenuName  = nullptr;
        window_class.lpszClassName = class_name;
        window_class.hIconSm       = ::LoadIcon(nullptr, /* hInstance */
                                                IDI_WINLOGO);

        /* Register window class. If more than one window is instantiated, this call will fail
         * but we don't really care.
         **/
        ::RegisterClassEx(&window_class);

        /* Create the window */
        window_rect.left   = 0;
        window_rect.top    = 0;
        window_rect.right  = static_cast<LONG>(window_size[0]);
        window_rect.bottom = static_cast<LONG>(window_size[1]);

        if (::AdjustWindowRect(&window_rect,
                               WS_OVERLAPPEDWINDOW,
                               FALSE /* bMenu */) == 0)
        {
            anvil_assert_fail();

            goto end;
        }

        /* NOTE: Anvil currently does not support automatic swapchain resizing so make sure the window is not scalable. */
        m_window = ::CreateWindowEx(0,                    /* dwExStyle */
                                    class_name,
                                    m_title.c_str(),
                                    (WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU) ^ WS_THICKFRAME,
                                    0, /* X */
                                    0, /* Y */
                                    window_rect.right - window_rect.left,
                                    window_rect.bottom - window_rect.top,
                                    nullptr, /* hWndParent */
                                    nullptr, /* hMenu */
                                    instance,
                                    nullptr); /* lParam */

        if (m_window == nullptr)
        {
            anvil_assert_fail();

            goto end;
        }
    }

    /* In order to verify if SetWindowLongPtr() has succeeded, we need to check the last thread-local error's value */
    ::SetLastError(0);

    if ((::SetWindowLongPtr(m_window,
                            GWLP_USERDATA,
                            reinterpret_cast<LONG_PTR>(this) ) == 0) &&
        (::GetLastError    ()                                  != 0) )
    {
        anvil_assert_fail();

        goto end;
    }

    result = true;
end:
    return result;
}

/** Window message handler.
 *
 *  @param in_window_handle Window handle.
 *  @param in_message_id    Message ID
 *  @param in_param_wide    Wide window message parameter.
 *  @param in_param_long    Long window message parameter.
 *
 *  @return Window message-specific return value.
 **/
LRESULT CALLBACK Anvil::WindowWin3264::msg_callback_pfn_proc(HWND   in_window_handle,
                                                             UINT   in_message_id,
                                                             WPARAM in_param_wide,
                                                             LPARAM in_param_long)
{
    WindowWin3264* window_ptr = reinterpret_cast<WindowWin3264*>(::GetWindowLongPtr(in_window_handle,
                                                                                    GWLP_USERDATA) );

    switch (in_message_id)
    {
        case WM_DESTROY:
        {
            window_ptr->m_window_should_close = true;

            ::PostQuitMessage(0);

            return 0;
        }

        case WM_DESTROY_WINDOW:
        {
            window_ptr->callback(WINDOW_CALLBACK_ID_ABOUT_TO_CLOSE,
                                 window_ptr);

            ::DestroyWindow(window_ptr->m_window);

            break;
        }

        case WM_KEYUP:
        {
            KeypressReleasedCallbackData callback_data(window_ptr,
                                                       static_cast<Anvil::KeyID>(LOWORD(in_param_wide) & 0xFF) );

            window_ptr->callback(WINDOW_CALLBACK_ID_KEYPRESS_RELEASED,
                                &callback_data);

            return 0;
        }

        default:
        {
            break;
        }
    }

    return DefWindowProc(in_window_handle,
                         in_message_id,
                         in_param_wide,
                         in_param_long);
}

/* Please see header for specification */
void Anvil::WindowWin3264::run()
{
    int done = 0;

    /* This function should only be called for wrapper instances which have created the window! */
    anvil_assert(m_window_owned);

    /* Run the message loop */
    while (!done)
    {
        MSG msg;

        while (::PeekMessage(&msg,
                             nullptr,
                             0,
                             0,
                             PM_REMOVE) )
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage (&msg);
        }

        if (msg.message == WM_QUIT)
        {
            done = 1;
        }

        m_present_callback_func_ptr(m_present_callback_func_user_arg);
    }

    m_window_close_finished = true;
}

void Anvil::WindowWin3264::set_title(const std::string& in_new_title)
{
    /* This function should only be called for wrapper instances which have created the window! */
    anvil_assert(m_window_owned);

    ::SetWindowText(m_window,
                    in_new_title.c_str() );
}
