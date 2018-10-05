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
#include "misc/compute_pipeline_create_info.h"

Anvil::ComputePipelineCreateInfo::ComputePipelineCreateInfo()
{
    /* Stub */
}

Anvil::ComputePipelineCreateInfo::~ComputePipelineCreateInfo()
{
    /* Stub */
}

Anvil::ComputePipelineCreateInfoUniquePtr Anvil::ComputePipelineCreateInfo::create(const Anvil::PipelineCreateFlags&  in_create_flags,
                                                                                   const ShaderModuleStageEntryPoint& in_compute_shader_stage_entrypoint_info,
                                                                                   const Anvil::PipelineID*           in_opt_base_pipeline_id_ptr)
{
    Anvil::ComputePipelineCreateInfoUniquePtr result_ptr(nullptr,
                                                         std::default_delete<Anvil::ComputePipelineCreateInfo>() );

    result_ptr.reset(
        new ComputePipelineCreateInfo()
    );

    if (result_ptr != nullptr)
    {
        result_ptr->init(in_create_flags,
                         1, /* in_n_shader_module_stage_entrypoints */
                        &in_compute_shader_stage_entrypoint_info,
                         in_opt_base_pipeline_id_ptr);
    }

    return result_ptr;
}

Anvil::ComputePipelineCreateInfoUniquePtr Anvil::ComputePipelineCreateInfo::create_proxy()
{
    Anvil::ComputePipelineCreateInfoUniquePtr result_ptr(nullptr,
                                                         std::default_delete<Anvil::ComputePipelineCreateInfo>() );

    result_ptr.reset(
        new ComputePipelineCreateInfo()
    );

    if (result_ptr != nullptr)
    {
        result_ptr->init_proxy();
    }

    return result_ptr;
}
