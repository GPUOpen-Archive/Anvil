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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/pipeline_layout_manager.h"
#include <algorithm>


/** Constructor. */
Anvil::PipelineLayoutManager::PipelineLayoutManager(const Anvil::BaseDevice* in_device_ptr,
                                                    bool                     in_mt_safe)
    :MTSafetySupportProvider(in_mt_safe),
     m_device_ptr           (in_device_ptr)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::ANVIL_PIPELINE_LAYOUT_MANAGER,
                                                  this);
}

/** Destructor */
Anvil::PipelineLayoutManager::~PipelineLayoutManager()
{
    /* If this assertion check explodes, your app has not released all pipelines it has created. */
    anvil_assert(m_pipeline_layouts.size() == 0);

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::ANVIL_PIPELINE_LAYOUT_MANAGER,
                                                    this);
}

/* Please see header for specification */
Anvil::PipelineLayoutManagerUniquePtr Anvil::PipelineLayoutManager::create(const Anvil::BaseDevice* in_device_ptr,
                                                                           bool                     in_mt_safe)
{
    PipelineLayoutManagerUniquePtr result_ptr(nullptr,
                                              std::default_delete<PipelineLayoutManager>() );

    result_ptr.reset(
        new Anvil::PipelineLayoutManager(in_device_ptr,
                                         in_mt_safe)
    );

    anvil_assert(result_ptr != nullptr);
    return result_ptr;
}

/* Please see header for specification */
bool Anvil::PipelineLayoutManager::get_layout(const std::vector<DescriptorSetCreateInfoUniquePtr>* in_ds_create_info_items_ptr,
                                              const PushConstantRanges&                            in_push_constant_ranges,
                                              Anvil::PipelineLayoutUniquePtr*                      out_pipeline_layout_ptr_ptr)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr                   = get_mutex();
    const uint32_t                         n_descriptor_sets_in_in_dsg = static_cast<uint32_t>(in_ds_create_info_items_ptr->size() );
    bool                                   result                      = false;
    Anvil::PipelineLayout*                 result_pipeline_layout_ptr  = nullptr;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    for (auto layout_iterator  = m_pipeline_layouts.begin();
              layout_iterator != m_pipeline_layouts.end();
            ++layout_iterator)
    {
        auto&      current_pipeline_layout_container_ptr     = *layout_iterator;
        auto&      current_pipeline_layout_ptr               = current_pipeline_layout_container_ptr->pipeline_layout_ptr;
        auto       current_pipeline_ds_create_info_ptrs      = current_pipeline_layout_ptr->get_ds_create_info_ptrs();
        bool       dss_match                                 = true;
        const auto n_descriptor_sets_in_current_pipeline_dsg = static_cast<uint32_t>(current_pipeline_ds_create_info_ptrs->size() );

        if (n_descriptor_sets_in_current_pipeline_dsg != n_descriptor_sets_in_in_dsg)
        {
            continue;
        }

        if (current_pipeline_layout_ptr->get_attached_push_constant_ranges() != in_push_constant_ranges)
        {
            continue;
        }

        for (uint32_t n_ds = 0;
                      n_ds < n_descriptor_sets_in_in_dsg && dss_match;
                    ++n_ds)
        {
            auto&       in_dsg_ds_create_info_ptr               = in_ds_create_info_items_ptr->at         (n_ds);
            const auto& current_pipeline_dsg_ds_create_info_ptr = current_pipeline_ds_create_info_ptrs->at(n_ds);

            if ((in_dsg_ds_create_info_ptr != nullptr && current_pipeline_dsg_ds_create_info_ptr == nullptr) ||
                (in_dsg_ds_create_info_ptr == nullptr && current_pipeline_dsg_ds_create_info_ptr != nullptr) )
            {
                dss_match = false;

                break;
            }

            if (in_dsg_ds_create_info_ptr               != nullptr &&
                current_pipeline_dsg_ds_create_info_ptr != nullptr)
            {
                if (!(*in_dsg_ds_create_info_ptr == *current_pipeline_dsg_ds_create_info_ptr) )
                {
                    dss_match = false;

                    break;
                }
            }
        }

        if (!dss_match)
        {
            continue;
        }

        result                       = true;
        result_pipeline_layout_ptr   = current_pipeline_layout_container_ptr->pipeline_layout_ptr.get();

        current_pipeline_layout_container_ptr->n_references.fetch_add(1);

        break;
    }

    if (!result)
    {
        auto new_layout_ptr           = Anvil::PipelineLayout::create(m_device_ptr,
                                                                      in_ds_create_info_items_ptr,
                                                                      in_push_constant_ranges,
                                                                      is_mt_safe() );
        auto new_layout_container_ptr = std::unique_ptr<PipelineLayoutContainer>(
            new PipelineLayoutContainer()
        );

        result                                        = true;
        result_pipeline_layout_ptr                    = new_layout_ptr.get();
        new_layout_container_ptr->pipeline_layout_ptr = std::move(new_layout_ptr);

        m_pipeline_layouts.push_back(
            std::move(new_layout_container_ptr)
        );
    }

    if (result)
    {
        anvil_assert(result_pipeline_layout_ptr != nullptr);

        *out_pipeline_layout_ptr_ptr = Anvil::PipelineLayoutUniquePtr(result_pipeline_layout_ptr,
                                                                      std::bind(&PipelineLayoutManager::on_pipeline_layout_dereferenced,
                                                                                this,
                                                                                result_pipeline_layout_ptr)
        );
    }

    return result;
}

void Anvil::PipelineLayoutManager::on_pipeline_layout_dereferenced(Anvil::PipelineLayout* in_layout_ptr)
{
    bool                                   has_found  = false;
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    for (auto layout_iterator  = m_pipeline_layouts.begin();
              layout_iterator != m_pipeline_layouts.end()    && !has_found;
            ++layout_iterator)
    {
        auto& current_pipeline_layout_container_ptr = *layout_iterator;
        auto& current_pipeline_layout_ptr           = current_pipeline_layout_container_ptr->pipeline_layout_ptr;

        if (current_pipeline_layout_ptr.get() == in_layout_ptr)
        {
            has_found = true;

            if (current_pipeline_layout_container_ptr->n_references.fetch_sub(1) == 1)
            {
                m_pipeline_layouts.erase(layout_iterator);
            }

            break;
        }
    }

    anvil_assert(has_found);
}