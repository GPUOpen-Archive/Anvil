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

#include "misc/base_pipeline_manager.h"
#include "misc/debug.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/device.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/pipeline_layout_manager.h"
#include "wrappers/pipeline_cache.h"
#include <algorithm>

/** Please see header for specification */
Anvil::BasePipelineManager::BasePipelineManager(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                                                bool                                  in_use_pipeline_cache,
                                                std::shared_ptr<Anvil::PipelineCache> in_pipeline_cache_to_reuse_ptr)
    :m_device_ptr      (in_device_ptr),
     m_pipeline_counter(0)
{
    anvil_assert(!in_use_pipeline_cache && in_pipeline_cache_to_reuse_ptr == nullptr ||
                  in_use_pipeline_cache);

    m_pipeline_layout_manager_ptr = in_device_ptr.lock()->get_pipeline_layout_manager();
    anvil_assert(m_pipeline_layout_manager_ptr != nullptr);

    if (in_pipeline_cache_to_reuse_ptr != nullptr)
    {
        m_pipeline_cache_ptr   = in_pipeline_cache_to_reuse_ptr;
        m_use_pipeline_cache   = true;
    }
    else
    {
        if (in_use_pipeline_cache)
        {
            m_pipeline_cache_ptr = Anvil::PipelineCache::create(m_device_ptr);
        }

        m_use_pipeline_cache = in_use_pipeline_cache;
    }
}

/** Please see header for specification */
Anvil::BasePipelineManager::~BasePipelineManager()
{
    anvil_assert(m_pipelines.size() == 0);

   m_pipeline_layout_manager_ptr.reset();
}


/** Helper function which releases the internally managed raw Vulkan pipeline object handle.
 *
 *  The function does NOT release the layout object, since it's owned by the Pipeline Layout manager.
 *s*/
void Anvil::BasePipelineManager::Pipeline::release_vulkan_objects()
{
    if (baked_pipeline != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(device_ptr);

        vkDestroyPipeline(device_locked_ptr->get_device_vk(),
                          baked_pipeline,
                          nullptr /* pAllocator */);

        baked_pipeline = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::add_derivative_pipeline_from_sibling_pipeline(bool                               in_disable_optimizations,
                                                                               bool                               in_allow_derivatives,
                                                                               uint32_t                           in_n_shader_module_stage_entrypoints,
                                                                               const ShaderModuleStageEntryPoint* in_shader_module_stage_entrypoint_ptrs,
                                                                               PipelineID                         in_base_pipeline_id,
                                                                               PipelineID*                        out_pipeline_id_ptr)
{
    std::shared_ptr<Pipeline> base_pipeline_ptr;
    PipelineID                new_pipeline_id  = 0;
    std::shared_ptr<Pipeline> new_pipeline_ptr;
    bool                      result           = false;

    /* Retrieve base pipeline's descriptor */
    if (m_pipelines.size() <= in_base_pipeline_id)
    {
        anvil_assert(!(m_pipelines.size() <= in_base_pipeline_id) );

        goto end;
    }
    else
    {
        base_pipeline_ptr = m_pipelines.at(in_base_pipeline_id);

        anvil_assert(base_pipeline_ptr != nullptr);
    }

    /* Create & store the new descriptor */
    anvil_assert(base_pipeline_ptr->allow_derivatives);

    new_pipeline_id  = (m_pipeline_counter++);
    new_pipeline_ptr = std::shared_ptr<Pipeline>(new Pipeline(m_device_ptr,
                                                              in_disable_optimizations,
                                                              in_allow_derivatives,
                                                              base_pipeline_ptr,
                                                              in_n_shader_module_stage_entrypoints,
                                                              in_shader_module_stage_entrypoint_ptrs) );

    anvil_assert(m_pipelines.find(new_pipeline_id) == m_pipelines.end() );

    m_pipelines[new_pipeline_id] = new_pipeline_ptr;
    *out_pipeline_id_ptr         = new_pipeline_id;

    /* All done */
    result  = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::add_derivative_pipeline_from_pipeline(bool                               in_disable_optimizations,
                                                                       bool                               in_allow_derivatives,
                                                                       uint32_t                           in_n_shader_module_stage_entrypoints,
                                                                       const ShaderModuleStageEntryPoint* in_shader_module_stage_entrypoint_ptrs,
                                                                       VkPipeline                         in_base_pipeline,
                                                                       PipelineID*                        out_pipeline_id_ptr)
{
    PipelineID                new_pipeline_id  = 0;
    std::shared_ptr<Pipeline> new_pipeline_ptr;
    bool                      result = false;

    if (in_base_pipeline == VK_NULL_HANDLE)
    {
        anvil_assert(!(in_base_pipeline != VK_NULL_HANDLE));

        goto end;
    }

    /* Create & store the new descriptor */
    new_pipeline_id  = (m_pipeline_counter++);
    new_pipeline_ptr = std::shared_ptr<Pipeline>(new Pipeline(m_device_ptr,
                                                              in_disable_optimizations,
                                                              in_allow_derivatives,
                                                              in_base_pipeline,
                                                              in_n_shader_module_stage_entrypoints,
                                                              in_shader_module_stage_entrypoint_ptrs) );

    *out_pipeline_id_ptr         = new_pipeline_id;
    m_pipelines[new_pipeline_id] = new_pipeline_ptr;

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::add_proxy_pipeline(PipelineID* out_pipeline_id_ptr)
{
    PipelineID                new_pipeline_id  = 0;
    std::shared_ptr<Pipeline> new_pipeline_ptr;

    /* Create & store the new descriptor */
    new_pipeline_id  = (m_pipeline_counter++);
    new_pipeline_ptr = std::shared_ptr<Pipeline>(new Pipeline(m_device_ptr,
                                                              false,   /* in_disable_optimizations */
                                                              false,   /* in_allow_derivatives     */
                                                              0,       /* in_n_shaders             */
                                                              nullptr, /* in_shaders               */
                                                              true) ); /* in_is_proxy              */

    *out_pipeline_id_ptr         = new_pipeline_id;
    m_pipelines[new_pipeline_id] = new_pipeline_ptr;

    /* All done */
    return true;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::add_regular_pipeline(bool                               in_disable_optimizations,
                                                      bool                               in_allow_derivatives,
                                                      uint32_t                           in_n_shader_module_stage_entrypoints,
                                                      const ShaderModuleStageEntryPoint* in_shader_module_stage_entrypoint_ptrs,
                                                      PipelineID*                        out_pipeline_id_ptr)
{
    PipelineID                new_pipeline_id  = 0;
    std::shared_ptr<Pipeline> new_pipeline_ptr;

    /* Create & store the new descriptor */
    new_pipeline_id  = (m_pipeline_counter++);
    new_pipeline_ptr = std::shared_ptr<Pipeline>(new Pipeline(m_device_ptr,
                                                              in_disable_optimizations,
                                                              in_allow_derivatives,
                                                              in_n_shader_module_stage_entrypoints,
                                                              in_shader_module_stage_entrypoint_ptrs,
                                                              false /* in_is_proxy */) );

    *out_pipeline_id_ptr         = new_pipeline_id;
    m_pipelines[new_pipeline_id] = new_pipeline_ptr;

    /* All done */
    return true;
}


bool Anvil::BasePipelineManager::add_specialization_constant_to_pipeline(PipelineID  in_pipeline_id,
                                                                         ShaderIndex in_shader_index,
                                                                         uint32_t    in_constant_id,
                                                                         uint32_t    in_n_data_bytes,
                                                                         const void* in_data_ptr)
{
    Pipelines::iterator       pipeline_iterator;
    std::shared_ptr<Pipeline> pipeline_ptr;
    uint32_t                  data_buffer_size = 0;
    bool                      result           = false;

    if (in_n_data_bytes == 0)
    {
        anvil_assert(!(in_n_data_bytes == 0) );

        goto end;
    }

    if (in_data_ptr == nullptr)
    {
        anvil_assert(!(in_data_ptr == nullptr) );

        goto end;
    }

    /* Retrieve the pipeline's descriptor */
    pipeline_iterator = m_pipelines.find(in_pipeline_id);

    if (pipeline_iterator == m_pipelines.end() )
    {
        anvil_assert(pipeline_iterator != m_pipelines.end() )

        goto end;
    }
    else
    {
        pipeline_ptr = pipeline_iterator->second;
    }

    if (pipeline_ptr->is_proxy)
    {
        anvil_assert(!pipeline_ptr->is_proxy);

        goto end;
    }

    /* Append specialization constant data and add a new descriptor. */
    data_buffer_size = static_cast<uint32_t>(pipeline_ptr->specialization_constant_data_buffer.size() );


    anvil_assert(pipeline_ptr->specialization_constants_map.find(in_shader_index) != pipeline_ptr->specialization_constants_map.end() );

    pipeline_ptr->specialization_constants_map[in_shader_index].push_back(SpecializationConstant(in_constant_id,
                                                                                                 in_n_data_bytes,
                                                                                                 data_buffer_size));

    pipeline_ptr->specialization_constant_data_buffer.resize(data_buffer_size + in_n_data_bytes);


    memcpy(&pipeline_ptr->specialization_constant_data_buffer.at(data_buffer_size),
           in_data_ptr,
           in_n_data_bytes);

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::set_pipeline_dsg(PipelineID                                 in_pipeline_id,
                                                  std::shared_ptr<Anvil::DescriptorSetGroup> in_dsg_ptr)
{
    std::shared_ptr<Pipeline> pipeline_ptr;
    bool                      result = false;

    if (in_dsg_ptr == nullptr)
    {
        anvil_assert(!(in_dsg_ptr == nullptr) );

        goto end;
    }

    /* Retrieve pipeline's descriptor and attach the specified DSG */
    if (m_pipelines.find(in_pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(in_pipeline_id) == m_pipelines.end() ));

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines.at(in_pipeline_id);
    }

    /* Make sure the DSG has not already been attached */
    if (pipeline_ptr->dsg_ptr == in_dsg_ptr)
    {
        anvil_assert_fail();

        goto end;
    }

    /* If we reached this place, we can update the DSG */
    pipeline_ptr->dirty        = true;
    pipeline_ptr->dsg_ptr      = in_dsg_ptr;
    pipeline_ptr->layout_dirty = true;

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::attach_push_constant_range_to_pipeline(PipelineID         in_pipeline_id,
                                                                        uint32_t           in_offset,
                                                                        uint32_t           in_size,
                                                                        VkShaderStageFlags in_stages)
{
    const auto                new_descriptor = Anvil::PushConstantRange(in_offset,
                                                                        in_size,
                                                                        in_stages);
    std::shared_ptr<Pipeline> pipeline_ptr;
    bool                      result         = false;

    /* Retrieve pipeline's descriptor and add the specified push constant range */
    if (m_pipelines.find(in_pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(in_pipeline_id) == m_pipelines.end()));

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines.at(in_pipeline_id);
    }

    pipeline_ptr->dirty        = true;
    pipeline_ptr->layout_dirty = true;

    anvil_assert(std::find(pipeline_ptr->push_constant_ranges.begin(),
                           pipeline_ptr->push_constant_ranges.end(),
                           new_descriptor) == pipeline_ptr->push_constant_ranges.end());

    pipeline_ptr->push_constant_ranges.push_back(new_descriptor);

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
void Anvil::BasePipelineManager::bake_specialization_info_vk(const SpecializationConstants&         in_specialization_constants,
                                                             const unsigned char*                   in_specialization_constant_data_ptr,
                                                             std::vector<VkSpecializationMapEntry>* out_specialization_map_entry_vk_vector_ptr,
                                                             VkSpecializationInfo*                  out_specialization_info_ptr)
{
    uint32_t n_specialization_constant_bytes = 0;
    uint32_t n_specialization_constants      = 0;

    /* Prepare specialization info descriptor */
    n_specialization_constants = static_cast<uint32_t>(in_specialization_constants.size() );

    out_specialization_map_entry_vk_vector_ptr->clear  ();
    out_specialization_map_entry_vk_vector_ptr->reserve(n_specialization_constants);

    for (uint32_t n_specialization_constant = 0;
                  n_specialization_constant < n_specialization_constants;
                ++n_specialization_constant)
    {
        const SpecializationConstant& current_specialization_constant = in_specialization_constants[n_specialization_constant];
        VkSpecializationMapEntry      new_entry;

        new_entry.constantID = current_specialization_constant.constant_id;
        new_entry.offset     = current_specialization_constant.start_offset;
        new_entry.size       = current_specialization_constant.n_bytes;

        n_specialization_constant_bytes += current_specialization_constant.n_bytes;

        out_specialization_map_entry_vk_vector_ptr->push_back(new_entry);
    }

    out_specialization_info_ptr->dataSize      = n_specialization_constant_bytes;
    out_specialization_info_ptr->mapEntryCount = n_specialization_constants;
    out_specialization_info_ptr->pData         = (n_specialization_constants > 0) ? in_specialization_constant_data_ptr
                                                                                  : nullptr;
    out_specialization_info_ptr->pMapEntries   = (n_specialization_constants > 0) ? &(*out_specialization_map_entry_vk_vector_ptr).at(0)
                                                                                  : nullptr;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::delete_pipeline(PipelineID in_pipeline_id)
{
    auto pipeline_iterator = m_pipelines.find(in_pipeline_id);
    bool result = false;

    if (pipeline_iterator == m_pipelines.end() )
    {
        goto end;
    }

    m_pipelines.erase(pipeline_iterator);

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::get_general_pipeline_properties(PipelineID in_pipeline_id,
                                                                 bool*      out_opt_has_optimizations_disabled_ptr,
                                                                 bool*      out_opt_allows_derivatives_ptr,
                                                                 bool*      out_opt_is_a_derivative_ptr) const
{
    auto pipeline_iterator = m_pipelines.find(in_pipeline_id);
    bool result = false;

    if (pipeline_iterator == m_pipelines.end() )
    {
        goto end;
    }

    if (out_opt_has_optimizations_disabled_ptr != nullptr)
    {
        *out_opt_has_optimizations_disabled_ptr = pipeline_iterator->second->disable_optimizations;
    }

    if (out_opt_allows_derivatives_ptr != nullptr)
    {
        *out_opt_allows_derivatives_ptr = pipeline_iterator->second->allow_derivatives;
    }

    if (out_opt_is_a_derivative_ptr != nullptr)
    {
        *out_opt_is_a_derivative_ptr = pipeline_iterator->second->is_derivative;
    }

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
VkPipeline Anvil::BasePipelineManager::get_pipeline(PipelineID in_pipeline_id)
{
    std::shared_ptr<Pipeline> pipeline_ptr;
    VkPipeline                result = VK_NULL_HANDLE;

    if (m_pipelines.find(in_pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(in_pipeline_id) == m_pipelines.end()) );

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines.at(in_pipeline_id);
    }

    if (pipeline_ptr->is_proxy)
    {
        anvil_assert(!pipeline_ptr->is_proxy);

        goto end;
    }

    if (pipeline_ptr->dirty)
    {
        bake();

        anvil_assert( pipeline_ptr->baked_pipeline != VK_NULL_HANDLE);
        anvil_assert(!pipeline_ptr->dirty);
    }

    result = m_pipelines.at(in_pipeline_id)->baked_pipeline;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::get_pipeline_id_at_index(uint32_t    in_n_pipeline,
                                                          PipelineID* out_pipeline_id_ptr) const
{
    Pipelines::const_iterator current_pipeline_it = m_pipelines.cbegin();
    uint32_t                  n_current_pipeline  = 0;
    bool                      result              = false;

    if (m_pipelines.size() <= in_n_pipeline)
    {
        goto end;
    }

    while (n_current_pipeline != in_n_pipeline)
    {
        current_pipeline_it++;
        n_current_pipeline ++;
    }

    *out_pipeline_id_ptr = current_pipeline_it->first;
    result               = true;
end:
    return result;
}

/* Please see header for specification */
std::shared_ptr<Anvil::PipelineLayout> Anvil::BasePipelineManager::get_pipeline_layout(PipelineID in_pipeline_id)
{
    std::shared_ptr<Pipeline>              pipeline_ptr;
    std::shared_ptr<Anvil::PipelineLayout> result_ptr;

    if (m_pipelines.find(in_pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(in_pipeline_id) == m_pipelines.end()) );

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines.at(in_pipeline_id);
    }

    if (pipeline_ptr->is_proxy)
    {
        anvil_assert(!pipeline_ptr->is_proxy);

        goto end;
    }

    if (pipeline_ptr->layout_dirty)
    {
        pipeline_ptr->layout_ptr.reset();

        if (!m_pipeline_layout_manager_ptr->get_layout(pipeline_ptr->dsg_ptr,
                                                       pipeline_ptr->push_constant_ranges,
                                                      &pipeline_ptr->layout_ptr) )
        {
            anvil_assert_fail();

            goto end;
        }

        pipeline_ptr->layout_dirty = false;
    }

    result_ptr = pipeline_ptr->layout_ptr;

end:
    return result_ptr;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::get_shader_stage_properties(PipelineID                   in_pipeline_id,
                                                             Anvil::ShaderStage           in_shader_stage,
                                                             ShaderModuleStageEntryPoint* out_opt_result_ptr) const
{
    auto pipeline_iterator = m_pipelines.find(in_pipeline_id);
    bool result            = false;

    if (pipeline_iterator == m_pipelines.end() )
    {
        anvil_assert(!(pipeline_iterator == m_pipelines.end()) );

        goto end;
    }

    for (const auto& current_stage : pipeline_iterator->second->shader_stages)
    {
        if (current_stage.stage == in_shader_stage)
        {
            if (out_opt_result_ptr != nullptr)
            {
                *out_opt_result_ptr = current_stage;
            }

            result = true;

            break;
        }
    }

    /* All done */
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::set_pipeline_bakeability(PipelineID in_pipeline_id,
                                                          bool       in_bakeable)
{
    std::shared_ptr<Pipeline> pipeline_ptr;
    bool                      result        = false;

    if (m_pipelines.find(in_pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(in_pipeline_id) == m_pipelines.end()) );

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines.at(in_pipeline_id);
    }

    pipeline_ptr->is_bakeable = in_bakeable;

    result = true;
end:
    return result;
}
