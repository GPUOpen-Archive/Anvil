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

/*
 * File manipulation helper functions.
 */
#include "misc/debug.h"
#include "misc/io.h"
#include <string.h>

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif


/** Creates a new sub-directory in the process' working directory.
 *
 *  @param name Name of the folder to create.
 *
 *  @return true if the operation succeeded, false otherwise.
 **/
bool Anvil::IO::create_directory(std::string name)
{
#ifdef _WIN32
    wchar_t      buffer[32767]; /* as per MSDN documentation */
    std::wstring buffer_string;
    std::wstring full_path_wide;
    std::wstring name_wide   = std::wstring(name.begin(), name.end() );
    std::string  prefix      = std::string ("\x5C\x5C?\x5C");
    std::wstring prefix_wide = std::wstring(prefix.begin(), prefix.end() );

    /* CreateDirectory*() require us to pass the full path to the directory
     * we want to create. Form a string that contains this data. */
    memset(buffer,
           0,
           sizeof(buffer) );

    GetCurrentDirectoryW(sizeof(buffer),
                         buffer);

    buffer_string  = std::wstring(buffer);
    full_path_wide = prefix_wide + buffer + std::wstring(L"\\") + name_wide;

    return (CreateDirectoryW(full_path_wide.c_str(),
                             nullptr /* lpSecurityAttributes */) != 0);
#else
    char         absolute_path[1000];
    struct stat  st;
    bool         status = true;

    /* mkdir() require us to pass the full path to the directory
     * we want to create. Form a string that contains this data. */
    memset(absolute_path,
           0,
           sizeof(absolute_path) );

    if (nullptr == getcwd(absolute_path, 1000))
    {
       anvil_assert(false);
    }

    strcat(absolute_path, name.c_str());

    if (stat(absolute_path, &st) != 0)
    {
        if(mkdir(absolute_path, 0777) != 0)
        {
            status = false;
        }
    }
    else if (!S_ISDIR(st.st_mode))
    {
        status = false;
    }

    if (status == false)
    {
        anvil_assert(false);
    }

    return status;
#endif
}

/** Deletes a file in the working directory.
 *
 *  @param filename Name of the file to remove.
 **/
void Anvil::IO::delete_file(std::string filename)
{
    remove(filename.c_str());
}

/** Reads contents of a file with user-specified name and returns it to the caller.
 *
 *  @param filename         Name of the file to use for the operation.
 *  @param is_text_file     true if the file should be treated as a text file; false
 *                          if it should be considered a binary file.
 *  @param out_result_ptr   Deref will be set to an array, holding the read file contents.
 *                          The array must be deleted with a delete[] operator when no
 *                          longer needed. Must not be nullptr.
 *  @param out_opt_size_ptr Deref will be set to the number of bytes allocated for @param out_result_ptr.
 *                          May be nullptr.
 **/
void Anvil::IO::read_file(std::string filename,
                            bool        is_text_file,
                            char**      out_result_ptr,
                            size_t*     out_opt_size_ptr)
{
    FILE*  file_handle = nullptr;
    long   file_size   = 0;
    char*  result      = nullptr;

    file_handle = fopen(filename.c_str(),
                        (is_text_file) ? "rt" : "rb");
    anvil_assert(file_handle != 0);

    fseek(file_handle,
          0,
          SEEK_END);

    file_size = ftell(file_handle);

    anvil_assert(file_size != -1 &&
              file_size != 0);

    fseek(file_handle,
          0,
          SEEK_SET);

    result = new char[file_size + 1];
    anvil_assert(result != nullptr);

    memset(result,
           0,
           file_size + 1);

    size_t n_bytes_read = fread(result,
                                1,
                                file_size,
                                file_handle);

    fclose(file_handle);

    result[n_bytes_read] = 0;

    /* Set the output variables */
    *out_result_ptr = result;

    if (out_opt_size_ptr != nullptr)
    {
        *out_opt_size_ptr = file_size;
    }
}

/** Writes the specified text string to a file in the working directory, 
 *  under the specified name.
 *
 *  @param filename Name of the file to write the contents of the string to.
 *  @param contents Contents to write.
 **/
void Anvil::IO::write_file(std::string filename,
                             std::string contents)
{
    FILE* file_handle = nullptr;

    file_handle = fopen(filename.c_str(),
                        "wb");
    anvil_assert(file_handle != 0);

    if (fwrite(contents.c_str(),
               strlen(contents.c_str() ),
               1, /* count */
               file_handle) != 1)
    {
        anvil_assert(false);
    }

    fclose(file_handle);
}
