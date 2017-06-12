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
#include "misc/xcb_loader.h"

// =====================================================================================================================
XCBLoader::XCBLoader()
    :m_initialized(false)
{
    memset(m_library_handles,
           0,
           sizeof(m_library_handles));

    memset(&m_funcs, 
           0,
           sizeof(m_funcs));
}


// =====================================================================================================================
XCBLoader::~XCBLoader()
{
    if (m_library_handles[XCB_LOADER_LIBRARIES_XCB_KEYSYMS] != nullptr)
    {
        dlclose(m_library_handles[XCB_LOADER_LIBRARIES_XCB_KEYSYMS]);
    }

    if (m_library_handles[XCB_LOADER_LIBRARIES_XCB] != nullptr)
    {
        dlclose(m_library_handles[XCB_LOADER_LIBRARIES_XCB]);
    }
}

// =====================================================================================================================
Result XCBLoader::init()
{
    Result result = Result::Success;

    if (m_initialized == false)
    {
        // resolve symbols from libxcb-keysyms.so
        m_library_handles[XCB_LOADER_LIBRARIES_XCB_KEYSYMS] = dlopen("libxcb-keysyms.so",
                                                                     RTLD_LAZY);

        if (m_library_handles[XCB_LOADER_LIBRARIES_XCB_KEYSYMS] == nullptr)
        {
            result = Result::ErrorUnavailable;
        }
        else
        {
            m_funcs.pfn_xcbKeySymbolsAlloc        = reinterpret_cast<PFN_XcbKeySymbolsAlloc>       (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB_KEYSYMS],
                                                                                                    "xcb_key_symbols_alloc") );
            m_funcs.pfn_xcbKeyReleaseLookupKeysym = reinterpret_cast<PFN_XcbKeyReleaseLookupKeysym>(dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB_KEYSYMS],
                                                                                                    "xcb_key_release_lookup_keysym") );
        }

        // resolve symbols from libxcb.so
        m_library_handles[XCB_LOADER_LIBRARIES_XCB] = dlopen("libxcb.so",
                                                             RTLD_LAZY);

        if (m_library_handles[XCB_LOADER_LIBRARIES_XCB] == nullptr)
        {
            result = Result::ErrorUnavailable;
        }
        else
        {
            m_funcs.pfn_xcbChangeProperty     = reinterpret_cast<PFN_XcbChangeProperty>   (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                           "xcb_change_property") );
            m_funcs.pfn_xcbConnect            = reinterpret_cast<PFN_XcbConnect>          (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                           "xcb_connect") );
            m_funcs.pfn_xcbCreateWindow       = reinterpret_cast<PFN_XcbCreateWindow>     (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                           "xcb_create_window") );
            m_funcs.pfn_xcbDestroyWindow      = reinterpret_cast<PFN_XcbDestroyWindow>     (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_destroy_window") );
            m_funcs.pfn_xcbDisconnect         = reinterpret_cast<PFN_XcbDisconnect>        (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_disconnect") );
            m_funcs.pfn_xcbFlush              = reinterpret_cast<PFN_XcbFlush>             (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_flush") );
            m_funcs.pfn_xcbGenerateId         = reinterpret_cast<PFN_XcbGenerateId>        (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_generate_id") );
            m_funcs.pfn_xcbGetGeometry        = reinterpret_cast<PFN_XcbGetGeometry>       (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_get_geometry") );
            m_funcs.pfn_xcbGetGeometryReply   = reinterpret_cast<PFN_XcbGetGeometryReply>  (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_get_geometry_reply") );
            m_funcs.pfn_xcbGetSetup           = reinterpret_cast<PFN_XcbGetSetup>          (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_get_setup") );
            m_funcs.pfn_xcbInternAtom         = reinterpret_cast<PFN_XcbInternAtom>        (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_intern_atom") );
            m_funcs.pfn_xcbInternAtomReply    = reinterpret_cast<PFN_XcbInternAtomReply>   (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_intern_atom_reply") );
            m_funcs.pfn_xcbMapWindow          = reinterpret_cast<PFN_XcbMapWindow>         (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_map_window") );
            m_funcs.pfn_xcbPollForEvent       = reinterpret_cast<PFN_XcbPollForEvent>      (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_poll_for_event") );
            m_funcs.pfn_xcbScreenNext         = reinterpret_cast<PFN_XcbScreenNext>        (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_screen_next") );
            m_funcs.pfn_xcbSetupRootsIterator = reinterpret_cast<PFN_XcbSetupRootsIterator>(dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_setup_roots_iterator") );
            m_funcs.pfn_xcbUnmapWindow        = reinterpret_cast<PFN_XcbUnmapWindow>       (dlsym(m_library_handles[XCB_LOADER_LIBRARIES_XCB],
                                                                                            "xcb_unmap_window") );
        }

        if (result == Result::Success)
        {
            m_initialized = true;
        }
    }

    return result;
}

