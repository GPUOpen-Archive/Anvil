//
// Copyright (c) 2017-2018 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/library.h"

#if !defined(_WIN32)
    #include <dlfcn.h>
#endif

Anvil::Library::Library(const char* in_dll_name)
    :m_dll_name(in_dll_name)
{
    anvil_assert(in_dll_name != nullptr);
}

Anvil::Library::~Library()
{
    if (m_handle != nullptr)
    {
        #if defined(_WIN32)
        {
            ::FreeLibrary(reinterpret_cast<HMODULE>(m_handle) );
        }
        #else
        {
            dlclose(m_handle);
        }
        #endif
    }
}

Anvil::LibraryUniquePtr Anvil::Library::create(const char* in_dll_name)
{
    Anvil::LibraryUniquePtr result_ptr;

    result_ptr.reset(
        new Anvil::Library(in_dll_name)
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

void* Anvil::Library::get_proc_address(const char* in_func_name) const
{
    void* result_ptr = nullptr;

    #if defined(_WIN32)
    {
        result_ptr = ::GetProcAddress(reinterpret_cast<HMODULE>(m_handle),
                                      in_func_name);
    }
    #else
    {
        result_ptr = dlsym(m_handle,
                           in_func_name);
    }
    #endif

    return result_ptr;
}

bool Anvil::Library::init()
{
    bool result = false;

    #if defined(_WIN32)
    {
        m_handle = reinterpret_cast<void*>(::LoadLibrary(m_dll_name.c_str() ));

        if (m_handle == INVALID_HANDLE_VALUE)
        {
            anvil_assert(m_handle != INVALID_HANDLE_VALUE);

            goto end;
        }
    }
    #else
    {
        m_handle = reinterpret_cast<void*>(dlopen(m_dll_name.c_str(),
                                                  RTLD_LAZY) );

        if (m_handle == nullptr)
        {
            anvil_assert(m_handle != nullptr);

            goto end;
        }
    }
    #endif

    result = true;
end:
    return result;
}