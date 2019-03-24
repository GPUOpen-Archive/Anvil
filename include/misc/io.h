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

#ifndef MISC_FILE_H
#define MISC_FILE_H

#include <stdio.h>
#include <string>
#include <vector>

#include "config.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        #include <X11/keysym.h>
        #include <xcb/xcb.h>
        #include <xcb/xcb_keysyms.h>
    #endif
#endif

namespace Anvil
{
    /* Keyboard character IDs */
    #ifdef _WIN32
        #define ANVIL_KEY_HELPER(key) VK_##key
    #else
        #define ANVIL_KEY_HELPER(key) XK_##key
    #endif

    typedef enum
    {
#ifdef _WIN32
        #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
            KEY_ID_ESCAPE = ANVIL_KEY_HELPER(ESCAPE),
            KEY_ID_LEFT   = ANVIL_KEY_HELPER(LEFT),
            KEY_ID_RETURN = ANVIL_KEY_HELPER(RETURN),
            KEY_ID_RIGHT  = ANVIL_KEY_HELPER(RIGHT),
            KEY_ID_SPACE  = ANVIL_KEY_HELPER(SPACE),
        #endif
#else
        #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
            KEY_ID_ESCAPE = ANVIL_KEY_HELPER(Escape),
            KEY_ID_LEFT   = ANVIL_KEY_HELPER(Left),
            KEY_ID_RETURN = ANVIL_KEY_HELPER(Return),
            KEY_ID_RIGHT  = ANVIL_KEY_HELPER(Right),
            KEY_ID_SPACE  = ANVIL_KEY_HELPER(space),
        #endif

#endif
        KEY_ID_COUNT,

        KEY_ID_UNKNOWN = KEY_ID_COUNT
    } KeyID;

    class IO
    {
    public:
        /** Creates a new directory in the process' working directory.
         *
         *  @param in_name Name of the new directory.
         *
         *  @return true if the directory was created successfully, false otherwise.
         **/
        static bool create_directory(std::string in_name);

        /** Deletes the specified file.
         *
         *  @param in_filename Name of the file to delete.
         *
         **/
        static bool delete_file(std::string in_filename);

        /** Enumerates files under user-specified directory. */
        static bool enumerate_files_in_directory(const std::string&        in_path,
                                                 bool                      in_recursive,
                                                 std::vector<std::string>* out_result_ptr);

        /** Tells whether the specified path exists and is a directory. */
        static bool is_directory(const std::string& in_path);

        /** Loads file contents and returns a buffer holding the read data.
         *
         *  Upon failure, the function generates an assertion failure.
         *
         *
         *  @param in_filename        Name of the file to read data from.
         *  @param in_is_text_file    True if the file is a text file; false otherwise.
         *  @param out_opt_result_ptr Deref will be set to a pointer to a buffer holding the read data. Must be released
         *                            with delete[] when no longer needed.
         *  @param out_opt_size_ptr   If not nullptr, deref will be set to the number of bytes exposed under @param out_result_ptr.
         *
         *  @return Read data, or nullptr if an error occurred. Make sure to release the returned buffer
         *          with delete[], when no longer needed.
         **/
        static bool read_file(std::string in_filename,
                              bool        in_is_text_file,
                              char**      out_opt_result_ptr,
                              size_t*     out_opt_size_ptr);

        /* TODO */
        static bool read_file(std::string in_filename,
                              bool        in_is_text_file,
                              size_t      in_start_offset,
                              size_t      in_size,
                              char**      out_result_ptr);

        /** Writes specified data to a file under specified location. If a file exists under
         *  given location, its contents is discarded.
         *
         *  Upon failure, the function generates an assertion failure.
         *
         *
         *  @param in_filename      Name of the file to write to (incl. path)
         *  @param in_contents      Contents to write.
         *  @param in_should_append True if new contents should be appended to the file;
         *                          false to purge existing contents (if any).
         **/
        static bool write_binary_file(std::string  in_filename,
                                      const void*  in_data,
                                      unsigned int in_data_size,
                                      bool         in_should_append = false);
        static bool write_text_file  (std::string  in_filename,
                                      std::string  in_contents,
                                      bool         in_should_append = false);
    };
}

#endif /* MISC_FILE_H */