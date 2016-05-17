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

#include "misc/debug.h"
#include "misc/window.h"

/** Please see header for specification */
Anvil::Window::Window(const std::string&     title,
                      unsigned int           width,
                      unsigned int           height,
                      PFNPRESENTCALLBACKPROC present_callback_func_ptr,
                      void*                  present_callback_func_user_arg)
    :CallbacksSupportProvider        (WINDOW_CALLBACK_ID_COUNT),
     m_height                        (height),
     m_present_callback_func_ptr     (present_callback_func_ptr),
     m_present_callback_func_user_arg(present_callback_func_user_arg),
     m_title                         (title),
     m_width                         (width)
    ,m_window_should_close           (false)

{
    init();
}

/** Destructor */
Anvil::Window::~Window()
{
    /* Stub */
}

/** Please see header for specification */
void Anvil::Window::close()
{
    if (!m_window_should_close)
    {
        m_window_should_close = true;

        #ifdef _WIN32
        {
            /* SWDEV-93059: Work around a issue where destroying a window, whose swapchain uses a FIFO presentation mode with
             *              a pending image present op, locks up the FIFO thread forever. */
            ::Sleep(1000);

            /* NOTE: This will unblock the thread executing run(). */
            DestroyWindow(m_window);
        }
        #else
        free(m_key_symbols);
        #endif
    }
}

/** Creates a new system window and prepares it for usage. */
void Anvil::Window::init()
{
    const uint32_t window_size[2] =
    {
        m_width,
        m_height
    };

    #ifdef _WIN32
    {
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
    #else
    {
        init_connection();

        const uint32_t value_mask    = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        uint32_t       value_list[sizeof(uint32_t) * 8];
        xcb_window_t   window;

        value_list[0] = m_screen_ptr->black_pixel;
        value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

        window = xcb_generate_id(m_connection_ptr);

        xcb_create_window(m_connection_ptr,
                          XCB_COPY_FROM_PARENT,
                          window,
                          m_screen_ptr->root,
                          0,
                          0,
                          window_size[0],
                          window_size[1],
                          0,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          m_screen_ptr->root_visual,
                          value_mask,
                          value_list);

        /* Magic code that will send notification when window is destroyed */
        xcb_intern_atom_reply_t* atom_reply_ptr            = nullptr;
        xcb_intern_atom_reply_t* atom_wm_delete_window_ptr = nullptr;
        xcb_intern_atom_cookie_t cookie;
        xcb_intern_atom_cookie_t cookie2;

        cookie  = xcb_intern_atom(m_connection_ptr,
                                  1,
                                  12,
                                  "WM_PROTOCOLS");
        cookie2 = xcb_intern_atom(m_connection_ptr,
                                  0,
                                  16,
                                  "WM_DELETE_WINDOW");

        atom_reply_ptr            = xcb_intern_atom_reply(m_connection_ptr,
                                                          cookie,
                                                          0);
        atom_wm_delete_window_ptr = xcb_intern_atom_reply(m_connection_ptr,
                                                          cookie2,
                                                          0);

        xcb_change_property(m_connection_ptr,
                            XCB_PROP_MODE_REPLACE,
                            window,
                            (*atom_reply_ptr).atom,
                            4,
                            32,
                            1,
                            &(*atom_wm_delete_window_ptr).atom);
        xcb_change_property(m_connection_ptr,
                            XCB_PROP_MODE_REPLACE,
                            window,
                            XCB_ATOM_WM_NAME,
                            XCB_ATOM_STRING,
                            8,
                            m_title.size(),
                            m_title.c_str() );

        free(atom_reply_ptr);

        xcb_map_window(m_connection_ptr,
                       window);

        xcb_flush(m_connection_ptr);

        m_atom_wm_delete_window_ptr = atom_wm_delete_window_ptr;
        m_window                    = window;

        m_key_symbols = xcb_key_symbols_alloc(m_connection_ptr);
    }
    #endif
}

#ifndef _WIN32

/** Initializes a XCB connection */
void Anvil::Window::init_connection()
{
    xcb_connection_t*     connection_ptr = nullptr;
    const xcb_setup_t*    setup_ptr      = nullptr;
    xcb_screen_iterator_t iter;
    int32_t               scr;

    connection_ptr = xcb_connect(nullptr,
                                &scr);

    if (connection_ptr == nullptr)
    {
        fflush(stdout);

        exit(1);
    }

    setup_ptr = xcb_get_setup           (connection_ptr);
    iter      = xcb_setup_roots_iterator(setup_ptr);

    while (scr-- > 0)
    {
        xcb_screen_next(&iter);
    }

    m_screen_ptr     = iter.data;
    m_connection_ptr = connection_ptr;
}
#else

/** Window message handler.
 *
 *  @param window_handle Window handle.
 *  @param message_id    Message ID
 *  @param param_wide    Wide window message parameter.
 *  @param param_long    Long window message parameter.
 *
 *  @return Window message-specific return value.
 **/
LRESULT CALLBACK Anvil::Window::msg_callback_pfn_proc(HWND   window_handle,
                                                      UINT   message_id,
                                                      WPARAM param_wide,
                                                      LPARAM param_long)
{
    Window* window_ptr = reinterpret_cast<Window*>(GetWindowLongPtr(window_handle,
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

#endif

/* Please see header for specification */
void Anvil::Window::run()
{
    #ifdef _WIN32
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
    #else
    {
        bool running = true;

        while (running && !m_window_should_close)
        {
            xcb_generic_event_t* event_ptr = xcb_poll_for_event(m_connection_ptr);

            if (event_ptr)
            {
                switch (event_ptr->response_type & 0x7f)
                {
                    case XCB_CLIENT_MESSAGE:
                    {
                        if ((*(xcb_client_message_event_t*) event_ptr).data.data32[0] == (*m_atom_wm_delete_window_ptr).atom)
                        {
                            running = false;
                        }

                        break;
                    }

                    case XCB_KEY_RELEASE:
                    {
                        const xcb_key_release_event_t* key_ptr = (const xcb_key_release_event_t *) event_ptr;

                        xcb_keysym_t sym = xcb_key_release_lookup_keysym(m_key_symbols, key_ptr, 0);

                        KeypressReleasedCallbackData callback_data(this,
                                                                   static_cast<Anvil::KeyID>(sym));

                        this->callback(WINDOW_CALLBACK_ID_KEYPRESS_RELEASED,
                                      &callback_data);

                        break;
                    }

                    case XCB_DESTROY_NOTIFY:
                    {
                        running = false;

                        break;
                    }

                    case XCB_EXPOSE:
                    {
                        m_present_callback_func_ptr(m_present_callback_func_user_arg);

                        running = !m_window_should_close;
                        break;
                    }

                    default:
                    {
                        break;
                    }
                }

                free(event_ptr);
            }
            else
            {
                m_present_callback_func_ptr(m_present_callback_func_user_arg); 
                
                running = !m_window_should_close;
            }
        }
    }
    #endif
}
