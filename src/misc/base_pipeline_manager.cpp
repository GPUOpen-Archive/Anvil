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

#include "misc/base_pipeline_manager.h"
#include "misc/debug.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/device.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/pipeline_layout_manager.h"
#include "wrappers/pipeline_cache.h"
#include <algorithm>

/** Please see header for specification */
Anvil::BasePipelineManager::BasePipelineManager(Anvil::Device*        device_ptr,
                                                bool                  use_pipeline_cache,
                                                Anvil::PipelineCache* pipeline_cache_to_reuse_ptr)
    :m_device_ptr        (device_ptr),
     m_pipeline_cache_ptr(nullptr),
     m_pipeline_counter  (0)
{
    anvil_assert(!use_pipeline_cache && pipeline_cache_to_reuse_ptr == nullptr ||
                 use_pipeline_cache);

    m_pipeline_layout_manager_ptr = Anvil::PipelineLayoutManager::acquire(m_device_ptr);

    if (pipeline_cache_to_reuse_ptr != nullptr)
    {
        m_pipeline_cache_ptr   = pipeline_cache_to_reuse_ptr;
        m_use_pipeline_cache   = true;

        pipeline_cache_to_reuse_ptr->retain();
    }
    else
    {
        if (use_pipeline_cache)
        {
            m_pipeline_cache_ptr = new Anvil::PipelineCache(m_device_ptr);
        }

        m_use_pipeline_cache = use_pipeline_cache;
    }
}

/** Please see header for specification */
Anvil::BasePipelineManager::~BasePipelineManager()
{
    anvil_assert(m_pipelines.size() == 0);

    if (m_pipeline_cache_ptr != nullptr)
    {
        m_pipeline_cache_ptr->release();

        m_pipeline_cache_ptr = nullptr;
    }

   if (m_pipeline_layout_manager_ptr != nullptr)
   {
       m_pipeline_layout_manager_ptr->release();

       m_pipeline_layout_manager_ptr = nullptr;
   }
}


/** Helper function which releases the internally managed raw Vulkan pipeline object handle.
 *
 *  The function does NOT release the layout object, since it's owned by the Pipeline Layout manager.
 *s*/
void Anvil::BasePipelineManager::Pipeline::release_vulkan_objects()
{
    if (baked_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device_ptr->get_device_vk(),
                          baked_pipeline,
                          nullptr /* pAllocator */);

        baked_pipeline = VK_NULL_HANDLE;
    }

    if (layout_ptr != nullptr)
    {
        layout_ptr->release();

        layout_ptr = nullptr;
    }
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::add_derivative_pipeline_from_sibling_pipeline(bool                               disable_optimizations,
                                                                               bool                               allow_derivatives,
                                                                               uint32_t                           n_shader_module_stage_entrypoints,
                                                                               const ShaderModuleStageEntryPoint* shader_module_stage_entrypoint_ptrs,
                                                                               PipelineID                         base_pipeline_id,
                                                                               PipelineID*                        out_pipeline_id_ptr)
{
    std::shared_ptr<Pipeline> base_pipeline_ptr;
    PipelineID                new_pipeline_id  = 0;
    std::shared_ptr<Pipeline> new_pipeline_ptr;
    bool                      result           = false;

    /* Retrieve base pipeline's descriptor */
    if (m_pipelines.size() <= base_pipeline_id)
    {
        anvil_assert(!(m_pipelines.size() <= base_pipeline_id) );

        goto end;
    }
    else
    {
        base_pipeline_ptr = m_pipelines[base_pipeline_id];

        anvil_assert(base_pipeline_ptr != nullptr);
    }

    /* Create & store the new descriptor */
    anvil_assert(base_pipeline_ptr->allow_derivatives);

    new_pipeline_id  = (m_pipeline_counter++);
    new_pipeline_ptr = std::shared_ptr<Pipeline>(new Pipeline(m_device_ptr,
                                                              disable_optimizations,
                                                              allow_derivatives,
                                                              base_pipeline_ptr,
                                                              n_shader_module_stage_entrypoints,
                                                              shader_module_stage_entrypoint_ptrs) );

    anvil_assert(m_pipelines.find(new_pipeline_id) == m_pipelines.end() );

    m_pipelines[new_pipeline_id] = new_pipeline_ptr;
    *out_pipeline_id_ptr         = new_pipeline_id;

    /* All done */
    result  = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::add_derivative_pipeline_from_pipeline(bool                               disable_optimizations,
                                                                       bool                               allow_derivatives,
                                                                       uint32_t                           n_shader_module_stage_entrypoints,
                                                                       const ShaderModuleStageEntryPoint* shader_module_stage_entrypoint_ptrs,
                                                                       VkPipeline                         base_pipeline,
                                                                       PipelineID*                        out_pipeline_id_ptr)
{
    PipelineID                new_pipeline_id  = 0;
    std::shared_ptr<Pipeline> new_pipeline_ptr;
    bool                      result = false;

    if (base_pipeline == VK_NULL_HANDLE)
    {
        anvil_assert(!(base_pipeline != VK_NULL_HANDLE));

        goto end;
    }

    /* Create & store the new descriptor */
    new_pipeline_id  = (m_pipeline_counter++);
    new_pipeline_ptr = std::shared_ptr<Pipeline>(new Pipeline(m_device_ptr,
                                                              disable_optimizations,
                                                              allow_derivatives,
                                                              base_pipeline,
                                                              n_shader_module_stage_entrypoints,
                                                              shader_module_stage_entrypoint_ptrs) );

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
bool Anvil::BasePipelineManager::add_regular_pipeline(bool                               disable_optimizations,
                                                      bool                               allow_derivatives,
                                                      uint32_t                           n_shader_module_stage_entrypoints,
                                                      const ShaderModuleStageEntryPoint* shader_module_stage_entrypoint_ptrs,
                                                      PipelineID*                        out_pipeline_id_ptr)
{
    PipelineID                new_pipeline_id  = 0;
    std::shared_ptr<Pipeline> new_pipeline_ptr;

    /* Create & store the new descriptor */
    new_pipeline_id  = (m_pipeline_counter++);
    new_pipeline_ptr = std::shared_ptr<Pipeline>(new Pipeline(m_device_ptr,
                                                              disable_optimizations,
                                                              allow_derivatives,
                                                              n_shader_module_stage_entrypoints,
                                                              shader_module_stage_entrypoint_ptrs,
                                                              false /* in_is_proxy */) );

    *out_pipeline_id_ptr         = new_pipeline_id;
    m_pipelines[new_pipeline_id] = new_pipeline_ptr;

    /* All done */
    return true;
}


bool Anvil::BasePipelineManager::add_specialization_constant_to_pipeline(PipelineID  pipeline_id,
                                                                         ShaderIndex shader_index,
                                                                         uint32_t    constant_id,
                                                                         uint32_t    n_data_bytes,
                                                                         void*       data_ptr)
{
    std::shared_ptr<Pipeline> pipeline_ptr;
    uint32_t                  data_buffer_size = 0;
    bool                      result           = false;

    if (n_data_bytes == 0)
    {
        anvil_assert(!(n_data_bytes == 0) );

        goto end;
    }

    if (data_ptr == nullptr)
    {
        anvil_assert(!(data_ptr == nullptr) );

        goto end;
    }

    /* Retrieve the pipeline's descriptor */
    if (pipeline_id >= m_pipelines.size() )
    {
        anvil_assert(!(pipeline_id >= m_pipelines.size()) );

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines[pipeline_id];
    }

    if (pipeline_ptr->is_proxy)
    {
        anvil_assert(!pipeline_ptr->is_proxy);

        goto end;
    }

    /* Append specialization constant data and add a new descriptor. */
    data_buffer_size = (uint32_t) pipeline_ptr->specialization_constant_data_buffer.size();


    anvil_assert(pipeline_ptr->specialization_constants_map.find(shader_index) != pipeline_ptr->specialization_constants_map.end() );

    pipeline_ptr->specialization_constants_map[shader_index].push_back(SpecializationConstant(constant_id,
                                                                                              n_data_bytes,
                                                                                              data_buffer_size));

    pipeline_ptr->specialization_constant_data_buffer.reserve(data_buffer_size + n_data_bytes);


    memcpy(&pipeline_ptr->specialization_constant_data_buffer[data_buffer_size],
           data_ptr,
           n_data_bytes);

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::attach_dsg_to_pipeline(PipelineID                 pipeline_id,
                                                        Anvil::DescriptorSetGroup* dsg_ptr)
{
    std::shared_ptr<Pipeline> pipeline_ptr;
    bool                      result = false;

    if (dsg_ptr == nullptr)
    {
        anvil_assert(!(dsg_ptr == nullptr) );

        goto end;
    }

    /* Retrieve pipeline's descriptor and attach the specified DSG */
    if (m_pipelines.find(pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(pipeline_id) == m_pipelines.end() ));

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines[pipeline_id];
    }

    /* Make sure the DSG has not already been attached */
    if (std::find(pipeline_ptr->descriptor_set_groups.begin(),
                  pipeline_ptr->descriptor_set_groups.end(),
                  dsg_ptr) != pipeline_ptr->descriptor_set_groups.end() )
    {
        anvil_assert(false);

        goto end;
    }

    /* If we reached this place, we can attach the DSG */
    pipeline_ptr->dirty        = true;
    pipeline_ptr->layout_dirty = true;

    pipeline_ptr->descriptor_set_groups.push_back(dsg_ptr);

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::attach_push_constant_range_to_pipeline(PipelineID         pipeline_id,
                                                                        uint32_t           offset,
                                                                        uint32_t           size,
                                                                        VkShaderStageFlags stages)
{
    std::shared_ptr<Pipeline> pipeline_ptr;
    bool                      result = false;

    /* Retrieve pipeline's descriptor and add the specified push constant range */
    if (m_pipelines.find(pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(pipeline_id) == m_pipelines.end()));

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines[pipeline_id];
    }

    if (pipeline_ptr->is_proxy)
    {
        anvil_assert(!pipeline_ptr->is_proxy);

        goto end;
    }

    pipeline_ptr->dirty        = true;
    pipeline_ptr->layout_dirty = true;

    pipeline_ptr->push_constant_ranges.push_back(Anvil::PushConstantRange(offset,
                                                                           size,
                                                                           stages) );

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
void Anvil::BasePipelineManager::bake_specialization_info_vk(const SpecializationConstants&         specialization_constants,
                                                             const unsigned char*                   specialization_constant_data_ptr,
                                                             std::vector<VkSpecializationMapEntry>* inout_specialization_map_entry_vk_vector_ptr,
                                                             VkSpecializationInfo*                  out_specialization_info_ptr)
{
    uint32_t n_specialization_constant_bytes = 0;
    uint32_t n_specialization_constants      = 0;

    /* Prepare specialization info descriptor */
    n_specialization_constants = (uint32_t) specialization_constants.size();

    inout_specialization_map_entry_vk_vector_ptr->clear  ();
    inout_specialization_map_entry_vk_vector_ptr->reserve(n_specialization_constants);

    for (uint32_t n_specialization_constant = 0;
                  n_specialization_constant < n_specialization_constants;
                ++n_specialization_constant)
    {
        const SpecializationConstant& current_specialization_constant = specialization_constants[n_specialization_constant];
        VkSpecializationMapEntry      new_entry;

        new_entry.constantID = current_specialization_constant.constant_id;
        new_entry.offset     = current_specialization_constant.start_offset;
        new_entry.size       = current_specialization_constant.n_bytes;

        n_specialization_constant_bytes += current_specialization_constant.n_bytes;

        inout_specialization_map_entry_vk_vector_ptr->push_back(new_entry);
    }

    out_specialization_info_ptr->dataSize      = n_specialization_constant_bytes;
    out_specialization_info_ptr->mapEntryCount = n_specialization_constants;
    out_specialization_info_ptr->pData         = (n_specialization_constants > 0) ? specialization_constant_data_ptr
                                                                                  : nullptr;
    out_specialization_info_ptr->pMapEntries   = (n_specialization_constants > 0) ? &(*inout_specialization_map_entry_vk_vector_ptr)[0]
                                                                                  : nullptr;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::delete_pipeline(PipelineID pipeline_id)
{
    bool result = false;

    auto pipeline_iterator = m_pipelines.find(pipeline_id);

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
VkPipeline Anvil::BasePipelineManager::get_pipeline(PipelineID pipeline_id)
{
    std::shared_ptr<Pipeline> pipeline_ptr;
    VkPipeline                result = VK_NULL_HANDLE;

    if (m_pipelines.find(pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(pipeline_id) == m_pipelines.end()) );

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines[pipeline_id];
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

    result = m_pipelines[pipeline_id]->baked_pipeline;
end:
    return result;
}

/* Please see header for specification */
Anvil::PipelineLayout* Anvil::BasePipelineManager::get_pipeline_layout(PipelineID pipeline_id)
{
    Anvil::PipelineLayout*   pipeline_layout_ptr = nullptr;
    std::shared_ptr<Pipeline> pipeline_ptr;
    Anvil::PipelineLayout*   result_ptr          = nullptr;

    if (m_pipelines.find(pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(pipeline_id) == m_pipelines.end()) );

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines[pipeline_id];
    }

    if (pipeline_ptr->is_proxy)
    {
        anvil_assert(!pipeline_ptr->is_proxy);

        goto end;
    }

    if (pipeline_ptr->layout_dirty)
    {
        if (pipeline_ptr->layout_ptr != nullptr)
        {
            pipeline_ptr->layout_ptr->release();

            pipeline_ptr->layout_ptr = nullptr;
        }

        if (!m_pipeline_layout_manager_ptr->get_retained_layout(pipeline_ptr->descriptor_set_groups,
                                                                pipeline_ptr->push_constant_ranges,
                                                               &pipeline_ptr->layout_ptr) )
        {
            anvil_assert(false);

            goto end;
        }

        pipeline_ptr->layout_dirty = false;
    }

    result_ptr = pipeline_ptr->layout_ptr;

end:
    return result_ptr;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::set_pipeline_bakeability(PipelineID pipeline_id,
                                                          bool       bakeable)
{
    std::shared_ptr<Pipeline> pipeline_ptr;
    bool                      result        = false;

    if (m_pipelines.find(pipeline_id) == m_pipelines.end() )
    {
        anvil_assert(!(m_pipelines.find(pipeline_id) == m_pipelines.end()) );

        goto end;
    }
    else
    {
        pipeline_ptr = m_pipelines[pipeline_id];
    }

    pipeline_ptr->is_bakeable = bakeable;

    result = true;
end:
    return result;
}
