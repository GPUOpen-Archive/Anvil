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

#pragma once

#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

typedef uint32_t uint32;

enum class Result : int32_t
{
#ifndef Success
    Success          = 0x00000000,
#endif
    ErrorUnavailable = -(0x00000002),
};

// symbols from libxcb-keysyms.so
typedef xcb_keysym_t       (*PFN_XcbKeyReleaseLookupKeysym)(xcb_key_symbols_t*       syms,
                                                            xcb_key_release_event_t* event,
                                                            int                      col);
typedef xcb_key_symbols_t* (*PFN_XcbKeySymbolsAlloc)       (xcb_connection_t*        c);

// symbols from libxcb.so
typedef xcb_void_cookie_t        (*PFN_XcbChangeProperty)    (xcb_connection_t*          c,
                                                              uint8_t                    mode,
                                                              xcb_window_t               window,
                                                              xcb_atom_t                 property,
                                                              xcb_atom_t                 type,
                                                              uint8_t                    format,
                                                              uint32_t                   data_len,
                                                              const void*                data);
typedef xcb_connection_t*        (*PFN_XcbConnect)           (const char*                displayname,
                                                              int*                       screenp);
typedef xcb_void_cookie_t        (*PFN_XcbCreateWindow)      (xcb_connection_t*          c,
                                                              uint8_t                    depth,
                                                              xcb_window_t               wid,
                                                              xcb_window_t               parent,
                                                              int16_t                    x,
                                                              int16_t                    y,
                                                              uint16_t                   width,
                                                              uint16_t                   height,
                                                              uint16_t                   border_width,
                                                              uint16_t                   _class,
                                                              xcb_visualid_t             visual,
                                                              uint32_t                   value_mask,
                                                              const uint32_t*            value_list);
typedef xcb_void_cookie_t        (*PFN_XcbDestroyWindow)     (xcb_connection_t*          c,
                                                              xcb_window_t               window);
typedef void                     (*PFN_XcbDisconnect)        (xcb_connection_t*          c);
typedef int                      (*PFN_XcbFlush)             (xcb_connection_t*          c);
typedef uint32_t                 (*PFN_XcbGenerateId)        (xcb_connection_t*          c);
typedef xcb_get_geometry_cookie_t(*PFN_XcbGetGeometry)       (xcb_connection_t *         connection,
                                                              xcb_drawable_t             drawable);
typedef xcb_get_geometry_reply_t*(*PFN_XcbGetGeometryReply)  (xcb_connection_t*          connection,
                                                              xcb_get_geometry_cookie_t  cookie,
                                                              xcb_generic_error_t**      error);
typedef xcb_setup_t*             (*PFN_XcbGetSetup)          (xcb_connection_t*          c);
typedef xcb_intern_atom_cookie_t (*PFN_XcbInternAtom)        (xcb_connection_t*          c,
                                                              uint8_t                    only_if_exists,
                                                              uint16_t                   name_len,
                                                              const char*                name);
typedef xcb_intern_atom_reply_t* (*PFN_XcbInternAtomReply)   (xcb_connection_t*          c,
                                                              xcb_intern_atom_cookie_t   cookie,
                                                              xcb_generic_error_t**      e);
typedef xcb_void_cookie_t        (*PFN_XcbMapWindow)         (xcb_connection_t*          c,
                                                              xcb_window_t               window);
typedef xcb_generic_event_t*     (*PFN_XcbPollForEvent)      (xcb_connection_t*          c);
typedef void                     (*PFN_XcbScreenNext)        (xcb_screen_iterator_t*     i);
typedef xcb_screen_iterator_t    (*PFN_XcbSetupRootsIterator)(const xcb_setup_t*         R);
typedef xcb_void_cookie_t        (*PFN_XcbUnmapWindow)       (xcb_connection_t*          c,
                                                              xcb_window_t               window);


enum XCBLoaderLibraries : uint32
{
    XCB_LOADER_LIBRARIES_XCB_KEYSYMS = 0,
    XCB_LOADER_LIBRARIES_XCB         = 1,

    XCB_LOADER_LIBRARIES_COUNT = 2
};

struct XCBLoaderFuncs
{
    PFN_XcbChangeProperty         pfn_xcbChangeProperty;
    PFN_XcbConnect                pfn_xcbConnect;
    PFN_XcbCreateWindow           pfn_xcbCreateWindow;
    PFN_XcbDestroyWindow          pfn_xcbDestroyWindow;
    PFN_XcbDisconnect             pfn_xcbDisconnect;
    PFN_XcbFlush                  pfn_xcbFlush;
    PFN_XcbGenerateId             pfn_xcbGenerateId;
    PFN_XcbGetGeometry            pfn_xcbGetGeometry;
    PFN_XcbGetGeometryReply       pfn_xcbGetGeometryReply;
    PFN_XcbGetSetup               pfn_xcbGetSetup;
    PFN_XcbInternAtom             pfn_xcbInternAtom;
    PFN_XcbInternAtomReply        pfn_xcbInternAtomReply;
    PFN_XcbKeyReleaseLookupKeysym pfn_xcbKeyReleaseLookupKeysym;
    PFN_XcbKeySymbolsAlloc        pfn_xcbKeySymbolsAlloc;
    PFN_XcbMapWindow              pfn_xcbMapWindow;
    PFN_XcbPollForEvent           pfn_xcbPollForEvent;
    PFN_XcbScreenNext             pfn_xcbScreenNext;
    PFN_XcbSetupRootsIterator     pfn_xcbSetupRootsIterator;
    PFN_XcbUnmapWindow            pfn_xcbUnmapWindow;
};

/*
 * Resolves all external symbols that are going to be needed when using XCB functionality.
 */
class XCBLoader
{
public:
    XCBLoader();
   ~XCBLoader();

    const XCBLoaderFuncs* get_procs_table() const
    {
        return &m_funcs;
    }

    Result init();

    bool is_initialized() const
    {
        return m_initialized;
    }

private:
    XCBLoaderFuncs m_funcs;
    bool           m_initialized;
    void*          m_library_handles[XCB_LOADER_LIBRARIES_COUNT];
};

