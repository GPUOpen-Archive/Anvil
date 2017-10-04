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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/device.h"
#include "wrappers/graphics_pipeline_manager.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/render_pass.h"
#include "wrappers/swapchain.h"
#include <algorithm>
#include <cmath>


/** Contsructor. Initializes the Renderpass instance with default values. */
Anvil::RenderPass::RenderPass(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
                              std::shared_ptr<Anvil::Swapchain> in_opt_swapchain_ptr)
    :CallbacksSupportProvider  (RENDER_PASS_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT),
     m_device_ptr              (in_device_ptr),
     m_dirty                   (false),
     m_render_pass             (VK_NULL_HANDLE),
     m_swapchain_ptr           (in_opt_swapchain_ptr)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_RENDER_PASS,
                                                  this);
}

/** Destructor. Will only be called by release() if the reference counter drops to zero. */
Anvil::RenderPass::~RenderPass()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_RENDER_PASS,
                                                    this);

    if (m_render_pass != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroyRenderPass(device_locked_ptr->get_device_vk(),
                            m_render_pass,
                            nullptr /* pAllocator */);

        m_render_pass = VK_NULL_HANDLE;
    }

    while (m_subpasses.size() > 0)
    {
        auto iterator = m_subpasses.begin();

        delete *iterator;

        m_subpasses.erase(iterator);
    }

    m_swapchain_ptr = nullptr;
}

/* Please see haeder for specification */
bool Anvil::RenderPass::add_color_attachment(VkFormat                in_format,
                                             VkSampleCountFlags      in_sample_count,
                                             VkAttachmentLoadOp      in_load_op,
                                             VkAttachmentStoreOp     in_store_op,
                                             VkImageLayout           in_initial_layout,
                                             VkImageLayout           in_final_layout,
                                             bool                    in_may_alias,
                                             RenderPassAttachmentID* out_attachment_id_ptr)
{
    uint32_t new_attachment_index = UINT32_MAX;
    bool     result               = false;

    if (out_attachment_id_ptr == nullptr)
    {
        anvil_assert(out_attachment_id_ptr != nullptr);

        goto end;
    }

    m_dirty               = true;
    new_attachment_index  = static_cast<uint32_t>(m_attachments.size() );

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

    /* Notify the subscribers about the event */
    callback(RENDER_PASS_CALLBACK_ID_BAKING_NEEDED,
             this);

end:
    return result;
}

/** Adds a new dependency to the internal data model.
 *
 *  This function does NOT re-create the internal VkRenderPass instance. Instead,
 *  it marks the RenderPass as dirty, which will cause the object to be re-created
 *  at next bake() or get_render_pass() request.
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
bool Anvil::RenderPass::add_dependency(SubPass*             in_destination_subpass_ptr,
                                       SubPass*             in_source_subpass_ptr,
                                       VkPipelineStageFlags in_source_stage_mask,
                                       VkPipelineStageFlags in_destination_stage_mask,
                                       VkAccessFlags        in_source_access_mask,
                                       VkAccessFlags        in_destination_access_mask,
                                       bool                 in_by_region)
{
    SubPassDependency new_dep(in_destination_stage_mask,
                              in_destination_subpass_ptr,
                              in_source_stage_mask,
                              in_source_subpass_ptr,
                              in_source_access_mask,
                              in_destination_access_mask,
                              in_by_region);

    if (std::find(m_subpass_dependencies.begin(),
                  m_subpass_dependencies.end(),
                  new_dep) == m_subpass_dependencies.end() )
    {
        m_subpass_dependencies.push_back(new_dep);

        m_dirty = true;

        /* Notify the subscribers about the event */
        callback(RENDER_PASS_CALLBACK_ID_BAKING_NEEDED,
                this);
    }

    return true;
}

/* Please see header for specification */
bool Anvil::RenderPass::add_depth_stencil_attachment(VkFormat                in_format,
                                                     VkSampleCountFlags      in_sample_count,
                                                     VkAttachmentLoadOp      in_depth_load_op,
                                                     VkAttachmentStoreOp     in_depth_store_op,
                                                     VkAttachmentLoadOp      in_stencil_load_op,
                                                     VkAttachmentStoreOp     in_stencil_store_op,
                                                     VkImageLayout           in_initial_layout,
                                                     VkImageLayout           in_final_layout,
                                                     bool                    in_may_alias,
                                                     RenderPassAttachmentID* out_attachment_id_ptr)
{
    uint32_t new_attachment_index = UINT32_MAX;
    bool     result               = false;

    if (out_attachment_id_ptr == nullptr)
    {
        anvil_assert(out_attachment_id_ptr != nullptr);

        goto end;
    }

    m_dirty              = true;
    new_attachment_index = static_cast<uint32_t>(m_attachments.size() );

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
bool Anvil::RenderPass::add_external_to_subpass_dependency(SubPassID            in_destination_subpass_id,
                                                           VkPipelineStageFlags in_source_stage_mask,
                                                           VkPipelineStageFlags in_destination_stage_mask,
                                                           VkAccessFlags        in_source_access_mask,
                                                           VkAccessFlags        in_destination_access_mask,
                                                           bool                 in_by_region)
{
    SubPass* destination_subpass_ptr = nullptr;
    bool     result                  = false;

    if (m_subpasses.size() <= in_destination_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_destination_subpass_id) );

        goto end;
    }

    destination_subpass_ptr = m_subpasses[in_destination_subpass_id];

    result = add_dependency(destination_subpass_ptr,
                            nullptr, /* source_subpass_ptr */
                            in_source_stage_mask,
                            in_destination_stage_mask,
                            in_source_access_mask,
                            in_destination_access_mask,
                            in_by_region);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPass::add_self_subpass_dependency(SubPassID            in_destination_subpass_id,
                                                    VkPipelineStageFlags in_source_stage_mask,
                                                    VkPipelineStageFlags in_destination_stage_mask,
                                                    VkAccessFlags        in_source_access_mask,
                                                    VkAccessFlags        in_destination_access_mask,
                                                    bool                 in_by_region)
{
    SubPass* destination_subpass_ptr = nullptr;
    bool     result                  = false;

    if (m_subpasses.size() <= in_destination_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_destination_subpass_id) );

        goto end;
    }

    destination_subpass_ptr = m_subpasses[in_destination_subpass_id];

    result = add_dependency(destination_subpass_ptr,
                            destination_subpass_ptr,
                            in_source_stage_mask,
                            in_destination_stage_mask,
                            in_source_access_mask,
                            in_destination_access_mask,
                            in_by_region);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPass::add_subpass(const ShaderModuleStageEntryPoint& in_fragment_shader_entrypoint,
                                    const ShaderModuleStageEntryPoint& in_geometry_shader_entrypoint,
                                    const ShaderModuleStageEntryPoint& in_tess_control_shader_entrypoint,
                                    const ShaderModuleStageEntryPoint& in_tess_evaluation_shader_entrypoint,
                                    const ShaderModuleStageEntryPoint& in_vertex_shader_entrypoint,
                                    SubPassID*                         out_subpass_id_ptr,
                                    const Anvil::PipelineID            in_opt_pipeline_id)
{
    uint32_t           new_subpass_index       = UINT32_MAX;
    GraphicsPipelineID new_subpass_pipeline_id = UINT32_MAX;
    bool               result                  = false;

    if (out_subpass_id_ptr == nullptr)
    {
        anvil_assert(out_subpass_id_ptr != nullptr);

        goto end;
    }

    m_dirty           = true;
    new_subpass_index = static_cast<uint32_t>(m_subpasses.size() );

    /* Create a new graphics pipeline for the subpass */
    anvil_assert(in_vertex_shader_entrypoint.name.size() != 0);

    if (in_opt_pipeline_id == UINT32_MAX)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        device_locked_ptr->get_graphics_pipeline_manager()->add_regular_pipeline(false, /* disable_optimizations */
                                                                                 false, /* allow_derivatives     */
                                                                                 shared_from_this(),
                                                                                 new_subpass_index,
                                                                                 in_fragment_shader_entrypoint,
                                                                                 in_geometry_shader_entrypoint,
                                                                                 in_tess_control_shader_entrypoint,
                                                                                 in_tess_evaluation_shader_entrypoint,
                                                                                 in_vertex_shader_entrypoint,
                                                                                &new_subpass_pipeline_id);
    }
    else
    {
        new_subpass_pipeline_id = in_opt_pipeline_id;
    }

    /* Spawn a new descriptor */
    {
        SubPass* new_subpass_ptr = new SubPass(m_device_ptr,
                                               new_subpass_index,
                                               new_subpass_pipeline_id);

        m_subpasses.push_back(new_subpass_ptr);

        *out_subpass_id_ptr = new_subpass_index;
    }

    result = true;
end:
    return result;
}

/** Adds a new attachment to the specified subpass.
 *
 *  This function does NOT re-create the internal VkRenderPass instance. Instead,
 *  it marks the RenderPass as dirty, which will cause the object to be re-created
 *  at next bake() or get_render_pass() request.
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
bool Anvil::RenderPass::add_subpass_attachment(SubPassID              in_subpass_id,
                                               bool                   in_is_color_attachment,
                                               VkImageLayout          in_layout,
                                               RenderPassAttachmentID in_attachment_id,
                                               uint32_t               in_attachment_location,
                                               bool                   in_should_resolve,
                                               RenderPassAttachmentID in_resolve_attachment_id)
{
    RenderPassAttachment*           renderpass_attachment_ptr = nullptr;
    RenderPassAttachment*           resolve_attachment_ptr    = nullptr;
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
        subpass_ptr = m_subpasses.at(in_subpass_id);
    }

    /* Retrieve the renderpass attachment descriptor */
    if (in_attachment_id >= m_attachments.size() )
    {
        anvil_assert(!(in_attachment_id >= m_attachments.size()) );

        goto end;
    }
    else
    {
        renderpass_attachment_ptr = &m_attachments.at(in_attachment_id);
    }

    /* Retrieve the resolve attachment descriptor, if one was requested */
    if (in_should_resolve)
    {
        if (in_resolve_attachment_id >= m_attachments.size() )
        {
            anvil_assert(!(in_resolve_attachment_id >= m_attachments.size()) );

            goto end;
        }
        else
        {
            resolve_attachment_ptr = &m_attachments.at(in_resolve_attachment_id);
        }
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
    (*subpass_attachments_ptr)[in_attachment_location] = SubPassAttachment(renderpass_attachment_ptr,
                                                                           in_layout,
                                                                           resolve_attachment_ptr);

    if (in_should_resolve)
    {
        anvil_assert(resolve_attachment_ptr != nullptr);

        subpass_ptr->resolved_attachments_map[in_attachment_location] = SubPassAttachment(resolve_attachment_ptr,
                                                                                          in_layout,
                                                                                          nullptr);
    }

    m_dirty = true;
    result  = true;

    /* Notify the subscribers about the event */
    callback(RENDER_PASS_CALLBACK_ID_BAKING_NEEDED,
             this);

end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPass::add_subpass_color_attachment(SubPassID                     in_subpass_id,
                                                     VkImageLayout                 in_input_layout,
                                                     RenderPassAttachmentID        in_attachment_id,
                                                     uint32_t                      in_location,
                                                     const RenderPassAttachmentID* in_attachment_resolve_id_ptr)
{
    return add_subpass_attachment(in_subpass_id,
                                  true, /* is_color_attachment */
                                  in_input_layout,
                                  in_attachment_id,
                                  in_location,
                                  (in_attachment_resolve_id_ptr != nullptr),
                                  (in_attachment_resolve_id_ptr != nullptr) ? *in_attachment_resolve_id_ptr
                                                                            : UINT32_MAX);
}

/* Please see header for specification */
bool Anvil::RenderPass::add_subpass_depth_stencil_attachment(SubPassID                     in_subpass_id,
                                                             RenderPassAttachmentID        in_attachment_id,
                                                             VkImageLayout                 in_layout)
{
    RenderPassAttachment* ds_attachment_ptr      = nullptr;
    RenderPassAttachment* resolve_attachment_ptr = nullptr;
    bool                  result                 = false;
    SubPass*              subpass_ptr            = nullptr;

    /* Retrieve the subpass descriptor */
    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }
    else
    {
        subpass_ptr = m_subpasses.at(in_subpass_id);
    }

    /* Retrieve the attachment descriptors */
    if (m_attachments.size() <= in_attachment_id)
    {
        anvil_assert(!(m_attachments.size() <= in_attachment_id) );

        goto end;
    }
    else
    {
        ds_attachment_ptr = &m_attachments.at(in_attachment_id);
    }

    /* Update the depth/stencil attachment for the subpass */
    if (subpass_ptr->depth_stencil_attachment.attachment_ptr != nullptr)
    {
        anvil_assert(!(subpass_ptr->depth_stencil_attachment.attachment_ptr != nullptr) );

        goto end;
    }

    subpass_ptr->depth_stencil_attachment = SubPassAttachment(ds_attachment_ptr,
                                                              in_layout,
                                                              resolve_attachment_ptr);

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPass::add_subpass_input_attachment(SubPassID              in_subpass_id,
                                                     VkImageLayout          in_layout,
                                                     RenderPassAttachmentID in_attachment_id,
                                                     uint32_t               in_attachment_index)
{
    return add_subpass_attachment(in_subpass_id,
                                  false, /* is_color_attachment */
                                  in_layout,
                                  in_attachment_id,
                                  in_attachment_index,
                                  false,         /* should_resolve        */
                                  UINT32_MAX);   /* resolve_attachment_id */
}

/* Please see header for specification */
bool Anvil::RenderPass::add_subpass_to_external_dependency(SubPassID            in_source_subpass_id,
                                                           VkPipelineStageFlags in_source_stage_mask,
                                                           VkPipelineStageFlags in_destination_stage_mask,
                                                           VkAccessFlags        in_source_access_mask,
                                                           VkAccessFlags        in_destination_access_mask,
                                                           bool                 in_by_region)
{
    SubPass* source_subpass_ptr = nullptr;
    bool     result             = false;

    if (m_subpasses.size() <= in_source_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_source_subpass_id) );

        goto end;
    }

    source_subpass_ptr = m_subpasses[in_source_subpass_id];

    result = add_dependency(nullptr, /* destination_subpass_ptr */
                            source_subpass_ptr,
                            in_source_stage_mask,
                            in_destination_stage_mask,
                            in_source_access_mask,
                            in_destination_access_mask,
                            in_by_region);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPass::add_subpass_to_subpass_dependency(SubPassID            in_source_subpass_id,
                                                          SubPassID            in_destination_subpass_id,
                                                          VkPipelineStageFlags in_source_stage_mask,
                                                          VkPipelineStageFlags in_destination_stage_mask,
                                                          VkAccessFlags        in_source_access_mask,
                                                          VkAccessFlags        in_destination_access_mask,
                                                          bool                 in_by_region)
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

    destination_subpass_ptr = m_subpasses[in_destination_subpass_id];
    source_subpass_ptr      = m_subpasses[in_source_subpass_id];

    result = add_dependency(destination_subpass_ptr,
                            source_subpass_ptr,
                            in_source_stage_mask,
                            in_destination_stage_mask,
                            in_source_access_mask,
                            in_destination_access_mask,
                            in_by_region);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPass::bake()
{
    std::shared_ptr<Anvil::BaseDevice>   device_locked_ptr         (m_device_ptr);
    std::vector<VkAttachmentDescription> renderpass_attachments_vk;
    VkRenderPassCreateInfo               render_pass_create_info;
    bool                                 result                    (false);
    VkResult                             result_vk;
    std::vector<VkSubpassDependency>     subpass_dependencies_vk;
    std::vector<VkSubpassDescription>    subpass_descriptions_vk;

    /* NOTE: We need to reserve storage in advance for each of the vectors below,
     *       so that it is guaranteed the push_back() calls do not cause a realloc()
     *       and invalidate already cached pointers to filled Vulkan descriptors.
     *       To achieve this, we could encapsulate the code below in a two-iteration loop,
     *       whose first iteration would count how many elements we need for each vector,
     *       and the second one would reserve that space and proceed with inserting the elements.
     *
     *       That would look ugly though.
     *
     *       In order to keep things clean & simple, we instantiate the following structure on heap
     *       for each subpass. On subpass level, we can easily predict how many elements in the worst
     *       case scenario we're going to insert, so that will do the trick. Slight performance cost,
     *       but baking is an offline task, so we should be OK.
     **/
    typedef struct SubPassAttachmentSet
    {
        /** Constructor.
         *
         *  @param in_n_max_color_attachments    Maximum number of color attachments the subpass will define.
         *  @param in_n_max_input_attachments    Maximum number of input attachments the subpass will define.
         *  @param in_n_max_preserve_attachments Maximum number of preserve attachments the subpass will define.
         **/
        explicit SubPassAttachmentSet(uint32_t in_n_max_color_attachments,
                                      uint32_t in_n_max_input_attachments,
                                      uint32_t in_n_max_preserve_attachments)
            :n_max_color_attachments   (in_n_max_color_attachments),
             n_max_input_attachments   (in_n_max_input_attachments),
             n_max_preserve_attachments(in_n_max_preserve_attachments)
        {
            color_attachments_vk.reserve        (n_max_color_attachments);
            input_attachments_vk.reserve        (n_max_input_attachments);
            preserve_attachments_vk.reserve     (n_max_preserve_attachments);
            resolve_color_attachments_vk.reserve(n_max_color_attachments);
        }

        /** Helper function which verifies the maximum number of attachments specified at
         *  creation time is not exceeded.
         **/
        void do_sanity_checks()
        {
            anvil_assert(color_attachments_vk.size()         <= n_max_color_attachments);
            anvil_assert(input_attachments_vk.size()         <= n_max_input_attachments);
            anvil_assert(preserve_attachments_vk.size()      <= n_max_preserve_attachments);
            anvil_assert(resolve_color_attachments_vk.size() <= n_max_color_attachments);
        }

        std::vector<VkAttachmentReference> color_attachments_vk;
        VkAttachmentReference              depth_attachment_vk;
        std::vector<VkAttachmentReference> input_attachments_vk;
        std::vector<uint32_t>              preserve_attachments_vk;
        std::vector<VkAttachmentReference> resolve_color_attachments_vk;
    private:
        uint32_t n_max_color_attachments;
        uint32_t n_max_input_attachments;
        uint32_t n_max_preserve_attachments;
    } SubPassAttachmentSet;

    std::vector<std::shared_ptr<SubPassAttachmentSet> > subpass_attachment_sets;

    /* If this is not first baking request, release the previously created render pass instance */
    if (m_render_pass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device_locked_ptr->get_device_vk(),
                            m_render_pass,
                            nullptr /* pAllocator */);

        m_render_pass = VK_NULL_HANDLE;
        set_vk_handle(VK_NULL_HANDLE);
    }

    /* Set up helper descriptor storage space */
    subpass_dependencies_vk.reserve(m_subpass_dependencies.size() );
    subpass_descriptions_vk.reserve(m_subpasses.size() );

    for (auto renderpass_attachment_iterator  = m_attachments.cbegin();
              renderpass_attachment_iterator != m_attachments.cend();
            ++renderpass_attachment_iterator)
    {
        VkAttachmentDescription attachment_vk;

        attachment_vk.finalLayout    = renderpass_attachment_iterator->final_layout;
        attachment_vk.flags          = (renderpass_attachment_iterator->may_alias) ? VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT
                                                                                   : 0u;
        attachment_vk.format         = renderpass_attachment_iterator->format;
        attachment_vk.initialLayout  = renderpass_attachment_iterator->initial_layout;
        attachment_vk.loadOp         = renderpass_attachment_iterator->color_depth_load_op;
        attachment_vk.samples        = static_cast<VkSampleCountFlagBits>(renderpass_attachment_iterator->sample_count);
        attachment_vk.stencilLoadOp  = renderpass_attachment_iterator->stencil_load_op;
        attachment_vk.stencilStoreOp = renderpass_attachment_iterator->stencil_store_op;
        attachment_vk.storeOp        = renderpass_attachment_iterator->color_depth_store_op;

        renderpass_attachments_vk.push_back(attachment_vk);
    }

    for (auto subpass_dependency_iterator  = m_subpass_dependencies.cbegin();
              subpass_dependency_iterator != m_subpass_dependencies.cend();
            ++subpass_dependency_iterator)
    {
        VkSubpassDependency dependency_vk;

        dependency_vk.dependencyFlags = ((subpass_dependency_iterator->by_region) ? VK_DEPENDENCY_BY_REGION_BIT : 0u);
        dependency_vk.dstAccessMask   = subpass_dependency_iterator->destination_access_mask;
        dependency_vk.dstStageMask    = subpass_dependency_iterator->destination_stage_mask;
        dependency_vk.dstSubpass      = (subpass_dependency_iterator->destination_subpass_ptr != nullptr) ? subpass_dependency_iterator->destination_subpass_ptr->index
                                                                                                         : VK_SUBPASS_EXTERNAL;
        dependency_vk.srcAccessMask   = subpass_dependency_iterator->source_access_mask;
        dependency_vk.srcStageMask    = subpass_dependency_iterator->source_stage_mask;
        dependency_vk.srcSubpass      = (subpass_dependency_iterator->source_subpass_ptr != nullptr) ? subpass_dependency_iterator->source_subpass_ptr->index
                                                                                                    : VK_SUBPASS_EXTERNAL;

        subpass_dependencies_vk.push_back(dependency_vk);
    }

    /* Update preserved attachments for all subpasses */
    update_preserved_attachments();

    /* We now have all the data needed to create Vulkan subpass instances. */
    for (auto subpass_iterator  = m_subpasses.cbegin();
              subpass_iterator != m_subpasses.cend();
            ++subpass_iterator)
    {
        std::shared_ptr<SubPassAttachmentSet> current_subpass_attachment_set_ptr;
        uint32_t                              highest_subpass_color_attachment_location = UINT32_MAX;
        uint32_t                              highest_subpass_input_attachment_index    = UINT32_MAX;
        bool                                  need_color_resolve_attachments            = false;
        VkSubpassDescription                  subpass_vk;
        VkAttachmentReference                 unused_reference;

        unused_reference.attachment = VK_ATTACHMENT_UNUSED;
        unused_reference.layout     = VK_IMAGE_LAYOUT_UNDEFINED;

        /* Determine whether any of the color attachments are going to be resolved. */
        for (auto subpass_color_attachment_iterator  = (*subpass_iterator)->color_attachments_map.cbegin();
                  subpass_color_attachment_iterator != (*subpass_iterator)->color_attachments_map.cend();
                ++subpass_color_attachment_iterator)
        {
            if (subpass_color_attachment_iterator->second.resolve_attachment_ptr != nullptr)
            {
                need_color_resolve_attachments = true;

                break;
            }
        }

        /* Determine the highest color attachment location & input attachment index. */
        for (auto subpass_color_attachment_iterator  = (*subpass_iterator)->color_attachments_map.cbegin();
                  subpass_color_attachment_iterator != (*subpass_iterator)->color_attachments_map.cend();
                ++subpass_color_attachment_iterator)
        {
            if (highest_subpass_color_attachment_location == UINT32_MAX                                ||
                subpass_color_attachment_iterator->first  > highest_subpass_color_attachment_location)
            {
                highest_subpass_color_attachment_location = subpass_color_attachment_iterator->first;
            }
        }

        for (auto subpass_input_attachment_iterator  = (*subpass_iterator)->input_attachments_map.cbegin();
                  subpass_input_attachment_iterator != (*subpass_iterator)->input_attachments_map.cend();
                ++subpass_input_attachment_iterator)
        {
            if (highest_subpass_input_attachment_index   == UINT32_MAX                               ||
                subpass_input_attachment_iterator->first >  highest_subpass_input_attachment_index)
            {
                highest_subpass_input_attachment_index = subpass_input_attachment_iterator->first;
            }
        }

        /* Instantiate a new subpass attachment set for current subpass */
        current_subpass_attachment_set_ptr.reset(
            new SubPassAttachmentSet(highest_subpass_color_attachment_location + 1,                             /* n_max_color_attachments     */
                                     static_cast<uint32_t>((*subpass_iterator)->input_attachments_map.size() ), /* n_max_input_attachments     */
                                     static_cast<uint32_t>((*subpass_iterator)->preserved_attachments.size() )  /* n_max_preserved_attachments */)
        );

        /* Prepare unused VK color, depth, input & resolve attachment descriptors */
        for (uint32_t n_color_attachment = 0;
                      n_color_attachment < static_cast<uint32_t>(highest_subpass_color_attachment_location + 1);
                    ++n_color_attachment)
        {
            current_subpass_attachment_set_ptr->color_attachments_vk.push_back(unused_reference);

            if (need_color_resolve_attachments)
            {
                current_subpass_attachment_set_ptr->resolve_color_attachments_vk.push_back(unused_reference);
            }
        }

        for (uint32_t n_input_attachment = 0;
                      n_input_attachment < static_cast<uint32_t>(highest_subpass_input_attachment_index + 1);
                    ++n_input_attachment)
        {
            current_subpass_attachment_set_ptr->input_attachments_vk.push_back(unused_reference);
        }

        /* Update those of the color/depth/input references, for which we have been provided actual descriptors */
        for (auto subpass_color_attachment_iterator  = (*subpass_iterator)->color_attachments_map.cbegin();
                  subpass_color_attachment_iterator != (*subpass_iterator)->color_attachments_map.cend();
                ++subpass_color_attachment_iterator)
        {
            current_subpass_attachment_set_ptr->color_attachments_vk[subpass_color_attachment_iterator->first] = get_attachment_reference_from_subpass_attachment(subpass_color_attachment_iterator->second);

            if (need_color_resolve_attachments)
            {
                if (subpass_color_attachment_iterator->second.resolve_attachment_ptr != nullptr)
                {
                    current_subpass_attachment_set_ptr->resolve_color_attachments_vk[subpass_color_attachment_iterator->first] = get_attachment_reference_for_resolve_attachment(subpass_iterator,
                                                                                                                                                                                 subpass_color_attachment_iterator);
                }
            }
        }

        if ((*subpass_iterator)->depth_stencil_attachment.attachment_ptr != nullptr)
        {
            current_subpass_attachment_set_ptr->depth_attachment_vk = get_attachment_reference_from_subpass_attachment((*subpass_iterator)->depth_stencil_attachment);
        }
        else
        {
            current_subpass_attachment_set_ptr->depth_attachment_vk = unused_reference;
        }

        for (auto subpass_input_attachment_iterator  = (*subpass_iterator)->input_attachments_map.cbegin();
                  subpass_input_attachment_iterator != (*subpass_iterator)->input_attachments_map.cend();
                ++subpass_input_attachment_iterator)
        {
            current_subpass_attachment_set_ptr->input_attachments_vk[subpass_input_attachment_iterator->first] = get_attachment_reference_from_subpass_attachment(subpass_input_attachment_iterator->second);
        }

        /* Fill the preserved attachments vector. These do not use indices or locations, so the process is much simpler */
        for (auto subpass_preserve_attachment_iterator  = (*subpass_iterator)->preserved_attachments.cbegin();
                  subpass_preserve_attachment_iterator != (*subpass_iterator)->preserved_attachments.cend();
                ++subpass_preserve_attachment_iterator)
        {
            current_subpass_attachment_set_ptr->preserve_attachments_vk.push_back(subpass_preserve_attachment_iterator->attachment_ptr->index);
        }

        /* Prepare the VK subpass descriptor */
        const uint32_t n_color_attachments     = highest_subpass_color_attachment_location + 1;
        const uint32_t n_input_attachments     = highest_subpass_input_attachment_index    + 1;
        const uint32_t n_preserved_attachments = static_cast<uint32_t>((*subpass_iterator)->preserved_attachments.size() );
        const uint32_t n_resolved_attachments  = ((*subpass_iterator)->resolved_attachments_map.size() == 0) ? 0
                                                                                                             : n_color_attachments;

        subpass_vk.colorAttachmentCount              = n_color_attachments;
        subpass_vk.flags                             = 0;
        subpass_vk.inputAttachmentCount              = n_input_attachments;
        subpass_vk.pColorAttachments                 = (n_color_attachments > 0)                                                 ? &current_subpass_attachment_set_ptr->color_attachments_vk.at(0)
                                                                                                                                 : nullptr;
        subpass_vk.pDepthStencilAttachment           = ((*subpass_iterator)->depth_stencil_attachment.attachment_ptr != nullptr) ? &current_subpass_attachment_set_ptr->depth_attachment_vk
                                                                                                                                 : nullptr;
        subpass_vk.pInputAttachments                 = (n_input_attachments > 0)                                                 ? &current_subpass_attachment_set_ptr->input_attachments_vk.at(0)
                                                                                                                                 : nullptr;
        subpass_vk.pipelineBindPoint                 = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_vk.pPreserveAttachments              = (n_preserved_attachments > 0) ? &current_subpass_attachment_set_ptr->preserve_attachments_vk.at(0)
                                                                                     : nullptr;
        subpass_vk.preserveAttachmentCount           = n_preserved_attachments;
        subpass_vk.pResolveAttachments               = (n_resolved_attachments > 0) ? &current_subpass_attachment_set_ptr->resolve_color_attachments_vk.at(0)
                                                                                    : nullptr;

        current_subpass_attachment_set_ptr->do_sanity_checks();

        subpass_attachment_sets.push_back(current_subpass_attachment_set_ptr);
        subpass_descriptions_vk.push_back(subpass_vk);
    }

    /* Set up a create info descriptor and spawn a new Vulkan RenderPass object. */
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(m_attachments.size() );
    render_pass_create_info.dependencyCount = static_cast<uint32_t>(m_subpass_dependencies.size() );
    render_pass_create_info.subpassCount    = static_cast<uint32_t>(m_subpasses.size() );
    render_pass_create_info.flags           = 0;
    render_pass_create_info.pAttachments    = (render_pass_create_info.attachmentCount > 0) ? &renderpass_attachments_vk.at(0)
                                                                                            : nullptr;
    render_pass_create_info.pDependencies   = (render_pass_create_info.dependencyCount > 0) ? &subpass_dependencies_vk.at(0)
                                                                                            : nullptr;
    render_pass_create_info.pNext           = nullptr;
    render_pass_create_info.pSubpasses      = (render_pass_create_info.subpassCount > 0) ? &subpass_descriptions_vk.at(0)
                                                                                         : nullptr;
    render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    result_vk = vkCreateRenderPass(device_locked_ptr->get_device_vk(),
                                  &render_pass_create_info,
                                   nullptr, /* pAllocator */
                                  &m_render_pass);

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert_vk_call_succeeded(result_vk);

        goto end;
    }

    set_vk_handle(m_render_pass);

    m_dirty = false;
    result  = true;

end:
    return result;
}

/* Please see header for specification */
std::shared_ptr<Anvil::RenderPass> Anvil::RenderPass::create(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
                                                             std::shared_ptr<Anvil::Swapchain> in_opt_swapchain_ptr)
{
    std::shared_ptr<RenderPass> result_ptr;

    result_ptr.reset(
        new Anvil::RenderPass(in_device_ptr,
                              in_opt_swapchain_ptr)
    );

    return result_ptr;
}

/** Creates a VkAttachmentReference descriptor from the specified RenderPassAttachment instance.
 *
 *  @param in_renderpass_attachment Renderpass attachment descriptor to create the Vulkan descriptor from.
 *
 *  @return As per description.
 **/
VkAttachmentReference Anvil::RenderPass::get_attachment_reference_from_renderpass_attachment(const RenderPassAttachment& in_renderpass_attachment) const
{
    VkAttachmentReference attachment_vk;

    attachment_vk.attachment = in_renderpass_attachment.index;
    attachment_vk.layout     = in_renderpass_attachment.initial_layout;

    return attachment_vk;
}

/** Creates a VkAttachmentReference descriptor from the specified SubPassAttachment instance.
 *
 *  @param in_renderpass_attachment Subpass attachment descriptor to create the Vulkan descriptor from.
 *
 *  @return As per description.
 **/
VkAttachmentReference Anvil::RenderPass::get_attachment_reference_from_subpass_attachment(const SubPassAttachment& in_subpass_attachment) const
{
    VkAttachmentReference attachment_vk;

    attachment_vk.attachment = in_subpass_attachment.attachment_ptr->index;
    attachment_vk.layout     = in_subpass_attachment.layout;

    return attachment_vk;
}

/** Creates a VkAttachmentReference descriptor for a resolve attachment for a color attachment specified by the user.
 *
 *  @param in_subpass_iterator                     Iterator pointing at the subpass which uses the color attachment of interest.
 *  @param in_location_to_subpass_att_map_iterator Iterator pointing at a color attachment which has a resolve attachment attached.
 *
 *  @return As per description.
 **/
VkAttachmentReference Anvil::RenderPass::get_attachment_reference_for_resolve_attachment(const SubPassesConstIterator&                      in_subpass_iterator,
                                                                                         const LocationToSubPassAttachmentMapConstIterator& in_location_to_subpass_att_map_iterator) const
{
    VkAttachmentReference result;

    anvil_assert((*in_subpass_iterator)->resolved_attachments_map.find(in_location_to_subpass_att_map_iterator->first) != (*in_subpass_iterator)->resolved_attachments_map.end() );

    result.attachment = in_location_to_subpass_att_map_iterator->second.resolve_attachment_ptr->index;
    result.layout     = (*in_subpass_iterator)->resolved_attachments_map.at(in_location_to_subpass_att_map_iterator->first).layout;

    return result;
}

/* Please see header for specification */
bool Anvil::RenderPass::get_attachment_type(RenderPassAttachmentID in_attachment_id,
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
bool Anvil::RenderPass::get_color_attachment_properties(RenderPassAttachmentID in_attachment_id,
                                                        VkSampleCountFlagBits* out_opt_sample_count_ptr,
                                                        VkAttachmentLoadOp*    out_opt_load_op_ptr,
                                                        VkAttachmentStoreOp*   out_opt_store_op_ptr,
                                                        VkImageLayout*         out_opt_initial_layout_ptr,
                                                        VkImageLayout*         out_opt_final_layout_ptr,
                                                        bool*                  out_opt_may_alias_ptr) const
{
    bool result = false;

    if (m_attachments.size() <= in_attachment_id)
    {
        goto end;
    }

    if (out_opt_sample_count_ptr != nullptr)
    {
        *out_opt_sample_count_ptr = static_cast<VkSampleCountFlagBits>(m_attachments[in_attachment_id].sample_count);
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
bool Anvil::RenderPass::get_dependency_properties(uint32_t              in_n_dependency,
                                                  SubPassID*            out_destination_subpass_id_ptr,
                                                  SubPassID*            out_source_subpass_id_ptr,
                                                  VkPipelineStageFlags* out_destination_stage_mask_ptr,
                                                  VkPipelineStageFlags* out_source_stage_mask_ptr,
                                                  VkAccessFlags*        out_destination_access_mask_ptr,
                                                  VkAccessFlags*        out_source_access_mask_ptr,
                                                  bool*                 out_by_region_ptr) const
{
    const Anvil::RenderPass::SubPassDependency* dep_ptr = nullptr;
    bool                                        result  = false;

    if (m_subpass_dependencies.size() <= in_n_dependency)
    {
        anvil_assert_fail();

        goto end;
    }

    dep_ptr = &m_subpass_dependencies[in_n_dependency];

    *out_destination_subpass_id_ptr  = (dep_ptr->destination_subpass_ptr != nullptr) ? dep_ptr->destination_subpass_ptr->index
                                                                                     : UINT32_MAX;
    *out_source_subpass_id_ptr       = (dep_ptr->source_subpass_ptr      != nullptr) ? dep_ptr->source_subpass_ptr->index
                                                                                     : UINT32_MAX;
    *out_destination_stage_mask_ptr  = dep_ptr->destination_stage_mask;
    *out_source_stage_mask_ptr       = dep_ptr->source_stage_mask;
    *out_destination_access_mask_ptr = dep_ptr->destination_access_mask;
    *out_source_access_mask_ptr      = dep_ptr->source_access_mask;
    *out_by_region_ptr               = dep_ptr->by_region;

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::RenderPass::get_depth_stencil_attachment_properties(RenderPassAttachmentID in_attachment_id,
                                                                VkAttachmentLoadOp*    out_opt_depth_load_op_ptr,
                                                                VkAttachmentStoreOp*   out_opt_depth_store_op_ptr,
                                                                VkAttachmentLoadOp*    out_opt_stencil_load_op_ptr,
                                                                VkAttachmentStoreOp*   out_opt_stencil_store_op_ptr,
                                                                VkImageLayout*         out_opt_initial_layout_ptr,
                                                                VkImageLayout*         out_opt_final_layout_ptr,
                                                                bool*                  out_opt_may_alias_ptr) const
{bool result = false;

    if (m_attachments.size() <= in_attachment_id)
    {
        goto end;
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
VkRenderPass Anvil::RenderPass::get_render_pass(bool allow_rebaking)
{
    VkRenderPass result = VK_NULL_HANDLE;

    if (m_dirty && allow_rebaking)
    {
        bake();

        anvil_assert(!m_dirty);
    }

    result = m_render_pass;

    anvil_assert(result != VK_NULL_HANDLE);
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPass::get_subpass_graphics_pipeline_id(SubPassID           in_subpass_id,
                                                         GraphicsPipelineID* out_graphics_pipeline_id_ptr) const
{
    bool result = false;

    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }
    else
    {
        *out_graphics_pipeline_id_ptr = m_subpasses[in_subpass_id]->pipeline_id;
        result                        = true;
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPass::get_subpass_n_attachments(SubPassID      in_subpass_id,
                                                  AttachmentType in_attachment_type,
                                                  uint32_t*      out_n_attachments_ptr)
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

        if (in_attachment_type == ATTACHMENT_TYPE_PRESERVE && m_dirty)
        {
            bake();

            anvil_assert(!m_dirty);
        }

        switch (in_attachment_type)
        {
            case ATTACHMENT_TYPE_COLOR:    *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->color_attachments_map.size() );    break;
            case ATTACHMENT_TYPE_INPUT:    *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->input_attachments_map.size() );    break;
            case ATTACHMENT_TYPE_PRESERVE: *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->preserved_attachments.size() );    break;
            case ATTACHMENT_TYPE_RESOLVE:  *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->resolved_attachments_map.size() ); break;

            case ATTACHMENT_TYPE_DEPTH_STENCIL:
            {
                *out_n_attachments_ptr = (m_subpasses[in_subpass_id]->depth_stencil_attachment.attachment_ptr != nullptr) ? 1u : 0u;

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
bool Anvil::RenderPass::get_subpass_attachment_properties(SubPassID               in_subpass_id,
                                                          AttachmentType          in_attachment_type,
                                                          uint32_t                in_n_subpass_attachment,
                                                          RenderPassAttachmentID* out_renderpass_attachment_id_ptr,
                                                          VkImageLayout*          out_layout_ptr)
{
    SubPassAttachment               attachment;
    bool                            result                  = false;
    LocationToSubPassAttachmentMap* subpass_attachments_ptr = nullptr;
    SubPass*                        subpass_ptr             = nullptr;

    /* Sanity checks */
    if (in_attachment_type == ATTACHMENT_TYPE_PRESERVE &&
        out_layout_ptr     != nullptr)
    {
        anvil_assert(!(in_attachment_type == ATTACHMENT_TYPE_PRESERVE &&
                       out_layout_ptr     != nullptr) );

        goto end;
    }

    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }

    /* Bake the renderpass if needed.. */
    subpass_ptr = m_subpasses[in_subpass_id];

    if (m_dirty)
    {
        bake();

        if (m_dirty)
        {
            anvil_assert(!m_dirty);

            goto end;
        }
    }

    /* Even more sanity checks.. */
    switch (in_attachment_type)
    {
        case ATTACHMENT_TYPE_COLOR:   /* Fall-through */
        case ATTACHMENT_TYPE_INPUT:   /* Fall-through */
        case ATTACHMENT_TYPE_RESOLVE:
        {
            subpass_attachments_ptr = (in_attachment_type == ATTACHMENT_TYPE_COLOR) ? &subpass_ptr->color_attachments_map
                                    : (in_attachment_type == ATTACHMENT_TYPE_INPUT) ? &subpass_ptr->input_attachments_map
                                                                                    : &subpass_ptr->resolved_attachments_map;

            auto iterator = subpass_attachments_ptr->find(in_n_subpass_attachment);

            if (iterator == subpass_attachments_ptr->end() )
            {
                anvil_assert_fail();

                goto end;
            }

            *out_layout_ptr                   = iterator->second.layout;
            *out_renderpass_attachment_id_ptr = iterator->second.attachment_ptr->index;

            break;
        }

        case ATTACHMENT_TYPE_DEPTH_STENCIL:
        {
            if (in_n_subpass_attachment > 0)
            {
                anvil_assert(in_n_subpass_attachment == 0);

                goto end;
            }

            *out_layout_ptr                   = subpass_ptr->depth_stencil_attachment.layout;
            *out_renderpass_attachment_id_ptr = subpass_ptr->depth_stencil_attachment.attachment_ptr->index;

            break;
        }

        case ATTACHMENT_TYPE_PRESERVE:
        {
            if (subpass_ptr->preserved_attachments.size() <= in_n_subpass_attachment)
            {
                anvil_assert(subpass_ptr->preserved_attachments.size() > in_n_subpass_attachment);

                goto end;
            }

            const auto& attachment_props = subpass_ptr->preserved_attachments[in_n_subpass_attachment];

            *out_renderpass_attachment_id_ptr = attachment_props.attachment_ptr->index;
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
bool Anvil::RenderPass::set_subpass_graphics_pipeline_id(SubPassID          in_subpass_id,
                                                         GraphicsPipelineID new_graphics_pipeline_id)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    bool                               result = false;

    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }

    if (in_subpass_id == m_subpasses[in_subpass_id]->pipeline_id)
    {
        goto end;
    }

    /* Remove the former graphics pipeline. */
    if (!device_locked_ptr->get_graphics_pipeline_manager()->delete_pipeline(m_subpasses[in_subpass_id]->pipeline_id) )
    {
        anvil_assert_fail();
    }

    m_subpasses[in_subpass_id]->pipeline_id = new_graphics_pipeline_id;

    m_dirty = true;
    result  = true;

    /* Notify the subscribers about the event */
    callback(RENDER_PASS_CALLBACK_ID_BAKING_NEEDED,
             this);

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
void Anvil::RenderPass::update_preserved_attachments()
{
    /* Cache color, depth+stencil, resolve attachments, as used by all defined subpasses.
     * Make sure not to insert duplicate items. */
    std::vector<SubPassAttachment*> unique_attachments;

    for (auto subpass_iterator  = m_subpasses.begin();
              subpass_iterator != m_subpasses.end();
            ++subpass_iterator)
    {
        SubPass* current_subpass_ptr = *subpass_iterator;

        for (uint32_t n_attachment_type = 0;
                      n_attachment_type < 3; /* color, depth+stencil, resolve */
                    ++n_attachment_type)
        {
            const uint32_t n_attachments = (n_attachment_type == 0) ? static_cast<uint32_t>(current_subpass_ptr->color_attachments_map.size() )
                                         : (n_attachment_type == 1) ? ((current_subpass_ptr->depth_stencil_attachment.attachment_ptr != nullptr) ? 1 : 0)
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
            SubPass* current_subpass_ptr = *subpass_iterator;
            bool     subpass_processed   = false;

            for (uint32_t n_attachment_type = 0;
                          n_attachment_type < 3 /* color, depth+stencil, resolve */ && !subpass_processed;
                        ++n_attachment_type)
            {
                const uint32_t n_attachments = (n_attachment_type == 0) ? static_cast<uint32_t>(current_subpass_ptr->color_attachments_map.size() )
                                             : (n_attachment_type == 1) ? ((current_subpass_ptr->depth_stencil_attachment.attachment_ptr != nullptr) ? 1 : 0)
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
            SubPass* current_subpass_ptr   = *subpass_iterator;
            bool     is_subpass_attachment = false;

            for (uint32_t n_attachment_type = 0;
                          n_attachment_type < 3 /* color, depth+stencil, resolve */ && !is_subpass_attachment;
                        ++n_attachment_type)
            {
                const uint32_t n_attachments = (n_attachment_type == 0) ? static_cast<uint32_t>(current_subpass_ptr->color_attachments_map.size() )
                                             : (n_attachment_type == 1) ? ((current_subpass_ptr->depth_stencil_attachment.attachment_ptr != nullptr) ? 1 : 0)
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
}

/** Please see header for specification */
Anvil::RenderPass::SubPass::~SubPass()
{
    if (pipeline_id != UINT32_MAX)
    {
        std::shared_ptr<Anvil::BaseDevice>            device_locked_ptr       (device_ptr);
        std::weak_ptr<Anvil::GraphicsPipelineManager> gfx_pipeline_manager_ptr(device_locked_ptr->get_graphics_pipeline_manager() );

        /* If this subpass is being automatically destroyed as a part of gfx pipeline manager destruction, the weak pointer
         * will have become expired. Since pipelines are automatically destroyed as well, only delete the pipeline, if the
         * manager is still around. */
        if (!gfx_pipeline_manager_ptr.expired() )
        {
            gfx_pipeline_manager_ptr.lock()->delete_pipeline(pipeline_id);
        }
    }
}
