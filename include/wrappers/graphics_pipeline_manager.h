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

/** Graphics pipeline manager. A class which inherits from the base pipeline object manager.
 *
 *  Apart from exposing the functionality offered by the parent class under slightly
 *  renamed, pipeline-specific function names, this wrapper implements the following features:
 *
 * - baking of the graphics pipeline object
 * - pipeline properties are assigned default values, as described below. They can be
 *   adjusted by calling relevant entrypoints, prior to baking.
 *
 *  Each baked graphics pipeline is configured as below at Pipeline object creation time:
 *
 *  All rendering modes & tests:          disabled
 *  Blend constant:                       vec4(0.0)
 *  Cull mode:                            VK_CULL_MODE_BACK
 *  Depth bias:                           0.0
 *  Depth bias clamp:                     0.0
 *  Depth bias slope factor:              1.0
 *  Depth test compare op:                VK_COMPARE_OP_ALWAYS
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
 *  Number of tessellation patches:       0
 *  Primitive topology:                   VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
 *  Sample mask:                          0xFFFFFFFF
 *  Slope scaled depth bias:              0.0
 *  Stencil comparison mask (back/front): 0xFFFFFFFF
 *  Stencil comparison op   (back/front): VK_COMPARE_OP_ALWAYS
 *  Stencil depth fail op   (back/front): VK_STENCIL_OP_KEEP
 *  Stencil fail op         (back/front): VK_STENCIL_OP_KEEP
 *  Stencil pass op         (back/front): VK_STENCIL_OP_KEEP
 *  Stencil reference value (back/front): 0
 *  Stencil write mask      (back/front): 0xFFFFFFFF
 *
 *  If no scissor or viewport is defined explicitly, one scissor box and one viewport,
 *  covering the whole screen, will be created at baking time.
 *
 *  If VK_AMD_rasterization_order extension is supported, strict rasterization order is assumed
 *  for the pipeline by default.
 *
 **/
#ifndef WRAPPERS_GRAPHICS_PIPELINE_MANAGER_H
#define WRAPPERS_GRAPHICS_PIPELINE_MANAGER_H

#include "misc/base_pipeline_manager.h"
#include "misc/types.h"
#include "wrappers/render_pass.h"
#include <map>

namespace Anvil
{
    class GraphicsPipelineManager : public BasePipelineManager
    {
    public:
        /* Public type definitions */
        typedef enum
        {
            DYNAMIC_STATE_BLEND_CONSTANTS_BIT      = 1 << 0,
            DYNAMIC_STATE_DEPTH_BIAS_BIT           = 1 << 1,
            DYNAMIC_STATE_DEPTH_BOUNDS_BIT         = 1 << 2,
            DYNAMIC_STATE_LINE_WIDTH_BIT           = 1 << 3,
            DYNAMIC_STATE_SCISSOR_BIT              = 1 << 4,
            DYNAMIC_STATE_STENCIL_COMPARE_MASK_BIT = 1 << 5,
            DYNAMIC_STATE_STENCIL_REFERENCE_BIT    = 1 << 6,
            DYNAMIC_STATE_STENCIL_WRITE_MASK_BIT   = 1 << 7,
            DYNAMIC_STATE_VIEWPORT_BIT             = 1 << 8,
        } DynamicStateBits;

        typedef uint32_t DynamicStateBitfield;

        /* Prototype for a call-back function, invoked right after vkCreateGraphicsPipelines() call returns. **/
        typedef void (*PFNPIPELINEPOSTBAKECALLBACKPROC)(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                        GraphicsPipelineID               in_pipeline_id,
                                                        void*                            in_user_arg);

        /* Prototype for a call-back function, invoked before a Vulkan graphics pipeline is created.
         *
         * The caller can adjust any of the VkGraphicsPipelineCreateInfo fields, with an assumption
         * Anvil::GraphicsPipelineManager will not adjust its internal state to sync with user-modified
         * fields.
         **/
        typedef void (*PFNPIPELINEPREBAKECALLBACKPROC)(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                       GraphicsPipelineID               in_pipeline_id,
                                                       VkGraphicsPipelineCreateInfo*    in_graphics_pipeline_create_info_ptr,
                                                       void*                            in_user_arg);

        /* Public functions */

        /** Creates a new GraphicsPipelineManager instance.
         *
         *  @param in_device_ptr                  Device to use.
         *  @param in_use_pipeline_cache          true if the manager should use a pipeline cache instance. false
         *                                        to pass nullptr whenever a Vulkan descriptor requires us to specify
         *                                        one.
         *  @param in_pipeline_cache_to_reuse_ptr if @param use_pipeline_cache is true, this argument can be optionally
         *                                        set to a non-nullptr value to point at an already allocated pipeline cache.
         *                                        If one is not provided and the other argument is set as described,
         *                                        a new pipeline cache with size 0 will be allocated.
         **/
        static std::shared_ptr<GraphicsPipelineManager> create(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                                                               bool                                  in_use_pipeline_cache          = false,
                                                               std::shared_ptr<Anvil::PipelineCache> in_pipeline_cache_to_reuse_ptr = std::shared_ptr<Anvil::PipelineCache>() );

        /** Destructor. */
        virtual ~GraphicsPipelineManager();

        /** Adds a new derivative pipeline, to be used the specified renderpass' subpass. All states of the base pipeline
         *  will be copied to the new pipeline.
         *
         *  @param in_disable_optimizations                        If true, the pipeline will be created with the 
         *                                                         VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT flag.
         *  @param in_allow_derivatives                            If true, the pipeline will be created with the
         *                                                         VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT flag.
         *  @param in_renderpass_ptr                               Renderpass instance the pipeline is to be created for. Must not be nullptr.
         *                                                         The renderpass instance will be implicitly retained.
         *  @param in_subpass_id                                   ID of the subpass the pipeline is to be created for.
         *  @param in_fragment_shader_stage_entrypoint_info        Fragment shader to use for the pipeline. Pass an instance initialized with
         *                                                         dummy constructor if the stage is irrelevant.
         *  @param in_geometry_shader_stage_entrypoint_info        Geometry shader to use for the pipeline. Pass an instance initialized with
         *                                                         dummy constructor if the stage is irrelevant.
         *  @param in_tess_control_shader_stage_entrypoint_info    Tessellation control shader to use for the pipeline. Pass an instance initialized with
         *                                                         dummy constructor if the stage is irrelevant.
         *  @param in_tess_evaluation_shader_stage_entrypoint_info Tessellation evaluation shader to use for the pipeline. Pass an instance initialized
         *                                                         with dummy constructor if the stage is irrelevant.
         *  @param in_vertex_shader_stage_entrypoint_info          Vertex shader to use for the pipeline. Must not be a dummy shader stage entrypoint
         *                                                         descriptor.
         *  @param in_base_pipeline_id                             Graphics pipeline ID of the base graphics pipeline. The ID must have been returned
         *                                                         by one of the preceding add_() calls, issued against the same GraphicsPipelineManager
         *                                                         instance. Must not refer to a proxy pipeline.
         *  @param out_graphics_pipeline_id_ptr                    Deref will be set to the new graphics pipeline's ID. Must not be nullptr.
         **/
        bool add_derivative_pipeline_from_sibling_pipeline(bool                               in_disable_optimizations,
                                                           bool                               in_allow_derivatives,
                                                           std::shared_ptr<RenderPass>        in_renderpass_ptr,
                                                           SubPassID                          in_subpass_id,
                                                           const ShaderModuleStageEntryPoint& in_fragment_shader_stage_entrypoint_info,
                                                           const ShaderModuleStageEntryPoint& in_geometry_shader_stage_entrypoint_info,
                                                           const ShaderModuleStageEntryPoint& in_tess_control_shader_stage_entrypoint_info,
                                                           const ShaderModuleStageEntryPoint& in_tess_evaluation_shader_stage_entrypoint_info,
                                                           const ShaderModuleStageEntryPoint& in_vertex_shader_shader_stage_entrypoint_info,
                                                           GraphicsPipelineID                 in_base_pipeline_id,
                                                           GraphicsPipelineID*                out_graphics_pipeline_id_ptr);

        /** Adds a new derivative pipeline, to be used the specified renderpass' subpass. Since the base pipeline is
         *  a Vulkan object, its state values are NOT automatically copied to the new derivative pipeline.
         *
         *  @param in_disable_optimizations                        If true, the pipeline will be created with the 
         *                                                         VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT flag.
         *  @param in_allow_derivatives                            If true, the pipeline will be created with the
         *                                                         VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT flag.
         *  @param in_renderpass_ptr                               Renderpass instance the pipeline is to be created for. Must not be nullptr.
         *                                                         The renderpass instance will be implicitly retained.
         *  @param in_subpass_id                                   ID of the subpass the pipeline is to be created for.
         *  @param in_fragment_shader_stage_entrypoint_info        Fragment shader to use for the pipeline. Pass an instance initialized with
         *                                                         dummy constructor if the stage is irrelevant.
         *  @param in_geometry_shader_stage_entrypoint_info        Geometry shader to use for the pipeline. Pass an instance initialized with
         *                                                         dummy constructor if the stage is irrelevant.
         *  @param in_tess_control_shader_stage_entrypoint_info    Tessellation control shader to use for the pipeline. Pass an instance initialized with
         *                                                         dummy constructor if the stage is irrelevant.
         *  @param in_tess_evaluation_shader_stage_entrypoint_info Tessellation evaluation shader to use for the pipeline. Pass an instance initialized
         *                                                         with dummy constructor if the stage is irrelevant.
         *  @param in_vertex_shader_stage_entrypoint_info          Vertex shader to use for the pipeline. Must not be a dummy shader stage descriptor.
         *  @param in_base_pipeline                                Base graphics pipeline. Must not be nullptr.
         *  @param out_graphics_pipeline_id_ptr                    Deref will be set to the new graphics pipeline's ID. Must not be nullptr.
        **/
        bool add_derivative_pipeline_from_pipeline(bool                               in_disable_optimizations,
                                                   bool                               in_allow_derivatives,
                                                   std::shared_ptr<RenderPass>        in_renderpass_ptr,
                                                   SubPassID                          in_subpass_id,
                                                   const ShaderModuleStageEntryPoint& in_fragment_shader_stage_entrypoint_info,
                                                   const ShaderModuleStageEntryPoint& in_geometry_shader_stage_entrypoint_info,
                                                   const ShaderModuleStageEntryPoint& in_tess_control_shader_stage_entrypoint_info,
                                                   const ShaderModuleStageEntryPoint& in_tess_evaluation_shader_stage_entrypoint_info,
                                                   const ShaderModuleStageEntryPoint& in_vertex_shader_shader_stage_entrypoint_info,
                                                   VkPipeline                         in_base_pipeline,
                                                   GraphicsPipelineID*                out_graphics_pipeline_id_ptr);

        /**  Registers a new proxy pipeline. A proxy pipeline cannot be baked, but it can hold state data and act
         *   as a parent for other pipelines, which inherit their state at creation time.
         *
         *  @param out_proxy_graphics_pipeline_id_ptr Deref will be set to the ID of the result pipeline object.
         *                                            Must not be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool add_proxy_pipeline(GraphicsPipelineID* out_proxy_graphics_pipeline_id_ptr);

        /** Adds a new non-derivative pipeline, to be used the specified renderpass' subpass.
         *
         *  @param in_disable_optimizations                        If true, the pipeline will be created with the 
         *                                                         VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT flag.
         *  @param in_allow_derivatives                            If true, the pipeline will be created with the
         *                                                         VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT flag.
         *  @param in_renderpass_ptr                               Renderpass instance the pipeline is to be created for. Must not be nullptr.
         *                                                         The renderpass instance will be implicitly retained.
         *  @param in_subpass_id                                   ID of the subpass the pipeline is to be created for.
         *  @param in_fragment_shader_stage_entrypoint_info        Fragment shader to use for the pipeline. Pass an instance initialized with
         *                                                         dummy constructor if the stage is irrelevant.
         *  @param in_geometry_shader_stage_entrypoint_info        Geometry shader to use for the pipeline. Pass an instance initialized with
         *                                                         dummy constructor if the stage is irrelevant.
         *  @param in_tess_control_shader_stage_entrypoint_info    Tessellation control shader to use for the pipeline. Pass an instance initialized with
         *                                                         dummy constructor if the stage is irrelevant.
         *  @param in_tess_evaluation_shader_stage_entrypoint_info Tessellation evaluation shader to use for the pipeline. Pass an instance initialized
         *                                                         with dummy constructor if the stage is irrelevant.
         *  @param in_vertex_shader_stage_entrypoint_info          Vertex shader to use for the pipeline. Must not be a dummy shader stage descriptor.
         *  @param out_graphics_pipeline_id_ptr                    Deref will be set to the new graphics pipeline's ID. Must not be nullptr.
         **/
        bool add_regular_pipeline(bool                               in_disable_optimizations,
                                  bool                               in_allow_derivatives,
                                  std::shared_ptr<RenderPass>        in_renderpass_ptr,
                                  SubPassID                          in_subpass_id,
                                  const ShaderModuleStageEntryPoint& in_fragment_shader_stage_entrypoint_info,
                                  const ShaderModuleStageEntryPoint& in_geometry_shader_stage_entrypoint_info,
                                  const ShaderModuleStageEntryPoint& in_tess_control_shader_stage_entrypoint_info,
                                  const ShaderModuleStageEntryPoint& in_tess_evaluation_shader_stage_entrypoint_info,
                                  const ShaderModuleStageEntryPoint& in_vertex_shader_shader_stage_entrypoint_info,
                                  GraphicsPipelineID*                out_graphics_pipeline_id_ptr);

        /** Deletes an existing pipeline.
        *
        *  @param in_graphics_pipeline_id ID of a pipeline to delete.
        *
        *  @return true if successful, false otherwise.
        **/
        bool delete_pipeline(GraphicsPipelineID in_graphics_pipeline_id);

        /** Adds a new vertex attribute descriptor to the specified graphics pipeline. This data will be used
         *  at baking time to configure input vertex attribute & bindings for the Vulkan pipeline object.
         *
         *  By default, Anvil only assigns a unique binding to those vertex attributes, whose characteristics
         *  are unique (ie. whose input rate & stride match). This works well for most of the use cases, the
         *  only exception being when you need to associate a unique offset to a specific vertex binding. In
         *  this case, you need to set @param in_explicit_binding_index to an index, under which your exclusive
         *  binding is going to be stored.
         *  When preparing the binding array, Anvil will not reuse user-specified "explicit" bindings for
         *  attributes, for which "explicit" bindings have not been requested, even if their properties match. 
         *
         *
         *  @param in_graphics_pipeine_id    ID of the graphics pipeline object, whose state should be changed. The ID
         *                                   must have been returned by one of the add_() functions, issued against the
         *                                   same GraphicsPipelineManager instance.
         *  @param in_location               Vertex attribute location
         *  @param in_format                 Vertex attribute format.
         *  @param in_offset_in_bytes        Start offset of the vertex attribute data.
         *  @param in_stride_in_bytes        Stride of the vertex attribute data.
         *  @param in_step_rate              Step rate to use for the vertex attribute data.
         *  @param in_explicit_binding_index Please see general description of the function for more details.
         *
         *  @return true if successful, false otherwise.
         **/
        bool add_vertex_attribute(GraphicsPipelineID in_graphics_pipeline_id,
                                  uint32_t           in_location,
                                  VkFormat           in_format,
                                  uint32_t           in_offset_in_bytes,
                                  uint32_t           in_stride_in_bytes,
                                  VkVertexInputRate  in_step_rate,
                                  uint32_t           in_explicit_binding_index = UINT32_MAX);

        /** Generates a VkPipeline instance for each pipeline object marked as dirty. If a dirty pipeline
         *  has already been baked in the past, the former object instance is released.
         *
         *  @return true if successful, false otherwise.
         **/
        bool bake();

        /** Tells whether the graphics pipeline has been created with enabled alpha-to-coverage mode.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr  If not NULL, deref will be set to true if the mode has been
         *                                 enabled for the pipeline, or to false otherwise.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_alpha_to_coverage_state(GraphicsPipelineID in_graphics_pipeline_id,
                                         bool*              out_opt_is_enabled_ptr) const;

        /** Tells whether the graphics pipeline has been created with enabled alpha-to-one mode.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr  If not NULL, deref will be set to true if the mode has been
         *                                 enabled for the pipeline, or to false otherwise.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_alpha_to_one_state(GraphicsPipelineID in_graphics_pipeline_id,
                                    bool*              out_opt_is_enabled_ptr) const;

        /** Retrieves blending properties of the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id         ID of the graphics pipeline the query is being made for.
         *  @param out_opt_blend_constant_vec4_ptr If not NULL, deref will be assigned four float values
         *                                         representing the blend constant.
         *  @param out_opt_n_blend_attachments_ptr If not NULL, deref will be set to the number of blend
         *                                         attachments the graphics pipeline supports.
         *
         *  @return true if successful, false otherwise
         **/
        bool get_blending_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                     float*             out_opt_blend_constant_vec4_ptr,
                                     uint32_t*          out_opt_n_blend_attachments_ptr) const;

        /** Retrieves color blend attachment properties for the specified graphics pipeline and subpass
         *  attachment.
         *
         *  @param in_graphics_pipeline_id            ID of the graphics pipeline the query is being made for.
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
        bool get_color_blend_attachment_properties(GraphicsPipelineID     in_graphics_pipeline_id,
                                                   SubPassAttachmentID    in_attachment_id,
                                                   bool*                  out_opt_blending_enabled_ptr,
                                                   VkBlendOp*             out_opt_blend_op_color_ptr,
                                                   VkBlendOp*             out_opt_blend_op_alpha_ptr,
                                                   VkBlendFactor*         out_opt_src_color_blend_factor_ptr,
                                                   VkBlendFactor*         out_opt_dst_color_blend_factor_ptr,
                                                   VkBlendFactor*         out_opt_src_alpha_blend_factor_ptr,
                                                   VkBlendFactor*         out_opt_dst_alpha_blend_factor_ptr,
                                                   VkColorComponentFlags* out_opt_channel_write_mask_ptr) const;

        /** Retrieves depth bias-related state configuration for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id                ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr                 If not null, deref will be set to true if depth bias has
         *                                                been enabled for the pipeline, or to false otherwise.
         *  @param out_opt_depth_bias_constant_factor_ptr If not null, deref will be set to the depth bias constant factor
         *                                                assigned to the pipeline.
         *  @param out_opt_depth_bias_clamp_ptr           If not null, deref will be set to the depth bias clamp value, as
         *                                                assigned to the pipeline.
         *  @param out_opt_depth_bias_slope_factor_ptr    If not null, deref will be set to the depth bias slope factor, as
         *                                                configured for the pipeline.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_depth_bias_state(GraphicsPipelineID in_graphics_pipeline_id,
                                  bool*              out_opt_is_enabled_ptr,
                                  float*             out_opt_depth_bias_constant_factor_ptr,
                                  float*             out_opt_depth_bias_clamp_ptr,
                                  float*             out_opt_depth_bias_slope_factor_ptr) const;

        /** Retrieves depth bounds-related state configuration for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id      ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr       If not null, deref will be set to true if depth bounds mode
         *                                      has been enabled for the pipeline, or to false otherwise.
         *  @param out_opt_min_depth_bounds_ptr If not null, deref will be set to the minimum depth bound value
         *                                      assigned to the pipeline.
         *  @param out_opt_max_depth_bounds_ptr If not null, deref will be set to the maximum depth bound value
         *                                      assigned to the pipeline.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_depth_bounds_state(GraphicsPipelineID in_graphics_pipeline_id,
                                    bool*              out_opt_is_enabled_ptr,
                                    float*             out_opt_min_depth_bounds_ptr,
                                    float*             out_opt_max_depth_bounds_ptr) const;

        /** Tells whether the graphics pipeline has been created with enabled depth clamping.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr  If not NULL, deref will be set to true if depth clampnig has
         *                                 been enabled for the pipeline, or to false otherwise.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_depth_clamp_state(GraphicsPipelineID in_graphics_pipeline_id,
                                   bool*              out_opt_is_enabled_ptr) const;

        /** Retrieves depth test-related state configuration for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr  If not null, deref will be set to true if depth test has been
         *                                 enabled for the pipeline, or to false otherwise.
         *  @param out_opt_compare_op_ptr  If not null, deref will be set to the depth compare op assigned
         *                                 to the pipeline.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_depth_test_state(GraphicsPipelineID in_graphics_pipeline_id,
                                  bool*              out_opt_is_enabled_ptr,
                                  VkCompareOp*       out_opt_compare_op_ptr) const;

        /** Tells whether the graphics pipeline has been created with enabled depth writes.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr  If not NULL, deref will be set to true if depth writes have
         *                                 been enabled for the pipeline, or to false otherwise.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_depth_write_state(GraphicsPipelineID in_graphics_pipeline_id,
                                   bool*              out_opt_is_enabled_ptr) const;

        /** Tells the number of dynamic scissor boxes, as specified at graphics pipeline instantiation time
         *
         *  @param in_graphics_pipeline_id             ID of the graphics pipeline the query is being made for.
         *  @param out_opt_n_dynamic_scissor_boxes_ptr If not null, deref will be set to the number of dynamic
         *                                             scissor boxes used by the pipeline.
         *
         *  @return true if successful, false otherwise.
         *
         **/
        bool get_dynamic_scissor_state_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                                  uint32_t*          out_opt_n_dynamic_scissor_boxes_ptr) const;

        /** Tells what dynamic states have been enabled for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id            ID of the graphics pipeline the query is being made for.
         *  @param out_opt_enabled_dynamic_states_ptr If not null, deref will be updated with a bitfield value,
         *                                            telling which dynamic states have been enabled for the pipeline.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_dynamic_states(GraphicsPipelineID    in_graphics_pipeline_id,
                                DynamicStateBitfield* out_opt_enabled_dynamic_states_ptr) const;

        /** Tells the number of dynamic viewports, as specified at graphics pipeline instantiation time
         *
         *  @param in_graphics_pipeline_id         ID of the graphics pipeline the query is being made for.
         *  @param out_opt_n_dynamic_viewports_ptr If not null, deref will be set to the number of dynamic viewports
         *                                         used by the pipeline.
         *
         *  @return true if successful, false otherwise.
         *
         **/
        bool get_dynamic_viewport_state_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                                   uint32_t*          out_n_dynamic_viewports_ptr) const;

        /** Retrieves general pipeline properties
         *
         *  @param in_graphics_pipeline_id               ID of the graphics pipeline the query is being made for.
         *  @param out_opt_n_scissors_ptr                If not null, deref will be set to the number of scissors used
         *                                               by the pipeline.
         *  @param out_opt_n_viewports_ptr               If not null, deref will be set to the number of viewports used
         *                                               by the pipeline.
         *  @param out_opt_n_vertex_input_attributes_ptr If not null, deref will be set to the number of vertex
         *                                               attributes used by the pipeline.
         *  @param out_opt_n_vertex_input_bindings_ptr   If not null, deref will be set to the number of vertex bindings
         *                                               defined for the pipeline.
         *  @param out_opt_renderpass_ptr                If not null, deref will be set to the renderpass assigned to
         *                                               the pipeline.
         *  @param out_opt_subpass_id_ptr                If not null, deref will be set to the ID of the subpass the pipeline
         *                                               has been created for.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_graphics_pipeline_properties(Anvil::GraphicsPipelineID    in_graphics_pipeline_id,
                                              uint32_t*                    out_opt_n_scissors_ptr,
                                              uint32_t*                    out_opt_n_viewports_ptr,
                                              uint32_t*                    out_opt_n_vertex_input_attributes_ptr,
                                              uint32_t*                    out_opt_n_vertex_input_bindings_ptr,
                                              std::shared_ptr<RenderPass>* out_opt_renderpass_ptr,
                                              SubPassID*                   out_opt_subpass_id_ptr) const;

        /** Returns a VkPipeline instance for the specified graphics pipeline ID. If the pipeline is marked as dirty,
         *  the Vulkan object will be created before returning after former instance is released.
         *
         *  @param in_pipeline_id ID of the pipeline to return the VkPipeline instance for. Must not describe
         *                        a proxy pipeline.
         *
         *  @return As per description.
         **/
        VkPipeline get_graphics_pipeline(GraphicsPipelineID in_pipeline_id);

        /** Retrieves a PipelineLayout instance associated with the specified pipeline ID.
         *
         *  The function will bake a pipeline object (and, possibly, a pipeline layout object, too) if
         *  the specified pipeline is marked as dirty.
         *
         *  @param in_pipeline_id ID of the pipeline to return the wrapper instance for.
         *                        Must not describe a proxy pipeline.
         *
         *  @return Ptr to a PipelineLayout instance or nullptr if the function failed.
         **/
        std::shared_ptr<Anvil::PipelineLayout> get_graphics_pipeline_layout(GraphicsPipelineID in_pipeline_id);

        /** Tells what primitive topology the specified graphics pipeline has been created for.
         *
         *  @param in_graphics_pipeline_id        ID of the graphics pipeline the query is being made for.
         *  @param out_opt_primitive_topology_ptr If not null, deref will be set to primitive topology, as specified
         *                                        at creation time.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_input_assembly_properties(GraphicsPipelineID   in_graphics_pipeline_id,
                                           VkPrimitiveTopology* out_opt_primitive_topology_ptr) const;

        /** Retrieves logic op-related state configuration for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr  If not null, deref will be set to true if the logic op has
         *                                 been enabled for the pipeline, or to false otherwise.
         *  @param out_opt_logic_op_ptr    If not null, deref will be set to the logic op enum, specified
         *                                 at creation time.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_logic_op_state(GraphicsPipelineID in_graphics_pipeline_id,
                                bool*              out_opt_is_enabled_ptr,
                                VkLogicOp*         out_opt_logic_op_ptr) const;

        /** Retrieves multisampling-related state configuration for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id  ID of the graphics pipeline the query is being made for.
         *  @param out_opt_sample_count_ptr If not null, deref will be set to the enum value telling
         *                                  the sample count assigned to the pipeline.
         *  @param out_opt_sample_mask_ptr  If not null, deref will be set to the sample mask assigned
         *                                  to the pipeline.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_multisampling_properties(GraphicsPipelineID  in_graphics_pipeline_id,
                                          VkSampleCountFlags* out_opt_sample_count_ptr,
                                          VkSampleMask*       out_opt_sample_mask_ptr) const;

        /** Tells whether the graphics pipeline has been created with enabled primitive restart mode.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr  If not NULL, deref will be set to true if primitive restart has
         *                                 been enabled for the pipeline, or to false otherwise.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_primitive_restart_state(GraphicsPipelineID in_graphics_pipeline_id,
                                         bool*              out_opt_is_enabled_ptr) const;

        /** Tells what rasterization order the graphics pipeline has been created for.
         *
         *  @param in_graphics_pipeline_id         ID of the graphics pipeline the query is being made for.
         *  @param out_opt_rasterization_order_ptr If not null, deref will be set to the rasterization order
         *                                         assigned to the pipeline.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_rasterization_order(GraphicsPipelineID       in_graphics_pipeline_id,
                                     VkRasterizationOrderAMD* out_opt_rasterization_order_ptr) const;

        /** Tells whether the graphics pipeline has been created with enabled rasterizer discard mode.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr  If not NULL, deref will be set to true if the mode has been
         *                                 enabled for the pipeline, or to false otherwise.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_rasterizer_discard_state(GraphicsPipelineID in_graphics_pipeline_id,
                                          bool*              out_opt_is_enabled_ptr) const;

        /** Retrieves various properties of the specified graphics pipeline which are related to
         *  rasterization.
         *
         *  @param in_graphics_pipeline_id  ID of the graphics pipeline the query is being made for.
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
        bool get_rasterization_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                          VkPolygonMode*     out_opt_polygon_mode_ptr,
                                          VkCullModeFlags*   out_opt_cull_mode_ptr,
                                          VkFrontFace*       out_opt_front_face_ptr,
                                          float*             out_opt_line_width_ptr) const;

        /** Retrieves state configuration related to per-sample shading for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id        ID of the graphics pipeline the query is being made for.
         *  @param out_opt_is_enabled_ptr         If not null, deref will be set to true if per-sample shading
         *                                        is enabled for the pipeline.
         *  @param out_opt_min_sample_shading_ptr If not null, deref will be set to the minimum sample shading value,
         *                                        as specified at creation time.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_sample_shading_state(GraphicsPipelineID in_graphics_pipeline_id,
                                      bool*              out_opt_is_enabled_ptr,
                                      float*             out_opt_min_sample_shading_ptr) const;

        /** Retrieves properties of a scissor box at a given index for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline the query is being made for.
         *  @param in_n_scissor_box        Index of the scissor box to retrieve data of.
         *  @param out_opt_x_ptr           If not null, deref will be set to X offset of the scissor box.
         *  @param out_opt_y_ptr           If not null, deref will be set to Y offset of the scissor box.
         *  @param out_opt_width_ptr       If not null, deref will be set to width of the scissor box.
         *  @param out_opt_height_ptr      If not null, deref will be set to height of the scissor box.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_scissor_box_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                        uint32_t           in_n_scissor_box,
                                        int32_t*           out_opt_x_ptr,
                                        int32_t*           out_opt_y_ptr,
                                        uint32_t*          out_opt_width_ptr,
                                        uint32_t*          out_opt_height_ptr) const;

        /** Retrieves stencil test-related state configuration for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id                 ID of the graphics pipeline the query is being
         *                                                 made for.
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
        bool get_stencil_test_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                         bool*              out_opt_is_enabled_ptr,
                                         VkStencilOp*       out_opt_front_stencil_fail_op_ptr,
                                         VkStencilOp*       out_opt_front_stencil_pass_op_ptr,
                                         VkStencilOp*       out_opt_front_stencil_depth_fail_op_ptr,
                                         VkCompareOp*       out_opt_front_stencil_compare_op_ptr,
                                         uint32_t*          out_opt_front_stencil_compare_mask_ptr,
                                         uint32_t*          out_opt_front_stencil_write_mask_ptr,
                                         uint32_t*          out_opt_front_stencil_reference_ptr,
                                         VkStencilOp*       out_opt_back_stencil_fail_op_ptr,
                                         VkStencilOp*       out_opt_back_stencil_pass_op_ptr,
                                         VkStencilOp*       out_opt_back_stencil_depth_fail_op_ptr,
                                         VkCompareOp*       out_opt_back_stencil_compare_op_ptr,
                                         uint32_t*          out_opt_back_stencil_compare_mask_ptr,
                                         uint32_t*          out_opt_back_stencil_write_mask_ptr,
                                         uint32_t*          out_opt_back_stencil_reference_ptr) const;

        /** Tells the number of patch control points, as specified at creation time for the specified
         *  graphics pipeline.
         *
         *  @param in_graphics_pipeline_id            ID of the graphics pipeline the query is being made for.
         *  @param out_opt_n_patch_control_points_ptr If not null, deref will be set to the number of patch control points,
         *                                            as specified at creation time.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_tessellation_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                         uint32_t*          out_opt_n_patch_control_points_ptr) const;

        /** Returns properties of a vertex attribute at a given index for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id     ID of the graphics pipeline the query is being made for.
         *  @param in_n_vertex_input_attribute Index of the vertex attribute to retrieve info of.
         *  @param out_opt_location_ptr        If not null, deref will be set to the specified attribute's location.
         *  @param out_opt_binding_ptr         If not null, deref will be set to the specified attribute's binding index.
         *  @param out_opt_format_ptr          If not null, deref will be set to the specified attribute's format.
         *  @param out_opt_offset_ptr          If not null, deref will be set to the specified attribute's start offset.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_vertex_input_attribute_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                                   uint32_t           in_n_vertex_input_attribute,
                                                   uint32_t*          out_opt_location_ptr,
                                                   uint32_t*          out_opt_binding_ptr,
                                                   VkFormat*          out_opt_format_ptr,
                                                   uint32_t*          out_opt_offset_ptr) const;

        /** Tells which binding has been assigned to a vertex attribute at location @param input_vertex_attribute_location.
         *
         *  This function may trigger baking of one or more graphics pipelines, if the user-specified graphics pipeline
         *  is marked as dirty.
         *
         *  @param in_graphics_pipeline_id                ID of the graphics pipeline the query is being made for.
         *  @param in_input_vertex_attribute_location     Location of the queried vertex attribute.
         *  @param out_input_vertex_attribute_binding_ptr Deref will be set to the index of the input vertex binding,
         *                                                assigned to the vertex attribute. Must not be null.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_vertex_input_binding_for_attribute_location(GraphicsPipelineID in_graphics_pipeline_id,
                                                             uint32_t           in_input_vertex_attribute_location,
                                                             uint32_t*          out_input_vertex_binding_ptr);

        /** Returns properties of a vertex binding at a given index for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id   ID of the graphics pipeline the query is being made for.
         *  @param in_n_vertex_input_binding Index of the vertex binding to retrieve info of.
         *  @param out_opt_binding_ptr       If not null, deref will be set to the specified binding's index.
         *  @param out_opt_stride_ptr        If not null, deref will be set to the specified binding's stride.
         *  @param out_opt_input_rate_ptr    If not null, deref will be set to the specified binding's input rate.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_vertex_input_binding_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                                 uint32_t           in_n_vertex_input_binding,
                                                 uint32_t*          out_opt_binding_ptr,
                                                 uint32_t*          out_opt_stride_ptr,
                                                 VkVertexInputRate* out_opt_input_rate_ptr) const;

        /** Retrieves properties of a viewport at a given index for the specified graphics pipeline.
         *
         *  @param in_graphics_pipeline_id  ID of the graphics pipeline the query is being made for.
         *  @param in_n_viewport            Index of the viewport to retrieve properties of.
         *  @param out_opt_origin_x_ptr     If not null, deref will be set to the viewport's X origin.
         *  @param out_opt_origin_y_ptr     If not null, deref will be set to the viewport's Y origin.
         *  @param out_opt_width_ptr        If not null, deref will be set to the viewport's width.
         *  @param out_opt_height_ptr       If not null, deref will be set to the viewport's height.
         *  @param out_opt_min_depth_ptr    If not null, deref will be set to the viewport's minimum depth value.
         *  @param out_opt_max_depth_ptr    If not null, deref will be set to the viewport's maximum depth value.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_viewport_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                     uint32_t           in_n_viewport,
                                     float*             out_opt_origin_x_ptr,
                                     float*             out_opt_origin_y_ptr,
                                     float*             out_opt_width_ptr,
                                     float*             out_opt_height_ptr,
                                     float*             out_opt_min_depth_ptr,
                                     float*             out_opt_max_depth_ptr) const;

        /** Sets a new blend constant for the specified graphics pipeline and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_blend_constant_vec4  4 floats, specifying the constant. Must not be nullptr.
         *
         **/
        void set_blending_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                     const float*       in_blend_constant_vec4);

        /** Updates color blend properties for the specified sub-pass attachment, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id    ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                    call, issued against the same GraphicsPipelineManager instance.
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
        void set_color_blend_attachment_properties(GraphicsPipelineID    in_graphics_pipeline_id,
                                                   SubPassAttachmentID   in_attachment_id,
                                                   bool                  in_blending_enabled,
                                                   VkBlendOp             in_blend_op_color,
                                                   VkBlendOp             in_blend_op_alpha,
                                                   VkBlendFactor         in_src_color_blend_factor,
                                                   VkBlendFactor         in_dst_color_blend_factor,
                                                   VkBlendFactor         in_src_alpha_blend_factor,
                                                   VkBlendFactor         in_dst_alpha_blend_factor,
                                                   VkColorComponentFlags in_channel_write_mask);

        /** Updates the number of scissor boxes to be used, when dynamic scissor state is enabled.
         *
         *  @param in_graphics_pipeline_id    ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                    call, issued against the same GraphicsPipelineManager instance.
         *  @param in_n_dynamic_scissor_boxes As per description.
         */
        void set_dynamic_scissor_state_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                                  uint32_t           in_n_dynamic_scissor_boxes);

        /** Updates the number of viewports to be used, when dynamic viewport state is enabled.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_n_dynamic_viewports  As per description.
         */
        void set_dynamic_viewport_state_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                                   uint32_t           in_n_dynamic_viewports);

        /** Sets the primitive topology to be used for the pipeline, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_primitive_topology   As per description.
         */
        void set_input_assembly_properties(GraphicsPipelineID  in_graphics_pipeline_id,
                                           VkPrimitiveTopology in_primitive_topology);

        /** Sets a number of multisampling properties to be used for the pipeline, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_sample_count         Number of rasterization samples to be used (expressed as one of the enum values).
         *  @param in_min_sample_shading   Minimum number of unique samples to shade for each fragment.
         *  @param in_sample_mask          Sample mask to use.
         */
        void set_multisampling_properties(GraphicsPipelineID    in_graphics_pipeline_id,
                                          VkSampleCountFlagBits in_sample_count,
                                          float                 in_min_sample_shading,
                                          const VkSampleMask    in_sample_mask);

        /** Sets a call-back, which GFX pipeline manager will invoke right after a vkCreateGraphicsPipeline() call is made.
         *
         *  The call-back will be issued right before the vkCreateGraphicsPipelines() call is issued.
         *
         *  This call overwrites any previous set_pipeline_post_bake_callback() calls.
         *
         *  @param in_graphics_pipeline_id       ID of the graphics pipeline to set the call-back for.
         *  @param in_pfn_pipeline_pre_bake_proc Function to call back. Must not be nullptr.
         *  @param in_user_arg                   User argument to pass with the call-back. May be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_pipeline_post_bake_callback(GraphicsPipelineID              in_graphics_pipeline_id,
                                             PFNPIPELINEPOSTBAKECALLBACKPROC in_pfn_pipeline_post_bake_proc,
                                             void*                           in_user_arg);

        /** Sets a call-back, which GFX pipeline manager will invoke right before a vkCreateGraphicsPipeline() call is made.
         *  This allows the callee to adjust the VkGraphicsPipelineCreateInfo instance, eg. by modifying pNext to user-defined
         *  chain of structures.
         *
         *  The call-back will be issued right before the vkCreateGraphicsPipelines() call is issued.
         *
         *  This call overwrites any previous set_pipeline_pre_bake_callback() calls.
         *
         *  @param in_graphics_pipeline_id       ID of the graphics pipeline to set the call-back for.
         *  @param in_pfn_pipeline_pre_bake_proc Function to call back. Must not be nullptr.
         *  @param in_user_arg                   User argument to pass with the call-back. May be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_pipeline_pre_bake_callback(GraphicsPipelineID             in_graphics_pipeline_id,
                                            PFNPIPELINEPREBAKECALLBACKPROC in_pfn_pipeline_pre_bake_proc,
                                            void*                          in_user_arg);

        /** Copies graphics state from @param in_source_pipeline_id to @param in_target_pipeline_id, leaving
         *  the originally assigned renderpass, as well as the subpass ID, unaffected.
         *
         *  @param in_target_pipeline_id ID of a graphics pipeline, to which state should be copied.
         *  @param in_source_pipeline_id ID of a graphics pipeline, from which state should be copied.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_pipeline_state_from_pipeline(GraphicsPipelineID in_target_pipeline_id,
                                              GraphicsPipelineID in_source_pipeline_id);

        /** Configures the rasterization order for the pipeline if the VK_AMD_rasterization_order extension
         *  is supported by the device, for which the pipeline has been created.
         *
         *  On drivers which do not support the extension, the call has no effect.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_rasterization_order  Rasterization order to use.
         **/
        void set_rasterization_order(GraphicsPipelineID      in_graphics_pipeline_id,
                                     VkRasterizationOrderAMD in_rasterization_order);

        /** Sets a number of rasterization properties to be used for the pipeline, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_polygon_mode         Polygon mode to use.
         *  @param in_cull_mode            Cull mode to use.
         *  @param in_front_face           Front face to use.
         *  @param in_line_width           Line width to use.
         **/
        void set_rasterization_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                          VkPolygonMode      in_polygon_mode,
                                          VkCullModeFlags    in_cull_mode,
                                          VkFrontFace        in_front_face,
                                          float              in_line_width);

        /** Sets properties of a scissor box at the specified index, and marks the pipeline as dirty.
         *
         *  If @param n_scissor_box is larger than 1, all previous scissor boxes must also be defined
         *  prior to baking. Number of scissor boxes must match the number of viewports defined for the
         *  pipeline.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_n_scissor_box        Index of the scissor box to be updated.
         *  @param in_x                    X offset of the scissor box.
         *  @param in_y                    Y offset of the scissor box.
         *  @param in_width                Width of the scissor box.
         *  @param in_height               Height of the scissor box.
         **/
        void set_scissor_box_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                        uint32_t           in_n_scissor_box,
                                        int32_t            in_x,
                                        int32_t            in_y,
                                        uint32_t           in_width,
                                        uint32_t           in_height);

        /** Sets a number of stencil test properties to be used for the pipeline, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id    ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                    call, issued against the same GraphicsPipelineManager instance.
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
        void set_stencil_test_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                         bool               in_update_front_face_state,
                                         VkStencilOp        in_stencil_fail_op,
                                         VkStencilOp        in_stencil_pass_op,
                                         VkStencilOp        in_stencil_depth_fail_op,
                                         VkCompareOp        in_stencil_compare_op,
                                         uint32_t           in_stencil_compare_mask,
                                         uint32_t           in_stencil_write_mask,
                                         uint32_t           in_stencil_reference);

        /** Updates the number of tessellation patch points to be used for the pipeline, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id   ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                   call, issued against the same GraphicsPipelineManager instance.
         *  @param in_n_patch_control_points As per description.
         **/
        void set_tessellation_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                         uint32_t           in_n_patch_control_points);

        /** Sets properties of a viewport at the specified index, and marks the pipeline as dirty.
         *
         *  If @param in_n_viewport is larger than 1, all previous viewports must also be defined
         *  prior to baking. Number of scissor boxes must match the number of viewports defined for the
         *  pipeline.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_n_viewport           Index of the viewport, whose properties should be changed.
         *  @param in_origin_x             X offset to use for the viewport's origin.
         *  @param in_origin_y             Y offset to use for the viewport's origin.
         *  @param in_width                Width of the viewport.
         *  @param in_height               Height of the viewport.
         *  @param in_min_depth            Minimum depth value to use for the viewport.
         *  @param in_max_depth            Maximum depth value to use for the viewport.
         **/
        void set_viewport_properties(GraphicsPipelineID in_graphics_pipeline_id,
                                     uint32_t           in_n_viewport,
                                     float              in_origin_x,
                                     float              in_origin_y,
                                     float              in_width,
                                     float              in_height,
                                     float              in_min_depth,
                                     float              in_max_depth);

        /** Enables or disables the "alpha to coverage" test, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the test; false to disable it.
         */
        void toggle_alpha_to_coverage(GraphicsPipelineID in_graphics_pipeline_id,
                                      bool               in_should_enable);

        /** Enables or disables the "alpha to one" test, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the test; false to disable it.
         */
        void toggle_alpha_to_one(GraphicsPipelineID in_graphics_pipeline_id,
                                 bool               in_should_enable);

        /** Enables or disables the "depth bias" mode, updates related state values, and marks
         *  the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id       ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                       call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable              true to enable the mode; false to disable it.
         *  @param in_depth_bias_constant_factor Depth bias constant factor to use for the mode.
         *  @param in_depth_bias_clamp           Depth bias clamp to use for the mode.
         *  @param in_depth_bias_slope_factor    Slope scale for the depth bias to use for the mode.
         */
        void toggle_depth_bias(GraphicsPipelineID in_graphics_pipeline_id,
                               bool               in_should_enable,
                               float              in_depth_bias_constant_factor,
                               float              in_depth_bias_clamp,
                               float              in_depth_bias_slope_factor);

        /** Enables or disables the "depth bounds" test, updates related state values, and marks
         *  the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the test; false to disable it.
         *  @param in_min_depth_bounds     Minimum boundary value to use for the test.
         *  @param in_max_depth_bounds     Maximum boundary value to use for the test. 
         */
        void toggle_depth_bounds_test(GraphicsPipelineID in_graphics_pipeline_id,
                                      bool               in_should_enable,
                                      float              in_min_depth_bounds,
                                      float              in_max_depth_bounds);

        /** Enables or disables the "depth clamp" test, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the test; false to disable it.
         */
        void toggle_depth_clamp(GraphicsPipelineID in_graphics_pipeline_id,
                                bool               in_should_enable);

        /** Enables or disables the depth test, updates related state values, and marks the
         *  pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the mode; false to disable it.
         *  @param in_compare_op           Compare operation to use for the test.
         */
        void toggle_depth_test(GraphicsPipelineID in_graphics_pipeline_id,
                               bool               in_should_enable,
                               VkCompareOp        in_compare_op);

        /** Enables or disables depth writes, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the writes; false to disable them.
         */
        void toggle_depth_writes(GraphicsPipelineID in_graphics_pipeline_id,
                                 bool               in_should_enable);

        /** Enables or disables the specified dynamic states, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the dynamic state(s) specified by @param in_dynamic_state_bits, false
         *                                 disable them if already enabled.
         *  @param in_dynamic_state_bits   An integer whose individual bits correspond to dynamic states which should either
         *                                 be enabled or disabled.
         **/
        void toggle_dynamic_states (GraphicsPipelineID   in_graphics_pipeline_id,
                                    bool                 in_should_enable,
                                    DynamicStateBitfield in_dynamic_state_bits);

        /** Enables or disables logic ops, specifies which logic op should be used, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the mode; false to disable it.
         *  @param in_logic_op             Logic operation type to use.
         */
        void toggle_logic_op(GraphicsPipelineID in_graphics_pipeline_id,
                             bool               in_should_enable,
                             VkLogicOp          in_logic_op);

        /** Enables or disables the "primitive restart" mode, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the mode; false to disable it.
         */
        void toggle_primitive_restart(GraphicsPipelineID in_graphics_pipeline_id,
                                      bool               in_should_enable);

        /** Enables or disables the "rasterizer discard" mode, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the test; false to disable it.
         */
        void toggle_rasterizer_discard(GraphicsPipelineID in_graphics_pipeline_id,
                                       bool               in_should_enable);

        /** Enables or disables the sample mask, and marks the pipeline as dirty.
         *
         *  Note: make sure to configure the sample mask using set_multisampling_properties() if you intend to use it.
         *
         *  Note: disabling sample mask will make the manager set VkPipelineMultisampleStateCreateInfo::pSampleMask to a non-null
         *        value at pipeline creation time.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable sample mask; false to disable it.
         */
        void toggle_sample_mask(GraphicsPipelineID in_graphics_pipeline_id,
                                bool               in_should_enable);

        /** Enables or disables the "per-sample shading" mode, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the test; false to disable it.
         */
        void toggle_sample_shading(GraphicsPipelineID in_graphics_pipeline_id,
                                   bool               in_should_enable);

        /** Enables or disables the stencil test test, and marks the pipeline as dirty.
         *
         *  @param in_graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param in_should_enable        true to enable the test; false to disable it.
         */
        void toggle_stencil_test(GraphicsPipelineID in_graphics_pipeline_id,
                                 bool               in_should_enable);

    private:
        /* Private type declarations */

        /* Defines blending properties for a single subpass attachment.
         *
         * This descriptor is not exposed to the Vulkan implementation. It is used to form Vulkan-specific
         * descriptors at baking time instead.
         */
        typedef struct BlendingProperties
        {
            VkColorComponentFlagsVariable(channel_write_mask);

            bool          blend_enabled;
            VkBlendOp     blend_op_alpha;
            VkBlendOp     blend_op_color;
            VkBlendFactor dst_alpha_blend_factor;
            VkBlendFactor dst_color_blend_factor;
            VkBlendFactor src_alpha_blend_factor;
            VkBlendFactor src_color_blend_factor;

            /** Constructor. */
            BlendingProperties()
            {
                blend_enabled           = false;
                blend_op_alpha          = VK_BLEND_OP_ADD;
                blend_op_color          = VK_BLEND_OP_ADD;
                channel_write_mask      = VK_COLOR_COMPONENT_A_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_R_BIT;
                dst_alpha_blend_factor  = VK_BLEND_FACTOR_ONE;
                dst_color_blend_factor  = VK_BLEND_FACTOR_ONE;
                src_alpha_blend_factor  = VK_BLEND_FACTOR_ONE;
                src_color_blend_factor  = VK_BLEND_FACTOR_ONE;
            }

            /** Forms a VkPipelineColorBlendAttachmentState instance from the stored state values. */
            VkPipelineColorBlendAttachmentState get_vk_descriptor() const
            {
                VkPipelineColorBlendAttachmentState result;

                result.alphaBlendOp        = blend_op_alpha;
                result.blendEnable         = static_cast<VkBool32>((blend_enabled) ? VK_TRUE : VK_FALSE);
                result.colorBlendOp        = blend_op_color;
                result.colorWriteMask      = channel_write_mask;
                result.dstAlphaBlendFactor = dst_alpha_blend_factor;
                result.dstColorBlendFactor = dst_color_blend_factor;
                result.srcAlphaBlendFactor = src_alpha_blend_factor;
                result.srcColorBlendFactor = src_color_blend_factor;

                return result;
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

        typedef enum
        {
            GRAPHICS_SHADER_STAGE_FRAGMENT,
            GRAPHICS_SHADER_STAGE_GEOMETRY,
            GRAPHICS_SHADER_STAGE_TESSELLATION_CONTROL,
            GRAPHICS_SHADER_STAGE_TESSELLATION_EVALUATION,
            GRAPHICS_SHADER_STAGE_VERTEX,

            /* Always last */
            GRAPHICS_SHADER_STAGE_COUNT
        } GraphicsShaderStage;

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

            /* Converts the internal descriptor to the Vulkan equivalent structure */
            VkRect2D get_vk_descriptor() const
            {
                VkRect2D result;

                result.extent.height = height;
                result.extent.width  = width;
                result.offset.x      = x;
                result.offset.y      = y;

                return result;
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

            /* Returns a Vulkan viewport descriptor, using this object's properties. */
            VkViewport get_vk_descriptor() const
            {
                VkViewport result;

                result.height   = height;
                result.maxDepth = max_depth;
                result.minDepth = min_depth;
                result.x        = origin_x;
                result.y        = origin_y;
                result.width    = width;

                return result;
            }
        } InternalViewport;

        /** A vertex attribute descriptor. This descriptor is not exposed to the Vulkan implementation. Instead,
         *  its members are used to create Vulkan input attribute & binding descriptors at baking time.
         */
        typedef struct InternalVertexAttribute
        {
            uint32_t          explicit_binding_index;
            VkFormat          format;
            uint32_t          location;
            uint32_t          offset_in_bytes;
            VkVertexInputRate rate;
            uint32_t          stride_in_bytes;

            /** Dummy constructor. Should only be used by STL. */
            InternalVertexAttribute()
            {
                explicit_binding_index = UINT32_MAX;
                format                 = VK_FORMAT_UNDEFINED;
                location               = UINT32_MAX;
                offset_in_bytes        = UINT32_MAX;
                rate                   = VK_VERTEX_INPUT_RATE_MAX_ENUM;
                stride_in_bytes        = UINT32_MAX;
            }

            /** Constructor.
             *
             *  @param in_format          Attribute format.
             *  @param in_location        Attribute location.
             *  @param in_offset_in_bytes Start offset in bytes.
             *  @param in_rate            Step rate.
             *  @param in_stride_in_bytes Stride in bytes.
             **/
            InternalVertexAttribute(uint32_t          in_explicit_binding_index,
                                    VkFormat          in_format,
                                    uint32_t          in_location,
                                    uint32_t          in_offset_in_bytes,
                                    VkVertexInputRate in_rate,
                                    uint32_t          in_stride_in_bytes)
            {
                explicit_binding_index = in_explicit_binding_index;
                format                 = in_format;
                location               = in_location;
                offset_in_bytes        = in_offset_in_bytes;
                rate                   = in_rate;
                stride_in_bytes        = in_stride_in_bytes;
            }
        } InternalVertexAttribute;

        typedef std::map<uint32_t, uint32_t>           AttributeLocationToBindingIndexMap;
        typedef std::map<uint32_t, InternalScissorBox> InternalScissorBoxes;
        typedef std::vector<InternalVertexAttribute>   InternalVertexAttributes;
        typedef std::map<uint32_t, InternalViewport>   InternalViewports;

        /** Descriptor which encapsulates all state of a single graphics pipeline.
         *
         *  This descriptor is not exposed to the Vulkan implementation. It is used to form Vulkan-specific
         *  descriptors at baking time instead.
         *
         *  A single unique graphics pipeline descriptor should be assigned to each subpass.
         */
        struct GraphicsPipelineConfiguration
        {
            AttributeLocationToBindingIndexMap             attribute_location_to_binding_index_map;
            std::vector<VkVertexInputAttributeDescription> vk_input_attributes;
            std::vector<VkVertexInputBindingDescription>   vk_input_bindings;

            bool  depth_bounds_test_enabled;
            float max_depth_bounds;
            float min_depth_bounds;

            bool  depth_bias_enabled;
            float depth_bias_clamp;
            float depth_bias_constant_factor;
            float depth_bias_slope_factor;

            bool        depth_test_enabled;
            VkCompareOp depth_test_compare_op;

            DynamicStateBitfield enabled_dynamic_states;

            bool alpha_to_coverage_enabled;
            bool alpha_to_one_enabled;
            bool depth_clamp_enabled;
            bool depth_writes_enabled;
            bool logic_op_enabled;
            bool primitive_restart_enabled;
            bool rasterizer_discard_enabled;
            bool sample_mask_enabled;
            bool sample_shading_enabled;

            bool             stencil_test_enabled;
            VkStencilOpState stencil_state_back_face;
            VkStencilOpState stencil_state_front_face;

            VkRasterizationOrderAMD rasterization_order;

            InternalVertexAttributes                 attributes;
            float                                    blend_constant[4];
            VkPolygonMode                            polygon_mode;
            VkFrontFace                              front_face;
            float                                    line_width;
            VkLogicOp                                logic_op;
            float                                    min_sample_shading;
            uint32_t                                 n_dynamic_scissor_boxes;
            uint32_t                                 n_dynamic_viewports;
            uint32_t                                 n_patch_control_points;
            VkPrimitiveTopology                      primitive_topology;
            VkSampleMask                             sample_mask;
            InternalScissorBoxes                     scissor_boxes;
            SubPassAttachmentToBlendingPropertiesMap subpass_attachment_blending_properties;
            InternalViewports                        viewports;

            VkCullModeFlagsVariable   (cull_mode);
            VkSampleCountFlagsVariable(sample_count);

            PFNPIPELINEPOSTBAKECALLBACKPROC pfn_pipeline_postbake_callback_proc;
            PFNPIPELINEPREBAKECALLBACKPROC  pfn_pipeline_prebake_callback_proc;
            void*                           pipeline_postbake_callback_user_arg;
            void*                           pipeline_prebake_callback_user_arg;

            std::shared_ptr<RenderPass> renderpass_ptr;
            SubPassID                   subpass_id;

            /** Constructor.
             *
             *  @param in_renderpass_ptr RenderPass instance the graphics pipeline will be used for.
             *                           The instance is retained. May be nullptr for proxy pipelines,
             *                           must not be nullptr for all other pipeline types.
             *  @param in_subpass_id     ID of the subpass the graphics pipeline descriptor is created
             *                           for.
             **/
            explicit GraphicsPipelineConfiguration(std::shared_ptr<RenderPass> in_renderpass_ptr,
                                                   SubPassID                   in_subpass_id)
            {
                alpha_to_coverage_enabled  = false;
                alpha_to_one_enabled       = false;
                depth_bias_clamp           = 0.0f;
                depth_bias_constant_factor = 0.0f;
                depth_bias_enabled         = false;
                depth_bias_slope_factor    = 1.0f;
                depth_bounds_test_enabled  = false;
                depth_clamp_enabled        = false;
                depth_test_compare_op      = VK_COMPARE_OP_ALWAYS;
                depth_test_enabled         = false;
                depth_writes_enabled       = false;
                enabled_dynamic_states     = 0;
                front_face                 = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                logic_op                   = VK_LOGIC_OP_NO_OP;
                logic_op_enabled           = false;
                max_depth_bounds           = 1.0f;
                min_depth_bounds           = 0.0f;
                n_dynamic_scissor_boxes    = 0;
                n_dynamic_viewports        = 0;
                n_patch_control_points     = 0;
                primitive_restart_enabled  = false;
                rasterizer_discard_enabled = false;
                sample_mask_enabled        = false;
                sample_shading_enabled     = false;
                stencil_test_enabled       = false;

                pfn_pipeline_postbake_callback_proc = nullptr;
                pfn_pipeline_prebake_callback_proc  = nullptr;
                pipeline_postbake_callback_user_arg = nullptr;
                pipeline_prebake_callback_user_arg  = nullptr;

                renderpass_ptr = in_renderpass_ptr;
                subpass_id     = in_subpass_id;

                stencil_state_back_face.compareMask = ~0u;
                stencil_state_back_face.compareOp   = VK_COMPARE_OP_ALWAYS;
                stencil_state_back_face.depthFailOp = VK_STENCIL_OP_KEEP;
                stencil_state_back_face.failOp      = VK_STENCIL_OP_KEEP;
                stencil_state_back_face.passOp      = VK_STENCIL_OP_KEEP;
                stencil_state_back_face.reference   = 0;
                stencil_state_back_face.writeMask   = ~0u;
                stencil_state_front_face            = stencil_state_back_face;

                rasterization_order = VK_RASTERIZATION_ORDER_STRICT_AMD;

                memset(blend_constant,
                       0,
                       sizeof(blend_constant) );

                cull_mode          = VK_CULL_MODE_BACK_BIT;
                line_width         = 1.0f;
                min_sample_shading = 1.0f;
                sample_count       = VK_SAMPLE_COUNT_1_BIT;
                polygon_mode       = VK_POLYGON_MODE_FILL;
                primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                sample_mask        = ~0u;
            }

            /** Constructor. Copies all state from the specified base pipeline configuration, replacing
             *  the renderpass instance & subpass ID with the argument values.
             *
             *  @param in_base_pipeline_config_ptr As per description. Must not be nullptr.
             *  @param in_renderpass_ptr           RenderPass instance the graphics pipeline will be used for.
             *                                     The instance is retained. May be nullptr for proxy pipelines.
             *                                     Must not be nullptr for all other pipeline types.
             *  @param in_subpass_id               ID of the subpass the graphics pipeline descriptor is created
             *                                     for.
             **/
            explicit GraphicsPipelineConfiguration(std::shared_ptr<GraphicsPipelineConfiguration> in_base_pipeline_config_ptr,
                                                   std::shared_ptr<RenderPass>                    in_renderpass_ptr,
                                                   SubPassID                                      in_subpass_id)
            {
                /* Copy all state from the base config.. */
                *this = *in_base_pipeline_config_ptr;

                /* Override the renderpass & the subpass with the data we've been provided */
                renderpass_ptr = in_renderpass_ptr;
                subpass_id     = in_subpass_id;
            }

            /** Destructor. */
            ~GraphicsPipelineConfiguration()
            {
                /* Stub */
            }

            /** Copy assignment operator. */
            GraphicsPipelineConfiguration& operator=(const GraphicsPipelineConfiguration& in)
            {
                /* NOTE: We leave window, renderpass & subpass ID intact. */
                attribute_location_to_binding_index_map = in.attribute_location_to_binding_index_map;
                vk_input_attributes                     = in.vk_input_attributes;
                vk_input_bindings                       = in.vk_input_bindings;

                depth_bounds_test_enabled = in.depth_bounds_test_enabled;
                max_depth_bounds          = in.max_depth_bounds;
                min_depth_bounds          = in.min_depth_bounds;

                depth_bias_enabled         = in.depth_bias_enabled;
                depth_bias_clamp           = in.depth_bias_clamp;
                depth_bias_constant_factor = in.depth_bias_constant_factor;
                depth_bias_slope_factor    = in.depth_bias_slope_factor;

                depth_test_enabled    = in.depth_test_enabled;
                depth_test_compare_op = in.depth_test_compare_op;

                enabled_dynamic_states = in.enabled_dynamic_states;

                alpha_to_coverage_enabled  = in.alpha_to_coverage_enabled;
                alpha_to_one_enabled       = in.alpha_to_one_enabled;
                depth_clamp_enabled        = in.depth_clamp_enabled;
                depth_writes_enabled       = in.depth_writes_enabled;
                logic_op_enabled           = in.logic_op_enabled;
                primitive_restart_enabled  = in.primitive_restart_enabled;
                rasterizer_discard_enabled = in.rasterizer_discard_enabled;
                sample_shading_enabled     = in.sample_shading_enabled;

                stencil_test_enabled     = in.stencil_test_enabled;
                stencil_state_back_face  = in.stencil_state_back_face;
                stencil_state_front_face = in.stencil_state_front_face;

                rasterization_order = in.rasterization_order;

                attributes                             = in.attributes;
                cull_mode                              = in.cull_mode;
                front_face                             = in.front_face;
                line_width                             = in.line_width;
                logic_op                               = in.logic_op;
                min_sample_shading                     = in.min_sample_shading;
                n_dynamic_scissor_boxes                = in.n_dynamic_scissor_boxes;
                n_dynamic_viewports                    = in.n_dynamic_viewports;
                n_patch_control_points                 = in.n_patch_control_points;
                polygon_mode                           = in.polygon_mode;
                primitive_topology                     = in.primitive_topology;
                sample_count                           = in.sample_count;
                sample_mask                            = in.sample_mask;
                scissor_boxes                          = in.scissor_boxes;
                subpass_attachment_blending_properties = in.subpass_attachment_blending_properties;
                viewports                              = in.viewports;

                pfn_pipeline_postbake_callback_proc = in.pfn_pipeline_postbake_callback_proc;
                pfn_pipeline_prebake_callback_proc  = in.pfn_pipeline_prebake_callback_proc;
                pipeline_postbake_callback_user_arg = in.pipeline_postbake_callback_user_arg;
                pipeline_prebake_callback_user_arg  = in.pipeline_prebake_callback_user_arg;

                memcpy(blend_constant,
                       in.blend_constant,
                       sizeof(blend_constant) );

                return *this;
            }

        };

        typedef std::map<GraphicsPipelineID, std::shared_ptr<GraphicsPipelineConfiguration> > GraphicsPipelineConfigurations;

        /* Private functions */
        explicit GraphicsPipelineManager(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                                         bool                                  in_use_pipeline_cache,
                                         std::shared_ptr<Anvil::PipelineCache> in_pipeline_cache_to_reuse_ptr);

        void bake_vk_attributes_and_bindings(std::shared_ptr<GraphicsPipelineConfiguration> inout_pipeline_config_ptr);

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(GraphicsPipelineManager);
        ANVIL_DISABLE_COPY_CONSTRUCTOR   (GraphicsPipelineManager);

        /* Private members */
        GraphicsPipelineConfigurations m_pipeline_configurations;
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_GRAPHICS_PIPELINE_MANAGER_H */