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

#include "misc/base_pipeline_create_info.h"
#include "misc/base_pipeline_manager.h"
#include "misc/debug.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/device.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/pipeline_layout_manager.h"
#include "wrappers/pipeline_cache.h"
#include <algorithm>

/** Please see header for specification */
Anvil::BasePipelineManager::BasePipelineManager(const Anvil::BaseDevice* in_device_ptr,
                                                bool                     in_mt_safe,
                                                bool                     in_use_pipeline_cache,
                                                Anvil::PipelineCache*    in_pipeline_cache_to_reuse_ptr)
    :CallbacksSupportProvider(BASE_PIPELINE_MANAGER_CALLBACK_ID_COUNT),
     MTSafetySupportProvider (in_mt_safe),
     m_device_ptr            (in_device_ptr),
     m_pipeline_cache_ptr    (nullptr),
     m_pipeline_counter      (0)
{
    anvil_assert((!in_use_pipeline_cache && in_pipeline_cache_to_reuse_ptr == nullptr) ||
                   in_use_pipeline_cache);

    m_pipeline_layout_manager_ptr = in_device_ptr->get_pipeline_layout_manager();
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
            m_pipeline_cache_owned_ptr = Anvil::PipelineCache::create(m_device_ptr,
                                                                      in_mt_safe,
                                                                      0,        /* in_initial_data_size */
                                                                      nullptr); /* in_initial_data      */

            m_pipeline_cache_ptr = m_pipeline_cache_owned_ptr.get();
        }

        m_use_pipeline_cache = in_use_pipeline_cache;
    }
}

/** Please see header for specification */
Anvil::BasePipelineManager::~BasePipelineManager()
{
    anvil_assert(m_baked_pipelines.size() == 0);
}


/** Helper function which releases the internally managed raw Vulkan pipeline object handle.
 *
 *  The function does NOT release the layout object, since it's owned by the Pipeline Layout manager.
 *s*/
void Anvil::BasePipelineManager::Pipeline::release_pipeline()
{
    if (baked_pipeline != VK_NULL_HANDLE)
    {
        device_ptr->get_pipeline_cache()->lock();
        lock();
        {
            Anvil::Vulkan::vkDestroyPipeline(device_ptr->get_device_vk(),
                                             baked_pipeline,
                                             nullptr /* pAllocator */);
        }
        unlock();
        device_ptr->get_pipeline_cache()->unlock();

        baked_pipeline = VK_NULL_HANDLE;
    }
}


/* Please see header for specification */
bool Anvil::BasePipelineManager::add_pipeline(Anvil::BasePipelineCreateInfoUniquePtr in_pipeline_create_info_ptr,
                                              PipelineID*                            out_pipeline_id_ptr)
{
    const Anvil::PipelineID                base_pipeline_id = in_pipeline_create_info_ptr->get_base_pipeline_id();
    auto                                   callback_arg     = Anvil::OnNewPipelineCreatedCallbackData(UINT32_MAX);
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr        = get_mutex();
    PipelineID                             new_pipeline_id  = 0;
    std::unique_ptr<Pipeline>              new_pipeline_ptr;
    bool                                   result           = false;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    if (base_pipeline_id != UINT32_MAX)
    {
        Anvil::BasePipelineCreateInfo* base_pipeline_create_info_ptr = nullptr;
        auto                           base_pipeline_iterator        = m_baked_pipelines.find(base_pipeline_id);

        if (base_pipeline_iterator == m_baked_pipelines.end() )
        {
            base_pipeline_iterator = m_outstanding_pipelines.find(base_pipeline_id);

            if (base_pipeline_iterator != m_outstanding_pipelines.end() )
            {
                base_pipeline_create_info_ptr = base_pipeline_iterator->second->pipeline_create_info_ptr.get();
            }
        }
        else
        {
            base_pipeline_create_info_ptr = base_pipeline_iterator->second->pipeline_create_info_ptr.get();
        }

        if (base_pipeline_create_info_ptr != nullptr)
        {
            anvil_assert(base_pipeline_create_info_ptr->allows_derivatives() );
        }
        else
        {
            /* Base pipeline ID is invalid */
            anvil_assert(base_pipeline_create_info_ptr != nullptr);

            goto end;
        }
    }

    /* Create & store the new descriptor */
    new_pipeline_id = (m_pipeline_counter.fetch_add(1) );

    /* NOTE: in_pipeline_create_info_ptr becomes NULL after the call below */
    new_pipeline_ptr.reset(
        new Pipeline(
            m_device_ptr,
            std::move(in_pipeline_create_info_ptr),
            is_mt_safe() )
    );

    if (new_pipeline_ptr->pipeline_create_info_ptr->is_proxy() )
    {
        m_baked_pipelines[new_pipeline_id] = std::move(new_pipeline_ptr);
    }
    else
    {
        m_outstanding_pipelines[new_pipeline_id] = std::move(new_pipeline_ptr);
    }

    *out_pipeline_id_ptr = new_pipeline_id;

    /* Inform subscribers about the new pipeline. */
    callback_arg.new_pipeline_id = new_pipeline_id;

    callback(BASE_PIPELINE_MANAGER_CALLBACK_ID_ON_NEW_PIPELINE_CREATED,
            &callback_arg);

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
void Anvil::BasePipelineManager::bake_specialization_info_vk(const SpecializationConstants&         in_specialization_constants,
                                                             const unsigned char*                   in_specialization_constant_data_ptr,
                                                             std::vector<VkSpecializationMapEntry>* out_specialization_map_entry_vk_vector_ptr,
                                                             VkSpecializationInfo*                  out_specialization_info_ptr) const
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
    bool result = false;

    {
        std::unique_lock<std::recursive_mutex> mutex_lock;
        auto                                   mutex_ptr         = get_mutex();
        Pipelines::iterator                    pipeline_iterator;

        if (mutex_ptr != nullptr)
        {
            mutex_lock = std::move(
                std::unique_lock<std::recursive_mutex>(*mutex_ptr)
            );
        }

        pipeline_iterator = m_baked_pipelines.find(in_pipeline_id);

        if (pipeline_iterator != m_baked_pipelines.end() )
        {
            m_baked_pipelines.erase(pipeline_iterator);
        }
        else
        {
            pipeline_iterator = m_outstanding_pipelines.find(in_pipeline_id);

            if (pipeline_iterator == m_outstanding_pipelines.end() )
            {
                goto end;
            }

            m_outstanding_pipelines.erase(pipeline_iterator);
        }
    }

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
VkPipeline Anvil::BasePipelineManager::get_pipeline(PipelineID in_pipeline_id)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr         = get_mutex();
    Pipelines::const_iterator              pipeline_iterator;
    Pipeline*                              pipeline_ptr      = nullptr;
    VkPipeline                             result            = VK_NULL_HANDLE;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    if (m_outstanding_pipelines.size() > 0)
    {
        bake();
    }

    pipeline_iterator = m_baked_pipelines.find(in_pipeline_id);

    if (pipeline_iterator == m_baked_pipelines.end() )
    {
        anvil_assert(!(pipeline_iterator == m_baked_pipelines.end()) );

        goto end;
    }

    pipeline_ptr = pipeline_iterator->second.get();

    if (pipeline_ptr->pipeline_create_info_ptr->is_proxy() )
    {
        anvil_assert(!pipeline_ptr->pipeline_create_info_ptr->is_proxy() );

        goto end;
    }

    result = pipeline_ptr->baked_pipeline;
    anvil_assert(result != VK_NULL_HANDLE);
end:
    return result;
}

const Anvil::BasePipelineCreateInfo* Anvil::BasePipelineManager::get_pipeline_create_info(PipelineID in_pipeline_id) const
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr         = get_mutex();
    Pipelines::const_iterator              pipeline_iterator;
    Pipeline*                              pipeline_ptr      = nullptr;
    const Anvil::BasePipelineCreateInfo*   result_ptr        = nullptr;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    pipeline_iterator = m_baked_pipelines.find(in_pipeline_id);

    if (pipeline_iterator == m_baked_pipelines.end() )
    {
        pipeline_iterator = m_outstanding_pipelines.find(in_pipeline_id);

        if (pipeline_iterator == m_outstanding_pipelines.end() )
        {
            anvil_assert(!(pipeline_iterator == m_outstanding_pipelines.end() ));

            goto end;
        }
    }

    pipeline_ptr = pipeline_iterator->second.get();
    result_ptr   = pipeline_ptr->pipeline_create_info_ptr.get();

end:
    return result_ptr;
}

/* Please see header for specification */
Anvil::PipelineLayout* Anvil::BasePipelineManager::get_pipeline_layout(PipelineID in_pipeline_id)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr         = get_mutex();
    Pipelines::iterator                    pipeline_iterator;
    Pipeline*                              pipeline_ptr      = nullptr;
    Anvil::PipelineLayout*                 result_ptr        = nullptr;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    pipeline_iterator = m_baked_pipelines.find(in_pipeline_id);

    if (pipeline_iterator == m_baked_pipelines.end() )
    {
        pipeline_iterator = m_outstanding_pipelines.find(in_pipeline_id);

        if (pipeline_iterator == m_outstanding_pipelines.end() )
        {
            anvil_assert(!(pipeline_iterator == m_outstanding_pipelines.end() ));

            goto end;
        }
    }

    pipeline_ptr = pipeline_iterator->second.get();

    if (pipeline_ptr->pipeline_create_info_ptr->is_proxy() )
    {
        anvil_assert(!pipeline_ptr->pipeline_create_info_ptr->is_proxy() );

        goto end;
    }

    if (pipeline_ptr->layout_ptr == nullptr)
    {
        if (!m_pipeline_layout_manager_ptr->get_layout(pipeline_ptr->pipeline_create_info_ptr->get_ds_create_info_items(),
                                                       pipeline_ptr->pipeline_create_info_ptr->get_push_constant_ranges(),
                                                      &pipeline_ptr->layout_ptr) )
        {
            anvil_assert_fail();

            goto end;
        }

        if (pipeline_ptr->layout_ptr == nullptr)
        {
            anvil_assert(!(pipeline_ptr->layout_ptr == nullptr) );

            goto end;
        }
    }

    result_ptr = pipeline_ptr->layout_ptr.get();

end:
    return result_ptr;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::get_shader_info(PipelineID                  in_pipeline_id,
                                                 Anvil::ShaderStage          in_shader_stage,
                                                 Anvil::ShaderInfoType       in_info_type,
                                                 std::vector<unsigned char>* out_data_ptr)
{
    Anvil::ExtensionAMDShaderInfoEntrypoints entrypoints       = m_device_ptr->get_extension_amd_shader_info_entrypoints();
    std::unique_lock<std::recursive_mutex>   mutex_lock;
    auto                                     mutex_ptr         = get_mutex();
    Pipelines::const_iterator                pipeline_iterator;
    Pipeline*                                pipeline_ptr      = nullptr;
    size_t                                   out_data_size     = out_data_ptr->size();
    bool                                     result            = false;
    const auto                               shader_stage_vk   = Anvil::Utils::get_shader_stage_flag_bits_from_shader_stage(in_shader_stage);
    VkShaderInfoTypeAMD                      vk_info_type;
    VkResult                                 vk_result;

    if (entrypoints.vkGetShaderInfoAMD == nullptr)
    {
        anvil_assert(!(entrypoints.vkGetShaderInfoAMD == nullptr));

        goto end;
    }

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    if (m_outstanding_pipelines.size() > 0)
    {
        bake();
    }

    pipeline_iterator = m_baked_pipelines.find(in_pipeline_id);
    if (pipeline_iterator == m_baked_pipelines.end())
    {
        anvil_assert(!(pipeline_iterator == m_baked_pipelines.end()));

        goto end;
    }

    pipeline_ptr = pipeline_iterator->second.get();

    if (pipeline_ptr->baked_pipeline == VK_NULL_HANDLE)
    {
        bake();

        anvil_assert(!pipeline_ptr->baked_pipeline != VK_NULL_HANDLE);
    }

    switch (in_info_type)
    {
        case ShaderInfoType::BINARY:
        {
            vk_info_type = VK_SHADER_INFO_TYPE_BINARY_AMD;

            break;
        }

        case ShaderInfoType::DISASSEMBLY:
        {
            vk_info_type = VK_SHADER_INFO_TYPE_DISASSEMBLY_AMD;

            break;
        }

        default:
        {
            anvil_assert(!"Unknown shader info type");

            goto end;
        }
    }

    if (out_data_size == 0)
    {
        vk_result = entrypoints.vkGetShaderInfoAMD(m_device_ptr->get_device_vk(),
                                                   pipeline_ptr->baked_pipeline,
                                                   static_cast<VkShaderStageFlagBits>(shader_stage_vk),
                                                   vk_info_type,
                                                  &out_data_size,
                                                   nullptr);

        if (vk_result == VK_ERROR_FEATURE_NOT_PRESENT)
        {
            goto end;
        }

        if (!is_vk_call_successful(vk_result)       ||
             out_data_size                    == 0)
        {
            goto end;
        }

        out_data_ptr->resize(out_data_size);
    }

    vk_result = entrypoints.vkGetShaderInfoAMD(m_device_ptr->get_device_vk(),
                                               pipeline_ptr->baked_pipeline,
                                               static_cast<VkShaderStageFlagBits>(shader_stage_vk),
                                               vk_info_type,
                                              &out_data_size,
                                              &(*out_data_ptr).at(0));

    if (vk_result == VK_ERROR_FEATURE_NOT_PRESENT)
    {
        goto end;
    }

    if (!is_vk_call_successful(vk_result)      ||
         out_data_size                    == 0)
    {
        goto end;
    }

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::BasePipelineManager::get_shader_statistics(PipelineID                  in_pipeline_id,
                                                       Anvil::ShaderStage          in_shader_stage,
                                                       VkShaderStatisticsInfoAMD*  out_shader_statistics_ptr)
{
    Anvil::ExtensionAMDShaderInfoEntrypoints entrypoints            = m_device_ptr->get_extension_amd_shader_info_entrypoints();
    std::unique_lock<std::recursive_mutex>   mutex_lock;
    auto                                     mutex_ptr              = get_mutex();
    Pipelines::const_iterator                pipeline_iterator;
    Pipeline*                                pipeline_ptr           = nullptr;
    bool                                     result                 = false;
    const auto                               shader_stage_vk        = Anvil::Utils::get_shader_stage_flag_bits_from_shader_stage(in_shader_stage);
    size_t                                   shader_statistics_size = sizeof(VkShaderStatisticsInfoAMD);
    VkResult                                 vk_result;

    if (entrypoints.vkGetShaderInfoAMD == nullptr)
    {
        anvil_assert(!(entrypoints.vkGetShaderInfoAMD == nullptr));

        goto end;
    }

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    if (m_outstanding_pipelines.size() > 0)
    {
        bake();
    }

    pipeline_iterator = m_baked_pipelines.find(in_pipeline_id);
    if (pipeline_iterator == m_baked_pipelines.end())
    {
        anvil_assert(!(pipeline_iterator == m_baked_pipelines.end()));

        goto end;
    }

    pipeline_ptr = pipeline_iterator->second.get();

    if (pipeline_ptr->baked_pipeline == VK_NULL_HANDLE)
    {
        bake();

        anvil_assert(!pipeline_ptr->baked_pipeline != VK_NULL_HANDLE);
    }

    vk_result = entrypoints.vkGetShaderInfoAMD(m_device_ptr->get_device_vk(),
                                               pipeline_ptr->baked_pipeline,
                                               static_cast<VkShaderStageFlagBits>(shader_stage_vk),
                                               VK_SHADER_INFO_TYPE_STATISTICS_AMD,
                                              &shader_statistics_size,
                                               out_shader_statistics_ptr);

    if (vk_result == VK_ERROR_FEATURE_NOT_PRESENT)
    {
        goto end;
    }

    if (!is_vk_call_successful(vk_result)                                      ||
         shader_statistics_size           != sizeof(VkShaderStatisticsInfoAMD))
    {
        goto end;
    }

    result = true;

end:
    return result;
}
