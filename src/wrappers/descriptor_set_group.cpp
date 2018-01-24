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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/descriptor_pool.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/pipeline_layout.h"
#include <map>

/* Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetGroup(std::weak_ptr<Anvil::BaseDevice>                        in_device_ptr,
                                              std::vector<std::unique_ptr<Anvil::DescriptorSetInfo> > in_ds_info_ptrs,
                                              bool                                                    in_releaseable_sets,
                                              MTSafety                                                in_mt_safety,
                                              const std::vector<OverheadAllocation>&                  in_opt_overhead_allocations)
    :MTSafetySupportProvider(Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                             in_device_ptr) ),
     m_device_ptr           (in_device_ptr),
     m_ds_info_ptrs         (std::move(in_ds_info_ptrs) ),
     m_releaseable_sets     (in_releaseable_sets)
{
    memset(m_overhead_allocations,
           0,
           sizeof(m_overhead_allocations) );

    for (const auto& overhead_alloc : in_opt_overhead_allocations)
    {
        m_overhead_allocations[overhead_alloc.descriptor_type] = overhead_alloc.n_overhead_allocations;
    }

    /* Initialize descriptor pool */
    uint32_t n_unique_dses = 0;

    for (uint32_t n_layout_info_ptr = 0;
                  n_layout_info_ptr < static_cast<uint32_t>(m_ds_info_ptrs.size() );
                ++n_layout_info_ptr)
    {
        auto& current_layout_info_ptr = m_ds_info_ptrs.at(n_layout_info_ptr);

        if (current_layout_info_ptr == nullptr)
        {
            continue;
        }

        n_unique_dses                                  += (current_layout_info_ptr != nullptr) ? 1 : 0;
        m_descriptor_sets[n_layout_info_ptr].layout_ptr = Anvil::DescriptorSetLayout::create(std::move(current_layout_info_ptr),
                                                                                             in_device_ptr,
                                                                                             in_mt_safety);
    }

    m_descriptor_pool_ptr = Anvil::DescriptorPool::create(in_device_ptr,
                                                          n_unique_dses,
                                                          in_releaseable_sets,
                                                          in_mt_safety);

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_DESCRIPTOR_SET_GROUP,
                                                 this);
}

/* Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetGroup(std::shared_ptr<DescriptorSetGroup> in_parent_dsg_ptr,
                                              bool                                in_releaseable_sets)
    :MTSafetySupportProvider(in_parent_dsg_ptr->is_mt_safe() ),
     m_device_ptr           (in_parent_dsg_ptr->m_device_ptr),
     m_parent_dsg_ptr       (in_parent_dsg_ptr),
     m_releaseable_sets     (in_releaseable_sets)
{
    anvil_assert(in_parent_dsg_ptr                                                != nullptr);
    anvil_assert(in_parent_dsg_ptr->m_parent_dsg_ptr                              == nullptr);
    anvil_assert(in_parent_dsg_ptr->m_descriptor_pool_ptr->are_sets_releaseable() == in_releaseable_sets);

    memset(m_overhead_allocations,
           0,
           sizeof(m_overhead_allocations) );

    /* Initialize descriptor pool */
    m_descriptor_pool_ptr = Anvil::DescriptorPool::create(in_parent_dsg_ptr->m_device_ptr,
                                                          in_parent_dsg_ptr->m_descriptor_pool_ptr->get_n_maximum_sets(),
                                                          in_releaseable_sets,
                                                          Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ));

    /* Configure the new DSG instance to use the specified parent DSG */
    m_descriptor_sets = in_parent_dsg_ptr->m_descriptor_sets;

    for (auto ds : m_descriptor_sets)
    {
        m_descriptor_sets[ds.first].descriptor_set_ptr = nullptr;
    }

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_DESCRIPTOR_SET_GROUP,
                                                 this);
}

/** Releases the internally managed descriptor pool. */
Anvil::DescriptorSetGroup::~DescriptorSetGroup()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_DESCRIPTOR_SET_GROUP,
                                                    this);
}

/** Re-creates internally-maintained descriptor pool. **/
void Anvil::DescriptorSetGroup::bake_descriptor_pool()
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr      = get_mutex();

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    if (m_descriptor_sets.size() == 0)
    {
        goto end;
    }

    /* Count how many descriptor of what types need to have pool space allocated */
    uint32_t n_descriptors_needed_array[VK_DESCRIPTOR_TYPE_RANGE_SIZE];

    memset(n_descriptors_needed_array,
           0,
           sizeof(n_descriptors_needed_array) );

    for (auto current_ds : m_descriptor_sets)
    {
        auto     current_ds_layout_info_ptr = current_ds.second.layout_ptr->get_info();
        uint32_t n_ds_bindings;

        if (current_ds.second.layout_ptr == nullptr)
        {
            continue;
        }

        n_ds_bindings = static_cast<uint32_t>(current_ds_layout_info_ptr->get_n_bindings() );

        for (uint32_t n_ds_binding = 0;
                      n_ds_binding < n_ds_bindings;
                    ++n_ds_binding)
        {
            uint32_t         ds_binding_array_size;
            VkDescriptorType ds_binding_type;

            current_ds_layout_info_ptr->get_binding_properties(n_ds_binding,
                                                               nullptr, /* out_opt_binding_index_ptr               */
                                                              &ds_binding_type,
                                                              &ds_binding_array_size,
                                                               nullptr,  /* out_opt_stage_flags_ptr                */
                                                               nullptr); /* out_opt_immutable_samplers_enabled_ptr */

            n_descriptors_needed_array[ds_binding_type] += ds_binding_array_size;
        }
    }

    /* Configure the underlying descriptor pool wrapper */
    for (uint32_t n_descriptor_type = 0;
                  n_descriptor_type < VK_DESCRIPTOR_TYPE_RANGE_SIZE;
                ++n_descriptor_type)
    {
        m_descriptor_pool_ptr->set_descriptor_array_size(static_cast<VkDescriptorType>(n_descriptor_type),
                                                         n_descriptors_needed_array[n_descriptor_type] + m_overhead_allocations[n_descriptor_type]);
    }

    m_descriptor_pool_ptr->bake();

end:
    ;
}

/* Please see header for specification */
bool Anvil::DescriptorSetGroup::bake_descriptor_sets()
{
    std::vector<std::shared_ptr<Anvil::DescriptorSet> >       dses;
    std::vector<std::shared_ptr<Anvil::DescriptorSetLayout> > ds_layouts;
    const Anvil::DescriptorSetGroup*                          layout_vk_owner_ptr = (m_parent_dsg_ptr != nullptr) ? m_parent_dsg_ptr.get()
                                                                                                                  : this;
    std::unique_lock<std::recursive_mutex>                    mutex_lock;
    auto                                                      mutex_ptr           = get_mutex();
    bool                                                      result              = false;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    anvil_assert(m_descriptor_pool_ptr != nullptr);


    /* Copy layout descriptors to the helper vector.. */
    const uint32_t n_sets = static_cast<uint32_t>(m_descriptor_sets.size() );

    for (auto ds : layout_vk_owner_ptr->m_descriptor_sets)
    {
        ds_layouts.push_back(ds.second.layout_ptr);
    }

    /* Reset all previous allocations */
    m_descriptor_pool_ptr->reset();

    /* Grab descriptor sets from the pool. */
    auto ds_iterator = m_descriptor_sets.begin();

    dses.resize(n_sets);

    /* Allocate everything from scratch */
    result = m_descriptor_pool_ptr->alloc_descriptor_sets(n_sets,
                                                         &ds_layouts[0],
                                                         &dses      [0]);
    anvil_assert(result);

    for (uint32_t n_set = 0;
                  n_set < n_sets;
                ++n_set, ++ds_iterator)
    {
        anvil_assert(dses[n_set]                            != nullptr);
        anvil_assert(ds_iterator->second.descriptor_set_ptr == nullptr);

        ds_iterator->second.descriptor_set_ptr = dses[n_set];
    }

    /* All done */
    result = true;

    return result;
}

/* Please see header for specification */
std::shared_ptr<Anvil::DescriptorSetGroup> Anvil::DescriptorSetGroup::create(std::weak_ptr<Anvil::BaseDevice>                         in_device_ptr,
                                                                             std::vector<std::unique_ptr<Anvil::DescriptorSetInfo> >& in_ds_info_ptrs,
                                                                             bool                                                     in_releaseable_sets,
                                                                             MTSafety                                                 in_mt_safety,
                                                                             const std::vector<OverheadAllocation>&                   in_opt_overhead_allocations)
{
    std::shared_ptr<Anvil::DescriptorSetGroup> result_ptr;

    result_ptr.reset(
        new Anvil::DescriptorSetGroup(in_device_ptr,
                                      std::move(in_ds_info_ptrs),
                                      in_releaseable_sets,
                                      in_mt_safety,
                                      in_opt_overhead_allocations)
    );

    if (result_ptr != nullptr)
    {
        result_ptr->bake_descriptor_pool();

        if (!result_ptr->bake_descriptor_sets() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::DescriptorSetGroup> Anvil::DescriptorSetGroup::create(std::shared_ptr<Anvil::DescriptorSetGroup> in_parent_dsg_ptr,
                                                                             bool                                       in_releaseable_sets)
{
    std::shared_ptr<Anvil::DescriptorSetGroup> result_ptr;

    result_ptr.reset(
        new Anvil::DescriptorSetGroup(in_parent_dsg_ptr,
                                      in_releaseable_sets)
    );

    if (result_ptr != nullptr)
    {
        result_ptr->bake_descriptor_pool();

        if (!result_ptr->bake_descriptor_sets() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::DescriptorSet> Anvil::DescriptorSetGroup::get_descriptor_set(uint32_t in_n_set) const
{
    decltype(m_descriptor_sets)::const_iterator ds_iterator;
    std::unique_lock<std::recursive_mutex>      mutex_lock;
    auto                                        mutex_ptr    = get_mutex();

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    ds_iterator = m_descriptor_sets.find(in_n_set); 
    anvil_assert(ds_iterator != m_descriptor_sets.end() );

    return ds_iterator->second.descriptor_set_ptr;
}

/* Please see header for specification */
std::shared_ptr<Anvil::DescriptorSetLayout> Anvil::DescriptorSetGroup::get_descriptor_set_layout(uint32_t in_n_set) const
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr    = get_mutex();

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }


    if (m_parent_dsg_ptr != nullptr)
    {
        return m_parent_dsg_ptr->get_descriptor_set_layout(in_n_set);
    }
    else
    {
        auto ds_iterator = m_descriptor_sets.find(in_n_set);

        if (ds_iterator != m_descriptor_sets.end() )
        {
            anvil_assert(ds_iterator->second.layout_ptr != nullptr)

            return ds_iterator->second.layout_ptr;
        }
        else
        {
            return nullptr;
        }
    }

}

/** Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetInfo::DescriptorSetInfo(const DescriptorSetInfo& in)
{
    descriptor_set_ptr = in.descriptor_set_ptr;
    layout_ptr         = in.layout_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetInfo& Anvil::DescriptorSetGroup::DescriptorSetInfo::operator=(const DescriptorSetInfo& in)
{
    descriptor_set_ptr = in.descriptor_set_ptr;
    layout_ptr         = in.layout_ptr;

    return *this;
}

/** Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetInfo::~DescriptorSetInfo()
{
    /* Stub */
}