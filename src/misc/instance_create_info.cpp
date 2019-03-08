//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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

#include "misc/instance_create_info.h"

Anvil::InstanceCreateInfo::InstanceCreateInfo(const std::string&              in_app_name,
                                              const std::string&              in_engine_name,
                                              Anvil::DebugCallbackFunction    in_opt_validation_callback_proc,
                                              bool                            in_mt_safe,
                                              const std::vector<std::string>& in_opt_disallowed_instance_level_extensions)
    :m_api_version                         (Anvil::APIVersion::UNKNOWN),
     m_app_name                            (in_app_name),
     m_app_version                         (0),
     m_disallowed_instance_level_extensions(in_opt_disallowed_instance_level_extensions),
     m_engine_name                         (in_engine_name),
     m_engine_version                      (0),
     m_is_mt_safe                          (in_mt_safe),
     m_n_memory_type_to_use_for_all_alocs  (UINT32_MAX),
     m_validation_callback                 (in_opt_validation_callback_proc)
{
    /* Stub */
}

Anvil::InstanceCreateInfoUniquePtr Anvil::InstanceCreateInfo::create(const std::string&              in_app_name,
                                                                     const std::string&              in_engine_name,
                                                                     Anvil::DebugCallbackFunction    in_opt_validation_callback_proc,
                                                                     bool                            in_mt_safe,
                                                                     const std::vector<std::string>& in_opt_disallowed_instance_level_extensions)
{
    Anvil::InstanceCreateInfoUniquePtr result_ptr;

    result_ptr.reset(
        new Anvil::InstanceCreateInfo(in_app_name,
                                      in_engine_name,
                                      in_opt_validation_callback_proc,
                                      in_mt_safe,
                                      in_opt_disallowed_instance_level_extensions)
    );
    anvil_assert(result_ptr != nullptr);

    return result_ptr;
}
