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
 *  Each baked graphics pipeline is configured as below at Pipeline object creation time:
 *
 *  All rendering modes & tests:          disabled
 *  Blend constant:                       vec4(0.0)
 *  Cull mode:                            VK_CULL_MODE_BACK
 *  Depth bias:                           0.0
 *  Depth bias clamp:                     0.0
 *  Depth bias slope factor:              1.0
 *  Depth test compare op:                VK_COMPARE_OP_ALWAYS
 *  Depth writes:                         disabled
 *  Dynamic states:                       all disabled
 *  Fill mode:                            VK_FILL_MODE_SOLID
 *  Front face:                           VK_FRONT_FACE_CCW
 *  Line width:                           1.0
 *  Logic op:                             VK_LOGIC_OP_NOOP;
 *  Max depth boundary:                   1.0
 *  Min depth boundary:                   0.0
 *  Min sample shading:                   1.0
 *  Number of raster samples:             1
 *  Number of tessellation patches:       1
 *  Primitive topology:                   VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
 *  Sample mask:                          0xFFFFFFFF
 *  Slope scaled depth bias:              0.0
 *  Stencil comparison mask (back/front): 0xFFFFFFFF
 *  Stencil comparison op   (back/front): VK_COMPARE_OP_ALWAYS
 *  Stencil depth fail op   (back/front): VK_STENCIL_OP_KEEP
 *  Stencil fail op         (back/front): VK_STENCIL_OP_KEEP
 *  Stencil pass op         (back/front): VK_STENCIL_OP_KEEP
 *  Stencil reference value (back/front): 0
 *  Stencil write mask      (back/front): 0xFFFFFFFF
 *
 *  If no scissor or viewport is defined explicitly, one scissor box and one viewport,
 *  covering the whole screen, will be created at baking time.
 *
 *  If VK_AMD_rasterization_order extension is supported, strict rasterization order is assumed
 *  for the pipeline by default.
 *
 **/
#ifndef WRAPPERS_GRAPHICS_PIPELINE_MANAGER_H
#define WRAPPERS_GRAPHICS_PIPELINE_MANAGER_H

#include "misc/base_pipeline_manager.h"
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

        typedef struct GraphicsPipelineData
        {
            AttributeLocationToBindingIndexMap             attribute_location_to_binding_index_map;
            const Anvil::GraphicsPipelineCreateInfo*       pipeline_create_info_ptr;
            std::vector<VkVertexInputAttributeDescription> vk_input_attributes;
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

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(GraphicsPipelineManager);
        ANVIL_DISABLE_COPY_CONSTRUCTOR   (GraphicsPipelineManager);

        /* Private variables */
        GraphicsPipelineDataMap m_pipeline_id_to_gfx_pipeline_data;
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_GRAPHICS_PIPELINE_MANAGER_H */