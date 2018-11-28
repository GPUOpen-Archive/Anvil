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

/** Graphics pipeline manager. A class which inherits from the base pipeline object manager.
 *
 *  Apart from exposing the functionality offered by the parent class under slightly
 *  renamed, pipeline-specific function names, this wrapper implements the following features:
 *
 * - baking of the graphics pipeline object
 * - pipeline properties are assigned default values, as described below. They can be
 *   adjusted by calling relevant entrypoints, prior to baking.
 *
 **/
#ifndef WRAPPERS_GRAPHICS_PIPELINE_MANAGER_H
#define WRAPPERS_GRAPHICS_PIPELINE_MANAGER_H

#include "misc/base_pipeline_manager.h"
#include "misc/graphics_pipeline_create_info.h"
#include "misc/struct_chainer.h"
#include "misc/types.h"
#include "wrappers/render_pass.h"
#include <map>

namespace Anvil
{
    class GraphicsPipelineManager : public BasePipelineManager
    {
    public:
        /* Public type definitions */

        /* Public functions */

        /** Generates a VkPipeline instance for each outstanding pipeline object.
         *
         *  @return true if successful, false otherwise.
         **/
        bool bake();

        bool delete_pipeline(PipelineID in_pipeline_id);

        /** Creates a new GraphicsPipelineManager instance.
         *
         *  @param in_device_ptr                  Device to use.
         *  @param in_mt_safe                     True if more than one thread at a time is going to be issuing calls against the pipeline manager.
         *  @param in_use_pipeline_cache          true if the manager should use a pipeline cache instance. false
         *                                        to pass nullptr whenever a Vulkan descriptor requires us to specify
         *                                        one.
         *  @param in_pipeline_cache_to_reuse_ptr if @param use_pipeline_cache is true, this argument can be optionally
         *                                        set to a non-nullptr value to point at an already allocated pipeline cache.
         *                                        If one is not provided and the other argument is set as described,
         *                                        a new pipeline cache with size 0 will be allocated.
         **/
        static GraphicsPipelineManagerUniquePtr create(const Anvil::BaseDevice* in_device_ptr,
                                                       bool                     in_mt_safe,
                                                       bool                     in_use_pipeline_cache          = false,
                                                       Anvil::PipelineCache*    in_pipeline_cache_to_reuse_ptr = nullptr);

        /** Destructor. */
        virtual ~GraphicsPipelineManager();

    private:
        /* Private type declarations */
        typedef std::map<uint32_t, uint32_t> AttributeLocationToBindingIndexMap;

        typedef struct VertexInputBinding
        {
            uint32_t               binding;
            uint32_t               divisor;
            Anvil::VertexInputRate input_rate;
            uint32_t               stride;

            VertexInputBinding()
            {
                binding    = 0;
                divisor    = 0;
                input_rate = Anvil::VertexInputRate::UNKNOWN;
                stride     = 0;
            }

            VertexInputBinding(const VkVertexInputBindingDescription& in_binding_vk,
                               const uint32_t&                        in_divisor)
            {
                binding    = in_binding_vk.binding;
                divisor    = in_divisor;
                input_rate = static_cast<Anvil::VertexInputRate>(in_binding_vk.inputRate);
                stride     = in_binding_vk.stride;
            }
        } VertexInputBinding;

        typedef struct GraphicsPipelineData
        {
            AttributeLocationToBindingIndexMap             attribute_location_to_binding_index_map;
            const Anvil::GraphicsPipelineCreateInfo*       pipeline_create_info_ptr;
            std::vector<VkVertexInputAttributeDescription> vk_input_attributes;

            /* NOTE: VkVertexInputBindingDescription does not include divisor information so we need to maintain a custom
             *       set of structs as well.
             */
            std::vector<VertexInputBinding>                input_bindings;
            std::vector<VkVertexInputBindingDescription>   vk_input_bindings;

            explicit GraphicsPipelineData(const Anvil::GraphicsPipelineCreateInfo* in_pipeline_create_info_ptr)
            {
                pipeline_create_info_ptr = in_pipeline_create_info_ptr;

                bake_vk_attributes_and_bindings();
            }

        private:
            void bake_vk_attributes_and_bindings();
        } GraphicsPipelineData;

        typedef std::map<PipelineID, std::unique_ptr<GraphicsPipelineData> > GraphicsPipelineDataMap;

        /* Private functions */
        explicit GraphicsPipelineManager(const Anvil::BaseDevice* in_device_ptr,
                                         bool                     in_mt_safe,
                                         bool                     in_use_pipeline_cache,
                                         Anvil::PipelineCache*    in_pipeline_cache_to_reuse_ptr);

        Anvil::StructChainUniquePtr<VkGraphicsPipelineCreateInfo>                   bake_graphics_pipeline_create_info                 (const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr,
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
                                                                                                                                        const VkPipelineViewportStateCreateInfo*      in_opt_viewport_state_create_info_ptr) const;
        Anvil::StructChainUniquePtr<VkPipelineColorBlendStateCreateInfo>            bake_pipeline_color_blend_state_create_info        (const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr,
                                                                                                                                        const Anvil::RenderPass*                      in_current_renderpass_ptr,
                                                                                                                                        const Anvil::SubPassID&                       in_subpass_id)                         const;
        Anvil::StructChainUniquePtr<VkPipelineDepthStencilStateCreateInfo>          bake_pipeline_depth_stencil_state_create_info      (const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr,
                                                                                                                                        const Anvil::RenderPass*                      in_current_renderpass_ptr)             const;
        Anvil::StructChainUniquePtr<VkPipelineDynamicStateCreateInfo>               bake_pipeline_dynamic_state_create_info            (const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr)       const;
        Anvil::StructChainUniquePtr<VkPipelineInputAssemblyStateCreateInfo>         bake_pipeline_input_assembly_state_create_info     (const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr)       const;
        Anvil::StructChainUniquePtr<VkPipelineMultisampleStateCreateInfo>           bake_pipeline_multisample_state_create_info        (const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr)       const;
        Anvil::StructChainUniquePtr<VkPipelineRasterizationStateCreateInfo>         bake_pipeline_rasterization_state_create_info      (const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr)       const;
        std::unique_ptr<Anvil::StructChainVector<VkPipelineShaderStageCreateInfo> > bake_pipeline_shader_stage_create_info_chain_vector(const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr)       const;
        Anvil::StructChainUniquePtr<VkPipelineTessellationStateCreateInfo>          bake_pipeline_tessellation_state_create_info       (const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr)       const;
        Anvil::StructChainUniquePtr<VkPipelineVertexInputStateCreateInfo>           bake_pipeline_vertex_input_state_create_info       (const Anvil::GraphicsPipelineCreateInfo*      in_gfx_pipeline_create_info_ptr)       const;
        Anvil::StructChainUniquePtr<VkPipelineViewportStateCreateInfo>              bake_pipeline_viewport_state_create_info           (Anvil::GraphicsPipelineCreateInfo*            in_gfx_pipeline_create_info_ptr,
                                                                                                                                        const bool&                                   in_is_dynamic_scissor_state_enabled,
                                                                                                                                        const bool&                                   in_is_dynamic_viewport_state_enabled)  const;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(GraphicsPipelineManager);
        ANVIL_DISABLE_COPY_CONSTRUCTOR   (GraphicsPipelineManager);

        /* Private variables */
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_GRAPHICS_PIPELINE_MANAGER_H */