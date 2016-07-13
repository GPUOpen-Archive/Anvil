// generated, please don't modify

#pragma once

#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
typedef uint32_t uint32;
enum class Result : int32_t
{
#ifndef Success
    Success                         = 0x00000000,
#endif
    ErrorUnavailable                = -(0x00000002),
};

// symbols from libxcb-keysyms.so
typedef xcb_key_symbols_t* (*XcbKeySymbolsAlloc)(xcb_connection_t*     c);

typedef xcb_keysym_t (*XcbKeyReleaseLookupKeysym)(xcb_key_symbols_t*        syms,
                                                  xcb_key_release_event_t*  event,
                                                  int                       col);

// symbols from libxcb.so
typedef uint32_t (*XcbGenerateId)(xcb_connection_t*     c);

typedef xcb_void_cookie_t (*XcbCreateWindow)(xcb_connection_t*     c,
                                             uint8_t               depth,
                                             xcb_window_t          wid,
                                             xcb_window_t          parent,
                                             int16_t               x,
                                             int16_t               y,
                                             uint16_t              width,
                                             uint16_t              height,
                                             uint16_t              border_width,
                                             uint16_t              _class,
                                             xcb_visualid_t        visual,
                                             uint32_t              value_mask,
                                             const uint32_t*       value_list);

typedef xcb_intern_atom_cookie_t (*XcbInternAtom)(xcb_connection_t*     c,
                                                  uint8_t               only_if_exists,
                                                  uint16_t              name_len,
                                                  const char*           name);

typedef xcb_intern_atom_reply_t* (*XcbInternAtomReply)(xcb_connection_t*         c,
                                                       xcb_intern_atom_cookie_t  cookie,
                                                       xcb_generic_error_t**     e);

typedef xcb_void_cookie_t (*XcbChangeProperty)(xcb_connection_t*     c,
                                               uint8_t               mode,
                                               xcb_window_t          window,
                                               xcb_atom_t            property,
                                               xcb_atom_t            type,
                                               uint8_t               format,
                                               uint32_t              data_len,
                                               const void*           data);

typedef xcb_void_cookie_t (*XcbMapWindow)(xcb_connection_t*     c,
                                          xcb_window_t          window);

typedef int (*XcbFlush)(xcb_connection_t*     c);

typedef xcb_connection_t* (*XcbConnect)(const char*   displayname,
                                        int*          screenp);

typedef xcb_setup_t* (*XcbGetSetup)(xcb_connection_t*     c);

typedef xcb_screen_iterator_t (*XcbSetupRootsIterator)(const xcb_setup_t*    R);

typedef void (*XcbScreenNext)(xcb_screen_iterator_t*    i);

typedef xcb_generic_event_t* (*XcbPollForEvent) (xcb_connection_t*     c);

typedef xcb_void_cookie_t    (*XcbUnmapWindow)  (xcb_connection_t*     c,
                                                 xcb_window_t          window);

typedef xcb_void_cookie_t    (*XcbDestroyWindow)(xcb_connection_t*     c,
                                                 xcb_window_t          window);

typedef void                 (*XcbDisconnect)   (xcb_connection_t*     c);


enum XCBLoaderForAnvilLibraries : uint32
{
    LibxcbKeysyms = 0,
    Libxcb = 1,
    XCBLoaderForAnvilLibrariesCount = 2
};

struct XCBLoaderForAnvilFuncs
{
    XcbKeySymbolsAlloc            pfnXcbKeySymbolsAlloc;
    XcbKeyReleaseLookupKeysym     pfnXcbKeyReleaseLookupKeysym;
    XcbGenerateId                 pfnXcbGenerateId;
    XcbCreateWindow               pfnXcbCreateWindow;
    XcbInternAtom                 pfnXcbInternAtom;
    XcbInternAtomReply            pfnXcbInternAtomReply;
    XcbChangeProperty             pfnXcbChangeProperty;
    XcbMapWindow                  pfnXcbMapWindow;
    XcbFlush                      pfnXcbFlush;
    XcbConnect                    pfnXcbConnect;
    XcbGetSetup                   pfnXcbGetSetup;
    XcbSetupRootsIterator         pfnXcbSetupRootsIterator;
    XcbScreenNext                 pfnXcbScreenNext;
    XcbPollForEvent               pfnXcbPollForEvent;
    XcbUnmapWindow                pfnXcbUnmapWindow;
    XcbDestroyWindow              pfnXcbDestroyWindow;
    XcbDisconnect                 pfnXcbDisconnect;
};

// =====================================================================================================================
// the class is responsible to resolve all external symbols that required by the Anvil.
class XCBLoaderForAnvil
{
public:
    XCBLoaderForAnvil();
   ~XCBLoaderForAnvil();

    bool                          Initialized()        { return m_initialized; }
    const XCBLoaderForAnvilFuncs* GetProcsTable()const { return &m_funcs; }
    Result                        Init();

private:
    void*                  m_libraryHandles[XCBLoaderForAnvilLibrariesCount];
    bool                   m_initialized;
    XCBLoaderForAnvilFuncs m_funcs;
};

