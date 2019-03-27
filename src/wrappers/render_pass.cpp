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
#include "misc/formats.h"
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
                                   Anvil::ObjectType::RENDER_PASS),
     m_render_pass                (VK_NULL_HANDLE),
     m_render_pass_create_info_ptr(std::move(in_renderpass_create_info_ptr) ),
     m_swapchain_ptr              (in_opt_swapchain_ptr)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::RENDER_PASS,
                                                  this);
}

/** Destructor. Will only be called by release() if the reference counter drops to zero. */
Anvil::RenderPass::~RenderPass()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::RENDER_PASS,
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
Anvil::RenderPassUniquePtr Anvil::RenderPass::create(Anvil::RenderPassCreateInfoUniquePtr in_renderpass_create_info_ptr,
                                                     Anvil::Swapchain*                    in_opt_swapchain_ptr)
{
    std::unique_ptr<RenderPass> result_ptr            (nullptr,
                                                       std::default_delete<RenderPass>() );
    auto                        rp_create_info_raw_ptr(in_renderpass_create_info_ptr.get() );

    result_ptr.reset(
        new Anvil::RenderPass(std::move(in_renderpass_create_info_ptr),
                              in_opt_swapchain_ptr)
    );

    if (result_ptr != nullptr)
    {
        bool result = false;

        if (rp_create_info_raw_ptr->get_device()->get_extension_info()->khr_create_renderpass2() )
        {
            result = result_ptr->init_using_rp2_create_extension();
        }
        else
        {
            result = result_ptr->init_using_core_vk10();
        }

        if (!result)
        {
            result_ptr.reset();
        }
    }

    return std::move(result_ptr);
}

/* Please see header for specification */
bool Anvil::RenderPass::init_using_core_vk10()
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

        /* NOTE: DS resolve operations are only accessible via KHR_depth_stencil_resolve + KHR_create_renderpass2. */
        anvil_assert( (*subpass_iterator)->depth_stencil_attachment.resolve_attachment_index == UINT32_MAX);

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

        subpass_vk.colorAttachmentCount    = n_color_attachments;
        subpass_vk.flags                   = 0;
        subpass_vk.inputAttachmentCount    = n_input_attachments;
        subpass_vk.pColorAttachments       = (n_color_attachments > 0)                                                      ? &current_subpass_attachment_set_ptr->color_attachments_vk.at(0)
                                                                                                                            : nullptr;
        subpass_vk.pDepthStencilAttachment = ((*subpass_iterator)->depth_stencil_attachment.attachment_index != UINT32_MAX) ? &current_subpass_attachment_set_ptr->depth_attachment_vk
                                                                                                                            : nullptr;
        subpass_vk.pInputAttachments       = (n_input_attachments > 0)                                                      ? &current_subpass_attachment_set_ptr->input_attachments_vk.at(0)
                                                                                                                            : nullptr;
        subpass_vk.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_vk.pPreserveAttachments    = (n_preserved_attachments > 0) ? &current_subpass_attachment_set_ptr->preserve_attachments_vk.at(0)
                                                                           : nullptr;
        subpass_vk.preserveAttachmentCount = n_preserved_attachments;
        subpass_vk.pResolveAttachments     = (n_resolved_attachments > 0) ? &current_subpass_attachment_set_ptr->resolve_color_attachments_vk.at(0)
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
bool Anvil::RenderPass::init_using_rp2_create_extension()
{
    const auto&                                      crp2_entrypoints = m_render_pass_create_info_ptr->get_device        ()->get_extension_khr_create_renderpass2_entrypoints();
    const uint32_t                                   n_rp_attachments = m_render_pass_create_info_ptr->get_n_attachments ();
    const uint32_t                                   n_subpass_deps   = m_render_pass_create_info_ptr->get_n_dependencies();
    const uint32_t                                   n_subpasses      = m_render_pass_create_info_ptr->get_n_subpasses   ();
    bool                                             result           = false;
    Anvil::StructChainer<VkRenderPassCreateInfo2KHR> struct_chainer;

    std::vector<VkAttachmentDescription2KHR>                     rp_attachment_vec               (n_rp_attachments);
    std::vector<VkAttachmentReference2KHR>                       subpass_attachment_reference_vec;
    Anvil::StructChainVector<VkSubpassDescription2KHR>           subpass_chain_vec;
    std::vector<Anvil::StructChainer<VkSubpassDescription2KHR> > subpass_chainer_vec             (n_subpasses);
    std::vector<VkSubpassDependency2KHR>                         subpass_dep_vec                 (n_subpass_deps);
    std::vector<uint32_t>                                        subpass_preserve_attachment_vec;

    /* Prepare renderpass attachment vec */
    for (uint32_t n_rp_attachment = 0;
                  n_rp_attachment < n_rp_attachments;
                ++n_rp_attachment)
    {
        Anvil::ImageLayout           current_rp_attachment_final_layout     = Anvil::ImageLayout::UNKNOWN;
        Anvil::Format                current_rp_attachment_format           = Anvil::Format::UNKNOWN;
        VkAttachmentDescription2KHR& current_rp_attachment_info             = rp_attachment_vec.at(n_rp_attachment);
        Anvil::ImageLayout           current_rp_attachment_initial_layout   = Anvil::ImageLayout::UNKNOWN;
        bool                         current_rp_attachment_may_alias        = false;
        Anvil::AttachmentLoadOp      current_rp_attachment_load_op          = Anvil::AttachmentLoadOp::UNKNOWN;
        Anvil::SampleCountFlagBits   current_rp_attachment_sample_count     = Anvil::SampleCountFlagBits::NONE;
        Anvil::AttachmentLoadOp      current_rp_attachment_stencil_load_op  = Anvil::AttachmentLoadOp::DONT_CARE;
        Anvil::AttachmentStoreOp     current_rp_attachment_stencil_store_op = Anvil::AttachmentStoreOp::DONT_CARE;
        Anvil::AttachmentStoreOp     current_rp_attachment_store_op         = Anvil::AttachmentStoreOp::UNKNOWN;
        Anvil::AttachmentType        current_rp_attachment_type             = Anvil::AttachmentType::UNKNOWN;

        m_render_pass_create_info_ptr->get_attachment_type(n_rp_attachment,
                                                          &current_rp_attachment_type);

        switch (current_rp_attachment_type)
        {
            case Anvil::AttachmentType::COLOR:
            {
                m_render_pass_create_info_ptr->get_color_attachment_properties(n_rp_attachment,
                                                                              &current_rp_attachment_format,
                                                                              &current_rp_attachment_sample_count,
                                                                              &current_rp_attachment_load_op,
                                                                              &current_rp_attachment_store_op,
                                                                              &current_rp_attachment_initial_layout,
                                                                              &current_rp_attachment_final_layout,
                                                                              &current_rp_attachment_may_alias);

                break;
            }

            case Anvil::AttachmentType::DEPTH_STENCIL:
            {
                m_render_pass_create_info_ptr->get_depth_stencil_attachment_properties(n_rp_attachment,
                                                                                      &current_rp_attachment_format,
                                                                                      &current_rp_attachment_sample_count,
                                                                                      &current_rp_attachment_load_op,
                                                                                      &current_rp_attachment_store_op,
                                                                                      &current_rp_attachment_stencil_load_op,
                                                                                      &current_rp_attachment_stencil_store_op,
                                                                                      &current_rp_attachment_initial_layout,
                                                                                      &current_rp_attachment_final_layout,
                                                                                      &current_rp_attachment_may_alias);

                break;
            }

            default:
            {
                anvil_assert_fail();

                goto end;
            }
        }

        current_rp_attachment_info.finalLayout    = static_cast<VkImageLayout>(current_rp_attachment_final_layout);
        current_rp_attachment_info.flags          = (current_rp_attachment_may_alias) ? VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT
                                                                                      : 0;
        current_rp_attachment_info.format         = static_cast<VkFormat>          (current_rp_attachment_format);
        current_rp_attachment_info.initialLayout  = static_cast<VkImageLayout>     (current_rp_attachment_initial_layout);
        current_rp_attachment_info.loadOp         = static_cast<VkAttachmentLoadOp>(current_rp_attachment_load_op);
        current_rp_attachment_info.pNext          = nullptr;
        current_rp_attachment_info.samples        = static_cast<VkSampleCountFlagBits>(current_rp_attachment_sample_count);
        current_rp_attachment_info.stencilLoadOp  = static_cast<VkAttachmentLoadOp>   (current_rp_attachment_stencil_load_op);
        current_rp_attachment_info.stencilStoreOp = static_cast<VkAttachmentStoreOp>  (current_rp_attachment_stencil_store_op);
        current_rp_attachment_info.storeOp        = static_cast<VkAttachmentStoreOp>  (current_rp_attachment_store_op);
        current_rp_attachment_info.sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR;
    }

    /* Prepare subpass dep vec */
    for (uint32_t n_subpass_dep = 0;
                  n_subpass_dep < n_subpass_deps;
                ++n_subpass_dep)
    {
        auto&                     current_subpass_dep                  = subpass_dep_vec.at(n_subpass_dep);
        Anvil::AccessFlags        current_subpass_dep_dst_access_mask  = Anvil::AccessFlagBits::NONE;
        Anvil::PipelineStageFlags current_subpass_dep_dst_stage_mask   = Anvil::PipelineStageFlagBits::NONE;
        Anvil::SubPassID          current_subpass_dep_dst_subpass_id   = UINT32_MAX;
        Anvil::DependencyFlags    current_subpass_dep_flags            = Anvil::DependencyFlagBits::NONE;
        int32_t                   current_subpass_dep_multiview_offset = 0;
        Anvil::AccessFlags        current_subpass_dep_src_access_mask  = Anvil::AccessFlagBits::NONE;
        Anvil::SubPassID          current_subpass_dep_src_subpass_id   = UINT32_MAX;
        Anvil::PipelineStageFlags current_subpass_dep_src_stage_mask   = Anvil::PipelineStageFlagBits::NONE;

        if (m_render_pass_create_info_ptr->is_multiview_enabled() )
        {
            m_render_pass_create_info_ptr->get_dependency_multiview_properties(n_subpass_dep,
                                                                              &current_subpass_dep_multiview_offset);
        }

        m_render_pass_create_info_ptr->get_dependency_properties(n_subpass_dep,
                                                                &current_subpass_dep_dst_subpass_id,
                                                                &current_subpass_dep_src_subpass_id,
                                                                &current_subpass_dep_dst_stage_mask,
                                                                &current_subpass_dep_src_stage_mask,
                                                                &current_subpass_dep_dst_access_mask,
                                                                &current_subpass_dep_src_access_mask,
                                                                &current_subpass_dep_flags);

        current_subpass_dep.dependencyFlags = current_subpass_dep_flags.get_vk          ();
        current_subpass_dep.dstAccessMask   = current_subpass_dep_dst_access_mask.get_vk();
        current_subpass_dep.dstStageMask    = current_subpass_dep_dst_stage_mask.get_vk ();
        current_subpass_dep.dstSubpass      = (current_subpass_dep_dst_subpass_id == UINT32_MAX) ? VK_SUBPASS_EXTERNAL
                                                                                                 : current_subpass_dep_dst_subpass_id;
        current_subpass_dep.pNext           = nullptr;
        current_subpass_dep.srcAccessMask   = current_subpass_dep_src_access_mask.get_vk();
        current_subpass_dep.srcStageMask    = current_subpass_dep_src_stage_mask.get_vk ();
        current_subpass_dep.srcSubpass      = (current_subpass_dep_src_subpass_id == UINT32_MAX) ? VK_SUBPASS_EXTERNAL
                                                                                                 : current_subpass_dep_src_subpass_id;
        current_subpass_dep.sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2_KHR;
        current_subpass_dep.viewOffset      = current_subpass_dep_multiview_offset;
    }

    /* Prepare subpass vec */
    for (uint32_t n_subpass = 0;
                  n_subpass < n_subpasses;
                ++n_subpass)
    {
        uint32_t  current_subpass_attachment_reference_vec_base_index      = static_cast<uint32_t>(subpass_attachment_reference_vec.size() );
        uint32_t  current_subpass_highest_location                         = 0;
        uint32_t  current_subpass_preserve_attachment_vec_base_index       = static_cast<uint32_t>(subpass_preserve_attachment_vec.size () );
        bool      current_subpass_uses_ds_resolve_operation                = false;
        uint32_t  current_subpass_view_mask                                = 0;
        uint32_t  n_current_subpass_color_attachments_actual               = 0;
        uint32_t  n_current_subpass_ds_attachments                         = 0;
        uint32_t  n_current_subpass_input_attachments_actual               = 0;
        uint32_t  n_current_subpass_color_input_resolve_attachments_needed = 0;
        uint32_t  n_current_subpass_preserve_attachments                   = 0;
        uint32_t  n_current_subpass_resolve_attachments_actual             = 0;

        m_render_pass_create_info_ptr->get_subpass_highest_location(n_subpass,
                                                                   &current_subpass_highest_location);

        n_current_subpass_color_input_resolve_attachments_needed = current_subpass_highest_location + 1;


        m_render_pass_create_info_ptr->get_subpass_n_attachments(n_subpass,
                                                                 Anvil::AttachmentType::COLOR,
                                                                &n_current_subpass_color_attachments_actual);
        m_render_pass_create_info_ptr->get_subpass_n_attachments(n_subpass,
                                                                 Anvil::AttachmentType::DEPTH_STENCIL,
                                                                &n_current_subpass_ds_attachments);
        m_render_pass_create_info_ptr->get_subpass_n_attachments(n_subpass,
                                                                 Anvil::AttachmentType::INPUT,
                                                                &n_current_subpass_input_attachments_actual);
        m_render_pass_create_info_ptr->get_subpass_n_attachments(n_subpass,
                                                                 Anvil::AttachmentType::PRESERVE,
                                                                &n_current_subpass_preserve_attachments);
        m_render_pass_create_info_ptr->get_subpass_n_attachments(n_subpass,
                                                                 Anvil::AttachmentType::RESOLVE,
                                                                &n_current_subpass_resolve_attachments_actual);

        const struct
        {
            Anvil::AttachmentType subpass_attachment_type;
            uint32_t              n_subpass_attachments;
            uint32_t              subpass_attachment_reference_vec_offset;
        } subpass_attachment_data[] =
        {
            /* NOTE: Preserve attachments are handled elsewhere */
            {Anvil::AttachmentType::COLOR,         n_current_subpass_color_input_resolve_attachments_needed, 0},
            {Anvil::AttachmentType::DEPTH_STENCIL, n_current_subpass_ds_attachments,                         n_current_subpass_color_input_resolve_attachments_needed},
            {Anvil::AttachmentType::INPUT,         n_current_subpass_color_input_resolve_attachments_needed, n_current_subpass_color_input_resolve_attachments_needed + n_current_subpass_ds_attachments},
            {Anvil::AttachmentType::RESOLVE,       n_current_subpass_color_input_resolve_attachments_needed, n_current_subpass_color_input_resolve_attachments_needed + n_current_subpass_ds_attachments + n_current_subpass_color_input_resolve_attachments_needed},
        };

        const auto& subpass_color_attachment_data   = subpass_attachment_data[0];
        const auto& subpass_ds_attachment_data      = subpass_attachment_data[1];
        const auto& subpass_input_attachment_data   = subpass_attachment_data[2];
        const auto& subpass_resolve_attachment_data = subpass_attachment_data[3];

        anvil_assert(subpass_color_attachment_data.subpass_attachment_type    == Anvil::AttachmentType::COLOR);
        anvil_assert(subpass_ds_attachment_data.subpass_attachment_type       == Anvil::AttachmentType::DEPTH_STENCIL);
        anvil_assert(subpass_input_attachment_data.subpass_attachment_type    == Anvil::AttachmentType::INPUT);
        anvil_assert(subpass_resolve_attachment_data.subpass_attachment_type  == Anvil::AttachmentType::RESOLVE);

        const uint32_t current_subpass_color_attachment_reference_vec_base_index      = current_subpass_attachment_reference_vec_base_index;
        const uint32_t current_subpass_ds_attachment_reference_vec_base_index         = current_subpass_color_attachment_reference_vec_base_index + subpass_color_attachment_data.n_subpass_attachments;
        uint32_t       current_subpass_ds_resolve_attachment_reference_vec_base_index = UINT32_MAX; /* optionally updated later */
        const uint32_t current_subpass_input_attachment_reference_vec_base_index      = current_subpass_ds_attachment_reference_vec_base_index    + subpass_ds_attachment_data.n_subpass_attachments;
        const uint32_t current_subpass_resolve_attachment_reference_vec_base_index    = current_subpass_input_attachment_reference_vec_base_index + subpass_input_attachment_data.n_subpass_attachments;

        subpass_attachment_reference_vec.resize(subpass_attachment_reference_vec.size()                   +
                                                subpass_color_attachment_data.n_subpass_attachments       +
                                                subpass_ds_attachment_data.n_subpass_attachments          +
                                                subpass_input_attachment_data.n_subpass_attachments       +
                                                subpass_resolve_attachment_data.n_subpass_attachments);

        subpass_preserve_attachment_vec.resize (subpass_preserve_attachment_vec.size() +
                                                n_current_subpass_preserve_attachments);

        /* Fill the new attachment reference structs with dummy data */
        for (uint32_t n_subpass_attachment_reference_item = current_subpass_attachment_reference_vec_base_index;
                      n_subpass_attachment_reference_item < static_cast<uint32_t>(subpass_attachment_reference_vec.size() );
                    ++n_subpass_attachment_reference_item)
        {
            auto& current_subpass_attachment_reference = subpass_attachment_reference_vec.at(n_subpass_attachment_reference_item);

            current_subpass_attachment_reference.aspectMask = 0;
            current_subpass_attachment_reference.attachment = VK_ATTACHMENT_UNUSED;
            current_subpass_attachment_reference.layout     = VK_IMAGE_LAYOUT_UNDEFINED;
            current_subpass_attachment_reference.pNext      = nullptr;
            current_subpass_attachment_reference.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR;
        }

        for (const auto& current_subpass_attachment_data : subpass_attachment_data)
        {
            for (uint32_t n_subpass_attachment = 0;
                          n_subpass_attachment < current_subpass_attachment_data.n_subpass_attachments;
                        ++n_subpass_attachment)
            {
                Anvil::ImageAspectFlags       current_subpass_attachment_aspects_accessed = Anvil::ImageAspectFlagBits::NONE;
                Anvil::ImageLayout            current_subpass_attachment_layout           = Anvil::ImageLayout::UNKNOWN;
                uint32_t                      current_subpass_attachment_location         = UINT32_MAX;
                Anvil::RenderPassAttachmentID current_subpass_attachment_rp_attachment_id = VK_ATTACHMENT_UNUSED;
                uint32_t                      reference_vec_base_index                    = UINT32_MAX;

                if (!m_render_pass_create_info_ptr->get_subpass_attachment_properties(n_subpass,
                                                                                      current_subpass_attachment_data.subpass_attachment_type,
                                                                                      n_subpass_attachment,
                                                                                     &current_subpass_attachment_rp_attachment_id,
                                                                                     &current_subpass_attachment_layout,
                                                                                     &current_subpass_attachment_aspects_accessed,
                                                                                      nullptr, /* out_opt_attachment_resolve_id_ptr */
                                                                                     &current_subpass_attachment_location) )
                {
                     /* Out of attachments of this type, move on. */
                    break;
                }

                if (current_subpass_attachment_aspects_accessed == Anvil::ImageAspectFlagBits::NONE)
                {
                    Anvil::AttachmentType rp_attachment_type = Anvil::AttachmentType::UNKNOWN;

                    if (!m_render_pass_create_info_ptr->get_attachment_type(current_subpass_attachment_rp_attachment_id,
                                                                           &rp_attachment_type) )
                    {
                        anvil_assert_fail();

                        goto end;
                    }

                    if (rp_attachment_type == Anvil::AttachmentType::COLOR)
                    {
                        current_subpass_attachment_aspects_accessed = Anvil::ImageAspectFlagBits::COLOR_BIT;
                    }
                    else
                    {
                        Anvil::Format rp_attachment_format = Anvil::Format::UNKNOWN;

                        anvil_assert(rp_attachment_type == Anvil::AttachmentType::DEPTH_STENCIL);

                        m_render_pass_create_info_ptr->get_depth_stencil_attachment_properties(current_subpass_attachment_rp_attachment_id,
                                                                                              &rp_attachment_format);

                        anvil_assert(rp_attachment_format != Anvil::Format::UNKNOWN);

                        current_subpass_attachment_aspects_accessed = ((Anvil::Formats::has_depth_aspect  (rp_attachment_format) ) ? Anvil::ImageAspectFlagBits::DEPTH_BIT   : Anvil::ImageAspectFlagBits::NONE) |
                                                                      ((Anvil::Formats::has_stencil_aspect(rp_attachment_format) ) ? Anvil::ImageAspectFlagBits::STENCIL_BIT : Anvil::ImageAspectFlagBits::NONE);
                    }
                }

                switch (current_subpass_attachment_data.subpass_attachment_type)
                {
                    case AttachmentType::COLOR:         reference_vec_base_index = current_subpass_color_attachment_reference_vec_base_index;   break;
                    case AttachmentType::DEPTH_STENCIL: reference_vec_base_index = current_subpass_ds_attachment_reference_vec_base_index;      break;
                    case AttachmentType::INPUT:         reference_vec_base_index = current_subpass_input_attachment_reference_vec_base_index;   break;
                    case AttachmentType::RESOLVE:       reference_vec_base_index = current_subpass_resolve_attachment_reference_vec_base_index; break;

                    default:
                    {
                        anvil_assert_fail();

                        goto end;
                    }
                }

                auto& current_subpass_attachment_reference = subpass_attachment_reference_vec.at(reference_vec_base_index + current_subpass_attachment_location);

                current_subpass_attachment_reference.aspectMask = current_subpass_attachment_aspects_accessed.get_vk();
                current_subpass_attachment_reference.attachment = current_subpass_attachment_rp_attachment_id;
                current_subpass_attachment_reference.layout     = static_cast<VkImageLayout>(current_subpass_attachment_layout);
                current_subpass_attachment_reference.pNext      = nullptr;
                current_subpass_attachment_reference.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR;
            }

            if (current_subpass_attachment_data.subpass_attachment_type == AttachmentType::DEPTH_STENCIL)
            {
                Anvil::ResolveModeFlagBits    current_subpass_ds_resolve_attachment_depth_resolve_mode       = Anvil::ResolveModeFlagBits::NONE;
                Anvil::ImageLayout            current_subpass_ds_resolve_attachment_layout                   = Anvil::ImageLayout::UNKNOWN;
                Anvil::RenderPassAttachmentID current_subpass_ds_resolve_attachment_renderpass_attachment_id = UINT32_MAX;
                Anvil::ResolveModeFlagBits    current_subpass_ds_resolve_attachment_stencil_resolve_mode     = Anvil::ResolveModeFlagBits::NONE;

                current_subpass_uses_ds_resolve_operation = (m_render_pass_create_info_ptr->get_subpass_ds_resolve_attachment_properties(n_subpass,
                                                                                                                                        &current_subpass_ds_resolve_attachment_renderpass_attachment_id,
                                                                                                                                        &current_subpass_ds_resolve_attachment_layout,
                                                                                                                                        &current_subpass_ds_resolve_attachment_depth_resolve_mode,
                                                                                                                                        &current_subpass_ds_resolve_attachment_stencil_resolve_mode) );

                if (current_subpass_uses_ds_resolve_operation)
                {
                    subpass_attachment_reference_vec.resize(subpass_attachment_reference_vec.size() + 1);

                    auto& current_subpass_ds_resolve_attachment_reference = subpass_attachment_reference_vec.back();

                    current_subpass_ds_resolve_attachment_reference.aspectMask = static_cast<VkImageAspectFlagBits>(((current_subpass_ds_resolve_attachment_depth_resolve_mode   != Anvil::ResolveModeFlagBits::NONE) ? Anvil::ImageAspectFlagBits::DEPTH_BIT   : Anvil::ImageAspectFlagBits::NONE) |
                                                                                                                    ((current_subpass_ds_resolve_attachment_stencil_resolve_mode != Anvil::ResolveModeFlagBits::NONE) ? Anvil::ImageAspectFlagBits::STENCIL_BIT : Anvil::ImageAspectFlagBits::NONE) );
                    current_subpass_ds_resolve_attachment_reference.attachment = current_subpass_ds_resolve_attachment_renderpass_attachment_id;
                    current_subpass_ds_resolve_attachment_reference.layout     = static_cast<VkImageLayout>(current_subpass_ds_resolve_attachment_layout);
                    current_subpass_ds_resolve_attachment_reference.pNext      = nullptr;
                    current_subpass_ds_resolve_attachment_reference.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR;

                    current_subpass_ds_resolve_attachment_reference_vec_base_index = static_cast<uint32_t>(subpass_attachment_reference_vec.size() - 1);
                }
            }

            current_subpass_attachment_reference_vec_base_index += current_subpass_attachment_data.n_subpass_attachments;
        }

        for (uint32_t n_subpass_preserve_attachment = 0;
                      n_subpass_preserve_attachment < n_current_subpass_preserve_attachments;
                    ++n_subpass_preserve_attachment)
        {
            auto&              current_subpass_preserve_attachment = subpass_preserve_attachment_vec.at(current_subpass_preserve_attachment_vec_base_index + n_subpass_preserve_attachment);
            Anvil::ImageLayout dummy_layout;

            m_render_pass_create_info_ptr->get_subpass_attachment_properties(n_subpass,
                                                                             Anvil::AttachmentType::PRESERVE,
                                                                             n_subpass_preserve_attachment,
                                                                            &current_subpass_preserve_attachment,
                                                                            &dummy_layout);

        }

        if (m_render_pass_create_info_ptr->is_multiview_enabled() )
        {
            m_render_pass_create_info_ptr->get_subpass_view_mask(n_subpass,
                                                                &current_subpass_view_mask);
        }

        /* NOTE: For ptrs, we only store offsets to reference structs in this pass. Once all subpasses are processed, base pointer
         *       is added. This is required since std::vector will likely realloc its storage as we keep pushing structs into it.
         */
        {
            auto& current_subpass_chainer = subpass_chainer_vec.at(n_subpass);

            /* Chain the root subpass struct */
            {
                VkSubpassDescription2KHR current_subpass;

                current_subpass.colorAttachmentCount    = (n_current_subpass_color_attachments_actual > 0) ? n_current_subpass_color_input_resolve_attachments_needed
                                                                                                           : 0;
                current_subpass.flags                   = 0;
                current_subpass.inputAttachmentCount    = (n_current_subpass_input_attachments_actual > 0) ? n_current_subpass_color_input_resolve_attachments_needed
                                                                                                           : 0;
                current_subpass.pColorAttachments       = (n_current_subpass_color_attachments_actual > 0) ? reinterpret_cast<const VkAttachmentReference2KHR*>(current_subpass_color_attachment_reference_vec_base_index * sizeof(VkAttachmentReference2KHR) )
                                                                                                           : nullptr;
                current_subpass.pDepthStencilAttachment = (n_current_subpass_ds_attachments           > 0) ? reinterpret_cast<const VkAttachmentReference2KHR*>(current_subpass_ds_attachment_reference_vec_base_index * sizeof(VkAttachmentReference2KHR) )
                                                                                                           : nullptr;
                current_subpass.pInputAttachments       = (n_current_subpass_input_attachments_actual > 0) ? reinterpret_cast<const VkAttachmentReference2KHR*>(current_subpass_input_attachment_reference_vec_base_index * sizeof(VkAttachmentReference2KHR) )
                                                                                                           : nullptr;
                current_subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
                current_subpass.pNext                   = nullptr;
                current_subpass.pPreserveAttachments    = (n_current_subpass_preserve_attachments > 0) ? reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(0) + current_subpass_preserve_attachment_vec_base_index)
                                                                                                       : nullptr;
                current_subpass.preserveAttachmentCount = n_current_subpass_preserve_attachments;
                current_subpass.pResolveAttachments     = (n_current_subpass_resolve_attachments_actual > 0) ? reinterpret_cast<const VkAttachmentReference2KHR*>(current_subpass_resolve_attachment_reference_vec_base_index * sizeof(VkAttachmentReference2KHR) )
                                                                                                             : nullptr;
                current_subpass.sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR;
                current_subpass.viewMask                = current_subpass_view_mask;

                current_subpass_chainer.append_struct(current_subpass);
            }

            /* Chain the DS resolve struct if needed. */
            if (current_subpass_uses_ds_resolve_operation)
            {
                Anvil::ResolveModeFlagBits                 depth_resolve_mode   = Anvil::ResolveModeFlagBits::NONE;
                VkSubpassDescriptionDepthStencilResolveKHR ds_resolve;
                Anvil::ResolveModeFlagBits                 stencil_resolve_mode = Anvil::ResolveModeFlagBits::NONE;

                anvil_assert(current_subpass_ds_resolve_attachment_reference_vec_base_index != UINT32_MAX);

                m_render_pass_create_info_ptr->get_subpass_ds_resolve_attachment_properties(n_subpass,
                                                                                            nullptr, /* out_opt_renderpass_attachment_id_ptr */
                                                                                            nullptr, /* out_opt_layout_ptr                   */
                                                                                           &depth_resolve_mode,
                                                                                           &stencil_resolve_mode);

                ds_resolve.depthResolveMode               = static_cast<VkResolveModeFlagBitsKHR>             (depth_resolve_mode);
                ds_resolve.pDepthStencilResolveAttachment = reinterpret_cast<const VkAttachmentReference2KHR*>(current_subpass_ds_resolve_attachment_reference_vec_base_index * sizeof(VkAttachmentReference2KHR) );
                ds_resolve.pNext                          = nullptr;
                ds_resolve.stencilResolveMode             = static_cast<VkResolveModeFlagBitsKHR>(stencil_resolve_mode);
                ds_resolve.sType                          = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE_KHR;

                current_subpass_chainer.append_struct(ds_resolve);
            }
        }
    }

    /* Convert offsets to actual pointers */
    for (uint32_t n_subpass = 0;
                  n_subpass < n_subpasses;
                ++n_subpass)
    {
        auto&    current_subpass_chainer                    = subpass_chainer_vec.at                 (n_subpass);
        auto     current_subpass_ptr                        = current_subpass_chainer.get_root_struct();
        bool     current_subpass_uses_ds_resolve_attachment = false;
        uint32_t n_current_subpass_ds_attachments           = 0;
        uint32_t n_current_subpass_preserve_attachments     = 0;

        m_render_pass_create_info_ptr->get_subpass_n_attachments(n_subpass,
                                                                 Anvil::AttachmentType::DEPTH_STENCIL,
                                                                &n_current_subpass_ds_attachments);
        m_render_pass_create_info_ptr->get_subpass_n_attachments(n_subpass,
                                                                 Anvil::AttachmentType::PRESERVE,
                                                                &n_current_subpass_preserve_attachments);

        current_subpass_uses_ds_resolve_attachment = m_render_pass_create_info_ptr->get_subpass_ds_resolve_attachment_properties(n_subpass);

        const struct
        {
            bool                              active;
            const VkAttachmentReference2KHR** attachment_reference_ptr_ptr;
        } subpass_attachment_data[] =
        {
            {current_subpass_ptr->colorAttachmentCount >  0,       &current_subpass_ptr->pColorAttachments},
            {n_current_subpass_ds_attachments          >  0,       &current_subpass_ptr->pDepthStencilAttachment},
            {current_subpass_ptr->inputAttachmentCount >  0,       &current_subpass_ptr->pInputAttachments},
            {current_subpass_ptr->pResolveAttachments  != nullptr, &current_subpass_ptr->pResolveAttachments},
        };

        for (const auto& current_subpass_attachment_data : subpass_attachment_data)
        {
            if (!current_subpass_attachment_data.active)
            {
                continue;
            }

            *current_subpass_attachment_data.attachment_reference_ptr_ptr = reinterpret_cast<const VkAttachmentReference2KHR*>(reinterpret_cast<const uint8_t*>(*current_subpass_attachment_data.attachment_reference_ptr_ptr) + reinterpret_cast<intptr_t>(&subpass_attachment_reference_vec.at(0) ));
        }

        if (n_current_subpass_preserve_attachments > 0)
        {
            current_subpass_ptr->pPreserveAttachments = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(current_subpass_ptr->pPreserveAttachments) + reinterpret_cast<intptr_t>(&subpass_preserve_attachment_vec.at(0) ));
        }

        if (current_subpass_uses_ds_resolve_attachment)
        {
            VkSubpassDescriptionDepthStencilResolveKHR* ds_resolve_struct_ptr = nullptr;
            const auto                                  n_structs             = current_subpass_chainer.get_n_structs();

            for (uint32_t n_struct = 1;
                          n_struct < n_structs;
                        ++n_struct)
            {
                auto struct_header_ptr = current_subpass_chainer.get_struct_at_index(n_struct);
                anvil_assert(struct_header_ptr != nullptr);

                if (struct_header_ptr->type == VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE_KHR)
                {
                    ds_resolve_struct_ptr = reinterpret_cast<VkSubpassDescriptionDepthStencilResolveKHR*>(struct_header_ptr);

                    break;
                }}


            anvil_assert(ds_resolve_struct_ptr != nullptr);

            ds_resolve_struct_ptr->pDepthStencilResolveAttachment = reinterpret_cast<const VkAttachmentReference2KHR*>(reinterpret_cast<const uint8_t*>(ds_resolve_struct_ptr->pDepthStencilResolveAttachment) + reinterpret_cast<intptr_t>(&subpass_attachment_reference_vec.at(0) ));
        }
    }

    /* Create subpass struct chains. */
    for (uint32_t n_subpass_chainer = 0;
                  n_subpass_chainer < static_cast<uint32_t>(subpass_chainer_vec.size() );
                ++n_subpass_chainer)
    {
        const auto& current_subpass_chainer = subpass_chainer_vec.at              (n_subpass_chainer);
        auto        subpass_chain_ptr       = current_subpass_chainer.create_chain();

        anvil_assert(subpass_chain_ptr != nullptr);

        subpass_chain_vec.append_struct_chain(std::move(subpass_chain_ptr) );
    }

    /* Chain the root struct. */
    {
        VkRenderPassCreateInfo2KHR create_info;
        const uint32_t*            multiview_correlated_masks   = nullptr;
        uint32_t                   n_multiview_correlated_masks = 0;

        if (m_render_pass_create_info_ptr->is_multiview_enabled() )
        {
            m_render_pass_create_info_ptr->get_multiview_correlation_masks(&n_multiview_correlated_masks,
                                                                           &multiview_correlated_masks);
        }

        create_info.attachmentCount         = n_rp_attachments;
        create_info.correlatedViewMaskCount = n_multiview_correlated_masks;
        create_info.dependencyCount         = n_subpass_deps;
        create_info.flags                   = 0;
        create_info.pAttachments            = (n_rp_attachments > 0) ? &rp_attachment_vec.at(0)
                                                                     : nullptr;
        create_info.pCorrelatedViewMasks    = multiview_correlated_masks;
        create_info.pDependencies           = (n_subpass_deps > 0) ? &subpass_dep_vec.at(0)
                                                                   : nullptr;
        create_info.pNext                   = nullptr;
        create_info.pSubpasses              = (n_subpasses > 0) ? subpass_chain_vec.get_root_structs()
                                                                : nullptr;
        create_info.sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2_KHR;
        create_info.subpassCount            = m_render_pass_create_info_ptr->get_n_subpasses();

        struct_chainer.append_struct(create_info);
    }

    /* All done! Spawn the final chain and try to create the renderpass. */
    {
        auto     create_info_chain_ptr = struct_chainer.create_chain();
        VkResult result_vk             = VK_ERROR_DEVICE_LOST;

        result_vk = crp2_entrypoints.vkCreateRenderPass2KHR(m_device_ptr->get_device_vk           (),
                                                            create_info_chain_ptr->get_root_struct(),
                                                            nullptr, /* pAllocator */
                                                           &m_render_pass);

        if (!is_vk_call_successful(result_vk) )
        {
            anvil_assert_vk_call_succeeded(result_vk);

            goto end;
        }
    }

    set_vk_handle(m_render_pass);
    result  = true;
end:
    return result;
}
