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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/compute_pipeline_manager.h"
#include "wrappers/device.h"
#include "wrappers/pipeline_cache.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/shader_module.h"
#include <algorithm>


/** Constructor. Initializes the compute pipeline manager.
 *
 *  @param device_ptr              Device to use.
 *  @param use_pipeline_cache      true if a VkPipelineCache instance should be used to spawn new pipeline objects.
 *                                 What pipeline cache ends up being used depends on @param pipeline_cache_to_reuse -
 *                                 if a nullptr object is passed via this argument, a new pipeline cache instance will
 *                                 be created, and later released by the destructor. If a non-nullptr object is passed,
 *                                 it will be used instead. In the latter scenario, it is the caller's responsibility
 *                                 to release the cache when no longer needed!
 *  @param pipeline_cache_to_reuse Please see above.
 **/
Anvil::ComputePipelineManager::ComputePipelineManager(Anvil::Device*        device_ptr,
                                                      bool                  use_pipeline_cache,
                                                      Anvil::PipelineCache* pipeline_cache_to_reuse_ptr)
    :BasePipelineManager(device_ptr,
                         use_pipeline_cache,
                         pipeline_cache_to_reuse_ptr)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_COMPUTE_PIPELINE_MANAGER,
                                                  this);
}


/* Stub destructor */
Anvil::ComputePipelineManager::~ComputePipelineManager()
{
    m_pipelines.clear();

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_COMPUTE_PIPELINE_MANAGER,
                                                    this);
}

/** Re-creates Vulkan compute pipeline objects for all added non-proxy pipelines marked
 *  as dirty. A new compute pipeline layout object may, but does not have to, be also
 *  created implicitly by calling this function.
 *
 *  @return true if the function was successful, false otherwise.
 **/
bool Anvil::ComputePipelineManager::bake()
{
    std::vector<VkComputePipelineCreateInfo> pipeline_create_info_items_vk;
    bool                                     result = false;
    std::vector<VkPipeline>                  result_pipeline_items_vk;
    VkResult                                 result_vk;
    std::vector<VkSpecializationMapEntry>    specialization_map_entries_vk;

    typedef struct _bake_item
    {
        VkComputePipelineCreateInfo create_info;
        std::shared_ptr<Pipeline>   pipeline_ptr;

        _bake_item(const VkComputePipelineCreateInfo& in_create_info,
                   std::shared_ptr<Pipeline>          in_pipeline_ptr)
        {
            create_info  = in_create_info;
            pipeline_ptr = in_pipeline_ptr;
        }

        bool operator==(std::shared_ptr<Pipeline> in_pipeline_ptr)
        {
            return pipeline_ptr == in_pipeline_ptr;
        }
    } _bake_item;

    std::map<VkPipelineLayout, std::vector<_bake_item> > layout_to_bake_item_map;

    /* Iterate over all compute pipelines and identify the ones marked as dirty. Only these
     * need to be re-created */
    for (auto pipeline_iterator  = m_pipelines.begin();
              pipeline_iterator != m_pipelines.end();
            ++pipeline_iterator)
    {
        uint32_t                    base_pipeline_index   = -1;
        std::shared_ptr<Pipeline>   current_pipeline_ptr  = pipeline_iterator->second;
        VkComputePipelineCreateInfo pipeline_create_info;
        VkSpecializationInfo        specialization_info;

        if (!current_pipeline_ptr->dirty     ||
             current_pipeline_ptr->is_proxy)
        {
            continue;
        }

        if (current_pipeline_ptr->baked_pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_device_ptr->get_device_vk(),
                              current_pipeline_ptr->baked_pipeline,
                              nullptr /* pAllocator */);

            current_pipeline_ptr->baked_pipeline = VK_NULL_HANDLE;
        }

        if (current_pipeline_ptr->layout_dirty)
        {
            if (current_pipeline_ptr->layout_ptr != nullptr)
            {
                current_pipeline_ptr->layout_ptr->release();
            }

            current_pipeline_ptr->layout_ptr = get_pipeline_layout(pipeline_iterator->first);
        }

        if (current_pipeline_ptr->specialization_constants_map[0].size() > 0)
        {
            anvil_assert(current_pipeline_ptr->specialization_constant_data_buffer.size() == current_pipeline_ptr->specialization_constants_map.size());

            bake_specialization_info_vk(current_pipeline_ptr->specialization_constants_map       [0],
                                       &current_pipeline_ptr->specialization_constant_data_buffer[0],
                                       &specialization_map_entries_vk,
                                       &specialization_info);
        }

        /* Prepare the Vulkan create info descriptor & store it in the map for later baking */
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
             * NOTE: A slightly adjusted version of this code is re-used in GraphicsPipelineManager::bake() */
            auto& pipeline_vector        = layout_to_bake_item_map[pipeline_create_info.layout];
            auto  base_pipeline_iterator = std::find(pipeline_vector.begin(),
                                                     pipeline_vector.end(),
                                                     current_pipeline_ptr->base_pipeline_ptr);

            if (base_pipeline_iterator != pipeline_vector.end() )
            {
                /* Case 1 */
                pipeline_create_info.basePipelineHandle = current_pipeline_ptr->base_pipeline;
                pipeline_create_info.basePipelineIndex  = (uint32_t) (base_pipeline_iterator - pipeline_vector.begin() );
            }
            else
            if (current_pipeline_ptr->base_pipeline_ptr                 != nullptr           &&
                current_pipeline_ptr->base_pipeline_ptr->baked_pipeline != VK_NULL_HANDLE)
            {
                /* Case 2 */
                pipeline_create_info.basePipelineHandle = current_pipeline_ptr->base_pipeline_ptr->baked_pipeline;
                pipeline_create_info.basePipelineIndex  = -1; /* unused. */
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
            pipeline_create_info.basePipelineHandle = current_pipeline_ptr->base_pipeline;
            pipeline_create_info.basePipelineIndex  = -1; /* unused. */
        }
        else
        {
            /* No base pipeline requested */
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex  = -1; /* unused */
        }

        anvil_assert(!current_pipeline_ptr->layout_dirty);

        pipeline_create_info.flags                     = 0;
        pipeline_create_info.layout                    = current_pipeline_ptr->layout_ptr->get_pipeline_layout();
        pipeline_create_info.pNext                     = VK_NULL_HANDLE;
        pipeline_create_info.stage.flags               = 0;
        pipeline_create_info.stage.pName               = current_pipeline_ptr->shader_stages[0].name;
        pipeline_create_info.stage.pNext               = nullptr;
        pipeline_create_info.stage.pSpecializationInfo = (current_pipeline_ptr->specialization_constants_map[0].size() > 0) ? &specialization_info
                                                                                                                            : VK_NULL_HANDLE;
        pipeline_create_info.stage.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
        pipeline_create_info.stage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_create_info.sType                     = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

        pipeline_create_info.stage.module = current_pipeline_ptr->shader_stages[0].shader_module_ptr->get_module();

        if (pipeline_create_info.basePipelineHandle != VK_NULL_HANDLE ||
            pipeline_create_info.basePipelineIndex != -1)
        {
            pipeline_create_info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        }

        pipeline_create_info.flags |= ((current_pipeline_ptr->allow_derivatives)                   ? VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT    : 0) |
                                      ((current_pipeline_ptr->disable_optimizations)               ? VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT : 0) |
                                      ((pipeline_create_info.basePipelineHandle != VK_NULL_HANDLE) ? VK_PIPELINE_CREATE_DERIVATIVE_BIT           : 0);

        layout_to_bake_item_map[pipeline_create_info.layout].push_back(_bake_item(pipeline_create_info,
                                                                                  current_pipeline_ptr) );
    }

    /* We can finally bake the pipeline objects. */
    for (auto map_iterator  = layout_to_bake_item_map.begin();
              map_iterator != layout_to_bake_item_map.end();
            ++map_iterator)
    {
        const VkPipelineLayout current_layout = map_iterator->first;
        uint32_t               n_current_item = 0;

        pipeline_create_info_items_vk.clear();

        for (auto item_iterator  = map_iterator->second.begin();
                  item_iterator != map_iterator->second.end();
                ++item_iterator)
        {
            const _bake_item& current_bake_item = *item_iterator;

            pipeline_create_info_items_vk.push_back(current_bake_item.create_info);
        }

        result_pipeline_items_vk.resize(pipeline_create_info_items_vk.size() );

        result_vk = vkCreateComputePipelines(m_device_ptr->get_device_vk(),
                                             m_pipeline_cache_ptr->get_pipeline_cache(),
                                             (uint32_t) pipeline_create_info_items_vk.size(),
                                            &pipeline_create_info_items_vk[0],
                                             nullptr, /* pAllocator */
                                            &result_pipeline_items_vk[0]);

        if (!is_vk_call_successful(result_vk))
        {
            anvil_assert_vk_call_succeeded(result_vk);

            goto end;
        }

        for (auto item_iterator  = map_iterator->second.begin();
                  item_iterator != map_iterator->second.end();
                ++item_iterator, ++n_current_item)
        {
            const _bake_item& current_bake_item = *item_iterator;

            anvil_assert(result_pipeline_items_vk[n_current_item] != VK_NULL_HANDLE);

            current_bake_item.pipeline_ptr->baked_pipeline  = result_pipeline_items_vk[n_current_item];
            current_bake_item.pipeline_ptr->dirty           = false;
        }
    }

    /* All done */
    result = true;
end:
    return result;
}

