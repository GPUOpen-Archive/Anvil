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

#include <algorithm>
#include "misc/base_pipeline_create_info.h"
#include "misc/base_pipeline_manager.h"
#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "misc/render_pass_create_info.h"
#include "wrappers/device.h"
#include "wrappers/graphics_pipeline_manager.h"
#include "wrappers/pipeline_cache.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/render_pass.h"
#include "wrappers/shader_module.h"
#include "wrappers/swapchain.h"

#if defined(max)
    #undef max
#endif


/* Please see header for specification */
Anvil::GraphicsPipelineManager::GraphicsPipelineManager(const Anvil::BaseDevice* in_device_ptr,
                                                        bool                     in_mt_safe,
                                                        bool                     in_use_pipeline_cache,
                                                        Anvil::PipelineCache*    in_pipeline_cache_to_reuse_ptr)
    :BasePipelineManager(in_device_ptr,
                         in_mt_safe,
                         in_use_pipeline_cache,
                         in_pipeline_cache_to_reuse_ptr)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::ANVIL_GRAPHICS_PIPELINE_MANAGER,
                                                  this);
}

/* Please see header for specification */
Anvil::GraphicsPipelineManager::~GraphicsPipelineManager()
{
    m_baked_pipelines.clear      ();
    m_outstanding_pipelines.clear();

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::ANVIL_GRAPHICS_PIPELINE_MANAGER,
                                                    this);
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::bake()
{
    typedef struct BakeItem
    {
        PipelineID pipeline_id;
        Pipeline*  pipeline_ptr;

        BakeItem(PipelineID in_pipeline_id,
                 Pipeline*  in_pipeline_ptr)
        {
            pipeline_id  = in_pipeline_id;
            pipeline_ptr = in_pipeline_ptr;
        }

        bool operator==(PipelineID in_pipeline_id)
        {
            return pipeline_id == in_pipeline_id;
        }
    } BakeItem;

    std::vector<BakeItem>                  bake_items;
    auto                                   color_blend_state_create_info_chain_cache          = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineColorBlendStateCreateInfo> > >   ();
    auto                                   depth_stencil_state_create_info_chain_cache        = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineDepthStencilStateCreateInfo> > > ();
    auto                                   dynamic_state_create_info_chain_cache              = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineDynamicStateCreateInfo> > >      ();
    auto                                   graphics_pipeline_create_info_chains               = Anvil::StructChainVector<VkGraphicsPipelineCreateInfo>                                    ();
    auto                                   input_assembly_state_create_info_chain_cache       = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineInputAssemblyStateCreateInfo> > >();
    auto                                   multisample_state_create_info_chain_cache          = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineMultisampleStateCreateInfo> > >  ();
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr                                          = get_mutex();
    uint32_t                               n_consumed_graphics_pipelines                      = 0;
    auto                                   raster_state_create_info_chain_cache               = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineRasterizationStateCreateInfo> > >();
    bool                                   result                                             = false;
    std::vector<VkPipeline>                result_graphics_pipelines;
    VkResult                               result_vk;
    auto                                   shader_stage_create_info_chain_ptrs                = std::vector<std::unique_ptr<Anvil::StructChainVector<VkPipelineShaderStageCreateInfo> > >();
    auto                                   tessellation_state_create_info_chain_cache         = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineTessellationStateCreateInfo> > >();
    auto                                   vertex_input_state_create_info_chain_cache         = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineVertexInputStateCreateInfo> > > ();
    auto                                   viewport_state_create_info_chain_cache             = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineViewportStateCreateInfo> > >    ();


    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    for (auto pipeline_iterator  = m_outstanding_pipelines.begin();
              pipeline_iterator != m_outstanding_pipelines.end();
            ++pipeline_iterator)
    {
        if (pipeline_iterator->second->layout_ptr == nullptr)
        {
            get_pipeline_layout(pipeline_iterator->first);

            anvil_assert(pipeline_iterator->second->layout_ptr != nullptr);
        }

        bake_items.push_back(
            BakeItem(pipeline_iterator->first,
                     pipeline_iterator->second.get() )
        );
    }

    if (bake_items.size() == 0)
    {
        result = true;

        goto end;
    }

    for (auto bake_item_iterator  = bake_items.begin();
              bake_item_iterator != bake_items.end();
            ++bake_item_iterator)
    {
        bool                                               color_blend_state_used                = false;
        auto                                               current_pipeline_create_info_ptr      = dynamic_cast<GraphicsPipelineCreateInfo*>(bake_item_iterator->pipeline_ptr->pipeline_create_info_ptr.get() );
        Pipeline*                                          current_pipeline_ptr                  = bake_item_iterator->pipeline_ptr;
        const Anvil::RenderPass*                           current_pipeline_renderpass_ptr       = nullptr;
        Anvil::SubPassID                                   current_pipeline_subpass_id;
        bool                                               depth_stencil_state_used              = false;
        bool                                               dynamic_state_used                    = false;
        Anvil::StructChainer<VkGraphicsPipelineCreateInfo> graphics_pipeline_create_info_chainer;
        bool                                               is_dynamic_scissor_state_enabled      = false;
        bool                                               is_dynamic_viewport_state_enabled     = false;
        bool                                               multisample_state_used                = false;
        uint32_t                                           n_shader_stages_used                  = 0;
        bool                                               tessellation_state_used               = false;
        bool                                               viewport_state_used                   = false;

        if (current_pipeline_create_info_ptr == nullptr)
        {
            anvil_assert(current_pipeline_create_info_ptr != nullptr);

            continue;
        }

        current_pipeline_renderpass_ptr = current_pipeline_create_info_ptr->get_renderpass();
        current_pipeline_subpass_id     = current_pipeline_create_info_ptr->get_subpass_id();

        if (current_pipeline_create_info_ptr->is_proxy() )
        {
            continue;
        }

        /* Form the color blend state create info descriptor, if needed */
        {
            auto color_blend_state_create_info_chain_ptr = bake_pipeline_color_blend_state_create_info(current_pipeline_create_info_ptr,
                                                                                                       current_pipeline_renderpass_ptr,
                                                                                                       current_pipeline_subpass_id);

            if (color_blend_state_create_info_chain_ptr != nullptr)
            {
                color_blend_state_create_info_chain_cache.push_back(std::move(color_blend_state_create_info_chain_ptr) );

                color_blend_state_used = true;
            }
            else
            {
                /* No color attachments available. Make sure none of the dependent modes are enabled. */
                bool logic_op_enabled = false;

                current_pipeline_create_info_ptr->get_logic_op_state(&logic_op_enabled,
                                                                     nullptr); /* out_opt_logic_op_ptr */

                anvil_assert(!logic_op_enabled);

                color_blend_state_used = false;
            }
        }

        /* Form the depth stencil state create info descriptor, if needed */
        {
            auto depth_stencil_state_create_info_chain_ptr = bake_pipeline_depth_stencil_state_create_info(current_pipeline_create_info_ptr,
                                                                                                           current_pipeline_renderpass_ptr);

            if (depth_stencil_state_create_info_chain_ptr != nullptr)
            {
                depth_stencil_state_create_info_chain_cache.push_back(std::move(depth_stencil_state_create_info_chain_ptr) );

                depth_stencil_state_used = true;
            }
            else
            {
                depth_stencil_state_used = false;
            }
        }

        /* Form the dynamic state create info descriptor, if needed */
        {
            auto dynamic_state_create_info_ptr = bake_pipeline_dynamic_state_create_info(current_pipeline_create_info_ptr);

            dynamic_state_used                = false;
            is_dynamic_scissor_state_enabled  = false;
            is_dynamic_viewport_state_enabled = false;

            if (dynamic_state_create_info_ptr != nullptr)
            {
                const Anvil::DynamicState* enabled_dynamic_states_ptr = nullptr;
                uint32_t                   n_enabled_dynamic_states   = 0;

                current_pipeline_create_info_ptr->get_enabled_dynamic_states(&enabled_dynamic_states_ptr,
                                                                             &n_enabled_dynamic_states);

                for (uint32_t n_dynamic_state = 0;
                              n_dynamic_state < n_enabled_dynamic_states;
                            ++n_dynamic_state)
                {
                    if (enabled_dynamic_states_ptr[n_dynamic_state] == Anvil::DynamicState::SCISSOR)
                    {
                        is_dynamic_scissor_state_enabled = true;
                    }
                    else
                    if (enabled_dynamic_states_ptr[n_dynamic_state] == Anvil::DynamicState::VIEWPORT)
                    {
                        is_dynamic_viewport_state_enabled = true;
                    }
                }

                dynamic_state_used = true;

                dynamic_state_create_info_chain_cache.push_back(std::move(dynamic_state_create_info_ptr) );
            }
        }

        /* Form the input assembly create info descriptor */
        {
            auto input_assembly_create_info_ptr = bake_pipeline_input_assembly_state_create_info(current_pipeline_create_info_ptr);

            anvil_assert(input_assembly_create_info_ptr != nullptr);

            input_assembly_state_create_info_chain_cache.push_back(std::move(input_assembly_create_info_ptr) );
        }

        /* Form the multisample state create info descriptor, if needed */
        {
            auto multisample_state_create_info_ptr = bake_pipeline_multisample_state_create_info(current_pipeline_create_info_ptr);

            if (multisample_state_create_info_ptr != nullptr)
            {
                multisample_state_used = true;
            }
            else
            {
                multisample_state_used = false;
            }

            multisample_state_create_info_chain_cache.push_back(std::move(multisample_state_create_info_ptr) );
        }

        /* Form the raster state create info chain */
        {
            auto raster_state_create_info_ptr = bake_pipeline_rasterization_state_create_info(current_pipeline_create_info_ptr);

            anvil_assert(raster_state_create_info_ptr != nullptr);

            raster_state_create_info_chain_cache.push_back(std::move(raster_state_create_info_ptr) );
        }

        /* Form stage descriptors */
        {
            auto shader_stage_create_info_chain_vec_ptr = bake_pipeline_shader_stage_create_info_chain_vector(current_pipeline_create_info_ptr);
            anvil_assert(shader_stage_create_info_chain_vec_ptr != nullptr);

            n_shader_stages_used = shader_stage_create_info_chain_vec_ptr->get_n_structs();

            shader_stage_create_info_chain_ptrs.push_back(std::move(shader_stage_create_info_chain_vec_ptr) );
        }

        /* Form the tessellation state create info descriptor if needed */
        {
            auto tessellation_state_create_info_ptr = bake_pipeline_tessellation_state_create_info(current_pipeline_create_info_ptr);

            if (tessellation_state_create_info_ptr != nullptr)
            {
                tessellation_state_create_info_chain_cache.push_back(std::move(tessellation_state_create_info_ptr) );

                tessellation_state_used = true;
            }
            else
            {
                tessellation_state_used = false;
            }
        }

        /* Form the vertex input state create info descriptor */
        {
            auto vertex_input_state_create_info_ptr = bake_pipeline_vertex_input_state_create_info(current_pipeline_create_info_ptr);

            anvil_assert(vertex_input_state_create_info_ptr != nullptr);

            vertex_input_state_create_info_chain_cache.push_back(std::move(vertex_input_state_create_info_ptr) );
        }

        /* Form the viewport state create info descriptor, if needed */
        {
            auto viewport_state_create_info_ptr = bake_pipeline_viewport_state_create_info(current_pipeline_create_info_ptr,
                                                                                           is_dynamic_scissor_state_enabled,
                                                                                           is_dynamic_viewport_state_enabled);

            if (viewport_state_create_info_ptr != nullptr)
            {
                viewport_state_create_info_chain_cache.push_back(std::move(viewport_state_create_info_ptr) );

                viewport_state_used = true;
            }
            else
            {
                viewport_state_used = false;
            }
        }

        /* Bake the GFX pipeline info struct chain */
        {
            VkPipeline base_pipeline_handle              = VK_NULL_HANDLE;
            int32_t    base_pipeline_index               = UINT32_MAX;
            const auto current_pipeline_base_pipeline_id = current_pipeline_create_info_ptr->get_base_pipeline_id();

            if (current_pipeline_base_pipeline_id != UINT32_MAX)
            {
                /* There are three cases we need to handle separately here:
                 *
                 * 1. The base pipeline is to be baked in the call we're preparing for. Determine
                 *    its index in the create info descriptor array and use it.
                 * 2. The pipeline has been baked earlier. Pass the baked pipeline's handle.
                 * 3. The pipeline under specified index uses a different layout. This indicates
                 *    a bug in the app or the manager.
                 *
                 * NOTE: A slightly adjusted version of this code is re-used in ComputePipelineManager::bake()
                 */
                auto base_bake_item_iterator = std::find(bake_items.begin(),
                                                         bake_items.end(),
                                                         current_pipeline_base_pipeline_id);

                if (base_bake_item_iterator != bake_items.end() )
                {
                    /* Case 1 */
                    base_pipeline_index = static_cast<int32_t>(base_bake_item_iterator - bake_items.begin() );
                }
                else
                {
                    auto baked_pipeline_iterator = m_baked_pipelines.find(current_pipeline_base_pipeline_id);

                    if (baked_pipeline_iterator                         != m_baked_pipelines.end() &&
                        baked_pipeline_iterator->second->baked_pipeline != VK_NULL_HANDLE)
                    {
                        /* Case 2 */
                        base_pipeline_handle = baked_pipeline_iterator->second->baked_pipeline;
                    }
                    else
                    {
                        /* Case 3 */
                        anvil_assert_fail();
                    }
                }
            }
            else
            {
                /* No base pipeline requested */
            }

            auto result_ptr = bake_graphics_pipeline_create_info(current_pipeline_create_info_ptr,
                                                                 current_pipeline_ptr->layout_ptr.get(),
                                                                 base_pipeline_handle,
                                                                 base_pipeline_index,
                                                                 (color_blend_state_used)    ? color_blend_state_create_info_chain_cache.back()->get_root_struct()
                                                                                             : nullptr,
                                                                 (depth_stencil_state_used)  ? depth_stencil_state_create_info_chain_cache.back()->get_root_struct()
                                                                                             : nullptr,
                                                                 (dynamic_state_used)        ? dynamic_state_create_info_chain_cache.back()->get_root_struct()
                                                                                             : nullptr,
                                                                 input_assembly_state_create_info_chain_cache.back()->get_root_struct(),
                                                                 (multisample_state_used)    ? multisample_state_create_info_chain_cache.back()->get_root_struct()
                                                                                             : nullptr,
                                                                 raster_state_create_info_chain_cache.back()->root_struct_ptr,
                                                                 n_shader_stages_used,
                                                                 (n_shader_stages_used > 0)  ? shader_stage_create_info_chain_ptrs.back()->get_root_structs()
                                                                                             : nullptr,
                                                                 (tessellation_state_used)   ? tessellation_state_create_info_chain_cache.back()->get_root_struct()
                                                                                             : nullptr,
                                                                 vertex_input_state_create_info_chain_cache.back()->get_root_struct(),
                                                                 (viewport_state_used)       ? viewport_state_create_info_chain_cache.back()->get_root_struct()
                                                                                             : nullptr);

            /* Stash the descriptor for now. We will issue one expensive vkCreateGraphicsPipelines() call after all pipeline objects
             * are iterated over. */
            graphics_pipeline_create_info_chains.append_struct_chain(std::move(result_ptr) );
        }
    }

    /* All right. Try to bake all pipeline objects at once */
    result_graphics_pipelines.resize(bake_items.size() );

    m_pipeline_cache_ptr->lock();
    {
        result_vk = Anvil::Vulkan::vkCreateGraphicsPipelines(m_device_ptr->get_device_vk(),
                                                             m_pipeline_cache_ptr->get_pipeline_cache(),
                                                             graphics_pipeline_create_info_chains.get_n_structs   (),
                                                             graphics_pipeline_create_info_chains.get_root_structs(),
                                                             nullptr, /* pAllocator */
                                                            &result_graphics_pipelines[0]);
    }
    m_pipeline_cache_ptr->unlock();

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert_vk_call_succeeded(result_vk);

        goto end;
    }

    /* Distribute the result pipeline objects to pipeline configuration descriptors */
    for (auto pipeline_iterator  = m_outstanding_pipelines.begin();
              pipeline_iterator != m_outstanding_pipelines.end();
            ++pipeline_iterator)
    {
        const PipelineID& current_pipeline_id = pipeline_iterator->first;

        anvil_assert(m_baked_pipelines.find(current_pipeline_id) == m_baked_pipelines.end() );

        pipeline_iterator->second->baked_pipeline = result_graphics_pipelines[n_consumed_graphics_pipelines++];
        m_baked_pipelines[current_pipeline_id]    = std::move(pipeline_iterator->second);
    }

    anvil_assert(n_consumed_graphics_pipelines == static_cast<uint32_t>(result_graphics_pipelines.size() ));

    m_outstanding_pipelines.clear();

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
Anvil::StructChainUniquePtr<VkGraphicsPipelineCreateInfo> Anvil::GraphicsPipelineManager::bake_graphics_pipeline_create_info(const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr,
                                                                                                                             const Anvil::PipelineLayout*                  in_pipeline_layout_ptr,
                                                                                                                             const VkPipeline&                             in_opt_base_pipeline_handle,
                                                                                                                             const int32_t&                                in_opt_base_pipeline_index,
                                                                                                                             const VkPipelineColorBlendStateCreateInfo*    in_opt_color_blend_state_create_info_ptr,
                                                                                                                             const VkPipelineDepthStencilStateCreateInfo*  in_opt_depth_stencil_state_create_info_ptr,
                                                                                                                             const VkPipelineDynamicStateCreateInfo*       in_opt_dynamic_state_create_info_ptr,
                                                                                                                             const VkPipelineInputAssemblyStateCreateInfo* in_input_assembly_state_create_info_ptr,
                                                                                                                             const VkPipelineMultisampleStateCreateInfo*   in_opt_multisample_state_create_info_ptr,
                                                                                                                             const VkPipelineRasterizationStateCreateInfo* in_rasterization_state_create_info_ptr,
                                                                                                                             const uint32_t&                               in_n_shader_stage_create_info_items,
                                                                                                                             const VkPipelineShaderStageCreateInfo*        in_shader_stage_create_info_items_ptr,
                                                                                                                             const VkPipelineTessellationStateCreateInfo*  in_opt_tessellation_state_create_info_ptr,
                                                                                                                             const VkPipelineVertexInputStateCreateInfo*   in_vertex_input_state_create_info_ptr,
                                                                                                                             const VkPipelineViewportStateCreateInfo*      in_opt_viewport_state_create_info_ptr) const
{
    Anvil::StructChainer<VkGraphicsPipelineCreateInfo> chainer;

    {
        VkGraphicsPipelineCreateInfo create_info;

        create_info.basePipelineHandle  = in_opt_base_pipeline_handle;
        create_info.basePipelineIndex   = in_opt_base_pipeline_index;
        create_info.flags               = 0;
        create_info.layout              = in_pipeline_layout_ptr->get_pipeline_layout();
        create_info.pColorBlendState    = in_opt_color_blend_state_create_info_ptr;
        create_info.pDepthStencilState  = in_opt_depth_stencil_state_create_info_ptr;
        create_info.pDynamicState       = in_opt_dynamic_state_create_info_ptr;
        create_info.pInputAssemblyState = in_input_assembly_state_create_info_ptr;
        create_info.pMultisampleState   = in_opt_multisample_state_create_info_ptr;
        create_info.pNext               = nullptr;
        create_info.pRasterizationState = in_rasterization_state_create_info_ptr;
        create_info.pStages             = in_shader_stage_create_info_items_ptr;
        create_info.pTessellationState  = in_opt_tessellation_state_create_info_ptr;
        create_info.pVertexInputState   = in_vertex_input_state_create_info_ptr;
        create_info.pViewportState      = in_opt_viewport_state_create_info_ptr;
        create_info.renderPass          = in_gfx_pipeline_create_info_ptr->get_renderpass()->get_render_pass();
        create_info.stageCount          = in_n_shader_stage_create_info_items;
        create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.subpass             = in_gfx_pipeline_create_info_ptr->get_subpass_id();

        if (create_info.basePipelineIndex != static_cast<int32_t>(UINT32_MAX) )
        {
            create_info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        }

        create_info.flags |= ((in_gfx_pipeline_create_info_ptr->allows_derivatives        () ) ? VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT    : 0) |
                             ((in_gfx_pipeline_create_info_ptr->has_optimizations_disabled() ) ? VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT : 0);

        chainer.append_struct(create_info);
    }

    return chainer.create_chain();
}

/* Please see header for specification */
Anvil::StructChainUniquePtr<VkPipelineColorBlendStateCreateInfo> Anvil::GraphicsPipelineManager::bake_pipeline_color_blend_state_create_info(const Anvil::GraphicsPipelineCreateInfo* in_gfx_pipeline_create_info_ptr,
                                                                                                                                             const Anvil::RenderPass*                 in_current_renderpass_ptr,
                                                                                                                                             const Anvil::SubPassID&                  in_subpass_id) const
{
    Anvil::StructChainUniquePtr<VkPipelineColorBlendStateCreateInfo> result_chain_ptr;
    uint32_t                                                         subpass_n_color_attachments = 0;

    in_current_renderpass_ptr->get_render_pass_create_info()->get_subpass_n_attachments(in_subpass_id,
                                                                                        Anvil::AttachmentType::COLOR,
                                                                                       &subpass_n_color_attachments);

    if (!in_gfx_pipeline_create_info_ptr->is_rasterizer_discard_enabled()     &&
         subpass_n_color_attachments                                       > 0)
    {
        const float*                                              blend_constant_ptr                      = nullptr;
        Anvil::StructChainer<VkPipelineColorBlendStateCreateInfo> color_blend_state_create_info_chainer;
        Anvil::StructID                                           color_blend_state_create_info_struct_id;
        Anvil::LogicOp                                            logic_op                                = Anvil::LogicOp::UNKNOWN;
        bool                                                      logic_op_enabled                        = false;
        uint32_t                                                  max_location_index                      = UINT32_MAX;

        max_location_index = in_current_renderpass_ptr->get_render_pass_create_info()->get_max_color_location_used_by_subpass(in_subpass_id);

        in_gfx_pipeline_create_info_ptr->get_blending_properties(&blend_constant_ptr,
                                                                 nullptr); /* out_opt_n_blend_attachments_ptr */

        in_gfx_pipeline_create_info_ptr->get_logic_op_state(&logic_op_enabled,
                                                            &logic_op);

        {
            VkPipelineColorBlendStateCreateInfo color_blend_state_create_info;

            color_blend_state_create_info.attachmentCount = max_location_index + 1;
            color_blend_state_create_info.flags           = 0;
            color_blend_state_create_info.logicOp         = static_cast<VkLogicOp>(logic_op);
            color_blend_state_create_info.logicOpEnable   = (logic_op_enabled) ? VK_TRUE : VK_FALSE;
            color_blend_state_create_info.pAttachments    = nullptr; /* will be patched by the chainer */
            color_blend_state_create_info.pNext           = nullptr;
            color_blend_state_create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

            memcpy(color_blend_state_create_info.blendConstants,
                   blend_constant_ptr,
                   sizeof(color_blend_state_create_info.blendConstants) );

            color_blend_state_create_info_struct_id = color_blend_state_create_info_chainer.append_struct(color_blend_state_create_info);
        }

        anvil_assert(subpass_n_color_attachments <= in_current_renderpass_ptr->get_render_pass_create_info()->get_n_attachments() );

        {
            std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachment_state_vec(max_location_index + 1);

            for (uint32_t n_subpass_color_attachment = 0;
                          n_subpass_color_attachment <= max_location_index;
                        ++n_subpass_color_attachment)
            {
                VkPipelineColorBlendAttachmentState* blend_state_ptr                    = &color_blend_attachment_state_vec.at(n_subpass_color_attachment);
                Anvil::ImageLayout                   dummy                              = Anvil::ImageLayout::UNKNOWN;
                bool                                 is_blending_enabled_for_attachment = false;
                Anvil::RenderPassAttachmentID        rp_attachment_id                   = UINT32_MAX;

                Anvil::BlendOp             alpha_blend_op         = Anvil::BlendOp::UNKNOWN;
                Anvil::BlendOp             color_blend_op         = Anvil::BlendOp::UNKNOWN;
                Anvil::ColorComponentFlags color_component_flags  = Anvil::ColorComponentFlagBits::NONE;
                Anvil::BlendFactor         dst_alpha_blend_factor = Anvil::BlendFactor::UNKNOWN;
                Anvil::BlendFactor         dst_color_blend_factor = Anvil::BlendFactor::UNKNOWN;
                Anvil::BlendFactor         src_alpha_blend_factor = Anvil::BlendFactor::UNKNOWN;
                Anvil::BlendFactor         src_color_blend_factor = Anvil::BlendFactor::UNKNOWN;

                if (!in_current_renderpass_ptr->get_render_pass_create_info()->get_subpass_attachment_properties(in_subpass_id,
                                                                                                                 Anvil::AttachmentType::COLOR,
                                                                                                                 n_subpass_color_attachment,
                                                                                                                &rp_attachment_id,
                                                                                                                &dummy) || /* out_layout_ptr */
                    !in_gfx_pipeline_create_info_ptr->get_color_blend_attachment_properties                      (rp_attachment_id,
                                                                                                                &is_blending_enabled_for_attachment,
                                                                                                                &color_blend_op,
                                                                                                                &alpha_blend_op,
                                                                                                                &src_color_blend_factor,
                                                                                                                &dst_color_blend_factor,
                                                                                                                &src_alpha_blend_factor,
                                                                                                                &dst_alpha_blend_factor,
                                                                                                                &color_component_flags) )
                {
                    /* The user has not defined blending properties for current color attachment. Use default state values .. */
                    blend_state_ptr->blendEnable         = VK_FALSE;
                    blend_state_ptr->alphaBlendOp        = VK_BLEND_OP_ADD;
                    blend_state_ptr->colorBlendOp        = VK_BLEND_OP_ADD;
                    blend_state_ptr->colorWriteMask      = VK_COLOR_COMPONENT_A_BIT |
                                                           VK_COLOR_COMPONENT_B_BIT |
                                                           VK_COLOR_COMPONENT_G_BIT |
                                                           VK_COLOR_COMPONENT_R_BIT;
                    blend_state_ptr->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    blend_state_ptr->dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                    blend_state_ptr->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    blend_state_ptr->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                }
                else
                {
                    blend_state_ptr->alphaBlendOp        = static_cast<VkBlendOp>(alpha_blend_op);
                    blend_state_ptr->blendEnable         = (is_blending_enabled_for_attachment) ? VK_TRUE : VK_FALSE;
                    blend_state_ptr->colorBlendOp        = static_cast<VkBlendOp>    (color_blend_op);
                    blend_state_ptr->colorWriteMask      = color_component_flags.get_vk();
                    blend_state_ptr->dstAlphaBlendFactor = static_cast<VkBlendFactor>(dst_alpha_blend_factor);
                    blend_state_ptr->dstColorBlendFactor = static_cast<VkBlendFactor>(dst_color_blend_factor);
                    blend_state_ptr->srcAlphaBlendFactor = static_cast<VkBlendFactor>(src_alpha_blend_factor);
                    blend_state_ptr->srcColorBlendFactor = static_cast<VkBlendFactor>(src_color_blend_factor);
                }
            }

            color_blend_state_create_info_chainer.store_helper_structure_vector(color_blend_attachment_state_vec,
                                                                                color_blend_state_create_info_struct_id,
                                                                                offsetof(VkPipelineColorBlendStateCreateInfo, pAttachments) );
        }

        result_chain_ptr = color_blend_state_create_info_chainer.create_chain();
    }

    return result_chain_ptr;
}

/* Please see header for specification */
Anvil::StructChainUniquePtr<VkPipelineDepthStencilStateCreateInfo> Anvil::GraphicsPipelineManager::bake_pipeline_depth_stencil_state_create_info(const Anvil::GraphicsPipelineCreateInfo* in_gfx_pipeline_create_info_ptr,
                                                                                                                                                 const Anvil::RenderPass*                 in_current_renderpass_ptr) const
{
    VkPipelineDepthStencilStateCreateInfo                              depth_stencil_state_create_info;
    auto                                                               depth_test_compare_op            = Anvil::CompareOp::UNKNOWN;
    bool                                                               is_depth_bounds_test_enabled     = false;
    bool                                                               is_depth_test_enabled            = false;
    bool                                                               is_stencil_test_enabled          = false;
    float                                                              max_depth_bounds                 = std::numeric_limits<float>::max();
    float                                                              min_depth_bounds                 = std::numeric_limits<float>::max();
    uint32_t                                                           n_depth_stencil_attachments      = 0;
    Anvil::StructChainUniquePtr<VkPipelineDepthStencilStateCreateInfo> result_ptr;

    in_gfx_pipeline_create_info_ptr->get_depth_bounds_state(&is_depth_bounds_test_enabled,
                                                            &min_depth_bounds,
                                                            &max_depth_bounds);

    in_gfx_pipeline_create_info_ptr->get_depth_test_state(&is_depth_test_enabled,
                                                          &depth_test_compare_op);

    {
        Anvil::CompareOp back_compare_op     = Anvil::CompareOp::UNKNOWN;
        Anvil::StencilOp back_depth_fail_op  = Anvil::StencilOp::UNKNOWN;
        Anvil::StencilOp back_fail_op        = Anvil::StencilOp::UNKNOWN;
        Anvil::StencilOp back_pass_op        = Anvil::StencilOp::UNKNOWN;
        Anvil::CompareOp front_compare_op    = Anvil::CompareOp::UNKNOWN;
        Anvil::StencilOp front_depth_fail_op = Anvil::StencilOp::UNKNOWN;
        Anvil::StencilOp front_fail_op       = Anvil::StencilOp::UNKNOWN;
        Anvil::StencilOp front_pass_op       = Anvil::StencilOp::UNKNOWN;

        in_gfx_pipeline_create_info_ptr->get_stencil_test_properties(&is_stencil_test_enabled,
                                                                     &front_fail_op,
                                                                     &front_pass_op,
                                                                     &front_depth_fail_op,
                                                                     &front_compare_op,
                                                                     &depth_stencil_state_create_info.front.compareMask,
                                                                     &depth_stencil_state_create_info.front.writeMask,
                                                                     &depth_stencil_state_create_info.front.reference,
                                                                     &back_fail_op,
                                                                     &back_pass_op,
                                                                     &back_depth_fail_op,
                                                                     &back_compare_op,
                                                                     &depth_stencil_state_create_info.back.compareMask,
                                                                     &depth_stencil_state_create_info.back.writeMask,
                                                                     &depth_stencil_state_create_info.back.reference);

        depth_stencil_state_create_info.back.compareOp    = static_cast<VkCompareOp>(back_compare_op);
        depth_stencil_state_create_info.back.depthFailOp  = static_cast<VkStencilOp>(back_depth_fail_op);
        depth_stencil_state_create_info.back.failOp       = static_cast<VkStencilOp>(back_fail_op);
        depth_stencil_state_create_info.back.passOp       = static_cast<VkStencilOp>(back_pass_op);
        depth_stencil_state_create_info.front.compareOp   = static_cast<VkCompareOp>(front_compare_op);
        depth_stencil_state_create_info.front.depthFailOp = static_cast<VkStencilOp>(front_depth_fail_op);
        depth_stencil_state_create_info.front.failOp      = static_cast<VkStencilOp>(front_fail_op);
        depth_stencil_state_create_info.front.passOp      = static_cast<VkStencilOp>(front_pass_op);
    }

    in_current_renderpass_ptr->get_render_pass_create_info()->get_subpass_n_attachments(in_gfx_pipeline_create_info_ptr->get_subpass_id(),
                                                                                        Anvil::AttachmentType::DEPTH_STENCIL,
                                                                                       &n_depth_stencil_attachments);

    if (n_depth_stencil_attachments)
    {
        Anvil::StructChainer<VkPipelineDepthStencilStateCreateInfo> depth_stencil_state_create_info_chainer;

        depth_stencil_state_create_info.depthBoundsTestEnable = is_depth_bounds_test_enabled ? VK_TRUE : VK_FALSE;
        depth_stencil_state_create_info.depthCompareOp        = static_cast<VkCompareOp>(depth_test_compare_op);
        depth_stencil_state_create_info.depthTestEnable       = is_depth_test_enabled                                       ? VK_TRUE : VK_FALSE;
        depth_stencil_state_create_info.depthWriteEnable      = in_gfx_pipeline_create_info_ptr->are_depth_writes_enabled() ? VK_TRUE : VK_FALSE;
        depth_stencil_state_create_info.flags                 = 0;
        depth_stencil_state_create_info.maxDepthBounds        = max_depth_bounds;
        depth_stencil_state_create_info.minDepthBounds        = min_depth_bounds;
        depth_stencil_state_create_info.pNext                 = nullptr;
        depth_stencil_state_create_info.stencilTestEnable     = is_stencil_test_enabled ? VK_TRUE : VK_FALSE;
        depth_stencil_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        depth_stencil_state_create_info_chainer.append_struct(depth_stencil_state_create_info);

        result_ptr = depth_stencil_state_create_info_chainer.create_chain();
    }
    else
    {
        /* No depth/stencil attachment available. Make sure none of the dependent modes are enabled. */
        anvil_assert(!is_depth_bounds_test_enabled);
        anvil_assert(!is_depth_test_enabled);
        anvil_assert(!in_gfx_pipeline_create_info_ptr->are_depth_writes_enabled());
        anvil_assert(!is_stencil_test_enabled);
    }

    return result_ptr;
}

/* Please see header for specification */
Anvil::StructChainUniquePtr<VkPipelineDynamicStateCreateInfo> Anvil::GraphicsPipelineManager::bake_pipeline_dynamic_state_create_info(const Anvil::GraphicsPipelineCreateInfo* in_gfx_pipeline_create_info_ptr) const
{
    const Anvil::DynamicState*                                    enabled_dynamic_states_ptr = nullptr;
    uint32_t                                                      n_enabled_dynamic_states   = 0;
    Anvil::StructChainUniquePtr<VkPipelineDynamicStateCreateInfo> result_ptr;

    in_gfx_pipeline_create_info_ptr->get_enabled_dynamic_states(&enabled_dynamic_states_ptr,
                                                                &n_enabled_dynamic_states);

    if (n_enabled_dynamic_states != 0)
    {
        Anvil::StructChainer<VkPipelineDynamicStateCreateInfo> dynamic_state_create_info_chainer;
        VkPipelineDynamicStateCreateInfo                       dynamic_state_create_info;

        dynamic_state_create_info.dynamicStateCount = n_enabled_dynamic_states;
        dynamic_state_create_info.flags             = 0;
        dynamic_state_create_info.pDynamicStates    = (dynamic_state_create_info.dynamicStateCount > 0) ? reinterpret_cast<const VkDynamicState*>(enabled_dynamic_states_ptr)
                                                                                                        : VK_NULL_HANDLE;
        dynamic_state_create_info.pNext             = nullptr;
        dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

        dynamic_state_create_info_chainer.append_struct(dynamic_state_create_info);

        result_ptr = dynamic_state_create_info_chainer.create_chain();
    }

    return result_ptr;
}

/* Please see header for specification */
Anvil::StructChainUniquePtr<VkPipelineInputAssemblyStateCreateInfo> Anvil::GraphicsPipelineManager::bake_pipeline_input_assembly_state_create_info(const Anvil::GraphicsPipelineCreateInfo* in_gfx_pipeline_create_info_ptr) const
{
    Anvil::StructChainer<VkPipelineInputAssemblyStateCreateInfo> input_assembly_state_create_info_chainer;
    VkPipelineInputAssemblyStateCreateInfo                       input_assembly_state_create_info;

    input_assembly_state_create_info.flags                  = 0;
    input_assembly_state_create_info.pNext                  = nullptr;
    input_assembly_state_create_info.primitiveRestartEnable = in_gfx_pipeline_create_info_ptr->is_primitive_restart_enabled() ? VK_TRUE : VK_FALSE;
    input_assembly_state_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology               = static_cast<VkPrimitiveTopology>(in_gfx_pipeline_create_info_ptr->get_primitive_topology() );

    input_assembly_state_create_info_chainer.append_struct(input_assembly_state_create_info);

    return input_assembly_state_create_info_chainer.create_chain();
}

/* Please see header for specification */
Anvil::StructChainUniquePtr<VkPipelineMultisampleStateCreateInfo> Anvil::GraphicsPipelineManager::bake_pipeline_multisample_state_create_info(const Anvil::GraphicsPipelineCreateInfo* in_gfx_pipeline_create_info_ptr) const
{
    bool                                                              is_sample_shading_enabled = false;
    float                                                             min_sample_shading        = std::numeric_limits<float>::max();
    Anvil::StructChainUniquePtr<VkPipelineMultisampleStateCreateInfo> result_ptr;

    in_gfx_pipeline_create_info_ptr->get_sample_shading_state(&is_sample_shading_enabled,
                                                              &min_sample_shading);

    if (!in_gfx_pipeline_create_info_ptr->is_rasterizer_discard_enabled() )
    {
        bool                                                       are_custom_sample_locations_enabled = false;
        VkExtent2D                                                 custom_sample_location_grid_size    = {0, 0};
        Anvil::SampleCountFlagBits                                 custom_sample_locations_per_pixel   = Anvil::SampleCountFlagBits::NONE;
        const Anvil::SampleLocation*                               custom_sample_locations_ptr         = nullptr;
        Anvil::StructChainer<VkPipelineMultisampleStateCreateInfo> chainer;
        const bool                                                 is_sample_mask_enabled              = in_gfx_pipeline_create_info_ptr->is_sample_mask_enabled();
        uint32_t                                                   n_custom_sample_locations           = 0;

        Anvil::SampleCountFlagBits sample_count    = static_cast<Anvil::SampleCountFlagBits>(0);
        const VkSampleMask*        sample_mask_ptr = nullptr;

        in_gfx_pipeline_create_info_ptr->get_multisampling_properties(&sample_count,
                                                                      &sample_mask_ptr);
        in_gfx_pipeline_create_info_ptr->get_sample_location_state   (&are_custom_sample_locations_enabled,
                                                                      &custom_sample_locations_per_pixel,
                                                                      &custom_sample_location_grid_size,
                                                                      &n_custom_sample_locations,
                                                                      &custom_sample_locations_ptr);

        /* If sample mask is not enabled, Vulkan spec will assume all samples need to pass. This is what the default sample mask value mimics.
         *
         * Hence, if the application specified a non-~0u sample mask and has NOT enabled the sample mask using toggle_sample_mask(), it's (in
         * all likelihood) a trivial app-side issue.
         */
        anvil_assert((!is_sample_mask_enabled && *sample_mask_ptr == ~0u) ||
                       is_sample_mask_enabled);

        {
            VkPipelineMultisampleStateCreateInfo multisample_state_create_info;

            multisample_state_create_info.alphaToCoverageEnable = in_gfx_pipeline_create_info_ptr->is_alpha_to_coverage_enabled() ? VK_TRUE : VK_FALSE;
            multisample_state_create_info.alphaToOneEnable      = in_gfx_pipeline_create_info_ptr->is_alpha_to_one_enabled()      ? VK_TRUE : VK_FALSE;
            multisample_state_create_info.flags                 = 0;
            multisample_state_create_info.minSampleShading      = min_sample_shading;
            multisample_state_create_info.pNext                 = nullptr;
            multisample_state_create_info.pSampleMask           = (is_sample_mask_enabled) ? sample_mask_ptr : nullptr;
            multisample_state_create_info.rasterizationSamples  = static_cast<VkSampleCountFlagBits>(sample_count);
            multisample_state_create_info.sampleShadingEnable   = is_sample_shading_enabled ? VK_TRUE : VK_FALSE;
            multisample_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

            chainer.append_struct(multisample_state_create_info);
        }

        if (are_custom_sample_locations_enabled)
        {
            VkPipelineSampleLocationsStateCreateInfoEXT psl_state_create_info;
            VkSampleLocationsInfoEXT                    sample_locations_info;

            sample_locations_info.pNext                   = nullptr;
            sample_locations_info.pSampleLocations        = reinterpret_cast<const VkSampleLocationEXT*>(custom_sample_locations_ptr);
            sample_locations_info.sampleLocationGridSize  = custom_sample_location_grid_size;
            sample_locations_info.sampleLocationsCount    = n_custom_sample_locations;
            sample_locations_info.sampleLocationsPerPixel = static_cast<VkSampleCountFlagBits>(custom_sample_locations_per_pixel);
            sample_locations_info.sType                   = VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;

            psl_state_create_info.pNext                 = nullptr;
            psl_state_create_info.sampleLocationsEnable = VK_TRUE;
            psl_state_create_info.sampleLocationsInfo   = sample_locations_info;
            psl_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT;

            anvil_assert(m_device_ptr->get_extension_info()->ext_sample_locations() );

            chainer.append_struct(psl_state_create_info);
        }

        result_ptr = chainer.create_chain();
    }
    else
    {
        /* Make sure any dependent modes are disabled */
        anvil_assert(!in_gfx_pipeline_create_info_ptr->is_alpha_to_coverage_enabled() );
        anvil_assert(!in_gfx_pipeline_create_info_ptr->is_alpha_to_one_enabled     () );
        anvil_assert(!is_sample_shading_enabled);
    }

    return result_ptr;
}

Anvil::StructChainUniquePtr<VkPipelineRasterizationStateCreateInfo> Anvil::GraphicsPipelineManager::bake_pipeline_rasterization_state_create_info(const Anvil::GraphicsPipelineCreateInfo* in_gfx_pipeline_create_info_ptr) const
{
    Anvil::CullModeFlags                                         cull_mode                  = Anvil::CullModeFlagBits::NONE;
    float                                                        depth_bias_clamp           = std::numeric_limits<float>::max();
    float                                                        depth_bias_constant_factor = std::numeric_limits<float>::max();
    float                                                        depth_bias_slope_factor    = std::numeric_limits<float>::max();
    Anvil::FrontFace                                             front_face                 = Anvil::FrontFace::UNKNOWN;
    bool                                                         is_depth_bias_enabled      = false;
    float                                                        line_width                 = std::numeric_limits<float>::max();
    Anvil::PolygonMode                                           polygon_mode               = Anvil::PolygonMode::UNKNOWN;
    Anvil::StructChainer<VkPipelineRasterizationStateCreateInfo> raster_state_create_info_chainer;

    static const VkPipelineRasterizationStateRasterizationOrderAMD relaxed_rasterization_order_item =
    {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD,
        nullptr,
        VK_RASTERIZATION_ORDER_RELAXED_AMD
    };
    static const VkPipelineRasterizationStateRasterizationOrderAMD strict_rasterization_order_item =
    {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD,
        nullptr,
        VK_RASTERIZATION_ORDER_STRICT_AMD
    };


    in_gfx_pipeline_create_info_ptr->get_depth_bias_state        (&is_depth_bias_enabled,
                                                                   &depth_bias_constant_factor,
                                                                   &depth_bias_clamp,
                                                                   &depth_bias_slope_factor);
    in_gfx_pipeline_create_info_ptr->get_rasterization_properties(&polygon_mode,
                                                                   &cull_mode,
                                                                   &front_face,
                                                                   &line_width);

    {
        VkPipelineRasterizationStateCreateInfo raster_state_create_info;

        raster_state_create_info.cullMode                = cull_mode.get_vk();
        raster_state_create_info.depthBiasClamp          = depth_bias_clamp;
        raster_state_create_info.depthBiasConstantFactor = depth_bias_constant_factor;
        raster_state_create_info.depthBiasEnable         = is_depth_bias_enabled ? VK_TRUE : VK_FALSE;
        raster_state_create_info.depthBiasSlopeFactor    = depth_bias_slope_factor;
        raster_state_create_info.depthClampEnable        = in_gfx_pipeline_create_info_ptr->is_depth_clamp_enabled() ? VK_TRUE : VK_FALSE;
        raster_state_create_info.flags                   = 0;
        raster_state_create_info.frontFace               = static_cast<VkFrontFace>(front_face);
        raster_state_create_info.lineWidth               = line_width;
        raster_state_create_info.pNext                   = nullptr;
        raster_state_create_info.polygonMode             = static_cast<VkPolygonMode>(polygon_mode);
        raster_state_create_info.rasterizerDiscardEnable = in_gfx_pipeline_create_info_ptr->is_rasterizer_discard_enabled() ? VK_TRUE : VK_FALSE;
        raster_state_create_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        raster_state_create_info_chainer.append_struct(raster_state_create_info);
    }

    if (m_device_ptr->is_extension_enabled(VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME) )
    {
        /* Chain a predefined struct which will toggle the relaxed rasterization, as long as the device supports the
         * VK_AMD_rasterization_order extension.
         */
        if (in_gfx_pipeline_create_info_ptr->get_rasterization_order() != Anvil::RasterizationOrderAMD::STRICT)
        {
            anvil_assert(in_gfx_pipeline_create_info_ptr->get_rasterization_order() == static_cast<Anvil::RasterizationOrderAMD>(relaxed_rasterization_order_item.rasterizationOrder) );

            raster_state_create_info_chainer.append_struct(relaxed_rasterization_order_item);
        }
        else
        {
            raster_state_create_info_chainer.append_struct(strict_rasterization_order_item);
        }
    }
    else
    {
        anvil_assert(in_gfx_pipeline_create_info_ptr->get_rasterization_order() == Anvil::RasterizationOrderAMD::STRICT);
    }

    if (m_device_ptr->is_extension_enabled(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME) )
    {
        /* Chain a predefined struct which will toggle the conservative rasterization mode, as long as the device supports the
         * VK_EXT_conservative_rasterization extension.
         */
        const VkPipelineRasterizationConservativeStateCreateInfoEXT conservative_rasterization_item
        {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT,
            nullptr,
            0,
            static_cast<VkConservativeRasterizationModeEXT>(in_gfx_pipeline_create_info_ptr->get_conservative_rasterization_mode() ),
            in_gfx_pipeline_create_info_ptr->get_extra_primitive_overestimation_size()
        };

        raster_state_create_info_chainer.append_struct(conservative_rasterization_item);
    }
    else
    {
        anvil_assert(in_gfx_pipeline_create_info_ptr->get_conservative_rasterization_mode() == Anvil::ConservativeRasterizationModeEXT::DISABLED);
    }

    if (m_device_ptr->is_extension_enabled(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME) )
    {
        const bool is_depth_clip_enabled = in_gfx_pipeline_create_info_ptr->is_depth_clip_enabled();

        /* NOTE: No need to chain the struct IF ::depthClipEnable is to be specified as VK_TRUE, since it does not affect Vulkan impl's behavior. */
        if (!is_depth_clip_enabled)
        {
            VkPipelineRasterizationDepthClipStateCreateInfoEXT create_info;

            create_info.depthClipEnable = VK_FALSE;
            create_info.flags           = 0;
            create_info.pNext           = nullptr;
            create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;

            raster_state_create_info_chainer.append_struct(create_info);
        }
    }

    if (m_device_ptr->is_extension_enabled(VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME) )
    {
        const auto& rasterization_stream_index = in_gfx_pipeline_create_info_ptr->get_rasterization_stream_index();

        if (rasterization_stream_index != 0)
        {
            VkPipelineRasterizationStateStreamCreateInfoEXT create_info;

            create_info.flags               = 0;
            create_info.pNext               = nullptr;
            create_info.rasterizationStream = rasterization_stream_index;
            create_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT;

            raster_state_create_info_chainer.append_struct(create_info);
        }
    }
    else
    {
        anvil_assert(in_gfx_pipeline_create_info_ptr->get_rasterization_stream_index() == 0);
    }

    return raster_state_create_info_chainer.create_chain();
}

/* Please see header for specification */
std::unique_ptr<Anvil::StructChainVector<VkPipelineShaderStageCreateInfo> > Anvil::GraphicsPipelineManager::bake_pipeline_shader_stage_create_info_chain_vector(const Anvil::GraphicsPipelineCreateInfo* in_gfx_pipeline_create_info_ptr) const
{
    static const Anvil::ShaderStage graphics_shader_stages[] =
    {
        Anvil::ShaderStage::FRAGMENT,
        Anvil::ShaderStage::GEOMETRY,
        Anvil::ShaderStage::TESSELLATION_CONTROL,
        Anvil::ShaderStage::TESSELLATION_EVALUATION,
        Anvil::ShaderStage::VERTEX
    };

    std::unique_ptr<Anvil::StructChainVector<VkPipelineShaderStageCreateInfo> > shader_stage_create_info_chainer_ptr(new Anvil::StructChainVector<VkPipelineShaderStageCreateInfo>() );


    for (const auto& current_graphics_shader_stage : graphics_shader_stages)
    {
        const Anvil::ShaderModuleStageEntryPoint* shader_stage_entry_point_ptr = nullptr;

        in_gfx_pipeline_create_info_ptr->get_shader_stage_properties(current_graphics_shader_stage,
                                                                    &shader_stage_entry_point_ptr);

        if (shader_stage_entry_point_ptr != nullptr)
        {
            const unsigned char*           current_shader_stage_specialization_constants_data_buffer_ptr = nullptr;
            const SpecializationConstants* current_shader_stage_specialization_constants_ptr             = nullptr;
            Anvil::ShaderModule*           shader_module_ptr                                             = shader_stage_entry_point_ptr->shader_module_ptr;

            in_gfx_pipeline_create_info_ptr->get_specialization_constants(current_graphics_shader_stage,
                                                                         &current_shader_stage_specialization_constants_ptr,
                                                                         &current_shader_stage_specialization_constants_data_buffer_ptr);

            {
                Anvil::StructID                                       root_struct_id;
                Anvil::StructChainer<VkPipelineShaderStageCreateInfo> shader_stage_create_info_chainer;

                {
                    VkPipelineShaderStageCreateInfo shader_stage_create_info;

                    shader_stage_create_info.module = shader_module_ptr->get_module();
                    shader_stage_create_info.pName  = (shader_stage_entry_point_ptr->stage == Anvil::ShaderStage::FRAGMENT)                ? shader_module_ptr->get_fs_entrypoint_name().c_str()
                                                    : (shader_stage_entry_point_ptr->stage == Anvil::ShaderStage::GEOMETRY)                ? shader_module_ptr->get_gs_entrypoint_name().c_str()
                                                    : (shader_stage_entry_point_ptr->stage == Anvil::ShaderStage::TESSELLATION_CONTROL)    ? shader_module_ptr->get_tc_entrypoint_name().c_str()
                                                    : (shader_stage_entry_point_ptr->stage == Anvil::ShaderStage::TESSELLATION_EVALUATION) ? shader_module_ptr->get_te_entrypoint_name().c_str()
                                                    : (shader_stage_entry_point_ptr->stage == Anvil::ShaderStage::VERTEX)                  ? shader_module_ptr->get_vs_entrypoint_name().c_str()
                                                    : nullptr;

                    shader_stage_create_info.flags               = 0;
                    shader_stage_create_info.pNext               = nullptr;
                    shader_stage_create_info.pSpecializationInfo = nullptr; /* patched by the chainer below */
                    shader_stage_create_info.stage               = static_cast<VkShaderStageFlagBits>(Anvil::Utils::get_shader_stage_flag_bits_from_shader_stage(shader_stage_entry_point_ptr->stage) );
                    shader_stage_create_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

                    root_struct_id = shader_stage_create_info_chainer.append_struct(shader_stage_create_info);
                }

                if (current_shader_stage_specialization_constants_ptr->size() > 0)
                {
                    VkSpecializationInfo                  specialization_info;
                    std::vector<VkSpecializationMapEntry> specialization_map_entries;

                    if (current_shader_stage_specialization_constants_ptr->size() > 0)
                    {
                        bake_specialization_info_vk(*current_shader_stage_specialization_constants_ptr,
                                                     current_shader_stage_specialization_constants_data_buffer_ptr,
                                                    &specialization_map_entries,
                                                    &specialization_info);
                    }

                    shader_stage_create_info_chainer.store_helper_structure       (specialization_info,
                                                                                   root_struct_id,
                                                                                   offsetof(VkPipelineShaderStageCreateInfo, pSpecializationInfo) );
                    shader_stage_create_info_chainer.store_helper_structure_vector(specialization_map_entries,
                                                                                   root_struct_id,
                                                                                   offsetof(VkSpecializationInfo, pMapEntries) );
                }

                shader_stage_create_info_chainer_ptr->append_struct_chain(shader_stage_create_info_chainer.create_chain() );
            }
        }
    }

    return shader_stage_create_info_chainer_ptr;
}

/* Please see header for specification */
Anvil::StructChainUniquePtr<VkPipelineTessellationStateCreateInfo> Anvil::GraphicsPipelineManager::bake_pipeline_tessellation_state_create_info(const Anvil::GraphicsPipelineCreateInfo* in_gfx_pipeline_create_info_ptr) const
{
    Anvil::StructChainUniquePtr<VkPipelineTessellationStateCreateInfo> result_ptr;
    const Anvil::ShaderModuleStageEntryPoint*                          tc_shader_stage_entry_point_ptr = nullptr;
    const Anvil::ShaderModuleStageEntryPoint*                          te_shader_stage_entry_point_ptr = nullptr;

    in_gfx_pipeline_create_info_ptr->get_shader_stage_properties(Anvil::ShaderStage::TESSELLATION_CONTROL,
                                                                 &tc_shader_stage_entry_point_ptr);
    in_gfx_pipeline_create_info_ptr->get_shader_stage_properties(Anvil::ShaderStage::TESSELLATION_EVALUATION,
                                                                 &te_shader_stage_entry_point_ptr);

    if (tc_shader_stage_entry_point_ptr != nullptr &&
        te_shader_stage_entry_point_ptr != nullptr)
    {
        Anvil::StructChainer<VkPipelineTessellationStateCreateInfo> tessellation_state_create_info_chainer;

        {
            VkPipelineTessellationStateCreateInfo tessellation_state_create_info;

            tessellation_state_create_info.flags              = 0;
            tessellation_state_create_info.patchControlPoints = in_gfx_pipeline_create_info_ptr->get_n_patch_control_points();
            tessellation_state_create_info.pNext              = nullptr;
            tessellation_state_create_info.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

            tessellation_state_create_info_chainer.append_struct(tessellation_state_create_info);
        }

        if (m_device_ptr->is_extension_enabled(VK_KHR_MAINTENANCE2_EXTENSION_NAME) )
        {
            VkPipelineTessellationDomainOriginStateCreateInfoKHR domain_origin_state_create_info;

            domain_origin_state_create_info.domainOrigin = static_cast<VkTessellationDomainOriginKHR>(in_gfx_pipeline_create_info_ptr->get_tessellation_domain_origin() );
            domain_origin_state_create_info.pNext        = nullptr;
            domain_origin_state_create_info.sType        = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO_KHR;

            tessellation_state_create_info_chainer.append_struct(domain_origin_state_create_info);
        }
        else
        {
            /* App wants to adjust the default tessellation domain origin value, even though KHR_maintenance2 is unsupported! */
            anvil_assert(in_gfx_pipeline_create_info_ptr->get_tessellation_domain_origin() == Anvil::TessellationDomainOrigin::UPPER_LEFT);
        }

        result_ptr = tessellation_state_create_info_chainer.create_chain();
    }

    return result_ptr;
}

/* Please see header for specification */
Anvil::StructChainUniquePtr<VkPipelineVertexInputStateCreateInfo> Anvil::GraphicsPipelineManager::bake_pipeline_vertex_input_state_create_info(const Anvil::GraphicsPipelineCreateInfo* in_gfx_pipeline_create_info_ptr) const
{
    GraphicsPipelineData                                       current_pipeline_gfx_data             (in_gfx_pipeline_create_info_ptr);
    Anvil::StructChainer<VkPipelineVertexInputStateCreateInfo> vertex_input_state_create_info_chainer;

    {
        VkPipelineVertexInputStateCreateInfo create_info;
        Anvil::StructID                      root_struct_id;

        create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(current_pipeline_gfx_data.vk_input_attributes.size());
        create_info.vertexBindingDescriptionCount   = static_cast<uint32_t>(current_pipeline_gfx_data.vk_input_bindings.size  ());

        create_info.flags                        = 0;
        create_info.pNext                        = nullptr;
        create_info.pVertexAttributeDescriptions = nullptr; /* will be patched by the chainer */
        create_info.pVertexBindingDescriptions   = nullptr; /* will be patched by the chainer */
        create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        root_struct_id = vertex_input_state_create_info_chainer.append_struct(create_info);

        if (create_info.vertexAttributeDescriptionCount > 0)
        {
            vertex_input_state_create_info_chainer.store_helper_structure_vector(current_pipeline_gfx_data.vk_input_attributes,
                                                                                 root_struct_id,
                                                                                 offsetof(VkPipelineVertexInputStateCreateInfo, pVertexAttributeDescriptions) );
        }

        if (create_info.vertexBindingDescriptionCount > 0)
        {
            vertex_input_state_create_info_chainer.store_helper_structure_vector(current_pipeline_gfx_data.vk_input_bindings,
                                                                                 root_struct_id,
                                                                                 offsetof(VkPipelineVertexInputStateCreateInfo, pVertexBindingDescriptions) );
        }
    }

    if (current_pipeline_gfx_data.input_bindings.size() > 0)
    {
        std::vector<VkVertexInputBindingDivisorDescriptionEXT> divisor_descriptors;

        divisor_descriptors.reserve(current_pipeline_gfx_data.input_bindings.size() );

        for (const auto& current_binding_info : current_pipeline_gfx_data.input_bindings)
        {
            /* Make sure to chain VkVertexInputBindingDivisorDescriptionEXT for each binding which uses a non-default divisor value */
            if (current_binding_info.divisor != 1)
            {
                VkVertexInputBindingDivisorDescriptionEXT divisor_info;

                divisor_info.binding = current_binding_info.binding;
                divisor_info.divisor = current_binding_info.divisor;

                divisor_descriptors.push_back(divisor_info);
            }
        }

        if (divisor_descriptors.size() != 0)
        {
            VkPipelineVertexInputDivisorStateCreateInfoEXT divisor_state_create_info;
            Anvil::StructID                                divisor_state_struct_id;

            divisor_state_create_info.pNext                     = nullptr;
            divisor_state_create_info.pVertexBindingDivisors    = nullptr; /* NOTE: This field is going to be patched by vertex_input_state_create_info_chainer ! */
            divisor_state_create_info.sType                     = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;
            divisor_state_create_info.vertexBindingDivisorCount = static_cast<uint32_t>(divisor_descriptors.size() );

            divisor_state_struct_id = vertex_input_state_create_info_chainer.append_struct(divisor_state_create_info);

            vertex_input_state_create_info_chainer.store_helper_structure_vector(divisor_descriptors,
                                                                                 divisor_state_struct_id,
                                                                                 offsetof(VkPipelineVertexInputDivisorStateCreateInfoEXT, pVertexBindingDivisors) );
        }
    }

    return vertex_input_state_create_info_chainer.create_chain();
}

/* Please see header for specification */
Anvil::StructChainUniquePtr<VkPipelineViewportStateCreateInfo> Anvil::GraphicsPipelineManager::bake_pipeline_viewport_state_create_info(Anvil::GraphicsPipelineCreateInfo* in_gfx_pipeline_create_info_ptr,
                                                                                                                                        const bool&                        in_is_dynamic_scissor_state_enabled,
                                                                                                                                        const bool&                        in_is_dynamic_viewport_state_enabled) const
{
    Anvil::StructChainUniquePtr<VkPipelineViewportStateCreateInfo> result_ptr;

    if (!in_gfx_pipeline_create_info_ptr->is_rasterizer_discard_enabled() )
    {
        uint32_t                n_scissor_boxes = (in_is_dynamic_scissor_state_enabled)  ? in_gfx_pipeline_create_info_ptr->get_n_dynamic_scissor_boxes()
                                                                                         : in_gfx_pipeline_create_info_ptr->get_n_scissor_boxes        ();
        uint32_t                n_viewports     = (in_is_dynamic_viewport_state_enabled) ? in_gfx_pipeline_create_info_ptr->get_n_dynamic_viewports    ()
                                                                                         : in_gfx_pipeline_create_info_ptr->get_n_viewports            ();
        std::vector<VkRect2D>   scissor_boxes;
        std::vector<VkViewport> viewports;

        anvil_assert(n_scissor_boxes == n_viewports);

        if (n_scissor_boxes == 0)
        {
            /* No scissor boxes / viewport defined. Use default settings.. */
            auto     swapchain_ptr  = in_gfx_pipeline_create_info_ptr->get_renderpass()->get_swapchain();
            uint32_t window_size[2] = {0};

            /* NOTE: If you hit this assertion, you either need to pass a Swapchain instance when this renderpass is being created,
             *       *or* specify scissor & viewport information for the GFX pipeline.
             */
            anvil_assert(swapchain_ptr != nullptr);
            anvil_assert(n_viewports   == 0);

            swapchain_ptr->get_image(0)->get_image_mipmap_size(0,              /* n_mipmap */
                                                               window_size + 0,
                                                               window_size + 1,
                                                               nullptr);       /* out_opt_depth_ptr */

            anvil_assert(window_size[0] != 0 &&
                         window_size[1] != 0);

            in_gfx_pipeline_create_info_ptr->set_scissor_box_properties(0, /* in_n_scissor_box */
                                                                        0, /* in_x             */
                                                                        0, /* in_y             */
                                                                        window_size[0],
                                                                        window_size[1]);
            in_gfx_pipeline_create_info_ptr->set_viewport_properties   (0,    /* in_n_viewport */
                                                                        0.0f, /* in_origin_x   */
                                                                        0.0f, /* in_origin_y   */
                                                                        static_cast<float>(window_size[0]),
                                                                        static_cast<float>(window_size[1]),
                                                                        0.0f,  /* in_min_depth */
                                                                        1.0f); /* in_max_depth */

            n_scissor_boxes = 1;
            n_viewports     = 1;
        }

        /* Convert internal scissor box & viewport representations to Vulkan descriptors */
        if (!in_is_dynamic_scissor_state_enabled)
        {
            for (uint32_t n_scissor_box = 0;
                          n_scissor_box < n_scissor_boxes;
                        ++n_scissor_box)
            {
                uint32_t current_scissor_box_height = UINT32_MAX;
                uint32_t current_scissor_box_width  = UINT32_MAX;
                int32_t  current_scissor_box_x      = INT32_MAX;
                int32_t  current_scissor_box_y      = INT32_MAX;
                VkRect2D current_scissor_box_vk;

                in_gfx_pipeline_create_info_ptr->get_scissor_box_properties(n_scissor_box,
                                                                            &current_scissor_box_x,
                                                                            &current_scissor_box_y,
                                                                            &current_scissor_box_width,
                                                                            &current_scissor_box_height);
                current_scissor_box_vk.extent.height = current_scissor_box_height;
                current_scissor_box_vk.extent.width  = current_scissor_box_width;
                current_scissor_box_vk.offset.x      = current_scissor_box_x;
                current_scissor_box_vk.offset.y      = current_scissor_box_y;

                scissor_boxes.push_back(current_scissor_box_vk);
            }
        }

        if (!in_is_dynamic_viewport_state_enabled)
        {
            for (uint32_t n_viewport = 0;
                          n_viewport < n_viewports;
                        ++n_viewport)
            {
                float      current_viewport_height    = std::numeric_limits<float>::max();
                float      current_viewport_max_depth = std::numeric_limits<float>::max();
                float      current_viewport_min_depth = std::numeric_limits<float>::max();
                float      current_viewport_origin_x  = std::numeric_limits<float>::max();
                float      current_viewport_origin_y  = std::numeric_limits<float>::max();
                float      current_viewport_width     = std::numeric_limits<float>::max();
                VkViewport current_viewport_vk;

                in_gfx_pipeline_create_info_ptr->get_viewport_properties(n_viewport,
                                                                        &current_viewport_origin_x,
                                                                        &current_viewport_origin_y,
                                                                        &current_viewport_width,
                                                                        &current_viewport_height,
                                                                        &current_viewport_min_depth,
                                                                        &current_viewport_max_depth);

                current_viewport_vk.height   = current_viewport_height;
                current_viewport_vk.maxDepth = current_viewport_max_depth;
                current_viewport_vk.minDepth = current_viewport_min_depth;
                current_viewport_vk.x        = current_viewport_origin_x;
                current_viewport_vk.y        = current_viewport_origin_y;
                current_viewport_vk.width    = current_viewport_width;

                viewports.push_back(current_viewport_vk);
            }
        }

        /* Bake the descriptor */
        {
            Anvil::StructID                                         root_struct_id;
            Anvil::StructChainer<VkPipelineViewportStateCreateInfo> viewport_state_create_info_chainer;

            {
                VkPipelineViewportStateCreateInfo viewport_state_create_info;

                viewport_state_create_info.flags         = 0;
                viewport_state_create_info.pNext         = nullptr; /* will be patched by the chainer below */
                viewport_state_create_info.pScissors     = nullptr; /* will be patched by the chainer below */
                viewport_state_create_info.pViewports    = nullptr; /* will be patched by the chainer below */
                viewport_state_create_info.scissorCount  = n_scissor_boxes;
                viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewport_state_create_info.viewportCount = n_viewports;

                anvil_assert(viewport_state_create_info.scissorCount == viewport_state_create_info.viewportCount);
                anvil_assert(viewport_state_create_info.scissorCount != 0);

                root_struct_id = viewport_state_create_info_chainer.append_struct(viewport_state_create_info);
            }

            if (!in_is_dynamic_scissor_state_enabled)
            {
                viewport_state_create_info_chainer.store_helper_structure_vector(scissor_boxes,
                                                                                 root_struct_id,
                                                                                 offsetof(VkPipelineViewportStateCreateInfo, pScissors) );
            }

            if (!in_is_dynamic_viewport_state_enabled)
            {
                viewport_state_create_info_chainer.store_helper_structure_vector(viewports,
                                                                                 root_struct_id,
                                                                                 offsetof(VkPipelineViewportStateCreateInfo, pViewports) );
            }

            result_ptr = viewport_state_create_info_chainer.create_chain();
            anvil_assert(result_ptr != nullptr);
        }
    }

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::delete_pipeline(PipelineID in_pipeline_id)
{
    bool result;

    lock();
    {
        result = BasePipelineManager::delete_pipeline(in_pipeline_id);

        if (result)
        {
            auto baked_pipelines_iterator = m_baked_pipelines.find(in_pipeline_id);

            if (baked_pipelines_iterator != m_baked_pipelines.end() )
            {
                m_baked_pipelines.erase(baked_pipelines_iterator);
            }
        }
    }
    unlock();

    return result;
}

/** Converts the internal vertex attribute descriptors to one or more VkVertexInputAttributeDescription & VkVertexInputBindingDescription
 *  structures.
 *
 *  The function goes an extra mile to re-use the same vertex binding for attributes whose binding properties match.
 */
void Anvil::GraphicsPipelineManager::GraphicsPipelineData::bake_vk_attributes_and_bindings()
{
    uint32_t n_attributes = 0;

    anvil_assert(attribute_location_to_binding_index_map.size() == 0);
    anvil_assert(input_bindings.size                         () == 0);
    anvil_assert(vk_input_attributes.size                    () == 0);
    anvil_assert(vk_input_bindings.size                      () == 0);

    pipeline_create_info_ptr->get_graphics_pipeline_properties(nullptr,       /* out_opt_n_scissors_ptr          */
                                                               nullptr,       /* out_opt_n_viewports_ptr         */
                                                               &n_attributes, /* out_opt_n_vertex_attributes_ptr */
                                                               nullptr,       /* out_opt_renderpass_ptr          */
                                                               nullptr);      /* out_opt_subpass_id_ptr          */

    for (uint32_t n_attribute = 0;
                  n_attribute < n_attributes;
                ++n_attribute)
    {
        /* Identify the binding index we should use for the attribute.
         *
         * If an explicit binding has been specified by the application, this step can be skipped */
        uint32_t                          current_attribute_divisor                       = 1;
        uint32_t                          current_attribute_explicit_vertex_binding_index = UINT32_MAX;
        uint32_t                          current_attribute_location                      = UINT32_MAX;
        Anvil::Format                     current_attribute_format                        = Anvil::Format::UNKNOWN;
        uint32_t                          current_attribute_offset                        = UINT32_MAX;
        Anvil::VertexInputRate            current_attribute_rate                          = Anvil::VertexInputRate::UNKNOWN;
        uint32_t                          current_attribute_stride                        = UINT32_MAX;
        VkVertexInputAttributeDescription current_attribute_vk;
        uint32_t                          n_attribute_binding                             = UINT32_MAX;
        bool                              has_found                                       = false;

        if (!pipeline_create_info_ptr->get_vertex_attribute_properties(n_attribute,
                                                                      &current_attribute_location,
                                                                      &current_attribute_format,
                                                                      &current_attribute_offset,
                                                                      &current_attribute_explicit_vertex_binding_index,
                                                                      &current_attribute_stride,
                                                                      &current_attribute_rate,
                                                                      &current_attribute_divisor) )
        {
            anvil_assert_fail();

            continue;
        }

        if (current_attribute_explicit_vertex_binding_index == UINT32_MAX)
        {
            for (auto input_binding_iterator  = input_bindings.begin();
                      input_binding_iterator != input_bindings.end();
                    ++input_binding_iterator)
            {
                if (input_binding_iterator->divisor    == current_attribute_divisor &&
                    input_binding_iterator->input_rate == current_attribute_rate    &&
                    input_binding_iterator->stride     == current_attribute_stride)
                {
                    has_found           = true;
                    n_attribute_binding = static_cast<uint32_t>(input_binding_iterator - input_bindings.begin());

                    break;
                }
            }
        }
        else
        {
            has_found           = false;
            n_attribute_binding = current_attribute_explicit_vertex_binding_index;
        }

        if (!has_found)
        {
            /* Got to create a new binding descriptor .. */
            VertexInputBinding              new_binding_anvil;
            VkVertexInputBindingDescription new_binding_vk;

            new_binding_vk.binding   = (current_attribute_explicit_vertex_binding_index == UINT32_MAX) ? static_cast<uint32_t>(vk_input_bindings.size() )
                                                                                                       : current_attribute_explicit_vertex_binding_index;
            new_binding_vk.inputRate = static_cast<VkVertexInputRate>(current_attribute_rate);
            new_binding_vk.stride    = current_attribute_stride;

            new_binding_anvil = VertexInputBinding(new_binding_vk,
                                                   current_attribute_divisor);

            input_bindings.push_back   (new_binding_anvil);
            vk_input_bindings.push_back(new_binding_vk);

            n_attribute_binding = new_binding_vk.binding;
        }

        /* Good to convert the internal attribute descriptor to the Vulkan's input attribute descriptor */
        current_attribute_vk.binding  = n_attribute_binding;
        current_attribute_vk.format   = static_cast<VkFormat>(current_attribute_format);
        current_attribute_vk.location = current_attribute_location;
        current_attribute_vk.offset   = current_attribute_offset;

        /* Associate attribute locations with assigned bindings */
        anvil_assert(attribute_location_to_binding_index_map.find(current_attribute_location) == attribute_location_to_binding_index_map.end() );
        attribute_location_to_binding_index_map[current_attribute_location] = current_attribute_vk.binding;

        /* Cache the descriptor */
        vk_input_attributes.push_back(current_attribute_vk);
    }
}

/* Please see header for specification */
Anvil::GraphicsPipelineManagerUniquePtr Anvil::GraphicsPipelineManager::create(const Anvil::BaseDevice* in_device_ptr,
                                                                               bool                     in_mt_safe,
                                                                               bool                     in_use_pipeline_cache,
                                                                               Anvil::PipelineCache*    in_pipeline_cache_to_reuse_ptr)
{
    GraphicsPipelineManagerUniquePtr result_ptr(nullptr,
                                                std::default_delete<Anvil::GraphicsPipelineManager>() );

    result_ptr.reset(
        new Anvil::GraphicsPipelineManager(in_device_ptr,
                                           in_mt_safe,
                                           in_use_pipeline_cache,
                                           in_pipeline_cache_to_reuse_ptr)
    );

    return result_ptr;
}
