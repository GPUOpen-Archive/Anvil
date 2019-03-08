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

#include "misc/render_pass_create_info.h"
#include "wrappers/device.h"
#include <algorithm>
#include <cmath>

#if defined(max)
    #undef max
#endif

Anvil::RenderPassCreateInfo::RenderPassCreateInfo(const Anvil::BaseDevice* in_device_ptr)
    :m_device_ptr                  (in_device_ptr),
     m_multiview_enabled           (false),
     m_update_preserved_attachments(false)
{
    anvil_assert(in_device_ptr != nullptr);
}

Anvil::RenderPassCreateInfo::~RenderPassCreateInfo()
{
    /* Stub */
}

/* Please see haeder for specification */
bool Anvil::RenderPassCreateInfo::add_color_attachment(Anvil::Format              in_format,
                                                       Anvil::SampleCountFlagBits in_sample_count,
                                                       Anvil::AttachmentLoadOp    in_load_op,
                                                       Anvil::AttachmentStoreOp   in_store_op,
                                                       Anvil::ImageLayout         in_initial_layout,
                                                       Anvil::ImageLayout         in_final_layout,
                                                       bool                       in_may_alias,
                                                       RenderPassAttachmentID*    out_attachment_id_ptr)
{
    uint32_t new_attachment_index = UINT32_MAX;
    bool     result               = false;

    if (out_attachment_id_ptr == nullptr)
    {
        anvil_assert(out_attachment_id_ptr != nullptr);

        goto end;
    }

    new_attachment_index   = static_cast<uint32_t>(m_attachments.size() );
    *out_attachment_id_ptr = new_attachment_index;

    m_attachments.push_back(RenderPassAttachment(in_format,
                                                 in_sample_count,
                                                 in_load_op,
                                                 in_store_op,
                                                 in_initial_layout,
                                                 in_final_layout,
                                                 in_may_alias,
                                                 new_attachment_index) );

    result = true;

end:
    return result;
}

/** Adds a new dependency to the internal data model.
 *
 *  @param in_destination_subpass_ptr Pointer to the descriptor of the destination subpass.
 *                                    If nullptr, it is assumed an external destination is requested.
 *  @param in_source_subpass_ptr      Pointer to the descriptor of the source subpass.
 *                                    If nullptr, it is assumed an external source is requested.
 *  @param in_source_stage_mask       Source pipeline stage mask.
 *  @param in_destination_stage_mask  Destination pipeline stage mask.
 *  @param in_source_access_mask      Source access mask.
 *  @param in_destination_access_mask Destination access mask.
 *  @param in_by_region               true if a "by-region" dependency is requested; false otherwise.
 *
 *  @return true if the dependency was added successfully; false otherwise.
 *
 **/
bool Anvil::RenderPassCreateInfo::add_dependency(SubPass*                  in_destination_subpass_ptr,
                                                 SubPass*                  in_source_subpass_ptr,
                                                 Anvil::PipelineStageFlags in_source_stage_mask,
                                                 Anvil::PipelineStageFlags in_destination_stage_mask,
                                                 Anvil::AccessFlags        in_source_access_mask,
                                                 Anvil::AccessFlags        in_destination_access_mask,
                                                 Anvil::DependencyFlags    in_dependency_flags)
{
    auto new_dep = SubPassDependency(in_destination_stage_mask,
                                     in_destination_subpass_ptr,
                                     in_source_stage_mask,
                                     in_source_subpass_ptr,
                                     in_source_access_mask,
                                     in_destination_access_mask,
                                     in_dependency_flags);

    if (std::find(m_subpass_dependencies.begin(),
                  m_subpass_dependencies.end(),
                  new_dep) == m_subpass_dependencies.end() )
    {
        m_subpass_dependencies.push_back(new_dep);
    }

    return true;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::add_depth_stencil_attachment(Anvil::Format              in_format,
                                                               Anvil::SampleCountFlagBits in_sample_count,
                                                               Anvil::AttachmentLoadOp    in_depth_load_op,
                                                               Anvil::AttachmentStoreOp   in_depth_store_op,
                                                               Anvil::AttachmentLoadOp    in_stencil_load_op,
                                                               Anvil::AttachmentStoreOp   in_stencil_store_op,
                                                               Anvil::ImageLayout         in_initial_layout,
                                                               Anvil::ImageLayout         in_final_layout,
                                                               bool                       in_may_alias,
                                                               RenderPassAttachmentID*    out_attachment_id_ptr)
{
    uint32_t new_attachment_index = UINT32_MAX;
    bool     result               = false;

    if (out_attachment_id_ptr == nullptr)
    {
        anvil_assert(out_attachment_id_ptr != nullptr);

        goto end;
    }

    new_attachment_index   = static_cast<uint32_t>(m_attachments.size() );
    *out_attachment_id_ptr = new_attachment_index;

    m_attachments.push_back(RenderPassAttachment(in_format,
                                                 in_sample_count,
                                                 in_depth_load_op,
                                                 in_depth_store_op,
                                                 in_stencil_load_op,
                                                 in_stencil_store_op,
                                                 in_initial_layout,
                                                 in_final_layout,
                                                 in_may_alias,
                                                 new_attachment_index) );

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::add_external_to_subpass_dependency(SubPassID                 in_destination_subpass_id,
                                                                     Anvil::PipelineStageFlags in_source_stage_mask,
                                                                     Anvil::PipelineStageFlags in_destination_stage_mask,
                                                                     Anvil::AccessFlags        in_source_access_mask,
                                                                     Anvil::AccessFlags        in_destination_access_mask,
                                                                     Anvil::DependencyFlags    in_dependency_flags)
{
    SubPass* destination_subpass_ptr = nullptr;
    bool     result                  = false;

    if (m_subpasses.size() <= in_destination_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_destination_subpass_id) );

        goto end;
    }

    destination_subpass_ptr = m_subpasses[in_destination_subpass_id].get();

    result = add_dependency(destination_subpass_ptr,
                            nullptr, /* source_subpass_ptr */
                            in_source_stage_mask,
                            in_destination_stage_mask,
                            in_source_access_mask,
                            in_destination_access_mask,
                            in_dependency_flags);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::add_self_subpass_dependency(SubPassID                 in_destination_subpass_id,
                                                              Anvil::PipelineStageFlags in_source_stage_mask,
                                                              Anvil::PipelineStageFlags in_destination_stage_mask,
                                                              Anvil::AccessFlags        in_source_access_mask,
                                                              Anvil::AccessFlags        in_destination_access_mask,
                                                              Anvil::DependencyFlags    in_dependency_flags)
{
    SubPass* destination_subpass_ptr = nullptr;
    bool     result                  = false;

    if (m_subpasses.size() <= in_destination_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_destination_subpass_id) );

        goto end;
    }

    destination_subpass_ptr = m_subpasses[in_destination_subpass_id].get();
    result                  = add_dependency(destination_subpass_ptr,
                                             destination_subpass_ptr,
                                             in_source_stage_mask,
                                             in_destination_stage_mask,
                                             in_source_access_mask,
                                             in_destination_access_mask,
                                             in_dependency_flags);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::add_subpass(SubPassID* out_subpass_id_ptr)
{
    uint32_t new_subpass_index = UINT32_MAX;
    bool     result            = false;

    if (out_subpass_id_ptr == nullptr)
    {
        anvil_assert(out_subpass_id_ptr != nullptr);

        goto end;
    }

    new_subpass_index = static_cast<uint32_t>(m_subpasses.size() );

    /* Spawn a new descriptor */
    {
        std::unique_ptr<SubPass> new_subpass_ptr(
            new SubPass(new_subpass_index)
        );

        m_subpasses.push_back(
            std::move(new_subpass_ptr)
        );

        *out_subpass_id_ptr = new_subpass_index;
    }

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::add_subpass_color_attachment(SubPassID                     in_subpass_id,
                                                               Anvil::ImageLayout            in_input_layout,
                                                               RenderPassAttachmentID        in_attachment_id,
                                                               uint32_t                      in_location,
                                                               const RenderPassAttachmentID* in_attachment_resolve_id_ptr)
{
    return add_subpass_color_input_attachment(in_subpass_id,
                                              true, /* is_color_attachment */
                                              in_input_layout,
                                              in_attachment_id,
                                              in_location,
                                              (in_attachment_resolve_id_ptr != nullptr),
                                              (in_attachment_resolve_id_ptr != nullptr) ? *in_attachment_resolve_id_ptr
                                                                                        : UINT32_MAX,
                                              Anvil::ImageAspectFlagBits::NONE); /* in_stencil_resolve_mode */
}

/** Adds a new attachment to the specified subpass.
 *
 *  @param in_subpass_id            ID of the subpass to update. The subpass must have been earlier
 *                                  created with an add_subpass() call.
 *  @param in_is_color_attachment   true if the added attachment is a color attachment;false if it's
 *                                  an input attachment.
 *  @param in_layout                Layout to use for the attachment when executing the subpass.
 *                                  Driver takes care of transforming the attachment to the requested layout
 *                                  before subpass commands starts executing.
 *  @param in_attachment_id         ID of a render-pass attachment ID this sub-pass attachment should
 *                                  refer to.
 *  @param in_attachment_location   Location, under which the specified attachment should be accessible.
 *  @param in_should_resolve        true if the specified attachment is multisample and should be
 *                                  resolved at the end of the sub-pass.
 *  @parma in_resolve_attachment_id If @param should_resolve is true, this argument should specify the
 *                                  ID of a render-pass attachment, to which the resolved data should
 *                                  written to.
 *
 *  @return true if the function executed successfully, false otherwise.
 *
 **/
bool Anvil::RenderPassCreateInfo::add_subpass_color_input_attachment(SubPassID                      in_subpass_id,
                                                                     bool                           in_is_color_attachment,
                                                                     Anvil::ImageLayout             in_layout,
                                                                     RenderPassAttachmentID         in_attachment_id,
                                                                     uint32_t                       in_attachment_location,
                                                                     bool                           in_should_resolve,
                                                                     RenderPassAttachmentID         in_resolve_attachment_id,
                                                                     const Anvil::ImageAspectFlags& in_aspects_accessed)
{
    bool                            result                    = false;
    LocationToSubPassAttachmentMap* subpass_attachments_ptr   = nullptr;
    SubPass*                        subpass_ptr               = nullptr;

    /* Retrieve the subpass descriptor */
    if (in_subpass_id >= m_subpasses.size() )
    {
        anvil_assert(!(in_subpass_id >= m_subpasses.size() ));

        goto end;
    }
    else
    {
        subpass_ptr = m_subpasses.at(in_subpass_id).get();
    }

    /* Sanity checks */
    if (in_attachment_id >= m_attachments.size() )
    {
        anvil_assert(!(in_attachment_id >= m_attachments.size()) );

        goto end;
    }

    if (in_should_resolve                                &&
        in_resolve_attachment_id >= m_attachments.size() )
    {
        anvil_assert(!(in_should_resolve && in_resolve_attachment_id >= m_attachments.size()) );

        goto end;
    }

    /* Make sure the attachment location is not already assigned an attachment */
    subpass_attachments_ptr = (in_is_color_attachment) ? &subpass_ptr->color_attachments_map
                                                       : &subpass_ptr->input_attachments_map;

    if (subpass_attachments_ptr->find(in_attachment_location) != subpass_attachments_ptr->end() )
    {
        anvil_assert(!(subpass_attachments_ptr->find(in_attachment_location) != subpass_attachments_ptr->end()) );

        goto end;
    }

    /* Add the attachment */
    (*subpass_attachments_ptr)[in_attachment_location] = SubPassAttachment(in_attachment_id,
                                                                           in_layout,
                                                                           in_resolve_attachment_id,
                                                                           in_aspects_accessed,
                                                                           Anvil::ResolveModeFlagBits::NONE,
                                                                           Anvil::ResolveModeFlagBits::NONE);

    if (in_should_resolve)
    {
        anvil_assert(in_is_color_attachment);

        subpass_ptr->resolved_attachments_map[in_attachment_location] = SubPassAttachment(in_resolve_attachment_id,
                                                                                          in_layout,
                                                                                          UINT32_MAX, /* in_opt_resolve_attachment_index */
                                                                                          in_aspects_accessed,
                                                                                          Anvil::ResolveModeFlagBits::NONE,  /* in_depth_resolve_mode   */
                                                                                          Anvil::ResolveModeFlagBits::NONE); /* in_stencil_resolve_mode */
    }

    if (subpass_ptr->n_highest_location_used < in_attachment_location)
    {
        subpass_ptr->n_highest_location_used = in_attachment_location;
    }

    m_update_preserved_attachments = true;
    result                         = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::add_subpass_depth_stencil_attachment(SubPassID                         in_subpass_id,
                                                                       Anvil::ImageLayout                in_layout,
                                                                       RenderPassAttachmentID            in_attachment_id,
                                                                       const RenderPassAttachmentID*     in_attachment_resolve_id_ptr,
                                                                       const Anvil::ResolveModeFlagBits* in_depth_resolve_mode_ptr,
                                                                       const Anvil::ResolveModeFlagBits* in_stencil_resolve_mode_ptr)
{
    bool     result      = false;
    SubPass* subpass_ptr = nullptr;

    /* Sanity checks .. */
    if (in_attachment_resolve_id_ptr != nullptr)
    {
        if (!m_device_ptr->get_extension_info()->khr_depth_stencil_resolve() )
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_depth_stencil_resolve() );

            goto end;
        }

        if (*in_attachment_resolve_id_ptr >= m_attachments.size() )
        {
            anvil_assert(!(*in_attachment_resolve_id_ptr >= m_attachments.size()) );

            goto end;
        }
    }

    /* Retrieve the subpass descriptor */
    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }
    else
    {
        subpass_ptr = m_subpasses.at(in_subpass_id).get();
    }

    /* Retrieve the attachment descriptors */
    if (m_attachments.size() <= in_attachment_id)
    {
        anvil_assert(!(m_attachments.size() <= in_attachment_id) );

        goto end;
    }

    /* Update the depth/stencil attachment for the subpass */
    if (subpass_ptr->depth_stencil_attachment.attachment_index != UINT32_MAX)
    {
        anvil_assert(!(subpass_ptr->depth_stencil_attachment.attachment_index != UINT32_MAX) );

        goto end;
    }

    subpass_ptr->depth_stencil_attachment = SubPassAttachment(in_attachment_id,
                                                              in_layout,
                                                              (in_attachment_resolve_id_ptr != nullptr) ? *in_attachment_resolve_id_ptr
                                                                                                        : UINT32_MAX,
                                                              Anvil::ImageAspectFlagBits::NONE,
                                                              Anvil::ResolveModeFlagBits::NONE,  /* in_depth_resolve_mode   */
                                                              Anvil::ResolveModeFlagBits::NONE); /* in_stencil_resolve_mode */

    if ( in_attachment_resolve_id_ptr != nullptr    &&
        *in_attachment_resolve_id_ptr != UINT32_MAX)
    {
        subpass_ptr->ds_resolve_attachment = SubPassAttachment(*in_attachment_resolve_id_ptr,
                                                                in_layout,
                                                                UINT32_MAX, /* in_opt_resolve_attachment_index */
                                                                Anvil::ImageAspectFlagBits::NONE,
                                                               *in_depth_resolve_mode_ptr,
                                                               *in_stencil_resolve_mode_ptr);
    }

    m_update_preserved_attachments = true;
    result                         = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::add_subpass_input_attachment(SubPassID                      in_subpass_id,
                                                               Anvil::ImageLayout             in_layout,
                                                               RenderPassAttachmentID         in_attachment_id,
                                                               uint32_t                       in_attachment_index,
                                                               const Anvil::ImageAspectFlags& in_opt_aspects_accessed)
{
    return add_subpass_color_input_attachment(in_subpass_id,
                                              false, /* is_color_attachment */
                                              in_layout,
                                              in_attachment_id,
                                              in_attachment_index,
                                              false,         /* should_resolve        */
                                              UINT32_MAX,    /* resolve_attachment_id */
                                              in_opt_aspects_accessed);
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::add_subpass_to_external_dependency(SubPassID                 in_source_subpass_id,
                                                                     Anvil::PipelineStageFlags in_source_stage_mask,
                                                                     Anvil::PipelineStageFlags in_destination_stage_mask,
                                                                     Anvil::AccessFlags        in_source_access_mask,
                                                                     Anvil::AccessFlags        in_destination_access_mask,
                                                                     Anvil::DependencyFlags    in_dependency_flags)
{
    bool     result             = false;
    SubPass* source_subpass_ptr = nullptr;

    if (m_subpasses.size() <= in_source_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_source_subpass_id) );

        goto end;
    }

    source_subpass_ptr = m_subpasses[in_source_subpass_id].get();

    result = add_dependency(nullptr, /* destination_subpass_ptr */
                            source_subpass_ptr,
                            in_source_stage_mask,
                            in_destination_stage_mask,
                            in_source_access_mask,
                            in_destination_access_mask,
                            in_dependency_flags);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::add_subpass_to_subpass_dependency(SubPassID                 in_source_subpass_id,
                                                                    SubPassID                 in_destination_subpass_id,
                                                                    Anvil::PipelineStageFlags in_source_stage_mask,
                                                                    Anvil::PipelineStageFlags in_destination_stage_mask,
                                                                    Anvil::AccessFlags        in_source_access_mask,
                                                                    Anvil::AccessFlags        in_destination_access_mask,
                                                                    Anvil::DependencyFlags    in_dependency_flags)
{
    SubPass* destination_subpass_ptr = nullptr;
    bool     result                  = false;
    SubPass* source_subpass_ptr      = nullptr;

    if (m_subpasses.size() <= in_destination_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_destination_subpass_id) );

        goto end;
    }

    if (m_subpasses.size() <= in_source_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_source_subpass_id) );

        goto end;
    }

    destination_subpass_ptr = m_subpasses[in_destination_subpass_id].get();
    source_subpass_ptr      = m_subpasses[in_source_subpass_id].get();

    result = add_dependency(destination_subpass_ptr,
                            source_subpass_ptr,
                            in_source_stage_mask,
                            in_destination_stage_mask,
                            in_source_access_mask,
                            in_destination_access_mask,
                            in_dependency_flags);
end:
    return result;
}

/** Creates a VkAttachmentReference descriptor from the specified RenderPassAttachment instance.
 *
 *  @param in_renderpass_attachment Renderpass attachment descriptor to create the Vulkan descriptor from.
 *
 *  @return As per description.
 **/
VkAttachmentReference Anvil::RenderPassCreateInfo::get_attachment_reference_from_renderpass_attachment(const RenderPassAttachment& in_renderpass_attachment) const
{
    VkAttachmentReference attachment_vk;

    attachment_vk.attachment = in_renderpass_attachment.index;
    attachment_vk.layout     = static_cast<VkImageLayout>(in_renderpass_attachment.initial_layout);

    return attachment_vk;
}

/** Creates a VkAttachmentReference descriptor from the specified SubPassAttachment instance.
 *
 *  @param in_renderpass_attachment Subpass attachment descriptor to create the Vulkan descriptor from.
 *
 *  @return As per description.
 **/
VkAttachmentReference Anvil::RenderPassCreateInfo::get_attachment_reference_from_subpass_attachment(const SubPassAttachment& in_subpass_attachment) const
{
    VkAttachmentReference attachment_vk;

    attachment_vk.attachment = m_attachments.at(in_subpass_attachment.attachment_index).index;
    attachment_vk.layout     = static_cast<VkImageLayout>(in_subpass_attachment.layout);

    return attachment_vk;
}

/** Creates a VkAttachmentReference descriptor for a resolve attachment for a color attachment specified by the user.
 *
 *  @param in_subpass_iterator                     Iterator pointing at the subpass which uses the color attachment of interest.
 *  @param in_location_to_subpass_att_map_iterator Iterator pointing at a color attachment which has a resolve attachment attached.
 *
 *  @return As per description.
 **/
VkAttachmentReference Anvil::RenderPassCreateInfo::get_attachment_reference_for_resolve_attachment(const SubPassesConstIterator&                      in_subpass_iterator,
                                                                                                   const LocationToSubPassAttachmentMapConstIterator& in_location_to_subpass_att_map_iterator) const
{
    VkAttachmentReference result;

    anvil_assert((*in_subpass_iterator)->resolved_attachments_map.find(in_location_to_subpass_att_map_iterator->first) != (*in_subpass_iterator)->resolved_attachments_map.end() );

    result.attachment = m_attachments.at(in_location_to_subpass_att_map_iterator->second.resolve_attachment_index).index;
    result.layout     = static_cast<VkImageLayout>((*in_subpass_iterator)->resolved_attachments_map.at(in_location_to_subpass_att_map_iterator->first).layout);

    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::get_attachment_type(RenderPassAttachmentID in_attachment_id,
                                                      AttachmentType*        out_attachment_type_ptr) const
{
    bool result = false;

    if (m_attachments.size() <= in_attachment_id)
    {
        anvil_assert(m_attachments.size() > in_attachment_id);

        goto end;
    }

    *out_attachment_type_ptr = m_attachments[in_attachment_id].type;

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::get_color_attachment_properties(RenderPassAttachmentID      in_attachment_id,
                                                                  Anvil::Format*              out_opt_format_ptr,
                                                                  Anvil::SampleCountFlagBits* out_opt_sample_count_ptr,
                                                                  Anvil::AttachmentLoadOp*    out_opt_load_op_ptr,
                                                                  Anvil::AttachmentStoreOp*   out_opt_store_op_ptr,
                                                                  Anvil::ImageLayout*         out_opt_initial_layout_ptr,
                                                                  Anvil::ImageLayout*         out_opt_final_layout_ptr,
                                                                  bool*                       out_opt_may_alias_ptr) const
{
    bool result = false;

    if (m_attachments.size() <= in_attachment_id)
    {
        goto end;
    }

    if (out_opt_format_ptr != nullptr)
    {
        *out_opt_format_ptr = m_attachments[in_attachment_id].format;
    }

    if (out_opt_sample_count_ptr != nullptr)
    {
        *out_opt_sample_count_ptr = m_attachments[in_attachment_id].sample_count;
    }

    if (out_opt_load_op_ptr != nullptr)
    {
        *out_opt_load_op_ptr = m_attachments[in_attachment_id].color_depth_load_op;
    }

    if (out_opt_store_op_ptr != nullptr)
    {
        *out_opt_store_op_ptr = m_attachments[in_attachment_id].color_depth_store_op;
    }

    if (out_opt_initial_layout_ptr != nullptr)
    {
        *out_opt_initial_layout_ptr = m_attachments[in_attachment_id].initial_layout;
    }

    if (out_opt_final_layout_ptr != nullptr)
    {
        *out_opt_final_layout_ptr = m_attachments[in_attachment_id].final_layout;
    }

    if (out_opt_may_alias_ptr != nullptr)
    {
        *out_opt_may_alias_ptr = m_attachments[in_attachment_id].may_alias;
    }

    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::RenderPassCreateInfo::get_dependency_properties(uint32_t                   in_n_dependency,
                                                            SubPassID*                 out_destination_subpass_id_ptr,
                                                            SubPassID*                 out_source_subpass_id_ptr,
                                                            Anvil::PipelineStageFlags* out_destination_stage_mask_ptr,
                                                            Anvil::PipelineStageFlags* out_source_stage_mask_ptr,
                                                            Anvil::AccessFlags*        out_destination_access_mask_ptr,
                                                            Anvil::AccessFlags*        out_source_access_mask_ptr,
                                                            DependencyFlags*           out_flags_ptr) const
{
    const Anvil::RenderPassCreateInfo::SubPassDependency* dep_ptr = nullptr;
    bool                                                  result  = false;

    if (m_subpass_dependencies.size() <= in_n_dependency)
    {
        anvil_assert_fail();

        goto end;
    }

    dep_ptr = &m_subpass_dependencies[in_n_dependency];

    *out_destination_subpass_id_ptr  = (dep_ptr->destination_subpass_ptr != nullptr) ? dep_ptr->destination_subpass_ptr->index
                                                                                     : UINT32_MAX;
    *out_flags_ptr                   = dep_ptr->flags;
    *out_source_subpass_id_ptr       = (dep_ptr->source_subpass_ptr      != nullptr) ? dep_ptr->source_subpass_ptr->index
                                                                                     : UINT32_MAX;
    *out_destination_stage_mask_ptr  = dep_ptr->destination_stage_mask;
    *out_source_stage_mask_ptr       = dep_ptr->source_stage_mask;
    *out_destination_access_mask_ptr = dep_ptr->destination_access_mask;
    *out_source_access_mask_ptr      = dep_ptr->source_access_mask;

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::RenderPassCreateInfo::get_dependency_multiview_properties(uint32_t in_n_dependency,
                                                                      int32_t* out_view_offset_ptr) const
{
    bool result = false;

    if (!m_multiview_enabled)
    {
        anvil_assert(m_multiview_enabled);

        goto end;
    }

    if (m_subpass_dependencies.size() <= in_n_dependency)
    {
        anvil_assert(m_subpass_dependencies.size() > in_n_dependency);

        goto end;
    }

    *out_view_offset_ptr = m_subpass_dependencies.at(in_n_dependency).multiview_view_offset;
    result               = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::RenderPassCreateInfo::get_depth_stencil_attachment_properties(RenderPassAttachmentID      in_attachment_id,
                                                                          Anvil::Format*              out_opt_format_ptr,
                                                                          Anvil::SampleCountFlagBits* out_opt_sample_count_ptr,
                                                                          Anvil::AttachmentLoadOp*    out_opt_depth_load_op_ptr,
                                                                          Anvil::AttachmentStoreOp*   out_opt_depth_store_op_ptr,
                                                                          Anvil::AttachmentLoadOp*    out_opt_stencil_load_op_ptr,
                                                                          Anvil::AttachmentStoreOp*   out_opt_stencil_store_op_ptr,
                                                                          Anvil::ImageLayout*         out_opt_initial_layout_ptr,
                                                                          Anvil::ImageLayout*         out_opt_final_layout_ptr,
                                                                          bool*                       out_opt_may_alias_ptr) const
{
    bool result = false;

    if (m_attachments.size() <= in_attachment_id)
    {
        goto end;
    }

    if (out_opt_format_ptr != nullptr)
    {
        *out_opt_format_ptr = m_attachments[in_attachment_id].format;
    }

    if (out_opt_sample_count_ptr != nullptr)
    {
        *out_opt_sample_count_ptr = m_attachments[in_attachment_id].sample_count;
    }

    if (out_opt_depth_load_op_ptr != nullptr)
    {
        *out_opt_depth_load_op_ptr = m_attachments[in_attachment_id].color_depth_load_op;
    }

    if (out_opt_depth_store_op_ptr != nullptr)
    {
        *out_opt_depth_store_op_ptr = m_attachments[in_attachment_id].color_depth_store_op;
    }

    if (out_opt_stencil_load_op_ptr != nullptr)
    {
        *out_opt_stencil_load_op_ptr = m_attachments[in_attachment_id].stencil_load_op;
    }

    if (out_opt_stencil_store_op_ptr != nullptr)
    {
        *out_opt_stencil_store_op_ptr = m_attachments[in_attachment_id].stencil_store_op;
    }

    if (out_opt_initial_layout_ptr != nullptr)
    {
        *out_opt_initial_layout_ptr = m_attachments[in_attachment_id].initial_layout;
    }

    if (out_opt_final_layout_ptr != nullptr)
    {
        *out_opt_final_layout_ptr = m_attachments[in_attachment_id].final_layout;
    }

    if (out_opt_may_alias_ptr != nullptr)
    {
        *out_opt_may_alias_ptr = m_attachments[in_attachment_id].may_alias;
    }

    result = true;
end:
    return result;
}

/* Please see header for specification */
uint32_t Anvil::RenderPassCreateInfo::get_max_color_location_used_by_subpass(const SubPassID& in_subpass_id) const
{
    uint32_t result           = UINT32_MAX;
    auto     subpass_iterator = (m_subpasses.size() > in_subpass_id) ? m_subpasses.begin() + in_subpass_id : m_subpasses.end();

    if (subpass_iterator == m_subpasses.end() )
    {
        anvil_assert(subpass_iterator != m_subpasses.end() );

        goto end;
    }

    result = 0;

    for (const auto& current_color_attachment_data : (*subpass_iterator)->color_attachments_map)
    {
        result = std::max(result,
                          current_color_attachment_data.first);
    }

end:
    return result;
}

/* Please see header for specification */
void Anvil::RenderPassCreateInfo::get_multiview_correlation_masks(uint32_t*        out_n_correlation_masks_ptr,
                                                                  const uint32_t** out_correlation_masks_ptr_ptr) const
{
    anvil_assert(m_multiview_enabled);

    *out_correlation_masks_ptr_ptr = (m_correlation_masks.size() > 0) ? &m_correlation_masks.at(0) : nullptr;
    *out_n_correlation_masks_ptr   = static_cast<uint32_t>(m_correlation_masks.size() );
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::get_subpass_n_attachments(SubPassID      in_subpass_id,
                                                            AttachmentType in_attachment_type,
                                                            uint32_t*      out_n_attachments_ptr) const
{
    bool result = false;

    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }
    else
    {
        result = true;

        if (in_attachment_type             == Anvil::AttachmentType::PRESERVE &&
            m_update_preserved_attachments)
        {
            update_preserved_attachments();

            anvil_assert(!m_update_preserved_attachments);
        }

        switch (in_attachment_type)
        {
            case Anvil::AttachmentType::COLOR:    *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->color_attachments_map.size() );    break;
            case Anvil::AttachmentType::INPUT:    *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->input_attachments_map.size() );    break;
            case Anvil::AttachmentType::PRESERVE: *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->preserved_attachments.size() );    break;
            case Anvil::AttachmentType::RESOLVE:  *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->resolved_attachments_map.size() ); break;

            case Anvil::AttachmentType::DEPTH_STENCIL:
            {
                *out_n_attachments_ptr = (m_subpasses[in_subpass_id]->depth_stencil_attachment.attachment_index != UINT32_MAX) ? 1u : 0u;

                break;
            }

            default:
            {
                anvil_assert_fail();

                result = false;
            }
        }
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::get_subpass_attachment_properties(SubPassID                   in_subpass_id,
                                                                    AttachmentType              in_attachment_type,
                                                                    uint32_t                    in_n_subpass_attachment,
                                                                    RenderPassAttachmentID*     out_renderpass_attachment_id_ptr,
                                                                    Anvil::ImageLayout*         out_layout_ptr,
                                                                    Anvil::ImageAspectFlags*    out_opt_aspects_accessed_ptr,
                                                                    RenderPassAttachmentID*     out_opt_attachment_resolve_id_ptr,
                                                                    uint32_t*                   out_opt_location_ptr) const
{
    SubPassAttachment               attachment;
    bool                            result                  = false;
    LocationToSubPassAttachmentMap* subpass_attachments_ptr = nullptr;
    SubPass*                        subpass_ptr             = nullptr;

    /* Sanity checks */
    if (in_attachment_type == Anvil::AttachmentType::PRESERVE &&
        out_layout_ptr     != nullptr)
    {
        anvil_assert(!(in_attachment_type == Anvil::AttachmentType::PRESERVE &&
                       out_layout_ptr     != nullptr) );

        goto end;
    }

    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }

    subpass_ptr = m_subpasses[in_subpass_id].get();

    /* Even more sanity checks.. */
    switch (in_attachment_type)
    {
        case Anvil::AttachmentType::COLOR:   /* Fall-through */
        case Anvil::AttachmentType::INPUT:   /* Fall-through */
        case Anvil::AttachmentType::RESOLVE:
        {
            subpass_attachments_ptr = (in_attachment_type == Anvil::AttachmentType::COLOR) ? &subpass_ptr->color_attachments_map
                                    : (in_attachment_type == Anvil::AttachmentType::INPUT) ? &subpass_ptr->input_attachments_map
                                                                                           : &subpass_ptr->resolved_attachments_map;

            auto iterator = subpass_attachments_ptr->begin();

            for (uint32_t n_key = 0;
                          n_key < in_n_subpass_attachment;
                        ++n_key)
            {
                if (iterator == subpass_attachments_ptr->end() )
                {
                    goto end;
                }

                iterator ++;
            }

            if (iterator == subpass_attachments_ptr->end() )
            {
                goto end;
            }

            if (out_opt_aspects_accessed_ptr != nullptr)
            {
                *out_opt_aspects_accessed_ptr = iterator->second.aspects_accessed;
            }

            if (out_opt_attachment_resolve_id_ptr != nullptr)
            {
                *out_opt_attachment_resolve_id_ptr = iterator->second.resolve_attachment_index;
            }

            if (out_opt_location_ptr != nullptr)
            {
                *out_opt_location_ptr = iterator->first;
            }

            *out_layout_ptr                   = iterator->second.layout;
            *out_renderpass_attachment_id_ptr = m_attachments.at(iterator->second.attachment_index).index;

            break;
        }

        case Anvil::AttachmentType::DEPTH_STENCIL:
        {
            if (in_n_subpass_attachment > 0)
            {
                anvil_assert(in_n_subpass_attachment == 0);

                goto end;
            }

            if (out_opt_aspects_accessed_ptr != nullptr)
            {
                *out_opt_aspects_accessed_ptr = Anvil::ImageAspectFlagBits::NONE;
            }

            if (out_opt_attachment_resolve_id_ptr != nullptr)
            {
                *out_opt_attachment_resolve_id_ptr = subpass_ptr->depth_stencil_attachment.resolve_attachment_index;
            }

            if (out_opt_location_ptr != nullptr)
            {
                *out_opt_location_ptr = 0;
            }

            *out_layout_ptr                   = subpass_ptr->depth_stencil_attachment.layout;
            *out_renderpass_attachment_id_ptr = m_attachments.at(subpass_ptr->depth_stencil_attachment.attachment_index).index;

            break;
        }

        case Anvil::AttachmentType::PRESERVE:
        {
            if (subpass_ptr->preserved_attachments.size() <= in_n_subpass_attachment)
            {
                anvil_assert(subpass_ptr->preserved_attachments.size() > in_n_subpass_attachment);

                goto end;
            }

            const auto& attachment_props = subpass_ptr->preserved_attachments[in_n_subpass_attachment];

            *out_renderpass_attachment_id_ptr = m_attachments.at(attachment_props.attachment_index).index;

            if (out_opt_aspects_accessed_ptr != nullptr)
            {
                *out_opt_aspects_accessed_ptr = Anvil::ImageAspectFlagBits::NONE;
            }

            if (out_opt_location_ptr != nullptr)
            {
                *out_opt_location_ptr = UINT32_MAX;
            }

            anvil_assert(out_opt_attachment_resolve_id_ptr == nullptr);
            anvil_assert(out_opt_location_ptr              == nullptr);

            break;
        }

        default:
        {
            anvil_assert_fail();

            goto end;
        }
    }

    /* All done */
    result = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::get_subpass_ds_resolve_attachment_properties(SubPassID                   in_subpass_id,
                                                                               RenderPassAttachmentID*     out_opt_renderpass_attachment_id_ptr,
                                                                               Anvil::ImageLayout*         out_opt_layout_ptr,
                                                                               Anvil::ResolveModeFlagBits* out_opt_depth_resolve_mode_ptr,
                                                                               Anvil::ResolveModeFlagBits* out_opt_stencil_resolve_mode_ptr) const
{
    SubPassAttachment        attachment;
    bool                     result                 = false;
    const SubPassAttachment* subpass_attachment_ptr = nullptr;
    SubPass*                 subpass_ptr            = nullptr;

    /* Sanity checks */
    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }

    subpass_ptr = m_subpasses[in_subpass_id].get();

    /* Even more sanity checks.. */
    subpass_attachment_ptr = &subpass_ptr->ds_resolve_attachment;

    if (subpass_attachment_ptr->depth_resolve_mode   == Anvil::ResolveModeFlagBits::NONE &&
        subpass_attachment_ptr->stencil_resolve_mode == Anvil::ResolveModeFlagBits::NONE)
    {
        goto end;
    }

    if (out_opt_depth_resolve_mode_ptr != nullptr)
    {
        *out_opt_depth_resolve_mode_ptr = subpass_attachment_ptr->depth_resolve_mode;
    }

    if (out_opt_layout_ptr != nullptr)
    {
        *out_opt_layout_ptr = subpass_attachment_ptr->layout;
    }

    if (out_opt_renderpass_attachment_id_ptr != nullptr)
    {
        *out_opt_renderpass_attachment_id_ptr = m_attachments.at(subpass_attachment_ptr->attachment_index).index;
    }

    if (out_opt_stencil_resolve_mode_ptr != nullptr)
    {
        *out_opt_stencil_resolve_mode_ptr = subpass_attachment_ptr->stencil_resolve_mode;
    }

    /* All done */
    result = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::get_subpass_highest_location(SubPassID in_subpass_id,
                                                               uint32_t* out_result_ptr) const
{
    bool result = false;

    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(m_subpasses.size() > in_subpass_id);

        goto end;
    }

    if (m_subpasses[in_subpass_id]->color_attachments_map.size   () == 0 &&
        m_subpasses[in_subpass_id]->input_attachments_map.size   () == 0 &&
        m_subpasses[in_subpass_id]->resolved_attachments_map.size() == 0)
    {
        goto end;
    }

    *out_result_ptr = m_subpasses[in_subpass_id]->n_highest_location_used;
    result          = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::get_subpass_view_mask(SubPassID in_subpass_id,
                                                        uint32_t* out_view_mask_ptr) const
{
    bool result = false;

    if (!m_multiview_enabled)
    {
        anvil_assert(m_multiview_enabled);

        goto end;
    }

    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(m_subpasses.size() > in_subpass_id);

        goto end;
    }

    *out_view_mask_ptr = m_subpasses.at(in_subpass_id)->multiview_view_mask;
    result             = true;
end:
    return result;
}

/* Please see header for specification */
void Anvil::RenderPassCreateInfo::set_correlation_masks(const uint32_t& in_n_correlation_masks,
                                                        const uint32_t* in_correlation_masks_ptr)
{
    anvil_assert(in_n_correlation_masks != 0);

    m_correlation_masks.resize(in_n_correlation_masks);

    memcpy(&m_correlation_masks.at(0),
           in_correlation_masks_ptr,
           in_n_correlation_masks * sizeof(uint32_t) );

    if (!m_multiview_enabled)
    {
        m_multiview_enabled = true;
    }
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::set_dependency_view_local_properties(const uint32_t& in_n_dependency,
                                                                       const int32_t&  in_view_offset)
{
    decltype(m_subpass_dependencies)::iterator dep_iterator;
    bool                                       result       = false;

    if (m_subpass_dependencies.size() <= in_n_dependency)
    {
        anvil_assert(m_subpass_dependencies.size() > in_n_dependency);

        goto end;
    }

    dep_iterator                        = m_subpass_dependencies.begin() + in_n_dependency;
    dep_iterator->multiview_view_offset = in_view_offset;

    if (!m_multiview_enabled)
    {
        m_multiview_enabled = true;
    }

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassCreateInfo::set_subpass_view_mask(SubPassID       in_subpass_id,
                                                        const uint32_t& in_view_mask)
{
    bool                            result           = false;
    decltype(m_subpasses)::iterator subpass_iterator;

    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(m_subpasses.size() > in_subpass_id);

        goto end;
    }

    subpass_iterator                         = m_subpasses.begin() + in_subpass_id;
    (*subpass_iterator)->multiview_view_mask = in_view_mask;

    if (!m_multiview_enabled)
    {
        m_multiview_enabled = true;
    }

    result = true;
end:
    return result;
}

/** Initializes the vector of preserved attachments for all defined attachments. The algorithm
 *  used is as follows:
 *
 *  For each subpass:
 *
 *  1. Check what the highest subpass index that the attachment is preserved/used in is.
 *  2. For all previous subpasses, determine which subpasses do not use it. For each subpass, preserve
 *     the attachment.
 *
 *  This approach may need to be changed or extended in the future.
 *
 *  This function should be considered expensive.
 **/
void Anvil::RenderPassCreateInfo::update_preserved_attachments() const
{
    anvil_assert(m_update_preserved_attachments);

    /* Cache color, depth+stencil, resolve attachments, as used by all defined subpasses.
     * Make sure not to insert duplicate items. */
    std::vector<SubPassAttachment*> unique_attachments;

    for (auto subpass_iterator  = m_subpasses.begin();
              subpass_iterator != m_subpasses.end();
            ++subpass_iterator)
    {
        SubPass* current_subpass_ptr = subpass_iterator->get();

        for (uint32_t n_attachment_type = 0;
                      n_attachment_type < 3; /* color, depth+stencil, resolve */
                    ++n_attachment_type)
        {
            const uint32_t n_attachments = (n_attachment_type == 0) ? static_cast<uint32_t>(current_subpass_ptr->color_attachments_map.size() )
                                         : (n_attachment_type == 1) ? ((current_subpass_ptr->depth_stencil_attachment.attachment_index != UINT32_MAX) ? 1 : 0)
                                                                    : static_cast<uint32_t>(current_subpass_ptr->resolved_attachments_map.size() );

            for (uint32_t n_attachment = 0;
                          n_attachment < n_attachments;
                        ++n_attachment)
            {
                SubPassAttachment* current_attachment_ptr = (n_attachment_type == 0) ?  current_subpass_ptr->get_color_attachment_at_index(n_attachment)
                                                          : (n_attachment_type == 1) ? &current_subpass_ptr->depth_stencil_attachment
                                                                                     :  current_subpass_ptr->get_resolved_attachment_at_index(n_attachment);

                if (std::find(unique_attachments.begin(),
                              unique_attachments.end(),
                              current_attachment_ptr) == unique_attachments.end() )
                {
                    unique_attachments.push_back(current_attachment_ptr);
                }
            }
        }

        /* Clean subpass's preserved attachments vector along the way.. */
        current_subpass_ptr->preserved_attachments.clear();
    }

    /* Determine what the index of the subpass which uses each unique attachment for the first, and for the last time, is. */
    for (std::vector<SubPassAttachment*>::iterator unique_attachment_iterator  = unique_attachments.begin();
                                                   unique_attachment_iterator != unique_attachments.end();
                                                 ++unique_attachment_iterator)
    {
        uint32_t                 current_subpass_index         = 0;
        const SubPassAttachment* current_unique_attachment_ptr = *unique_attachment_iterator;
        uint32_t                 lowest_subpass_index          = static_cast<uint32_t>(m_subpasses.size() - 1);
        uint32_t                 highest_subpass_index         = 0;

        for (auto subpass_iterator  = m_subpasses.begin();
                  subpass_iterator != m_subpasses.end();
                ++subpass_iterator, ++current_subpass_index)
        {
            SubPass* current_subpass_ptr = subpass_iterator->get();
            bool     subpass_processed   = false;

            for (uint32_t n_attachment_type = 0;
                          n_attachment_type < 3 /* color, depth+stencil, resolve */ && !subpass_processed;
                        ++n_attachment_type)
            {
                const uint32_t n_attachments = (n_attachment_type == 0) ? static_cast<uint32_t>(current_subpass_ptr->color_attachments_map.size() )
                                             : (n_attachment_type == 1) ? ((current_subpass_ptr->depth_stencil_attachment.attachment_index != UINT32_MAX) ? 1 : 0)
                                                                        : static_cast<uint32_t>(current_subpass_ptr->resolved_attachments_map.size() );

                for (uint32_t n_attachment = 0;
                              n_attachment < n_attachments && !subpass_processed;
                            ++n_attachment)
                {
                    SubPassAttachment* current_attachment_ptr = (n_attachment_type == 0) ?  current_subpass_ptr->get_color_attachment_at_index(n_attachment)
                                                              : (n_attachment_type == 1) ? &current_subpass_ptr->depth_stencil_attachment
                                                                                         :  current_subpass_ptr->get_resolved_attachment_at_index(n_attachment);

                    if (current_attachment_ptr == current_unique_attachment_ptr)
                    {
                        if (lowest_subpass_index > current_subpass_index)
                        {
                            lowest_subpass_index = current_subpass_index;
                        }

                        highest_subpass_index = current_subpass_index;
                        subpass_processed     = true;
                    }
                }
            }
        }

        (*unique_attachment_iterator)->highest_subpass_index  = highest_subpass_index;
        (*unique_attachment_iterator)->lowest_subpass_index   = lowest_subpass_index;
    }

    /* For each unique attachment, add it to the list of preserved attachments for all subpasses that precede the
     * one at the highest subpass index and follow the one at the lowest subpass index, as long as the
     * attachment is not used by a subpass. */
    for (std::vector<SubPassAttachment*>::iterator unique_attachment_iterator  = unique_attachments.begin();
                                                   unique_attachment_iterator != unique_attachments.end();
                                                 ++unique_attachment_iterator)
    {
        const SubPassAttachment* current_unique_attachment_ptr = *unique_attachment_iterator;

        if ((*unique_attachment_iterator)->highest_subpass_index == (*unique_attachment_iterator)->lowest_subpass_index)
        {
            /* There is only one producer subpass and no consumer subpasses defined for this renderpass.
             *No need to create any preserve attachments. */
            continue;
        }

        for (auto subpass_iterator  = (m_subpasses.begin() + static_cast<int>(current_unique_attachment_ptr->lowest_subpass_index) );
                  subpass_iterator != (m_subpasses.begin() + static_cast<int>(current_unique_attachment_ptr->highest_subpass_index) ) + 1;
                ++subpass_iterator)
        {
            SubPass* current_subpass_ptr   = subpass_iterator->get();
            bool     is_subpass_attachment = false;

            for (uint32_t n_attachment_type = 0;
                          n_attachment_type < 3 /* color, depth+stencil, resolve */ && !is_subpass_attachment;
                        ++n_attachment_type)
            {
                const uint32_t n_attachments = (n_attachment_type == 0) ? static_cast<uint32_t>(current_subpass_ptr->color_attachments_map.size() )
                                             : (n_attachment_type == 1) ? ((current_subpass_ptr->depth_stencil_attachment.attachment_index != UINT32_MAX) ? 1 : 0)
                                                                        : static_cast<uint32_t>(current_subpass_ptr->resolved_attachments_map.size() );

                for (uint32_t n_attachment = 0;
                              n_attachment < n_attachments && !is_subpass_attachment;
                            ++n_attachment)
                {
                    SubPassAttachment* current_attachment_ptr = (n_attachment_type == 0) ?  current_subpass_ptr->get_color_attachment_at_index(n_attachment)
                                                              : (n_attachment_type == 1) ? &current_subpass_ptr->depth_stencil_attachment
                                                                                         :  current_subpass_ptr->get_resolved_attachment_at_index(n_attachment);

                    if (current_attachment_ptr == current_unique_attachment_ptr)
                    {
                        is_subpass_attachment = true;
                    }
                }
            }

            if (!is_subpass_attachment)
            {
                #ifdef _DEBUG
                {
                    bool           is_already_preserved    = false;
                    const uint32_t n_preserved_attachments = static_cast<uint32_t>(current_subpass_ptr->preserved_attachments.size() );

                    for (uint32_t n_preserved_attachment = 0;
                                  n_preserved_attachment < n_preserved_attachments;
                                ++n_preserved_attachment)
                    {
                        if (&current_subpass_ptr->preserved_attachments[n_preserved_attachment] == current_unique_attachment_ptr)
                        {
                            is_already_preserved = true;

                            break;
                        }
                    }

                    anvil_assert(!is_already_preserved);
                }
                #endif

                current_subpass_ptr->preserved_attachments.push_back(*current_unique_attachment_ptr);
            }
        }
    }

    m_update_preserved_attachments = false;
}

/** Please see header for specification */
Anvil::RenderPassCreateInfo::SubPass::~SubPass()
{
    /* Stub */
}
