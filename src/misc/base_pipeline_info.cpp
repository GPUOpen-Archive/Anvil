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
#include "misc/base_pipeline_info.h"
#include <algorithm>


Anvil::BasePipelineInfo::BasePipelineInfo()
{
    /* Stub */
}

Anvil::BasePipelineInfo::~BasePipelineInfo()
{
    /* Stub */
}

bool Anvil::BasePipelineInfo::add_specialization_constant(Anvil::ShaderStage in_shader_stage,
                                                          uint32_t           in_constant_id,
                                                          uint32_t           in_n_data_bytes,
                                                          const void*        in_data_ptr)
{
    uint32_t data_buffer_size = 0;
    bool     result           = false;

    if (in_n_data_bytes == 0)
    {
        anvil_assert(!(in_n_data_bytes == 0) );

        goto end;
    }

    if (in_data_ptr == nullptr)
    {
        anvil_assert(!(in_data_ptr == nullptr) );

        goto end;
    }

    /* Append specialization constant data and add a new descriptor. */
    data_buffer_size = static_cast<uint32_t>(m_specialization_constants_data_buffer.size() );


    anvil_assert(m_specialization_constants_map.find(in_shader_stage) != m_specialization_constants_map.end() );

    m_specialization_constants_map[in_shader_stage].push_back(
        SpecializationConstant(
            in_constant_id,
            in_n_data_bytes,
            data_buffer_size)
    );

    m_specialization_constants_data_buffer.resize(data_buffer_size + in_n_data_bytes);


    memcpy(&m_specialization_constants_data_buffer.at(data_buffer_size),
           in_data_ptr,
           in_n_data_bytes);

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineInfo::attach_push_constant_range(uint32_t           in_offset,
                                                         uint32_t           in_size,
                                                         VkShaderStageFlags in_stages)
{
    bool result = false;

    /* Retrieve pipeline's descriptor and add the specified push constant range */
    const auto new_descriptor = Anvil::PushConstantRange(in_offset,
                                                         in_size,
                                                         in_stages);

    if (std::find(m_push_constant_ranges.begin(),
                  m_push_constant_ranges.end(),
                  new_descriptor) != m_push_constant_ranges.end() )
    {
        anvil_assert_fail();

        goto end;
    }

    m_push_constant_ranges.push_back(new_descriptor);

    /* All done */
    result = true;

end:
    return result;
}

void Anvil::BasePipelineInfo::copy_state_from(const BasePipelineInfo* in_src_pipeline_info_ptr)
{
    m_base_pipeline_id = in_src_pipeline_info_ptr->m_base_pipeline_id;

    m_dsg_ptr              = in_src_pipeline_info_ptr->m_dsg_ptr;
    m_push_constant_ranges = in_src_pipeline_info_ptr->m_push_constant_ranges;

    m_allow_derivatives     = in_src_pipeline_info_ptr->m_allow_derivatives;
    m_disable_optimizations = in_src_pipeline_info_ptr->m_disable_optimizations;
    m_shader_stages         = in_src_pipeline_info_ptr->m_shader_stages;

    m_specialization_constants_data_buffer = in_src_pipeline_info_ptr->m_specialization_constants_data_buffer;
    m_specialization_constants_map         = in_src_pipeline_info_ptr->m_specialization_constants_map;
}

bool Anvil::BasePipelineInfo::get_shader_stage_properties(Anvil::ShaderStage                  in_shader_stage,
                                                          const ShaderModuleStageEntryPoint** out_opt_result_ptr_ptr) const
{
    bool result                = false;
    auto shader_stage_iterator = m_shader_stages.find(in_shader_stage);

    if (shader_stage_iterator != m_shader_stages.end() )
    {
        if (out_opt_result_ptr_ptr != nullptr)
        {
            *out_opt_result_ptr_ptr = &shader_stage_iterator->second;
        }

        result = true;
    }

    return result;
}

bool Anvil::BasePipelineInfo::get_specialization_constants(Anvil::ShaderStage              in_shader_stage,
                                                           const SpecializationConstants** out_opt_spec_constants_ptr,
                                                           const unsigned char**           out_opt_spec_constants_data_buffer_ptr) const
{
    bool       result          = false;
    const auto sc_map_iterator = m_specialization_constants_map.find(in_shader_stage);

    if (sc_map_iterator != m_specialization_constants_map.end() )
    {
        if (out_opt_spec_constants_ptr != nullptr)
        {
            *out_opt_spec_constants_ptr = &sc_map_iterator->second;
        }

        if (out_opt_spec_constants_data_buffer_ptr != nullptr)
        {
            *out_opt_spec_constants_data_buffer_ptr = (m_specialization_constants_data_buffer.size() > 0) ? &m_specialization_constants_data_buffer.at(0)
                                                                                                          : nullptr;
        }

        result = true;
    }

    return result;
}

void Anvil::BasePipelineInfo::init_derivative_pipeline_info(bool                                       in_disable_optimizations,
                                                            bool                                       in_allow_derivatives,
                                                            uint32_t                                   in_n_shader_module_stage_entrypoints,
                                                            const ShaderModuleStageEntryPoint*         in_shader_module_stage_entrypoint_ptrs,
                                                            Anvil::PipelineID                          in_base_pipeline_id,
                                                            std::shared_ptr<Anvil::DescriptorSetGroup> in_opt_dsg_ptr)
{
    m_allow_derivatives     = in_allow_derivatives;
    m_base_pipeline_id      = in_base_pipeline_id;
    m_disable_optimizations = in_disable_optimizations;
    m_is_proxy              = false;

    if (in_opt_dsg_ptr != nullptr)
    {
        m_dsg_ptr = in_opt_dsg_ptr;
    }

    init_shader_modules(in_n_shader_module_stage_entrypoints,
                        in_shader_module_stage_entrypoint_ptrs);
}

void Anvil::BasePipelineInfo::init_proxy_pipeline_info()
{
    m_allow_derivatives     = false;
    m_base_pipeline_id      = UINT32_MAX;
    m_disable_optimizations = false;
    m_is_proxy              = true;
}

void Anvil::BasePipelineInfo::init_regular_pipeline_info(bool                                       in_disable_optimizations,
                                                         bool                                       in_allow_derivatives,
                                                         uint32_t                                   in_n_shader_module_stage_entrypoints,
                                                         const ShaderModuleStageEntryPoint*         in_shader_module_stage_entrypoint_ptrs,
                                                         std::shared_ptr<Anvil::DescriptorSetGroup> in_opt_dsg_ptr)
{
    m_allow_derivatives     = in_allow_derivatives;
    m_base_pipeline_id      = UINT32_MAX;
    m_disable_optimizations = in_disable_optimizations;
    m_is_proxy              = false;

    if (in_opt_dsg_ptr != nullptr)
    {
        m_dsg_ptr = in_opt_dsg_ptr;
    }

    init_shader_modules(in_n_shader_module_stage_entrypoints,
                        in_shader_module_stage_entrypoint_ptrs);
}

void Anvil::BasePipelineInfo::init_shader_modules(uint32_t                                  in_n_shader_module_stage_entrypoints,
                                                  const Anvil::ShaderModuleStageEntryPoint* in_shader_module_stage_entrypoint_ptrs)
{
    for (uint32_t n_shader_module_stage_entrypoint = 0;
                  n_shader_module_stage_entrypoint < in_n_shader_module_stage_entrypoints;
                ++n_shader_module_stage_entrypoint)
    {
        const auto& shader_module_stage_entrypoint = in_shader_module_stage_entrypoint_ptrs[n_shader_module_stage_entrypoint];

        if (shader_module_stage_entrypoint.stage == Anvil::SHADER_STAGE_UNKNOWN)
        {
            continue;
        }

        m_shader_stages               [shader_module_stage_entrypoint.stage] = shader_module_stage_entrypoint;
        m_specialization_constants_map[shader_module_stage_entrypoint.stage] = SpecializationConstants();
    }
}

void Anvil::BasePipelineInfo::set_dsg(std::shared_ptr<DescriptorSetGroup> in_dsg_ptr)
{
    m_dsg_ptr = in_dsg_ptr;
}