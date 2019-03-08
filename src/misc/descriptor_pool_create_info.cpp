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
//

#include "misc/descriptor_pool_create_info.h"

Anvil::DescriptorPoolCreateInfoUniquePtr Anvil::DescriptorPoolCreateInfo::create(const Anvil::BaseDevice*                in_device_ptr,
                                                                                 const uint32_t&                         in_n_max_sets,
                                                                                 const Anvil::DescriptorPoolCreateFlags& in_create_flags,
                                                                                 const Anvil::MTSafety&                  in_mt_safety)
{
    Anvil::DescriptorPoolCreateInfoUniquePtr result_ptr;

    result_ptr.reset(
        new Anvil::DescriptorPoolCreateInfo(in_device_ptr,
                                            in_n_max_sets,
                                            in_create_flags,
                                            in_mt_safety)
    );

    anvil_assert(result_ptr != nullptr);
    return result_ptr;
}

Anvil::DescriptorPoolCreateInfo::DescriptorPoolCreateInfo(const Anvil::BaseDevice*                in_device_ptr,
                                                          const uint32_t&                         in_n_max_sets,
                                                          const Anvil::DescriptorPoolCreateFlags& in_create_flags,
                                                          const Anvil::MTSafety&                  in_mt_safety)
    :m_create_flags                       (in_create_flags),
     m_device_ptr                         (in_device_ptr),
     m_mt_safety                          (in_mt_safety),
     m_n_max_inline_uniform_block_bindings(0),
     m_n_max_sets                         (in_n_max_sets)
{
    /* Stub */
}
