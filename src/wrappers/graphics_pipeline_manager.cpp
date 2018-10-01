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
#include "misc/graphics_pipeline_create_info.h"
#include "misc/object_tracker.h"
#include "misc/render_pass_create_info.h"
#include "misc/struct_chainer.h"
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

/** When baking a graphics pipeline descriptor, we currently use STL vectors to maintain storage
 *  for various Vulkan descriptors. These need to be stored on heap because:
 *
 *  1. The number of required descriptors depends on the pipeline configuration.
 *  2. Some parent descriptors use pointers to point at other child descriptors.
 *
 *  In order for vectors not to reallocate memory, we need to reserve memory up-front before
 *  we start pushing stuff. The number defined by N_CACHE_ITEMS tells how many items each
 *  vector will hold at max.
 *
 *  This #define could be removed by handling memory allocations manually, but for simplicity
 *  reasons we'll go with STL vectors for now.
 *  */
#define N_CACHE_ITEMS 256


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
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_GRAPHICS_PIPELINE_MANAGER,
                                                  this);
}

/* Please see header for specification */
Anvil::GraphicsPipelineManager::~GraphicsPipelineManager()
{
    m_baked_pipelines.clear      ();
    m_outstanding_pipelines.clear();

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_GRAPHICS_PIPELINE_MANAGER,
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
    auto                                   color_blend_attachment_states_vk_cache             = std::vector<VkPipelineColorBlendAttachmentState>      ();
    auto                                   color_blend_state_create_info_items_vk_cache       = std::vector<VkPipelineColorBlendStateCreateInfo>      ();
    auto                                   depth_stencil_state_create_info_items_vk_cache     = std::vector<VkPipelineDepthStencilStateCreateInfo>    ();
    auto                                   dynamic_state_create_info_items_vk_cache           = std::vector<VkPipelineDynamicStateCreateInfo>         ();
    auto                                   graphics_pipeline_create_info_chains               = Anvil::StructChainVector<VkGraphicsPipelineCreateInfo>();
    auto                                   input_assembly_state_create_info_items_vk_cache    = std::vector<VkPipelineInputAssemblyStateCreateInfo>   ();
    auto                                   multisample_state_create_info_items_vk_cache       = std::vector<VkPipelineMultisampleStateCreateInfo>     ();
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr                                          = get_mutex();
    uint32_t                               n_consumed_graphics_pipelines                      = 0;
    auto                                   raster_state_create_info_chains_vk_cache           = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineRasterizationStateCreateInfo> > >();
    bool                                   result                                             = false;
    std::vector<VkPipeline>                result_graphics_pipelines;
    VkResult                               result_vk;
    auto                                   scissor_boxes_vk_cache                             = std::vector<VkRect2D>                                                                    ();
    auto                                   shader_stage_create_info_items_vk_cache            = std::vector<VkPipelineShaderStageCreateInfo>                                             ();
    auto                                   specialization_info_vk_cache                       = std::vector<VkSpecializationInfo>                                                        ();
    auto                                   specialization_map_entry_vk_cache                  = std::vector<VkSpecializationMapEntry>                                                    ();
    auto                                   tessellation_state_create_info_chain_cache         = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineTessellationStateCreateInfo> > >();
    auto                                   vertex_input_binding_divisor_description_vk_cache  = std::vector<VkVertexInputBindingDivisorDescriptionEXT>                                   ();
    auto                                   vertex_input_state_create_info_chain_cache         = std::vector<std::unique_ptr<Anvil::StructChain<VkPipelineVertexInputStateCreateInfo> > > ();
    auto                                   viewport_state_create_info_items_vk_cache          = std::vector<VkPipelineViewportStateCreateInfo>                                           ();
    std::vector<VkViewport>                viewports_vk_cache;


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


    color_blend_attachment_states_vk_cache.reserve           (N_CACHE_ITEMS);
    color_blend_state_create_info_items_vk_cache.reserve     (N_CACHE_ITEMS);
    depth_stencil_state_create_info_items_vk_cache.reserve   (N_CACHE_ITEMS);
    dynamic_state_create_info_items_vk_cache.reserve         (N_CACHE_ITEMS);
    input_assembly_state_create_info_items_vk_cache.reserve  (N_CACHE_ITEMS);
    multisample_state_create_info_items_vk_cache.reserve     (N_CACHE_ITEMS);
    raster_state_create_info_chains_vk_cache.reserve         (N_CACHE_ITEMS);
    scissor_boxes_vk_cache.reserve                           (N_CACHE_ITEMS);
    shader_stage_create_info_items_vk_cache.reserve          (N_CACHE_ITEMS);
    specialization_info_vk_cache.reserve                     (N_CACHE_ITEMS);
    specialization_map_entry_vk_cache.reserve                (N_CACHE_ITEMS);
    tessellation_state_create_info_chain_cache.reserve       (N_CACHE_ITEMS);
    vertex_input_binding_divisor_description_vk_cache.reserve(N_CACHE_ITEMS);
    viewport_state_create_info_items_vk_cache.reserve        (N_CACHE_ITEMS);
    viewports_vk_cache.reserve                               (N_CACHE_ITEMS);

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
        const PipelineID                                   current_pipeline_id                   = bake_item_iterator->pipeline_id;
        auto                                               current_pipeline_create_info_ptr      = dynamic_cast<GraphicsPipelineCreateInfo*>(bake_item_iterator->pipeline_ptr->pipeline_create_info_ptr.get() );
        Pipeline*                                          current_pipeline_ptr                  = bake_item_iterator->pipeline_ptr;
        const Anvil::RenderPass*                           current_pipeline_renderpass_ptr       = nullptr;
        Anvil::SubPassID                                   current_pipeline_subpass_id;
        bool                                               depth_stencil_state_used              = false;
        bool                                               dynamic_state_used                    = false;
        Anvil::StructChainer<VkGraphicsPipelineCreateInfo> graphics_pipeline_create_info_chainer;
        VkPipelineInputAssemblyStateCreateInfo             input_assembly_state_create_info;
        bool                                               is_dynamic_scissor_state_enabled      = false;
        bool                                               is_dynamic_viewport_state_enabled     = false;
        bool                                               multisample_state_used                = false;
        uint32_t                                           subpass_n_color_attachments           = 0;
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

        /* Extract subpass information */
        current_pipeline_renderpass_ptr->get_render_pass_create_info()->get_subpass_n_attachments(current_pipeline_subpass_id,
                                                                                                  Anvil::AttachmentType::COLOR,
                                                                                                 &subpass_n_color_attachments);

        /* Form the color blend state create info descriptor, if needed */
        {
            if (!current_pipeline_create_info_ptr->is_rasterizer_discard_enabled()     &&
                 subpass_n_color_attachments                                       > 0)
            {
                const float*                        blend_constant_ptr            = nullptr;
                VkPipelineColorBlendStateCreateInfo color_blend_state_create_info;
                Anvil::LogicOp                      logic_op                      = Anvil::LogicOp::UNKNOWN;
                bool                                logic_op_enabled              = false;
                uint32_t                            max_location_index            = UINT32_MAX;
                const uint32_t                      start_offset                  = static_cast<uint32_t>(color_blend_attachment_states_vk_cache.size() );

                max_location_index = current_pipeline_renderpass_ptr->get_render_pass_create_info()->get_max_color_location_used_by_subpass(current_pipeline_subpass_id);

                current_pipeline_create_info_ptr->get_blending_properties(&blend_constant_ptr,
                                                                          nullptr); /* out_opt_n_blend_attachments_ptr */

                current_pipeline_create_info_ptr->get_logic_op_state(&logic_op_enabled,
                                                                     &logic_op);

                color_blend_state_create_info.attachmentCount       = max_location_index + 1;
                color_blend_state_create_info.flags                 = 0;
                color_blend_state_create_info.logicOp               = static_cast<VkLogicOp>(logic_op);
                color_blend_state_create_info.logicOpEnable         = (logic_op_enabled) ? VK_TRUE : VK_FALSE;
                color_blend_state_create_info.pAttachments          = nullptr;
                color_blend_state_create_info.pNext                 = nullptr;
                color_blend_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

                memcpy(color_blend_state_create_info.blendConstants,
                       blend_constant_ptr,
                       sizeof(color_blend_state_create_info.blendConstants) );

                anvil_assert(subpass_n_color_attachments <= current_pipeline_renderpass_ptr->get_render_pass_create_info()->get_n_attachments() );

                for (uint32_t n_subpass_color_attachment = 0;
                              n_subpass_color_attachment <= max_location_index;
                            ++n_subpass_color_attachment)
                {
                    color_blend_attachment_states_vk_cache.push_back(VkPipelineColorBlendAttachmentState() );

                    VkPipelineColorBlendAttachmentState* blend_attachment_state_ptr         = &color_blend_attachment_states_vk_cache.back();
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

                    if (!current_pipeline_renderpass_ptr->get_render_pass_create_info()->get_subpass_attachment_properties(current_pipeline_subpass_id,
                                                                                                                           Anvil::AttachmentType::COLOR,
                                                                                                                           n_subpass_color_attachment,
                                                                                                                          &rp_attachment_id,
                                                                                                                          &dummy) || /* out_layout_ptr */
                        !current_pipeline_create_info_ptr->get_color_blend_attachment_properties                          (rp_attachment_id,
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
                        blend_attachment_state_ptr->blendEnable         = VK_FALSE;
                        blend_attachment_state_ptr->alphaBlendOp        = VK_BLEND_OP_ADD;
                        blend_attachment_state_ptr->colorBlendOp        = VK_BLEND_OP_ADD;
                        blend_attachment_state_ptr->colorWriteMask      = VK_COLOR_COMPONENT_A_BIT |
                                                                          VK_COLOR_COMPONENT_B_BIT |
                                                                          VK_COLOR_COMPONENT_G_BIT |
                                                                          VK_COLOR_COMPONENT_R_BIT;
                        blend_attachment_state_ptr->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        blend_attachment_state_ptr->dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        blend_attachment_state_ptr->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        blend_attachment_state_ptr->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                    }
                    else
                    {
                        blend_attachment_state_ptr->alphaBlendOp        = static_cast<VkBlendOp>(alpha_blend_op);
                        blend_attachment_state_ptr->blendEnable         = (is_blending_enabled_for_attachment) ? VK_TRUE : VK_FALSE;
                        blend_attachment_state_ptr->colorBlendOp        = static_cast<VkBlendOp>    (color_blend_op);
                        blend_attachment_state_ptr->colorWriteMask      = color_component_flags.get_vk();
                        blend_attachment_state_ptr->dstAlphaBlendFactor = static_cast<VkBlendFactor>(dst_alpha_blend_factor);
                        blend_attachment_state_ptr->dstColorBlendFactor = static_cast<VkBlendFactor>(dst_color_blend_factor);
                        blend_attachment_state_ptr->srcAlphaBlendFactor = static_cast<VkBlendFactor>(src_alpha_blend_factor);
                        blend_attachment_state_ptr->srcColorBlendFactor = static_cast<VkBlendFactor>(src_color_blend_factor);
                    }
                }

                color_blend_state_create_info.pAttachments = (subpass_n_color_attachments > 0) ? &color_blend_attachment_states_vk_cache[start_offset]
                                                                                               : nullptr;
                color_blend_state_used                     = true;

                color_blend_state_create_info_items_vk_cache.push_back(color_blend_state_create_info);
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
            VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info;
            auto                                  depth_test_compare_op            = Anvil::CompareOp::UNKNOWN;
            bool                                  is_depth_bounds_test_enabled     = false;
            bool                                  is_depth_test_enabled            = false;
            bool                                  is_stencil_test_enabled          = false;
            float                                 max_depth_bounds                 = std::numeric_limits<float>::max();
            float                                 min_depth_bounds                 = std::numeric_limits<float>::max();
            uint32_t                              n_depth_stencil_attachments      = 0;

            current_pipeline_create_info_ptr->get_depth_bounds_state(&is_depth_bounds_test_enabled,
                                                                     &min_depth_bounds,
                                                                     &max_depth_bounds);

            current_pipeline_create_info_ptr->get_depth_test_state(&is_depth_test_enabled,
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

                current_pipeline_create_info_ptr->get_stencil_test_properties(&is_stencil_test_enabled,
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
                                                                              &depth_stencil_state_create_info.back.reference);\
                depth_stencil_state_create_info.back.compareOp    = static_cast<VkCompareOp>(back_compare_op);
                depth_stencil_state_create_info.back.depthFailOp  = static_cast<VkStencilOp>(back_depth_fail_op);
                depth_stencil_state_create_info.back.failOp       = static_cast<VkStencilOp>(back_fail_op);
                depth_stencil_state_create_info.back.passOp       = static_cast<VkStencilOp>(back_pass_op);
                depth_stencil_state_create_info.front.compareOp   = static_cast<VkCompareOp>(front_compare_op);
                depth_stencil_state_create_info.front.depthFailOp = static_cast<VkStencilOp>(front_depth_fail_op);
                depth_stencil_state_create_info.front.failOp      = static_cast<VkStencilOp>(front_fail_op);
                depth_stencil_state_create_info.front.passOp      = static_cast<VkStencilOp>(front_pass_op);
            }

            current_pipeline_renderpass_ptr->get_render_pass_create_info()->get_subpass_n_attachments(current_pipeline_create_info_ptr->get_subpass_id(),
                                                                                                      Anvil::AttachmentType::DEPTH_STENCIL,
                                                                                                     &n_depth_stencil_attachments);

            if (n_depth_stencil_attachments)
            {
                depth_stencil_state_create_info.depthBoundsTestEnable = is_depth_bounds_test_enabled ? VK_TRUE : VK_FALSE;
                depth_stencil_state_create_info.depthCompareOp        = static_cast<VkCompareOp>(depth_test_compare_op);
                depth_stencil_state_create_info.depthTestEnable       = is_depth_test_enabled                                        ? VK_TRUE : VK_FALSE;
                depth_stencil_state_create_info.depthWriteEnable      = current_pipeline_create_info_ptr->are_depth_writes_enabled() ? VK_TRUE : VK_FALSE;
                depth_stencil_state_create_info.flags                 = 0;
                depth_stencil_state_create_info.maxDepthBounds        = max_depth_bounds;
                depth_stencil_state_create_info.minDepthBounds        = min_depth_bounds;
                depth_stencil_state_create_info.pNext                 = nullptr;
                depth_stencil_state_create_info.stencilTestEnable     = is_stencil_test_enabled ? VK_TRUE : VK_FALSE;
                depth_stencil_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

                depth_stencil_state_used = true;

                depth_stencil_state_create_info_items_vk_cache.push_back(depth_stencil_state_create_info);
            }
            else
            {
                /* No depth/stencil attachment available. Make sure none of the dependent modes are enabled. */
                anvil_assert(!is_depth_bounds_test_enabled);
                anvil_assert(!is_depth_test_enabled);
                anvil_assert(!current_pipeline_create_info_ptr->are_depth_writes_enabled());
                anvil_assert(!is_stencil_test_enabled);

                depth_stencil_state_used = false;
            }
        }

        /* Form the dynamic state create info descriptor, if needed */
        {
            const Anvil::DynamicState* enabled_dynamic_states_ptr = nullptr;
            uint32_t                   n_enabled_dynamic_states   = 0;

            current_pipeline_create_info_ptr->get_enabled_dynamic_states(&enabled_dynamic_states_ptr,
                                                                         &n_enabled_dynamic_states);

            if (n_enabled_dynamic_states != 0)
            {
                VkPipelineDynamicStateCreateInfo dynamic_state_create_info;

                dynamic_state_create_info.dynamicStateCount = n_enabled_dynamic_states;
                dynamic_state_create_info.flags             = 0;
                dynamic_state_create_info.pDynamicStates    = (dynamic_state_create_info.dynamicStateCount > 0) ? reinterpret_cast<const VkDynamicState*>(enabled_dynamic_states_ptr)
                                                                                                                : VK_NULL_HANDLE;
                dynamic_state_create_info.pNext             = nullptr;
                dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

                dynamic_state_used = true;

                dynamic_state_create_info_items_vk_cache.push_back(dynamic_state_create_info);

                for (uint32_t n_dynamic_state = 0;
                              n_dynamic_state < n_enabled_dynamic_states;
                            ++n_dynamic_state)
                {
                    if (enabled_dynamic_states_ptr[n_dynamic_state] == Anvil::DynamicState::SCISSOR)
                    {
                        is_dynamic_scissor_state_enabled = true;
                    }

                    if (enabled_dynamic_states_ptr[n_dynamic_state] == Anvil::DynamicState::VIEWPORT)
                    {
                        is_dynamic_viewport_state_enabled = true;
                    }
                }
            }
            else
            {
                dynamic_state_used = false;
            }
        }

        /* Form the input assembly create info descriptor */
        {
            input_assembly_state_create_info.flags                  = 0;
            input_assembly_state_create_info.pNext                  = nullptr;
            input_assembly_state_create_info.primitiveRestartEnable = current_pipeline_create_info_ptr->is_primitive_restart_enabled() ? VK_TRUE : VK_FALSE;
            input_assembly_state_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_state_create_info.topology               = static_cast<VkPrimitiveTopology>(current_pipeline_create_info_ptr->get_primitive_topology() );

            input_assembly_state_create_info_items_vk_cache.push_back(input_assembly_state_create_info);
        }

        /* Form the multisample state create info descriptor, if needed */
        {
            bool  is_sample_shading_enabled = false;
            float min_sample_shading        = std::numeric_limits<float>::max();

            current_pipeline_create_info_ptr->get_sample_shading_state(&is_sample_shading_enabled,
                                                                       &min_sample_shading);

            if (!current_pipeline_create_info_ptr->is_rasterizer_discard_enabled() )
            {
                const bool                           is_sample_mask_enabled        = current_pipeline_create_info_ptr->is_sample_mask_enabled();
                VkPipelineMultisampleStateCreateInfo multisample_state_create_info;
                Anvil::SampleCountFlagBits           sample_count                  = static_cast<Anvil::SampleCountFlagBits>(0);
                VkSampleMask                         sample_mask;

                current_pipeline_create_info_ptr->get_multisampling_properties(&sample_count,
                                                                               &sample_mask);

                /* If sample mask is not enabled, Vulkan spec will assume all samples need to pass. This is what the default sample mask value mimics.
                *
                 * Hence, if the application specified a non-~0u sample mask and has NOT enabled the sample mask using toggle_sample_mask(), it's (in
                 * all likelihood) a trivial app-side issue.
                 */
                anvil_assert((!is_sample_mask_enabled && sample_mask == ~0u) ||
                               is_sample_mask_enabled);

                multisample_state_create_info.alphaToCoverageEnable = current_pipeline_create_info_ptr->is_alpha_to_coverage_enabled() ? VK_TRUE : VK_FALSE;
                multisample_state_create_info.alphaToOneEnable      = current_pipeline_create_info_ptr->is_alpha_to_one_enabled()      ? VK_TRUE : VK_FALSE;
                multisample_state_create_info.flags                 = 0;
                multisample_state_create_info.minSampleShading      = min_sample_shading;
                multisample_state_create_info.pNext                 = nullptr;
                multisample_state_create_info.pSampleMask           = (is_sample_mask_enabled) ? &sample_mask : nullptr;
                multisample_state_create_info.rasterizationSamples  = static_cast<VkSampleCountFlagBits>(sample_count);
                multisample_state_create_info.sampleShadingEnable   = is_sample_shading_enabled ? VK_TRUE : VK_FALSE;
                multisample_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

                multisample_state_used = true;

                multisample_state_create_info_items_vk_cache.push_back(multisample_state_create_info);
            }
            else
            {
                /* Make sure any dependent modes are disabled */
                anvil_assert(!current_pipeline_create_info_ptr->is_alpha_to_coverage_enabled() );
                anvil_assert(!current_pipeline_create_info_ptr->is_alpha_to_one_enabled     () );
                anvil_assert(!is_sample_shading_enabled);

                multisample_state_used = false;
            }
        }

        /* Form the raster state create info chain */
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

            current_pipeline_create_info_ptr->get_depth_bias_state        (&is_depth_bias_enabled,
                                                                           &depth_bias_constant_factor,
                                                                           &depth_bias_clamp,
                                                                           &depth_bias_slope_factor);
            current_pipeline_create_info_ptr->get_rasterization_properties(&polygon_mode,
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
                raster_state_create_info.depthClampEnable        = current_pipeline_create_info_ptr->is_depth_clamp_enabled() ? VK_TRUE : VK_FALSE;
                raster_state_create_info.flags                   = 0;
                raster_state_create_info.frontFace               = static_cast<VkFrontFace>(front_face);
                raster_state_create_info.lineWidth               = line_width;
                raster_state_create_info.pNext                   = nullptr;
                raster_state_create_info.polygonMode             = static_cast<VkPolygonMode>(polygon_mode);
                raster_state_create_info.rasterizerDiscardEnable = current_pipeline_create_info_ptr->is_rasterizer_discard_enabled() ? VK_TRUE : VK_FALSE;
                raster_state_create_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

                raster_state_create_info_chainer.append_struct(raster_state_create_info);
            }

            if (m_device_ptr->is_extension_enabled(VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME) )
            {
                /* Chain a predefined struct which will toggle the relaxed rasterization, as long as the device supports the
                 * VK_AMD_rasterization_order extension.
                 */
                if (current_pipeline_create_info_ptr->get_rasterization_order() != Anvil::RasterizationOrderAMD::STRICT)
                {
                    anvil_assert(current_pipeline_create_info_ptr->get_rasterization_order() == static_cast<Anvil::RasterizationOrderAMD>(relaxed_rasterization_order_item.rasterizationOrder) );

                    raster_state_create_info_chainer.append_struct(relaxed_rasterization_order_item);
                }
                else
                {
                    raster_state_create_info_chainer.append_struct(strict_rasterization_order_item);
                }
            }
            else
            if (current_pipeline_create_info_ptr->get_rasterization_order() != Anvil::RasterizationOrderAMD::STRICT)
            {
                fprintf(stderr,
                        "[!] Cannot enable out-of-order rasterization - VK_AMD_rasterization_order extension not enabled at device creation time\n");
            }

            raster_state_create_info_chains_vk_cache.push_back(
                raster_state_create_info_chainer.create_chain()
            );
        }

        /* Form stage descriptors */
        uint32_t shader_stage_start_offset = static_cast<uint32_t>(shader_stage_create_info_items_vk_cache.size() );
        {
            static const Anvil::ShaderStage graphics_shader_stages[] =
            {
                Anvil::ShaderStage::FRAGMENT,
                Anvil::ShaderStage::GEOMETRY,
                Anvil::ShaderStage::TESSELLATION_CONTROL,
                Anvil::ShaderStage::TESSELLATION_EVALUATION,
                Anvil::ShaderStage::VERTEX
            };

            for (const auto& current_graphics_shader_stage : graphics_shader_stages)
            {
                const Anvil::ShaderModuleStageEntryPoint* shader_stage_entry_point_ptr = nullptr;

                current_pipeline_create_info_ptr->get_shader_stage_properties(current_graphics_shader_stage,
                                                                             &shader_stage_entry_point_ptr);

                if (shader_stage_entry_point_ptr != nullptr)
                {
                    VkPipelineShaderStageCreateInfo current_shader_stage_create_info;
                    const unsigned char*            current_shader_stage_specialization_constants_data_buffer_ptr = nullptr;
                    const SpecializationConstants*  current_shader_stage_specialization_constants_ptr             = nullptr;
                    Anvil::ShaderModule*            shader_module_ptr                                             = nullptr;

                    specialization_info_vk_cache.push_back(VkSpecializationInfo() );

                    current_pipeline_create_info_ptr->get_specialization_constants(current_graphics_shader_stage,
                                                                                  &current_shader_stage_specialization_constants_ptr,
                                                                                  &current_shader_stage_specialization_constants_data_buffer_ptr);

                    if (current_shader_stage_specialization_constants_ptr->size() > 0)
                    {
                        bake_specialization_info_vk(*current_shader_stage_specialization_constants_ptr,
                                                     current_shader_stage_specialization_constants_data_buffer_ptr,
                                                    &specialization_map_entry_vk_cache,
                                                    &specialization_info_vk_cache.back() );
                    }

                    shader_module_ptr = shader_stage_entry_point_ptr->shader_module_ptr;

                    current_shader_stage_create_info.module = shader_module_ptr->get_module();
                    current_shader_stage_create_info.pName  = (shader_stage_entry_point_ptr->stage == Anvil::ShaderStage::FRAGMENT)                ? shader_module_ptr->get_fs_entrypoint_name().c_str()
                                                            : (shader_stage_entry_point_ptr->stage == Anvil::ShaderStage::GEOMETRY)                ? shader_module_ptr->get_gs_entrypoint_name().c_str()
                                                            : (shader_stage_entry_point_ptr->stage == Anvil::ShaderStage::TESSELLATION_CONTROL)    ? shader_module_ptr->get_tc_entrypoint_name().c_str()
                                                            : (shader_stage_entry_point_ptr->stage == Anvil::ShaderStage::TESSELLATION_EVALUATION) ? shader_module_ptr->get_te_entrypoint_name().c_str()
                                                            : (shader_stage_entry_point_ptr->stage == Anvil::ShaderStage::VERTEX)                  ? shader_module_ptr->get_vs_entrypoint_name().c_str()
                                                            : nullptr;

                    current_shader_stage_create_info.flags               = 0;
                    current_shader_stage_create_info.pNext               = nullptr;
                    current_shader_stage_create_info.pSpecializationInfo = (current_shader_stage_specialization_constants_ptr->size() > 0) ? &specialization_info_vk_cache.back()
                                                                                                                                           : VK_NULL_HANDLE;
                    current_shader_stage_create_info.stage               = static_cast<VkShaderStageFlagBits>(Anvil::Utils::get_shader_stage_flag_bits_from_shader_stage(shader_stage_entry_point_ptr->stage) );
                    current_shader_stage_create_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

                    shader_stage_create_info_items_vk_cache.push_back(current_shader_stage_create_info);
                }
            }
        }

        /* Form the tessellation state create info descriptor if needed */
        {
            const Anvil::ShaderModuleStageEntryPoint* tc_shader_stage_entry_point_ptr = nullptr;
            const Anvil::ShaderModuleStageEntryPoint* te_shader_stage_entry_point_ptr = nullptr;

            current_pipeline_create_info_ptr->get_shader_stage_properties(Anvil::ShaderStage::TESSELLATION_CONTROL,
                                                                         &tc_shader_stage_entry_point_ptr);
            current_pipeline_create_info_ptr->get_shader_stage_properties(Anvil::ShaderStage::TESSELLATION_EVALUATION,
                                                                         &te_shader_stage_entry_point_ptr);

            if (tc_shader_stage_entry_point_ptr != nullptr &&
                te_shader_stage_entry_point_ptr != nullptr)
            {
                Anvil::StructChainer<VkPipelineTessellationStateCreateInfo> tessellation_state_create_info_chainer;

                {
                    VkPipelineTessellationStateCreateInfo tessellation_state_create_info;

                    tessellation_state_create_info.flags              = 0;
                    tessellation_state_create_info.patchControlPoints = current_pipeline_create_info_ptr->get_n_patch_control_points();
                    tessellation_state_create_info.pNext              = nullptr;
                    tessellation_state_create_info.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

                    tessellation_state_create_info_chainer.append_struct(tessellation_state_create_info);
                }

                if (m_device_ptr->is_extension_enabled(VK_KHR_MAINTENANCE2_EXTENSION_NAME) )
                {
                    VkPipelineTessellationDomainOriginStateCreateInfoKHR domain_origin_state_create_info;

                    domain_origin_state_create_info.domainOrigin = static_cast<VkTessellationDomainOriginKHR>(current_pipeline_create_info_ptr->get_tessellation_domain_origin() );
                    domain_origin_state_create_info.pNext        = nullptr;
                    domain_origin_state_create_info.sType        = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO_KHR;

                    tessellation_state_create_info_chainer.append_struct(domain_origin_state_create_info);
                }
                else
                {
                    /* App wants to adjust the default tessellation domain origin value, even though KHR_maintenance2 is unsupported! */
                    anvil_assert(current_pipeline_create_info_ptr->get_tessellation_domain_origin() == Anvil::TessellationDomainOrigin::UPPER_LEFT);
                }

                tessellation_state_used = true;

                tessellation_state_create_info_chain_cache.push_back(tessellation_state_create_info_chainer.create_chain() );
            }
            else
            {
                tessellation_state_used = false;
            }
        }

        /* Form the vertex input state create info descriptor */
        {
            std::unique_ptr<GraphicsPipelineData>                      current_pipeline_gfx_data_ptr;
            Anvil::StructChainer<VkPipelineVertexInputStateCreateInfo> vertex_input_state_create_info_chainer;

            anvil_assert(m_pipeline_id_to_gfx_pipeline_data.find(current_pipeline_id) == m_pipeline_id_to_gfx_pipeline_data.end() );

            current_pipeline_gfx_data_ptr.reset(
                new GraphicsPipelineData(current_pipeline_create_info_ptr)
            );

            if (current_pipeline_gfx_data_ptr == nullptr)
            {
                anvil_assert(current_pipeline_gfx_data_ptr != nullptr);

                continue;
            }

            {
                VkPipelineVertexInputStateCreateInfo create_info;

                create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(current_pipeline_gfx_data_ptr->vk_input_attributes.size());
                create_info.vertexBindingDescriptionCount   = static_cast<uint32_t>(current_pipeline_gfx_data_ptr->vk_input_bindings.size  ());

                create_info.flags                        = 0;
                create_info.pNext                        = nullptr;
                create_info.pVertexAttributeDescriptions = (create_info.vertexAttributeDescriptionCount > 0) ? &current_pipeline_gfx_data_ptr->vk_input_attributes.at(0)
                                                                                                             : nullptr;
                create_info.pVertexBindingDescriptions   = (create_info.vertexBindingDescriptionCount   > 0) ? &current_pipeline_gfx_data_ptr->vk_input_bindings.at(0)
                                                                                                             : nullptr;
                create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

                vertex_input_state_create_info_chainer.append_struct(create_info);
            }

            {
                uint32_t n_divisor_descriptors_pre_cached = static_cast<uint32_t>(vertex_input_binding_divisor_description_vk_cache.size() );

                for (const auto& current_binding_info : current_pipeline_gfx_data_ptr->input_bindings)
                {
                    /* Make sure to chain VkVertexInputBindingDivisorDescriptionEXT for each binding which uses a non-default divisor value */
                    if (current_binding_info.divisor != 1)
                    {
                        VkVertexInputBindingDivisorDescriptionEXT divisor_info;

                        divisor_info.binding = current_binding_info.binding;
                        divisor_info.divisor = current_binding_info.divisor;

                        vertex_input_binding_divisor_description_vk_cache.push_back(divisor_info);
                    }
                }

                if (n_divisor_descriptors_pre_cached < static_cast<uint32_t>(vertex_input_binding_divisor_description_vk_cache.size() ))
                {
                    VkPipelineVertexInputDivisorStateCreateInfoEXT divisor_state_create_info;

                    divisor_state_create_info.pNext                     = nullptr;
                    divisor_state_create_info.pVertexBindingDivisors    = &vertex_input_binding_divisor_description_vk_cache.at(n_divisor_descriptors_pre_cached);
                    divisor_state_create_info.sType                     = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;
                    divisor_state_create_info.vertexBindingDivisorCount = static_cast<uint32_t>(vertex_input_binding_divisor_description_vk_cache.size() ) - n_divisor_descriptors_pre_cached;

                    vertex_input_state_create_info_chainer.append_struct(divisor_state_create_info);
                }
            }

            vertex_input_state_create_info_chain_cache.push_back(
                vertex_input_state_create_info_chainer.create_chain()
            );

            m_pipeline_id_to_gfx_pipeline_data[current_pipeline_id] = std::move(current_pipeline_gfx_data_ptr);
        }

        /* Form the viewport state create info descriptor, if needed */
        if (!current_pipeline_create_info_ptr->is_rasterizer_discard_enabled() )
        {
            uint32_t                          n_scissor_boxes            = (is_dynamic_scissor_state_enabled)  ? current_pipeline_create_info_ptr->get_n_dynamic_scissor_boxes()
                                                                                                               : current_pipeline_create_info_ptr->get_n_scissor_boxes        ();
            uint32_t                          n_viewports                = (is_dynamic_viewport_state_enabled) ? current_pipeline_create_info_ptr->get_n_dynamic_viewports    ()
                                                                                                               : current_pipeline_create_info_ptr->get_n_viewports            ();
            const uint32_t                    scissor_boxes_start_offset = static_cast<uint32_t>(scissor_boxes_vk_cache.size() );
            const uint32_t                    viewports_start_offset     = static_cast<uint32_t>(viewports_vk_cache.size() );
            VkPipelineViewportStateCreateInfo viewport_state_create_info;

            anvil_assert(n_scissor_boxes == n_viewports);

            if (n_scissor_boxes == 0)
            {
                /* No scissor boxes / viewport defined. Use default settings.. */
                auto     swapchain_ptr  = current_pipeline_create_info_ptr->get_renderpass()->get_swapchain();
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

                current_pipeline_create_info_ptr->set_scissor_box_properties(0, /* in_n_scissor_box */
                                                                             0, /* in_x             */
                                                                             0, /* in_y             */
                                                                             window_size[0],
                                                                             window_size[1]);
                current_pipeline_create_info_ptr->set_viewport_properties   (0,    /* in_n_viewport */
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
            if (!is_dynamic_scissor_state_enabled)
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

                    current_pipeline_create_info_ptr->get_scissor_box_properties(n_scissor_box,
                                                                                &current_scissor_box_x,
                                                                                &current_scissor_box_y,
                                                                                &current_scissor_box_width,
                                                                                &current_scissor_box_height);
                    current_scissor_box_vk.extent.height = current_scissor_box_height;
                    current_scissor_box_vk.extent.width  = current_scissor_box_width;
                    current_scissor_box_vk.offset.x      = current_scissor_box_x;
                    current_scissor_box_vk.offset.y      = current_scissor_box_y;

                    scissor_boxes_vk_cache.push_back(current_scissor_box_vk);
                }
            }

            if (!is_dynamic_viewport_state_enabled)
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

                    current_pipeline_create_info_ptr->get_viewport_properties(n_viewport,
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

                    viewports_vk_cache.push_back(current_viewport_vk);
                }
            }

            /* Bake the descriptor */
            viewport_state_create_info.flags         = 0;
            viewport_state_create_info.pNext         = nullptr;
            viewport_state_create_info.pScissors     = (is_dynamic_scissor_state_enabled)  ? VK_NULL_HANDLE
                                                                                           : &scissor_boxes_vk_cache.at(scissor_boxes_start_offset);
            viewport_state_create_info.pViewports    = (is_dynamic_viewport_state_enabled) ? VK_NULL_HANDLE
                                                                                           : &viewports_vk_cache.at(viewports_start_offset);
            viewport_state_create_info.scissorCount  = n_scissor_boxes;
            viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_create_info.viewportCount = n_viewports;

            anvil_assert(viewport_state_create_info.scissorCount == viewport_state_create_info.viewportCount);
            anvil_assert(viewport_state_create_info.scissorCount != 0);

            viewport_state_used = true;

            viewport_state_create_info_items_vk_cache.push_back(viewport_state_create_info);
        }
        else
        {
            viewport_state_used = false;
        }

        /* Bake the GFX pipeline info struct chain */
        {
            VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};

            /* Configure base pipeline handle/indices fields of the create info descriptor */
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
                    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
                    graphics_pipeline_create_info.basePipelineIndex  = static_cast<int32_t>(base_bake_item_iterator - bake_items.begin() );
                }
                else
                if (base_bake_item_iterator->pipeline_ptr                 != nullptr            &&
                    base_bake_item_iterator->pipeline_ptr->baked_pipeline != VK_NULL_HANDLE)
                {
                    /* Case 2 */
                    graphics_pipeline_create_info.basePipelineHandle = base_bake_item_iterator->pipeline_ptr->baked_pipeline;
                    graphics_pipeline_create_info.basePipelineIndex  = UINT32_MAX;
                }
                else
                {
                    /* Case 3 */
                    anvil_assert_fail();
                }
            }
            else
            {
                /* No base pipeline requested */
                graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
                graphics_pipeline_create_info.basePipelineIndex  = UINT32_MAX;
            }

            /* Form the rest of the create info descriptor */
            anvil_assert(current_pipeline_ptr->layout_ptr != nullptr);

            graphics_pipeline_create_info.flags               = 0;
            graphics_pipeline_create_info.layout              = current_pipeline_ptr->layout_ptr->get_pipeline_layout();
            graphics_pipeline_create_info.pColorBlendState    = (color_blend_state_used)    ? &color_blend_state_create_info_items_vk_cache[color_blend_state_create_info_items_vk_cache.size() - 1]
                                                                                            : VK_NULL_HANDLE;
            graphics_pipeline_create_info.pDepthStencilState  = (depth_stencil_state_used)  ? &depth_stencil_state_create_info_items_vk_cache[depth_stencil_state_create_info_items_vk_cache.size() - 1]
                                                                                            : VK_NULL_HANDLE;
            graphics_pipeline_create_info.pDynamicState       = (dynamic_state_used)        ? &dynamic_state_create_info_items_vk_cache[dynamic_state_create_info_items_vk_cache.size() - 1]
                                                                                            : VK_NULL_HANDLE;
            graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info_items_vk_cache[input_assembly_state_create_info_items_vk_cache.size() - 1];
            graphics_pipeline_create_info.pMultisampleState   = (multisample_state_used)    ? &multisample_state_create_info_items_vk_cache[multisample_state_create_info_items_vk_cache.size() - 1]
                                                                                            : VK_NULL_HANDLE;
            graphics_pipeline_create_info.pNext               = nullptr;
            graphics_pipeline_create_info.pRasterizationState = raster_state_create_info_chains_vk_cache.at(raster_state_create_info_chains_vk_cache.size() - 1)->root_struct_ptr;
            graphics_pipeline_create_info.pStages             = (shader_stage_start_offset < shader_stage_create_info_items_vk_cache.size() ) ? &shader_stage_create_info_items_vk_cache[shader_stage_start_offset]
                                                                                                                                              : nullptr;
            graphics_pipeline_create_info.pTessellationState  = (tessellation_state_used)   ? tessellation_state_create_info_chain_cache.at(tessellation_state_create_info_chain_cache.size() - 1)->root_struct_ptr
                                                                                            : VK_NULL_HANDLE;
            graphics_pipeline_create_info.pVertexInputState   = vertex_input_state_create_info_chain_cache.at(vertex_input_state_create_info_chain_cache.size() - 1)->get_root_struct();
            graphics_pipeline_create_info.pViewportState      = (viewport_state_used)       ? &viewport_state_create_info_items_vk_cache[viewport_state_create_info_items_vk_cache.size() - 1]
                                                                                            : VK_NULL_HANDLE;
            graphics_pipeline_create_info.renderPass          = current_pipeline_create_info_ptr->get_renderpass()->get_render_pass();
            graphics_pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stage_create_info_items_vk_cache.size() - shader_stage_start_offset);
            graphics_pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            graphics_pipeline_create_info.subpass             = current_pipeline_create_info_ptr->get_subpass_id();

            if (graphics_pipeline_create_info.basePipelineIndex != static_cast<int32_t>(UINT32_MAX) )
            {
                graphics_pipeline_create_info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
            }

            graphics_pipeline_create_info.flags |= ((current_pipeline_create_info_ptr->allows_derivatives        () ) ? VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT    : 0) |
                                                   ((current_pipeline_create_info_ptr->has_optimizations_disabled() ) ? VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT : 0);

            graphics_pipeline_create_info_chainer.append_struct(graphics_pipeline_create_info);
        }

        /* Stash the descriptor for now. We will issue one expensive vkCreateGraphicsPipelines() call after all pipeline objects
         * are iterated over. */
        graphics_pipeline_create_info_chains.append_struct_chain(
            std::move(graphics_pipeline_create_info_chainer.create_chain() )
        );
    }

    #ifdef _DEBUG
    {
        /* If you hit any of the below assertion failures, please bump up the value under the #define.
         *
         * We use the vectors to hold structure instances. If more than N_CACHE_ITEMS items is ever requested from any of these vectors,
         * the underlying storage will likely be reallocated. Many of the descriptors use pointers to refer to children descriptors,
         * and when the data is moved around, the pointers are no longer valid.
         */
        anvil_assert(color_blend_attachment_states_vk_cache.size()            <= N_CACHE_ITEMS);
        anvil_assert(color_blend_state_create_info_items_vk_cache.size()      <= N_CACHE_ITEMS);
        anvil_assert(depth_stencil_state_create_info_items_vk_cache.size()    <= N_CACHE_ITEMS);
        anvil_assert(dynamic_state_create_info_items_vk_cache.size()          <= N_CACHE_ITEMS);
        anvil_assert(input_assembly_state_create_info_items_vk_cache.size()   <= N_CACHE_ITEMS);
        anvil_assert(multisample_state_create_info_items_vk_cache.size()      <= N_CACHE_ITEMS);
        anvil_assert(raster_state_create_info_chains_vk_cache.size()          <= N_CACHE_ITEMS);
        anvil_assert(scissor_boxes_vk_cache.size()                            <= N_CACHE_ITEMS);
        anvil_assert(shader_stage_create_info_items_vk_cache.size()           <= N_CACHE_ITEMS);
        anvil_assert(specialization_info_vk_cache.size()                      <= N_CACHE_ITEMS);
        anvil_assert(specialization_map_entry_vk_cache.size()                 <= N_CACHE_ITEMS);
        anvil_assert(tessellation_state_create_info_chain_cache.size()        <= N_CACHE_ITEMS);
        anvil_assert(viewport_state_create_info_items_vk_cache.size()         <= N_CACHE_ITEMS);
        anvil_assert(viewports_vk_cache.size()                                <= N_CACHE_ITEMS);
    }
    #endif

    /* All right. Try to bake all pipeline objects at once */
    result_graphics_pipelines.resize(bake_items.size() );

    m_pipeline_cache_ptr->lock();
    {
        result_vk = vkCreateGraphicsPipelines(m_device_ptr->get_device_vk(),
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
bool Anvil::GraphicsPipelineManager::delete_pipeline(PipelineID in_pipeline_id)
{
    bool result;

    lock();
    {
        result = BasePipelineManager::delete_pipeline(in_pipeline_id);

        if (result)
        {
            auto baked_pipelines_iterator                  = m_baked_pipelines.find                 (in_pipeline_id);
            auto pipeline_id_to_gfx_pipeline_data_iterator = m_pipeline_id_to_gfx_pipeline_data.find(in_pipeline_id);

            if (baked_pipelines_iterator != m_baked_pipelines.end() )
            {
                m_baked_pipelines.erase(baked_pipelines_iterator);
            }

            if (pipeline_id_to_gfx_pipeline_data_iterator != m_pipeline_id_to_gfx_pipeline_data.end() )
            {
                m_pipeline_id_to_gfx_pipeline_data.erase(pipeline_id_to_gfx_pipeline_data_iterator);
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
