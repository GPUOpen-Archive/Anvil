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

/** Implements a wrapper for a single Vulkan instance. Implemented in order to:
 *
 *  - manage life-time of Vulkan instances.
 *  - encapsulate all logic required to manipulate instances and children objects.
 *  - let ObjectTracker detect leaking Vulkan instance wrapper instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef MISC_LIBRARY_H
#define MISC_LIBRARY_H

#include "misc/types.h"

namespace Anvil
{
    /* Forward declarations */
    class Library;

    typedef std::unique_ptr<Library> LibraryUniquePtr;

    class Library
    {
    public:
        /* Public functions */
        static LibraryUniquePtr create(const char* in_dll_name);

        ~Library();

        void* get_proc_address(const char* in_func_name) const;

    private:
        /* Private functions */
        Library(const char* in_dll_name);

        bool init();

        /* Private variables */
        const std::string m_dll_name;
        void*             m_handle;

        Library           (const Library&) = delete;
        Library& operator=(const Library&) = delete;
    };
}

#endif /* MISC_LIBRARY_H */