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
#include "misc/types.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

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
bool Anvil::IO::create_directory(std::string in_name)
{
#ifdef _WIN32
    std::vector<wchar_t> buffer(32767); /* as per MSDN documentation */
    std::wstring         buffer_string;
    std::wstring         full_path_wide;
    std::wstring         name_wide   = std::wstring(in_name.begin(), in_name.end() );
    std::string          prefix      = std::string ("\x5C\x5C?\x5C");
    std::wstring         prefix_wide = std::wstring(prefix.begin(), prefix.end() );

    /* CreateDirectory*() require us to pass the full path to the directory
     * we want to create. Form a string that contains this data. */
    memset(&buffer.at(0),
           0,
           buffer.size() * sizeof(wchar_t) );

    GetCurrentDirectoryW(static_cast<DWORD>(buffer.size()) / sizeof(wchar_t),
                        &buffer.at(0) );

    buffer_string  = std::wstring(&buffer.at(0) );
    full_path_wide = prefix_wide + &buffer.at(0) + std::wstring(L"\\") + name_wide;

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
           anvil_assert_fail();
        }

        absolute_path = std::string(buffer);

        if (*absolute_path.rbegin() != '/')
            absolute_path.append("/");

        absolute_path += in_name;
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
        anvil_assert_fail();
    }

    return status;
#endif
}

/** Deletes a file in the working directory.
 *
 *  @param filename Name of the file to remove.
 **/
bool Anvil::IO::delete_file(std::string in_filename)
{
    return (remove(in_filename.c_str()) == 0);
}

/* Please see header for specification */
bool Anvil::IO::enumerate_files_in_directory(const std::string&        in_path,
                                             bool                      in_recursive,
                                             std::vector<std::string>* out_result_ptr)
{
    bool result = false;

    #ifdef _WIN32
    {
        std::vector<std::wstring> outstanding_directories;
        HANDLE                    file_finder(INVALID_HANDLE_VALUE);
        WIN32_FIND_DATAW          find_data;

        {
            const std::string  path_expanded     (in_path + "//");
            const std::wstring path_expanded_wide(path_expanded.begin(), path_expanded.end() );

            outstanding_directories.push_back(path_expanded_wide);
        }

        while (!outstanding_directories.empty() )
        {
            std::wstring current_path_wide               = outstanding_directories.back();
            std::wstring current_path_with_wildcard_wide = current_path_wide + L"*";

            outstanding_directories.pop_back();

            if ( ((file_finder = ::FindFirstFileW(current_path_with_wildcard_wide.c_str(),
                                                 &find_data)) == INVALID_HANDLE_VALUE) ||
                  (file_finder == nullptr) )
            {
                goto end;
            }

            do
            {
                if (find_data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
                {
                    std::wstring file_name_wide          (find_data.cFileName);
                    std::wstring file_name_with_path_wide(current_path_wide + file_name_wide);
                    std::string  file_name               (file_name_with_path_wide.begin(), file_name_with_path_wide.end() );

                    out_result_ptr->push_back(file_name);
                }
                else
                if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    in_recursive)
                {
                    std::wstring dir_name_wide(find_data.cFileName);

                    if ((dir_name_wide != L".") &&
                        (dir_name_wide != L"..") )
                    {
                        std::wstring base_path_wide   (current_path_wide.begin(), current_path_wide.end() );
                        std::wstring new_path_expanded(base_path_wide + dir_name_wide + L"//");

                        outstanding_directories.push_back(new_path_expanded);
                    }
                }
            }
            while (::FindNextFileW(file_finder,
                                  &find_data) );

            if (file_finder != INVALID_HANDLE_VALUE)
            {
                ::FindClose(file_finder);
            }
        }

        result = true;
    }
    #else
    {

        struct dirent*           dir_ptr     = nullptr;
        DIR*                     file_finder = nullptr;
        std::vector<std::string> outstanding_directories;

        outstanding_directories.push_back(in_path);

        while (!outstanding_directories.empty() )
        {
            std::string current_path;

            current_path = outstanding_directories.back();
            outstanding_directories.pop_back();

            file_finder = opendir(current_path.c_str() );

            if (file_finder != nullptr)
            {
                while ( (dir_ptr = readdir(file_finder) ) != nullptr)
                {
                    if (dir_ptr->d_type != DT_DIR)
                    {
                        const std::string current_file_name = dir_ptr->d_name;

                        out_result_ptr->push_back(current_path + "/" + current_file_name);
                    }
                    else
                    if (in_recursive)
                    {
                        const std::string dir_name_string = std::string(dir_ptr->d_name);

                        if ((dir_name_string != ".")  &&
                            (dir_name_string != "..") )
                        {
                            outstanding_directories.push_back(current_path + "/" + dir_name_string);

                            continue;
                        }
                    }
                }

                closedir(file_finder);
                result = true;
            }
        }
    }
    #endif

    #ifdef _WIN32
end:
    #endif

    return result;
}

/* Please see header for specification */
bool Anvil::IO::is_directory(const std::string& in_path)
{
    bool        result    = false;
    struct stat stat_data = {0};

    if ((stat(in_path.c_str(),
             &stat_data)             == 0)      &&
        (stat_data.st_mode & S_IFMT) == S_IFDIR)
    {
        result = true;
    }

    return result;
}

/** Reads contents of a file with user-specified name and returns it to the caller.
 *
 *  @param in_filename        Name of the file to use for the operation.
 *  @param in_is_text_file    true if the file should be treated as a text file; false
 *                            if it should be considered a binary file.
 *  @param out_opt_result_ptr Deref will be set to an array, holding the read file contents.
 *                            The array must be deleted with a delete[] operator when no
 *                            longer needed. May be nullptr.
 *  @param out_opt_size_ptr   Deref will be set to the number of bytes allocated for @param out_result_ptr.
 *                            May be nullptr.
 **/
bool Anvil::IO::read_file(std::string in_filename,
                          bool        in_is_text_file,
                          char**      out_opt_result_ptr,
                          size_t*     out_opt_size_ptr)
{
    size_t file_size    = 0;
    char*  result       = nullptr;
    bool   result_bool  = false;

    #if defined(_WIN32)
        HANDLE             file_handle;
        LARGE_INTEGER      file_size_large;
        const std::wstring filename_wide   = std::wstring(in_filename.begin(), in_filename.end() );
        DWORD              n_bytes_read    = 0;

        ANVIL_REDUNDANT_ARGUMENT(in_is_text_file);

        file_handle = ::CreateFileW(filename_wide.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    nullptr, /* lpSecurityAttributes */
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr);

        if (file_handle == INVALID_HANDLE_VALUE)
        {
            goto end;
        }

        if (::GetFileSizeEx(file_handle,
                           &file_size_large) == 0)
        {
            goto end;
        }

        file_size = static_cast<size_t>(file_size_large.QuadPart);

        if (out_opt_result_ptr != nullptr)
        {
            result = new char[file_size + 1];

            if (result == nullptr)
            {
                goto end;
            }

            memset(result,
                   0,
                   file_size + 1);

            if (::ReadFile(file_handle,
                           result,
                           static_cast<DWORD>(file_size),
                          &n_bytes_read,
                           nullptr /* lpOverlapped */) != TRUE)
            {
                goto end;
            }

            if (n_bytes_read != file_size)
            {
                goto end;
            }
        }
    #else
        struct stat64 file_stats;
        FILE*         file_handle  = nullptr;
        size_t        n_bytes_read = 0;

        file_handle = fopen64(in_filename.c_str(),
                             (in_is_text_file) ? "rt" : "rb");

        if (file_handle == 0)
        {
            goto end;
        }

        fstat64(fileno(file_handle),
               &file_stats);

        file_size = static_cast<size_t>(file_stats.st_size);

        if (file_size == static_cast<size_t>(-1) ||
            file_size == static_cast<size_t>(0) )
        {
            goto end;
        }

        if (out_opt_result_ptr != nullptr)
        {
            result = new char[file_size + 1];

            if (result == nullptr)
            {
                goto end;
            }

            memset(result,
                   0,
                   file_size + 1);

            n_bytes_read = fread(result,
                                 1,
                                 file_size,
                                 file_handle);

            result[n_bytes_read] = 0;
        }
    #endif

    /* Set the output variables */
    if (out_opt_result_ptr != nullptr)
    {
        *out_opt_result_ptr = result;
    }

    if (out_opt_size_ptr != nullptr)
    {
        *out_opt_size_ptr = file_size;
    }

    result_bool = true;
end:
    if (file_handle != 0)
    {
        #if defined(_WIN32)
            ::CloseHandle(file_handle);
        #else
            fclose(file_handle);
        #endif
    }

    if (!result_bool)
    {
        if (result != nullptr)
        {
            delete [] result;
        }
    }

    return result_bool;
}

bool Anvil::IO::read_file(std::string in_filename,
                          bool        in_is_text_file,
                          size_t      in_start_offset,
                          size_t      in_size,
                          char**      out_result_ptr)
{
    char*  result       = nullptr;
    bool   result_bool  = false;

    #if defined(_WIN32)
        HANDLE             file_handle;
        const std::wstring filename_wide = std::wstring(in_filename.begin(), in_filename.end() );
        DWORD              n_bytes_read  = 0;

        ANVIL_REDUNDANT_ARGUMENT(in_is_text_file);

        file_handle = ::CreateFileW(filename_wide.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    nullptr, /* lpSecurityAttributes */
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr);

        if (file_handle == INVALID_HANDLE_VALUE)
        {
            goto end;
        }

        if (in_start_offset != 0)
        {
            LARGE_INTEGER start_offset_large;

            start_offset_large.QuadPart = in_start_offset;

            if (::SetFilePointerEx(file_handle,
                                   start_offset_large,
                                   nullptr, /* lpNewFilePointer */
                                   FILE_BEGIN) == FALSE)
            {
                goto end;
            }
        }

        result = new char[in_size];

        if (result == nullptr)
        {
            goto end;
        }


        if (::ReadFile(file_handle,
                       result,
                       static_cast<DWORD>(in_size),
                      &n_bytes_read,
                       nullptr /* lpOverlapped */) != TRUE)
        {
            goto end;
        }

        if (n_bytes_read != in_size)
        {
            goto end;
        }
    #else
        FILE*  file_handle  = nullptr;
        size_t n_bytes_read = 0;

        file_handle = fopen64(in_filename.c_str(),
                              (in_is_text_file) ? "rt" : "rb");

        if (file_handle == 0)
        {
            goto end;
        }

        if (fseek(file_handle,
                  in_start_offset,
                  SEEK_END) != 0)
        {
            goto end;
        }

        result = new char[in_size];

        if (result == nullptr)
        {
            goto end;
        }


        n_bytes_read = fread(result,
                             1,
                             in_size,
                             file_handle);

        if (n_bytes_read != in_size)
        {
            goto end;
        }
    #endif

    /* Set the output variables */
    *out_result_ptr = result;

    result_bool = true;
end:
    if (file_handle != 0)
    {
        #if defined(_WIN32)
            ::CloseHandle(file_handle);
        #else
            fclose(file_handle);
        #endif
    }

    if (!result_bool)
    {
        if (result != nullptr)
        {
            delete [] result;
        }
    }

    return result_bool;
}

/** Please see header for specification */
bool Anvil::IO::write_binary_file(std::string  in_filename,
                                  const void*  in_data,
                                  unsigned int in_data_size,
                                  bool         in_should_append)
{
    FILE* file_handle = nullptr;
    bool  result      = false;

    file_handle = fopen(in_filename.c_str(),
                        (in_should_append) ? "ab" : "wb");

    if (file_handle == 0)
    {
        goto end;
    }

    if (fwrite(in_data,
               in_data_size,
               1, /* count */
               file_handle) != 1)
    {
        goto end;
    }

    result = true;
end:
    if (file_handle != 0)
    {
        fclose(file_handle);
    }

    return result;
}

/** Please see header for specification */
bool Anvil::IO::write_text_file(std::string in_filename,
                                std::string in_contents,
                                bool        in_should_append)
{
    FILE* file_handle = nullptr;
    bool  result      = false;

    file_handle = fopen(in_filename.c_str(),
                        (in_should_append) ? "a" : "wt");

    if (file_handle == 0)
    {
        goto end;
    }

    if (fwrite(in_contents.c_str(),
               strlen(in_contents.c_str() ),
               1, /* count */
               file_handle) != 1)
    {
        goto end;
    }

    result = true;
end:
    if (file_handle != 0)
    {
        fclose(file_handle);
    }

    return result;
}
 