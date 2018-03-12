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

#include <algorithm>
#include "misc/base_pipeline_info.h"
#include "misc/base_pipeline_manager.h"
#include "misc/debug.h"
#include "misc/graphics_pipeline_info.h"
#include "misc/object_tracker.h"
#include "misc/render_pass_info.h"
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
    auto                                   enabled_dynamic_states_vk_cache                    = std::vector<VkDynamicState>                           ();
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
    auto                                   scissor_boxes_vk_cache                             = std::vector<VkRect2D>                              ();
    auto                                   shader_stage_create_info_items_vk_cache            = std::vector<VkPipelineShaderStageCreateInfo>       ();
    auto                                   specialization_info_vk_cache                       = std::vector<VkSpecializationInfo>                  ();
    auto                                   specialization_map_entry_vk_cache                  = std::vector<VkSpecializationMapEntry>              ();
    auto                                   tessellation_state_create_info_items_vk_cache      = std::vector<VkPipelineTessellationStateCreateInfo> ();
    auto                                   vertex_input_state_create_info_items_vk_cache      = std::vector<VkPipelineVertexInputStateCreateInfo>  ();
    auto                                   viewport_state_create_info_items_vk_cache          = std::vector<VkPipelineViewportStateCreateInfo>     ();
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


    color_blend_attachment_states_vk_cache.reserve            (N_CACHE_ITEMS);
    color_blend_state_create_info_items_vk_cache.reserve      (N_CACHE_ITEMS);
    depth_stencil_state_create_info_items_vk_cache.reserve    (N_CACHE_ITEMS);
    dynamic_state_create_info_items_vk_cache.reserve          (N_CACHE_ITEMS);
    enabled_dynamic_states_vk_cache.reserve                   (N_CACHE_ITEMS);
    input_assembly_state_create_info_items_vk_cache.reserve   (N_CACHE_ITEMS);
    multisample_state_create_info_items_vk_cache.reserve      (N_CACHE_ITEMS);
    raster_state_create_info_chains_vk_cache.reserve          (N_CACHE_ITEMS);
    scissor_boxes_vk_cache.reserve                            (N_CACHE_ITEMS);
    shader_stage_create_info_items_vk_cache.reserve           (N_CACHE_ITEMS);
    specialization_info_vk_cache.reserve                      (N_CACHE_ITEMS);
    specialization_map_entry_vk_cache.reserve                 (N_CACHE_ITEMS);
    tessellation_state_create_info_items_vk_cache.reserve     (N_CACHE_ITEMS);
    vertex_input_state_create_info_items_vk_cache.reserve     (N_CACHE_ITEMS);
    viewport_state_create_info_items_vk_cache.reserve         (N_CACHE_ITEMS);
    viewports_vk_cache.reserve                                (N_CACHE_ITEMS);

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
        auto                                               current_pipeline_info_ptr             = dynamic_cast<GraphicsPipelineInfo*>(bake_item_iterator->pipeline_ptr->pipeline_info_ptr.get() );
        Pipeline*                                          current_pipeline_ptr                  = bake_item_iterator->pipeline_ptr;
        const Anvil::RenderPass*                           current_pipeline_renderpass_ptr       = nullptr;
        Anvil::SubPassID                                   current_pipeline_subpass_id;
        bool                                               depth_stencil_state_used              = false;
        bool                                               dynamic_state_used                    = false;
        Anvil::StructChainer<VkGraphicsPipelineCreateInfo> graphics_pipeline_create_info_chainer;
        VkPipelineInputAssemblyStateCreateInfo             input_assembly_state_create_info;
        bool                                               multisample_state_used                = false;
        uint32_t                                           subpass_n_color_attachments           = 0;
        bool                                               tessellation_state_used               = false;
        VkPipelineVertexInputStateCreateInfo               vertex_input_state_create_info;
        bool                                               viewport_state_used                   = false;

        if (current_pipeline_info_ptr == nullptr)
        {
            anvil_assert(current_pipeline_info_ptr != nullptr);

            continue;
        }

        current_pipeline_renderpass_ptr = current_pipeline_info_ptr->get_renderpass();
        current_pipeline_subpass_id     = current_pipeline_info_ptr->get_subpass_id();

        if (current_pipeline_info_ptr->is_proxy() )
        {
            continue;
        }

        /* Extract subpass information */
        current_pipeline_renderpass_ptr->get_render_pass_info()->get_subpass_n_attachments(current_pipeline_subpass_id,
                                                                                           ATTACHMENT_TYPE_COLOR,
                                                                                          &subpass_n_color_attachments);

        /* Form the color blend state create info descriptor, if needed */
        {
            if (!current_pipeline_info_ptr->is_rasterizer_discard_enabled()     &&
                 subpass_n_color_attachments                                > 0)
            {
                const float*                        blend_constant_ptr            = nullptr;
                VkPipelineColorBlendStateCreateInfo color_blend_state_create_info;
                VkLogicOp                           logic_op;
                bool                                logic_op_enabled              = false;
                const uint32_t                      start_offset                  = static_cast<uint32_t>(color_blend_attachment_states_vk_cache.size() );

                current_pipeline_info_ptr->get_blending_properties(&blend_constant_ptr,
                                                                   nullptr); /* out_opt_n_blend_attachments_ptr */

                current_pipeline_info_ptr->get_logic_op_state(&logic_op_enabled,
                                                              &logic_op);

                color_blend_state_create_info.attachmentCount       = subpass_n_color_attachments;
                color_blend_state_create_info.flags                 = 0;
                color_blend_state_create_info.logicOp               = logic_op;
                color_blend_state_create_info.logicOpEnable         = (logic_op_enabled) ? VK_TRUE : VK_FALSE;
                color_blend_state_create_info.pAttachments          = nullptr;
                color_blend_state_create_info.pNext                 = nullptr;
                color_blend_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

                memcpy(color_blend_state_create_info.blendConstants,
                       blend_constant_ptr,
                       sizeof(color_blend_state_create_info.blendConstants) );

                for (uint32_t n_subpass_color_attachment = 0;
                              n_subpass_color_attachment < subpass_n_color_attachments;
                            ++n_subpass_color_attachment)
                {
                    color_blend_attachment_states_vk_cache.push_back(VkPipelineColorBlendAttachmentState() );

                    VkPipelineColorBlendAttachmentState* blend_attachment_state_ptr         = &color_blend_attachment_states_vk_cache.back();
                    bool                                 is_blending_enabled_for_attachment = false;

                    if (!current_pipeline_info_ptr->get_color_blend_attachment_properties(n_subpass_color_attachment,
                                                                                         &is_blending_enabled_for_attachment,
                                                                                         &blend_attachment_state_ptr->colorBlendOp,
                                                                                         &blend_attachment_state_ptr->alphaBlendOp,
                                                                                         &blend_attachment_state_ptr->srcColorBlendFactor,
                                                                                         &blend_attachment_state_ptr->dstColorBlendFactor,
                                                                                         &blend_attachment_state_ptr->srcAlphaBlendFactor,
                                                                                         &blend_attachment_state_ptr->dstAlphaBlendFactor,
                                                                                         &blend_attachment_state_ptr->colorWriteMask) )
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
                        anvil_assert(is_blending_enabled_for_attachment);

                        blend_attachment_state_ptr->blendEnable = VK_TRUE;
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

                current_pipeline_info_ptr->get_logic_op_state(&logic_op_enabled,
                                                              nullptr); /* out_opt_logic_op_ptr */

                anvil_assert(!logic_op_enabled);

                color_blend_state_used = false;
            }
        }

        /* Form the depth stencil state create info descriptor, if needed */
        {
            VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info;
            VkCompareOp                           depth_test_compare_op            = VK_COMPARE_OP_MAX_ENUM;
            bool                                  is_depth_bounds_test_enabled     = false;
            bool                                  is_depth_test_enabled            = false;
            bool                                  is_stencil_test_enabled          = false;
            float                                 max_depth_bounds                 = std::numeric_limits<float>::max();
            float                                 min_depth_bounds                 = std::numeric_limits<float>::max();
            uint32_t                              n_depth_stencil_attachments      = 0;

            current_pipeline_info_ptr->get_depth_bounds_state(&is_depth_bounds_test_enabled,
                                                              &min_depth_bounds,
                                                              &max_depth_bounds);

            current_pipeline_info_ptr->get_depth_test_state(&is_depth_test_enabled,
                                                            &depth_test_compare_op);

            current_pipeline_info_ptr->get_stencil_test_properties(&is_stencil_test_enabled,
                                                                   &depth_stencil_state_create_info.front.failOp,
                                                                   &depth_stencil_state_create_info.front.passOp,
                                                                   &depth_stencil_state_create_info.front.depthFailOp,
                                                                   &depth_stencil_state_create_info.front.compareOp,
                                                                   &depth_stencil_state_create_info.front.compareMask,
                                                                   &depth_stencil_state_create_info.front.writeMask,
                                                                   &depth_stencil_state_create_info.front.reference,
                                                                   &depth_stencil_state_create_info.back.failOp,
                                                                   &depth_stencil_state_create_info.back.passOp,
                                                                   &depth_stencil_state_create_info.back.depthFailOp,
                                                                   &depth_stencil_state_create_info.back.compareOp,
                                                                   &depth_stencil_state_create_info.back.compareMask,
                                                                   &depth_stencil_state_create_info.back.writeMask,
                                                                   &depth_stencil_state_create_info.back.reference);

            current_pipeline_renderpass_ptr->get_render_pass_info()->get_subpass_n_attachments(current_pipeline_info_ptr->get_subpass_id(),
                                                                                               ATTACHMENT_TYPE_DEPTH_STENCIL,
                                                                                              &n_depth_stencil_attachments);

            if (n_depth_stencil_attachments)
            {
                depth_stencil_state_create_info.depthBoundsTestEnable = is_depth_bounds_test_enabled ? VK_TRUE : VK_FALSE;
                depth_stencil_state_create_info.depthCompareOp        = depth_test_compare_op;
                depth_stencil_state_create_info.depthTestEnable       = is_depth_test_enabled                                 ? VK_TRUE : VK_FALSE;
                depth_stencil_state_create_info.depthWriteEnable      = current_pipeline_info_ptr->are_depth_writes_enabled() ? VK_TRUE : VK_FALSE;
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
                anvil_assert(!current_pipeline_info_ptr->are_depth_writes_enabled());
                anvil_assert(!is_stencil_test_enabled);

                depth_stencil_state_used = false;
            }
        }

        /* Form the dynamic state create info descriptor, if needed */
        {
            const auto& enabled_dynamic_states = current_pipeline_info_ptr->get_enabled_dynamic_states();

            if (enabled_dynamic_states != 0)
            {
                VkPipelineDynamicStateCreateInfo dynamic_state_create_info;
                const uint32_t                   start_offset = static_cast<uint32_t>(enabled_dynamic_states_vk_cache.size() );

                if ((enabled_dynamic_states & DYNAMIC_STATE_BLEND_CONSTANTS_BIT) != 0)
                {
                    enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
                }

                if ((enabled_dynamic_states & DYNAMIC_STATE_DEPTH_BIAS_BIT) != 0)
                {
                    enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
                }

                if ((enabled_dynamic_states & DYNAMIC_STATE_DEPTH_BOUNDS_BIT) != 0)
                {
                    enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
                }

                if ((enabled_dynamic_states & DYNAMIC_STATE_LINE_WIDTH_BIT) != 0)
                {
                    enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
                }

                if ((enabled_dynamic_states & DYNAMIC_STATE_SCISSOR_BIT) != 0)
                {
                    enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_SCISSOR);
                }

                if ((enabled_dynamic_states & DYNAMIC_STATE_STENCIL_COMPARE_MASK_BIT) != 0)
                {
                    enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
                }

                if ((enabled_dynamic_states & DYNAMIC_STATE_STENCIL_REFERENCE_BIT) != 0)
                {
                    enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
                }

                if ((enabled_dynamic_states & DYNAMIC_STATE_STENCIL_WRITE_MASK_BIT) != 0)
                {
                    enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
                }

                if ((enabled_dynamic_states & DYNAMIC_STATE_VIEWPORT_BIT) != 0)
                {
                    enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_VIEWPORT);
                }

                dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(enabled_dynamic_states_vk_cache.size() - start_offset);
                dynamic_state_create_info.flags             = 0;
                dynamic_state_create_info.pDynamicStates    = (dynamic_state_create_info.dynamicStateCount > 0) ? &enabled_dynamic_states_vk_cache[start_offset]
                                                                                                                : VK_NULL_HANDLE;
                dynamic_state_create_info.pNext             = nullptr;
                dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

                dynamic_state_used = true;

                dynamic_state_create_info_items_vk_cache.push_back(dynamic_state_create_info);
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
            input_assembly_state_create_info.primitiveRestartEnable = current_pipeline_info_ptr->is_primitive_restart_enabled() ? VK_TRUE : VK_FALSE;
            input_assembly_state_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_state_create_info.topology               = current_pipeline_info_ptr->get_primitive_topology();

            input_assembly_state_create_info_items_vk_cache.push_back(input_assembly_state_create_info);
        }

        /* Form the multisample state create info descriptor, if needed */
        {
            bool  is_sample_shading_enabled = false;
            float min_sample_shading        = std::numeric_limits<float>::max();

            current_pipeline_info_ptr->get_sample_shading_state(&is_sample_shading_enabled,
                                                                &min_sample_shading);

            if (!current_pipeline_info_ptr->is_rasterizer_discard_enabled() )
            {
                const bool                           is_sample_mask_enabled        = current_pipeline_info_ptr->is_sample_mask_enabled();
                VkPipelineMultisampleStateCreateInfo multisample_state_create_info;
                VkSampleCountFlags                   sample_count                  = VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
                VkSampleMask                         sample_mask;

                current_pipeline_info_ptr->get_multisampling_properties(&sample_count,
                                                                        &sample_mask);

                /* If sample mask is not enabled, Vulkan spec will assume all samples need to pass. This is what the default sample mask value mimics.
                *
                 * Hence, if the application specified a non-~0u sample mask and has NOT enabled the sample mask using toggle_sample_mask(), it's (in
                 * all likelihood) a trivial app-side issue.
                 */
                anvil_assert((!is_sample_mask_enabled && sample_mask == ~0u) ||
                               is_sample_mask_enabled);

                multisample_state_create_info.alphaToCoverageEnable = current_pipeline_info_ptr->is_alpha_to_coverage_enabled() ? VK_TRUE : VK_FALSE;
                multisample_state_create_info.alphaToOneEnable      = current_pipeline_info_ptr->is_alpha_to_one_enabled()      ? VK_TRUE : VK_FALSE;
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
                anvil_assert(!current_pipeline_info_ptr->is_alpha_to_coverage_enabled() );
                anvil_assert(!current_pipeline_info_ptr->is_alpha_to_one_enabled     () );
                anvil_assert(!is_sample_shading_enabled);

                multisample_state_used = false;
            }
        }

        /* Form the raster state create info chain */
        {
            VkCullModeFlags                                              cull_mode                  = VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
            float                                                        depth_bias_clamp           = std::numeric_limits<float>::max();
            float                                                        depth_bias_constant_factor = std::numeric_limits<float>::max();
            float                                                        depth_bias_slope_factor    = std::numeric_limits<float>::max();
            VkFrontFace                                                  front_face                 = VK_FRONT_FACE_MAX_ENUM;
            bool                                                         is_depth_bias_enabled      = false;
            float                                                        line_width                 = std::numeric_limits<float>::max();
            VkPolygonMode                                                polygon_mode               = VK_POLYGON_MODE_MAX_ENUM;
            Anvil::StructChainer<VkPipelineRasterizationStateCreateInfo> raster_state_create_info_chainer;

            current_pipeline_info_ptr->get_depth_bias_state        (&is_depth_bias_enabled,
                                                                    &depth_bias_constant_factor,
                                                                    &depth_bias_clamp,
                                                                    &depth_bias_slope_factor);
            current_pipeline_info_ptr->get_rasterization_properties(&polygon_mode,
                                                                    &cull_mode,
                                                                    &front_face,
                                                                    &line_width);

            {
                VkPipelineRasterizationStateCreateInfo raster_state_create_info;

                raster_state_create_info.cullMode                = cull_mode;
                raster_state_create_info.depthBiasClamp          = depth_bias_clamp;
                raster_state_create_info.depthBiasConstantFactor = depth_bias_constant_factor;
                raster_state_create_info.depthBiasEnable         = is_depth_bias_enabled ? VK_TRUE : VK_FALSE;
                raster_state_create_info.depthBiasSlopeFactor    = depth_bias_slope_factor;
                raster_state_create_info.depthClampEnable        = current_pipeline_info_ptr->is_depth_clamp_enabled() ? VK_TRUE : VK_FALSE;
                raster_state_create_info.flags                   = 0;
                raster_state_create_info.frontFace               = front_face;
                raster_state_create_info.lineWidth               = line_width;
                raster_state_create_info.pNext                   = nullptr;
                raster_state_create_info.polygonMode             = polygon_mode;
                raster_state_create_info.rasterizerDiscardEnable = current_pipeline_info_ptr->is_rasterizer_discard_enabled() ? VK_TRUE : VK_FALSE;
                raster_state_create_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

                raster_state_create_info_chainer.append_struct(raster_state_create_info);
            }

            if (m_device_ptr->is_extension_enabled(VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME) )
            {
                /* Chain a predefined struct which will toggle the relaxed rasterization, as long as the device supports the
                 * VK_AMD_rasterization_order extension.
                 */
                if (current_pipeline_info_ptr->get_rasterization_order() != VK_RASTERIZATION_ORDER_STRICT_AMD)
                {
                    anvil_assert(current_pipeline_info_ptr->get_rasterization_order() == relaxed_rasterization_order_item.rasterizationOrder);

                    raster_state_create_info_chainer.append_struct(relaxed_rasterization_order_item);
                }
                else
                {
                    raster_state_create_info_chainer.append_struct(strict_rasterization_order_item);
                }
            }
            else
            if (current_pipeline_info_ptr->get_rasterization_order() != VK_RASTERIZATION_ORDER_STRICT_AMD)
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
                Anvil::SHADER_STAGE_FRAGMENT,
                Anvil::SHADER_STAGE_GEOMETRY,
                Anvil::SHADER_STAGE_TESSELLATION_CONTROL,
                Anvil::SHADER_STAGE_TESSELLATION_EVALUATION,
                Anvil::SHADER_STAGE_VERTEX
            };

            for (const auto& current_graphics_shader_stage : graphics_shader_stages)
            {
                const Anvil::ShaderModuleStageEntryPoint* shader_stage_entry_point_ptr = nullptr;

                current_pipeline_info_ptr->get_shader_stage_properties(current_graphics_shader_stage,
                                                                      &shader_stage_entry_point_ptr);

                if (shader_stage_entry_point_ptr != nullptr)
                {
                    VkPipelineShaderStageCreateInfo current_shader_stage_create_info;
                    const unsigned char*            current_shader_stage_specialization_constants_data_buffer_ptr = nullptr;
                    const SpecializationConstants*  current_shader_stage_specialization_constants_ptr             = nullptr;
                    Anvil::ShaderModule*            shader_module_ptr                                             = nullptr;

                    specialization_info_vk_cache.push_back(VkSpecializationInfo() );

                    current_pipeline_info_ptr->get_specialization_constants(current_graphics_shader_stage,
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
                    current_shader_stage_create_info.pName  = (shader_stage_entry_point_ptr->stage == Anvil::SHADER_STAGE_FRAGMENT)                ? shader_module_ptr->get_fs_entrypoint_name().c_str()
                                                            : (shader_stage_entry_point_ptr->stage == Anvil::SHADER_STAGE_GEOMETRY)                ? shader_module_ptr->get_gs_entrypoint_name().c_str()
                                                            : (shader_stage_entry_point_ptr->stage == Anvil::SHADER_STAGE_TESSELLATION_CONTROL)    ? shader_module_ptr->get_tc_entrypoint_name().c_str()
                                                            : (shader_stage_entry_point_ptr->stage == Anvil::SHADER_STAGE_TESSELLATION_EVALUATION) ? shader_module_ptr->get_te_entrypoint_name().c_str()
                                                            : (shader_stage_entry_point_ptr->stage == Anvil::SHADER_STAGE_VERTEX)                  ? shader_module_ptr->get_vs_entrypoint_name().c_str()
                                                            : nullptr;

                    current_shader_stage_create_info.flags               = 0;
                    current_shader_stage_create_info.pNext               = nullptr;
                    current_shader_stage_create_info.pSpecializationInfo = (current_shader_stage_specialization_constants_ptr->size() > 0) ? &specialization_info_vk_cache.back()
                                                                                                                                           : VK_NULL_HANDLE;
                    current_shader_stage_create_info.stage               = Anvil::Utils::get_shader_stage_flag_bits_from_shader_stage(shader_stage_entry_point_ptr->stage);
                    current_shader_stage_create_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

                    shader_stage_create_info_items_vk_cache.push_back(current_shader_stage_create_info);
                }
            }
        }

        /* Form the tessellation state create info descriptor if needed */
        {
            const Anvil::ShaderModuleStageEntryPoint* tc_shader_stage_entry_point_ptr = nullptr;
            const Anvil::ShaderModuleStageEntryPoint* te_shader_stage_entry_point_ptr = nullptr;

            current_pipeline_info_ptr->get_shader_stage_properties(Anvil::SHADER_STAGE_TESSELLATION_CONTROL,
                                                                  &tc_shader_stage_entry_point_ptr);
            current_pipeline_info_ptr->get_shader_stage_properties(Anvil::SHADER_STAGE_TESSELLATION_EVALUATION,
                                                                  &te_shader_stage_entry_point_ptr);

            if (tc_shader_stage_entry_point_ptr != nullptr &&
                te_shader_stage_entry_point_ptr != nullptr)
            {
                VkPipelineTessellationStateCreateInfo tessellation_state_create_info;

                tessellation_state_create_info.flags              = 0;
                tessellation_state_create_info.patchControlPoints = current_pipeline_info_ptr->get_n_patch_control_points();
                tessellation_state_create_info.pNext              = nullptr;
                tessellation_state_create_info.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

                tessellation_state_used = true;

                tessellation_state_create_info_items_vk_cache.push_back(tessellation_state_create_info);
            }
            else
            {
                tessellation_state_used = false;
            }
        }

        /* Form the vertex input state create info descriptor */
        {
            std::unique_ptr<GraphicsPipelineData> current_pipeline_gfx_data_ptr;

            anvil_assert(m_pipeline_id_to_gfx_pipeline_data.find(current_pipeline_id) == m_pipeline_id_to_gfx_pipeline_data.end() );

            current_pipeline_gfx_data_ptr.reset(
                new GraphicsPipelineData(current_pipeline_info_ptr)
            );

            if (current_pipeline_gfx_data_ptr == nullptr)
            {
                anvil_assert(current_pipeline_gfx_data_ptr != nullptr);

                continue;
            }

            vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(current_pipeline_gfx_data_ptr->vk_input_attributes.size());
            vertex_input_state_create_info.vertexBindingDescriptionCount   = static_cast<uint32_t>(current_pipeline_gfx_data_ptr->vk_input_bindings.size  ());

            vertex_input_state_create_info.flags                        = 0;
            vertex_input_state_create_info.pNext                        = nullptr;
            vertex_input_state_create_info.pVertexAttributeDescriptions = (vertex_input_state_create_info.vertexAttributeDescriptionCount > 0) ? &current_pipeline_gfx_data_ptr->vk_input_attributes.at(0)
                                                                                                                                               : nullptr;
            vertex_input_state_create_info.pVertexBindingDescriptions   = (vertex_input_state_create_info.vertexBindingDescriptionCount   > 0) ? &current_pipeline_gfx_data_ptr->vk_input_bindings.at(0)
                                                                                                                                               : nullptr;
            vertex_input_state_create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            vertex_input_state_create_info_items_vk_cache.push_back(vertex_input_state_create_info);

            m_pipeline_id_to_gfx_pipeline_data[current_pipeline_id] = std::move(current_pipeline_gfx_data_ptr);
        }

        /* Form the viewport state create info descriptor, if needed */
        if (!current_pipeline_info_ptr->is_rasterizer_discard_enabled() )
        {
            const auto                        enabled_dynamic_states     = current_pipeline_info_ptr->get_enabled_dynamic_states();
            uint32_t                          n_scissor_boxes            = (enabled_dynamic_states & DYNAMIC_STATE_SCISSOR_BIT)  ? current_pipeline_info_ptr->get_n_dynamic_scissor_boxes()
                                                                                                                                 : current_pipeline_info_ptr->get_n_scissor_boxes        ();
            uint32_t                          n_viewports                = (enabled_dynamic_states & DYNAMIC_STATE_VIEWPORT_BIT) ? current_pipeline_info_ptr->get_n_dynamic_viewports    ()
                                                                                                                                 : current_pipeline_info_ptr->get_n_viewports            ();
            const uint32_t                    scissor_boxes_start_offset = static_cast<uint32_t>(scissor_boxes_vk_cache.size() );
            const uint32_t                    viewports_start_offset     = static_cast<uint32_t>(viewports_vk_cache.size() );
            VkPipelineViewportStateCreateInfo viewport_state_create_info;

            anvil_assert(n_scissor_boxes == n_viewports);

            if (n_scissor_boxes == 0)
            {
                /* No scissor boxes / viewport defined. Use default settings.. */
                auto     swapchain_ptr  = current_pipeline_info_ptr->get_renderpass()->get_swapchain();
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

                current_pipeline_info_ptr->set_scissor_box_properties(0, /* in_n_scissor_box */
                                                                      0, /* in_x             */
                                                                      0, /* in_y             */
                                                                      window_size[0],
                                                                      window_size[1]);
                current_pipeline_info_ptr->set_viewport_properties   (0,    /* in_n_viewport */
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
            if ((enabled_dynamic_states & DYNAMIC_STATE_SCISSOR_BIT) == 0)
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

                    current_pipeline_info_ptr->get_scissor_box_properties(n_scissor_box,
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

            if ((enabled_dynamic_states & DYNAMIC_STATE_VIEWPORT_BIT) == 0)
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

                    current_pipeline_info_ptr->get_viewport_properties(n_viewport,
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
            viewport_state_create_info.pScissors     = ((enabled_dynamic_states & DYNAMIC_STATE_SCISSOR_BIT)  != 0) ? VK_NULL_HANDLE
                                                                                                                    : &scissor_boxes_vk_cache.at(scissor_boxes_start_offset);
            viewport_state_create_info.pViewports    = ((enabled_dynamic_states & DYNAMIC_STATE_VIEWPORT_BIT) != 0) ? VK_NULL_HANDLE
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
            VkGraphicsPipelineCreateInfo                       graphics_pipeline_create_info          = {};

            /* Configure base pipeline handle/indices fields of the create info descriptor */
            const auto current_pipeline_base_pipeline_id = current_pipeline_info_ptr->get_base_pipeline_id();

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
            graphics_pipeline_create_info.pTessellationState  = (tessellation_state_used)   ? &tessellation_state_create_info_items_vk_cache[tessellation_state_create_info_items_vk_cache.size() - 1]
                                                                                            : VK_NULL_HANDLE;
            graphics_pipeline_create_info.pVertexInputState   = &vertex_input_state_create_info_items_vk_cache[vertex_input_state_create_info_items_vk_cache.size() - 1];
            graphics_pipeline_create_info.pViewportState      = (viewport_state_used)       ? &viewport_state_create_info_items_vk_cache[viewport_state_create_info_items_vk_cache.size() - 1]
                                                                                            : VK_NULL_HANDLE;
            graphics_pipeline_create_info.renderPass          = current_pipeline_info_ptr->get_renderpass()->get_render_pass();
            graphics_pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stage_create_info_items_vk_cache.size() - shader_stage_start_offset);
            graphics_pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            graphics_pipeline_create_info.subpass             = current_pipeline_info_ptr->get_subpass_id();

            if (graphics_pipeline_create_info.basePipelineIndex != static_cast<int32_t>(UINT32_MAX) )
            {
                graphics_pipeline_create_info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
            }

            graphics_pipeline_create_info.flags |= ((current_pipeline_info_ptr->allows_derivatives        () ) ? VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT    : 0) |
                                                   ((current_pipeline_info_ptr->has_optimizations_disabled() ) ? VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT : 0);

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
        anvil_assert(enabled_dynamic_states_vk_cache.size()                   <= N_CACHE_ITEMS);
        anvil_assert(input_assembly_state_create_info_items_vk_cache.size()   <= N_CACHE_ITEMS);
        anvil_assert(multisample_state_create_info_items_vk_cache.size()      <= N_CACHE_ITEMS);
        anvil_assert(raster_state_create_info_chains_vk_cache.size()          <= N_CACHE_ITEMS);
        anvil_assert(scissor_boxes_vk_cache.size()                            <= N_CACHE_ITEMS);
        anvil_assert(shader_stage_create_info_items_vk_cache.size()           <= N_CACHE_ITEMS);
        anvil_assert(specialization_info_vk_cache.size()                      <= N_CACHE_ITEMS);
        anvil_assert(specialization_map_entry_vk_cache.size()                 <= N_CACHE_ITEMS);
        anvil_assert(tessellation_state_create_info_items_vk_cache.size()     <= N_CACHE_ITEMS);
        anvil_assert(vertex_input_state_create_info_items_vk_cache.size()     <= N_CACHE_ITEMS);
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
    anvil_assert(vk_input_attributes.size                    () == 0);
    anvil_assert(vk_input_bindings.size                      () == 0);

    pipeline_info_ptr->get_graphics_pipeline_properties(nullptr,       /* out_opt_n_scissors_ptr          */
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
        uint32_t                          current_attribute_explicit_vertex_binding_index = UINT32_MAX;
        uint32_t                          current_attribute_location                      = UINT32_MAX;
        VkFormat                          current_attribute_format                        = VK_FORMAT_MAX_ENUM;
        uint32_t                          current_attribute_offset                        = UINT32_MAX;
        VkVertexInputRate                 current_attribute_rate                          = VK_VERTEX_INPUT_RATE_MAX_ENUM;
        uint32_t                          current_attribute_stride                        = UINT32_MAX;
        VkVertexInputAttributeDescription current_attribute_vk;
        uint32_t                          n_attribute_binding                             = UINT32_MAX;
        bool                              has_found                                       = false;

        if (!pipeline_info_ptr->get_vertex_attribute_properties(n_attribute,
                                                               &current_attribute_location,
                                                               &current_attribute_format,
                                                               &current_attribute_offset,
                                                               &current_attribute_explicit_vertex_binding_index,
                                                               &current_attribute_stride,
                                                               &current_attribute_rate) )
        {
            anvil_assert_fail();

            continue;
        }

        if (current_attribute_explicit_vertex_binding_index == UINT32_MAX)
        {
            for (auto vk_input_binding_iterator  = vk_input_bindings.begin();
                      vk_input_binding_iterator != vk_input_bindings.end();
                    ++vk_input_binding_iterator)
            {
                if (vk_input_binding_iterator->inputRate == current_attribute_rate  &&
                    vk_input_binding_iterator->stride    == current_attribute_stride)
                {
                    has_found           = true;
                    n_attribute_binding = static_cast<uint32_t>(vk_input_binding_iterator - vk_input_bindings.begin());

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
            VkVertexInputBindingDescription new_binding_vk;

            new_binding_vk.binding   = (current_attribute_explicit_vertex_binding_index == UINT32_MAX) ? static_cast<uint32_t>(vk_input_bindings.size() )
                                                                                                       : current_attribute_explicit_vertex_binding_index;
            new_binding_vk.inputRate = current_attribute_rate;
            new_binding_vk.stride    = current_attribute_stride;

            vk_input_bindings.push_back(new_binding_vk);

            n_attribute_binding = new_binding_vk.binding;
        }

        /* Good to convert the internal attribute descriptor to the Vulkan's input attribute descriptor */
        current_attribute_vk.binding  = n_attribute_binding;
        current_attribute_vk.format   = current_attribute_format;
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
