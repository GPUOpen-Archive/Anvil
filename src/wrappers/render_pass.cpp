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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "misc/render_pass_create_info.h"
#include "misc/struct_chainer.h"
#include "wrappers/device.h"
#include "wrappers/graphics_pipeline_manager.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/render_pass.h"
#include "wrappers/swapchain.h"


/** Contsructor. Initializes the Renderpass instance with default values. */
Anvil::RenderPass::RenderPass(Anvil::RenderPassCreateInfoUniquePtr in_renderpass_create_info_ptr,
                              Anvil::Swapchain*                    in_opt_swapchain_ptr)
    :DebugMarkerSupportProvider   (in_renderpass_create_info_ptr->get_device(),
                                   VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT),
     m_render_pass                (VK_NULL_HANDLE),
     m_render_pass_create_info_ptr(std::move(in_renderpass_create_info_ptr) ),
     m_swapchain_ptr              (in_opt_swapchain_ptr)
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
        Anvil::Vulkan::vkDestroyRenderPass(m_render_pass_create_info_ptr->get_device()->get_device_vk(),
                                           m_render_pass,
                                           nullptr /* pAllocator */);

        m_render_pass = VK_NULL_HANDLE;
    }

    m_swapchain_ptr = nullptr;
}

/* Please see header for specification */
bool Anvil::RenderPass::init()
{
    std::vector<std::unique_ptr<VkInputAttachmentAspectReferenceKHR> > input_attachment_aspect_reference_ptrs;
    std::unique_ptr<std::vector<uint32_t> >                            multiview_view_mask_vec_ptr;
    std::unique_ptr<std::vector<int32_t> >                             multiview_view_offset_vec_ptr;
    std::vector<VkAttachmentDescription>                               renderpass_attachments_vk;
    Anvil::StructChainer<VkRenderPassCreateInfo>                       render_pass_create_info_chainer;
    bool                                                               result                                (false);
    VkResult                                                           result_vk;
    std::vector<VkSubpassDependency>                                   subpass_dependencies_vk;
    std::vector<VkSubpassDescription>                                  subpass_descriptions_vk;

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

    std::vector<std::unique_ptr<SubPassAttachmentSet> > subpass_attachment_sets;

    anvil_assert(m_render_pass == VK_NULL_HANDLE);

    if (m_render_pass_create_info_ptr->is_multiview_enabled() )
    {
        if (!m_device_ptr->get_extension_info()->khr_multiview() )
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_multiview() );

            goto end;
        }
    }

    /* Set up helper descriptor storage space */
    subpass_dependencies_vk.reserve(m_render_pass_create_info_ptr->m_subpass_dependencies.size() );
    subpass_descriptions_vk.reserve(m_render_pass_create_info_ptr->m_subpasses.size() );

    for (auto renderpass_attachment_iterator  = m_render_pass_create_info_ptr->m_attachments.cbegin();
              renderpass_attachment_iterator != m_render_pass_create_info_ptr->m_attachments.cend();
            ++renderpass_attachment_iterator)
    {
        VkAttachmentDescription attachment_vk;

        attachment_vk.finalLayout    = static_cast<VkImageLayout>(renderpass_attachment_iterator->final_layout);
        attachment_vk.flags          = (renderpass_attachment_iterator->may_alias) ? VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT
                                                                                   : 0u;
        attachment_vk.format         = static_cast<VkFormat>             (renderpass_attachment_iterator->format);
        attachment_vk.initialLayout  = static_cast<VkImageLayout>        (renderpass_attachment_iterator->initial_layout);
        attachment_vk.loadOp         = static_cast<VkAttachmentLoadOp>   (renderpass_attachment_iterator->color_depth_load_op);
        attachment_vk.samples        = static_cast<VkSampleCountFlagBits>(renderpass_attachment_iterator->sample_count);
        attachment_vk.stencilLoadOp  = static_cast<VkAttachmentLoadOp>   (renderpass_attachment_iterator->stencil_load_op);
        attachment_vk.stencilStoreOp = static_cast<VkAttachmentStoreOp>  (renderpass_attachment_iterator->stencil_store_op);
        attachment_vk.storeOp        = static_cast<VkAttachmentStoreOp>  (renderpass_attachment_iterator->color_depth_store_op);

        renderpass_attachments_vk.push_back(attachment_vk);
    }

    for (auto subpass_dependency_iterator  = m_render_pass_create_info_ptr->m_subpass_dependencies.cbegin();
              subpass_dependency_iterator != m_render_pass_create_info_ptr->m_subpass_dependencies.cend();
            ++subpass_dependency_iterator)
    {
        VkSubpassDependency dependency_vk;

        dependency_vk.dependencyFlags = subpass_dependency_iterator->flags.get_vk();
        dependency_vk.dstAccessMask   = subpass_dependency_iterator->destination_access_mask.get_vk();
        dependency_vk.dstStageMask    = subpass_dependency_iterator->destination_stage_mask.get_vk ();
        dependency_vk.dstSubpass      = (subpass_dependency_iterator->destination_subpass_ptr != nullptr) ? subpass_dependency_iterator->destination_subpass_ptr->index
                                                                                                         : VK_SUBPASS_EXTERNAL;
        dependency_vk.srcAccessMask   = subpass_dependency_iterator->source_access_mask.get_vk();
        dependency_vk.srcStageMask    = subpass_dependency_iterator->source_stage_mask.get_vk ();
        dependency_vk.srcSubpass      = (subpass_dependency_iterator->source_subpass_ptr != nullptr) ? subpass_dependency_iterator->source_subpass_ptr->index
                                                                                                    : VK_SUBPASS_EXTERNAL;

        #if defined(_DEBUG)
        {
            if (m_render_pass_create_info_ptr->is_multiview_enabled() )
            {
                if (dependency_vk.dstSubpass == dependency_vk.srcSubpass &&
                    dependency_vk.dstSubpass != VK_SUBPASS_EXTERNAL)
                {
                    uint32_t n_views_active = 0;
                    uint32_t view_mask      = 0;

                    m_render_pass_create_info_ptr->get_subpass_view_mask(dependency_vk.dstSubpass,
                                                                        &view_mask);

                    n_views_active = Anvil::Utils::count_set_bits(view_mask);

                    anvil_assert( (n_views_active <= 1)                                                                         ||
                                 ((n_views_active >  1 && (dependency_vk.dependencyFlags & VK_DEPENDENCY_VIEW_LOCAL_BIT) != 0)) );
                }
            }
        }
        #endif

        subpass_dependencies_vk.push_back(dependency_vk);
    }

    /* We now have all the data needed to create Vulkan subpass instances. */
    for (auto subpass_iterator  = m_render_pass_create_info_ptr->m_subpasses.cbegin();
              subpass_iterator != m_render_pass_create_info_ptr->m_subpasses.cend();
            ++subpass_iterator)
    {
        std::unique_ptr<SubPassAttachmentSet> current_subpass_attachment_set_ptr;
        uint32_t                              highest_subpass_color_attachment_location = UINT32_MAX;
        uint32_t                              highest_subpass_input_attachment_index    = UINT32_MAX;
        bool                                  need_color_resolve_attachments            = false;
        VkSubpassDescription                  subpass_vk;
        VkAttachmentReference                 unused_reference;

        unused_reference.attachment = VK_ATTACHMENT_UNUSED;
        unused_reference.layout     = static_cast<VkImageLayout>(Anvil::ImageLayout::UNDEFINED);

        /* Determine whether any of the color attachments are going to be resolved. */
        for (auto subpass_color_attachment_iterator  = (*subpass_iterator)->color_attachments_map.cbegin();
                  subpass_color_attachment_iterator != (*subpass_iterator)->color_attachments_map.cend();
                ++subpass_color_attachment_iterator)
        {
            if (subpass_color_attachment_iterator->second.resolve_attachment_index != UINT32_MAX)
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
            current_subpass_attachment_set_ptr->color_attachments_vk[subpass_color_attachment_iterator->first] = m_render_pass_create_info_ptr->get_attachment_reference_from_subpass_attachment(subpass_color_attachment_iterator->second);

            if (need_color_resolve_attachments)
            {
                if (subpass_color_attachment_iterator->second.resolve_attachment_index != UINT32_MAX)
                {
                    current_subpass_attachment_set_ptr->resolve_color_attachments_vk[subpass_color_attachment_iterator->first] = m_render_pass_create_info_ptr->get_attachment_reference_for_resolve_attachment(subpass_iterator,
                                                                                                                                                                                                                subpass_color_attachment_iterator);
                }
            }
        }

        if ((*subpass_iterator)->depth_stencil_attachment.attachment_index != UINT32_MAX)
        {
            current_subpass_attachment_set_ptr->depth_attachment_vk = m_render_pass_create_info_ptr->get_attachment_reference_from_subpass_attachment((*subpass_iterator)->depth_stencil_attachment);
        }
        else
        {
            current_subpass_attachment_set_ptr->depth_attachment_vk = unused_reference;
        }

        for (auto subpass_input_attachment_iterator  = (*subpass_iterator)->input_attachments_map.cbegin();
                  subpass_input_attachment_iterator != (*subpass_iterator)->input_attachments_map.cend();
                ++subpass_input_attachment_iterator)
        {
            current_subpass_attachment_set_ptr->input_attachments_vk[subpass_input_attachment_iterator->first] = m_render_pass_create_info_ptr->get_attachment_reference_from_subpass_attachment(subpass_input_attachment_iterator->second);
        }

        /* Fill the preserved attachments vector. These do not use indices or locations, so the process is much simpler */
        for (auto subpass_preserve_attachment_iterator  = (*subpass_iterator)->preserved_attachments.cbegin();
                  subpass_preserve_attachment_iterator != (*subpass_iterator)->preserved_attachments.cend();
                ++subpass_preserve_attachment_iterator)
        {
            current_subpass_attachment_set_ptr->preserve_attachments_vk.push_back(
                m_render_pass_create_info_ptr->m_attachments.at(subpass_preserve_attachment_iterator->attachment_index).index
            );
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
        subpass_vk.pColorAttachments                 = (n_color_attachments > 0)                                                      ? &current_subpass_attachment_set_ptr->color_attachments_vk.at(0)
                                                                                                                                      : nullptr;
        subpass_vk.pDepthStencilAttachment           = ((*subpass_iterator)->depth_stencil_attachment.attachment_index != UINT32_MAX) ? &current_subpass_attachment_set_ptr->depth_attachment_vk
                                                                                                                                      : nullptr;
        subpass_vk.pInputAttachments                 = (n_input_attachments > 0)                                                      ? &current_subpass_attachment_set_ptr->input_attachments_vk.at(0)
                                                                                                                                      : nullptr;
        subpass_vk.pipelineBindPoint                 = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_vk.pPreserveAttachments              = (n_preserved_attachments > 0) ? &current_subpass_attachment_set_ptr->preserve_attachments_vk.at(0)
                                                                                     : nullptr;
        subpass_vk.preserveAttachmentCount           = n_preserved_attachments;
        subpass_vk.pResolveAttachments               = (n_resolved_attachments > 0) ? &current_subpass_attachment_set_ptr->resolve_color_attachments_vk.at(0)
                                                                                    : nullptr;

        current_subpass_attachment_set_ptr->do_sanity_checks();

        subpass_attachment_sets.push_back(
            std::move(current_subpass_attachment_set_ptr)
        );

        subpass_descriptions_vk.push_back(subpass_vk);
    }

    /* Set up a create info descriptor and spawn a new Vulkan RenderPass object. */
    {
        VkRenderPassCreateInfo render_pass_create_info;

        render_pass_create_info.attachmentCount = static_cast<uint32_t>(m_render_pass_create_info_ptr->m_attachments.size         () );
        render_pass_create_info.dependencyCount = static_cast<uint32_t>(m_render_pass_create_info_ptr->m_subpass_dependencies.size() );
        render_pass_create_info.subpassCount    = static_cast<uint32_t>(m_render_pass_create_info_ptr->m_subpasses.size           () );
        render_pass_create_info.flags           = 0;
        render_pass_create_info.pAttachments    = (render_pass_create_info.attachmentCount > 0) ? &renderpass_attachments_vk.at(0)
                                                                                                : nullptr;
        render_pass_create_info.pDependencies   = (render_pass_create_info.dependencyCount > 0) ? &subpass_dependencies_vk.at(0)
                                                                                                : nullptr;
        render_pass_create_info.pNext           = nullptr;
        render_pass_create_info.pSubpasses      = (render_pass_create_info.subpassCount > 0) ? &subpass_descriptions_vk.at(0)
                                                                                             : nullptr;
        render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        render_pass_create_info_chainer.append_struct(render_pass_create_info);
    }

    /* Also chain info regarding aspects accessed by input attachments, if KHR_maintenance2 is supported and relevant info
     * has been provided at attachment addition time.
     */
    if (m_device_ptr->is_extension_enabled(VK_KHR_MAINTENANCE2_EXTENSION_NAME) )
    {
        for (auto subpass_iterator  = m_render_pass_create_info_ptr->m_subpasses.cbegin();
                  subpass_iterator != m_render_pass_create_info_ptr->m_subpasses.cend();
                ++subpass_iterator)
        {
            const auto current_subpass_index = (*subpass_iterator)->index;

            for (auto subpass_input_attachment_iterator  = (*subpass_iterator)->input_attachments_map.cbegin();
                      subpass_input_attachment_iterator != (*subpass_iterator)->input_attachments_map.cend();
                    ++subpass_input_attachment_iterator)
            {
                const auto& current_attachment_accessed_aspects = subpass_input_attachment_iterator->second.aspects_accessed;
                const auto& current_attachment_index            = subpass_input_attachment_iterator->first;

                if (current_attachment_accessed_aspects != Anvil::ImageAspectFlagBits::NONE)
                {
                    std::unique_ptr<VkInputAttachmentAspectReferenceKHR> attachment_aspect_reference_info_ptr(new VkInputAttachmentAspectReferenceKHR() );

                    attachment_aspect_reference_info_ptr->aspectMask           = current_attachment_accessed_aspects.get_vk();
                    attachment_aspect_reference_info_ptr->inputAttachmentIndex = current_attachment_index;
                    attachment_aspect_reference_info_ptr->subpass              = current_subpass_index;

                    input_attachment_aspect_reference_ptrs.push_back( std::move(attachment_aspect_reference_info_ptr) );
                }
            }
        }

        if (input_attachment_aspect_reference_ptrs.size() > 0)
        {
            VkRenderPassInputAttachmentAspectCreateInfoKHR input_attachment_aspect_create_info;

            input_attachment_aspect_create_info.aspectReferenceCount = static_cast<uint32_t>(input_attachment_aspect_reference_ptrs.size() );
            input_attachment_aspect_create_info.pAspectReferences    = reinterpret_cast<VkInputAttachmentAspectReference*>(&input_attachment_aspect_reference_ptrs.at(0) );
            input_attachment_aspect_create_info.pNext                = nullptr;
            input_attachment_aspect_create_info.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO_KHR;

            render_pass_create_info_chainer.append_struct(input_attachment_aspect_create_info);
        }
    }

    /* Don't forget about multiview structure, if multiview has been enabled for the renderpass. */
    if (m_render_pass_create_info_ptr->is_multiview_enabled() )
    {
        const uint32_t n_subpasses = m_render_pass_create_info_ptr->get_n_subpasses();

        if (n_subpasses > 0)
        {
            VkRenderPassMultiviewCreateInfoKHR multiview_create_info;
            const uint32_t*                    correlation_masks_ptr = nullptr;
            uint32_t                           n_correlation_masks   = 0;
            const uint32_t                     n_dependencies        = m_render_pass_create_info_ptr->get_n_dependencies();

            m_render_pass_create_info_ptr->get_multiview_correlation_masks(&n_correlation_masks,
                                                                           &correlation_masks_ptr);

            multiview_view_mask_vec_ptr.reset(new std::vector<uint32_t>(n_subpasses) );
            anvil_assert(multiview_view_mask_vec_ptr!= nullptr);

            multiview_view_offset_vec_ptr.reset(new std::vector<int32_t>(n_dependencies, UINT32_MAX) );
            anvil_assert(multiview_view_offset_vec_ptr != nullptr);

            for (uint32_t n_subpass = 0;
                          n_subpass < n_subpasses;
                        ++n_subpass)
            {
                uint32_t multiview_mask = 0;

                if (!m_render_pass_create_info_ptr->get_subpass_view_mask(n_subpass,
                                                                         &multiview_mask) )
                {
                    anvil_assert_fail();

                    goto end;
                }

                multiview_view_mask_vec_ptr->at(n_subpass) = multiview_mask;
            }

            for (uint32_t n_dependency = 0;
                          n_dependency < n_dependencies;
                        ++n_dependency)
            {
                int32_t view_offset = INT32_MAX;

                if (!m_render_pass_create_info_ptr->get_dependency_multiview_properties(n_dependency,
                                                                                       &view_offset) )
                {
                    anvil_assert_fail();

                    goto end;
                }

                multiview_view_offset_vec_ptr->at(n_dependency) = view_offset;
            }

            multiview_create_info.correlationMaskCount = n_correlation_masks;
            multiview_create_info.dependencyCount      = n_dependencies;
            multiview_create_info.pCorrelationMasks    = correlation_masks_ptr;
            multiview_create_info.pNext                = nullptr;
            multiview_create_info.pViewMasks           = &multiview_view_mask_vec_ptr->at(0);
            multiview_create_info.pViewOffsets         = (n_dependencies != 0) ? &multiview_view_offset_vec_ptr->at(0) : nullptr;
            multiview_create_info.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO_KHR;
            multiview_create_info.subpassCount         = n_subpasses;

            render_pass_create_info_chainer.append_struct(multiview_create_info);
        }
    }

    /* All done! Spawn the final chain and try to create the renderpass. */
    {
        auto create_info_chain_ptr = render_pass_create_info_chainer.create_chain();

        result_vk = Anvil::Vulkan::vkCreateRenderPass(m_device_ptr->get_device_vk(),
                                                      create_info_chain_ptr->get_root_struct(),
                                                      nullptr, /* pAllocator */
                                                     &m_render_pass);
    }

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert_vk_call_succeeded(result_vk);

        goto end;
    }

    set_vk_handle(m_render_pass);

    result  = true;

end:
    return result;
}

/* Please see header for specification */
Anvil::RenderPassUniquePtr Anvil::RenderPass::create(Anvil::RenderPassCreateInfoUniquePtr in_renderpass_create_info_ptr,
                                                     Anvil::Swapchain*                    in_opt_swapchain_ptr)
{
    std::unique_ptr<RenderPass> result_ptr(nullptr,
                                           std::default_delete<RenderPass>() );

    result_ptr.reset(
        new Anvil::RenderPass(std::move(in_renderpass_create_info_ptr),
                              in_opt_swapchain_ptr)
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return std::move(result_ptr);
}
