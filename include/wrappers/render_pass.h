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
#include "misc/debug_marker.h"
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
                       public DebugMarkerSupportProvider<RenderPass>,
                       public std::enable_shared_from_this<RenderPass>
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
         *  @param in_device_ptr        Device which will consume the renderpass. Must not be nullptr.
         *  @param in_opt_swapchain_ptr Swapchain, that the render-pass will render to. See brief for more information.
         *                              May be nullptr.
         *
         **/
        static std::shared_ptr<RenderPass> create(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
                                                  std::shared_ptr<Anvil::Swapchain> in_opt_swapchain_ptr);

        /** Destructor.
         *
         *  Destroys the Vulkan counterpart and unregisters the wrapper instance from the object tracker.
         **/
        virtual ~RenderPass();

        /** Adds a new render-pass color attachment to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param in_format             Image format to use for the color attachment.
         *  @param in_sample_count       Number of samples to use for the color attachment (expressed as enum value).
         *  @param in_load_op            Load operation to use for the color attachment.
         *  @param in_store_op           Store operation to use for the color attachment.
         *  @param in_initial_layout     Initial layout to use for the color attachment.
         *  @param in_final_layout       Final layout to use for the color attachment.
         *  @param in_may_alias          true if the memory backing of the image that will be attached may
         *                               overlap with the backing of another attachment within the same
         *                               subpass.
         *  @param out_attachment_id_ptr Deref will be set to a unique ID assigned to the new color attachment.
         *                               Must not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_color_attachment(VkFormat                in_format,
                                  VkSampleCountFlags      in_sample_count,
                                  VkAttachmentLoadOp      in_load_op,
                                  VkAttachmentStoreOp     in_store_op,
                                  VkImageLayout           in_initial_layout,
                                  VkImageLayout           in_final_layout,
                                  bool                    in_may_alias,
                                  RenderPassAttachmentID* out_attachment_id_ptr);

        /** Adds a new render-pass depth/stencil attachment to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param in_format             Image format to use for the color attachment.
         *  @param in_sample_count       Number of samples to use for the depth-stencil attachment (expressed as enum value).
         *  @param in_depth_load_op      Load operation to use for the depth data.
         *  @param in_depth_store_op     Store operation to use for the depth data.
         *  @param in_stencil_load_op    Load operation to use for the stencil data.
         *  @param in_stencil_store_op   Store operation to use for the stencil data.
         *  @param in_initial_layout     Initial layout to use for the color attachment.
         *  @param in_final_layout       Final layout to use for the color attachment.
         *  @param in_may_alias          true if the memory backing of the image that will be attached may
         *                               overlap with the backing of another attachment within the same
         *                               subpass.
         *  @param out_attachment_id_ptr Deref will be set to a unique ID assigned to the new color attachment.
         *                               Must not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_depth_stencil_attachment(VkFormat                in_format,
                                          VkSampleCountFlags      in_sample_count,
                                          VkAttachmentLoadOp      in_depth_load_op,
                                          VkAttachmentStoreOp     in_depth_store_op,
                                          VkAttachmentLoadOp      in_stencil_load_op,
                                          VkAttachmentStoreOp     in_stencil_store_op,
                                          VkImageLayout           in_initial_layout,
                                          VkImageLayout           in_final_layout,
                                          bool                    in_may_alias,
                                          RenderPassAttachmentID* out_attachment_id_ptr);

        /** Adds a new subpass to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param in_fragment_shader_entrypoint        Shader module stage entrypoint descriptor for the fragment stage.
         *                                              Please pass an instance created by a dummy constructor if the stage
         *                                              is irrelevant.
         *  @param in_geometry_shader_entrypoint        Shader module stage entrypoint descriptor for the geometry stage.
         *                                              Please pass an instance created by a dummy constructor if the stage
         *                                              is irrelevant.
         *  @param in_tess_control_shader_entrypoint    Shader module stage entrypoint descriptor for the tessellation control stage.
         *                                              Please pass an instance created by a dummy constructor if the stage
         *                                              is irrelevant.
         *  @param in_tess_evaluation_shader_entrypoint Shader module stage entrypoint descriptor for the tessellation
         *                                              evaluation stage. Please pass an instance created by a dummy constructor
         *                                              if the stage is irrelevant.
         *  @param in_vertex_shader_entrypoint          Shader module stage entrypoint descriptor for the vertex stage.
         *                                              Must NOT be a dummy instance.
         *  @param out_subpass_id_ptr                   Deref will be set to a unique ID of the created subpass.
         *                                              Must not be nullptr.
         *  @param in_opt_pipeline_id                   (optional) ID of a graphics pipeline to use for the graphics pipeline.
         *                                              If not specified, a new pipeline wil be created from scratch.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_subpass(const ShaderModuleStageEntryPoint& in_fragment_shader_entrypoint,
                         const ShaderModuleStageEntryPoint& in_geometry_shader_entrypoint,
                         const ShaderModuleStageEntryPoint& in_tess_control_shader_entrypoint,
                         const ShaderModuleStageEntryPoint& in_tess_evaluation_shader_entrypoint,
                         const ShaderModuleStageEntryPoint& in_vertex_shader_entrypoint,
                         SubPassID*                         out_subpass_id_ptr,
                         const Anvil::PipelineID            in_opt_pipeline_id = UINT32_MAX);

        /** Adds a new color attachment to the RenderPass instance's specified subpass.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param in_subpass_id                ID of the render-pass subpass to update.
         *  @param in_layout                    Layout to use for the attachment when executing the subpass.
         *                                      Driver takes care of transforming the attachment to the requested layout
         *                                      before subpass commands starts executing.
         *  @param in_attachment_id             ID of the render-pass attachment the new sub-pass color attachment
         *                                      should refer to.
         *  @param in_location                  Location, under which the specified attachment should be accessible to
         *                                      the fragment shader.
         *  @param in_attachment_resolve_id_ptr Must be nullptr if the new color attachment should be single-sample.
         *                                      For multi-sample, this argument can optionally point to a render-pass
         *                                      attachment descriptor, to which the MS attachment should be resolved to.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_subpass_color_attachment(SubPassID                     in_subpass_id,
                                          VkImageLayout                 in_layout,
                                          RenderPassAttachmentID        in_attachment_id,
                                          uint32_t                      in_location,
                                          const RenderPassAttachmentID* in_opt_attachment_resolve_id_ptr);

        /** Configures the depth+stencil attachment the subpass should use.
         *
         *  Note that only up to one depth/stencil attachment may be added for each subpass.
         *  Any attempt to add more such attachments will results in an assertion failure.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param in_subpass_id                ID of the subpass to update the depth+stencil attachment for.
         *                                      The subpass must have been earlier created with an add_subpass() call.
         *  @param in_attachment_id             ID of the render-pass attachment the depth-stencil attachment should refer to.
         *  @param in_layout                    Layout to use for the attachment when executing the subpass.
         *                                      Driver takes care of transforming the attachment to the requested layout
         *                                      before subpass commands starts executing.
         *
         *  @return true if the function executed successfully, false otherwise.
         *
         */
        bool add_subpass_depth_stencil_attachment(SubPassID                     in_subpass_id,
                                                  RenderPassAttachmentID        in_attachment_id,
                                                  VkImageLayout                 in_layout);

        /** Adds a new input attachment to the RenderPass instance's specified subpass.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param in_subpass_id       ID of the render-pass subpass to update.
         *  @param in_layout           Layout to use for the attachment when executing the subpass.
         *                             Driver takes care of transforming the attachment to the requested layout
         *                             before subpass commands starts executing.
         *  @param in_attachment_id    ID of the render-pass attachment the new sub-pass input attachment
         *                             should refer to.
         *  @param in_attachment_index The "input attachment index", under which the specified attachment should
         *                             be accessible to the fragment shader. Do not forget the image view also
         *                             needs to be bound to the descriptor set for the input attachment to work!
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_subpass_input_attachment(SubPassID              in_subpass_id,
                                          VkImageLayout          in_layout,
                                          RenderPassAttachmentID in_attachment_id,
                                          uint32_t               in_attachment_index);

        /** Adds a new external->subpass dependency to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param in_destination_subpass_id  ID of the destination subpass. The subpass must have been
         *                                    created earlier with an add_subpass() call.
         *  @param in_source_stage_mask       Source pipeline stage mask.
         *  @param in_destination_stage_mask  Destination pipeline stage mask.
         *  @param in_source_access_mask      Source access mask.
         *  @param in_destination_access_mask Destination access mask.
         *  @param in_by_region               true if a "by-region" dependency is requested; false otherwise.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         *
         **/
        bool add_external_to_subpass_dependency(SubPassID            in_destination_subpass_id,
                                                VkPipelineStageFlags in_source_stage_mask,
                                                VkPipelineStageFlags in_destination_stage_mask,
                                                VkAccessFlags        in_source_access_mask,
                                                VkAccessFlags        in_destination_access_mask,
                                                bool                 in_by_region);

        /** Adds a new self-subpass dependency to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param in_destination_subpass_id  ID of the subpass to create the dep for. The subpass must have been
         *                                    created earlier with an add_subpass() call.
         *  @param in_source_stage_mask       Source pipeline stage mask.
         *  @param in_destination_stage_mask  Destination pipeline stage mask.
         *  @param in_source_access_mask      Source access mask.
         *  @param in_destination_access_mask Destination access mask.
         *  @param in_by_region               true if a "by-region" dependency is requested; false otherwise.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         *
         **/
        bool add_self_subpass_dependency(SubPassID            in_destination_subpass_id,
                                         VkPipelineStageFlags in_source_stage_mask,
                                         VkPipelineStageFlags in_destination_stage_mask,
                                         VkAccessFlags        in_source_access_mask,
                                         VkAccessFlags        in_destination_access_mask,
                                         bool                 in_by_region);

        /** Adds a new subpass->external dependency to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param in_source_subpass_id       ID of the source subpass. The subpass must have been
         *                                    created earlier with an add_subpass() call.
         *  @param in_source_stage_mask       Source pipeline stage mask.
         *  @param in_destination_stage_mask  Destination pipeline stage mask.
         *  @param in_source_access_mask      Source access mask.
         *  @param in_destination_access_mask Destination access mask.
         *  @param in_by_region               true if a "by-region" dependency is requested; false otherwise.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         **/
        bool add_subpass_to_external_dependency(SubPassID            in_source_subpass_id,
                                                VkPipelineStageFlags in_source_stage_mask,
                                                VkPipelineStageFlags in_destination_stage_mask,
                                                VkAccessFlags        in_source_access_mask,
                                                VkAccessFlags        in_destination_access_mask,
                                                bool                 in_by_region);

        /** Adds a new subpass->subpass dependency to the internal data model.
         *
         *  This function does NOT re-create the internal VkRenderPass instance. Instead,
         *  it marks the RenderPass as dirty, which will cause the object to be re-created
         *  at next bake() or get_render_pass() request.
         *
         *  @param in_source_subpass_id       ID of the source subpass. The subpass must have been
         *                                    created earlier with an add_subpass() call.
         *  @param in_destination_subpass_id  ID of the source subpass. The subpass must have been
         *                                    created earlier with an add_subpass() call.
         *  @param in_source_stage_mask       Source pipeline stage mask.
         *  @param in_destination_stage_mask  Destination pipeline stage mask.
         *  @param in_source_access_mask      Source access mask.
         *  @param in_destination_access_mask Destination access mask.
         *  @param in_by_region               true if a "by-region" dependency is requested; false otherwise.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         **/
        bool add_subpass_to_subpass_dependency(SubPassID            in_source_subpass_id,
                                               SubPassID            in_destination_subpass_id,
                                               VkPipelineStageFlags in_source_stage_mask,
                                               VkPipelineStageFlags in_destination_stage_mask,
                                               VkAccessFlags        in_source_access_mask,
                                               VkAccessFlags        in_destination_access_mask,
                                               bool                 in_by_region);

        /** Re-creates the internal VkRenderPass object.
         *
         *  This function should be considered expensive.
         *
         *  @return true if successful, false otherwise.
         **/
        bool bake();

        /** Tells what type an attachment with user-specified ID has.
         *
         *  @return true if successful, false otherwise
         */
        bool get_attachment_type(RenderPassAttachmentID in_attachment_id,
                                 AttachmentType*        out_attachment_type_ptr) const;

        /** Retrieves properties of the render pass color attachment with the user-specified ID
         *
         *  @param in_attachment_id           ID of the attachment to retrieve properties of.
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
        bool get_color_attachment_properties(RenderPassAttachmentID in_attachment_id,
                                             VkSampleCountFlagBits* out_opt_sample_count_ptr   = nullptr,
                                             VkAttachmentLoadOp*    out_opt_load_op_ptr        = nullptr,
                                             VkAttachmentStoreOp*   out_opt_store_op_ptr       = nullptr,
                                             VkImageLayout*         out_opt_initial_layout_ptr = nullptr,
                                             VkImageLayout*         out_opt_final_layout_ptr   = nullptr,
                                             bool*                  out_opt_may_alias_ptr      = nullptr) const;

        /** Retrieves properties of a dependency at user-specified index.
         *
         *  @param in_n_dependency                 Index of the dependency to retrieve properties of.
         *  @param out_destination_subpass_id_ptr  Deref will be set to the ID of the dependency's destination,
         *                                         or to UINT32_MAX, if the destination of the dependency is external.
         *                                         Must not be null.
         *  @param out_source_subpass_id_ptr       Deref will be set to the ID of the dependency's source,
         *                                         or to UINT32_MAX, if the source of the dependency is external.
         *                                         Must not be null.
         *  @param out_destination_stage_mask_ptr  Deref will be set to the destination stage mask set for the dependency.
         *                                         Must not be null.
         *  @param out_source_stage_mask_ptr       Deref will be set to the source stage mask set for the dependency.
         *                                         Must not be null.
         *  @param out_destination_access_mask_ptr Deref will be set to the destination access mask set for the dependency.
         *                                         Must not be null.
         *  @param out_source_access_mask_ptr      Deref will be set to the source access mask set for the dependency.
         *                                         Must not be null.
         *  @param out_by_region_ptr               Deref will be set to true, if the dependency is a by-region dependency.
         *                                         If it is not, deref will be set to false. Must not be null.
         *
         *  @return true if successful, false otherwise
         */
        bool get_dependency_properties(uint32_t              in_n_dependency,
                                       SubPassID*            out_destination_subpass_id_ptr,
                                       SubPassID*            out_source_subpass_id_ptr,
                                       VkPipelineStageFlags* out_destination_stage_mask_ptr,
                                       VkPipelineStageFlags* out_source_stage_mask_ptr,
                                       VkAccessFlags*        out_destination_access_mask_ptr,
                                       VkAccessFlags*        out_source_access_mask_ptr,
                                       bool*                 out_by_region_ptr) const;

        /** Retrieves properties of the render pass color attachment with the user-specified ID
         *
         *  @param in_attachment_id             ID of the attachment to retrieve properties of.
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
        bool get_depth_stencil_attachment_properties(RenderPassAttachmentID in_attachment_id,
                                                     VkAttachmentLoadOp*    out_opt_depth_load_op_ptr    = nullptr,
                                                     VkAttachmentStoreOp*   out_opt_depth_store_op_ptr   = nullptr,
                                                     VkAttachmentLoadOp*    out_opt_stencil_load_op_ptr  = nullptr,
                                                     VkAttachmentStoreOp*   out_opt_stencil_store_op_ptr = nullptr,
                                                     VkImageLayout*         out_opt_initial_layout_ptr   = nullptr,
                                                     VkImageLayout*         out_opt_final_layout_ptr     = nullptr,
                                                     bool*                  out_opt_may_alias_ptr        = nullptr) const;

        /** Returns the number of added attachments */
        uint32_t get_n_attachments() const
        {
            return static_cast<uint32_t>(m_attachments.size() );
        }

        /** Returns the number of added dependencies */
        uint32_t get_n_dependencies() const
        {
            return static_cast<uint32_t>(m_subpass_dependencies.size() );
        }

        /** Returns the number of added subpasses */
        uint32_t get_n_subpasses() const
        {
            return static_cast<uint32_t>(m_subpasses.size() );
        }

        /** Bakes the VkRenderPass object if the RenderPass instance is marked as dirty, and returns it.
         *
         *  @return The requested object.
         **/
        VkRenderPass get_render_pass(bool allow_rebaking = true);

        /** Retrieves subpass attachment properties, as specified at creation time. 
         *
         *  Triggers baking process if the renderpass is marked as dirty.
         *
         *  @param in_subpass_id                    ID of the subpass to use for the query.
         *  @param in_attachment_type               Type of the attachment to use for the query. Must not be ATTACHMENT_TYPE_PRESERVE.
         *  @param out_renderpass_attachment_id_ptr Deref will be set to the ID of the renderpass attachment this subpass
         *                                          attachment uses.
         *  @param out_layout_ptr                   Deref will be set to the image layout assigned to the attachment for the specific
         *                                          subpass. Must be NULL if @param in_attachment_type is ATTACHMENT_TYPE_PRESERVE.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_subpass_attachment_properties(SubPassID               in_subpass_id,
                                               AttachmentType          in_attachment_type,
                                               uint32_t                in_n_subpass_attachment,
                                               RenderPassAttachmentID* out_renderpass_attachment_id_ptr,
                                               VkImageLayout*          out_layout_ptr);

        /** Returns the graphics pipeline ID, associated with the specified subpass.
         *
         *  @param in_subpass_id                As per description.
         *  @param out_graphics_pipeline_id_ptr Deref will be set to the aforementioned ID. Must not be nullptr.
         *                                      Will only be touched if the function returns true.
         *
         *  @return true if the function was successful, false otherwise.
         **/
        bool get_subpass_graphics_pipeline_id(SubPassID           in_subpass_id,
                                              GraphicsPipelineID* out_graphics_pipeline_id_ptr) const;

        /** Returns the number of attachments of user-specified type, defined for the specified subpass.
         *
         *  This function may trigger renderpass bake process, if the renderpass is marked as dirty
         *  at the query time, and @param in_attachment_type is set to ATTACHMENT_TYPE_PRESERVE.
         *
         *  @param in_subpass_id         As per description.
         *  @param in_attachment_type    Type of the attachment to use for the query.
         *  @param out_n_attachments_ptr Deref will be set to the value above. Must not be nullptr.
         *                               Will only be touched if the function returns true.
         *
         *  @return true if the function was successful, false otherwise.
         **/
        bool get_subpass_n_attachments(SubPassID      in_subpass_id,
                                       AttachmentType in_attachment_type,
                                       uint32_t*      out_n_attachments_ptr);

        /** Returns the Swapchain instance, associated with the RenderPass wrapper instance at creation time. */
        std::shared_ptr<Anvil::Swapchain> get_swapchain() const
        {
            return m_swapchain_ptr;
        }

        /* Releases the graphics pipeline, as used by the specified subpass at the time of the call,
         * and assigns the user-specified one.
         *
         *  @param in_subpass_id               ID of the subpass, whose graphics pipeline should be changed.
         *  @param in_new_graphics_pipeline_id ID of the graphics pipeline to start using.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_subpass_graphics_pipeline_id(SubPassID          in_subpass_id,
                                              GraphicsPipelineID in_new_graphics_pipeline_id);

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
            VkAttachmentLoadOp    stencil_load_op;
            VkAttachmentStoreOp   stencil_store_op;
            AttachmentType        type;

            VkSampleCountFlagsVariable(sample_count);

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
            RenderPassAttachment(VkFormat            in_format,
                                 VkSampleCountFlags  in_sample_count,
                                 VkAttachmentLoadOp  in_load_op,
                                 VkAttachmentStoreOp in_store_op,
                                 VkImageLayout       in_initial_layout,
                                 VkImageLayout       in_final_layout,
                                 bool                in_may_alias,
                                 uint32_t            in_index)
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
                type                 = ATTACHMENT_TYPE_COLOR;
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
            RenderPassAttachment(VkFormat            in_format,
                                 VkSampleCountFlags  in_sample_count,
                                 VkAttachmentLoadOp  in_depth_load_op,
                                 VkAttachmentStoreOp in_depth_store_op,
                                 VkAttachmentLoadOp  in_stencil_load_op,
                                 VkAttachmentStoreOp in_stencil_store_op,
                                 VkImageLayout       in_initial_layout,
                                 VkImageLayout       in_final_layout,
                                 bool                in_may_alias,
                                 uint32_t            in_index)
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
                type                 = ATTACHMENT_TYPE_DEPTH_STENCIL;
            }

            /** Dummy constructor. This should only be used by STL containers. */
            RenderPassAttachment()
            {
                color_depth_load_op  = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
                color_depth_store_op = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
                final_layout         = VK_IMAGE_LAYOUT_MAX_ENUM;
                format               = VK_FORMAT_MAX_ENUM;
                index                = UINT32_MAX;
                initial_layout       = VK_IMAGE_LAYOUT_MAX_ENUM;
                may_alias            = false;
                sample_count         = static_cast<VkSampleCountFlagBits>(0);
                stencil_load_op      = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
                stencil_store_op     = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
                type                 = ATTACHMENT_TYPE_UNKNOWN;
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
                highest_subpass_index  = UINT32_MAX;
                layout                 = VK_IMAGE_LAYOUT_MAX_ENUM;
                lowest_subpass_index   = UINT32_MAX;
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
                highest_subpass_index  = UINT32_MAX;
                layout                 = in_layout;
                lowest_subpass_index   = UINT32_MAX;
                resolve_attachment_ptr = in_opt_resolve_attachment_ptr;
            }
        } SubPassAttachment;

        typedef std::map<uint32_t, SubPassAttachment>          LocationToSubPassAttachmentMap;
        typedef LocationToSubPassAttachmentMap::const_iterator LocationToSubPassAttachmentMapConstIterator;
        typedef std::vector<SubPassAttachment>                 SubPassAttachmentVector;
        
        /** Holds properties of a single sub-pass */
        typedef struct SubPass
        {
            LocationToSubPassAttachmentMap   color_attachments_map;
            SubPassAttachment                depth_stencil_attachment;
            std::weak_ptr<Anvil::BaseDevice> device_ptr;
            uint32_t                         index;
            LocationToSubPassAttachmentMap   input_attachments_map;
            GraphicsPipelineID               pipeline_id;
            SubPassAttachmentVector          preserved_attachments;
            LocationToSubPassAttachmentMap   resolved_attachments_map;

            SubPassAttachment* get_color_attachment_at_index(uint32_t in_index)
            {
                return get_attachment_at_index(color_attachments_map,
                                               in_index);
            }

            SubPassAttachment* get_input_attachment_at_index(uint32_t in_index)
            {
                return get_attachment_at_index(input_attachments_map,
                                               in_index);
            }

            SubPassAttachment* get_resolved_attachment_at_index(uint32_t in_index)
            {
                return get_attachment_at_index(resolved_attachments_map,
                                               in_index);
            }

            /** Dummy constructor. This should only be used by STL containers */
            SubPass()
            {
                index       = UINT32_MAX;
                pipeline_id = UINT32_MAX;
            }

            /** Constructor.
             *
             *  @param in_index       Index of the sub-pass
             *  @param in_pipeline_id ID of the graphics pipeline which is associated with the subpass.
             *
             **/
            SubPass(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                    uint32_t                         in_index,
                    GraphicsPipelineID               in_pipeline_id)
            {
                device_ptr  = in_device_ptr;
                index       = in_index;
                pipeline_id = in_pipeline_id;
            }

            /* Destructor */
            ~SubPass();

            private:
                /** Returns a pointer to the SubPassAttachment instance, assigned to the index specified
                 *  under @param index in @param in_map.
                 **/
                SubPassAttachment* get_attachment_at_index(LocationToSubPassAttachmentMap& in_map,
                                                           uint32_t                        in_index)
                {
                    uint32_t           current_index = 0;
                    SubPassAttachment* result_ptr    = nullptr;

                    for (auto attachment_iterator  = in_map.begin();
                              attachment_iterator != in_map.end();
                            ++attachment_iterator, ++current_index)
                    {
                        if (current_index == in_index)
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

        typedef std::vector<SubPass*>     SubPasses;
        typedef SubPasses::const_iterator SubPassesConstIterator;

        /** Holds properties of a single subpass<->subpass dependency. */
        typedef struct SubPassDependency
        {
            VkAccessFlagsVariable       (destination_access_mask);
            VkPipelineStageFlagsVariable(destination_stage_mask);
            VkAccessFlagsVariable       (source_access_mask);
            VkPipelineStageFlagsVariable(source_stage_mask);

            bool           by_region;
            const SubPass* destination_subpass_ptr;
            const SubPass* source_subpass_ptr;

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

        /* Constructor. Please see create() for specification */
        RenderPass(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
                   std::shared_ptr<Anvil::Swapchain> in_opt_swapchain_ptr);

        RenderPass& operator=(const RenderPass&);
        RenderPass           (const RenderPass&);

        bool add_dependency        (SubPass*               in_destination_subpass_ptr,
                                    SubPass*               in_source_subpass_ptr,
                                    VkPipelineStageFlags   in_source_stage_mask,
                                    VkPipelineStageFlags   in_destination_stage_mask,
                                    VkAccessFlags          in_source_access_mask,
                                    VkAccessFlags          in_destination_access_mask,
                                    bool                   in_by_region);
        bool add_subpass_attachment(SubPassID              in_subpass_id,
                                    bool                   in_is_color_attachment,
                                    VkImageLayout          in_input_layout,
                                    RenderPassAttachmentID in_attachment_id,
                                    uint32_t               in_attachment_location,
                                    bool                   in_should_resolve,
                                    RenderPassAttachmentID in_resolve_attachment_id);

        VkAttachmentReference get_attachment_reference_from_renderpass_attachment(const RenderPassAttachment&                        in_renderpass_attachment)                const;
        VkAttachmentReference get_attachment_reference_from_subpass_attachment   (const SubPassAttachment&                           in_subpass_attachment)                   const;
        VkAttachmentReference get_attachment_reference_for_resolve_attachment    (const SubPassesConstIterator&                      in_subpass_iterator,
                                                                                  const LocationToSubPassAttachmentMapConstIterator& in_location_to_subpass_att_map_iterator) const;
        void                  update_preserved_attachments                       ();

        /* Private members */
        RenderPassAttachments            m_attachments;
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        bool                             m_dirty;
        VkRenderPass                     m_render_pass;
        SubPasses                        m_subpasses;
        SubPassDependencies              m_subpass_dependencies;
        std::shared_ptr<Swapchain>       m_swapchain_ptr;
    };
};

#endif /* WRAPPERS_RENDER_PASS_H */