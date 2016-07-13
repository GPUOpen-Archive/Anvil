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

#include "misc/window_xcb.h"

/** Please see header for specification */
Anvil::WindowXcb::WindowXcb(const std::string&     title,
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

Anvil::WindowXcb::~WindowXcb()
{
    const XCBLoaderForAnvilFuncs* xcb_procs_ptr = m_xcb_loader.GetProcsTable();

    xcb_procs_ptr->pfnXcbDisconnect(static_cast<xcb_connection_t*>(m_connection_ptr));
}

/** Please see header for specification */
void Anvil::WindowXcb::close()
{
    if (!m_window_should_close)
    {
        const XCBLoaderForAnvilFuncs* xcb_procs_ptr = m_xcb_loader.GetProcsTable();

        m_window_should_close                       = true;

        free(m_key_symbols);
        free(m_atom_wm_delete_window_ptr);

        xcb_procs_ptr->pfnXcbUnmapWindow  (static_cast<xcb_connection_t*>(m_connection_ptr),
                                           m_window);

        xcb_procs_ptr->pfnXcbDestroyWindow(static_cast<xcb_connection_t*>(m_connection_ptr),
                                           m_window);
    }
}

/** Creates a new system window and prepares it for usage. */
void Anvil::WindowXcb::init()
{
    const uint32_t window_size[2] =
    {
        m_width,
        m_height
    };

    m_xcb_loader.Init();
    anvil_assert(m_xcb_loader.Initialized() == true);

    init_connection();

    const uint32_t      value_mask     = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t            value_list[sizeof(uint32_t) * 8];
    xcb_window_t        window;
    xcb_connection_t*   connection_ptr = static_cast<xcb_connection_t*>(m_connection_ptr);

    const XCBLoaderForAnvilFuncs* xcb_procs_ptr = m_xcb_loader.GetProcsTable();
    anvil_assert(xcb_procs_ptr != NULL);

    value_list[0] = m_screen_ptr->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    window = xcb_procs_ptr->pfnXcbGenerateId(connection_ptr);

    xcb_procs_ptr->pfnXcbCreateWindow(connection_ptr,
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

    cookie  = xcb_procs_ptr->pfnXcbInternAtom(connection_ptr,
                                              1,
                                              12,
                                              "WM_PROTOCOLS");
    cookie2 = xcb_procs_ptr->pfnXcbInternAtom(connection_ptr,
                                              0,
                                              16,
                                              "WM_DELETE_WINDOW");

    atom_reply_ptr            = xcb_procs_ptr->pfnXcbInternAtomReply(connection_ptr,
                                                                     cookie,
                                                                     0);
    atom_wm_delete_window_ptr = xcb_procs_ptr->pfnXcbInternAtomReply(connection_ptr,
                                                                     cookie2,
                                                                     0);

    xcb_procs_ptr->pfnXcbChangeProperty(connection_ptr,
                                        XCB_PROP_MODE_REPLACE,
                                        window,
                                        (*atom_reply_ptr).atom,
                                        4,
                                        32,
                                        1,
                                        &(*atom_wm_delete_window_ptr).atom);
    xcb_procs_ptr->pfnXcbChangeProperty(connection_ptr,
                                        XCB_PROP_MODE_REPLACE,
                                        window,
                                        XCB_ATOM_WM_NAME,
                                        XCB_ATOM_STRING,
                                        8,
                                        m_title.size(),
                                        m_title.c_str() );

    free(atom_reply_ptr);

    xcb_procs_ptr->pfnXcbMapWindow(connection_ptr,
                                   window);

    xcb_procs_ptr->pfnXcbFlush    (connection_ptr);

    m_atom_wm_delete_window_ptr = atom_wm_delete_window_ptr;
    m_window                    = window;

    m_key_symbols = xcb_procs_ptr->pfnXcbKeySymbolsAlloc(connection_ptr);
}

/** Initializes a XCB connection */
void Anvil::WindowXcb::init_connection()
{
    xcb_connection_t*     connection_ptr = nullptr;
    const xcb_setup_t*    setup_ptr      = nullptr;
    xcb_screen_iterator_t iter;
    int32_t               scr;

    const XCBLoaderForAnvilFuncs* xcb_procs_ptr = m_xcb_loader.GetProcsTable();
    anvil_assert(xcb_procs_ptr != NULL);

    connection_ptr = xcb_procs_ptr->pfnXcbConnect(nullptr,
                                                  &scr);

    if (connection_ptr == nullptr)
    {
        fflush(stdout);

        exit(1);
    }

    setup_ptr = xcb_procs_ptr->pfnXcbGetSetup          (connection_ptr);
    iter      = xcb_procs_ptr->pfnXcbSetupRootsIterator(setup_ptr);

    while (scr-- > 0)
    {
        xcb_procs_ptr->pfnXcbScreenNext(&iter);
    }

    m_screen_ptr     = iter.data;
    m_connection_ptr = static_cast<void*>(connection_ptr);
}

/* Please see header for specification */
void Anvil::WindowXcb::run()
{
    bool running = true;

    while (running && !m_window_should_close)
    {
        xcb_generic_event_t* event_ptr = m_xcb_loader.GetProcsTable()->pfnXcbPollForEvent(static_cast<xcb_connection_t*>(m_connection_ptr));

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
                    const xcb_key_release_event_t* key_ptr = (const xcb_key_release_event_t*) event_ptr;

                    xcb_keysym_t sym = m_xcb_loader.GetProcsTable()->pfnXcbKeyReleaseLookupKeysym(m_key_symbols,
                                                                                                  (xcb_key_release_event_t*) key_ptr,
                                                                                                  0);

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

