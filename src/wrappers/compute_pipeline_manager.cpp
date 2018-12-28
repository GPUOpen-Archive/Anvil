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

#include "misc/compute_pipeline_create_info.h"
#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/compute_pipeline_manager.h"
#include "wrappers/device.h"
#include "wrappers/pipeline_cache.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/shader_module.h"
#include <algorithm>


Anvil::ComputePipelineManager::ComputePipelineManager(Anvil::BaseDevice*    in_device_ptr,
                                                      bool                  in_mt_safe,
                                                      bool                  in_use_pipeline_cache,
                                                      Anvil::PipelineCache* in_pipeline_cache_to_reuse_ptr)
    :BasePipelineManager(in_device_ptr,
                         in_mt_safe,
                         in_use_pipeline_cache,
                         in_pipeline_cache_to_reuse_ptr)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::ANVIL_COMPUTE_PIPELINE_MANAGER,
                                                  this);
}


/* Stub destructor */
Anvil::ComputePipelineManager::~ComputePipelineManager()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::ANVIL_COMPUTE_PIPELINE_MANAGER,
                                                    this);

    m_baked_pipelines.clear      ();
    m_outstanding_pipelines.clear();
}

/** Re-creates Vulkan compute pipeline objects for all added non-proxy pipelines marked
 *  as dirty. A new compute pipeline layout object may, but does not have to, be also
 *  created implicitly by calling this function.
 *
 *  @return true if the function was successful, false otherwise.
 **/
bool Anvil::ComputePipelineManager::bake()
{
    typedef struct BakeItem
    {
        VkComputePipelineCreateInfo create_info;
        PipelineID                  pipeline_id;
        Pipeline*                   pipeline_ptr;

        BakeItem(const VkComputePipelineCreateInfo& in_create_info,
                 PipelineID                         in_pipeline_id,
                 Pipeline*                          in_pipeline_ptr)
        {
            create_info  = in_create_info;
            pipeline_id  = in_pipeline_id;
            pipeline_ptr = in_pipeline_ptr;
        }

        bool operator==(const PipelineID& in_pipeline_id) const
        {
            return pipeline_id == in_pipeline_id;
        }
    } BakeItem;

    std::map<VkPipelineLayout, std::vector<BakeItem> > layout_to_bake_item_map;
    std::unique_lock<std::recursive_mutex>             mutex_lock;
    auto                                               mutex_ptr                    (get_mutex() );
    uint32_t                                           n_current_pipeline           (0);
    std::vector<VkComputePipelineCreateInfo>           pipeline_create_info_items_vk;
    bool                                               result                       (false);
    std::vector<VkPipeline>                            result_pipeline_items_vk;
    VkResult                                           result_vk;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    std::vector<std::vector<VkSpecializationMapEntry> > specialization_map_entries_vk(m_outstanding_pipelines.size() );
    std::vector<VkSpecializationInfo>                   specialization_info_vk       (m_outstanding_pipelines.size());

    for (auto pipeline_iterator  = m_outstanding_pipelines.begin();
              pipeline_iterator != m_outstanding_pipelines.end();
            ++pipeline_iterator, ++n_current_pipeline)
    {
        auto                                      current_pipeline_id                      = pipeline_iterator->first;
        Pipeline*                                 current_pipeline_ptr                     = pipeline_iterator->second.get();
        const auto                                current_pipeline_create_info_ptr         = current_pipeline_ptr->pipeline_create_info_ptr.get();
        VkComputePipelineCreateInfo               pipeline_create_info;
        const Anvil::ShaderModuleStageEntryPoint* shader_stage_entry_point_ptr             = nullptr;
        const unsigned char*                      specialization_constants_data_buffer_ptr = nullptr;
        const SpecializationConstants*            specialization_constants_ptr             = nullptr;

        anvil_assert(current_pipeline_ptr->baked_pipeline == VK_NULL_HANDLE);

        if (current_pipeline_ptr->layout_ptr == nullptr)
        {
            get_pipeline_layout(pipeline_iterator->first);

            anvil_assert(current_pipeline_ptr->layout_ptr != nullptr);
        }

        current_pipeline_create_info_ptr->get_specialization_constants(Anvil::ShaderStage::COMPUTE,
                                                                      &specialization_constants_ptr,
                                                                      &specialization_constants_data_buffer_ptr);

        if (specialization_constants_ptr->size() > 0)
        {
            bake_specialization_info_vk(*specialization_constants_ptr,
                                         specialization_constants_data_buffer_ptr,
                                        &specialization_map_entries_vk[n_current_pipeline],
                                        &specialization_info_vk       [n_current_pipeline]);
        }

        /* Prepare the Vulkan create info descriptor & store it in the map for later baking */
        const auto current_pipeline_base_pipeline_id = current_pipeline_create_info_ptr->get_base_pipeline_id();

        if (current_pipeline_base_pipeline_id != UINT32_MAX)
        {
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
            auto  base_pipeline_id       = current_pipeline_ptr->pipeline_create_info_ptr->get_base_pipeline_id();
            auto  base_pipeline_iterator = std::find(pipeline_vector.begin(),
                                                     pipeline_vector.end(),
                                                     base_pipeline_id);

            if (base_pipeline_iterator != pipeline_vector.end() )
            {
                /* Case 1 */
                pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
                pipeline_create_info.basePipelineIndex  = static_cast<int32_t>(base_pipeline_iterator - pipeline_vector.begin() );
            }
            else
            {
                auto baked_pipeline_iterator = m_baked_pipelines.find(base_pipeline_id);

                if (baked_pipeline_iterator                         != m_baked_pipelines.end() &&
                    baked_pipeline_iterator->second->baked_pipeline != VK_NULL_HANDLE)
                {
                    /* Case 2 */
                    pipeline_create_info.basePipelineHandle = baked_pipeline_iterator->second->baked_pipeline;
                    pipeline_create_info.basePipelineIndex  = UINT32_MAX;
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
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex  = UINT32_MAX;
        }

        anvil_assert(current_pipeline_ptr->layout_ptr != nullptr);

        current_pipeline_create_info_ptr->get_shader_stage_properties(Anvil::ShaderStage::COMPUTE,
                                                                     &shader_stage_entry_point_ptr);

        pipeline_create_info.flags                     = 0;
        pipeline_create_info.layout                    = current_pipeline_ptr->layout_ptr->get_pipeline_layout();
        pipeline_create_info.pNext                     = nullptr;
        pipeline_create_info.stage.flags               = 0;
        pipeline_create_info.stage.pName               = shader_stage_entry_point_ptr->name.c_str();
        pipeline_create_info.stage.pNext               = nullptr;
        pipeline_create_info.stage.pSpecializationInfo = (specialization_constants_ptr->size() > 0) ? &specialization_info_vk[n_current_pipeline]
                                                                                                    : VK_NULL_HANDLE;
        pipeline_create_info.stage.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
        pipeline_create_info.stage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_create_info.sType                     = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

        pipeline_create_info.stage.module = shader_stage_entry_point_ptr->shader_module_ptr->get_module();

        anvil_assert(pipeline_create_info.stage.module != VK_NULL_HANDLE);

        if (pipeline_create_info.basePipelineHandle != VK_NULL_HANDLE                   ||
            pipeline_create_info.basePipelineIndex  != static_cast<int32_t>(UINT32_MAX) )
        {
            pipeline_create_info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        }

        pipeline_create_info.flags |= ((current_pipeline_create_info_ptr->allows_derivatives        () ) ? VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT    : 0) |
                                      ((current_pipeline_create_info_ptr->has_optimizations_disabled() ) ? VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT : 0);

        if (m_device_ptr->get_type() == Anvil::DeviceType::MULTI_GPU)
        {
            pipeline_create_info.flags |= VK_PIPELINE_CREATE_DISPATCH_BASE_KHR;
        }

        layout_to_bake_item_map[pipeline_create_info.layout].push_back(BakeItem(pipeline_create_info,
                                                                                current_pipeline_id,
                                                                                current_pipeline_ptr) );
    }

    /* We can finally bake the pipeline objects. */
    for (auto map_iterator  = layout_to_bake_item_map.begin();
              map_iterator != layout_to_bake_item_map.end();
            ++map_iterator)
    {
        uint32_t n_current_item = 0;

        pipeline_create_info_items_vk.clear();

        for (auto item_iterator  = map_iterator->second.begin();
                  item_iterator != map_iterator->second.end();
                ++item_iterator)
        {
            const BakeItem& current_bake_item = *item_iterator;

            pipeline_create_info_items_vk.push_back(current_bake_item.create_info);
        }

        result_pipeline_items_vk.resize(pipeline_create_info_items_vk.size() );

        m_pipeline_cache_ptr->lock();
        {
            result_vk = Anvil::Vulkan::vkCreateComputePipelines(m_device_ptr->get_device_vk(),
                                                                m_pipeline_cache_ptr->get_pipeline_cache(),
                                                                static_cast<uint32_t>(pipeline_create_info_items_vk.size() ),
                                                               &pipeline_create_info_items_vk[0],
                                                                nullptr, /* pAllocator */
                                                               &result_pipeline_items_vk[0]);
        }
        m_pipeline_cache_ptr->unlock();

        if (!is_vk_call_successful(result_vk))
        {
            anvil_assert_vk_call_succeeded(result_vk);

            goto end;
        }

        for (auto item_iterator  = map_iterator->second.begin();
                  item_iterator != map_iterator->second.end();
                ++item_iterator, ++n_current_item)
        {
            const BakeItem& current_bake_item = *item_iterator;

            anvil_assert(m_baked_pipelines.find(current_bake_item.pipeline_id) == m_baked_pipelines.end() );
            anvil_assert(result_pipeline_items_vk[n_current_item]    != VK_NULL_HANDLE);

            current_bake_item.pipeline_ptr->baked_pipeline  = result_pipeline_items_vk[n_current_item];
        }
    }

    for (auto& current_baked_pipeline : m_outstanding_pipelines)
    {
        m_baked_pipelines[current_baked_pipeline.first] = std::move(current_baked_pipeline.second);
    }

    m_outstanding_pipelines.clear();

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
std::unique_ptr<Anvil::ComputePipelineManager> Anvil::ComputePipelineManager::create(Anvil::BaseDevice*    in_device_ptr,
                                                                                     bool                  in_mt_safe,
                                                                                     bool                  in_use_pipeline_cache,
                                                                                     Anvil::PipelineCache* in_pipeline_cache_to_reuse_ptr)
{
    std::unique_ptr<Anvil::ComputePipelineManager> result_ptr;

    result_ptr.reset(
        new Anvil::ComputePipelineManager(in_device_ptr,
                                          in_mt_safe,
                                          in_use_pipeline_cache,
                                          in_pipeline_cache_to_reuse_ptr)
    );

    return result_ptr;
}
