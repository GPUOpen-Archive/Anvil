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
Anvil::WindowWin3264::WindowWin3264(const std::string&     title,
                                    unsigned int           width,
                                    unsigned int           height,
                                    PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                                    void*                  present_callback_func_user_arg)
    :Window(title,
            width,
            height,
            present_callback_func_ptr,
            present_callback_func_user_arg)
{
    m_window_owned = true;
}

/* See create() for documentation */
Anvil::WindowWin3264::WindowWin3264(HWND                   handle,
                                    const std::string&     title,
                                    unsigned int           width,
                                    unsigned int           height,
                                    PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                                    void*                  present_callback_func_user_arg)
    :Window(title,
            width,
            height,
            present_callback_func_ptr,
            present_callback_func_user_arg)
{
    m_window       = handle;
    m_window_owned = false;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Window> Anvil::WindowWin3264::create(const std::string&     title,
                                                            unsigned int           width,
                                                            unsigned int           height,
                                                            PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                                                            void*                  present_callback_func_user_arg)
{
    std::shared_ptr<Anvil::WindowWin3264> result_ptr(
        new Anvil::WindowWin3264(title,
                                 width,
                                 height,
                                 present_callback_func_ptr,
                                 present_callback_func_user_arg)
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
std::shared_ptr<Anvil::Window> Anvil::WindowWin3264::create(HWND window_handle)
{
    std::shared_ptr<Anvil::WindowWin3264> result_ptr;
    RECT                                  window_rect;
    uint32_t                              window_size[2] = {0};
    std::vector<char>                     window_title;
    uint32_t                              window_title_length = 0;

    /* The window has already been spawned by the user. Gather all the info we need in order to instantiate
     * the wrapper instance.
     */
    if (::IsWindow(window_handle) == 0)
    {
        anvil_assert(false);

        goto end;
    }

    if (::GetClientRect(window_handle,
                       &window_rect) == 0)
    {
        anvil_assert(false);

        goto end;
    }

    window_size[0] = static_cast<uint32_t>(window_rect.right  - window_rect.left);
    window_size[1] = static_cast<uint32_t>(window_rect.bottom - window_rect.top);

    window_title_length = static_cast<uint32_t>(::GetWindowTextLength(window_handle) );

    if (window_title_length != 0)
    {
        window_title.resize(window_title_length);

        ::GetWindowText(window_handle,
                        static_cast<LPSTR>(&window_title.at(0) ),
                        static_cast<int>  (window_title_length) );
    }

    /* Go ahead and create the window wrapper instance */
    result_ptr.reset(
        new Anvil::WindowWin3264(window_handle,
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
            anvil_assert(false);

            goto end;
        }

        m_window = ::CreateWindowEx(0,                    /* dwExStyle */
                                    class_name,
                                    m_title.c_str(),
                                    WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU,
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
            anvil_assert(false);

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
        anvil_assert(false);

        goto end;
    }

    result = true;
end:
    return result;
}

/** Window message handler.
 *
 *  @param window_handle Window handle.
 *  @param message_id    Message ID
 *  @param param_wide    Wide window message parameter.
 *  @param param_long    Long window message parameter.
 *
 *  @return Window message-specific return value.
 **/
LRESULT CALLBACK Anvil::WindowWin3264::msg_callback_pfn_proc(HWND   window_handle,
                                                             UINT   message_id,
                                                             WPARAM param_wide,
                                                             LPARAM param_long)
{
    WindowWin3264* window_ptr = reinterpret_cast<WindowWin3264*>(::GetWindowLongPtr(window_handle,
                                                                                    GWLP_USERDATA) );

    switch (message_id)
    {
        case WM_DESTROY:
        {
            window_ptr->m_window_should_close = true;

            ::PostQuitMessage(0);

            return 0;
        }

        case WM_DESTROY_WINDOW:
        {
            ::DestroyWindow(window_ptr->m_window);

            break;
        }

        case WM_KEYUP:
        {
            KeypressReleasedCallbackData callback_data(window_ptr,
                                                       static_cast<Anvil::KeyID>(LOWORD(param_wide) & 0xFF) );

            window_ptr->callback(WINDOW_CALLBACK_ID_KEYPRESS_RELEASED,
                                &callback_data);

            return 0;
        }

        case WM_PAINT:
        {
            if (!window_ptr->m_window_should_close)
            {
                window_ptr->m_present_callback_func_ptr(window_ptr->m_present_callback_func_user_arg);
            }

            return 0;
        }

        default:
        {
            break;
        }
    }

    return DefWindowProc(window_handle,
                         message_id,
                         param_wide,
                         param_long);
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

        ::PeekMessage(&msg,
                      nullptr,
                      0,
                      0,
                      PM_REMOVE);

        if (msg.message == WM_QUIT)
        {
            done = 1;
        }
        else
        {
            /* Translate and dispatch to event queue */
            ::TranslateMessage(&msg);
            ::DispatchMessage (&msg);
        }
    }
}

void Anvil::WindowWin3264::set_title(const char* new_title)
{
    /* This function should only be called for wrapper instances which have created the window! */
    anvil_assert(m_window_owned);

    ::SetWindowText(m_window,
                    new_title);
}
