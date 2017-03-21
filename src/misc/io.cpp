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

/*
 * File manipulation helper functions.
 */
#include "misc/debug.h"
#include "misc/io.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <dirent.h>
    #include <unistd.h>
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
    std::string absolute_path;
    {
        const int   max_size = 1000;
        char        buffer[max_size];

        /* mkdir() require us to pass the full path to the directory
         * we want to create. Form a string that contains this data. */
        memset(buffer, 0, max_size);

        if (nullptr == getcwd(buffer, max_size))
        {
           anvil_assert(false);
        }

        absolute_path = std::string(buffer);

        if (*absolute_path.rbegin() != '/')
            absolute_path.append("/");

        absolute_path += name;
    }

    struct stat  st;
    bool         status = true;

    if (stat(absolute_path.c_str(), &st) != 0)
    {
        if(mkdir(absolute_path.c_str(), 0777) != 0)
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

/* Please see header for specification */
bool Anvil::IO::enumerate_files_in_directory(const std::string&        path,
                                             std::vector<std::string>* out_result_ptr)
{
    bool result = false;

    #ifdef _WIN32
    {
        HANDLE             file_finder(INVALID_HANDLE_VALUE);
        WIN32_FIND_DATAW   find_data;
        const std::string  path_expanded     (path + "//*");
        const std::wstring path_expanded_wide(path_expanded.begin(), path_expanded.end() );

        if ( (file_finder = ::FindFirstFileW(path_expanded_wide.c_str(),
                                            &find_data)) == INVALID_HANDLE_VALUE)
        {
            goto end;
        }

        do
        {
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
            {
                std::wstring file_name_wide(find_data.cFileName);
                std::string  file_name     (file_name_wide.begin(), file_name_wide.end() );

                out_result_ptr->push_back(file_name);
            }
        }
        while (::FindNextFileW(file_finder,
                              &find_data) );

        if (file_finder != INVALID_HANDLE_VALUE)
        {
            ::FindClose(file_finder);

            result = true;
        }
    }
    #else
    {
        struct dirent* dir_ptr     = nullptr;
        DIR*           file_finder = nullptr;

        file_finder = opendir(path.c_str() );

        if (file_finder != nullptr)
        {
            while ( (dir_ptr = readdir(file_finder) ) != nullptr)
            {
                out_result_ptr->push_back(dir_ptr->d_name);
            }

            closedir(file_finder);
            result = true;
        }
    }
    #endif

    #ifdef _WIN32
end:
    #endif

    return result;
}

/* Please see header for specification */
bool Anvil::IO::is_directory(const std::string& path)
{
    bool        result    = false;
    struct stat stat_data = {0};

    if ((stat(path.c_str(),
             &stat_data)             == 0)      &&
        (stat_data.st_mode & S_IFMT) == S_IFDIR)
    {
        result = true;
    }

    return result;
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
                            bool      is_text_file,
                            char**    out_result_ptr,
                            size_t*   out_opt_size_ptr)
{
    FILE*  file_handle = nullptr;
    size_t file_size   = 0;
    char*  result      = nullptr;

    file_handle = fopen(filename.c_str(),
                        (is_text_file) ? "rt" : "rb");
    anvil_assert(file_handle != 0);

    fseek(file_handle,
          0,
          SEEK_END);

    file_size = static_cast<size_t>(ftell(file_handle) );

    anvil_assert(file_size != -1 &&
                 file_size !=  0);

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

/** Please see header for specification */
void Anvil::IO::write_binary_file(std::string  filename,
                                  const void*  data,
                                  unsigned int data_size,
                                  bool         should_append)
{
    FILE* file_handle = nullptr;

    file_handle = fopen(filename.c_str(),
                        (should_append) ? "ab" : "wb");
    anvil_assert(file_handle != 0);

    if (fwrite(data,
               data_size,
               1, /* count */
               file_handle) != 1)
    {
        anvil_assert(false);
    }

    fclose(file_handle);
}

/** Please see header for specification */
void Anvil::IO::write_text_file(std::string filename,
                                std::string contents,
                                bool        should_append)
{
    FILE* file_handle = nullptr;

    file_handle = fopen(filename.c_str(),
                        (should_append) ? "a" : "wt");
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
 