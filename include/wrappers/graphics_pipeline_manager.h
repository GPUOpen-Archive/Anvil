//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
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

        /** As defined by VK_AMD_rasterization order
         *
         *  Once this lands in the header, we'll need to get rid of this declaration
         **/
        typedef enum VkRasterizationOrderAMD
        {
            VK_RASTERIZATION_ORDER_STRICT_AMD  = 0,
            VK_RASTERIZATION_ORDER_RELAXED_AMD = 1,
        } VkRasterizationOrderAMD;

        typedef struct VkPipelineRasterizationStateRasterizationOrderAMD
        {
            VkStructureType         sType;
            const void*             pNext;
            VkRasterizationOrderAMD rasterizationOrder;
        } VkPipelineRasterizationStateRasterizationOrderAMD;

        /* Prototype for a call-back function, invoked right after vkCreateGraphicsPipelines() call returns. **/
        typedef void (*PFNPIPELINEPOSTBAKECALLBACKPROC)(Anvil::Device*     device_ptr,
                                                        GraphicsPipelineID pipeline_id,
                                                        void*              user_arg);

        /* Prototype for a call-back function, invoked before a Vulkan graphics pipeline is created.
         *
         * The caller can adjust any of the VkGraphicsPipelineCreateInfo fields, with an assumption
         * Anvil::GraphicsPipelineManager will not adjust its internal state to sync with user-modified
         * fields.
         **/
        typedef void (*PFNPIPELINEPREBAKECALLBACKPROC)(Anvil::Device*                device_ptr,
                                                       GraphicsPipelineID            pipeline_id,
                                                       VkGraphicsPipelineCreateInfo* graphics_pipeline_create_info_ptr,
                                                       void*                         user_arg);

        /* Public functions */

        /** Constructor.
         *
         *  @param device_ptr                  Device to use.
         *  @param use_pipeline_cache          true if the manager should use a pipeline cache instance. false
         *                                     to pass nullptr whenever a Vulkan descriptor requires us to specify
         *                                     one.
         *  @param pipeline_cache_to_reuse_ptr if @param use_pipeline_cache is true, this argument can be optionally
         *                                     set to a non-nullptr value to point at an already allocated pipeline cache.
         *                                     If one is not provided and the other argument is set as described,
         *                                     a new pipeline cache with size 0 will be allocated.
         **/
        explicit GraphicsPipelineManager(Anvil::Device*        device_ptr,
                                         bool                  use_pipeline_cache          = false,
                                         Anvil::PipelineCache* pipeline_cache_to_reuse_ptr = nullptr);

        /** Destructor. */
        ~GraphicsPipelineManager();

        /** Adds a new derivative pipeline, to be used the specified renderpass' subpass. All states of the base pipeline
         *  will be copied to the new pipeline.
         *
         *  @param disable_optimizations                        If true, the pipeline will be created with the 
         *                                                      VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT flag.
         *  @param allow_derivatives                            If true, the pipeline will be created with the
         *                                                      VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT flag.
         *  @param renderpass_ptr                               Renderpass instance the pipeline is to be created for. Must not be nullptr.
         *                                                      The renderpass instance will be implicitly retained.
         *  @param subpass_id                                   ID of the subpass the pipeline is to be created for.
         *  @param fragment_shader_stage_entrypoint_info        Fragment shader to use for the pipeline. Pass an instance initialized with
         *                                                      dummy constructor if the stage is irrelevant.
         *  @param geometry_shader_stage_entrypoint_info        Geometry shader to use for the pipeline. Pass an instance initialized with
         *                                                      dummy constructor if the stage is irrelevant.
         *  @param tess_control_shader_stage_entrypoint_info    Tessellation control shader to use for the pipeline. Pass an instance initialized with
         *                                                      dummy constructor if the stage is irrelevant.
         *  @param tess_evaluation_shader_stage_entrypoint_info Tessellation evaluation shader to use for the pipeline. Pass an instance initialized
         *                                                      with dummy constructor if the stage is irrelevant.
         *  @param vertex_shader_stage_entrypoint_info          Vertex shader to use for the pipeline. Must not be a dummy shader stage entrypoint
         *                                                      descriptor.
         *  @param base_pipeline_id                             Graphics pipeline ID of the base graphics pipeline. The ID must have been returned
         *                                                      by one of the preceding add_() calls, issued against the same GraphicsPipelineManager
         *                                                      instance. Must not refer to a proxy pipeline.
         *  @param out_graphics_pipeline_id_ptr                 Deref will be set to the new graphics pipeline's ID. Must not be nullptr.
         **/
        bool add_derivative_pipeline_from_sibling_pipeline(bool                               disable_optimizations,
                                                           bool                               allow_derivatives,
                                                           RenderPass*                        renderpass_ptr,
                                                           SubPassID                          subpass_id,
                                                           const ShaderModuleStageEntryPoint& fragment_shader_stage_entrypoint_info,
                                                           const ShaderModuleStageEntryPoint& geometry_shader_stage_entrypoint_info,
                                                           const ShaderModuleStageEntryPoint& tess_control_shader_stage_entrypoint_info,
                                                           const ShaderModuleStageEntryPoint& tess_evaluation_shader_stage_entrypoint_info,
                                                           const ShaderModuleStageEntryPoint& vertex_shader_shader_stage_entrypoint_info,
                                                           GraphicsPipelineID                 base_pipeline_id,
                                                           GraphicsPipelineID*                out_graphics_pipeline_id_ptr);

        /** Adds a new derivative pipeline, to be used the specified renderpass' subpass. Since the base pipeline is
         *  a Vulkan object, its state values are NOT automatically copied to the new derivative pipeline.
         *
         *  @param disable_optimizations                        If true, the pipeline will be created with the 
         *                                                      VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT flag.
         *  @param allow_derivatives                            If true, the pipeline will be created with the
         *                                                      VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT flag.
         *  @param renderpass_ptr                               Renderpass instance the pipeline is to be created for. Must not be nullptr.
         *                                                      The renderpass instance will be implicitly retained.
         *  @param subpass_id                                   ID of the subpass the pipeline is to be created for.
         *  @param fragment_shader_stage_entrypoint_info        Fragment shader to use for the pipeline. Pass an instance initialized with
         *                                                      dummy constructor if the stage is irrelevant.
         *  @param geometry_shader_stage_entrypoint_info        Geometry shader to use for the pipeline. Pass an instance initialized with
         *                                                      dummy constructor if the stage is irrelevant.
         *  @param tess_control_shader_stage_entrypoint_info    Tessellation control shader to use for the pipeline. Pass an instance initialized with
         *                                                      dummy constructor if the stage is irrelevant.
         *  @param tess_evaluation_shader_stage_entrypoint_info Tessellation evaluation shader to use for the pipeline. Pass an instance initialized
         *                                                      with dummy constructor if the stage is irrelevant.
         *  @param vertex_shader_stage_entrypoint_info          Vertex shader to use for the pipeline. Must not be a dummy shader stage descriptor.
         *  @param base_pipeline                                Base graphics pipeline. Must not be nullptr.
         *  @param out_graphics_pipeline_id_ptr                 Deref will be set to the new graphics pipeline's ID. Must not be nullptr.
        **/
        bool add_derivative_pipeline_from_pipeline(bool                               disable_optimizations,
                                                   bool                               allow_derivatives,
                                                   RenderPass*                        renderpass_ptr,
                                                   SubPassID                          subpass_id,
                                                   const ShaderModuleStageEntryPoint& fragment_shader_stage_entrypoint_info,
                                                   const ShaderModuleStageEntryPoint& geometry_shader_stage_entrypoint_info,
                                                   const ShaderModuleStageEntryPoint& tess_control_shader_stage_entrypoint_info,
                                                   const ShaderModuleStageEntryPoint& tess_evaluation_shader_stage_entrypoint_info,
                                                   const ShaderModuleStageEntryPoint& vertex_shader_shader_stage_entrypoint_info,
                                                   VkPipeline                         base_pipeline,
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
         *  @param disable_optimizations                        If true, the pipeline will be created with the 
         *                                                      VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT flag.
         *  @param allow_derivatives                            If true, the pipeline will be created with the
         *                                                      VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT flag.
         *  @param renderpass_ptr                               Renderpass instance the pipeline is to be created for. Must not be nullptr.
         *                                                      The renderpass instance will be implicitly retained.
         *  @param subpass_id                                   ID of the subpass the pipeline is to be created for.
         *  @param fragment_shader_stage_entrypoint_info        Fragment shader to use for the pipeline. Pass an instance initialized with
         *                                                      dummy constructor if the stage is irrelevant.
         *  @param geometry_shader_stage_entrypoint_info        Geometry shader to use for the pipeline. Pass an instance initialized with
         *                                                      dummy constructor if the stage is irrelevant.
         *  @param tess_control_shader_stage_entrypoint_info    Tessellation control shader to use for the pipeline. Pass an instance initialized with
         *                                                      dummy constructor if the stage is irrelevant.
         *  @param tess_evaluation_shader_stage_entrypoint_info Tessellation evaluation shader to use for the pipeline. Pass an instance initialized
         *                                                      with dummy constructor if the stage is irrelevant.
         *  @param vertex_shader_stage_entrypoint_info          Vertex shader to use for the pipeline. Must not be a dummy shader stage descriptor.
         *  @param out_graphics_pipeline_id_ptr                 Deref will be set to the new graphics pipeline's ID. Must not be nullptr.
         **/
        bool add_regular_pipeline(bool                               disable_optimizations,
                                  bool                               allow_derivatives,
                                  RenderPass*                        renderpass_ptr,
                                  SubPassID                          subpass_id,
                                  const ShaderModuleStageEntryPoint& fragment_shader_stage_entrypoint_info,
                                  const ShaderModuleStageEntryPoint& geometry_shader_stage_entrypoint_info,
                                  const ShaderModuleStageEntryPoint& tess_control_shader_stage_entrypoint_info,
                                  const ShaderModuleStageEntryPoint& tess_evaluation_shader_stage_entrypoint_info,
                                  const ShaderModuleStageEntryPoint& vertex_shader_shader_stage_entrypoint_info,
                                  GraphicsPipelineID*                out_graphics_pipeline_id_ptr);

        /** Deletes an existing pipeline.
        *
        *  @param pipeline_id ID of a pipeline to delete.
        *
        *  @return true if successful, false otherwise.
        **/
        bool delete_pipeline(GraphicsPipelineID graphics_pipeline_id);

        /** Adds a new vertex attribute descriptor to the specified graphics pipeline. This data will be used
         *  at baking time to configure input vertex attribute & bindings for the Vulkan pipeline object.
         *
         *  @param graphics_pipeine_id ID of the graphics pipeline object, whose state should be changed. The ID
         *                             must have been returned by one of the add_() functions, issued against the
         *                             same GraphicsPipelineManager instance.
         *  @param location            Vertex attribute location
         *  @param format              Vertex attribute format.
         *  @param offset_in_bytes     Start offset of the vertex attribute data.
         *  @param stride_in_bytes     Stride of the vertex attribute data.
         *  @param rate                Step rate to use for the vertex attribute data.
         *
         *  @return true if successful, false otherwise.
         **/
        bool add_vertex_attribute(GraphicsPipelineID graphics_pipeline_id,
                                  uint32_t           location,
                                  VkFormat           format,
                                  uint32_t           offset_in_bytes,
                                  uint32_t           stride_in_bytes,
                                  VkVertexInputRate  step_rate);

        /** Generates a VkPipeline instance for each pipeline object marked as dirty. If a dirty pipeline
         *  has already been baked in the past, the former object instance is released.
         *
         *  @return true if successful, false otherwise.
         **/
        bool bake();

        /** Returns a VkPipeline instance for the specified graphics pipeline ID. If the pipeline is marked as dirty,
         *  the Vulkan object will be created before returning after former instance is released.
         *
         *  @param pipeline_id ID of the pipeline to return the VkPipeline instance for. Must not describe
         *                     a proxy pipeline.
         *
         *  @return As per description.
         **/
        VkPipeline get_graphics_pipeline(GraphicsPipelineID pipeline_id);

        /** Retrieves a PipelineLayout instance associated with the specified pipeline ID.
         *
         *  The function will bake a pipeline object (and, possibly, a pipeline layout object, too) if
         *  the specified pipeline is marked as dirty.
         *
         *  @param pipeline_id ID of the pipeline to return the wrapper instance for.
         *                     Must not describe a proxy pipeline.
         *
         *  @return Ptr to a PipelineLayout instance or nullptr if the function failed.
         **/
        Anvil::PipelineLayout* get_graphics_pipeline_layout(GraphicsPipelineID pipeline_id);

        /** Sets a new blend constant for the specified graphics pipeline and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param blend_constant_vec4  4 floats, specifying the constant. Must not be nullptr.
         *
         **/
        void set_blending_properties(GraphicsPipelineID graphics_pipeline_id,
                                     const float*       blend_constant_vec4);

        /** Updates color blend properties for the specified sub-pass attachment, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id    ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param attachment_id           ID of the subpass attachment, for which the color blend properties should be applied.
         *  @param blending_enabled        true if blending should be enabled for the specified attachment, false otherwise.
         *  @param blend_op_color          Blend operation color to use for the attachment.
         *  @param blend_op_alpha          Blend operation alpha to use for the attachment.
         *  @param src_color_blend_factor  Source blend factor for color components.
         *  @param dst_color_blend_factor  Destination blend factor for color components.
         *  @param src_alpha_blend_factor  Source blend factor for the alpha component.
         *  @param dst_alpha_blend_factor  Destination blend factor for the alpha component.
         *  @param channel_write_mask      Component write mask to use for the attachment.
         *
         **/
        void set_color_blend_attachment_properties(GraphicsPipelineID    graphics_pipeline_id,
                                                   SubPassAttachmentID   attachment_id,
                                                   bool                  blending_enabled,
                                                   VkBlendOp             blend_op_color,
                                                   VkBlendOp             blend_op_alpha,
                                                   VkBlendFactor         src_color_blend_factor,
                                                   VkBlendFactor         dst_color_blend_factor,
                                                   VkBlendFactor         src_alpha_blend_factor,
                                                   VkBlendFactor         dst_alpha_blend_factor,
                                                   VkColorComponentFlags channel_write_mask);

        /** Updates the number of scissor boxes to be used, when dynamic scissor state is enabled.
         *
         *  @param graphics_pipeline_id    ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param n_dynamic_scissor_boxes As per description.
         */
        void set_dynamic_scissor_state_properties(GraphicsPipelineID graphics_pipeline_id,
                                                  uint32_t           n_dynamic_scissor_boxes);

        /** Updates the number of viewports to be used, when dynamic viewport state is enabled.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param n_dynamic_viewports  As per description.
         */
        void set_dynamic_viewport_state_properties(GraphicsPipelineID graphics_pipeline_id,
                                                   uint32_t           n_dynamic_viewports);

        /** Sets the primitive topology to be used for the pipeline, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param primitive_topology   As per description.
         */
        void set_input_assembly_properties(GraphicsPipelineID  graphics_pipeline_id,
                                           VkPrimitiveTopology primitive_topology);

        /** Sets a number of multisampling properties to be used for the pipeline, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param sample_count         Number of rasterization samples to be used (expressed as one of the enum values).
         *  @param min_sample_shading   Minimum number of unique samples to shade for each fragment.
         *  @param sample_mask          Sample mask to use.
         */
        void set_multisampling_properties(GraphicsPipelineID    graphics_pipeline_id,
                                          VkSampleCountFlagBits sample_count,
                                          float                 min_sample_shading,
                                          const VkSampleMask    sample_mask);

        /** Sets a call-back, which GFX pipeline manager will invoke right after a vkCreateGraphicsPipeline() call is made.
         *
         *  The call-back will be issued right before the vkCreateGraphicsPipelines() call is issued.
         *
         *  This call overwrites any previous set_pipeline_post_bake_callback() calls.
         *
         *  @param graphics_pipeline_id       ID of the graphics pipeline to set the call-back for.
         *  @param pfn_pipeline_pre_bake_proc Function to call back. Must not be nullptr.
         *  @param user_arg                   User argument to pass with the call-back. May be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_pipeline_post_bake_callback(GraphicsPipelineID              graphics_pipeline_id,
                                             PFNPIPELINEPOSTBAKECALLBACKPROC pfn_pipeline_post_bake_proc,
                                             void*                           user_arg);

        /** Sets a call-back, which GFX pipeline manager will invoke right before a vkCreateGraphicsPipeline() call is made.
         *  This allows the callee to adjust the VkGraphicsPipelineCreateInfo instance, eg. by modifying pNext to user-defined
         *  chain of structures.
         *
         *  The call-back will be issued right before the vkCreateGraphicsPipelines() call is issued.
         *
         *  This call overwrites any previous set_pipeline_pre_bake_callback() calls.
         *
         *  @param graphics_pipeline_id       ID of the graphics pipeline to set the call-back for.
         *  @param pfn_pipeline_pre_bake_proc Function to call back. Must not be nullptr.
         *  @param user_arg                   User argument to pass with the call-back. May be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_pipeline_pre_bake_callback(GraphicsPipelineID             graphics_pipeline_id,
                                            PFNPIPELINEPREBAKECALLBACKPROC pfn_pipeline_pre_bake_proc,
                                            void*                          user_arg);

        /** Copies graphics state from @param source_pipeline_id to @param target_pipeline_id, leaving
         *  the originally assigned renderpass, as well as the subpass ID, unaffected.
         *
         *  @param target_pipeline_id ID of a graphics pipeline, to which state should be copied.
         *  @param source_pipeline_id ID of a graphics pipeline, from which state should be copied.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_pipeline_state_from_pipeline(GraphicsPipelineID target_pipeline_id,
                                              GraphicsPipelineID source_pipeline_id);

        /** Configures the rasterization order for the pipeline if the VK_AMD_rasterization_order extension
         *  is supported by the device, for which the pipeline has been created.
         *
         *  On drivers which do not support the extension, the call has no effect.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param rasterization_order  Rasterization order to use.
         **/
        void set_rasterization_order(GraphicsPipelineID      graphics_pipeline_id,
                                     VkRasterizationOrderAMD rasterization_order);

        /** Sets a number of rasterization properties to be used for the pipeline, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param polygon_mode         Polygon mode to use.
         *  @param cull_mode            Cull mode to use.
         *  @param front_face           Front face to use.
         *  @param line_width           Line width to use.
         **/
        void set_rasterization_properties(GraphicsPipelineID graphics_pipeline_id,
                                          VkPolygonMode      polygon_mode,
                                          VkCullModeFlags    cull_mode,
                                          VkFrontFace        front_face,
                                          float              line_width);

        /** Sets properties of a scissor box at the specified index, and marks the pipeline as dirty.
         *
         *  If @param n_scissor_box is larger than 1, all previous scissor boxes must also be defined
         *  prior to baking. Number of scissor boxes must match the number of viewports defined for the
         *  pipeline.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param n_scissor_box        Index of the scissor box to be updated.
         *  @param x                    X offset of the scissor box.
         *  @param y                    Y offset of the scissor box.
         *  @param width                Width of the scissor box.
         *  @param height               Height of the scissor box.
         **/
        void set_scissor_box_properties(GraphicsPipelineID graphics_pipeline_id,
                                        uint32_t           n_scissor_box,
                                        int32_t            x,
                                        int32_t            y,
                                        int32_t            width,
                                        int32_t            height);

        /** Sets a number of stencil test properties to be used for the pipeline, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id    ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                 call, issued against the same GraphicsPipelineManager instance.
         *  @param update_front_face_state true if the front face stencil states should be updated; false to update the
         *                                 back stencil states instead.
         *  @param stencil_fail_op         Stencil fail operation to use.
         *  @param stencil_pass_op         Stencil pass operation to use.
         *  @param stencil_depth_fail_op   Stencil depth fail operation to use.
         *  @param stencil_compare_op      Stencil compare operation to use.
         *  @param stencil_compare_mask    Stencil compare mask to use.
         *  @param stencil_write_mask      Stencil write mask to use.
         *  @param stencil_reference       Stencil reference value to use.
         **/
        void set_stencil_test_properties(GraphicsPipelineID graphics_pipeline_id,
                                         bool               update_front_face_state,
                                         VkStencilOp        stencil_fail_op,
                                         VkStencilOp        stencil_pass_op,
                                         VkStencilOp        stencil_depth_fail_op,
                                         VkCompareOp        stencil_compare_op,
                                         uint32_t           stencil_compare_mask,
                                         uint32_t           stencil_write_mask,
                                         uint32_t           stencil_reference);

        /** Updates the number of tessellation patch points to be used for the pipeline, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id   ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                call, issued against the same GraphicsPipelineManager instance.
         *  @param n_patch_control_points As per description.
         **/
        void set_tessellation_properties(GraphicsPipelineID graphics_pipeline_id,
                                         uint32_t           n_patch_control_points);

        /** Sets properties of a viewport at the specified index, and marks the pipeline as dirty.
         *
         *  If @param n_viewport is larger than 1, all previous viewports must also be defined
         *  prior to baking. Number of scissor boxes must match the number of viewports defined for the
         *  pipeline.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param n_viewport           Index of the viewport, whose properties should be changed.
         *  @param origin_x             X offset to use for the viewport's origin.
         *  @param origin_y             Y offset to use for the viewport's origin.
         *  @param width                Width of the viewport.
         *  @param height               Height of the viewport.
         *  @param min_depth            Minimum depth value to use for the viewport.
         *  @param max_depth            Maximum depth value to use for the viewport.
         **/
        void set_viewport_properties(GraphicsPipelineID graphics_pipeline_id,
                                     uint32_t           n_viewport,
                                     float              origin_x,
                                     float              origin_y,
                                     float              width,
                                     float              height,
                                     float              min_depth,
                                     float              max_depth);

        /** Enables or disables the "alpha to coverage" test, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the test; false to disable it.
         */
        void toggle_alpha_to_coverage(GraphicsPipelineID graphics_pipeline_id,
                                      bool               should_enable);

        /** Enables or disables the "alpha to one" test, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the test; false to disable it.
         */
        void toggle_alpha_to_one(GraphicsPipelineID graphics_pipeline_id,
                                 bool               should_enable);

        /** Enables or disables the "depth bias" mode, updates related state values, and marks
         *  the pipeline as dirty.
         *
         *  @param graphics_pipeline_id       ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                                    call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable              true to enable the mode; false to disable it.
         *  @param depth_bias_constant_factor Depth bias constant factor to use for the mode.
         *  @param depth_bias_clamp           Depth bias clamp to use for the mode.
         *  @param depth_bias_slope_factor    Slope scale for the depth bias to use for the mode.
         */
        void toggle_depth_bias(GraphicsPipelineID graphics_pipeline_id,
                               bool               should_enable,
                               float              depth_bias_constant_factor,
                               float              depth_bias_clamp,
                               float              depth_bias_slope_factor);

        /** Enables or disables the "depth bounds" test, updates related state values, and marks
         *  the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the test; false to disable it.
         *  @param min_depth_bounds     Minimum boundary value to use for the test.
         *  @param max_depth_bounds     Maximum boundary value to use for the test. 
         */
        void toggle_depth_bounds_test(GraphicsPipelineID graphics_pipeline_id,
                                      bool               should_enable,
                                      float              min_depth_bounds,
                                      float              max_depth_bounds);

        /** Enables or disables the "depth clamp" test, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the test; false to disable it.
         */
        void toggle_depth_clamp(GraphicsPipelineID graphics_pipeline_id,
                                bool               should_enable);

        /** Enables or disables the depth test, updates related state values, and marks the
         *  pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the mode; false to disable it.
         *  @param compare_op           Compare operation to use for the test.
         */
        void toggle_depth_test(GraphicsPipelineID graphics_pipeline_id,
                               bool               should_enable,
                               VkCompareOp        compare_op);

        /** Enables or disables depth writes, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the writes; false to disable them.
         */
        void toggle_depth_writes(GraphicsPipelineID graphics_pipeline_id,
                                 bool               should_enable);

        /** Enables or disables the specified dynamic states, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the dynamic state(s) specified by @param dynamic_state_bits, false
         *                              disable them if already enabled.
         *  @param dynamic_state_bits   An integer whose individual bits correspond to dynamic states which should either
         *                              be enabled or disabled.
         **/
        void toggle_dynamic_states (GraphicsPipelineID   graphics_pipeline_id,
                                    bool                 should_enable,
                                    DynamicStateBitfield dynamic_state_bits);

        /** Enables or disables logic ops, specifies which logic op should be used, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the mode; false to disable it.
         *  @param logic_op             Logic operation type to use.
         */
        void toggle_logic_op(GraphicsPipelineID graphics_pipeline_id,
                             bool               should_enable,
                             VkLogicOp          logic_op);

        /** Enables or disables the "primitive restart" mode, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the mode; false to disable it.
         */
        void toggle_primitive_restart(GraphicsPipelineID graphics_pipeline_id,
                                      bool               should_enable);

        /** Enables or disables the "rasterizer discard" mode, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the test; false to disable it.
         */
        void toggle_rasterizer_discard(GraphicsPipelineID graphics_pipeline_id,
                                       bool               should_enable);

        /** Enables or disables the "per-sample shading" mode, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the test; false to disable it.
         */
        void toggle_sample_shading(GraphicsPipelineID graphics_pipeline_id,
                                   bool               should_enable);

        /** Enables or disables the stencil test test, and marks the pipeline as dirty.
         *
         *  @param graphics_pipeline_id ID of the graphics pipeline to update. The ID must come from a preceding add_()
         *                              call, issued against the same GraphicsPipelineManager instance.
         *  @param should_enable        true to enable the test; false to disable it.
         */
        void toggle_stencil_test(GraphicsPipelineID graphics_pipeline_id,
                                 bool               should_enable);

    private:
        /* Private type declarations */

        /* Defines blending properties for a single subpass attachment.
         *
         * This descriptor is not exposed to the Vulkan implementation. It is used to form Vulkan-specific
         * descriptors at baking time instead.
         */
        typedef struct BlendingProperties
        {
            bool                  blend_enabled;
            VkBlendOp             blend_op_alpha;
            VkBlendOp             blend_op_color;
            VkColorComponentFlags channel_write_mask;
            VkBlendFactor         dst_alpha_blend_factor;
            VkBlendFactor         dst_color_blend_factor;
            VkBlendFactor         src_alpha_blend_factor;
            VkBlendFactor         src_color_blend_factor;

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
                result.blendEnable         = blend_enabled;
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
            int32_t x;
            int32_t y;
            int32_t width;
            int32_t height;

            /* Constructor. Should only be used by STL. */
            InternalScissorBox()
            {
                x      = 0;
                y      = 0;
                width  = 32;
                height = 32;
            }

            /* Constructor
             *
             * @param in_x      X offset of the scissor box
             * @param in_y      Y offset of the scissor box
             * @param in_width  Width of the scissor box
             * @param in_height Height of the scissor box
             **/
            InternalScissorBox(int32_t in_x,
                               int32_t in_y,
                               int32_t in_width,
                               int32_t in_height)
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
            VkFormat          format;
            uint32_t          location;
            uint32_t          offset_in_bytes;
            VkVertexInputRate rate;
            uint32_t          stride_in_bytes;

            /** Dummy constructor. Should only be used by STL. */
            InternalVertexAttribute()
            {
                format          = VK_FORMAT_UNDEFINED;
                location        = -1;
                offset_in_bytes = -1;
                rate            = VK_VERTEX_INPUT_RATE_MAX_ENUM;
                stride_in_bytes = -1;
            }

            /** Constructor.
             *
             *  @param in_format          Attribute format.
             *  @param in_location        Attribute location.
             *  @param in_offset_in_bytes Start offset in bytes.
             *  @param in_rate            Step rate.
             *  @param in_stride_in_bytes Stride in bytes.
             **/
            InternalVertexAttribute(VkFormat          in_format,
                                    uint32_t          in_location,
                                    uint32_t          in_offset_in_bytes,
                                    VkVertexInputRate in_rate,
                                    uint32_t          in_stride_in_bytes)
            {
                format          = in_format;
                location        = in_location;
                offset_in_bytes = in_offset_in_bytes;
                rate            = in_rate;
                stride_in_bytes = in_stride_in_bytes;
            }
        } InternalVertexAttribute;

        typedef std::vector<InternalVertexAttribute>     InternalVertexAttributes;
        typedef std::map<uint32_t, InternalScissorBox>   InternalScissorBoxes;
        typedef std::map<uint32_t, InternalViewport>     InternalViewports;

        /** Descriptor which encapsulates all state of a single graphics pipeline.
         *
         *  This descriptor is not exposed to the Vulkan implementation. It is used to form Vulkan-specific
         *  descriptors at baking time instead.
         *
         *  A single unique graphics pipeline descriptor should be assigned to each subpass.
         */
        struct GraphicsPipelineConfiguration
        {
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
            bool sample_shading_enabled;

            bool             stencil_test_enabled;
            VkStencilOpState stencil_state_back_face;
            VkStencilOpState stencil_state_front_face;

            VkRasterizationOrderAMD rasterization_order;

            InternalVertexAttributes                 attributes;
            float                                    blend_constant[4];
            VkCullModeFlags                          cull_mode;
            VkPolygonMode                            polygon_mode;
            VkFrontFace                              front_face;
            float                                    line_width;
            VkLogicOp                                logic_op;
            float                                    min_sample_shading;
            uint32_t                                 n_dynamic_scissor_boxes;
            uint32_t                                 n_dynamic_viewports;
            uint32_t                                 n_patch_control_points;
            VkPrimitiveTopology                      primitive_topology;
            VkSampleCountFlagBits                    sample_count;
            VkSampleMask                             sample_mask;
            InternalScissorBoxes                     scissor_boxes;
            SubPassAttachmentToBlendingPropertiesMap subpass_attachment_blending_properties;
            InternalViewports                        viewports;

            PFNPIPELINEPOSTBAKECALLBACKPROC pfn_pipeline_postbake_callback_proc;
            PFNPIPELINEPREBAKECALLBACKPROC  pfn_pipeline_prebake_callback_proc;
            void*                           pipeline_postbake_callback_user_arg;
            void*                           pipeline_prebake_callback_user_arg;

            RenderPass* renderpass_ptr;
            SubPassID   subpass_id;

            /** Constructor.
             *
             *  @param in_renderpass_ptr RenderPass instance the graphics pipeline will be used for.
             *                           The instance is retained. May be nullptr for proxy pipelines,
             *                           must not be nullptr for all other pipeline types.
             *  @param in_subpass_id     ID of the subpass the graphics pipeline descriptor is created
             *                           for.
             **/
            explicit GraphicsPipelineConfiguration(RenderPass* in_renderpass_ptr,
                                                   SubPassID   in_subpass_id)
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
                sample_shading_enabled     = false;
                stencil_test_enabled       = false;

                pfn_pipeline_postbake_callback_proc = nullptr;
                pfn_pipeline_prebake_callback_proc  = nullptr;
                pipeline_postbake_callback_user_arg = nullptr;
                pipeline_prebake_callback_user_arg  = nullptr;

                renderpass_ptr = in_renderpass_ptr;
                subpass_id     = in_subpass_id;

                if (renderpass_ptr != nullptr)
                {
                    renderpass_ptr->retain();
                }

                stencil_state_back_face.compareMask = ~0;
                stencil_state_back_face.compareOp   = VK_COMPARE_OP_ALWAYS;
                stencil_state_back_face.depthFailOp = VK_STENCIL_OP_KEEP;
                stencil_state_back_face.failOp      = VK_STENCIL_OP_KEEP;
                stencil_state_back_face.passOp      = VK_STENCIL_OP_KEEP;
                stencil_state_back_face.reference   = 0;
                stencil_state_back_face.writeMask   = ~0;
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
                sample_mask        = ~0;
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
                                                   RenderPass*                                    in_renderpass_ptr,
                                                   SubPassID                                      in_subpass_id)
            {
                /* Copy all state from the base config.. */
                *this = *in_base_pipeline_config_ptr;

                /* Override the renderpass & the subpass with the data we've been provided */
                renderpass_ptr = in_renderpass_ptr;
                subpass_id     = in_subpass_id;

                if (renderpass_ptr != nullptr)
                {
                    renderpass_ptr->retain();
                }
            }

            /** Destructor. */
            ~GraphicsPipelineConfiguration()
            {
                if (renderpass_ptr != nullptr)
                {
                    renderpass_ptr->release();

                    renderpass_ptr = nullptr;
                }
            }

            /** Copy assignment operator. */
            GraphicsPipelineConfiguration& operator=(const GraphicsPipelineConfiguration& in)
            {
                /* NOTE: We leave window, renderpass & subpass ID intact. */
                vk_input_attributes = in.vk_input_attributes;
                vk_input_bindings   = in.vk_input_bindings;

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
        void bake_vk_attributes_and_bindings(std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr);

        /* Private members */
        GraphicsPipelineConfigurations m_pipeline_configurations;
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_GRAPHICS_PIPELINE_MANAGER_H */