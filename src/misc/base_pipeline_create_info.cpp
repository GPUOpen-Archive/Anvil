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
#include "misc/base_pipeline_create_info.h"
#include "misc/descriptor_set_create_info.h"
#include "wrappers/descriptor_set_group.h"
#include <algorithm>


Anvil::BasePipelineCreateInfo::BasePipelineCreateInfo()
{
    /* Stub */
}

Anvil::BasePipelineCreateInfo::~BasePipelineCreateInfo()
{
    /* Stub */
}

bool Anvil::BasePipelineCreateInfo::add_specialization_constant(Anvil::ShaderStage in_shader_stage,
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
bool Anvil::BasePipelineCreateInfo::attach_push_constant_range(uint32_t                in_offset,
                                                               uint32_t                in_size,
                                                               Anvil::ShaderStageFlags in_stages)
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

void Anvil::BasePipelineCreateInfo::copy_state_from(const BasePipelineCreateInfo* in_src_pipeline_create_info_ptr)
{
    m_base_pipeline_id = in_src_pipeline_create_info_ptr->m_base_pipeline_id;

    for (auto& current_ds_create_info_item_ptr : in_src_pipeline_create_info_ptr->m_ds_create_info_items)
    {
        m_ds_create_info_items.push_back(
            Anvil::DescriptorSetCreateInfoUniquePtr(
                new DescriptorSetCreateInfo(*current_ds_create_info_item_ptr.get() )
            )
        );
    }

    m_push_constant_ranges = in_src_pipeline_create_info_ptr->m_push_constant_ranges;

    m_create_flags  = in_src_pipeline_create_info_ptr->m_create_flags;
    m_shader_stages = in_src_pipeline_create_info_ptr->m_shader_stages;

    m_specialization_constants_data_buffer = in_src_pipeline_create_info_ptr->m_specialization_constants_data_buffer;
    m_specialization_constants_map         = in_src_pipeline_create_info_ptr->m_specialization_constants_map;
}

bool Anvil::BasePipelineCreateInfo::get_shader_stage_properties(Anvil::ShaderStage                  in_shader_stage,
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

bool Anvil::BasePipelineCreateInfo::get_specialization_constants(Anvil::ShaderStage              in_shader_stage,
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

void Anvil::BasePipelineCreateInfo::init(const Anvil::PipelineCreateFlags&                         in_create_flags,
                                         uint32_t                                                  in_n_shader_module_stage_entrypoints,
                                         const ShaderModuleStageEntryPoint*                        in_shader_module_stage_entrypoint_ptrs,
                                         const Anvil::PipelineID*                                  in_opt_base_pipeline_id_ptr,
                                         const std::vector<const Anvil::DescriptorSetCreateInfo*>* in_opt_ds_create_info_vec_ptr)
{
    m_base_pipeline_id = (in_opt_base_pipeline_id_ptr != nullptr) ? *in_opt_base_pipeline_id_ptr : UINT32_MAX;
    m_create_flags     = in_create_flags;
    m_is_proxy         = false;

    if (in_opt_ds_create_info_vec_ptr != nullptr)
    {
        set_descriptor_set_create_info(in_opt_ds_create_info_vec_ptr);
    }

    init_shader_modules(in_n_shader_module_stage_entrypoints,
                        in_shader_module_stage_entrypoint_ptrs);
}

void Anvil::BasePipelineCreateInfo::init_proxy()
{
    m_base_pipeline_id = UINT32_MAX;
    m_create_flags     = Anvil::PipelineCreateFlagBits::NONE;
    m_is_proxy         = true;
}

void Anvil::BasePipelineCreateInfo::init_shader_modules(uint32_t                                  in_n_shader_module_stage_entrypoints,
                                                        const Anvil::ShaderModuleStageEntryPoint* in_shader_module_stage_entrypoint_ptrs)
{
    for (uint32_t n_shader_module_stage_entrypoint = 0;
                  n_shader_module_stage_entrypoint < in_n_shader_module_stage_entrypoints;
                ++n_shader_module_stage_entrypoint)
    {
        const auto& shader_module_stage_entrypoint = in_shader_module_stage_entrypoint_ptrs[n_shader_module_stage_entrypoint];

        if (shader_module_stage_entrypoint.stage == Anvil::ShaderStage::UNKNOWN)
        {
            continue;
        }

        m_shader_stages               [shader_module_stage_entrypoint.stage] = shader_module_stage_entrypoint;
        m_specialization_constants_map[shader_module_stage_entrypoint.stage] = SpecializationConstants();
    }
}

void Anvil::BasePipelineCreateInfo::set_descriptor_set_create_info(const std::vector<const Anvil::DescriptorSetCreateInfo*>* in_ds_create_info_vec_ptr)
{
    const uint32_t n_descriptor_sets = static_cast<uint32_t>(in_ds_create_info_vec_ptr->size() );

    for (uint32_t n_descriptor_set = 0;
                  n_descriptor_set < n_descriptor_sets;
                ++n_descriptor_set)
    {
        const auto reference_ds_create_info_ptr = in_ds_create_info_vec_ptr->at(n_descriptor_set);

        if (reference_ds_create_info_ptr == nullptr)
        {
            m_ds_create_info_items.push_back(
                Anvil::DescriptorSetCreateInfoUniquePtr()
            );

            continue;
        }

        m_ds_create_info_items.push_back(
            Anvil::DescriptorSetCreateInfoUniquePtr(
                new Anvil::DescriptorSetCreateInfo(*reference_ds_create_info_ptr)
            )
        );
    }
}