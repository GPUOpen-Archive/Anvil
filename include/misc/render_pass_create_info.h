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
#ifndef MISC_RENDERPASS_CREATE_INFO_H
#define MISC_RENDERPASS_CREATE_INFO_H

#include "misc/types.h"

namespace Anvil
{
    class RenderPassCreateInfo
    {
    public:
         RenderPassCreateInfo(const Anvil::BaseDevice* in_device_ptr);
        ~RenderPassCreateInfo();

        /** Adds a new render-pass color attachment to the internal data model.
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
        bool add_color_attachment(Anvil::Format              in_format,
                                  Anvil::SampleCountFlagBits in_sample_count,
                                  Anvil::AttachmentLoadOp    in_load_op,
                                  Anvil::AttachmentStoreOp   in_store_op,
                                  Anvil::ImageLayout         in_initial_layout,
                                  Anvil::ImageLayout         in_final_layout,
                                  bool                       in_may_alias,
                                  RenderPassAttachmentID*    out_attachment_id_ptr);

        /** Adds a new render-pass depth/stencil attachment to the internal data model.
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
        bool add_depth_stencil_attachment(Anvil::Format              in_format,
                                          Anvil::SampleCountFlagBits in_sample_count,
                                          Anvil::AttachmentLoadOp    in_depth_load_op,
                                          Anvil::AttachmentStoreOp   in_depth_store_op,
                                          Anvil::AttachmentLoadOp    in_stencil_load_op,
                                          Anvil::AttachmentStoreOp   in_stencil_store_op,
                                          Anvil::ImageLayout         in_initial_layout,
                                          Anvil::ImageLayout         in_final_layout,
                                          bool                       in_may_alias,
                                          RenderPassAttachmentID*    out_attachment_id_ptr);

        /** Adds a new subpass to the internal data model.
         *
         *  @param out_subpass_id_ptr Deref will be set to a unique ID of the created subpass.
         *                            Must not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_subpass(SubPassID* out_subpass_id_ptr);

        /** Adds a new color attachment to the RenderPass instance's specified subpass.
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
                                          Anvil::ImageLayout            in_layout,
                                          RenderPassAttachmentID        in_attachment_id,
                                          uint32_t                      in_location,
                                          const RenderPassAttachmentID* in_opt_attachment_resolve_id_ptr = nullptr);

        /** Configures the depth+stencil attachment the subpass should use.
         *
         *  Note that only up to one depth/stencil attachment may be added for each subpass.
         *  Any attempt to add more such attachments will result in an assertion failure.
         *
         *  If VK_KHR_depth_stencil_resolve is supported, DS resolve attachment can be configured by calling the func prototype
         *  which includes @param in_attachment_resolve_id_ptr, @param in_depth_resolve_mode_ptr and @param in_stencil_resolve_mode_ptr
         *  parameters. Otherwise, please use the prototype w/o the arguments.
         *
         *  @param in_subpass_id                ID of the subpass to update the depth+stencil attachment for.
         *                                      The subpass must have been earlier created with an add_subpass() call.
         *  @param in_layout                    Layout to use for the attachment when executing the subpass.
         *                                      Driver takes care of transforming the attachment to the requested layout
         *                                      before subpass commands starts executing.
         *  @param in_attachment_id             ID of the render-pass attachment the depth-stencil attachment should refer to.
         *  @param in_attachment_resolve_id_ptr Pointer to an ID of the renderpass attachment, to which the MS attachment should be resolved to.
         *                                      Requires VK_KHR_depth_stencil_resolve support. If not nullptr, @param in_depth_resolve_mode_ptr
         *                                      and @param in_stencil_resolve_mode_ptr must also not be nullptr.
         *  @param in_depth_resolve_mode_ptr    Pointer to resolve mode that should be used for the depth aspect.
         *                                      Requires VK_KHR_depth_stencil_resolve support. If not nullptr, @param in_opt_attachment_resolve_id_ptr
         *                                      and @param in_opt_stencil_resolve_mode_ptr must also not be nullptr.
         *  @param in_stencil_resolve_mode_ptr  Pointer to resolve mode that should be used for the stencil aspect.
         *                                      Requires VK_KHR_depth_stencil_resolve support. If not nullptr, @param in_opt_attachment_resolve_id_ptr
         *                                      and @param in_opt_depth_resolve_mode_ptr must also not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         *
         */
        bool add_subpass_depth_stencil_attachment(SubPassID                         in_subpass_id,
                                                  Anvil::ImageLayout                in_layout,
                                                  RenderPassAttachmentID            in_attachment_id,
                                                  const RenderPassAttachmentID*     in_attachment_resolve_id_ptr,
                                                  const Anvil::ResolveModeFlagBits* in_depth_resolve_mode_ptr,
                                                  const Anvil::ResolveModeFlagBits* in_stencil_resolve_mode_ptr);

        bool add_subpass_depth_stencil_attachment(SubPassID              in_subpass_id,
                                                  Anvil::ImageLayout     in_layout,
                                                  RenderPassAttachmentID in_attachment_id)
        {
            return add_subpass_depth_stencil_attachment(in_subpass_id,
                                                        in_layout,
                                                        in_attachment_id,
                                                        nullptr,  /* in_attachment_resolve_id_ptr */
                                                        nullptr,  /* in_depth_resolve_mode_ptr    */
                                                        nullptr); /* in_stencil_resolve_mode_ptr  */
        }

        /** Adds a new input attachment to the RenderPass instance's specified subpass.
         *
         *  @param in_subpass_id           ID of the render-pass subpass to update.
         *  @param in_layout               Layout to use for the attachment when executing the subpass.
         *                                 Driver takes care of transforming the attachment to the requested layout
         *                                 before subpass commands starts executing.
         *  @param in_attachment_id        ID of the render-pass attachment the new sub-pass input attachment
         *                                 should refer to.
         *  @param in_attachment_index     The "input attachment index", under which the specified attachment should
         *                                 be accessible to the fragment shader. Do not forget the image view also
         *                                 needs to be bound to the descriptor set for the input attachment to work!
         *  @param in_opt_aspects_accessed If set to a value != ImageAspectFlagBits::UNKNOWN and KHR_maintenance2 is supported,
         *                                 the additional information will be passed down to the driver which may result
         *                                 in improved performance.

         *  @return true if the function executed successfully, false otherwise.
         **/
        bool add_subpass_input_attachment(SubPassID                      in_subpass_id,
                                          Anvil::ImageLayout             in_layout,
                                          RenderPassAttachmentID         in_attachment_id,
                                          uint32_t                       in_attachment_index,
                                          const Anvil::ImageAspectFlags& in_opt_aspects_accessed = Anvil::ImageAspectFlagBits::NONE);

        /** Adds a new external->subpass dependency to the internal data model.
         *
         *  NOTE: @param in_dependency_flags must NOT include Anvil::DependencyFlagBits::VIEW_LOCAL_BIT.
         *
         *  @param in_destination_subpass_id  ID of the destination subpass. The subpass must have been
         *                                    created earlier with an add_subpass() call.
         *  @param in_source_stage_mask       Source pipeline stage mask.
         *  @param in_destination_stage_mask  Destination pipeline stage mask.
         *  @param in_source_access_mask      Source access mask.
         *  @param in_destination_access_mask Destination access mask.
         *  @param in_dependency_flags        Flags to use for the dependency.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         *
         **/
        bool add_external_to_subpass_dependency(SubPassID                 in_destination_subpass_id,
                                                Anvil::PipelineStageFlags in_source_stage_mask,
                                                Anvil::PipelineStageFlags in_destination_stage_mask,
                                                Anvil::AccessFlags        in_source_access_mask,
                                                Anvil::AccessFlags        in_destination_access_mask,
                                                Anvil::DependencyFlags    in_dependency_flags);

        /** Adds a new self-subpass dependency to the internal data model.
         *
         *  NOTE: If @param in_dependency_flags includes Anvil::DependencyFlagBits::VIEW_LOCAL_BIT, you must
         *        also call set_dependency_view_local_properties() to define the src<->dst view index relationship
         *        for the subpass.
         *
         *  @param in_destination_subpass_id  ID of the subpass to create the dep for. The subpass must have been
         *                                    created earlier with an add_subpass() call.
         *  @param in_source_stage_mask       Source pipeline stage mask.
         *  @param in_destination_stage_mask  Destination pipeline stage mask.
         *  @param in_source_access_mask      Source access mask.
         *  @param in_destination_access_mask Destination access mask.
         *  @param in_dependency_flags        Flags to use for the dependency.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         *
         **/
        bool add_self_subpass_dependency(SubPassID                 in_destination_subpass_id,
                                         Anvil::PipelineStageFlags in_source_stage_mask,
                                         Anvil::PipelineStageFlags in_destination_stage_mask,
                                         Anvil::AccessFlags        in_source_access_mask,
                                         Anvil::AccessFlags        in_destination_access_mask,
                                         Anvil::DependencyFlags    in_dependency_flags);

        /** Adds a new subpass->external dependency to the internal data model.
         *
         *  NOTE: @param in_dependency_flags must NOT include Anvil::DependencyFlagBits::VIEW_LOCAL_BIT.
         *
         *  @param in_source_subpass_id       ID of the source subpass. The subpass must have been
         *                                    created earlier with an add_subpass() call.
         *  @param in_source_stage_mask       Source pipeline stage mask.
         *  @param in_destination_stage_mask  Destination pipeline stage mask.
         *  @param in_source_access_mask      Source access mask.
         *  @param in_destination_access_mask Destination access mask.
         *  @param in_dependency_flags        Flags to use for the dependency.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         **/
        bool add_subpass_to_external_dependency(SubPassID                 in_source_subpass_id,
                                                Anvil::PipelineStageFlags in_source_stage_mask,
                                                Anvil::PipelineStageFlags in_destination_stage_mask,
                                                Anvil::AccessFlags        in_source_access_mask,
                                                Anvil::AccessFlags        in_destination_access_mask,
                                                Anvil::DependencyFlags    in_dependency_flags);

        /** Adds a new subpass->subpass dependency to the internal data model.
         *
         *  NOTE: If @param in_dependency_flags includes Anvil::DependencyFlagBits::VIEW_LOCAL_BIT, you must
         *        also call set_dependency_view_local_properties() to define the src<->dst view index relationship
         *        for the subpass.
         *
         *  @param in_source_subpass_id       ID of the source subpass. The subpass must have been
         *                                    created earlier with an add_subpass() call.
         *  @param in_destination_subpass_id  ID of the source subpass. The subpass must have been
         *                                    created earlier with an add_subpass() call.
         *  @param in_source_stage_mask       Source pipeline stage mask.
         *  @param in_destination_stage_mask  Destination pipeline stage mask.
         *  @param in_source_access_mask      Source access mask.
         *  @param in_destination_access_mask Destination access mask.
         *  @param in_dependency_flags        Flags to use for the dependency.
         *
         *  @return true if the dependency was added successfully; false otherwise.
         **/
        bool add_subpass_to_subpass_dependency(SubPassID                 in_source_subpass_id,
                                               SubPassID                 in_destination_subpass_id,
                                               Anvil::PipelineStageFlags in_source_stage_mask,
                                               Anvil::PipelineStageFlags in_destination_stage_mask,
                                               Anvil::AccessFlags        in_source_access_mask,
                                               Anvil::AccessFlags        in_destination_access_mask,
                                               Anvil::DependencyFlags    in_dependency_flags);

        /** Tells what type an attachment with user-specified ID has.
         *
         *  @return true if successful, false otherwise
         */
        bool get_attachment_type(RenderPassAttachmentID in_attachment_id,
                                 AttachmentType*        out_attachment_type_ptr) const;

        /** Retrieves properties of the render pass color attachment with the user-specified ID
         *
         *  @param in_attachment_id           ID of the attachment to retrieve properties of.
         *  @param out_opt_format_ptr         If not nullptr, deref will be set to the format, specified
         *                                    at attachment creation time. May be nullptr.
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
        bool get_color_attachment_properties(RenderPassAttachmentID      in_attachment_id,
                                             Anvil::Format*              out_opt_format_ptr         = nullptr,
                                             Anvil::SampleCountFlagBits* out_opt_sample_count_ptr   = nullptr,
                                             Anvil::AttachmentLoadOp*    out_opt_load_op_ptr        = nullptr,
                                             Anvil::AttachmentStoreOp*   out_opt_store_op_ptr       = nullptr,
                                             Anvil::ImageLayout*         out_opt_initial_layout_ptr = nullptr,
                                             Anvil::ImageLayout*         out_opt_final_layout_ptr   = nullptr,
                                             bool*                       out_opt_may_alias_ptr      = nullptr) const;

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
         *  @param out_flags_ptr                   Deref will be set to flags specified for the dependency. Must not be null.
         *
         *  @return true if successful, false otherwise
         */
        bool get_dependency_properties(uint32_t                   in_n_dependency,
                                       SubPassID*                 out_destination_subpass_id_ptr,
                                       SubPassID*                 out_source_subpass_id_ptr,
                                       Anvil::PipelineStageFlags* out_destination_stage_mask_ptr,
                                       Anvil::PipelineStageFlags* out_source_stage_mask_ptr,
                                       Anvil::AccessFlags*        out_destination_access_mask_ptr,
                                       Anvil::AccessFlags*        out_source_access_mask_ptr,
                                       DependencyFlags*           out_flags_ptr) const;

        /* Returns the view offset associated with a given dependency. For view-global dependencies, zero will be returned.
         *
         * This function should only be called if is_multiview_enabled() returns true.
         *
         * @param in_n_dependency     Index of the dependency to use for the query.
         * @param out_view_offset_ptr If the function returns true, deref will be set to the view offset associated with the dependency.
         *                            Must not be nullptr.
         *
         * @return true if successful, false otherwise.
         **/
        bool get_dependency_multiview_properties(uint32_t in_n_dependency,
                                                 int32_t* out_view_offset_ptr) const;

        /** Retrieves properties of the render pass color attachment with the user-specified ID
         *
         *  @param in_attachment_id             ID of the attachment to retrieve properties of.
         *  @param out_opt_format_ptr           If not nullptr, deref will be set to the format, specified
         *                                      at attachment creation time. May be nullptr.
         *  @param out_opt_sample_count_ptr     If not nullptr, deref will be set to the sample count, specified
         *                                      at attachment creation time. May be nullptr.
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
        bool get_depth_stencil_attachment_properties(RenderPassAttachmentID      in_attachment_id,
                                                     Anvil::Format*              out_opt_format_ptr                = nullptr,
                                                     Anvil::SampleCountFlagBits* out_opt_sample_count_ptr          = nullptr,
                                                     Anvil::AttachmentLoadOp*    out_opt_depth_load_op_ptr         = nullptr,
                                                     Anvil::AttachmentStoreOp*   out_opt_depth_store_op_ptr        = nullptr,
                                                     Anvil::AttachmentLoadOp*    out_opt_stencil_load_op_ptr       = nullptr,
                                                     Anvil::AttachmentStoreOp*   out_opt_stencil_store_op_ptr      = nullptr,
                                                     Anvil::ImageLayout*         out_opt_initial_layout_ptr        = nullptr,
                                                     Anvil::ImageLayout*         out_opt_final_layout_ptr          = nullptr,
                                                     bool*                       out_opt_may_alias_ptr             = nullptr) const;

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        /* Returns the largest location assigned to color attachments for the specified subpass.
         *
         * If no color attachments have been defined for the queried subpass, UINT32_MAX is returned.
         *
         * @param in_subpass_id ID of the subpass to use for the query.
         *
         * @return As per description.
         **/
        uint32_t get_max_color_location_used_by_subpass(const SubPassID& in_subpass_id) const;

        /* Returns the correlation masks specified for the renderpass.
         *
         * This function should only be called if is_multiview_enabled() returns true.
         *
         * @param out_n_correlation_masks_ptr   Deref will be set to the number of uint32s available for reading under *out_correlation_masks_ptr_ptr
         *                                      after the function leaves. Must not be nullptr.
         * @param out_correlation_masks_ptr_ptr Deref will be set to a pointer to an array of uint32s holding correlation masks. Must nto be nullptr.
         *
         **/
        void get_multiview_correlation_masks(uint32_t*        out_n_correlation_masks_ptr,
                                             const uint32_t** out_correlation_masks_ptr_ptr) const;

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

        /** Retrieves subpass attachment properties, as specified at creation time. 
         *
         *  Supports all attachment types except depth/stencil resolve attachment. You can retrieve details of the attachment
         *  by calling get_subpass_ds_resolve_attachment_properties() instead.
         *
         *  @param in_subpass_id                    ID of the subpass to use for the query.
         *  @param in_attachment_type               Type of the attachment to use for the query. Must not be ATTACHMENT_TYPE_PRESERVE.
         *  @param out_renderpass_attachment_id_ptr Deref will be set to the ID of the renderpass attachment this subpass
         *                                          attachment uses.
         *  @param out_layout_ptr                   Deref will be set to the image layout assigned to the attachment for the specific
         *                                          subpass. Must be NULL if @param in_attachment_type is ATTACHMENT_TYPE_PRESERVE.
         *  @param out_opt_aspects_accessed_ptr     Only relevant for input attachments and KHR_maintenance2 is supported.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_subpass_attachment_properties(SubPassID                   in_subpass_id,
                                               AttachmentType              in_attachment_type,
                                               uint32_t                    in_n_subpass_attachment,
                                               RenderPassAttachmentID*     out_renderpass_attachment_id_ptr,
                                               Anvil::ImageLayout*         out_layout_ptr,
                                               Anvil::ImageAspectFlags*    out_opt_aspects_accessed_ptr      = nullptr,
                                               RenderPassAttachmentID*     out_opt_attachment_resolve_id_ptr = nullptr,
                                               uint32_t*                   out_opt_location_ptr              = nullptr) const;

        /* TODO.
         *
         * Returns false if no resolve attachment has been specified for d/ds/s attachment.
         */
        bool get_subpass_ds_resolve_attachment_properties(SubPassID                   in_subpass_id,
                                                          RenderPassAttachmentID*     out_opt_renderpass_attachment_id_ptr = nullptr,
                                                          Anvil::ImageLayout*         out_opt_layout_ptr                   = nullptr,
                                                          Anvil::ResolveModeFlagBits* out_opt_depth_resolve_mode_ptr       = nullptr,
                                                          Anvil::ResolveModeFlagBits* out_opt_stencil_resolve_mode_ptr     = nullptr) const;

        /* Returns highest location used by subpass attachments.
         *
         * @param in_subpass_id ID of the subpass to issue the query against.
         * @param out_result_ptr If function returns true, deref will be set to the highest location specified when adding attachments
         *                       to this subpass.
         *
         * @return true if successful, false otherwise. The latter will be returned if no attachments have been added to the subpass, prior
         *         to making this call.
         */
        bool get_subpass_highest_location(SubPassID in_subpass_id,
                                          uint32_t* out_result_ptr) const;

        /** Returns the number of attachments of user-specified type, defined for the specified subpass.
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
                                       uint32_t*      out_n_attachments_ptr) const;

        /* Returns the view mask associated with a given subpass.
         *
         * This function should only be called if is_multiview_enabled() returns true.
         *
         * @param in_subpass_id     ID of the subpass to use for the query.
         * @param out_view_mask_ptr If the function returns true, deref will be set to the view mask associated with the subpass.
         *                          Must not be nullptr.
         *
         * @return true if successful, false otherwise.
         **/
        bool get_subpass_view_mask(SubPassID in_subpass_id,
                                   uint32_t* out_view_mask_ptr) const;

        /* Tells whether the renderpass uses multiview functionality. */
        bool is_multiview_enabled() const
        {
            return m_multiview_enabled;
        }

        /* Specifies correlation mask(s) for the renderpass.
         *
         *  If multiview is not already enabled for the renderpass, calling this function enables the functionality.
         *
         *  Requires VK_KHR_multiview.
         *
         *  @param in_n_correlation_masks   Number of masks available for reading under @param in_correlation_masks_ptr.
         *                                  Must not be 0.
         *  @param in_correlation_masks_ptr Correlation masks to use for the renderpass. Please read VK_KHR_multiview spec
         *                                  for more details.
         */
        void set_correlation_masks(const uint32_t& in_n_correlation_masks,
                                   const uint32_t* in_correlation_masks_ptr);

        /** Specifies a view offset to use for a view-local dependency.
         *
         *  If multiview is not already enabled for the renderpass, calling this function enables the functionality.
         *
         *  Requires VK_KHR_multiview.
         *
         *  @param in_subpass_id  ID of the subpass to use.
         *  @param in_view_offset View offset to use for a view-local dependency defined at index @param in_n_dependency. Must not be zero
         *                        for self-dependencies.
         *
         *  @return true if successful, false otherwise.
         */
        bool set_dependency_view_local_properties(const uint32_t& in_n_dependency,
                                                  const int32_t&  in_view_offset);

        void set_device_ptr(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        /** Specifies the view mask for a subpass associated with ID @param in_subpass_id.
         *
         *  If multiview is not already enabled for the renderpass, calling this function enables the functionality.
         *  The implication is the application needs to set the view mask for all subpasses defined for the renderpass,
         *  before the create info structure can be passed over to Anvil::RenderPass.
         *
         *  Requires VK_KHR_multiview.
         *
         *  @param in_subpass_id ID of the subpass to use.
         *  @param in_view_mask  View mask to use for the subpass. Please consult VK_KHR_multiview spec for more details.
         *
         *  @return true if successful, false otherwise.
         */
        bool set_subpass_view_mask(SubPassID       in_subpass_id,
                                   const uint32_t& in_view_mask);

    private:
        /* Private type definitions */

        /** Holds properties of a single render-pass attachment. **/
        typedef struct RenderPassAttachment
        {
            Anvil::AttachmentLoadOp    color_depth_load_op;
            Anvil::AttachmentStoreOp   color_depth_store_op;
            Anvil::ImageLayout         final_layout;
            Anvil::Format              format;
            uint32_t                   index;
            Anvil::ImageLayout         initial_layout;
            bool                       may_alias;
            Anvil::SampleCountFlagBits sample_count;
            Anvil::AttachmentLoadOp    stencil_load_op;
            Anvil::AttachmentStoreOp   stencil_store_op;
            AttachmentType             type;

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
            RenderPassAttachment(Anvil::Format              in_format,
                                 Anvil::SampleCountFlagBits in_sample_count,
                                 Anvil::AttachmentLoadOp    in_load_op,
                                 Anvil::AttachmentStoreOp   in_store_op,
                                 Anvil::ImageLayout         in_initial_layout,
                                 Anvil::ImageLayout         in_final_layout,
                                 bool                       in_may_alias,
                                 uint32_t                   in_index)
            {
                color_depth_load_op  = in_load_op;
                color_depth_store_op = in_store_op;
                final_layout         = in_final_layout;
                format               = in_format;
                index                = in_index;
                initial_layout       = in_initial_layout;
                may_alias            = in_may_alias;
                sample_count         = in_sample_count;
                stencil_load_op      = Anvil::AttachmentLoadOp::DONT_CARE;
                stencil_store_op     = Anvil::AttachmentStoreOp::DONT_CARE;
                type                 = Anvil::AttachmentType::COLOR;
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
            RenderPassAttachment(Anvil::Format              in_format,
                                 Anvil::SampleCountFlagBits in_sample_count,
                                 Anvil::AttachmentLoadOp    in_depth_load_op,
                                 Anvil::AttachmentStoreOp   in_depth_store_op,
                                 Anvil::AttachmentLoadOp    in_stencil_load_op,
                                 Anvil::AttachmentStoreOp   in_stencil_store_op,
                                 Anvil::ImageLayout         in_initial_layout,
                                 Anvil::ImageLayout         in_final_layout,
                                 bool                       in_may_alias,
                                 uint32_t                   in_index)
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
                type                 = Anvil::AttachmentType::DEPTH_STENCIL;
            }

            /** Dummy constructor. This should only be used by STL containers. */
            RenderPassAttachment()
            {
                color_depth_load_op  = Anvil::AttachmentLoadOp::UNKNOWN;
                color_depth_store_op = Anvil::AttachmentStoreOp::UNKNOWN;
                final_layout         = Anvil::ImageLayout::UNKNOWN;
                format               = Anvil::Format::UNKNOWN;
                index                = UINT32_MAX;
                initial_layout       = Anvil::ImageLayout::UNKNOWN;
                may_alias            = false;
                sample_count         = static_cast<Anvil::SampleCountFlagBits>(0);
                stencil_load_op      = Anvil::AttachmentLoadOp::UNKNOWN;
                stencil_store_op     = Anvil::AttachmentStoreOp::UNKNOWN;
                type                 = Anvil::AttachmentType::UNKNOWN;
            }
        } RenderPassAttachment;

        typedef std::vector<RenderPassAttachment> RenderPassAttachments;

        /* Holds properties of a sub-pass attachment */
        typedef struct SubPassAttachment
        {
            Anvil::ImageAspectFlags aspects_accessed; /* Only used for input attachments! */

            Anvil::ResolveModeFlagBits depth_resolve_mode;   /* Only used for MS D/DS attachments! */
            Anvil::ResolveModeFlagBits stencil_resolve_mode; /* Only used for MS DS/S attachments! */

            uint32_t           attachment_index;
            uint32_t           highest_subpass_index;
            Anvil::ImageLayout layout;
            uint32_t           lowest_subpass_index;
            uint32_t           resolve_attachment_index;

            /* Dummy constructor. Should only be used by STL containers */
            SubPassAttachment()
            {
                aspects_accessed         = Anvil::ImageAspectFlagBits::NONE;
                attachment_index         = UINT32_MAX;
                depth_resolve_mode       = Anvil::ResolveModeFlagBits::NONE;
                highest_subpass_index    = UINT32_MAX;
                layout                   = Anvil::ImageLayout::UNKNOWN;
                lowest_subpass_index     = UINT32_MAX;
                resolve_attachment_index = UINT32_MAX;
                stencil_resolve_mode     = Anvil::ResolveModeFlagBits::NONE;
            }

            /** Constructor.
             *
             *  @param in_attachment_index             Index of render-pass attachment that this sub-pass attachment should reference.
             *                                         Must not be UINT32_MAX.
             *  @param in_layout                       Layout to use for the attachment when executing the subpass.
             *                                         Driver takes care of transforming the attachment to the requested layout
             *                                         before subpass commands starts executing.
             *  @param in_opt_resolve_attachment_index If not UINT32_MAX, this should point to the render-pass attachment, to which
             *                                         MS data of @param in_attachment_ptr should be resolved. If UINT32_MAX, it is
             *                                         assumed the sub-pass should not resolve the MS data.
             **/
            SubPassAttachment(const uint32_t&                   in_attachment_index,
                              Anvil::ImageLayout                in_layout,
                              const uint32_t&                   in_opt_resolve_attachment_index,
                              const Anvil::ImageAspectFlags&    in_opt_aspects_accessed,
                              const Anvil::ResolveModeFlagBits& in_depth_resolve_mode,
                              const Anvil::ResolveModeFlagBits& in_stencil_resolve_mode)
            {
                aspects_accessed         = in_opt_aspects_accessed;
                attachment_index         = in_attachment_index;
                depth_resolve_mode       = in_depth_resolve_mode;
                highest_subpass_index    = UINT32_MAX;
                layout                   = in_layout;
                lowest_subpass_index     = UINT32_MAX;
                resolve_attachment_index = in_opt_resolve_attachment_index;
                stencil_resolve_mode     = in_stencil_resolve_mode;
            }
        } SubPassAttachment;

        typedef std::map<uint32_t, SubPassAttachment>          LocationToSubPassAttachmentMap;
        typedef LocationToSubPassAttachmentMap::const_iterator LocationToSubPassAttachmentMapConstIterator;
        typedef std::vector<SubPassAttachment>                 SubPassAttachmentVector;
        
        /** Holds properties of a single sub-pass */
        typedef struct SubPass
        {
            LocationToSubPassAttachmentMap color_attachments_map;
            SubPassAttachment              depth_stencil_attachment;
            SubPassAttachment              ds_resolve_attachment;
            uint32_t                       index;
            uint32_t                       multiview_view_mask;
            uint32_t                       n_highest_location_used;
            LocationToSubPassAttachmentMap input_attachments_map;
            SubPassAttachmentVector        preserved_attachments;
            LocationToSubPassAttachmentMap resolved_attachments_map;

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
                index                   = UINT32_MAX;
                multiview_view_mask     = 0;
                n_highest_location_used = 0;
            }

            /** Constructor.
             *
             *  @param in_index Index of the sub-pass
             *
             **/
            SubPass(uint32_t in_index)
            {
                index                   = in_index;
                multiview_view_mask     = 0;
                n_highest_location_used = 0;
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

        typedef std::vector<std::unique_ptr<SubPass> > SubPasses;
        typedef SubPasses::const_iterator              SubPassesConstIterator;

        /** Holds properties of a single subpass<->subpass dependency. */
        typedef struct SubPassDependency
        {
            Anvil::AccessFlags           destination_access_mask;
            Anvil::PipelineStageFlags    destination_stage_mask;
            Anvil::AccessFlags           source_access_mask;
            Anvil::PipelineStageFlags    source_stage_mask;

            DependencyFlags flags;
            const SubPass*  destination_subpass_ptr;
            const SubPass*  source_subpass_ptr;

            int32_t multiview_view_offset;

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
             *  @param in_dependency_flags        Flags to use for the dependency.
             **/
            SubPassDependency(Anvil::PipelineStageFlags in_destination_stage_mask,
                              const SubPass*            in_destination_subpass_ptr,
                              Anvil::PipelineStageFlags in_source_stage_mask,
                              const SubPass*            in_source_subpass_ptr,
                              Anvil::AccessFlags        in_source_access_mask,
                              Anvil::AccessFlags        in_destination_access_mask,
                              const DependencyFlags&    in_flags)
            {
                destination_stage_mask  = in_destination_stage_mask;
                destination_subpass_ptr = in_destination_subpass_ptr;
                destination_access_mask = in_destination_access_mask;
                flags                   = in_flags;
                multiview_view_offset   = INT32_MAX;
                source_access_mask      = in_source_access_mask;
                source_stage_mask       = in_source_stage_mask;
                source_subpass_ptr      = in_source_subpass_ptr;
            }

            /** Dummy constructor. Should only be used by STL containers */
            SubPassDependency()
            {
                destination_subpass_ptr = nullptr;
                multiview_view_offset   = INT32_MAX;
                source_subpass_ptr      = nullptr;
            }

            /** Comparator operator */
            bool operator==(const SubPassDependency& in)
            {
                return in.destination_access_mask == destination_access_mask &&
                       in.destination_stage_mask  == destination_stage_mask  &&
                       in.destination_subpass_ptr == destination_subpass_ptr &&
                       in.flags                   == flags                   &&
                       in.multiview_view_offset   == multiview_view_offset   &&
                       in.source_access_mask      == source_access_mask      &&
                       in.source_stage_mask       == source_stage_mask       &&
                       in.source_subpass_ptr      == source_subpass_ptr;
            }
        } SubPassDependency;

        typedef std::vector<SubPassDependency> SubPassDependencies;


        /* Private functions */

        bool add_dependency                    (SubPass*                       in_destination_subpass_ptr,
                                                SubPass*                       in_source_subpass_ptr,
                                                Anvil::PipelineStageFlags      in_source_stage_mask,
                                                Anvil::PipelineStageFlags      in_destination_stage_mask,
                                                Anvil::AccessFlags             in_source_access_mask,
                                                Anvil::AccessFlags             in_destination_access_mask,
                                                Anvil::DependencyFlags         in_dependency_flags);
        bool add_subpass_color_input_attachment(SubPassID                      in_subpass_id,
                                                bool                           in_is_color_attachment,
                                                Anvil::ImageLayout             in_input_layout,
                                                RenderPassAttachmentID         in_attachment_id,
                                                uint32_t                       in_attachment_location,
                                                bool                           in_should_resolve,
                                                RenderPassAttachmentID         in_resolve_attachment_id,
                                                const Anvil::ImageAspectFlags& in_aspects_accessed);

        VkAttachmentReference get_attachment_reference_from_renderpass_attachment(const RenderPassAttachment&                        in_renderpass_attachment)                const;
        VkAttachmentReference get_attachment_reference_from_subpass_attachment   (const SubPassAttachment&                           in_subpass_attachment)                   const;
        VkAttachmentReference get_attachment_reference_for_resolve_attachment    (const SubPassesConstIterator&                      in_subpass_iterator,
                                                                                  const LocationToSubPassAttachmentMapConstIterator& in_location_to_subpass_att_map_iterator) const;
        void                  update_preserved_attachments                       () const;


        /* Private variables */
        RenderPassAttachments    m_attachments;
        std::vector<uint32_t>    m_correlation_masks;
        const Anvil::BaseDevice* m_device_ptr;
        bool                     m_multiview_enabled;
        SubPasses                m_subpasses;
        SubPassDependencies      m_subpass_dependencies;
        mutable bool             m_update_preserved_attachments;

        friend class Anvil::RenderPass;
    };
};

#endif /* MISC_RENDERPASS_CREATE_INFO_H */
