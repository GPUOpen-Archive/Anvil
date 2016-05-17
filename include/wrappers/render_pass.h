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

/** Defines a render pass wrapper class which simplifies the following processes:
 *
 *  - Attachment configuration & management
 *  - Life-time management
 *  - Render pass initialization and tear-down.
 *  - Subpass configuration & management
 *  - Support for adding new renderpass/subpass attachments or subpasses with automatic Vulkan FB object re-creation.
 **/
#ifndef WRAPPERS_RENDER_PASS_H
#define WRAPPERS_RENDER_PASS_H

#include "misc/callbacks.h"
#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
    enum RenderPassCallbackID
    {
        /* Call-back issued whenever the originating renderpass becomes dirty
         *
         * callback_arg: source RenderPass instance
         */
        RENDER_PASS_CALLBACK_ID_BAKING_NEEDED,

        /* Always last */
        RENDER_PASS_CALLBACK_ID_COUNT
    };

    class RenderPass : public CallbacksSupportProvider,
                       public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Creates a new RenderPass wrapper instance.
         *
         *  NOTE: This function takes a Swapchain instance, which is later passed to GFX pipelines at creation time.
         *        The actual Swapchain wrapper is only used, if the GFX pipeline is not specified a scissor area and/or
         *        viewport size, in which case it needs to deduce that info from the swapchain.
         *
         *        Passing a non-nullptr value through the <opt_swapchain_ptr> argument is optional, in which case Anvil will
         *        throw a NPE if it ever needs to deduce the window size.
         *
         *
         *  @param device_ptr        Device which will consume the renderpass. Must not be nullptr.
         *  @param opt_swapchain_ptr Swapchain, that the render-pass will render to. See brief for more information.
         *                           May be nullptr.
         *
         **/
        RenderPass(Anvil::Device*    device_ptr,
                   Anvil::Swapchain* opt_swapchain_ptr);

        virtual void retain()
        {
            RefCounterSupportProvider::retain();
        }

        /** Adds a new render-pass color attachment to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param format                Image format to use for the color attachment.
         *  @param sample_count          Number of samples to use for the color attachment (expressed as enum value).
         *  @param load_op               Load operation to use for the color attachment.
         *  @param store_op              Store operation to use for the color attachment.
         *  @param initial_layout        Initial layout to use for the color attachment.
         *  @param final_layout          Final layout to use for the color attachment.
         *  @param may_alias             true if the memory backing of the image that will be attached may
         *                               overlap with the backing of another attachment within the same
         *                               subpass.
         *  @param out_attachment_id_ptr Deref will be set to a unique ID assigned to the new color attachment.
         *                               Must not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_color_attachment(VkFormat                format,
                                  VkSampleCountFlagBits   sample_count,
                                  VkAttachmentLoadOp      load_op,
                                  VkAttachmentStoreOp     store_op,
                                  VkImageLayout           initial_layout,
                                  VkImageLayout           final_layout,
                                  bool                    may_alias,
                                  RenderPassAttachmentID* out_attachment_id_ptr);

        /** Adds a new render-pass depth/stencil attachment to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param format                Image format to use for the color attachment.
         *  @param sample_count          Number of samples to use for the depth-stencil attachment (expressed as enum value).
         *  @param depth_load_op         Load operation to use for the depth data.
         *  @param depth_store_op        Store operation to use for the depth data.
         *  @param stencil_load_op       Load operation to use for the stencil data.
         *  @param stencil_store_op      Store operation to use for the stencil data.
         *  @param initial_layout        Initial layout to use for the color attachment.
         *  @param final_layout          Final layout to use for the color attachment.
         *  @param may_alias             true if the memory backing of the image that will be attached may
         *                               overlap with the backing of another attachment within the same
         *                               subpass.
         *  @param out_attachment_id_ptr Deref will be set to a unique ID assigned to the new color attachment.
         *                               Must not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_depth_stencil_attachment(VkFormat                format,
                                          VkSampleCountFlagBits   sample_count,
                                          VkAttachmentLoadOp      depth_load_op,
                                          VkAttachmentStoreOp     depth_store_op,
                                          VkAttachmentLoadOp      stencil_load_op,
                                          VkAttachmentStoreOp     stencil_store_op,
                                          VkImageLayout           initial_layout,
                                          VkImageLayout           final_layout,
                                          bool                    may_alias,
                                          RenderPassAttachmentID* out_attachment_id_ptr);

        /** Adds a new subpass to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param fragment_shader_entrypoint        Shader module stage entrypoint descriptor for the fragment stage.
         *                                           Please pass an instance created by a dummy constructor if the stage
         *                                           is irrelevant.
         *  @param geometry_shader_entrypoint        Shader module stage entrypoint descriptor for the geometry stage.
         *                                           Please pass an instance created by a dummy constructor if the stage
         *                                           is irrelevant.
         *  @param tess_control_shader_entrypoint    Shader module stage entrypoint descriptor for the tessellation control stage.
         *                                           Please pass an instance created by a dummy constructor if the stage
         *                                           is irrelevant.
         *  @param tess_evaluation_shader_entrypoint Shader module stage entrypoint descriptor for the tessellation
         *                                           evaluation stage. Please pass an instance created by a dummy constructor
         *                                           if the stage is irrelevant.
         *  @param vertex_shader_entrypoint          Shader module stage entrypoint descriptor for the vertex stage.
         *                                           Must NOT be a dummy instance.
         *  @param out_subpass_id_ptr                Deref will be set to a unique ID of the created subpass.
         *                                           Must not be nullptr.
         *  @param opt_pipeline_id                   (optional) ID of a graphics pipeline to use for the graphics pipeline.
         *                                           If not specified, a new pipeline wil be created from scratch.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_subpass(const ShaderModuleStageEntryPoint& fragment_shader_entrypoint,
                         const ShaderModuleStageEntryPoint& geometry_shader_entrypoint,
                         const ShaderModuleStageEntryPoint& tess_control_shader_entrypoint,
                         const ShaderModuleStageEntryPoint& tess_evaluation_shader_entrypoint,
                         const ShaderModuleStageEntryPoint& vertex_shader_entrypoint,
                         SubPassID*                         out_subpass_id_ptr,
                         const Anvil::PipelineID            opt_pipeline_id = -1);

        /** Adds a new color attachment to the RenderPass instance's specified subpass.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param subpass_id                ID of the render-pass subpass to update.
         *  @param layout                    Layout to use for the attachment when executing the subpass.
         *                                   Driver takes care of transforming the attachment to the requested layout
         *                                   before subpass commands starts executing.
         *  @param attachment_id             ID of the render-pass attachment the new sub-pass color attachment
         *                                   should refer to.
         *  @param location                  Location, under which the specified attachment should be accessible to
         *                                   the fragment shader.
         *  @param attachment_resolve_id_ptr Must be nullptr if the new color attachment should be single-sample.
         *                                   For multi-sample, this argument can optionally point to a render-pass
         *                                   attachment descriptor, to which the MS attachment should be resolved to.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_subpass_color_attachment(SubPassID                     subpass_id,
                                          VkImageLayout                 layout,
                                          RenderPassAttachmentID        attachment_id,
                                          uint32_t                      location,
                                          const RenderPassAttachmentID* opt_attachment_resolve_id_ptr);

        /** Adds a new input attachment to the RenderPass instance's specified subpass.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param subpass_id       ID of the render-pass subpass to update.
         *  @param layout           Layout to use for the attachment when executing the subpass.
         *                          Driver takes care of transforming the attachment to the requested layout
         *                          before subpass commands starts executing.
         *  @param attachment_id    ID of the render-pass attachment the new sub-pass input attachment
         *                          should refer to.
         *  @param attachment_index The "input attachment index", under which the specified attachment should
         *                          be accessible to the fragment shader. Do not forget the image view also
         *                          needs to be bound to the descriptor set for the input attachment to work!
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_subpass_input_attachment(SubPassID              subpass_id,
                                          VkImageLayout          layout,
                                          RenderPassAttachmentID attachment_id,
                                          uint32_t               attachment_index);

        /** Configures the depth+stencil attachment the subpass should use.
         *
         *  Note that only up to one depth/stencil attachment may be added for each subpass.
         *  Any attempt to add more such attachments will results in an assertion failure.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param subpass_id    ID of the subpass to update the depth+stencil attachment for.
         *                       The subpass must have been earlier created with an add_subpass() call.
         *  @param attachment_id ID of the render-pass attachment the depth-stencil attachment should refer to.
         *  @param layout        Layout to use for the attachment when executing the subpass.
         *                       Driver takes care of transforming the attachment to the requested layout
         *                       before subpass commands starts executing.
         *
         *  @return true if the function executed successfully, false otherwise.
         *
         */
        bool add_subpass_depth_stencil_attachment(SubPassID              subpass_id,
                                                  RenderPassAttachmentID attachment_id,
                                                  VkImageLayout          layout);

        /** Adds a new external->subpass dependency to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param destination_subpass_id  ID of the destination subpass. The subpass must have been
         *                                 created earlier with an add_subpass() call.
         *  @param source_stage_mask       Source pipeline stage mask.
         *  @param destination_stage_mask  Destination pipeline stage mask.
         *  @param source_access_mask      Source access mask.
         *  @param destination_access_mask Destination access mask.
         *  @param by_region               true if a "by-region" dependency is requested; false otherwise.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         *
         **/
        bool add_external_to_subpass_dependency(SubPassID            destination_subpass_id,
                                                VkPipelineStageFlags source_stage_mask,
                                                VkPipelineStageFlags destination_stage_mask,
                                                VkAccessFlags        source_access_mask,
                                                VkAccessFlags        destination_access_mask,
                                                bool                 by_region);

        /** Adds a new self-subpass dependency to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param destination_subpass_id  ID of the subpass to create the dep for. The subpass must have been
         *                                 created earlier with an add_subpass() call.
         *  @param source_stage_mask       Source pipeline stage mask.
         *  @param destination_stage_mask  Destination pipeline stage mask.
         *  @param source_access_mask      Source access mask.
         *  @param destination_access_mask Destination access mask.
         *  @param by_region               true if a "by-region" dependency is requested; false otherwise.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         *
         **/
        bool add_self_subpass_dependency(SubPassID            destination_subpass_id,
                                         VkPipelineStageFlags source_stage_mask,
                                         VkPipelineStageFlags destination_stage_mask,
                                         VkAccessFlags        source_access_mask,
                                         VkAccessFlags        destination_access_mask,
                                         bool                 by_region);

        /** Adds a new subpass->external dependency to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param source_subpass_id       ID of the source subpass. The subpass must have been
         *                                 created earlier with an add_subpass() call.
         *  @param source_stage_mask       Source pipeline stage mask.
         *  @param destination_stage_mask  Destination pipeline stage mask.
         *  @param source_access_mask      Source access mask.
         *  @param destination_access_mask Destination access mask.
         *  @param by_region               true if a "by-region" dependency is requested; false otherwise.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         **/
        bool add_subpass_to_external_dependency(SubPassID            source_subpass_id,
                                                VkPipelineStageFlags source_stage_mask,
                                                VkPipelineStageFlags destination_stage_mask,
                                                VkAccessFlags        source_access_mask,
                                                VkAccessFlags        destination_access_mask,
                                                bool                 by_region);

        /** Adds a new subpass->subpass dependency to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param source_subpass_id       ID of the source subpass. The subpass must have been
         *                                 created earlier with an add_subpass() call.
         *  @param destination_subpass_id  ID of the source subpass. The subpass must have been
         *                                 created earlier with an add_subpass() call.
         *  @param source_stage_mask       Source pipeline stage mask.
         *  @param destination_stage_mask  Destination pipeline stage mask.
         *  @param source_access_mask      Source access mask.
         *  @param destination_access_mask Destination access mask.
         *  @param by_region               true if a "by-region" dependency is requested; false otherwise.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         **/
        bool add_subpass_to_subpass_dependency(SubPassID            source_subpass_id,
                                               SubPassID            destination_subpass_id,
                                               VkPipelineStageFlags source_stage_mask,
                                               VkPipelineStageFlags destination_stage_mask,
                                               VkAccessFlags        source_access_mask,
                                               VkAccessFlags        destination_access_mask,
                                               bool                 by_region);

        /** Re-creates the internal VkRenderPass object.
         *
         *  This function should be considered expensive.
         *
         *  @return true if successful, false otherwise.
         **/
        bool bake();

        /** Retrieves properties of the render pass color attachment with the user-specified ID
         *
         *  @param attachment_id              ID of the attachment to retrieve properties of.
         *  @param out_opt_sample_count_ptr   If not nullptr, deref will be set to the sample count, specified
         *                                    at attachment creation time. May be nullptr.
         *  @param out_opt_load_op_ptr        If not nullptr, deref will be set to the load op, specified at
         *                                    attachment creation time. May be nullptr.
         *  @param out_opt_store_op_ptr       If not nullptr, deref will be set to the store op, specified at
         *                                    attachment creation time. May be nullptr.
         *  @param out_opt_initial_layout_ptr If not nullptr, deref will be set to the initial layout, specified
         *                                    at attachment creation time. May be nullptr.
         *  @param out_opt_final_layout_ptr   If not nullptr, deref will be set to the final layout, specified at
         *                                    attachment creation time. May be nullptr.
         *  @param out_opt_may_alias_ptr      If not nullptr, deref will be set to set to true, if the attachment
         *                                    can alias other attachments. Otherwise, it will be set to false.
         *                                    May be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_color_attachment_properties(RenderPassAttachmentID attachment_id,
                                             VkSampleCountFlagBits* out_opt_sample_count_ptr   = nullptr,
                                             VkAttachmentLoadOp*    out_opt_load_op_ptr        = nullptr,
                                             VkAttachmentStoreOp*   out_opt_store_op_ptr       = nullptr,
                                             VkImageLayout*         out_opt_initial_layout_ptr = nullptr,
                                             VkImageLayout*         out_opt_final_layout_ptr   = nullptr,
                                             bool*                  out_opt_may_alias_ptr      = nullptr);

        /** Retrieves properties of the render pass color attachment with the user-specified ID
         *
         *  @param attachment_id                ID of the attachment to retrieve properties of.
         *  @param out_opt_depth_load_op_ptr    If not nullptr, deref will be set to the depth-specific load op, specified at
         *                                      attachment creation time. May be nullptr.
         *  @param out_opt_depth_store_op_ptr   If not nullptr, deref will be set to the depth-specific store op, specified at
         *                                      attachment creation time. May be nullptr.
         *  @param out_opt_stencil_load_op_ptr  If not nullptr, deref will be set to the stencil-specific load op, specified at
         *                                      attachment creation time. May be nullptr.
         *  @param out_opt_stencil_store_op_ptr If not nullptr, deref will be set to the stencil-specific store op, specified at
         *                                      attachment creation time. May be nullptr.
         *  @param out_opt_initial_layout_ptr   If not nullptr, deref will be set to the initial layout, specified
         *                                      at attachment creation time. May be nullptr.
         *  @param out_opt_final_layout_ptr     If not nullptr, deref will be set to the final layout, specified at
         *                                      attachment creation time. May be nullptr.
         *  @param out_opt_may_alias_ptr        If not nullptr, deref will be set to set to true, if the attachment
         *                                      can alias other attachments. Otherwise, it will be set to false.
         *                                      May be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_depth_stencil_attachment_properties(RenderPassAttachmentID attachment_id,
                                                     VkAttachmentLoadOp*    out_opt_depth_load_op_ptr    = nullptr,
                                                     VkAttachmentStoreOp*   out_opt_depth_store_op_ptr   = nullptr,
                                                     VkAttachmentLoadOp*    out_opt_stencil_load_op_ptr  = nullptr,
                                                     VkAttachmentStoreOp*   out_opt_stencil_store_op_ptr = nullptr,
                                                     VkImageLayout*         out_opt_initial_layout_ptr   = nullptr,
                                                     VkImageLayout*         out_opt_final_layout_ptr     = nullptr,
                                                     bool*                  out_opt_may_alias_ptr        = nullptr);

        /** Returns the number of added subpasses */
        uint32_t get_n_subpasses() const
        {
            return (uint32_t) m_subpasses.size();
        }

        /** Bakes the VkRenderPass object if the RenderPass instance is marked as dirty, and returns it.
         *
         *  @return The requested object.
         **/
        VkRenderPass get_render_pass();

        /** Returns the graphics pipeline ID, associated with the specified subpass.
         *
         *  @param subpass_id                   As per description.
         *  @param out_graphics_pipeline_id_ptr Deref will be set to the aforementioned ID. Must not be nullptr.
         *                                      Will only be touched if the function returns true.
         *
         *  @return true if the function was successful, false otherwise.
         **/
        bool get_subpass_graphics_pipeline_id(SubPassID           subpass_id,
                                              GraphicsPipelineID* out_graphics_pipeline_id_ptr) const;

        /** Returns the number of color attachments, defined for the specified subpass.
         *
         *  @param subpass_id                  As per description.
         *  @param out_n_color_attachments_ptr Deref will be set to the value above.. Must not be nullptr.
         *                                     Will only be touched if the function returns true.
         *
         *  @return true if the function was successful, false otherwise.
         **/
        bool get_subpass_n_color_attachments(SubPassID subpass_id,
                                             uint32_t* out_n_color_attachments_ptr) const;

        /** Returns the Swapchain instance, associated with the RenderPass wrapper instance at creation time. */
        Anvil::Swapchain* get_swapchain() const
        {
            return m_swapchain_ptr;
        }

        /** Tells if a depth (+stencil) attachment has been defined for the specified subpass.
         *
         *  @param subpass_id     As per description
         *  @param out_result_ptr If function returns true: deref will be set to true if the attachment has
         *                        been defined; otherwise, it will be set to false.
         *
         *  @return true if successful, false otherwise.
         **/
        bool is_depth_stencil_attachment_defined_for_subpass(SubPassID subpass_id,
                                                             bool*     out_result_ptr) const;

        /* Releases the graphics pipeline, as used by the specified subpass at the time of the call,
         * and assigns the user-specified one.
         *
         *  @param subpass_id               ID of the subpass, whose graphics pipeline should be changed.
         *  @param new_graphics_pipeline_id ID of the graphics pipeline to start using.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_subpass_graphics_pipeline_id(SubPassID          subpass_id,
                                              GraphicsPipelineID new_graphics_pipeline_id);

    private:
        /* Private type definitions */
        
        /** Holds properties of a single render-pass attachment. **/
        typedef struct RenderPassAttachment
        {
            VkAttachmentLoadOp    color_depth_load_op;
            VkAttachmentStoreOp   color_depth_store_op;
            VkImageLayout         final_layout;
            VkFormat              format;
            uint32_t              index;
            VkImageLayout         initial_layout;
            bool                  may_alias;
            VkSampleCountFlagBits sample_count;
            VkAttachmentLoadOp    stencil_load_op;
            VkAttachmentStoreOp   stencil_store_op;

            /** Constructor. Should only be used for color attachments.
             *
             *  @param in_format         Format that will be used by the render-pass attachment.
             *  @param in_sample_count   Number of samples of the render-pass attachment (expressed as enum value)
             *  @param in_load_op        Load operation to use for the render-pass attachment.
             *  @param in_store_op       Store operation to use for the render-pass attachment.
             *  @param in_initial_layout Initial layout fo the render-pass attachment.
             *  @param in_final_layout   Layout to transfer the render-pass attachment to, after
             *                           the render-pass finishes.
             *  @param in_may_alias      true if the attachment's memory backing may alias with
             *                           memory region of another attachment; false otherwise.
             *  @param in_index          Index of the created render-pass attachment.
             ***/
            RenderPassAttachment(VkFormat              in_format,
                                 VkSampleCountFlagBits in_sample_count,
                                 VkAttachmentLoadOp    in_load_op,
                                 VkAttachmentStoreOp   in_store_op,
                                 VkImageLayout         in_initial_layout,
                                 VkImageLayout         in_final_layout,
                                 bool                  in_may_alias,
                                 uint32_t              in_index)
            {
                color_depth_load_op  = in_load_op;
                color_depth_store_op = in_store_op;
                final_layout         = in_final_layout;
                format               = in_format;
                index                = in_index;
                initial_layout       = in_initial_layout;
                may_alias            = in_may_alias;
                sample_count         = in_sample_count;
                stencil_load_op      = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                stencil_store_op     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            }

            /** Constructor. Should only be used for depth/stencil attachments
             *
             *  @param in_format           Format that will be used by the render-pass attachment.
             *  @param in_sample_count   Number of samples of the render-pass attachment (expressed as enum value)
             *  @param in_depth_load_op    Load operation to use for the render-pass attachment's depth data.
             *  @param in_depth_store_op   Store operation to use for the render-pass attachment's depth data.
             *  @param in_stencil_load_op  Load operation to use for the render-pass attachment's stencil data.
             *  @param in_stencil_store_op Store operation to use for the render-pass attachment's stencil data.
             *  @param in_initial_layout   Initial layout fo the render-pass attachment.
             *  @param in_final_layout     Layout to transfer the render-pass attachment to, after
             *                             the render-pass finishes.
             *  @param in_may_alias        true if the attachment's memory backing may alias with
             *                             memory region of another attachment; false otherwise.
             *  @param in_index            Index of the created render-pass attachment.
             **/
            RenderPassAttachment(VkFormat              in_format,
                                 VkSampleCountFlagBits in_sample_count,
                                 VkAttachmentLoadOp    in_depth_load_op,
                                 VkAttachmentStoreOp   in_depth_store_op,
                                 VkAttachmentLoadOp    in_stencil_load_op,
                                 VkAttachmentStoreOp   in_stencil_store_op,
                                 VkImageLayout         in_initial_layout,
                                 VkImageLayout         in_final_layout,
                                 bool                  in_may_alias,
                                 uint32_t              in_index)
            {
                color_depth_load_op  = in_depth_load_op;
                color_depth_store_op = in_depth_store_op;
                final_layout         = in_final_layout;
                format               = in_format;
                index                = in_index;
                initial_layout       = in_initial_layout;
                may_alias            = in_may_alias;
                sample_count         = in_sample_count;
                stencil_load_op      = in_stencil_load_op;
                stencil_store_op     = in_stencil_store_op;
            }

            /** Dummy constructor. This should only be used by STL containers. */
            RenderPassAttachment()
            {
                color_depth_load_op  = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
                color_depth_store_op = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
                final_layout         = VK_IMAGE_LAYOUT_MAX_ENUM;
                format               = VK_FORMAT_MAX_ENUM;
                index                = -1;
                initial_layout       = VK_IMAGE_LAYOUT_MAX_ENUM;
                may_alias            = false;
                sample_count         = static_cast<VkSampleCountFlagBits>(0);
                stencil_load_op      = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
                stencil_store_op     = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
            }
        } RenderPassAttachment;

        typedef std::vector<RenderPassAttachment> RenderPassAttachments;

        /* Holds properties of a sub-pass attachment */
        typedef struct SubPassAttachment
        {
            RenderPassAttachment* attachment_ptr;
            uint32_t              highest_subpass_index;
            VkImageLayout         layout;
            uint32_t              lowest_subpass_index;
            RenderPassAttachment* resolve_attachment_ptr;

            /* Dummy constructor. Should only be used by STL containers */
            SubPassAttachment()
            {
                attachment_ptr         = nullptr;
                highest_subpass_index  = -1;
                layout                 = VK_IMAGE_LAYOUT_MAX_ENUM;
                lowest_subpass_index   = -1;
                resolve_attachment_ptr = nullptr;
            }

            /** Constructor.
             *
             *  @param in_attachment_ptr         Render-pass attachment that this sub-pass attachment should reference.
             *                                   Must not be nullptr.
             *  @param in_layout                 Layout to use for the attachment when executing the subpass.
             *                                   Driver takes care of transforming the attachment to the requested layout
             *                                   before subpass commands starts executing.
             *  @param in_resolve_attachment_ptr If not nullptr, this should point to the render-pass attachment, to which
             *                                   MS data of @param in_attachment_ptr should be resolved. If nullptr, it is
             *                                   assumed the sub-pass should not resolve the MS data.
             **/
            SubPassAttachment(RenderPassAttachment* in_attachment_ptr,
                              VkImageLayout         in_layout,
                              RenderPassAttachment* in_opt_resolve_attachment_ptr)
            {
                attachment_ptr         = in_attachment_ptr;
                highest_subpass_index  = -1;
                layout                 = in_layout;
                lowest_subpass_index   = -1;
                resolve_attachment_ptr = in_opt_resolve_attachment_ptr;
            }
        } SubPassAttachment;

        typedef std::map<uint32_t, SubPassAttachment> LocationToSubPassAttachmentMap;
        typedef std::vector<SubPassAttachment>        SubPassAttachmentVector;
        
        /** Holds properties of a single sub-pass */
        typedef struct SubPass
        {
            LocationToSubPassAttachmentMap color_attachments_map;
            SubPassAttachment              depth_stencil_attachment;
            Anvil::Device*                 device_ptr;
            uint32_t                       index;
            LocationToSubPassAttachmentMap input_attachments_map;
            GraphicsPipelineID             pipeline_id;
            SubPassAttachmentVector        preserved_attachments;
            LocationToSubPassAttachmentMap resolved_attachments_map;

            SubPassAttachment* get_color_attachment_at_index(uint32_t index)
            {
                return get_attachment_at_index(color_attachments_map,
                                               index);
            }

            SubPassAttachment* get_input_attachment_at_index(uint32_t index)
            {
                return get_attachment_at_index(input_attachments_map,
                                               index);
            }

            SubPassAttachment* get_resolved_attachment_at_index(uint32_t index)
            {
                return get_attachment_at_index(resolved_attachments_map,
                                               index);
            }

            /** Dummy constructor. This should only be used by STL containers */
            SubPass()
            {
                device_ptr  = nullptr;
                index       = -1;
                pipeline_id = -1;
            }

            /** Constructor.
             *
             *  @param in_index       Index of the sub-pass
             *  @param in_pipeline_id ID of the graphics pipeline which is associated with the subpass.
             *
             **/
            SubPass(Anvil::Device*     in_device_ptr,
                    uint32_t           in_index,
                    GraphicsPipelineID in_pipeline_id)
            {
                device_ptr  = in_device_ptr;
                index       = in_index;
                pipeline_id = in_pipeline_id;
            }

            /* Destructor */
            ~SubPass();

            private:
                /** Returns a pointer to the SubPassAttachment instance, assigned to the index specified
                 *  under @param index in @param map.
                 **/
                SubPassAttachment* get_attachment_at_index(LocationToSubPassAttachmentMap& map,
                                                           uint32_t                        index)
                {
                    uint32_t           current_index = 0;
                    SubPassAttachment* result_ptr    = nullptr;

                    for (auto attachment_iterator  = map.begin();
                              attachment_iterator != map.end();
                            ++attachment_iterator, ++current_index)
                    {
                        if (current_index == index)
                        {
                            result_ptr = &attachment_iterator->second;

                            break;
                        }
                    }

                    return result_ptr;
                }

                SubPass           (const SubPass&);
                SubPass& operator=(const SubPass&);
        } SubPass;

        typedef std::vector<SubPass*> SubPasses;

        /** Holds properties of a single subpass<->subpass dependency. */
        typedef struct SubPassDependency
        {
            bool                    by_region;
            VkAccessFlagBits        destination_access_mask;
            VkPipelineStageFlagBits destination_stage_mask;
            const SubPass*          destination_subpass_ptr;
            VkAccessFlagBits        source_access_mask;
            VkPipelineStageFlagBits source_stage_mask;
            const SubPass*          source_subpass_ptr;

            /** Constructor.
             *
             *  @param in_destination_stage_mask  Destination pipeline stage mask.
             *  @param in_destination_subpass_ptr Pointer to the descriptor of the destination subpass.
             *                                    If nullptr, it is assumed an external destination is requested.
             *  @param in_source_stage_mask       Source pipeline stage mask.
             *  @param in_source_subpass_ptr      Pointer to the descriptor of the source subpass.
             *                                    If nullptr, it is assumed an external source is requested.
             *  @param in_source_access_mask      Source access mask.
             *  @param in_destination_access_mask Destination access mask.
             *  @param in_by_region               true if a "by-region" dependency is requested; false otherwise.
             **/
            SubPassDependency(VkPipelineStageFlags in_destination_stage_mask,
                              const SubPass*       in_destination_subpass_ptr,
                              VkPipelineStageFlags in_source_stage_mask,
                              const SubPass*       in_source_subpass_ptr,
                              VkAccessFlags        in_source_access_mask,
                              VkAccessFlags        in_destination_access_mask,
                              bool                 in_by_region)
            {
                by_region               = in_by_region;
                destination_stage_mask  = static_cast<VkPipelineStageFlagBits>(in_destination_stage_mask);
                destination_subpass_ptr = in_destination_subpass_ptr;
                destination_access_mask = static_cast<VkAccessFlagBits>       (in_destination_access_mask);
                source_access_mask      = static_cast<VkAccessFlagBits>       (in_source_access_mask);
                source_stage_mask       = static_cast<VkPipelineStageFlagBits>(in_source_stage_mask);
                source_subpass_ptr      = in_source_subpass_ptr;
            }

            /** Dummy constructor. Should only be used by STL containers */
            SubPassDependency()
            {
                destination_access_mask = static_cast<VkAccessFlagBits>       (0);
                destination_stage_mask  = static_cast<VkPipelineStageFlagBits>(0);
                destination_subpass_ptr = nullptr;
                source_access_mask      = static_cast<VkAccessFlagBits>       (0);
                source_stage_mask       = static_cast<VkPipelineStageFlagBits>(0);
                source_subpass_ptr      = nullptr;
            }

            /** Comparator operator */
            bool operator==(const SubPassDependency& in)
            {
                return in.by_region               == by_region &&
                       in.destination_access_mask == destination_access_mask &&
                       in.destination_stage_mask  == destination_stage_mask  &&
                       in.destination_subpass_ptr == destination_subpass_ptr &&
                       in.source_access_mask      == source_access_mask      &&
                       in.source_stage_mask       == source_stage_mask       &&
                       in.source_subpass_ptr      == source_subpass_ptr;
            }
        } SubPassDependency;

        typedef std::vector<SubPassDependency> SubPassDependencies;

        /* Private functions */
         RenderPass& operator=(const RenderPass&);
         RenderPass           (const RenderPass&);

         virtual ~RenderPass();

        bool add_dependency        (SubPass*               destination_subpass_ptr,
                                    SubPass*               source_subpass_ptr,
                                    VkPipelineStageFlags   source_stage_mask,
                                    VkPipelineStageFlags   destination_stage_mask,
                                    VkAccessFlags          source_access_mask,
                                    VkAccessFlags          destination_access_mask,
                                    bool                   by_region);
        bool add_subpass_attachment(SubPassID              subpass_id,
                                    bool                   is_color_attachment,
                                    VkImageLayout          input_layout,
                                    RenderPassAttachmentID attachment_id,
                                    uint32_t               attachment_location,
                                    bool                   should_resolve,
                                    RenderPassAttachmentID resolve_attachment_id);

        VkAttachmentReference get_attachment_reference_from_renderpass_attachment(const RenderPassAttachment& renderpass_attachment) const;
        VkAttachmentReference get_attachment_reference_from_subpass_attachment   (const SubPassAttachment&    subpass_attachment)    const;
        void                  update_preserved_attachments                       ();

        /* Private members */
        RenderPassAttachments m_attachments;
        Anvil::Device*        m_device_ptr;
        bool                  m_dirty;
        VkRenderPass          m_render_pass;
        SubPasses             m_subpasses;
        SubPassDependencies   m_subpass_dependencies;
        Swapchain*            m_swapchain_ptr;
    };

    struct RenderPassDeleter
    {
        /* Delete functor. Useful when a renderpass instance needs to be wrapped in an auto pointer */
        void operator()(RenderPass* renderpass_ptr)
        {
            renderpass_ptr->release();
        }
    };
};

#endif /* WRAPPERS_RENDER_PASS_H */