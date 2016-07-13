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

#include "misc/window_win3264.h"

/** Please see header for specification */
Anvil::WindowWin3264::WindowWin3264(const std::string&     title,
                                    unsigned int           width,
                                    unsigned int           height,
                                    PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                                    void*                  present_callback_func_user_arg)
    : Window(title,
             width,
             height,
             present_callback_func_ptr,
             present_callback_func_user_arg)
{
    init();
}

/** Please see header for specification */
void Anvil::WindowWin3264::close()
{
    if (!m_window_should_close)
    {
        m_window_should_close = true;

        {
            /* NOTE: This will unblock the thread executing run(). */
            DestroyWindow(m_window);
        }
    }
}

/** Creates a new system window and prepares it for usage. */
void Anvil::WindowWin3264::init()
{
    const uint32_t window_size[2] =
    {
        m_width,
        m_height
    };

    static const char* class_name  = "Anvil window class";
    HINSTANCE          instance    = GetModuleHandle(nullptr);
    WNDCLASSEX         window_class;
    RECT               window_rect;

    // Initialize the window class structure:
    window_class.cbSize        = sizeof(WNDCLASSEX);
    window_class.style         = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc   = msg_callback_pfn_proc;
    window_class.cbClsExtra    = 0;
    window_class.cbWndExtra    = 0;
    window_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH) );
    window_class.hInstance     = instance;
    window_class.hIcon         = LoadIcon  (nullptr,  /* hInstance */
                                            IDI_APPLICATION);
    window_class.hCursor       = LoadCursor(nullptr,  /* hInstance */
                                            IDC_ARROW);
    window_class.lpszMenuName  = nullptr;
    window_class.lpszClassName = class_name;
    window_class.hIconSm       = LoadIcon(nullptr, /* hInstance */
                                          IDI_WINLOGO);

    /* Register window class. If more than one window is instantiated, this call will fail
     * but we don't really care.
     **/
    RegisterClassEx(&window_class);

    /* Create the window */
    window_rect.left   = 0;
    window_rect.top    = 0;
    window_rect.right  = window_size[0];
    window_rect.bottom = window_size[1];

    AdjustWindowRect(&window_rect,
                      WS_OVERLAPPEDWINDOW,
                      FALSE /* bMenu */);

    m_window = CreateWindowEx(0,                    /* dwExStyle */
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
        MessageBox(HWND_DESKTOP,
                   "Failed to create a window.",
                   "Error",
                   MB_OK | MB_ICONERROR);
    }

    SetWindowLongPtr(m_window,
                     GWLP_USERDATA,
                     reinterpret_cast<LONG_PTR>(this) );
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
    WindowWin3264* window_ptr = reinterpret_cast<WindowWin3264*>(GetWindowLongPtr(window_handle,
                                                                                  GWLP_USERDATA) );

    switch (message_id)
    {
        case WM_DESTROY:
        {
            window_ptr->m_window_should_close = true;

            PostQuitMessage(0);

            return 0;
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
    int done   = 0;
    int result = 0;

    /* Run the message loop */
    while (!done)
    {
        MSG msg;

        PeekMessage(&msg,
                    nullptr,
                    0,
                    0,
                    PM_REMOVE);

        if (msg.message == WM_QUIT)
        {
            done   = 1;
            result = (int) msg.wParam;
        }
        else
        {
            /* Translate and dispatch to event queue */
            TranslateMessage(&msg);
            DispatchMessage (&msg);
        }
    }
}
