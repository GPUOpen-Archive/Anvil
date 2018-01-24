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
#include "misc/compute_pipeline_info.h"

Anvil::ComputePipelineInfo::ComputePipelineInfo()
{
    /* Stub */
}

Anvil::ComputePipelineInfo::~ComputePipelineInfo()
{
    /* Stub */
}

std::unique_ptr<Anvil::ComputePipelineInfo> Anvil::ComputePipelineInfo::create_derivative_pipeline_info(bool                               in_disable_optimizations,
                                                                                                        bool                               in_allow_derivatives,
                                                                                                        const ShaderModuleStageEntryPoint& in_compute_shader_stage_entrypoint_info,
                                                                                                        Anvil::PipelineID                  in_base_pipeline_id)
{
    std::unique_ptr<Anvil::ComputePipelineInfo> result_ptr;

    result_ptr.reset(
        new ComputePipelineInfo()
    );

    if (result_ptr != nullptr)
    {
        result_ptr->init_derivative_pipeline_info(in_disable_optimizations,
                                                  in_allow_derivatives,
                                                  1, /* in_n_shader_module_stage_entrypoints */
                                                 &in_compute_shader_stage_entrypoint_info,
                                                  in_base_pipeline_id);
    }

    return result_ptr;
}

std::unique_ptr<Anvil::ComputePipelineInfo> Anvil::ComputePipelineInfo::create_proxy_pipeline_info()
{
    std::unique_ptr<Anvil::ComputePipelineInfo> result_ptr;

    result_ptr.reset(
        new ComputePipelineInfo()
    );

    if (result_ptr != nullptr)
    {
        result_ptr->init_proxy_pipeline_info();
    }

    return result_ptr;
}

std::unique_ptr<Anvil::ComputePipelineInfo> Anvil::ComputePipelineInfo::create_regular_pipeline_info(bool                               in_disable_optimizations,
                                                                                                     bool                               in_allow_derivatives,
                                                                                                     const ShaderModuleStageEntryPoint& in_compute_shader_stage_entrypoint_info)
{
    std::unique_ptr<Anvil::ComputePipelineInfo> result_ptr;

    result_ptr.reset(
        new ComputePipelineInfo()
    );

    if (result_ptr != nullptr)
    {
        result_ptr->init_regular_pipeline_info(in_disable_optimizations,
                                               in_allow_derivatives,
                                               1, /* in_n_shader_module_stage_entrypoints */
                                              &in_compute_shader_stage_entrypoint_info);
    }

    return result_ptr;
}