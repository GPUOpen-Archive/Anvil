//
// Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef ANVIL_GRAPHICS_PIPELINE_CREATE_INFO_H
#define ANVIL_GRAPHICS_PIPELINE_CREATE_INFO_H

#include "misc/base_pipeline_create_info.h"


namespace Anvil
{
    /* By default, graphics pipeline create info uses the following settings:
     *
     *  All rendering modes & tests:          disabled
     *  Blend constant:                       vec4(0.0)
     *  Cull mode:                            VK_CULL_MODE_BACK
     *  Depth bias:                           0.0
     *  Depth bias clamp:                     0.0
     *  Depth bias slope factor:              1.0
     *  Depth test compare op:                Anvil::CompareOp::ALWAYS
     *  Depth writes:                         disabled
     *  Dynamic states:                       all disabled
     *  Fill mode:                            VK_FILL_MODE_SOLID
     *  Front face:                           VK_FRONT_FACE_CCW
     *  Line width:                           1.0
     *  Logic op:                             VK_LOGIC_OP_NOOP;
     *  Max depth boundary:                   1.0
     *  Min depth boundary:                   0.0
     *  Min sample shading:                   1.0
     *  Number of raster samples:             1
     *  Number of tessellation patches:       1
     *  Primitive topology:                   VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
     *  Sample mask:                          0xFFFFFFFF
     *  Slope scaled depth bias:              0.0
     *  Stencil comparison mask (back/front): 0xFFFFFFFF
     *  Stencil comparison op   (back/front): Anvil::CompareOp::ALWAYS
     *  Stencil depth fail op   (back/front): VK_STENCIL_OP_KEEP
     *  Stencil fail op         (back/front): VK_STENCIL_OP_KEEP
     *  Stencil pass op         (back/front): VK_STENCIL_OP_KEEP
     *  Stencil reference value (back/front): 0
     *  Stencil write mask      (back/front): 0xFFFFFFFF
     *
     *  If no scissor or viewport is defined explicitly, one scissor box and one viewport,
     *  covering the whole screen, will be created at baking time.
     *
     *  If VK_AMD_rasterization_order extension is supported:
     *
     *  + Rasterization order: strict
     *
     *  If VK_EXT_depth_clip_enable extension is supported:
     *
     *  + Depth clip enabled: true
     *
     *  If VK_EXT_transform_feedback extension is supported:
     *
     *  + Rasterization stream index: 0
     *
     *  If VK_KHR_maintenance2 extension is supported:
     *
     *  + Tessellation domain origin: upper-left
     */
    class GraphicsPipelineCreateInfo : public BasePipelineCreateInfo
    {
    public:
        /* Public functions */
        static Anvil::GraphicsPipelineCreateInfoUniquePtr create      (const Anvil::PipelineCreateFlags&        in_create_flags,
                                                                       const RenderPass*                        in_renderpass_ptr,
                                                                       SubPassID                                in_subpass_id,
                                                                       const ShaderModuleStageEntryPoint&       in_fragment_shader_stage_entrypoint_info,
                                                                       const ShaderModuleStageEntryPoint&       in_geometry_shader_stage_entrypoint_info,
                                                                       const ShaderModuleStageEntryPoint&       in_tess_control_shader_stage_entrypoint_info,
                                                                       const ShaderModuleStageEntryPoint&       in_tess_evaluation_shader_stage_entrypoint_info,
                                                                       const ShaderModuleStageEntryPoint&       in_vertex_shader_shader_stage_entrypoint_info,
                                                                       const Anvil::GraphicsPipelineCreateInfo* in_opt_reference_pipeline_info_ptr = nullptr,
                                                                       const Anvil::PipelineID*                 in_opt_base_pipeline_id_ptr        = nullptr);
        static Anvil::GraphicsPipelineCreateInfoUniquePtr create_proxy();

        ~GraphicsPipelineCreateInfo();

        /** Adds a new specialization constant.
         *
         *  @param in_shader_stage Shader stage, with which the new specialization constant should be
         *                         associated. Must be one of the graphics shader
         *  @param in_constant_id  ID of the specialization constant to assign data for.
         *  @param in_n_data_bytes Number of bytes under @param in_data_ptr to assign to the specialization constant.
         *  @param in_data_ptr     A buffer holding the data to be assigned to the constant. Must hold at least
         *                         @param in_n_data_bytes bytes that will be read by the function.
         *
         *  @return true if successful, false otherwise.
         **/
        bool add_specialization_constant(Anvil::ShaderStage in_shader_stage,
                                         uint32_t           in_constant_id,
                                         uint32_t           in_n_data_bytes,
                                         const void*        in_data_ptr) final
        {
            if (in_shader_stage != Anvil::ShaderStage::FRAGMENT                &&
                in_shader_stage != Anvil::ShaderStage::GEOMETRY                &&
                in_shader_stage != Anvil::ShaderStage::TESSELLATION_CONTROL    &&
                in_shader_stage != Anvil::ShaderStage::TESSELLATION_EVALUATION &&
                in_shader_stage != Anvil::ShaderStage::VERTEX)
            {
                anvil_assert(in_shader_stage == Anvil::ShaderStage::FRAGMENT                ||
                             in_shader_stage == Anvil::ShaderStage::GEOMETRY                ||
                             in_shader_stage == Anvil::ShaderStage::TESSELLATION_CONTROL    ||
                             in_shader_stage == Anvil::ShaderStage::TESSELLATION_EVALUATION ||
                             in_shader_stage == Anvil::ShaderStage::VERTEX);

                return false;
            }

            return BasePipelineCreateInfo::add_specialization_constant(in_shader_stage,
                                                                       in_constant_id,
                                                                       in_n_data_bytes,
                                                                       in_data_ptr);
        }

        /** Adds a new vertex attribute descriptor to the specified graphics pipeline. This data will be used
         *  at baking time to configure input vertex attribute & bindings for the Vulkan pipeline object.
         *
         *  By default, Anvil only assigns a unique binding to those vertex attributes, whose characteristics
         *  are unique (ie. whose divisor & input rate & stride match). This works well for most of the use cases, the
         *  only exception being when you need to associate a unique offset to a specific vertex binding. In
         *  this case, you need to set @param in_explicit_binding_index to an index, under which your exclusive
         *  binding is going to be stored.
         *  When preparing the binding array, Anvil will not reuse user-specified "explicit" bindings for
         *  attributes, for which "explicit" bindings have not been requested, even if their properties match. 
         *
         *
         *  @param in_location               Vertex attribute location
         *  @param in_format                 Vertex attribute format.
         *  @param in_offset_in_bytes        Start offset of the vertex attribute data.
         *  @param in_stride_in_bytes        Stride of the vertex attribute data.
         *  @param in_step_rate              Step rate to use for the vertex attribute data.
         *  @param in_explicit_binding_index Please see general description of the function for more details.
         *  @param in_divisor                Divisor to use for the attribute. Please read EXT_vertex_attribute_divisor
         *                                   for more details. Only set to values different than 1 if the extension
         *                                   is reported as supported.
         *
         *  @return true if successful, false otherwise.
         **/
        bool add_vertex_attribute(uint32_t               in_location,
                                  Anvil::Format          in_format,
                                  uint32_t               in_offset_in_bytes,
                                  uint32_t               in_stride_in_bytes,
                                  Anvil::VertexInputRate in_step_rate,
                                  uint32_t               in_explicit_binding_index = UINT32_MAX,
                                  uint32_t               in_divisor                = 1);

        /** Tells whether depth writes have been enabled. **/
        bool are_depth_writes_enabled() const;

        /** Retrieves blending properties defined.
         *
         *  @param out_opt_blend_constant_vec4_ptr If not NULL, deref will be assigned to a ptr holding four float values
         *                                         representing the blend constant.
         *  @param out_opt_n_blend_attachments_ptr If not NULL, deref will be set to the number of blend
         *                                         attachments the graphics pipeline supports.
         **/
        void get_blending_properties(const float** out_opt_blend_constant_vec4_ptr_ptr,
                                     uint32_t*     out_opt_n_blend_attachments_ptr) const;

        /** Retrieves color blend attachment properties specified for a given subpass attachment.
         *
         *  @param in_attachment_id                   ID of the attachment the query is being made for.
         *  @param out_opt_blending_enabled_ptr       If not null, deref will be set to true if blending has been
         *                                            enabled for this pipeline, or to false otherwise.
         *  @param out_opt_blend_op_color_ptr         If not null, deref will be set to the specified attachment's
         *                                            color blend op.
         *  @param out_opt_blend_op_alpha_ptr         If not null, deref will be set to the specified attachment's
         *                                            alpha blend op.
         *  @param out_opt_src_color_blend_factor_ptr If not null, deref will be set to the specified attachment's
         *                                            source color blend factor.
         *  @param out_opt_dst_color_blend_factor_ptr If not null, deref will be set to the specified attachment's
         *                                            destination color blend factor.
         *  @param out_opt_src_alpha_blend_factor_ptr If not null, deref will be set to the specified attachment's
         *                                            source alpha blend factor.
         *  @param out_opt_dst_alpha_blend_factor_ptr If not null, deref will be set to the specified attachment's
         *                                            destination alpha blend factor.
         *  @param out_opt_channel_write_mask_ptr     If not null, deref will be set to the specified attachment's
         *                                            write mask.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_color_blend_attachment_properties(SubPassAttachmentID         in_attachment_id,
                                                   bool*                       out_opt_blending_enabled_ptr,
                                                   Anvil::BlendOp*             out_opt_blend_op_color_ptr,
                                                   Anvil::BlendOp*             out_opt_blend_op_alpha_ptr,
                                                   Anvil::BlendFactor*         out_opt_src_color_blend_factor_ptr,
                                                   Anvil::BlendFactor*         out_opt_dst_color_blend_factor_ptr,
                                                   Anvil::BlendFactor*         out_opt_src_alpha_blend_factor_ptr,
                                                   Anvil::BlendFactor*         out_opt_dst_alpha_blend_factor_ptr,
                                                   Anvil::ColorComponentFlags* out_opt_channel_write_mask_ptr) const;

        /** Tells what conservative rasterization mode has been specified for this instance. **/
        Anvil::ConservativeRasterizationModeEXT get_conservative_rasterization_mode() const;

        /** Retrieves depth bias-related state configuration.
         *
         *  @param out_opt_is_enabled_ptr                 If not null, deref will be set to true if depth bias has
         *                                                been enabled for the pipeline, or to false otherwise.
         *  @param out_opt_depth_bias_constant_factor_ptr If not null, deref will be set to the depth bias constant factor
         *                                                assigned to the pipeline.
         *  @param out_opt_depth_bias_clamp_ptr           If not null, deref will be set to the depth bias clamp value, as
         *                                                assigned to the pipeline.
         *  @param out_opt_depth_bias_slope_factor_ptr    If not null, deref will be set to the depth bias slope factor, as
         *                                                configured for the pipeline.
         **/
        void get_depth_bias_state(bool*  out_opt_is_enabled_ptr,
                                  float* out_opt_depth_bias_constant_factor_ptr,
                                  float* out_opt_depth_bias_clamp_ptr,
                                  float* out_opt_depth_bias_slope_factor_ptr) const;

        /** Retrieves depth bounds-related state configuration.
         *
         *  @param out_opt_is_enabled_ptr       If not null, deref will be set to true if depth bounds mode
         *                                      has been enabled for the pipeline, or to false otherwise.
         *  @param out_opt_min_depth_bounds_ptr If not null, deref will be set to the minimum depth bound value
         *                                      assigned to the pipeline.
         *  @param out_opt_max_depth_bounds_ptr If not null, deref will be set to the maximum depth bound value
         *                                      assigned to the pipeline.
         **/
        void get_depth_bounds_state(bool*  out_opt_is_enabled_ptr,
                                    float* out_opt_min_depth_bounds_ptr,
                                    float* out_opt_max_depth_bounds_ptr) const;

        /** Retrieves depth test-related state configuration.
         *
         *  @param out_opt_is_enabled_ptr  If not null, deref will be set to true if depth test has been
         *                                 enabled for the pipeline, or to false otherwise.
         *  @param out_opt_compare_op_ptr  If not null, deref will be set to the depth compare op assigned
         *                                 to the pipeline.
         **/
        void get_depth_test_state(bool*             out_opt_is_enabled_ptr,
                                  Anvil::CompareOp* out_opt_compare_op_ptr) const;

        /** Tells what dynamic states have been enabled. **/
        void get_enabled_dynamic_states(const Anvil::DynamicState** out_dynamic_states_ptr_ptr,
                                        uint32_t*                   out_n_dynamic_states_ptr) const;

        /** Retrieves general properties.
         *
         *  @param out_opt_n_scissors_ptr              If not null, deref will be set to the number of scissors used
         *                                             by the pipeline.
         *  @param out_opt_n_viewports_ptr             If not null, deref will be set to the number of viewports used
         *                                             by the pipeline.
         *  @param out_opt_n_vertex_attributes_ptr     If not null, deref will be set to the number of vertex
         *                                             attributes specified by the owner.
         *  @param out_opt_renderpass_ptr_ptr          If not null, deref will be set to the renderpass assigned to
         *                                             the pipeline.
         *  @param out_opt_subpass_id_ptr              If not null, deref will be set to the ID of the subpass the pipeline
         *                                             has been created for.
         *
         *  @return true if successful, false otherwise.
         **/
        void get_graphics_pipeline_properties(uint32_t*          out_opt_n_scissors_ptr,
                                              uint32_t*          out_opt_n_viewports_ptr,
                                              uint32_t*          out_opt_n_vertex_attributes_ptr,
                                              const RenderPass** out_opt_renderpass_ptr_ptr,
                                              SubPassID*         out_opt_subpass_id_ptr) const;

        /** Retrieves logic op-related state configuration.
         *
         *  @param out_opt_is_enabled_ptr  If not null, deref will be set to true if the logic op has
         *                                 been enabled for the pipeline, or to false otherwise.
         *  @param out_opt_logic_op_ptr    If not null, deref will be set to the logic op enum, specified
         *                                 at creation time.
         **/
        void get_logic_op_state(bool*           out_opt_is_enabled_ptr,
                                Anvil::LogicOp* out_opt_logic_op_ptr) const;

        /** Retrieves multisampling-related state configuration.
         *
         *  @param out_opt_sample_count_ptr If not null, deref will be set to the enum value telling
         *                                  the sample count assigned to the pipeline.
         *  @param out_opt_sample_mask_ptr  If not null, deref will be set to a ptr to the sample mask assigned
         *                                  to the pipeline.
         **/
        void get_multisampling_properties(Anvil::SampleCountFlagBits* out_opt_sample_count_ptr,
                                          const VkSampleMask**        out_opt_sample_mask_ptr_ptr) const;

        /** Tells the number of dynamic scissor boxes. **/
        uint32_t get_n_dynamic_scissor_boxes() const;

        /** Tells the number of dynamic viewports. **/
        uint32_t get_n_dynamic_viewports() const;

        uint32_t get_n_scissor_boxes() const;

        uint32_t get_n_viewports() const;

        /** Tells what primitive topology has been specified for this instance. **/
        Anvil::PrimitiveTopology get_primitive_topology() const;

        /** Tells what rasterization order has been specified for this instance. **/
        Anvil::RasterizationOrderAMD get_rasterization_order() const;

        /** Tells what primitive overestimation size has been specified for this instance. **/
        float get_extra_primitive_overestimation_size() const;

        /** Retrieves various rasterization properties of the graphics pipeline.
         *
         *  @param out_opt_polygon_mode_ptr If not null, deref will be set to polygon mode used by the
         *                                  pipeline.
         *  @param out_opt_cull_mode_ptr    If not null, deref will be set to cull mode used by the
         *                                  pipeline.
         *  @param out_opt_front_face_ptr   If not null, deref will be set to front face setting, used
         *                                  by the pipeline.
         *  @param out_opt_line_width_ptr   If not null, deref will be set to line width setting, as used
         *                                  by the pipeline.
         *
         *  @return true if successful, false otherwise.
         **/
        void get_rasterization_properties(Anvil::PolygonMode*   out_opt_polygon_mode_ptr,
                                          Anvil::CullModeFlags* out_opt_cull_mode_ptr,
                                          Anvil::FrontFace*     out_opt_front_face_ptr,
                                          float*                out_opt_line_width_ptr) const;

        /* Returns rasterization stream index associated with the create info structure. */
        const uint32_t& get_rasterization_stream_index() const
        {
            return m_rasterization_stream_index;
        }

        const RenderPass* get_renderpass() const
        {
            return m_renderpass_ptr;
        }

        /** Retrieves state configuration related to custom sample locations.
         *
         *  @param out_opt_is_enabled_ptr                 If not null, deref will be set to true if custom sample locations have been
         *                                                enabled for the pipeline.
         *  @param out_opt_sample_locations_per_pixel_ptr If not null, deref will be set to the number of sample locations to be used
         *                                                per pixel.
         *  @param out_opt_sample_location_grid_size_ptr  If not null, deref will be set to sample location grid size.
         *  @param out_opt_n_sample_locations_ptr         If not null, deref will be set to the number of sample locations available
         *                                                for reading under *out_opt_sample_locations_ptr
         *  @param out_opt_sample_locations_ptr_ptr       If not null, deref will be set to a ptr to an array holding custom sample
         *                                                locations.
         *
         **/
        void get_sample_location_state(bool*                         out_opt_is_enabled_ptr,
                                       Anvil::SampleCountFlagBits*   out_opt_sample_locations_per_pixel_ptr,
                                       VkExtent2D*                   out_opt_sample_location_grid_size_ptr,
                                       uint32_t*                     out_opt_n_sample_locations_ptr,
                                       const Anvil::SampleLocation** out_opt_sample_locations_ptr_ptr) const;

        /** Retrieves state configuration related to per-sample shading.
         *
         *  @param out_opt_is_enabled_ptr         If not null, deref will be set to true if per-sample shading
         *                                        is enabled for the pipeline.
         *  @param out_opt_min_sample_shading_ptr If not null, deref will be set to the minimum sample shading value,
         *                                        as specified at creation time.
         *
         **/
        void get_sample_shading_state(bool*  out_opt_is_enabled_ptr,
                                      float* out_opt_min_sample_shading_ptr) const;

        /** Retrieves properties of a scissor box at a given index.
         *
         *  @param in_n_scissor_box   Index of the scissor box to retrieve data of.
         *  @param out_opt_x_ptr      If not null, deref will be set to X offset of the scissor box.
         *  @param out_opt_y_ptr      If not null, deref will be set to Y offset of the scissor box.
         *  @param out_opt_width_ptr  If not null, deref will be set to width of the scissor box.
         *  @param out_opt_height_ptr If not null, deref will be set to height of the scissor box.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_scissor_box_properties(uint32_t  in_n_scissor_box,
                                        int32_t*  out_opt_x_ptr,
                                        int32_t*  out_opt_y_ptr,
                                        uint32_t* out_opt_width_ptr,
                                        uint32_t* out_opt_height_ptr) const;

        /** Retrieves stencil test-related state configuration.
         *
         *  @param out_opt_is_enabled_ptr                  If not null, deref will be set to true if stencil test
         *                                                 has been enabled for the pipeline.
         *  @param out_opt_front_stencil_fail_op_ptr       If not null, deref will be set to "front stencil fail operation"
         *                                                 setting, as specified at creation time.
         *  @param out_opt_front_stencil_pass_op_ptr       If not null, deref will be set to "front stencil pass operation"
         *                                                 setting, as specified at creation time.
         *  @param out_opt_front_stencil_depth_fail_op_ptr If not null, deref will be set to "front stencil depth fail operation"
         *                                                 setting, as specified at creation time.
         *  @param out_opt_front_stencil_compare_op_ptr    If not null, deref will be set to "front stencil compare operation"
         *                                                 setting, as specified at creation time.
         *  @param out_opt_front_stencil_compare_mask_ptr  If not null, deref will be set to front stencil compare mask, as
         *                                                 specified at creation time.
         *  @param out_opt_front_stencil_write_mask_ptr    If not null, deref will be set to front stencil write mask, as
         *                                                 specified at creation time.
         *  @param out_opt_front_stencil_reference_ptr     If not null, deref will be set to front stencil reference value, as
         *                                                 specified at creation time.
         *  @param out_opt_back_stencil_fail_op_ptr        If not null, deref will be set to "back stencil fail operation"
         *                                                 setting, as specified at creation time.
         *  @param out_opt_back_stencil_pass_op_ptr        If not null, deref will be set to "back stencil pass operation"
         *                                                 setting, as specified at creation time.
         *  @param out_opt_back_stencil_depth_fail_op_ptr  If not null, deref will be set to "back stencil depth fail operation"
         *                                                 setting, as specified at creation time.
         *  @param out_opt_back_stencil_compare_op_ptr     If not null, deref will be set to "back stencil compare operation"
         *                                                 setting, as specified at creation time.
         *  @param out_opt_back_stencil_compare_mask_ptr   If not null, deref will be set to back stencil compare mask, as
         *                                                 specified at creation time.
         *  @param out_opt_back_stencil_write_mask_ptr     If not null, deref will be set to back stencil write mask, as
         *                                                 specified at creation time.
         *  @param out_opt_back_stencil_reference_ptr      If not null, deref will be set to back stencil reference value, as
         *                                                 specified at creation time.
         *
         *  @return true if successful, false otherwise.
         **/
        void get_stencil_test_properties(bool*             out_opt_is_enabled_ptr,
                                         Anvil::StencilOp* out_opt_front_stencil_fail_op_ptr,
                                         Anvil::StencilOp* out_opt_front_stencil_pass_op_ptr,
                                         Anvil::StencilOp* out_opt_front_stencil_depth_fail_op_ptr,
                                         Anvil::CompareOp* out_opt_front_stencil_compare_op_ptr,
                                         uint32_t*         out_opt_front_stencil_compare_mask_ptr,
                                         uint32_t*         out_opt_front_stencil_write_mask_ptr,
                                         uint32_t*         out_opt_front_stencil_reference_ptr,
                                         Anvil::StencilOp* out_opt_back_stencil_fail_op_ptr,
                                         Anvil::StencilOp* out_opt_back_stencil_pass_op_ptr,
                                         Anvil::StencilOp* out_opt_back_stencil_depth_fail_op_ptr,
                                         Anvil::CompareOp* out_opt_back_stencil_compare_op_ptr,
                                         uint32_t*         out_opt_back_stencil_compare_mask_ptr,
                                         uint32_t*         out_opt_back_stencil_write_mask_ptr,
                                         uint32_t*         out_opt_back_stencil_reference_ptr) const;

        const SubPassID& get_subpass_id() const
        {
            return m_subpass_id;
        }

        /* TODO
         *
         * Requires VK_KHR_maintenance2.
         */
        Anvil::TessellationDomainOrigin get_tessellation_domain_origin() const
        {
            return m_tessellation_domain_origin;
        }

        /** Tells the number of patch control points associated with this instance. **/
        uint32_t get_n_patch_control_points() const;

        /** Returns properties of a vertex attribute at a given index. These properties are as specified by the owner.
         *
         *  This means the binding ultimately assigned by graphics pipeline manager does not necessarily have to match
         *  @param out_opt_binding_ptr.
         *
         *  @param in_n_vertex_input_attribute Index of the vertex attribute to retrieve info of.
         *  @param out_opt_location_ptr                      If not null, deref will be set to the specified attribute's location.
         *  @param out_opt_format_ptr                        If not null, deref will be set to the specified attribute's format.
         *  @param out_opt_offset_ptr                        If not null, deref will be set to the specified attribute's start offset.
         *  @param out_opt_explicit_vertex_binding_index_ptr If not null, deref will be set to the specified attribute's explicit vertex binding index.
         *  @param out_opt_stride_ptr                        If not null, deref will be set to the specified attribute's stride.
         *  @param out_opt_rate_ptr                          If not null, deref will be set to the specified attribute's step rate.
         *  @param out_opt_divisor_ptr                       If not null, deref will be set to the specified attribute's divisor.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_vertex_attribute_properties(uint32_t                in_n_vertex_input_attribute,
                                             uint32_t*               out_opt_location_ptr                      = nullptr,
                                             Anvil::Format*          out_opt_format_ptr                        = nullptr,
                                             uint32_t*               out_opt_offset_ptr                        = nullptr,
                                             uint32_t*               out_opt_explicit_vertex_binding_index_ptr = nullptr,
                                             uint32_t*               out_opt_stride_ptr                        = nullptr,
                                             Anvil::VertexInputRate* out_opt_rate_ptr                          = nullptr,
                                             uint32_t*               out_opt_divisor_ptr                       = nullptr) const;

        /** Retrieves properties of a viewport at a given index.
         *
         *  @param in_n_viewport         Index of the viewport to retrieve properties of.
         *  @param out_opt_origin_x_ptr  If not null, deref will be set to the viewport's X origin.
         *  @param out_opt_origin_y_ptr  If not null, deref will be set to the viewport's Y origin.
         *  @param out_opt_width_ptr     If not null, deref will be set to the viewport's width.
         *  @param out_opt_height_ptr    If not null, deref will be set to the viewport's height.
         *  @param out_opt_min_depth_ptr If not null, deref will be set to the viewport's minimum depth value.
         *  @param out_opt_max_depth_ptr If not null, deref will be set to the viewport's maximum depth value.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_viewport_properties(uint32_t in_n_viewport,
                                     float*   out_opt_origin_x_ptr,
                                     float*   out_opt_origin_y_ptr,
                                     float*   out_opt_width_ptr,
                                     float*   out_opt_height_ptr,
                                     float*   out_opt_min_depth_ptr,
                                     float*   out_opt_max_depth_ptr) const;

        /** Tells if alpha-to-coverage mode has been enabled. **/
        bool is_alpha_to_coverage_enabled() const;

        /** Tells if alpha-to-one mode has been enabled. **/
        bool is_alpha_to_one_enabled() const;

        /** Tells whether depth clamping has been enabled. **/
        bool is_depth_clamp_enabled() const;

        /** Tells whether depth clipping has been enabled. **/
        bool is_depth_clip_enabled() const;

        /** Tells whether primitive restart mode has been enabled. **/
        bool is_primitive_restart_enabled() const;

        /** Tells whether rasterizer discard has been enabled. */
        bool is_rasterizer_discard_enabled() const;

        /** Tells whether sample mask has been enabled. */
        bool is_sample_mask_enabled() const;

        /** Sets a new blend constant.
         *
         *  @param in_blend_constant_vec4 4 floats, specifying the constant. Must not be nullptr.
         *
         **/
        void set_blending_properties(const float* in_blend_constant_vec4);

        /** Updates color blend properties for the specified sub-pass attachment.
         *
         *  @param in_attachment_id           ID of the subpass attachment, for which the color blend properties should be applied.
         *  @param in_blending_enabled        true if blending should be enabled for the specified attachment, false otherwise.
         *  @param in_blend_op_color          Blend operation color to use for the attachment.
         *  @param in_blend_op_alpha          Blend operation alpha to use for the attachment.
         *  @param in_src_color_blend_factor  Source blend factor for color components.
         *  @param in_dst_color_blend_factor  Destination blend factor for color components.
         *  @param in_src_alpha_blend_factor  Source blend factor for the alpha component.
         *  @param in_dst_alpha_blend_factor  Destination blend factor for the alpha component.
         *  @param in_channel_write_mask      Component write mask to use for the attachment.
         *
         **/
        void set_color_blend_attachment_properties(SubPassAttachmentID        in_attachment_id,
                                                   bool                       in_blending_enabled,
                                                   Anvil::BlendOp             in_blend_op_color,
                                                   Anvil::BlendOp             in_blend_op_alpha,
                                                   Anvil::BlendFactor         in_src_color_blend_factor,
                                                   Anvil::BlendFactor         in_dst_color_blend_factor,
                                                   Anvil::BlendFactor         in_src_alpha_blend_factor,
                                                   Anvil::BlendFactor         in_dst_alpha_blend_factor,
                                                   Anvil::ColorComponentFlags in_channel_write_mask);

        /** Updates multisampling properties.
         *
         *  @param in_sample_count       Number of rasterization samples to be used (expressed as one of the enum values).
         *  @param in_min_sample_shading Minimum number of unique samples to shade for each fragment.
         *  @param in_sample_mask        Sample mask to use.
         */
        void set_multisampling_properties(Anvil::SampleCountFlagBits in_sample_count,
                                          float                      in_min_sample_shading,
                                          const VkSampleMask         in_sample_mask);

        /** Updates the number of scissor boxes to be used, when dynamic scissor state is enabled.
         *
         *  @param in_n_dynamic_scissor_boxes As per description.
         */
        void set_n_dynamic_scissor_boxes(uint32_t in_n_dynamic_scissor_boxes);

        /** Updates the number of viewports to be used, when dynamic viewport state is enabled.
         *
         *  @param in_n_dynamic_viewports As per description.
         */
        void set_n_dynamic_viewports(uint32_t in_n_dynamic_viewports);

        /** Updates the number of tessellation patch points.
         *
         *  @param in_n_patch_control_points As per description.
         **/
        void set_n_patch_control_points(uint32_t in_n_patch_control_points);

        /** Sets primitive topology. */
        void set_primitive_topology(Anvil::PrimitiveTopology in_primitive_topology);

        /** Configures the rasterization order for the pipeline if the VK_AMD_rasterization_order extension
         *  is supported by the device, for which the pipeline has been created.
         *
         *  On drivers which do not support the extension, the setting will be ignored.
         *
         *  @param in_rasterization_order  Rasterization order to use.
         **/
        void set_rasterization_order(Anvil::RasterizationOrderAMD in_rasterization_order);

        /** Configures rasterization stream index for the pipeline if VK_EXT_transform_feedback extension 
         *  is supported by the device, for which the pipeline is going to be created.
         *
         *  On drivers not supporting the extension, the setting will be ignored.
         *
         *  @param in_rasterization_stream_index Index to use.
         **/
        void set_rasterization_stream_index(const uint32_t& in_rasterization_stream_index);

        /** Configures the conservative rasterization mode for the pipeline if the VK_EXT_conservative_rasterization
         *  extension is supported by the device, for which the pipeline has been created.
         *
         *  On drivers which do not support the extension, the setting will be ignored.
         *
         *  @param in_conservative_rasterization_mode  Conservative rasterization mode to use.
        **/
        void set_conservative_rasterization_mode(Anvil::ConservativeRasterizationModeEXT in_conservative_rasterization_mode);

        /** if the VK_EXT_conservative_rasterization extension is supported by the device and
         *  Anvil::ConservativeRasterizationModeEXT::OVERESTIMATE conservative rasterization mode is set for the pipeline,
         *  this setting controls extra size in pixels by which the primitive is incresed during conservative rasterization.
         *
         *  On drivers which do not support the extension, the setting will be ignored.
         *
         *  @param extra_primitive_overestimation_size  extra size to increase the primitive
        **/
        void set_extra_primitive_overestimation_size(float extra_primitive_overestimation_size);

        /** Sets a number of rasterization properties to be used for the pipeline.
         *
         *  @param in_polygon_mode Polygon mode to use.
         *  @param in_cull_mode    Cull mode to use.
         *  @param in_front_face   Front face to use.
         *  @param in_line_width   Line width to use.
         **/
        void set_rasterization_properties(Anvil::PolygonMode   in_polygon_mode,
                                          Anvil::CullModeFlags in_cull_mode,
                                          Anvil::FrontFace     in_front_face,
                                          float                in_line_width);

        /** Sets all state related to custom sample locations support.
         *
         *  This information is only used if custom sample locations is enabled (which can be done by calling
         *  toggle_sample_locations().
         *
         *  @param in_sample_locations_per_pixel Number of sample locations to use per pixel.
         *  @param in_sample_location_grid_size  Size of the sample location grid to select custom sample locations for.
         *  @param in_n_sample_locations         Number of sample locations defined under @param in_sample_locations_ptr.
         *                                       Must not be 0.
         *  @param in_sample_locations_ptr       Array of sample locations to use. Must not be nullptr. Must hold at least
         *                                       @param in_n_sample_locations items.
         */
        void set_sample_location_properties(const Anvil::SampleCountFlagBits& in_sample_locations_per_pixel,
                                            const VkExtent2D&                 in_sample_location_grid_size,
                                            const uint32_t&                   in_n_sample_locations,
                                            const Anvil::SampleLocation*      in_sample_locations_ptr);

        /** Sets properties of a scissor box at the specified index.
         *
         *  If @param n_scissor_box is larger than 1, all previous scissor boxes must also be defined
         *  prior to creating a pipeline. Number of scissor boxes must match the number of viewports
         *  defined for the pipeline.
         *
         *  @param in_n_scissor_box Index of the scissor box to be updated.
         *  @param in_x             X offset of the scissor box.
         *  @param in_y             Y offset of the scissor box.
         *  @param in_width         Width of the scissor box.
         *  @param in_height        Height of the scissor box.
         **/
        void set_scissor_box_properties(uint32_t in_n_scissor_box,
                                        int32_t  in_x,
                                        int32_t  in_y,
                                        uint32_t in_width,
                                        uint32_t in_height);

        /** Sets a number of stencil test properties.
         *
         *  @param in_update_front_face_state true if the front face stencil states should be updated; false to update the
         *                                    back stencil states instead.
         *  @param in_stencil_fail_op         Stencil fail operation to use.
         *  @param in_stencil_pass_op         Stencil pass operation to use.
         *  @param in_stencil_depth_fail_op   Stencil depth fail operation to use.
         *  @param in_stencil_compare_op      Stencil compare operation to use.
         *  @param in_stencil_compare_mask    Stencil compare mask to use.
         *  @param in_stencil_write_mask      Stencil write mask to use.
         *  @param in_stencil_reference       Stencil reference value to use.
         **/
        void set_stencil_test_properties(bool             in_update_front_face_state,
                                         Anvil::StencilOp in_stencil_fail_op,
                                         Anvil::StencilOp in_stencil_pass_op,
                                         Anvil::StencilOp in_stencil_depth_fail_op,
                                         Anvil::CompareOp in_stencil_compare_op,
                                         uint32_t         in_stencil_compare_mask,
                                         uint32_t         in_stencil_write_mask,
                                         uint32_t         in_stencil_reference);

        /* TODO
         *
         * Requires VK_KHR_maintenance2.
         */
        void set_tessellation_domain_origin(const Anvil::TessellationDomainOrigin& in_new_origin);

        /** Sets properties of a viewport at the specified index.
         *
         *  If @param in_n_viewport is larger than 1, all previous viewports must also be defined
         *  prior to creating a pipeline. Number of scissor boxes must match the number of viewports
         *  defined for the pipeline.
         *
         *  @param in_n_viewport Index of the viewport, whose properties should be changed.
         *  @param in_origin_x   X offset to use for the viewport's origin.
         *  @param in_origin_y   Y offset to use for the viewport's origin.
         *  @param in_width      Width of the viewport.
         *  @param in_height     Height of the viewport.
         *  @param in_min_depth  Minimum depth value to use for the viewport.
         *  @param in_max_depth  Maximum depth value to use for the viewport.
         **/
        void set_viewport_properties(uint32_t in_n_viewport,
                                     float    in_origin_x,
                                     float    in_origin_y,
                                     float    in_width,
                                     float    in_height,
                                     float    in_min_depth,
                                     float    in_max_depth);

        /** Enables or disables the "alpha to coverage" test.
         *
         *  @param in_should_enable true to enable the test; false to disable it.
         */
        void toggle_alpha_to_coverage(bool in_should_enable);

        /** Enables or disables the "alpha to one" test.
         *
         *  @param in_should_enable true to enable the test; false to disable it.
         */
        void toggle_alpha_to_one(bool in_should_enable);

        /** Enables or disables the "depth bias" mode and updates related state values.
         *
         *  @param in_should_enable              true to enable the mode; false to disable it.
         *  @param in_depth_bias_constant_factor Depth bias constant factor to use for the mode.
         *  @param in_depth_bias_clamp           Depth bias clamp to use for the mode.
         *  @param in_depth_bias_slope_factor    Slope scale for the depth bias to use for the mode.
         */
        void toggle_depth_bias(bool  in_should_enable,
                               float in_depth_bias_constant_factor,
                               float in_depth_bias_clamp,
                               float in_depth_bias_slope_factor);

        /** Enables or disables the "depth bounds" test and updates related state values.
         *
         *  @param in_should_enable    true to enable the test; false to disable it.
         *  @param in_min_depth_bounds Minimum boundary value to use for the test.
         *  @param in_max_depth_bounds Maximum boundary value to use for the test. 
         */
        void toggle_depth_bounds_test(bool  in_should_enable,
                                      float in_min_depth_bounds,
                                      float in_max_depth_bounds);

        /** Enables or disables the "depth clamp" test.
         *
         *  @param in_should_enable true to enable the test; false to disable it.
         */
        void toggle_depth_clamp(bool in_should_enable);

        /** Enables or disables the "depth clip" test.
         *
         *  Requires VK_EXT_depth_clip_enable extension support.
         *
         *  @param in_should_enable true to enable the test; false to disable it.
         */
        void toggle_depth_clip(bool in_should_enable);

        /** Enables or disables the depth test and updates related state values.
         *
         *  @param in_should_enable true to enable the mode; false to disable it.
         *  @param in_compare_op    Compare operation to use for the test.
         */
        void toggle_depth_test(bool             in_should_enable,
                               Anvil::CompareOp in_compare_op);

        /** Enables or disables depth writes.
         *
         *  @param in_should_enable true to enable the writes; false to disable them.
         */
        void toggle_depth_writes(bool in_should_enable);

        /** Enables or disables the specified dynamic states.
         *
         *  @param in_should_enable      true to enable the dynamic state(s) specified by @param in_dynamic_state_bits, false
         *                               disable them if already enabled.
         *  @param in_dynamic_states_ptr Pointer to an array of dynamic states to disable or enable. Must not be nullptr.
         *  @param in_n_dynamic_states   Number of array items available for reading under @param in_dynamic_states_ptr
         **/
        void toggle_dynamic_state (bool                                    in_should_enable,
                                   const Anvil::DynamicState&              in_dynamic_state);
        void toggle_dynamic_states(bool                                    in_should_enable,
                                   const Anvil::DynamicState*              in_dynamic_states_ptr,
                                   const uint32_t&                         in_n_dynamic_states);
        void toggle_dynamic_states(bool                                    in_should_enable,
                                   const std::vector<Anvil::DynamicState>& in_dynamic_states);

        /** Enables or disables logic ops and specifies which logic op should be used.
         *
         *  @param in_should_enable true to enable the mode; false to disable it.
         *  @param in_logic_op      Logic operation type to use.
         */
        void toggle_logic_op(bool           in_should_enable,
                             Anvil::LogicOp in_logic_op);

        /** Enables or disables the "primitive restart" mode.
         *
         *  @param in_should_enable true to enable the mode; false to disable it.
         */
        void toggle_primitive_restart(bool in_should_enable);

        /** Enables or disables the "rasterizer discard" mode.
         *
         *  @param in_should_enable true to enable the test; false to disable it.
         */
        void toggle_rasterizer_discard(bool in_should_enable);

        /** Enables or disables custom sample locations.
         *
         *  NOTE: If you enable the functionality, also make sure to call set_sample_locations_properties()
         *        to configure additional state required at pipeline creation time.
         *
         *  Requires VK_EXT_sample_locations.
         *
         *  @param in_should_enable True to enable custom sample locations, false to disable the functionality.
         */
        void toggle_sample_locations(bool in_should_enable);

        /** Enables or disables the sample mask.
         *
         *  Note: make sure to configure the sample mask using set_multisampling_properties() if you intend to use it.
         *
         *  Note: disabling sample mask will make the manager set VkPipelineMultisampleStateCreateInfo::pSampleMask to a non-null
         *        value at pipeline creation time.
         *
         *  @param in_should_enable true to enable sample mask; false to disable it.
         */
        void toggle_sample_mask(bool in_should_enable);

        /** Enables or disables the "per-sample shading" mode.
         *
         *  @param in_should_enable true to enable the test; false to disable it.
         */
        void toggle_sample_shading(bool in_should_enable);

        /** Enables or disables the stencil test.
         *
         *  @param in_should_enable true to enable the test; false to disable it.
         */
        void toggle_stencil_test(bool in_should_enable);

    private:
        /* Private type definitions */

        /* Defines blending properties for a single subpass attachment. */
        typedef struct BlendingProperties
        {
            Anvil::ColorComponentFlags channel_write_mask;

            bool               blend_enabled;
            Anvil::BlendOp     blend_op_alpha;
            Anvil::BlendOp     blend_op_color;
            Anvil::BlendFactor dst_alpha_blend_factor;
            Anvil::BlendFactor dst_color_blend_factor;
            Anvil::BlendFactor src_alpha_blend_factor;
            Anvil::BlendFactor src_color_blend_factor;

            /** Constructor. */
            BlendingProperties()
            {
                blend_enabled           = false;
                blend_op_alpha          = Anvil::BlendOp::ADD;
                blend_op_color          = Anvil::BlendOp::ADD;
                channel_write_mask      = Anvil::ColorComponentFlagBits::A_BIT |
                                          Anvil::ColorComponentFlagBits::B_BIT |
                                          Anvil::ColorComponentFlagBits::G_BIT |
                                          Anvil::ColorComponentFlagBits::R_BIT;
                dst_alpha_blend_factor  = Anvil::BlendFactor::ONE;
                dst_color_blend_factor  = Anvil::BlendFactor::ONE;
                src_alpha_blend_factor  = Anvil::BlendFactor::ONE;
                src_color_blend_factor  = Anvil::BlendFactor::ONE;
            }

            /** Comparison operator
             *
             *  @param in Object to compare against.
             *
             *  @return true if all states match, false otherwise.
             **/
            bool operator==(const BlendingProperties& in) const
            {
                return (in.blend_enabled          == blend_enabled          &&
                        in.blend_op_alpha         == blend_op_alpha         &&
                        in.blend_op_color         == blend_op_color         &&
                        in.channel_write_mask     == channel_write_mask     &&
                        in.dst_alpha_blend_factor == dst_alpha_blend_factor &&
                        in.dst_color_blend_factor == dst_color_blend_factor &&
                        in.src_alpha_blend_factor == src_alpha_blend_factor &&
                        in.src_color_blend_factor == src_color_blend_factor);
            }
        } BlendingProperties;

        typedef std::map<SubPassAttachmentID, BlendingProperties> SubPassAttachmentToBlendingPropertiesMap;

        /** Defines a single scissor box
         *
         * This descriptor is not exposed to the Vulkan implementation. It is used to form Vulkan-specific
         * descriptors at baking time instead.
         */
        struct InternalScissorBox
        {
            int32_t  x;
            int32_t  y;
            uint32_t width;
            uint32_t height;

            /* Constructor. Should only be used by STL. */
            InternalScissorBox()
            {
                x      = 0;
                y      = 0;
                width  = 32u;
                height = 32u;
            }

            /* Constructor
             *
             * @param in_x      X offset of the scissor box
             * @param in_y      Y offset of the scissor box
             * @param in_width  Width of the scissor box
             * @param in_height Height of the scissor box
             **/
            InternalScissorBox(int32_t  in_x,
                               int32_t  in_y,
                               uint32_t in_width,
                               uint32_t in_height)
            {
                height = in_height;
                width  = in_width;
                x      = in_x;
                y      = in_y;
            }
        };

        /** Defines a single viewport
         *
         * This descriptor is not exposed to the Vulkan implementation. It is used to form Vulkan-specific
         * descriptors at baking time instead.
         */
        typedef struct InternalViewport
        {
            float height;
            float max_depth;
            float min_depth;
            float origin_x;
            float origin_y;
            float width;

            /* Constructor. Should only be used by STL */
            InternalViewport()
            {
                height    =  32.0f;
                max_depth =  1.0f;
                min_depth =  0.0f;
                origin_x  =  0.0f;
                origin_y  =  0.0f;
                width     =  32.0f;
            }

            /* Constructor.
             *
             * @param in_origin_x  Origin X of the viewport.
             * @param in_origin_y  Origin Y of the viewport.
             * @param in_width     Width of the viewport.
             * @param in_height    Height of the viewport.
             * @param in_min_depth Minimum depth value the viewport should use.
             * @param in_max_depth Maximum depth value the viewport should use.
             **/
            InternalViewport(float in_origin_x,
                             float in_origin_y,
                             float in_width,
                             float in_height,
                             float in_min_depth,
                             float in_max_depth)
            {
                height    = in_height;
                max_depth = in_max_depth;
                min_depth = in_min_depth;
                origin_x  = in_origin_x;
                origin_y  = in_origin_y;
                width     = in_width;
            }
        } InternalViewport;

        /** A vertex attribute descriptor. This descriptor is not exposed to the Vulkan implementation. Instead,
         *  its members are used to create Vulkan input attribute & binding descriptors at baking time.
         */
        typedef struct InternalVertexAttribute
        {
            uint32_t               divisor;
            uint32_t               explicit_binding_index;
            Anvil::Format          format;
            uint32_t               location;
            uint32_t               offset_in_bytes;
            Anvil::VertexInputRate rate;
            uint32_t               stride_in_bytes;

            /** Dummy constructor. Should only be used by STL. */
            InternalVertexAttribute()
            {
                divisor                = 1;
                explicit_binding_index = UINT32_MAX;
                format                 = Anvil::Format::UNKNOWN;
                location               = UINT32_MAX;
                offset_in_bytes        = UINT32_MAX;
                rate                   = Anvil::VertexInputRate::UNKNOWN;
                stride_in_bytes        = UINT32_MAX;
            }

            /** Constructor.
             *
             *  @param in_divisor         Divisor to use for the attribute.
             *  @param in_format          Attribute format.
             *  @param in_location        Attribute location.
             *  @param in_offset_in_bytes Start offset in bytes.
             *  @param in_rate            Step rate.
             *  @param in_stride_in_bytes Stride in bytes.
             **/
            InternalVertexAttribute(uint32_t               in_divisor,
                                    uint32_t               in_explicit_binding_index,
                                    Anvil::Format          in_format,
                                    uint32_t               in_location,
                                    uint32_t               in_offset_in_bytes,
                                    Anvil::VertexInputRate in_rate,
                                    uint32_t               in_stride_in_bytes)
            {
                divisor                = in_divisor;
                explicit_binding_index = in_explicit_binding_index;
                format                 = in_format;
                location               = in_location;
                offset_in_bytes        = in_offset_in_bytes;
                rate                   = in_rate;
                stride_in_bytes        = in_stride_in_bytes;
            }
        } InternalVertexAttribute;

        typedef std::map<uint32_t, InternalScissorBox> InternalScissorBoxes;
        typedef std::vector<InternalVertexAttribute>   InternalVertexAttributes;
        typedef std::map<uint32_t, InternalViewport>   InternalViewports;

        /* Private functions */
        explicit GraphicsPipelineCreateInfo(const RenderPass* in_renderpass_ptr,
                                            SubPassID         in_subpass_id);

        bool copy_gfx_state_from(const Anvil::GraphicsPipelineCreateInfo* in_src_pipeline_create_info_ptr);

        /* Private variables */
        bool m_depth_clip_enabled;

        bool  m_depth_bounds_test_enabled;
        float m_max_depth_bounds;
        float m_min_depth_bounds;

        bool  m_depth_bias_enabled;
        float m_depth_bias_clamp;
        float m_depth_bias_constant_factor;
        float m_depth_bias_slope_factor;

        bool             m_depth_test_enabled;
        Anvil::CompareOp m_depth_test_compare_op;

        std::vector<DynamicState> m_enabled_dynamic_states;

        bool m_alpha_to_coverage_enabled;
        bool m_alpha_to_one_enabled;
        bool m_depth_clamp_enabled;
        bool m_depth_writes_enabled;
        bool m_logic_op_enabled;
        bool m_primitive_restart_enabled;
        bool m_rasterizer_discard_enabled;
        bool m_sample_locations_enabled;
        bool m_sample_mask_enabled;
        bool m_sample_shading_enabled;

        bool             m_stencil_test_enabled;
        VkStencilOpState m_stencil_state_back_face;
        VkStencilOpState m_stencil_state_front_face;

        VkExtent2D                         m_sample_location_grid_size;
        std::vector<Anvil::SampleLocation> m_sample_locations;
        Anvil::SampleCountFlagBits         m_sample_locations_per_pixel;

        Anvil::RasterizationOrderAMD m_rasterization_order;

        Anvil::ConservativeRasterizationModeEXT m_conservative_rasterization_mode;
        float                                   m_extra_primitive_overestimation_size;

        TessellationDomainOrigin m_tessellation_domain_origin;

        InternalVertexAttributes                 m_attributes;
        float                                    m_blend_constant[4];
        Anvil::CullModeFlags                     m_cull_mode;
        Anvil::PolygonMode                       m_polygon_mode;
        Anvil::FrontFace                         m_front_face;
        float                                    m_line_width;
        Anvil::LogicOp                           m_logic_op;
        float                                    m_min_sample_shading;
        uint32_t                                 m_n_dynamic_scissor_boxes;
        uint32_t                                 m_n_dynamic_viewports;
        uint32_t                                 m_n_patch_control_points;
        Anvil::PrimitiveTopology                 m_primitive_topology;
        uint32_t                                 m_rasterization_stream_index;
        Anvil::SampleCountFlagBits               m_sample_count;
        VkSampleMask                             m_sample_mask;
        InternalScissorBoxes                     m_scissor_boxes;
        SubPassAttachmentToBlendingPropertiesMap m_subpass_attachment_blending_properties;
        InternalViewports                        m_viewports;

        const RenderPass* m_renderpass_ptr;
        SubPassID         m_subpass_id;
    };

};

#endif /* ANVIL_GRAPHICS_PIPELINE_CREATE_INFO_H */
