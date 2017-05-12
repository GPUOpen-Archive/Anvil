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

#include <dlfcn.h>
#include <string.h>
#include <xcb/xcb.h>
#include "misc/xcb_loader_for_anvil.h"

// =====================================================================================================================
XCBLoaderForAnvil::XCBLoaderForAnvil()
    :m_initialized(false)
{
    memset(m_libraryHandles, 0, sizeof(m_libraryHandles));
    memset(&m_funcs,         0, sizeof(m_funcs));
}


// =====================================================================================================================
XCBLoaderForAnvil::~XCBLoaderForAnvil()
{
    if (m_libraryHandles[LibxcbKeysyms] != nullptr)
    {
        dlclose(m_libraryHandles[LibxcbKeysyms]);
    }

    if (m_libraryHandles[Libxcb] != nullptr)
    {
        dlclose(m_libraryHandles[Libxcb]);
    }
}

// =====================================================================================================================
Result XCBLoaderForAnvil::Init()
{
    Result result = Result::Success;
    if (m_initialized == false)
    {
        // resolve symbols from libxcb-keysyms.so
        m_libraryHandles[LibxcbKeysyms] = dlopen("libxcb-keysyms.so",
                                                 RTLD_LAZY);

        if (m_libraryHandles[LibxcbKeysyms] == nullptr)
        {
            result = Result::ErrorUnavailable;
        }
        else
        {
            m_funcs.pfnXcbKeySymbolsAlloc        = (XcbKeySymbolsAlloc)       dlsym(m_libraryHandles[LibxcbKeysyms],
                                                                                    "xcb_key_symbols_alloc");
            m_funcs.pfnXcbKeyReleaseLookupKeysym = (XcbKeyReleaseLookupKeysym)dlsym(m_libraryHandles[LibxcbKeysyms],
                                                                                    "xcb_key_release_lookup_keysym");
        }

        // resolve symbols from libxcb.so
        m_libraryHandles[Libxcb] = dlopen("libxcb.so",
                                          RTLD_LAZY);

        if (m_libraryHandles[Libxcb] == nullptr)
        {
            result = Result::ErrorUnavailable;
        }
        else
        {
            m_funcs.pfnXcbChangeProperty     = (XcbChangeProperty)     dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_change_property");
            m_funcs.pfnXcbConnect            = (XcbConnect)            dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_connect");
            m_funcs.pfnXcbCreateWindow       = (XcbCreateWindow)       dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_create_window");
            m_funcs.pfnXcbDestroyWindow      = (XcbDestroyWindow)      dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_destroy_window");
            m_funcs.pfnXcbDisconnect         = (XcbDisconnect)         dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_disconnect");
            m_funcs.pfnXcbFlush              = (XcbFlush)              dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_flush");
            m_funcs.pfnXcbGenerateId         = (XcbGenerateId)         dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_generate_id");
            m_funcs.pfnXcbGetGeometry        = (XcbGetGeometry)        dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_get_geometry");
            m_funcs.pfnXcbGetGeometryReply   = (XcbGetGeometryReply)   dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_get_geometry_reply");
            m_funcs.pfnXcbGetSetup           = (XcbGetSetup)           dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_get_setup");
            m_funcs.pfnXcbInternAtom         = (XcbInternAtom)         dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_intern_atom");
            m_funcs.pfnXcbInternAtomReply    = (XcbInternAtomReply)    dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_intern_atom_reply");
            m_funcs.pfnXcbMapWindow          = (XcbMapWindow)          dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_map_window");
            m_funcs.pfnXcbPollForEvent       = (XcbPollForEvent)       dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_poll_for_event");
            m_funcs.pfnXcbScreenNext         = (XcbScreenNext)         dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_screen_next");
            m_funcs.pfnXcbSetupRootsIterator = (XcbSetupRootsIterator) dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_setup_roots_iterator");
            m_funcs.pfnXcbUnmapWindow        = (XcbUnmapWindow)        dlsym(m_libraryHandles[Libxcb],
                                                                             "xcb_unmap_window");
        }

        if (result == Result::Success)
        {
            m_initialized = true;
        }
    }

    return result;
}

