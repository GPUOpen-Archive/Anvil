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
#include "misc/graphics_pipeline_info.h"

Anvil::GraphicsPipelineInfo::GraphicsPipelineInfo(std::shared_ptr<RenderPass> in_renderpass_ptr,
                                                  SubPassID                   in_subpass_id)
{
    m_alpha_to_coverage_enabled  = false;
    m_alpha_to_one_enabled       = false;
    m_depth_bias_clamp           = 0.0f;
    m_depth_bias_constant_factor = 0.0f;
    m_depth_bias_enabled         = false;
    m_depth_bias_slope_factor    = 1.0f;
    m_depth_bounds_test_enabled  = false;
    m_depth_clamp_enabled        = false;
    m_depth_test_compare_op      = VK_COMPARE_OP_ALWAYS;
    m_depth_test_enabled         = false;
    m_depth_writes_enabled       = false;
    m_enabled_dynamic_states     = 0;
    m_front_face                 = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m_logic_op                   = VK_LOGIC_OP_NO_OP;
    m_logic_op_enabled           = false;
    m_max_depth_bounds           = 1.0f;
    m_min_depth_bounds           = 0.0f;
    m_n_dynamic_scissor_boxes    = 0;
    m_n_dynamic_viewports        = 0;
    m_n_patch_control_points     = 1;
    m_primitive_restart_enabled  = false;
    m_rasterizer_discard_enabled = false;
    m_sample_mask_enabled        = false;
    m_sample_shading_enabled     = false;
    m_stencil_test_enabled       = false;

    m_renderpass_ptr = in_renderpass_ptr;
    m_subpass_id     = in_subpass_id;

    m_stencil_state_back_face.compareMask = ~0u;
    m_stencil_state_back_face.compareOp   = VK_COMPARE_OP_ALWAYS;
    m_stencil_state_back_face.depthFailOp = VK_STENCIL_OP_KEEP;
    m_stencil_state_back_face.failOp      = VK_STENCIL_OP_KEEP;
    m_stencil_state_back_face.passOp      = VK_STENCIL_OP_KEEP;
    m_stencil_state_back_face.reference   = 0;
    m_stencil_state_back_face.writeMask   = ~0u;
    m_stencil_state_front_face            = m_stencil_state_back_face;

    m_rasterization_order = VK_RASTERIZATION_ORDER_STRICT_AMD;

    memset(m_blend_constant,
           0,
           sizeof(m_blend_constant) );

    m_cull_mode          = VK_CULL_MODE_BACK_BIT;
    m_line_width         = 1.0f;
    m_min_sample_shading = 1.0f;
    m_sample_count       = VK_SAMPLE_COUNT_1_BIT;
    m_polygon_mode       = VK_POLYGON_MODE_FILL;
    m_primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_sample_mask        = ~0u;
}

Anvil::GraphicsPipelineInfo::~GraphicsPipelineInfo()
{
    /* Stub */
}

bool Anvil::GraphicsPipelineInfo::add_vertex_attribute(uint32_t           in_location,
                                                       VkFormat           in_format,
                                                       uint32_t           in_offset_in_bytes,
                                                       uint32_t           in_stride_in_bytes,
                                                       VkVertexInputRate  in_step_rate,
                                                       uint32_t           in_explicit_binding_index)
{
    bool result = false;

    #ifdef _DEBUG
    {
        /* Make sure the location is not already referred to by already added attributes */
        for (auto attribute_iterator  = m_attributes.cbegin();
                  attribute_iterator != m_attributes.cend();
                ++attribute_iterator)
        {
            anvil_assert(attribute_iterator->location != in_location);
        }

        /* If an explicit binding has been requested for the new attribute, we need to make sure that any user-specified attributes
         * that refer to this binding specify the same stride and input rate. */
        if (in_explicit_binding_index != UINT32_MAX)
        {
            for (auto attribute_iterator  = m_attributes.cbegin();
                      attribute_iterator != m_attributes.cend();
                    ++attribute_iterator)
            {
                const auto& current_attribute = *attribute_iterator;

                if (current_attribute.explicit_binding_index == in_explicit_binding_index)
                {
                    anvil_assert(current_attribute.rate            == in_step_rate);
                    anvil_assert(current_attribute.stride_in_bytes == in_stride_in_bytes);
                }
            }
        }
    }
    #endif

    /* Add a new vertex attribute descriptor. At this point, we do not differentiate between
     * attributes and bindings. Actual Vulkan attributes and bindings will be created at baking
     * time. */
    m_attributes.push_back(
        InternalVertexAttribute(
            in_explicit_binding_index,
            in_format,
            in_location,
            in_offset_in_bytes,
            in_step_rate,
            in_stride_in_bytes)
    );

    /* All done */
    result = true;

    return result;
}

bool Anvil::GraphicsPipelineInfo::are_depth_writes_enabled() const
{
    return m_depth_writes_enabled;
}

bool Anvil::GraphicsPipelineInfo::copy_gfx_state_from(const Anvil::GraphicsPipelineInfo* in_src_pipeline_info_ptr)
{
    bool result = false;

    if (in_src_pipeline_info_ptr == nullptr)
    {
        anvil_assert(in_src_pipeline_info_ptr != nullptr);

        goto end;
    }

    /* GFX pipeline info-level data */
    m_max_depth_bounds = in_src_pipeline_info_ptr->m_max_depth_bounds;
    m_min_depth_bounds = in_src_pipeline_info_ptr->m_min_depth_bounds;

    m_depth_bias_enabled         = in_src_pipeline_info_ptr->m_depth_bias_enabled;
    m_depth_bias_clamp           = in_src_pipeline_info_ptr->m_depth_bias_clamp;
    m_depth_bias_constant_factor = in_src_pipeline_info_ptr->m_depth_bias_constant_factor;
    m_depth_bias_slope_factor    = in_src_pipeline_info_ptr->m_depth_bias_slope_factor;
    m_depth_test_enabled         = in_src_pipeline_info_ptr->m_depth_test_enabled;
    m_depth_test_compare_op      = in_src_pipeline_info_ptr->m_depth_test_compare_op;

    m_enabled_dynamic_states = in_src_pipeline_info_ptr->m_enabled_dynamic_states;

    m_alpha_to_coverage_enabled  = in_src_pipeline_info_ptr->m_alpha_to_coverage_enabled;
    m_alpha_to_one_enabled       = in_src_pipeline_info_ptr->m_alpha_to_one_enabled;
    m_depth_clamp_enabled        = in_src_pipeline_info_ptr->m_depth_clamp_enabled;
    m_depth_writes_enabled       = in_src_pipeline_info_ptr->m_depth_writes_enabled;
    m_logic_op_enabled           = in_src_pipeline_info_ptr->m_logic_op_enabled;
    m_primitive_restart_enabled  = in_src_pipeline_info_ptr->m_primitive_restart_enabled;
    m_rasterizer_discard_enabled = in_src_pipeline_info_ptr->m_rasterizer_discard_enabled;
    m_sample_mask_enabled        = in_src_pipeline_info_ptr->m_sample_mask_enabled;
    m_sample_shading_enabled     = in_src_pipeline_info_ptr->m_sample_shading_enabled;

    m_stencil_test_enabled     = in_src_pipeline_info_ptr->m_stencil_test_enabled;
    m_stencil_state_back_face  = in_src_pipeline_info_ptr->m_stencil_state_back_face;
    m_stencil_state_front_face = in_src_pipeline_info_ptr->m_stencil_state_front_face;

    m_rasterization_order = in_src_pipeline_info_ptr->m_rasterization_order;

    m_attributes                             = in_src_pipeline_info_ptr->m_attributes;
    m_blend_constant[0]                      = in_src_pipeline_info_ptr->m_blend_constant[0];
    m_blend_constant[1]                      = in_src_pipeline_info_ptr->m_blend_constant[1];
    m_blend_constant[2]                      = in_src_pipeline_info_ptr->m_blend_constant[2];
    m_blend_constant[3]                      = in_src_pipeline_info_ptr->m_blend_constant[3];
    m_polygon_mode                           = in_src_pipeline_info_ptr->m_polygon_mode;
    m_front_face                             = in_src_pipeline_info_ptr->m_front_face;
    m_line_width                             = in_src_pipeline_info_ptr->m_line_width;
    m_logic_op                               = in_src_pipeline_info_ptr->m_logic_op;
    m_min_sample_shading                     = in_src_pipeline_info_ptr->m_min_sample_shading;
    m_n_dynamic_scissor_boxes                = in_src_pipeline_info_ptr->m_n_dynamic_scissor_boxes;
    m_n_dynamic_viewports                    = in_src_pipeline_info_ptr->m_n_dynamic_viewports;
    m_n_patch_control_points                 = in_src_pipeline_info_ptr->m_n_patch_control_points;
    m_primitive_topology                     = in_src_pipeline_info_ptr->m_primitive_topology;
    m_sample_mask                            = in_src_pipeline_info_ptr->m_sample_mask;
    m_scissor_boxes                          = in_src_pipeline_info_ptr->m_scissor_boxes;
    m_subpass_attachment_blending_properties = in_src_pipeline_info_ptr->m_subpass_attachment_blending_properties;
    m_viewports                              = in_src_pipeline_info_ptr->m_viewports;

    m_cull_mode    = in_src_pipeline_info_ptr->m_cull_mode;
    m_sample_count = in_src_pipeline_info_ptr->m_sample_count;

    BasePipelineInfo::copy_state_from(in_src_pipeline_info_ptr);

    result = true;
end:
    return result;
}

std::unique_ptr<Anvil::GraphicsPipelineInfo> Anvil::GraphicsPipelineInfo::create_derivative_pipeline_info(bool                                       in_disable_optimizations,
                                                                                                          bool                                       in_allow_derivatives,
                                                                                                          std::shared_ptr<RenderPass>                in_renderpass_ptr,
                                                                                                          SubPassID                                  in_subpass_id,
                                                                                                          const ShaderModuleStageEntryPoint&         in_fragment_shader_stage_entrypoint_info,
                                                                                                          const ShaderModuleStageEntryPoint&         in_geometry_shader_stage_entrypoint_info,
                                                                                                          const ShaderModuleStageEntryPoint&         in_tess_control_shader_stage_entrypoint_info,
                                                                                                          const ShaderModuleStageEntryPoint&         in_tess_evaluation_shader_stage_entrypoint_info,
                                                                                                          const ShaderModuleStageEntryPoint&         in_vertex_shader_shader_stage_entrypoint_info,
                                                                                                          Anvil::PipelineID                          in_base_pipeline_id)
{
    std::unique_ptr<Anvil::GraphicsPipelineInfo> result_ptr;

    result_ptr.reset(
        new GraphicsPipelineInfo(in_renderpass_ptr,
                                 in_subpass_id)
    );

    if (result_ptr != nullptr)
    {
        const ShaderModuleStageEntryPoint stages[] =
        {
            in_fragment_shader_stage_entrypoint_info,
            in_geometry_shader_stage_entrypoint_info,
            in_tess_control_shader_stage_entrypoint_info,
            in_tess_evaluation_shader_stage_entrypoint_info,
            in_vertex_shader_shader_stage_entrypoint_info
        };

        result_ptr->init_derivative_pipeline_info(in_disable_optimizations,
                                                  in_allow_derivatives,
                                                  sizeof(stages) / sizeof(stages[0]),
                                                  stages,
                                                  in_base_pipeline_id);
    }

    return result_ptr;
}

std::unique_ptr<Anvil::GraphicsPipelineInfo> Anvil::GraphicsPipelineInfo::create_proxy_pipeline_info()
{
    std::unique_ptr<Anvil::GraphicsPipelineInfo> result_ptr;

    result_ptr.reset(
        new GraphicsPipelineInfo(std::shared_ptr<Anvil::RenderPass>(),
                                 UINT32_MAX)
    );

    if (result_ptr != nullptr)
    {
        result_ptr->init_proxy_pipeline_info();
    }

    return result_ptr;
}

std::unique_ptr<Anvil::GraphicsPipelineInfo> Anvil::GraphicsPipelineInfo::create_regular_pipeline_info(bool                               in_disable_optimizations,
                                                                                                       bool                               in_allow_derivatives,
                                                                                                       std::shared_ptr<RenderPass>        in_renderpass_ptr,
                                                                                                       SubPassID                          in_subpass_id,
                                                                                                       const ShaderModuleStageEntryPoint& in_fragment_shader_stage_entrypoint_info,
                                                                                                       const ShaderModuleStageEntryPoint& in_geometry_shader_stage_entrypoint_info,
                                                                                                       const ShaderModuleStageEntryPoint& in_tess_control_shader_stage_entrypoint_info,
                                                                                                       const ShaderModuleStageEntryPoint& in_tess_evaluation_shader_stage_entrypoint_info,
                                                                                                       const ShaderModuleStageEntryPoint& in_vertex_shader_shader_stage_entrypoint_info,
                                                                                                       const Anvil::GraphicsPipelineInfo* in_opt_reference_pipeline_info_ptr)
{
    std::unique_ptr<Anvil::GraphicsPipelineInfo> result_ptr;

    result_ptr.reset(
        new GraphicsPipelineInfo(in_renderpass_ptr,
                                 in_subpass_id)
    );

    if (result_ptr != nullptr)
    {
        const ShaderModuleStageEntryPoint stages[] =
        {
            in_fragment_shader_stage_entrypoint_info,
            in_geometry_shader_stage_entrypoint_info,
            in_tess_control_shader_stage_entrypoint_info,
            in_tess_evaluation_shader_stage_entrypoint_info,
            in_vertex_shader_shader_stage_entrypoint_info
        };

        if (in_opt_reference_pipeline_info_ptr != nullptr)
        {
            if (!result_ptr->copy_gfx_state_from(in_opt_reference_pipeline_info_ptr) )
            {
                anvil_assert_fail();

                result_ptr.reset();
            }
        }

        result_ptr->init_regular_pipeline_info(in_disable_optimizations,
                                               in_allow_derivatives,
                                               sizeof(stages) / sizeof(stages[0]),
                                               stages);
    }

    return result_ptr;
}

void Anvil::GraphicsPipelineInfo::get_blending_properties(const float** out_opt_blend_constant_vec4_ptr_ptr,
                                                          uint32_t*     out_opt_n_blend_attachments_ptr) const
{
    if (out_opt_blend_constant_vec4_ptr_ptr != nullptr)
    {
        *out_opt_blend_constant_vec4_ptr_ptr = m_blend_constant;
    }

    if (out_opt_n_blend_attachments_ptr != nullptr)
    {
        *out_opt_n_blend_attachments_ptr = static_cast<uint32_t>(m_subpass_attachment_blending_properties.size() );
    }
}

bool Anvil::GraphicsPipelineInfo::get_color_blend_attachment_properties(SubPassAttachmentID    in_attachment_id,
                                                                        bool*                  out_opt_blending_enabled_ptr,
                                                                        VkBlendOp*             out_opt_blend_op_color_ptr,
                                                                        VkBlendOp*             out_opt_blend_op_alpha_ptr,
                                                                        VkBlendFactor*         out_opt_src_color_blend_factor_ptr,
                                                                        VkBlendFactor*         out_opt_dst_color_blend_factor_ptr,
                                                                        VkBlendFactor*         out_opt_src_alpha_blend_factor_ptr,
                                                                        VkBlendFactor*         out_opt_dst_alpha_blend_factor_ptr,
                                                                        VkColorComponentFlags* out_opt_channel_write_mask_ptr) const
{
    SubPassAttachmentToBlendingPropertiesMap::const_iterator props_iterator;
    bool                                                     result         = false;

    props_iterator = m_subpass_attachment_blending_properties.find(in_attachment_id);

    if (props_iterator == m_subpass_attachment_blending_properties.end() )
    {
        goto end;
    }

    if (out_opt_blending_enabled_ptr != nullptr)
    {
        *out_opt_blending_enabled_ptr = props_iterator->second.blend_enabled;
    }

    if (out_opt_blend_op_color_ptr != nullptr)
    {
        *out_opt_blend_op_color_ptr = props_iterator->second.blend_op_color;
    }

    if (out_opt_blend_op_alpha_ptr != nullptr)
    {
        *out_opt_blend_op_alpha_ptr = props_iterator->second.blend_op_alpha;
    }

    if (out_opt_src_color_blend_factor_ptr != nullptr)
    {
        *out_opt_src_color_blend_factor_ptr = props_iterator->second.src_color_blend_factor;
    }

    if (out_opt_dst_color_blend_factor_ptr != nullptr)
    {
        *out_opt_dst_color_blend_factor_ptr = props_iterator->second.dst_color_blend_factor;
    }

    if (out_opt_src_alpha_blend_factor_ptr != nullptr)
    {
        *out_opt_src_alpha_blend_factor_ptr = props_iterator->second.src_alpha_blend_factor;
    }

    if (out_opt_dst_alpha_blend_factor_ptr != nullptr)
    {
        *out_opt_dst_alpha_blend_factor_ptr = props_iterator->second.dst_alpha_blend_factor;
    }

    if (out_opt_channel_write_mask_ptr != nullptr)
    {
        *out_opt_channel_write_mask_ptr = props_iterator->second.channel_write_mask;
    }

    result = true;
end:
    return result;
}

void Anvil::GraphicsPipelineInfo::get_depth_bias_state(bool*  out_opt_is_enabled_ptr,
                                                       float* out_opt_depth_bias_constant_factor_ptr,
                                                       float* out_opt_depth_bias_clamp_ptr,
                                                       float* out_opt_depth_bias_slope_factor_ptr) const
{

    if (out_opt_is_enabled_ptr != nullptr)
    {
        *out_opt_is_enabled_ptr = m_depth_bias_enabled;
    }

    if (out_opt_depth_bias_constant_factor_ptr != nullptr)
    {
        *out_opt_depth_bias_constant_factor_ptr = m_depth_bias_constant_factor;
    }

    if (out_opt_depth_bias_clamp_ptr != nullptr)
    {
        *out_opt_depth_bias_clamp_ptr = m_depth_bias_clamp;
    }

    if (out_opt_depth_bias_slope_factor_ptr != nullptr)
    {
        *out_opt_depth_bias_slope_factor_ptr = m_depth_bias_slope_factor;
    }
}

void Anvil::GraphicsPipelineInfo::get_depth_bounds_state(bool*  out_opt_is_enabled_ptr,
                                                         float* out_opt_min_depth_bounds_ptr,
                                                         float* out_opt_max_depth_bounds_ptr) const
{
    if (out_opt_is_enabled_ptr != nullptr)
    {
        *out_opt_is_enabled_ptr = m_depth_bounds_test_enabled;
    }

    if (out_opt_min_depth_bounds_ptr != nullptr)
    {
        *out_opt_min_depth_bounds_ptr = m_min_depth_bounds;
    }

    if (out_opt_max_depth_bounds_ptr != nullptr)
    {
        *out_opt_max_depth_bounds_ptr = m_max_depth_bounds;
    }
}

void Anvil::GraphicsPipelineInfo::get_depth_test_state(bool*        out_opt_is_enabled_ptr,
                                                       VkCompareOp* out_opt_compare_op_ptr) const
{
    if (out_opt_is_enabled_ptr != nullptr)
    {
        *out_opt_is_enabled_ptr = m_depth_test_enabled;
    }

    if (out_opt_compare_op_ptr != nullptr)
    {
        *out_opt_compare_op_ptr = m_depth_test_compare_op;
    }
}

Anvil::DynamicStateBitfield Anvil::GraphicsPipelineInfo::get_enabled_dynamic_states() const
{
    return m_enabled_dynamic_states;
}

void Anvil::GraphicsPipelineInfo::get_graphics_pipeline_properties(uint32_t*                    out_opt_n_scissors_ptr,
                                                                   uint32_t*                    out_opt_n_viewports_ptr,
                                                                   uint32_t*                    out_opt_n_vertex_attributes_ptr,
                                                                   std::shared_ptr<RenderPass>* out_opt_renderpass_ptr,
                                                                   SubPassID*                   out_opt_subpass_id_ptr) const
{
    if (out_opt_n_scissors_ptr != nullptr)
    {
        *out_opt_n_scissors_ptr = static_cast<uint32_t>(m_scissor_boxes.size() );
    }

    if (out_opt_n_viewports_ptr != nullptr)
    {
        *out_opt_n_viewports_ptr = static_cast<uint32_t>(m_viewports.size() );
    }

    if (out_opt_n_vertex_attributes_ptr != nullptr)
    {
        *out_opt_n_vertex_attributes_ptr = static_cast<uint32_t>(m_attributes.size() );
    }

    if (out_opt_renderpass_ptr != nullptr)
    {
        *out_opt_renderpass_ptr = m_renderpass_ptr;
    }

    if (out_opt_subpass_id_ptr != nullptr)
    {
        *out_opt_subpass_id_ptr = m_subpass_id;
    }
}

void Anvil::GraphicsPipelineInfo::get_logic_op_state(bool*      out_opt_is_enabled_ptr,
                                                     VkLogicOp* out_opt_logic_op_ptr) const
{
    if (out_opt_is_enabled_ptr != nullptr)
    {
        *out_opt_is_enabled_ptr = m_logic_op_enabled;
    }

    if (out_opt_logic_op_ptr != nullptr)
    {
        *out_opt_logic_op_ptr = m_logic_op;
    }
}

void Anvil::GraphicsPipelineInfo::get_multisampling_properties(VkSampleCountFlags* out_opt_sample_count_ptr,
                                                               VkSampleMask*       out_opt_sample_mask_ptr) const
{
    if (out_opt_sample_count_ptr != nullptr)
    {
        *out_opt_sample_count_ptr = m_sample_count;
    }

    if (out_opt_sample_mask_ptr != nullptr)
    {
        *out_opt_sample_mask_ptr = m_sample_mask;
    }
}

uint32_t Anvil::GraphicsPipelineInfo::get_n_dynamic_scissor_boxes() const
{
    return m_n_dynamic_scissor_boxes;
}

uint32_t Anvil::GraphicsPipelineInfo::get_n_dynamic_viewports() const
{
    return m_n_dynamic_viewports;
}

uint32_t Anvil::GraphicsPipelineInfo::get_n_scissor_boxes() const
{
    return static_cast<uint32_t>(m_scissor_boxes.size() );
}

uint32_t Anvil::GraphicsPipelineInfo::get_n_viewports() const
{
    return static_cast<uint32_t>(m_viewports.size() );
}

VkPrimitiveTopology Anvil::GraphicsPipelineInfo::get_primitive_topology() const
{
    return m_primitive_topology;
}

VkRasterizationOrderAMD Anvil::GraphicsPipelineInfo::get_rasterization_order() const
{
    return m_rasterization_order;
}

void Anvil::GraphicsPipelineInfo::get_rasterization_properties(VkPolygonMode*   out_opt_polygon_mode_ptr,
                                                               VkCullModeFlags* out_opt_cull_mode_ptr,
                                                               VkFrontFace*     out_opt_front_face_ptr,
                                                               float*           out_opt_line_width_ptr) const
{
    if (out_opt_polygon_mode_ptr != nullptr)
    {
        *out_opt_polygon_mode_ptr = m_polygon_mode;
    }

    if (out_opt_cull_mode_ptr != nullptr)
    {
        *out_opt_cull_mode_ptr = m_cull_mode;
    }

    if (out_opt_front_face_ptr != nullptr)
    {
        *out_opt_front_face_ptr = m_front_face;
    }

    if (out_opt_line_width_ptr != nullptr)
    {
        *out_opt_line_width_ptr = m_line_width;
    }
}

void Anvil::GraphicsPipelineInfo::get_sample_shading_state(bool*  out_opt_is_enabled_ptr,
                                                           float* out_opt_min_sample_shading_ptr) const
{
    if (out_opt_is_enabled_ptr != nullptr)
    {
        *out_opt_is_enabled_ptr = m_sample_shading_enabled;
    }

    if (out_opt_min_sample_shading_ptr != nullptr)
    {
        *out_opt_min_sample_shading_ptr = m_min_sample_shading;
    }
}

bool Anvil::GraphicsPipelineInfo::get_scissor_box_properties(uint32_t  in_n_scissor_box,
                                                             int32_t*  out_opt_x_ptr,
                                                             int32_t*  out_opt_y_ptr,
                                                             uint32_t* out_opt_width_ptr,
                                                             uint32_t* out_opt_height_ptr) const
{
    bool                      result                = false;
    const InternalScissorBox* scissor_box_props_ptr = nullptr;

    if (m_scissor_boxes.find(in_n_scissor_box) == m_scissor_boxes.end() )
    {
        anvil_assert(!(m_scissor_boxes.find(in_n_scissor_box) == m_scissor_boxes.end()) );

        goto end;
    }
    else
    {
        scissor_box_props_ptr = &m_scissor_boxes.at(in_n_scissor_box);
    }

    if (out_opt_x_ptr != nullptr)
    {
        *out_opt_x_ptr = scissor_box_props_ptr->x;
    }

    if (out_opt_y_ptr != nullptr)
    {
        *out_opt_y_ptr = scissor_box_props_ptr->y;
    }

    if (out_opt_width_ptr != nullptr)
    {
        *out_opt_width_ptr = scissor_box_props_ptr->width;
    }

    if (out_opt_height_ptr != nullptr)
    {
        *out_opt_height_ptr = scissor_box_props_ptr->height;
    }

    result = true;
end:
    return result;
}

void Anvil::GraphicsPipelineInfo::get_stencil_test_properties(bool*        out_opt_is_enabled_ptr,
                                                              VkStencilOp* out_opt_front_stencil_fail_op_ptr,
                                                              VkStencilOp* out_opt_front_stencil_pass_op_ptr,
                                                              VkStencilOp* out_opt_front_stencil_depth_fail_op_ptr,
                                                              VkCompareOp* out_opt_front_stencil_compare_op_ptr,
                                                              uint32_t*    out_opt_front_stencil_compare_mask_ptr,
                                                              uint32_t*    out_opt_front_stencil_write_mask_ptr,
                                                              uint32_t*    out_opt_front_stencil_reference_ptr,
                                                              VkStencilOp* out_opt_back_stencil_fail_op_ptr,
                                                              VkStencilOp* out_opt_back_stencil_pass_op_ptr,
                                                              VkStencilOp* out_opt_back_stencil_depth_fail_op_ptr,
                                                              VkCompareOp* out_opt_back_stencil_compare_op_ptr,
                                                              uint32_t*    out_opt_back_stencil_compare_mask_ptr,
                                                              uint32_t*    out_opt_back_stencil_write_mask_ptr,
                                                              uint32_t*    out_opt_back_stencil_reference_ptr) const
{
    if (out_opt_is_enabled_ptr != nullptr)
    {
        *out_opt_is_enabled_ptr = m_stencil_test_enabled;
    }

    if (out_opt_front_stencil_fail_op_ptr != nullptr)
    {
        *out_opt_front_stencil_fail_op_ptr = m_stencil_state_front_face.failOp;
    }

    if (out_opt_front_stencil_pass_op_ptr != nullptr)
    {
        *out_opt_front_stencil_pass_op_ptr = m_stencil_state_front_face.passOp;
    }

    if (out_opt_front_stencil_depth_fail_op_ptr != nullptr)
    {
        *out_opt_front_stencil_depth_fail_op_ptr = m_stencil_state_front_face.depthFailOp;
    }

    if (out_opt_front_stencil_compare_op_ptr != nullptr)
    {
        *out_opt_front_stencil_compare_op_ptr = m_stencil_state_front_face.compareOp;
    }

    if (out_opt_front_stencil_compare_mask_ptr != nullptr)
    {
        *out_opt_front_stencil_compare_mask_ptr = m_stencil_state_front_face.compareMask;
    }

    if (out_opt_front_stencil_write_mask_ptr != nullptr)
    {
        *out_opt_front_stencil_write_mask_ptr = m_stencil_state_front_face.writeMask;
    }

    if (out_opt_front_stencil_reference_ptr != nullptr)
    {
        *out_opt_front_stencil_reference_ptr = m_stencil_state_front_face.reference;
    }

    if (out_opt_back_stencil_fail_op_ptr != nullptr)
    {
        *out_opt_back_stencil_fail_op_ptr = m_stencil_state_back_face.failOp;
    }

    if (out_opt_back_stencil_pass_op_ptr != nullptr)
    {
        *out_opt_back_stencil_pass_op_ptr = m_stencil_state_back_face.passOp;
    }

    if (out_opt_back_stencil_depth_fail_op_ptr != nullptr)
    {
        *out_opt_back_stencil_depth_fail_op_ptr = m_stencil_state_back_face.depthFailOp;
    }

    if (out_opt_back_stencil_compare_op_ptr != nullptr)
    {
        *out_opt_back_stencil_compare_op_ptr = m_stencil_state_back_face.compareOp;
    }

    if (out_opt_back_stencil_compare_mask_ptr != nullptr)
    {
        *out_opt_back_stencil_compare_mask_ptr = m_stencil_state_back_face.compareMask;
    }

    if (out_opt_back_stencil_write_mask_ptr != nullptr)
    {
        *out_opt_back_stencil_write_mask_ptr = m_stencil_state_back_face.writeMask;
    }

    if (out_opt_back_stencil_reference_ptr != nullptr)
    {
        *out_opt_back_stencil_reference_ptr = m_stencil_state_back_face.reference;
    }
}

uint32_t Anvil::GraphicsPipelineInfo::get_n_patch_control_points() const
{
    return m_n_patch_control_points;
}

uint32_t Anvil::GraphicsPipelineInfo::get_n_vertex_attribute_properties() const
{
    return m_attributes.size();
}

bool Anvil::GraphicsPipelineInfo::get_vertex_attribute_properties(uint32_t           in_n_vertex_input_attribute,
                                                                  uint32_t*          out_opt_location_ptr,
                                                                  VkFormat*          out_opt_format_ptr,
                                                                  uint32_t*          out_opt_offset_ptr,
                                                                  uint32_t*          out_opt_explicit_vertex_binding_index_ptr,
                                                                  uint32_t*          out_opt_stride_ptr,
                                                                  VkVertexInputRate* out_opt_rate_ptr) const
{
    const InternalVertexAttribute* attribute_ptr = nullptr;
    bool                           result        = false;

    if (m_attributes.size() < in_n_vertex_input_attribute)
    {
        goto end;
    }
    else
    {
        attribute_ptr = &m_attributes.at(in_n_vertex_input_attribute);
    }

    if (out_opt_location_ptr != nullptr)
    {
        *out_opt_location_ptr = attribute_ptr->location;
    }

    if (out_opt_format_ptr != nullptr)
    {
        *out_opt_format_ptr = attribute_ptr->format;
    }

    if (out_opt_offset_ptr != nullptr)
    {
        *out_opt_offset_ptr = attribute_ptr->offset_in_bytes;
    }

    if (out_opt_explicit_vertex_binding_index_ptr != nullptr)
    {
        *out_opt_explicit_vertex_binding_index_ptr = attribute_ptr->explicit_binding_index;
    }

    if (out_opt_stride_ptr != nullptr)
    {
        *out_opt_stride_ptr = attribute_ptr->stride_in_bytes;
    }

    if (out_opt_rate_ptr != nullptr)
    {
        *out_opt_rate_ptr = attribute_ptr->rate;
    }

    /* All done */
    result = true;
end:
    return result;
}

bool Anvil::GraphicsPipelineInfo::get_viewport_properties(uint32_t in_n_viewport,
                                                          float*   out_opt_origin_x_ptr,
                                                          float*   out_opt_origin_y_ptr,
                                                          float*   out_opt_width_ptr,
                                                          float*   out_opt_height_ptr,
                                                          float*   out_opt_min_depth_ptr,
                                                          float*   out_opt_max_depth_ptr) const
{
    bool                    result              = false;
    const InternalViewport* viewport_props_ptr;

    if (m_viewports.find(in_n_viewport) == m_viewports.end() )
    {
        anvil_assert(!(m_viewports.find(in_n_viewport) == m_viewports.end()) );

        goto end;
    }
    else
    {
        viewport_props_ptr = &m_viewports.at(in_n_viewport);
    }

    if (out_opt_origin_x_ptr != nullptr)
    {
        *out_opt_origin_x_ptr = viewport_props_ptr->origin_x;
    }

    if (out_opt_origin_y_ptr != nullptr)
    {
        *out_opt_origin_y_ptr = viewport_props_ptr->origin_y;
    }

    if (out_opt_width_ptr != nullptr)
    {
        *out_opt_width_ptr = viewport_props_ptr->width;
    }

    if (out_opt_height_ptr != nullptr)
    {
        *out_opt_height_ptr = viewport_props_ptr->height;
    }

    if (out_opt_min_depth_ptr != nullptr)
    {
        *out_opt_min_depth_ptr = viewport_props_ptr->min_depth;
    }

    if (out_opt_max_depth_ptr != nullptr)
    {
        *out_opt_max_depth_ptr = viewport_props_ptr->max_depth;
    }

    result = true;
end:
    return result;
}

bool Anvil::GraphicsPipelineInfo::is_alpha_to_coverage_enabled() const
{
    return m_alpha_to_coverage_enabled;
}

bool Anvil::GraphicsPipelineInfo::is_alpha_to_one_enabled() const
{
    return m_alpha_to_one_enabled;
}

bool Anvil::GraphicsPipelineInfo::is_depth_clamp_enabled() const
{
    return m_depth_clamp_enabled;
}

bool Anvil::GraphicsPipelineInfo::is_primitive_restart_enabled() const
{
    return m_primitive_restart_enabled;
}

bool Anvil::GraphicsPipelineInfo::is_rasterizer_discard_enabled() const
{
    return m_rasterizer_discard_enabled;
}

bool Anvil::GraphicsPipelineInfo::is_sample_mask_enabled() const
{
    return m_sample_mask_enabled;
}

void Anvil::GraphicsPipelineInfo::set_blending_properties(const float* in_blend_constant_vec4)
{
    memcpy(m_blend_constant,
           in_blend_constant_vec4,
           sizeof(m_blend_constant) );
}

void Anvil::GraphicsPipelineInfo::set_color_blend_attachment_properties(SubPassAttachmentID   in_attachment_id,
                                                                        bool                  in_blending_enabled,
                                                                        VkBlendOp             in_blend_op_color,
                                                                        VkBlendOp             in_blend_op_alpha,
                                                                        VkBlendFactor         in_src_color_blend_factor,
                                                                        VkBlendFactor         in_dst_color_blend_factor,
                                                                        VkBlendFactor         in_src_alpha_blend_factor,
                                                                        VkBlendFactor         in_dst_alpha_blend_factor,
                                                                        VkColorComponentFlags in_channel_write_mask)
{
    BlendingProperties* attachment_blending_props_ptr = &m_subpass_attachment_blending_properties[in_attachment_id];

    attachment_blending_props_ptr->blend_enabled          = in_blending_enabled;
    attachment_blending_props_ptr->blend_op_alpha         = in_blend_op_alpha;
    attachment_blending_props_ptr->blend_op_color         = in_blend_op_color;
    attachment_blending_props_ptr->channel_write_mask     = in_channel_write_mask;
    attachment_blending_props_ptr->dst_alpha_blend_factor = in_dst_alpha_blend_factor;
    attachment_blending_props_ptr->dst_color_blend_factor = in_dst_color_blend_factor;
    attachment_blending_props_ptr->src_alpha_blend_factor = in_src_alpha_blend_factor;
    attachment_blending_props_ptr->src_color_blend_factor = in_src_color_blend_factor;
}

void Anvil::GraphicsPipelineInfo::set_multisampling_properties(VkSampleCountFlagBits in_sample_count,
                                                               float                 in_min_sample_shading,
                                                               const VkSampleMask    in_sample_mask)
{
    m_min_sample_shading = in_min_sample_shading;
    m_sample_count       = in_sample_count;
    m_sample_mask        = in_sample_mask;
}

void Anvil::GraphicsPipelineInfo::set_n_dynamic_scissor_boxes(uint32_t in_n_dynamic_scissor_boxes)
{
    m_n_dynamic_scissor_boxes = in_n_dynamic_scissor_boxes;
}

void Anvil::GraphicsPipelineInfo::set_n_dynamic_viewports(uint32_t in_n_dynamic_viewports)
{
    m_n_dynamic_viewports = in_n_dynamic_viewports;
}

void Anvil::GraphicsPipelineInfo::set_n_patch_control_points(uint32_t in_n_patch_control_points)
{
    m_n_patch_control_points = in_n_patch_control_points;
}

void Anvil::GraphicsPipelineInfo::set_primitive_topology(VkPrimitiveTopology in_primitive_topology)
{
    m_primitive_topology = in_primitive_topology;
}

void Anvil::GraphicsPipelineInfo::set_rasterization_order(VkRasterizationOrderAMD in_rasterization_order)
{
    m_rasterization_order = in_rasterization_order;
}

void Anvil::GraphicsPipelineInfo::set_rasterization_properties(VkPolygonMode   in_polygon_mode,
                                                               VkCullModeFlags in_cull_mode,
                                                               VkFrontFace     in_front_face,
                                                               float           in_line_width)
{
    m_cull_mode    = in_cull_mode;
    m_front_face   = in_front_face;
    m_line_width   = in_line_width;
    m_polygon_mode = in_polygon_mode;
}

void Anvil::GraphicsPipelineInfo::set_scissor_box_properties(uint32_t in_n_scissor_box,
                                                             int32_t  in_x,
                                                             int32_t  in_y,
                                                             uint32_t in_width,
                                                             uint32_t in_height)
{
    m_scissor_boxes[in_n_scissor_box] = InternalScissorBox(in_x,
                                                           in_y,
                                                           in_width,
                                                           in_height);
}

void Anvil::GraphicsPipelineInfo::set_stencil_test_properties(bool        in_update_front_face_state,
                                                              VkStencilOp in_stencil_fail_op,
                                                              VkStencilOp in_stencil_pass_op,
                                                              VkStencilOp in_stencil_depth_fail_op,
                                                              VkCompareOp in_stencil_compare_op,
                                                              uint32_t    in_stencil_compare_mask,
                                                              uint32_t    in_stencil_write_mask,
                                                              uint32_t    in_stencil_reference)
{
    VkStencilOpState* const stencil_op_state_ptr = (in_update_front_face_state) ? &m_stencil_state_front_face
                                                                                : &m_stencil_state_back_face;

    stencil_op_state_ptr->compareMask = in_stencil_compare_mask;
    stencil_op_state_ptr->compareOp   = in_stencil_compare_op;
    stencil_op_state_ptr->depthFailOp = in_stencil_depth_fail_op;
    stencil_op_state_ptr->failOp      = in_stencil_fail_op;
    stencil_op_state_ptr->passOp      = in_stencil_pass_op;
    stencil_op_state_ptr->reference   = in_stencil_reference;
    stencil_op_state_ptr->writeMask   = in_stencil_write_mask;
}

void Anvil::GraphicsPipelineInfo::set_viewport_properties(uint32_t in_n_viewport,
                                                          float    in_origin_x,
                                                          float    in_origin_y,
                                                          float    in_width,
                                                          float    in_height,
                                                          float    in_min_depth,
                                                          float    in_max_depth)
{
    m_viewports[in_n_viewport] = InternalViewport(in_origin_x,
                                                  in_origin_y,
                                                  in_width,
                                                  in_height,
                                                  in_min_depth,
                                                  in_max_depth);
}

void Anvil::GraphicsPipelineInfo::toggle_alpha_to_coverage(bool in_should_enable)
{
    m_alpha_to_coverage_enabled = in_should_enable;
}

void Anvil::GraphicsPipelineInfo::toggle_alpha_to_one(bool in_should_enable)
{
    m_alpha_to_one_enabled = in_should_enable;
}

void Anvil::GraphicsPipelineInfo::toggle_depth_bias(bool  in_should_enable,
                                                    float in_depth_bias_constant_factor,
                                                    float in_depth_bias_clamp,
                                                    float in_depth_bias_slope_factor)
{
    m_depth_bias_constant_factor = in_depth_bias_constant_factor;
    m_depth_bias_clamp           = in_depth_bias_clamp;
    m_depth_bias_enabled         = in_should_enable;
    m_depth_bias_slope_factor    = in_depth_bias_slope_factor;
}

void Anvil::GraphicsPipelineInfo::toggle_depth_bounds_test(bool  in_should_enable,
                                                           float in_min_depth_bounds,
                                                           float in_max_depth_bounds)
{
    m_depth_bounds_test_enabled = in_should_enable;
    m_max_depth_bounds          = in_max_depth_bounds;
    m_min_depth_bounds          = in_min_depth_bounds;
}

void Anvil::GraphicsPipelineInfo::toggle_depth_clamp(bool in_should_enable)
{
    m_depth_clamp_enabled = in_should_enable;
}

void Anvil::GraphicsPipelineInfo::toggle_depth_test(bool        in_should_enable,
                                                    VkCompareOp in_compare_op)
{
    m_depth_test_enabled    = in_should_enable;
    m_depth_test_compare_op = in_compare_op;
}

void Anvil::GraphicsPipelineInfo::toggle_depth_writes(bool in_should_enable)
{
    m_depth_writes_enabled = in_should_enable;
}

void Anvil::GraphicsPipelineInfo::toggle_dynamic_states(bool                 in_should_enable,
                                                        DynamicStateBitfield in_dynamic_state_bits)
{
    if (in_should_enable)
    {
        m_enabled_dynamic_states |= in_dynamic_state_bits;
    }
    else
    {
        m_enabled_dynamic_states &= ~in_dynamic_state_bits;
    }
}

void Anvil::GraphicsPipelineInfo::toggle_logic_op(bool      in_should_enable,
                                                  VkLogicOp in_logic_op)
{
    m_logic_op         = in_logic_op;
    m_logic_op_enabled = in_should_enable;
}

void Anvil::GraphicsPipelineInfo::toggle_primitive_restart(bool in_should_enable)
{
    m_primitive_restart_enabled = in_should_enable;
}

void Anvil::GraphicsPipelineInfo::toggle_rasterizer_discard(bool in_should_enable)
{
    m_rasterizer_discard_enabled = in_should_enable;
}

void Anvil::GraphicsPipelineInfo::toggle_sample_mask(bool in_should_enable)
{
    m_sample_mask_enabled = in_should_enable;
}

void Anvil::GraphicsPipelineInfo::toggle_sample_shading(bool in_should_enable)
{
    m_sample_shading_enabled = in_should_enable;
}

void Anvil::GraphicsPipelineInfo::toggle_stencil_test(bool in_should_enable)
{
    m_stencil_test_enabled = in_should_enable;
}
