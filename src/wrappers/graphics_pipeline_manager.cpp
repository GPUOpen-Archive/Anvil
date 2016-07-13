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

#include <algorithm>
#include "misc/base_pipeline_manager.h"
#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/device.h"
#include "wrappers/graphics_pipeline_manager.h"
#include "wrappers/pipeline_cache.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/render_pass.h"
#include "wrappers/shader_module.h"
#include "wrappers/swapchain.h"

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
#define N_CACHE_ITEMS 128


/* Please see header for specification */
Anvil::GraphicsPipelineManager::GraphicsPipelineManager(Anvil::Device*        device_ptr,
                                                        bool                  use_pipeline_cache,
                                                        Anvil::PipelineCache* pipeline_cache_to_reuse_ptr)
    :BasePipelineManager(device_ptr,
                         use_pipeline_cache,
                         pipeline_cache_to_reuse_ptr)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_GRAPHICS_PIPELINE_MANAGER,
                                                  this);
}

/* Please see header for specification */
Anvil::GraphicsPipelineManager::~GraphicsPipelineManager()
{
    m_pipelines.clear();

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_GRAPHICS_PIPELINE_MANAGER,
                                                    this);
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::add_derivative_pipeline_from_sibling_pipeline(bool                               disable_optimizations,
                                                                                   bool                               allow_derivatives,
                                                                                   RenderPass*                        renderpass_ptr,
                                                                                   SubPassID                          subpass_id,
                                                                                   const ShaderModuleStageEntryPoint& fragment_shader_stage_entrypoint_info,
                                                                                   const ShaderModuleStageEntryPoint& geometry_shader_stage_entrypoint_info,
                                                                                   const ShaderModuleStageEntryPoint& tess_control_shader_stage_entrypoint_info,
                                                                                   const ShaderModuleStageEntryPoint& tess_evaluation_shader_stage_entrypoint_info,
                                                                                   const ShaderModuleStageEntryPoint& vertex_shader_shader_stage_entrypoint_info,
                                                                                   GraphicsPipelineID                 base_pipeline_id,
                                                                                   GraphicsPipelineID*                out_graphics_pipeline_id_ptr)
{
    bool                        result;
    ShaderModuleStageEntryPoint stages[GRAPHICS_SHADER_STAGE_COUNT];

    stages[GRAPHICS_SHADER_STAGE_FRAGMENT]                = fragment_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_GEOMETRY]                = geometry_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_TESSELLATION_CONTROL]    = tess_control_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_TESSELLATION_EVALUATION] = tess_evaluation_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_VERTEX]                  = vertex_shader_shader_stage_entrypoint_info;

    result = BasePipelineManager::add_derivative_pipeline_from_sibling_pipeline(disable_optimizations,
                                                                                allow_derivatives,
                                                                                GRAPHICS_SHADER_STAGE_COUNT,
                                                                                stages,
                                                                                base_pipeline_id,
                                                                                out_graphics_pipeline_id_ptr);
    anvil_assert(result);

    if (result)
    {
        anvil_assert(m_pipeline_configurations.find(base_pipeline_id)              != m_pipeline_configurations.end() );
        anvil_assert(m_pipeline_configurations.find(*out_graphics_pipeline_id_ptr) == m_pipeline_configurations.end() );

        m_pipeline_configurations[*out_graphics_pipeline_id_ptr] = std::shared_ptr<GraphicsPipelineConfiguration>(new GraphicsPipelineConfiguration(m_pipeline_configurations[base_pipeline_id],
                                                                                                                                                    renderpass_ptr,
                                                                                                                                                    subpass_id) );
    }

    return result;
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::add_derivative_pipeline_from_pipeline(bool                               disable_optimizations,
                                                                           bool                               allow_derivatives,
                                                                           RenderPass*                        renderpass_ptr,
                                                                           SubPassID                          subpass_id,
                                                                           const ShaderModuleStageEntryPoint& fragment_shader_stage_entrypoint_info,
                                                                           const ShaderModuleStageEntryPoint& geometry_shader_stage_entrypoint_info,
                                                                           const ShaderModuleStageEntryPoint& tess_control_shader_stage_entrypoint_info,
                                                                           const ShaderModuleStageEntryPoint& tess_evaluation_shader_stage_entrypoint_info,
                                                                           const ShaderModuleStageEntryPoint& vertex_shader_shader_stage_entrypoint_info,
                                                                           VkPipeline                         base_pipeline,
                                                                           GraphicsPipelineID*                out_graphics_pipeline_id_ptr)
{
    bool                        result;
    ShaderModuleStageEntryPoint stages[GRAPHICS_SHADER_STAGE_COUNT];

    stages[GRAPHICS_SHADER_STAGE_FRAGMENT]                = fragment_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_GEOMETRY]                = geometry_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_TESSELLATION_CONTROL]    = tess_control_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_TESSELLATION_EVALUATION] = tess_evaluation_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_VERTEX]                  = vertex_shader_shader_stage_entrypoint_info;

    result = BasePipelineManager::add_derivative_pipeline_from_pipeline(disable_optimizations,
                                                                        allow_derivatives,
                                                                        GRAPHICS_SHADER_STAGE_COUNT,
                                                                        stages,
                                                                        base_pipeline,
                                                                        out_graphics_pipeline_id_ptr);
    anvil_assert(result);

    if (result)
    {
        anvil_assert(m_pipeline_configurations.find(*out_graphics_pipeline_id_ptr) == m_pipeline_configurations.end() );

        m_pipeline_configurations[*out_graphics_pipeline_id_ptr] = std::shared_ptr<GraphicsPipelineConfiguration>(new GraphicsPipelineConfiguration(renderpass_ptr,
                                                                                                                                                    subpass_id) );
    }

    return result;
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::add_proxy_pipeline(GraphicsPipelineID* out_proxy_graphics_pipeline_id_ptr)
{
    bool result = false;

    /* For proxy pipelines, we *only* need the configuration container. */
    result = BasePipelineManager::add_proxy_pipeline(out_proxy_graphics_pipeline_id_ptr);

    anvil_assert(result);
    if (result)
    {
        anvil_assert(m_pipeline_configurations.find(*out_proxy_graphics_pipeline_id_ptr) == m_pipeline_configurations.end() );

        m_pipeline_configurations[*out_proxy_graphics_pipeline_id_ptr] = std::shared_ptr<GraphicsPipelineConfiguration>(new GraphicsPipelineConfiguration(nullptr,  /* in_renderpass_ptr */
                                                                                                                                                          -1) );    /* in_subpass_id     */
    }

    return result;
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::add_regular_pipeline(bool                               disable_optimizations,
                                                          bool                               allow_derivatives,
                                                          RenderPass*                        renderpass_ptr,
                                                          SubPassID                          subpass_id,
                                                          const ShaderModuleStageEntryPoint& fragment_shader_stage_entrypoint_info,
                                                          const ShaderModuleStageEntryPoint& geometry_shader_stage_entrypoint_info,
                                                          const ShaderModuleStageEntryPoint& tess_control_shader_stage_entrypoint_info,
                                                          const ShaderModuleStageEntryPoint& tess_evaluation_shader_stage_entrypoint_info,
                                                          const ShaderModuleStageEntryPoint& vertex_shader_shader_stage_entrypoint_info,
                                                          GraphicsPipelineID*                out_graphics_pipeline_id_ptr)
{
    bool                        result;
    ShaderModuleStageEntryPoint stages[GRAPHICS_SHADER_STAGE_COUNT];

    stages[GRAPHICS_SHADER_STAGE_FRAGMENT]                = fragment_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_GEOMETRY]                = geometry_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_TESSELLATION_CONTROL]    = tess_control_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_TESSELLATION_EVALUATION] = tess_evaluation_shader_stage_entrypoint_info;
    stages[GRAPHICS_SHADER_STAGE_VERTEX]                  = vertex_shader_shader_stage_entrypoint_info;

    result = BasePipelineManager::add_regular_pipeline(disable_optimizations,
                                                       allow_derivatives,
                                                       GRAPHICS_SHADER_STAGE_COUNT,
                                                       stages,
                                                       out_graphics_pipeline_id_ptr);
    anvil_assert(result);

    if (result)
    {
        anvil_assert(m_pipeline_configurations.find(*out_graphics_pipeline_id_ptr) == m_pipeline_configurations.end() );

        m_pipeline_configurations[*out_graphics_pipeline_id_ptr] = std::shared_ptr<GraphicsPipelineConfiguration>(new GraphicsPipelineConfiguration(renderpass_ptr,
                                                                                                                                                    subpass_id) );
    }

    return result;
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::add_vertex_attribute(GraphicsPipelineID    graphics_pipeline_id,
                                                          uint32_t              location,
                                                          VkFormat              format,
                                                          uint32_t              offset_in_bytes,
                                                          uint32_t              stride_in_bytes,
                                                          VkVertexInputRate     rate)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    bool                                           result            = false;

    /* Retrieve the pipeline config descriptor ..*/
    if (pipeline_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(false);

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_iterator->second;
    }

    #ifdef _DEBUG
    {
        /* Make sure the location is not already referred to by already added attributes */
        for (auto attribute_iterator  = pipeline_config_ptr->attributes.cbegin();
                  attribute_iterator != pipeline_config_ptr->attributes.cend();
                ++attribute_iterator)
        {
            anvil_assert(attribute_iterator->location != location);
        }
    }
    #endif

    /* Add a new vertex attribute descriptor. At this point, we do not differentiate between
     * attributes and bindings. Actual Vulkan attributes and bindings will be created at baking
     * time. */
    pipeline_config_ptr->attributes.push_back(InternalVertexAttribute(format,
                                                                      location,
                                                                      offset_in_bytes,
                                                                      rate,
                                                                      stride_in_bytes));

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::bake()
{
    std::vector<VkPipelineColorBlendAttachmentState>    color_blend_attachment_states_vk_cache;
    std::vector<VkPipelineColorBlendStateCreateInfo>    color_blend_state_create_info_items_vk_cache;
    std::vector<VkPipelineDepthStencilStateCreateInfo>  depth_stencil_state_create_info_items_vk_cache;
    std::vector<VkPipelineDynamicStateCreateInfo>       dynamic_state_create_info_items_vk_cache;
    std::vector<VkDynamicState>                         enabled_dynamic_states_vk_cache;
    std::vector<VkGraphicsPipelineCreateInfo>           graphics_pipeline_create_info_items_vk_cache;
    std::vector<VkPipelineInputAssemblyStateCreateInfo> input_assembly_state_create_info_items_vk_cache;
    std::vector<VkPipelineMultisampleStateCreateInfo>   multisample_state_create_info_items_vk_cache;
    uint32_t                                            n_consumed_graphics_pipelines = 0;
    std::vector<VkPipelineRasterizationStateCreateInfo> raster_state_create_info_items_vk_cache;
    bool                                                result                        = false;
    std::vector<VkPipeline>                             result_graphics_pipelines;
    VkResult                                            result_vk;
    std::vector<VkRect2D>                               scissor_boxes_vk_cache;
    std::vector<VkPipelineShaderStageCreateInfo>        shader_stage_create_info_items_vk_cache;
    std::vector<VkSpecializationInfo>                   specialization_info_vk_cache;
    std::vector<VkSpecializationMapEntry>               specialization_map_entry_vk_cache;
    std::vector<VkPipelineTessellationStateCreateInfo>  tessellation_state_create_info_items_vk_cache;
    std::vector<VkPipelineVertexInputStateCreateInfo>   vertex_input_state_create_info_items_vk_cache;
    std::vector<VkPipelineViewportStateCreateInfo>      viewport_state_create_info_items_vk_cache;
    std::vector<VkViewport>                             viewports_vk_cache;

    static const VkPipelineRasterizationStateRasterizationOrderAMD relaxed_rasterization_order_item =
    {
        static_cast<VkStructureType>(1000018000), /* VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD */
        nullptr,
        VK_RASTERIZATION_ORDER_RELAXED_AMD
    };
    static const VkPipelineRasterizationStateRasterizationOrderAMD strict_rasterization_order_item =
    {
        static_cast<VkStructureType>(1000018000), /* VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD */
        nullptr,
        VK_RASTERIZATION_ORDER_STRICT_AMD
    };

    /* Iterate over all stored pipelines, focus on the ones marked as dirty. */
    typedef struct _bake_item
    {
        PipelineID                pipeline_id;
        std::shared_ptr<Pipeline> pipeline_ptr;

        _bake_item(PipelineID                in_pipeline_id,
                   std::shared_ptr<Pipeline> in_pipeline_ptr)
        {
            pipeline_id  = in_pipeline_id;
            pipeline_ptr = in_pipeline_ptr;
        }

        bool operator==(std::shared_ptr<Pipeline> in_pipeline_ptr)
        {
            return pipeline_ptr == in_pipeline_ptr;
        }
    } _bake_item;

    std::vector<_bake_item> bake_items;


    for (auto pipeline_iterator  = m_pipelines.begin();
              pipeline_iterator != m_pipelines.end();
            ++pipeline_iterator)
    {
        if ( pipeline_iterator->second->dirty       &&
            !pipeline_iterator->second->is_proxy    &&
             pipeline_iterator->second->is_bakeable)
        {
            if (pipeline_iterator->second->layout_dirty)
            {
                if (pipeline_iterator->second->layout_ptr != nullptr)
                {
                    pipeline_iterator->second->layout_ptr->release();
                }

                pipeline_iterator->second->layout_ptr = get_pipeline_layout(pipeline_iterator->first);
            }

            bake_items.push_back(_bake_item(pipeline_iterator->first,
                                            pipeline_iterator->second) );
        }
    }

    color_blend_attachment_states_vk_cache.reserve         (N_CACHE_ITEMS);
    color_blend_state_create_info_items_vk_cache.reserve   (N_CACHE_ITEMS);
    depth_stencil_state_create_info_items_vk_cache.reserve (N_CACHE_ITEMS);
    dynamic_state_create_info_items_vk_cache.reserve       (N_CACHE_ITEMS);
    enabled_dynamic_states_vk_cache.reserve                (N_CACHE_ITEMS);
    graphics_pipeline_create_info_items_vk_cache.reserve   (N_CACHE_ITEMS);
    input_assembly_state_create_info_items_vk_cache.reserve(N_CACHE_ITEMS);
    multisample_state_create_info_items_vk_cache.reserve   (N_CACHE_ITEMS);
    raster_state_create_info_items_vk_cache.reserve        (N_CACHE_ITEMS);
    scissor_boxes_vk_cache.reserve                         (N_CACHE_ITEMS);
    shader_stage_create_info_items_vk_cache.reserve        (N_CACHE_ITEMS);
    specialization_info_vk_cache.reserve                   (N_CACHE_ITEMS);
    specialization_map_entry_vk_cache.reserve              (N_CACHE_ITEMS);
    tessellation_state_create_info_items_vk_cache.reserve  (N_CACHE_ITEMS);
    vertex_input_state_create_info_items_vk_cache.reserve  (N_CACHE_ITEMS);
    viewport_state_create_info_items_vk_cache.reserve      (N_CACHE_ITEMS);
    viewports_vk_cache.reserve                             (N_CACHE_ITEMS);

    for (auto bake_item_iterator  = bake_items.begin();
              bake_item_iterator != bake_items.end();
            ++bake_item_iterator)
    {
        bool                                           color_blend_state_used          = false;
        std::shared_ptr<GraphicsPipelineConfiguration> current_pipeline_config_ptr     = nullptr;
        const GraphicsPipelineID                       current_pipeline_id             = bake_item_iterator->pipeline_id;
        std::shared_ptr<Pipeline>                      current_pipeline_ptr            = bake_item_iterator->pipeline_ptr;
        bool                                           depth_stencil_state_used        = false;
        bool                                           dynamic_state_used              = false;
        VkGraphicsPipelineCreateInfo                   graphics_pipeline_create_info;
        VkPipelineInputAssemblyStateCreateInfo         input_assembly_state_create_info;
        bool                                           multisample_state_used          = false;
        VkPipelineRasterizationStateCreateInfo         raster_state_create_info;
        uint32_t                                       shader_stage_start_offset       = -1;
        uint32_t                                       subpass_n_color_attachments     = 0;
        bool                                           tessellation_state_used         = false;
        VkPipelineVertexInputStateCreateInfo           vertex_input_state_create_info;
        bool                                           viewport_state_used             = false;

        if (!current_pipeline_ptr->dirty)
        {
            continue;
        }

        if (m_pipeline_configurations.find(current_pipeline_id) == m_pipeline_configurations.end() )
        {
            anvil_assert(false);

            goto end;
        }
        else
        {
            current_pipeline_config_ptr = m_pipeline_configurations[current_pipeline_id];
        }

        /* Extract subpass information */
        current_pipeline_config_ptr->renderpass_ptr->get_subpass_n_color_attachments(current_pipeline_config_ptr->subpass_id,
                                                                                    &subpass_n_color_attachments);

        /* Form the color blend state create info descriptor, if needed */
        if (!current_pipeline_config_ptr->rasterizer_discard_enabled &&
             subpass_n_color_attachments > 0)
        {
            VkPipelineColorBlendStateCreateInfo color_blend_state_create_info;
            const uint32_t                      start_offset = (uint32_t) color_blend_attachment_states_vk_cache.size();

            color_blend_state_create_info.attachmentCount       = subpass_n_color_attachments;
            color_blend_state_create_info.flags                 = 0;
            color_blend_state_create_info.logicOp               = current_pipeline_config_ptr->logic_op;
            color_blend_state_create_info.logicOpEnable         = current_pipeline_config_ptr->logic_op_enabled;
            color_blend_state_create_info.pAttachments          = nullptr;
            color_blend_state_create_info.pNext                 = nullptr;
            color_blend_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

            memcpy(color_blend_state_create_info.blendConstants,
                   current_pipeline_config_ptr->blend_constant,
                   sizeof(color_blend_state_create_info.blendConstants) );

            for (uint32_t n_subpass_color_attachment = 0;
                          n_subpass_color_attachment < subpass_n_color_attachments;
                        ++n_subpass_color_attachment)
            {
                VkPipelineColorBlendAttachmentState blend_attachment_state;
                BlendingProperties*                 current_attachment_blending_props_ptr = nullptr;

                if (current_pipeline_config_ptr->subpass_attachment_blending_properties.find(n_subpass_color_attachment) == current_pipeline_config_ptr->subpass_attachment_blending_properties.end() )
                {
                    /* The user has not defined blending properties for current color attachment. Use default state values .. */
                    current_attachment_blending_props_ptr = &current_pipeline_config_ptr->subpass_attachment_blending_properties[n_subpass_color_attachment];

                    current_attachment_blending_props_ptr->blend_enabled          = false;
                    current_attachment_blending_props_ptr->blend_op_alpha         = VK_BLEND_OP_ADD;
                    current_attachment_blending_props_ptr->blend_op_color         = VK_BLEND_OP_ADD;
                    current_attachment_blending_props_ptr->channel_write_mask     = VK_COLOR_COMPONENT_A_BIT |
                                                                                    VK_COLOR_COMPONENT_B_BIT |
                                                                                    VK_COLOR_COMPONENT_G_BIT |
                                                                                    VK_COLOR_COMPONENT_R_BIT;
                    current_attachment_blending_props_ptr->dst_alpha_blend_factor = VK_BLEND_FACTOR_ONE;
                    current_attachment_blending_props_ptr->dst_color_blend_factor = VK_BLEND_FACTOR_ONE;
                    current_attachment_blending_props_ptr->src_alpha_blend_factor = VK_BLEND_FACTOR_ONE;
                    current_attachment_blending_props_ptr->src_color_blend_factor = VK_BLEND_FACTOR_ONE;
                }
                else
                {
                    current_attachment_blending_props_ptr = &current_pipeline_config_ptr->subpass_attachment_blending_properties[n_subpass_color_attachment];
                }

                #ifdef _DEBUG
                {
                    if (n_subpass_color_attachment > 0)
                    {
                        const BlendingProperties& zero_attachment_blending_props = current_pipeline_config_ptr->subpass_attachment_blending_properties[0];

                        anvil_assert(zero_attachment_blending_props == *current_attachment_blending_props_ptr);
                    }
                }
                #endif /* _DEBUG */

                /* Convert internal descriptor to Vulkan descriptor & stash it in the cache vector. */
                blend_attachment_state = current_attachment_blending_props_ptr->get_vk_descriptor();

                color_blend_attachment_states_vk_cache.push_back(blend_attachment_state);
            }

            color_blend_state_create_info.pAttachments = (subpass_n_color_attachments > 0) ? &color_blend_attachment_states_vk_cache[start_offset]
                                                                                           : nullptr;
            color_blend_state_used                     = true;

            color_blend_state_create_info_items_vk_cache.push_back(color_blend_state_create_info);
        }
        else
        {
            /* No color attachments available. Make sure none of the dependent modes are enabled. */
            anvil_assert(!current_pipeline_config_ptr->logic_op_enabled);

            color_blend_state_used = false;
        }

        /* Form the depth stencil state create info descriptor, if needed */
        bool is_depth_stencil_attachment_defined = false;

        current_pipeline_config_ptr->renderpass_ptr->is_depth_stencil_attachment_defined_for_subpass(current_pipeline_config_ptr->subpass_id,
                                                                                                    &is_depth_stencil_attachment_defined);

        if (is_depth_stencil_attachment_defined)
        {
            VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info;

            depth_stencil_state_create_info.back                  = current_pipeline_config_ptr->stencil_state_back_face;
            depth_stencil_state_create_info.depthBoundsTestEnable = current_pipeline_config_ptr->depth_bounds_test_enabled;
            depth_stencil_state_create_info.depthCompareOp        = current_pipeline_config_ptr->depth_test_compare_op;
            depth_stencil_state_create_info.depthTestEnable       = current_pipeline_config_ptr->depth_test_enabled;
            depth_stencil_state_create_info.depthWriteEnable      = current_pipeline_config_ptr->depth_writes_enabled;
            depth_stencil_state_create_info.flags                 = 0;
            depth_stencil_state_create_info.front                 = current_pipeline_config_ptr->stencil_state_front_face;
            depth_stencil_state_create_info.maxDepthBounds        = current_pipeline_config_ptr->max_depth_bounds;
            depth_stencil_state_create_info.minDepthBounds        = current_pipeline_config_ptr->min_depth_bounds;
            depth_stencil_state_create_info.pNext                 = nullptr;
            depth_stencil_state_create_info.stencilTestEnable     = current_pipeline_config_ptr->stencil_test_enabled;
            depth_stencil_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

            depth_stencil_state_used = true;

            depth_stencil_state_create_info_items_vk_cache.push_back(depth_stencil_state_create_info);
        }
        else
        {
            /* No depth/stencil attachment available. Make sure none of the dependent modes are enabled. */
            anvil_assert(!current_pipeline_config_ptr->depth_bounds_test_enabled);
            anvil_assert(!current_pipeline_config_ptr->depth_test_enabled);
            anvil_assert(!current_pipeline_config_ptr->depth_writes_enabled);
            anvil_assert(!current_pipeline_config_ptr->stencil_test_enabled);

            depth_stencil_state_used = false;
        }

        /* Form the dynamic state create info descriptor, if needed */
        if (current_pipeline_config_ptr->enabled_dynamic_states != 0)
        {
            VkPipelineDynamicStateCreateInfo dynamic_state_create_info;
            const uint32_t                   start_offset = (uint32_t) enabled_dynamic_states_vk_cache.size();

            if ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_BLEND_CONSTANTS_BIT) != 0)
            {
                enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
            }

            if ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_DEPTH_BIAS_BIT) != 0)
            {
                enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
            }

            if ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_DEPTH_BOUNDS_BIT) != 0)
            {
                enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
            }

            if ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_LINE_WIDTH_BIT) != 0)
            {
                enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
            }

            if ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_SCISSOR_BIT) != 0)
            {
                enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_SCISSOR);
            }

            if ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_STENCIL_COMPARE_MASK_BIT) != 0)
            {
                enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
            }

            if ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_STENCIL_REFERENCE_BIT) != 0)
            {
                enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
            }

            if ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_STENCIL_WRITE_MASK_BIT) != 0)
            {
                enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
            }

            if ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_VIEWPORT_BIT) != 0)
            {
                enabled_dynamic_states_vk_cache.push_back(VK_DYNAMIC_STATE_VIEWPORT);
            }

            dynamic_state_create_info.dynamicStateCount = (uint32_t) (enabled_dynamic_states_vk_cache.size() - start_offset);
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

        /* Form the input assembly create info descriptor */
        input_assembly_state_create_info.flags                  = 0;
        input_assembly_state_create_info.pNext                  = nullptr;
        input_assembly_state_create_info.primitiveRestartEnable = current_pipeline_config_ptr->primitive_restart_enabled;
        input_assembly_state_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.topology               = current_pipeline_config_ptr->primitive_topology;

        input_assembly_state_create_info_items_vk_cache.push_back(input_assembly_state_create_info);

        /* Form the multisample state create info descriptor, if needed */
        if (!current_pipeline_config_ptr->rasterizer_discard_enabled)
        {
            VkPipelineMultisampleStateCreateInfo multisample_state_create_info;

            multisample_state_create_info.alphaToCoverageEnable = current_pipeline_config_ptr->alpha_to_coverage_enabled;
            multisample_state_create_info.alphaToOneEnable      = current_pipeline_config_ptr->alpha_to_one_enabled;
            multisample_state_create_info.flags                 = 0;
            multisample_state_create_info.minSampleShading      = current_pipeline_config_ptr->min_sample_shading;
            multisample_state_create_info.pNext                 = nullptr;
            multisample_state_create_info.pSampleMask           = &current_pipeline_config_ptr->sample_mask;
            multisample_state_create_info.rasterizationSamples  = current_pipeline_config_ptr->sample_count;
            multisample_state_create_info.sampleShadingEnable   = current_pipeline_config_ptr->sample_shading_enabled;
            multisample_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

            multisample_state_used = true;

            multisample_state_create_info_items_vk_cache.push_back(multisample_state_create_info);
        }
        else
        {
            /* Make sure any dependent modes are disabled */
            anvil_assert(!current_pipeline_config_ptr->alpha_to_coverage_enabled);
            anvil_assert(!current_pipeline_config_ptr->alpha_to_one_enabled);
            anvil_assert(!current_pipeline_config_ptr->sample_shading_enabled);

            multisample_state_used = false;
        }

        /* Form the raster state create info descriptor */
        raster_state_create_info.cullMode                = current_pipeline_config_ptr->cull_mode;
        raster_state_create_info.depthBiasClamp          = current_pipeline_config_ptr->depth_bias_clamp;
        raster_state_create_info.depthBiasConstantFactor = current_pipeline_config_ptr->depth_bias_constant_factor;
        raster_state_create_info.depthBiasEnable         = current_pipeline_config_ptr->depth_bias_enabled;
        raster_state_create_info.depthBiasSlopeFactor    = current_pipeline_config_ptr->depth_bias_slope_factor;
        raster_state_create_info.depthClampEnable        = current_pipeline_config_ptr->depth_clamp_enabled;
        raster_state_create_info.flags                   = 0;
        raster_state_create_info.frontFace               = current_pipeline_config_ptr->front_face;
        raster_state_create_info.lineWidth               = current_pipeline_config_ptr->line_width;
        raster_state_create_info.pNext                   = nullptr;
        raster_state_create_info.polygonMode             = current_pipeline_config_ptr->polygon_mode;
        raster_state_create_info.rasterizerDiscardEnable = current_pipeline_config_ptr->rasterizer_discard_enabled;
        raster_state_create_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        if (m_device_ptr->is_extension_enabled("VK_AMD_rasterization_order") )
        {
            /* Chain a predefined struct which will toggle the relaxed rasterization, as long as the device supports the
             * VK_AMD_rasterization_order extension.
             */
            const Anvil::GraphicsPipelineManager::VkPipelineRasterizationStateRasterizationOrderAMD* chained_item_ptr = nullptr;

            if (current_pipeline_config_ptr->rasterization_order != VK_RASTERIZATION_ORDER_STRICT_AMD)
            {
                anvil_assert(current_pipeline_config_ptr->rasterization_order == relaxed_rasterization_order_item.rasterizationOrder);

                chained_item_ptr = &relaxed_rasterization_order_item;
            }
            else
            {
                chained_item_ptr = &strict_rasterization_order_item;
            }

            raster_state_create_info.pNext = chained_item_ptr;
        }
        else
        if (current_pipeline_config_ptr->rasterization_order != VK_RASTERIZATION_ORDER_STRICT_AMD)
        {
            fprintf(stderr,
                    "[!] Cannot enable out-of-order rasterization - VK_AMD_rasterization_order extension not enabled at device creation time");
        }

        raster_state_create_info_items_vk_cache.push_back(raster_state_create_info);

        /* Form stage descriptors */
        shader_stage_start_offset = (uint32_t) shader_stage_create_info_items_vk_cache.size();

        for (uint32_t n_shader = 0;
                      n_shader < GRAPHICS_SHADER_STAGE_COUNT;
                    ++n_shader)
        {
            if (current_pipeline_ptr->shader_stages[n_shader].name != nullptr)
            {
                VkPipelineShaderStageCreateInfo current_shader_stage_create_info;

                if (current_pipeline_ptr->specialization_constants_map[n_shader].size() > 0)
                {
                    bake_specialization_info_vk(current_pipeline_ptr->specialization_constants_map       [n_shader],
                                               &current_pipeline_ptr->specialization_constant_data_buffer[0],
                                               &specialization_map_entry_vk_cache,
                                               &specialization_info_vk_cache[n_shader]);
                }

                Anvil::ShaderModule* shader_module_ptr = current_pipeline_ptr->shader_stages[n_shader].shader_module_ptr;

                current_shader_stage_create_info.module = shader_module_ptr->get_module();
                current_shader_stage_create_info.pName  = (n_shader == GRAPHICS_SHADER_STAGE_FRAGMENT)                ? shader_module_ptr->get_fs_entrypoint_name()
                                                        : (n_shader == GRAPHICS_SHADER_STAGE_GEOMETRY)                ? shader_module_ptr->get_gs_entrypoint_name()
                                                        : (n_shader == GRAPHICS_SHADER_STAGE_TESSELLATION_CONTROL)    ? shader_module_ptr->get_tc_entrypoint_name()
                                                        : (n_shader == GRAPHICS_SHADER_STAGE_TESSELLATION_EVALUATION) ? shader_module_ptr->get_te_entrypoint_name()
                                                        : (n_shader == GRAPHICS_SHADER_STAGE_VERTEX)                  ? shader_module_ptr->get_vs_entrypoint_name()
                                                        : nullptr;

                current_shader_stage_create_info.flags               = 0;
                current_shader_stage_create_info.pNext               = nullptr;
                current_shader_stage_create_info.pSpecializationInfo = (current_pipeline_ptr->specialization_constants_map[n_shader].size() > 0) ? &specialization_info_vk_cache[n_shader]
                                                                                                                                                 : VK_NULL_HANDLE;
                current_shader_stage_create_info.stage               = (n_shader == GRAPHICS_SHADER_STAGE_FRAGMENT)                ? VK_SHADER_STAGE_FRAGMENT_BIT
                                                                     : (n_shader == GRAPHICS_SHADER_STAGE_GEOMETRY)                ? VK_SHADER_STAGE_GEOMETRY_BIT
                                                                     : (n_shader == GRAPHICS_SHADER_STAGE_TESSELLATION_CONTROL)    ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
                                                                     : (n_shader == GRAPHICS_SHADER_STAGE_TESSELLATION_EVALUATION) ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
                                                                                                                                   : VK_SHADER_STAGE_VERTEX_BIT;
                current_shader_stage_create_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

                shader_stage_create_info_items_vk_cache.push_back(current_shader_stage_create_info);
            }
        }

        /* Form the tessellation state create info descriptor if needed */
        if (current_pipeline_ptr->shader_stages[GRAPHICS_SHADER_STAGE_TESSELLATION_CONTROL].name    != nullptr ||
            current_pipeline_ptr->shader_stages[GRAPHICS_SHADER_STAGE_TESSELLATION_EVALUATION].name != nullptr)
        {
            VkPipelineTessellationStateCreateInfo tessellation_state_create_info;

            tessellation_state_create_info.flags              = 0;
            tessellation_state_create_info.patchControlPoints = current_pipeline_config_ptr->n_patch_control_points;
            tessellation_state_create_info.pNext              = nullptr;
            tessellation_state_create_info.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

            tessellation_state_used = true;

            tessellation_state_create_info_items_vk_cache.push_back(tessellation_state_create_info);
        }
        else
        {
            tessellation_state_used = false;
        }

        /* Form the vertex input state create info descriptor */
        bake_vk_attributes_and_bindings(current_pipeline_config_ptr);

        vertex_input_state_create_info.vertexAttributeDescriptionCount = (uint32_t) current_pipeline_config_ptr->vk_input_attributes.size();
        vertex_input_state_create_info.vertexBindingDescriptionCount   = (uint32_t) current_pipeline_config_ptr->vk_input_bindings.size();

        vertex_input_state_create_info.flags                        = 0;
        vertex_input_state_create_info.pNext                        = nullptr;
        vertex_input_state_create_info.pVertexAttributeDescriptions = (vertex_input_state_create_info.vertexAttributeDescriptionCount > 0) ? &current_pipeline_config_ptr->vk_input_attributes[0]
                                                                                                                                           : nullptr;
        vertex_input_state_create_info.pVertexBindingDescriptions   = (vertex_input_state_create_info.vertexBindingDescriptionCount   > 0) ? &current_pipeline_config_ptr->vk_input_bindings[0]
                                                                                                                                           : nullptr;
        vertex_input_state_create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        vertex_input_state_create_info_items_vk_cache.push_back(vertex_input_state_create_info);

        /* Form the viewport state create info descriptor, if needed */
        if (!current_pipeline_config_ptr->rasterizer_discard_enabled)
        {
            const uint32_t                    scissor_boxes_start_offset = (uint32_t) scissor_boxes_vk_cache.size();
            const uint32_t                    viewports_start_offset     = (uint32_t) viewports_vk_cache.size();
            VkPipelineViewportStateCreateInfo viewport_state_create_info;

            #ifdef _DEBUG
            {
                const uint32_t n_scissor_boxes = (current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_SCISSOR_BIT)  ? current_pipeline_config_ptr->n_dynamic_scissor_boxes
                                                                                                                                    : (uint32_t) current_pipeline_config_ptr->scissor_boxes.size();
                const uint32_t n_viewports     = (current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_VIEWPORT_BIT) ? current_pipeline_config_ptr->n_dynamic_viewports
                                                                                                                                    : (uint32_t) current_pipeline_config_ptr->viewports.size();

                anvil_assert(n_scissor_boxes == n_viewports);
            }
            #endif /* _DEBUG */

            if (current_pipeline_config_ptr->scissor_boxes.size() == 0)
            {
                /* No scissor boxes / viewport defined. Use default settings.. */
                InternalScissorBox default_scissor_box;
                InternalViewport   default_viewport;
                const Swapchain*   swapchain_ptr  = current_pipeline_config_ptr->renderpass_ptr->get_swapchain();
                uint32_t           window_size[2] = {0};

                /* NOTE: If you hit this assertion, you either need to pass a Swapchain instance when this renderpass is being created,
                 *       *or* specify scissor & viewport information for the GFX pipeline.
                 */
                anvil_assert(swapchain_ptr != nullptr);

                swapchain_ptr->get_image(0)->get_image_mipmap_size(0,              /* n_mipmap */
                                                                   window_size + 0,
                                                                   window_size + 1,
                                                                   nullptr);       /* opt_out_depth_ptr */

                anvil_assert(window_size[0] != 0 &&
                             window_size[1] != 0);

                default_scissor_box.height = window_size[1];
                default_scissor_box.width  = window_size[0];
                default_scissor_box.x      = 0;
                default_scissor_box.y      = 0;

                default_viewport.height   = float(window_size[1]);
                default_viewport.max_depth = 1.0f;
                default_viewport.min_depth = 0.0f;
                default_viewport.origin_x  = 0.0f;
                default_viewport.origin_y  = 0.0f;
                default_viewport.width    = float(window_size[0]);

                current_pipeline_config_ptr->scissor_boxes[0] = default_scissor_box;
                current_pipeline_config_ptr->viewports    [0] = default_viewport;
            }

            /* Convert internal scissor box & viewport representations to Vulkan descriptors */
            for (auto scissor_box_iterator  = current_pipeline_config_ptr->scissor_boxes.cbegin();
                      scissor_box_iterator != current_pipeline_config_ptr->scissor_boxes.cend();
                    ++scissor_box_iterator)
            {
                scissor_boxes_vk_cache.push_back(scissor_box_iterator->second.get_vk_descriptor() );
            }

            for (auto viewport_iterator  = current_pipeline_config_ptr->viewports.cbegin();
                      viewport_iterator != current_pipeline_config_ptr->viewports.cend();
                    ++viewport_iterator)
            {
                viewports_vk_cache.push_back(viewport_iterator->second.get_vk_descriptor() );
            }

            /* Bake the descriptor */
            viewport_state_create_info.flags         = 0;
            viewport_state_create_info.pNext         = nullptr;
            viewport_state_create_info.pScissors     = ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_SCISSOR_BIT)  != 0) ? VK_NULL_HANDLE
                                                                                                                                                 : &scissor_boxes_vk_cache[scissor_boxes_start_offset];
            viewport_state_create_info.pViewports    = ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_VIEWPORT_BIT) != 0) ? VK_NULL_HANDLE
                                                                                                                                                 : &viewports_vk_cache    [viewports_start_offset];
            viewport_state_create_info.scissorCount  = ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_SCISSOR_BIT)  != 0) ? current_pipeline_config_ptr->n_dynamic_scissor_boxes 
                                                                                                                                                 : (uint32_t) current_pipeline_config_ptr->scissor_boxes.size();
            viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_create_info.viewportCount = ((current_pipeline_config_ptr->enabled_dynamic_states & DYNAMIC_STATE_VIEWPORT_BIT) != 0) ? current_pipeline_config_ptr->n_dynamic_viewports
                                                                                                                                                 : (uint32_t) current_pipeline_config_ptr->viewports.size();

            anvil_assert(viewport_state_create_info.scissorCount == viewport_state_create_info.viewportCount);

            viewport_state_used = true;

            viewport_state_create_info_items_vk_cache.push_back(viewport_state_create_info);
        }
        else
        {
            viewport_state_used = false;
        }

        /* Configure base pipeline handle/indices fields of the create info descriptor */
        if (current_pipeline_ptr->base_pipeline_ptr != nullptr)
        {
            anvil_assert(current_pipeline_ptr->base_pipeline == VK_NULL_HANDLE);

            /* There are three cases we need to handle separately here:
             *
             * 1. The base pipeline is to be baked in the call we're preparing for. Determine
             *    its index in the create info descriptor array and use it.
             * 2. The pipeline has been baked earlier. We should be able to work around this
             *    by providing a handle to the pipeline, instead of the index.
             * 3. The pipeline under specified index uses a different layout. This indicates
             *    a bug in the app or the manager.
             *
             * NOTE: A slightly adjusted version of this code is re-used in ComputePipelineManager::bake()
             */
            auto base_bake_item_iterator = std::find(bake_items.begin(),
                                                     bake_items.end(),
                                                     current_pipeline_ptr->base_pipeline_ptr);

            if (base_bake_item_iterator != bake_items.end() )
            {
                /* Case 1 */
                graphics_pipeline_create_info.basePipelineHandle = current_pipeline_ptr->base_pipeline;
                graphics_pipeline_create_info.basePipelineIndex  = (uint32_t) (base_bake_item_iterator - bake_items.begin() );
            }
            else
            if (current_pipeline_ptr->base_pipeline_ptr                 != nullptr            &&
                current_pipeline_ptr->base_pipeline_ptr->baked_pipeline != VK_NULL_HANDLE)
            {
                /* Case 2 */
                graphics_pipeline_create_info.basePipelineHandle = current_pipeline_ptr->base_pipeline_ptr->baked_pipeline;
                graphics_pipeline_create_info.basePipelineIndex  = -1; /* unused. */
            }
            else
            {
                /* Case 3 */
                anvil_assert(false);
            }
        }
        else
        if (current_pipeline_ptr->base_pipeline != VK_NULL_HANDLE)
        {
            graphics_pipeline_create_info.basePipelineHandle = current_pipeline_ptr->base_pipeline;
            graphics_pipeline_create_info.basePipelineIndex  = -1; /* unused. */
        }
        else
        {
            /* No base pipeline requested */
            graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            graphics_pipeline_create_info.basePipelineIndex  = -1; /* unused */
        }

        /* Form the rest of the create info descriptor */
        anvil_assert(!current_pipeline_ptr->layout_dirty);

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
        graphics_pipeline_create_info.pRasterizationState = &raster_state_create_info_items_vk_cache[raster_state_create_info_items_vk_cache.size() - 1];
        graphics_pipeline_create_info.pStages             = &shader_stage_create_info_items_vk_cache[shader_stage_start_offset];
        graphics_pipeline_create_info.pTessellationState  = (tessellation_state_used)   ? &tessellation_state_create_info_items_vk_cache[tessellation_state_create_info_items_vk_cache.size() - 1]
                                                                                        : VK_NULL_HANDLE;
        graphics_pipeline_create_info.pVertexInputState   = &vertex_input_state_create_info_items_vk_cache[vertex_input_state_create_info_items_vk_cache.size() - 1];
        graphics_pipeline_create_info.pViewportState      = (viewport_state_used)       ? &viewport_state_create_info_items_vk_cache[viewport_state_create_info_items_vk_cache.size() - 1]
                                                                                        : VK_NULL_HANDLE;
        graphics_pipeline_create_info.renderPass          = current_pipeline_config_ptr->renderpass_ptr->get_render_pass();
        graphics_pipeline_create_info.stageCount          = (uint32_t) shader_stage_create_info_items_vk_cache.size() - shader_stage_start_offset;
        graphics_pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphics_pipeline_create_info.subpass             = current_pipeline_config_ptr->subpass_id;

        if (graphics_pipeline_create_info.basePipelineHandle != VK_NULL_HANDLE ||
            graphics_pipeline_create_info.basePipelineIndex != -1)
        {
            graphics_pipeline_create_info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        }

        graphics_pipeline_create_info.flags |= ((current_pipeline_ptr->allow_derivatives)     ? VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT    : 0) |
                                               ((current_pipeline_ptr->disable_optimizations) ? VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT : 0);

        if (current_pipeline_config_ptr->pfn_pipeline_prebake_callback_proc != nullptr)
        {
            current_pipeline_config_ptr->pfn_pipeline_prebake_callback_proc(m_device_ptr,
                                                                            current_pipeline_id,
                                                                           &graphics_pipeline_create_info,
                                                                            current_pipeline_config_ptr->pipeline_prebake_callback_user_arg);
        }

        /* Stash the descriptor for now. We will issue one expensive vkCreateGraphicsPipelines() call after all pipeline objects
         * are iterated over. */
        graphics_pipeline_create_info_items_vk_cache.push_back(graphics_pipeline_create_info);
    }

    #ifdef _DEBUG
    {
        /* If you hit any of the below assertion failures, please bump up the value under the #define.
         *
         * We use the vectors to hold structure instances. If more than N_CACHE_ITEMS items is ever requested from any of these vectors,
         * the underlying storage will likely be reallocated. Many of the descriptors use pointers to refer to children descriptors,
         * and when the data is moved around, the pointers are no longer valid.
         */
        anvil_assert(color_blend_attachment_states_vk_cache.size()          <= N_CACHE_ITEMS);
        anvil_assert(color_blend_state_create_info_items_vk_cache.size()    <= N_CACHE_ITEMS);
        anvil_assert(depth_stencil_state_create_info_items_vk_cache.size()  <= N_CACHE_ITEMS);
        anvil_assert(dynamic_state_create_info_items_vk_cache.size()        <= N_CACHE_ITEMS);
        anvil_assert(enabled_dynamic_states_vk_cache.size()                 <= N_CACHE_ITEMS);
        anvil_assert(graphics_pipeline_create_info_items_vk_cache.size()    <= N_CACHE_ITEMS);
        anvil_assert(input_assembly_state_create_info_items_vk_cache.size() <= N_CACHE_ITEMS);
        anvil_assert(multisample_state_create_info_items_vk_cache.size()    <= N_CACHE_ITEMS);
        anvil_assert(raster_state_create_info_items_vk_cache.size()         <= N_CACHE_ITEMS);
        anvil_assert(scissor_boxes_vk_cache.size()                          <= N_CACHE_ITEMS);
        anvil_assert(shader_stage_create_info_items_vk_cache.size()         <= N_CACHE_ITEMS);
        anvil_assert(specialization_info_vk_cache.size()                    <= N_CACHE_ITEMS);
        anvil_assert(specialization_map_entry_vk_cache.size()               <= N_CACHE_ITEMS);
        anvil_assert(tessellation_state_create_info_items_vk_cache.size()   <= N_CACHE_ITEMS);
        anvil_assert(vertex_input_state_create_info_items_vk_cache.size()   <= N_CACHE_ITEMS);
        anvil_assert(viewport_state_create_info_items_vk_cache.size()       <= N_CACHE_ITEMS);
        anvil_assert(viewports_vk_cache.size()                              <= N_CACHE_ITEMS);
    }
    #endif

    /* All right. Try to bake all pipeline objects at once */
    result_graphics_pipelines.resize(bake_items.size() );

    result_vk = vkCreateGraphicsPipelines(m_device_ptr->get_device_vk(),
                                          m_pipeline_cache_ptr->get_pipeline_cache(),
                                          (uint32_t) graphics_pipeline_create_info_items_vk_cache.size(),
                                         &graphics_pipeline_create_info_items_vk_cache[0],
                                          nullptr, /* pAllocator */
                                         &result_graphics_pipelines[0]);

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert(result_vk);

        goto end;
    }

    for (auto bake_item_iterator  = bake_items.begin();
              bake_item_iterator != bake_items.end();
            ++bake_item_iterator)
    {
        if (!bake_item_iterator->pipeline_ptr->dirty)
        {
            continue;
        }

        if (m_pipeline_configurations[bake_item_iterator->pipeline_id]->pfn_pipeline_postbake_callback_proc != nullptr)
        {
            m_pipeline_configurations[bake_item_iterator->pipeline_id]->pfn_pipeline_postbake_callback_proc(m_device_ptr,
                                                                                                            bake_item_iterator->pipeline_id,
                                                                                                            m_pipeline_configurations[bake_item_iterator->pipeline_id]->pipeline_postbake_callback_user_arg);
        }
    }

    /* Distribute the result pipeline objects to pipeline configuration descriptors */
    for (auto pipeline_iterator  = m_pipelines.begin();
              pipeline_iterator != m_pipelines.end();
            ++pipeline_iterator)
    {
        const GraphicsPipelineID  current_pipeline_id  = pipeline_iterator->first;
        std::shared_ptr<Pipeline> current_pipeline_ptr = pipeline_iterator->second;

        if (!current_pipeline_ptr->dirty       ||
             current_pipeline_ptr->is_proxy    ||
            !current_pipeline_ptr->is_bakeable)
        {
            continue;
        }

        m_pipelines[current_pipeline_id]->baked_pipeline = result_graphics_pipelines[n_consumed_graphics_pipelines++];
        current_pipeline_ptr->dirty                      = false;
    }

    anvil_assert(n_consumed_graphics_pipelines == (uint32_t) result_graphics_pipelines.size() );

    /* All done */
    result = true;
end:
    return result;
}

/** Converts the internal vertex attribute descriptors to one or more VkVertexInputAttributeDescription & VkVertexInputBindingDescription
 *  structures.
 *
 *  The function goes an extra mile to re-use the same vertex binding for attributes whose binding properties match.
 *
 *  @param pipeline_config_ptr Pipeline configuration descriptor to use. Must not be nullptr.
 */
void Anvil::GraphicsPipelineManager::bake_vk_attributes_and_bindings(std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr)
{
    pipeline_config_ptr->vk_input_attributes.clear();
    pipeline_config_ptr->vk_input_bindings.clear();

    for (auto attribute_iterator  = pipeline_config_ptr->attributes.cbegin();
              attribute_iterator != pipeline_config_ptr->attributes.cend();
            ++attribute_iterator)
    {
        /* Identify the binding index we should use for the attribute */
        const InternalVertexAttribute&    current_attribute   = *attribute_iterator;
        VkVertexInputAttributeDescription current_attribute_vk;
        uint32_t                          n_attribute_binding = -1;
        bool                              has_found           = false;

        for (auto vk_input_binding_iterator  = pipeline_config_ptr->vk_input_bindings.begin();
                  vk_input_binding_iterator != pipeline_config_ptr->vk_input_bindings.end();
                ++vk_input_binding_iterator)
        {
            if (vk_input_binding_iterator->inputRate == attribute_iterator->rate           &&
                vk_input_binding_iterator->stride    == attribute_iterator->stride_in_bytes)
            {
                has_found           = true;
                n_attribute_binding = (uint32_t) (vk_input_binding_iterator - pipeline_config_ptr->vk_input_bindings.begin());

                break;
            }
        }

        if (!has_found)
        {
            /* Got to create a new binding descriptor .. */
            VkVertexInputBindingDescription new_binding_vk;

            new_binding_vk.binding   = (uint32_t) pipeline_config_ptr->vk_input_bindings.size();
            new_binding_vk.inputRate = attribute_iterator->rate;
            new_binding_vk.stride    = attribute_iterator->stride_in_bytes;

            pipeline_config_ptr->vk_input_bindings.push_back(new_binding_vk);

            n_attribute_binding = new_binding_vk.binding;
        }

        /* Good to convert the internal attribute descriptor to the Vulkan's input attribute descriptor */
        current_attribute_vk.binding  = n_attribute_binding;
        current_attribute_vk.format   = attribute_iterator->format;
        current_attribute_vk.location = attribute_iterator->location;
        current_attribute_vk.offset   = attribute_iterator->offset_in_bytes;

        pipeline_config_ptr->vk_input_attributes.push_back(current_attribute_vk);
    }
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::delete_pipeline(GraphicsPipelineID graphics_pipeline_id)
{
    GraphicsPipelineConfigurations::iterator configuration_iterator;
    bool                                     result = false;

    result = BasePipelineManager::delete_pipeline(graphics_pipeline_id);

    if (!result)
    {
        goto end;
    }

    configuration_iterator = m_pipeline_configurations.find(graphics_pipeline_id);

    if (configuration_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(configuration_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        m_pipeline_configurations.erase(configuration_iterator);
    }

    result = true;
end:
    return result;
}

/* Please see header for specification */
VkPipeline Anvil::GraphicsPipelineManager::get_graphics_pipeline(GraphicsPipelineID pipeline_id)
{
    auto       pipeline_iterator = m_pipelines.find(pipeline_id);
    bool       result;
    VkPipeline result_pipeline = VK_NULL_HANDLE;

    if (pipeline_iterator == m_pipelines.end() )
    {
        anvil_assert(!(pipeline_iterator == m_pipelines.end()) );

        goto end;
    }

    if (pipeline_iterator->second->baked_pipeline == VK_NULL_HANDLE ||
        pipeline_iterator->second->dirty)
    {
        anvil_assert(!pipeline_iterator->second->is_proxy);

        result = bake();

        anvil_assert(result);
        anvil_assert(pipeline_iterator->second->dirty          == false);
        anvil_assert(pipeline_iterator->second->baked_pipeline != VK_NULL_HANDLE);
    }

    result_pipeline = pipeline_iterator->second->baked_pipeline;

end:
    return result_pipeline;
}

/* Please see header for specification */
Anvil::PipelineLayout* Anvil::GraphicsPipelineManager::get_graphics_pipeline_layout(GraphicsPipelineID pipeline_id)
{
    return get_pipeline_layout(pipeline_id);
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_blending_properties(GraphicsPipelineID graphics_pipeline_id,
                                                              const float*       blend_constant_vec4)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    memcpy(pipeline_config_ptr->blend_constant,
           blend_constant_vec4,
           sizeof(pipeline_config_ptr->blend_constant) );

    pipeline_iterator->second->dirty = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_color_blend_attachment_properties(GraphicsPipelineID    graphics_pipeline_id,
                                                                           SubPassAttachmentID   attachment_id,
                                                                           bool                  blending_enabled,
                                                                           VkBlendOp             blend_op_color,
                                                                           VkBlendOp             blend_op_alpha,
                                                                           VkBlendFactor         src_color_blend_factor,
                                                                           VkBlendFactor         dst_color_blend_factor,
                                                                           VkBlendFactor         src_alpha_blend_factor,
                                                                           VkBlendFactor         dst_alpha_blend_factor,
                                                                           VkColorComponentFlags channel_write_mask)
{
    BlendingProperties*                            attachment_blending_props_ptr = nullptr;
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    /* Retrieve and update the attachment descriptor. */
    attachment_blending_props_ptr = &pipeline_config_ptr->subpass_attachment_blending_properties[attachment_id];

    attachment_blending_props_ptr->blend_enabled          = blending_enabled;
    attachment_blending_props_ptr->blend_op_alpha         = blend_op_alpha;
    attachment_blending_props_ptr->blend_op_color         = blend_op_color;
    attachment_blending_props_ptr->channel_write_mask     = channel_write_mask;
    attachment_blending_props_ptr->dst_alpha_blend_factor = dst_alpha_blend_factor;
    attachment_blending_props_ptr->dst_color_blend_factor = dst_color_blend_factor;
    attachment_blending_props_ptr->src_alpha_blend_factor = src_alpha_blend_factor;
    attachment_blending_props_ptr->src_color_blend_factor = src_color_blend_factor;
    pipeline_iterator->second->dirty                      = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_dynamic_scissor_state_properties(GraphicsPipelineID graphics_pipeline_id,
                                                                          uint32_t           n_dynamic_scissor_boxes)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->n_dynamic_scissor_boxes = n_dynamic_scissor_boxes;
    pipeline_iterator->second->dirty             = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_dynamic_viewport_state_properties(GraphicsPipelineID graphics_pipeline_id,
                                                                           uint32_t           n_dynamic_viewports)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->n_dynamic_viewports = n_dynamic_viewports;
    pipeline_iterator->second->dirty         = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_input_assembly_properties(GraphicsPipelineID  graphics_pipeline_id,
                                                                   VkPrimitiveTopology primitive_topology)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->primitive_topology = primitive_topology;
    pipeline_iterator->second->dirty        = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_multisampling_properties(GraphicsPipelineID    graphics_pipeline_id,
                                                                  VkSampleCountFlagBits sample_count,
                                                                  float                 min_sample_shading,
                                                                  const VkSampleMask    sample_mask)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->min_sample_shading = min_sample_shading;
    pipeline_config_ptr->sample_count       = sample_count;
    pipeline_config_ptr->sample_mask        = sample_mask;
    pipeline_iterator->second->dirty        = true;

end:
    ;
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::set_pipeline_post_bake_callback(GraphicsPipelineID              graphics_pipeline_id,
                                                                     PFNPIPELINEPOSTBAKECALLBACKPROC pfn_pipeline_post_bake_proc,
                                                                     void*                           user_arg)
{
    bool result = false;

    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->pfn_pipeline_postbake_callback_proc = pfn_pipeline_post_bake_proc;
    pipeline_config_ptr->pipeline_postbake_callback_user_arg = user_arg;

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::set_pipeline_pre_bake_callback(GraphicsPipelineID             graphics_pipeline_id,
                                                                    PFNPIPELINEPREBAKECALLBACKPROC pfn_pipeline_pre_bake_proc,
                                                                    void*                          user_arg)
{
    bool result = false;

    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->pfn_pipeline_prebake_callback_proc = pfn_pipeline_pre_bake_proc;
    pipeline_config_ptr->pipeline_prebake_callback_user_arg = user_arg;

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::GraphicsPipelineManager::set_pipeline_state_from_pipeline(GraphicsPipelineID target_pipeline_id,
                                                                      GraphicsPipelineID source_pipeline_id)
{
    RenderPass*                                    cached_renderpass_ptr = nullptr;
    SubPassID                                      cached_subpass_id     = -1;
    bool                                           result                = false;
    std::shared_ptr<GraphicsPipelineConfiguration> source_pipeline_config_ptr;

    if (m_pipelines.find(target_pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(target_pipeline_id) == m_pipelines.end()) );

        goto end;
    }

    if (m_pipeline_configurations.find(target_pipeline_id) == m_pipeline_configurations.end() )
    {
        anvil_assert(!(m_pipeline_configurations.find(target_pipeline_id) == m_pipeline_configurations.end() ));

        goto end;
    }

    if (m_pipeline_configurations.find(source_pipeline_id) == m_pipeline_configurations.end() )
    {
        anvil_assert(!(m_pipeline_configurations.find(source_pipeline_id) == m_pipeline_configurations.end() ));

        goto end;
    }

    cached_renderpass_ptr = m_pipeline_configurations[target_pipeline_id]->renderpass_ptr;
    cached_subpass_id     = m_pipeline_configurations[target_pipeline_id]->subpass_id;

    m_pipeline_configurations[target_pipeline_id]                        = m_pipeline_configurations[source_pipeline_id];
    m_pipelines              [target_pipeline_id]->descriptor_set_groups = m_pipelines[source_pipeline_id]->descriptor_set_groups;
    m_pipelines              [target_pipeline_id]->dirty                 = true;

    if (m_pipeline_configurations[target_pipeline_id]->renderpass_ptr != nullptr)
    {
        m_pipeline_configurations[target_pipeline_id]->renderpass_ptr->release();
    }

    m_pipeline_configurations[target_pipeline_id]->renderpass_ptr = cached_renderpass_ptr;
    m_pipeline_configurations[target_pipeline_id]->subpass_id     = cached_subpass_id;

    cached_renderpass_ptr->retain();

    result = true;
end:
    return result;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_rasterization_properties(GraphicsPipelineID graphics_pipeline_id,
                                                                  VkPolygonMode      polygon_mode,
                                                                  VkCullModeFlags    cull_mode,
                                                                  VkFrontFace        front_face,
                                                                  float              line_width)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->cull_mode    = cull_mode;
    pipeline_config_ptr->front_face   = front_face;
    pipeline_config_ptr->line_width   = line_width;
    pipeline_config_ptr->polygon_mode = polygon_mode;

    pipeline_iterator->second->dirty = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_scissor_box_properties(GraphicsPipelineID graphics_pipeline_id,
                                                                uint32_t           n_scissor_box,
                                                                int32_t            x,
                                                                int32_t            y,
                                                                int32_t            width,
                                                                int32_t            height)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->scissor_boxes[n_scissor_box] = InternalScissorBox(x,
                                                                           y,
                                                                           width,
                                                                           height);

    pipeline_iterator->second->dirty = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_stencil_test_properties(GraphicsPipelineID graphics_pipeline_id,
                                                                 bool               update_front_face_state,
                                                                 VkStencilOp        stencil_fail_op,
                                                                 VkStencilOp        stencil_pass_op,
                                                                 VkStencilOp        stencil_depth_fail_op,
                                                                 VkCompareOp        stencil_compare_op,
                                                                 uint32_t           stencil_compare_mask,
                                                                 uint32_t           stencil_write_mask,
                                                                 uint32_t           stencil_reference)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);
    VkStencilOpState*                              stencil_op_state_ptr     = nullptr;

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()                ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    stencil_op_state_ptr = (update_front_face_state) ? &pipeline_config_ptr->stencil_state_front_face
                                                     : &pipeline_config_ptr->stencil_state_back_face;

    stencil_op_state_ptr->compareMask = stencil_compare_mask;
    stencil_op_state_ptr->compareOp   = stencil_compare_op;
    stencil_op_state_ptr->depthFailOp = stencil_depth_fail_op;
    stencil_op_state_ptr->failOp      = stencil_fail_op;
    stencil_op_state_ptr->passOp      = stencil_pass_op;
    stencil_op_state_ptr->reference   = stencil_reference;
    stencil_op_state_ptr->writeMask   = stencil_write_mask;

    pipeline_iterator->second->dirty  = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_tessellation_properties(GraphicsPipelineID graphics_pipeline_id,
                                                                 uint32_t           n_patch_control_points)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->n_patch_control_points = n_patch_control_points;
    pipeline_iterator->second->dirty            = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_viewport_properties(GraphicsPipelineID graphics_pipeline_id,
                                                             uint32_t           n_viewport,
                                                             float              origin_x,
                                                             float              origin_y,
                                                             float              width,
                                                             float              height,
                                                             float              min_depth,
                                                             float              max_depth)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->viewports[n_viewport] = InternalViewport(origin_x,
                                                                  origin_y,
                                                                  width,
                                                                  height,
                                                                  min_depth,
                                                                  max_depth);

    pipeline_iterator->second->dirty = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_alpha_to_coverage(GraphicsPipelineID graphics_pipeline_id,
                                                              bool               should_enable)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->alpha_to_coverage_enabled = should_enable;
    pipeline_iterator->second->dirty               = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_alpha_to_one(GraphicsPipelineID graphics_pipeline_id,
                                                         bool               should_enable)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->alpha_to_one_enabled = should_enable;
    pipeline_iterator->second->dirty          = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_depth_bounds_test(GraphicsPipelineID graphics_pipeline_id,
                                                              bool               should_enable,
                                                              float              min_depth_bounds,
                                                              float              max_depth_bounds)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->depth_bounds_test_enabled = should_enable;
    pipeline_config_ptr->max_depth_bounds          = max_depth_bounds;
    pipeline_config_ptr->min_depth_bounds          = min_depth_bounds;
    pipeline_iterator->second->dirty               = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_depth_bias(GraphicsPipelineID graphics_pipeline_id,
                                                       bool               should_enable,
                                                       float              depth_bias_constant_factor,
                                                       float              depth_bias_clamp,
                                                       float              depth_bias_slope_factor)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->depth_bias_constant_factor = depth_bias_constant_factor;
    pipeline_config_ptr->depth_bias_clamp           = depth_bias_clamp;
    pipeline_config_ptr->depth_bias_enabled         = should_enable;
    pipeline_config_ptr->depth_bias_slope_factor    = depth_bias_slope_factor;
    pipeline_iterator->second->dirty                = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_depth_clamp(GraphicsPipelineID graphics_pipeline_id,
                                                        bool               should_enable)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->depth_clamp_enabled = should_enable;
    pipeline_iterator->second->dirty         = true;

end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_depth_test(GraphicsPipelineID graphics_pipeline_id,
                                                       bool               should_enable,
                                                       VkCompareOp        compare_op)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->depth_test_enabled    = should_enable;
    pipeline_config_ptr->depth_test_compare_op = compare_op;
    pipeline_iterator->second->dirty           = true;

end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_depth_writes(GraphicsPipelineID graphics_pipeline_id,
                                                         bool               should_enable)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->depth_writes_enabled = should_enable;
    pipeline_iterator->second->dirty          = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_dynamic_states(GraphicsPipelineID   graphics_pipeline_id,
                                                           bool                 should_enable,
                                                           DynamicStateBitfield dynamic_state_bits)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    if (should_enable)
    {
        pipeline_config_ptr->enabled_dynamic_states |= dynamic_state_bits;
    }
    else
    {
        pipeline_config_ptr->enabled_dynamic_states &= ~dynamic_state_bits;
    }

    pipeline_iterator->second->dirty = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_logic_op(GraphicsPipelineID graphics_pipeline_id,
                                                     bool               should_enable,
                                                     VkLogicOp          logic_op)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->logic_op         = logic_op;
    pipeline_config_ptr->logic_op_enabled = should_enable;
    pipeline_iterator->second->dirty      = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_primitive_restart(GraphicsPipelineID graphics_pipeline_id,
                                                              bool               should_enable)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->primitive_restart_enabled = should_enable;
    pipeline_iterator->second->dirty               = true;

end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_rasterizer_discard(GraphicsPipelineID graphics_pipeline_id,
                                                               bool               should_enable)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->rasterizer_discard_enabled = should_enable;
    pipeline_iterator->second->dirty                = true;

end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::set_rasterization_order(GraphicsPipelineID      graphics_pipeline_id,
                                                             VkRasterizationOrderAMD rasterization_order)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    if (pipeline_config_ptr->rasterization_order != rasterization_order)
    {
        pipeline_config_ptr->rasterization_order = rasterization_order;
        pipeline_iterator->second->dirty         = true;
    }

end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_sample_shading(GraphicsPipelineID graphics_pipeline_id,
                                                           bool               should_enable)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->sample_shading_enabled = should_enable;
    pipeline_iterator->second->dirty            = true;
end:
    ;
}

/* Please see header for specification */
void Anvil::GraphicsPipelineManager::toggle_stencil_test(GraphicsPipelineID graphics_pipeline_id,
                                                         bool               should_enable)
{
    std::shared_ptr<GraphicsPipelineConfiguration> pipeline_config_ptr;
    auto                                           pipeline_config_iterator = m_pipeline_configurations.find(graphics_pipeline_id);
    auto                                           pipeline_iterator        = m_pipelines.find              (graphics_pipeline_id);

    if (pipeline_iterator        == m_pipelines.end()               ||
        pipeline_config_iterator == m_pipeline_configurations.end() )
    {
        anvil_assert(!(pipeline_iterator        == m_pipelines.end()               ||
                      pipeline_config_iterator == m_pipeline_configurations.end()) );

        goto end;
    }
    else
    {
        pipeline_config_ptr = pipeline_config_iterator->second;
    }

    pipeline_config_ptr->stencil_test_enabled = should_enable;
    pipeline_iterator->second->dirty          = true;

end:
    ;
}