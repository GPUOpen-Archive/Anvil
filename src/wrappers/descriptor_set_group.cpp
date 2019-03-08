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
#include "misc/descriptor_pool_create_info.h"
#include "misc/object_tracker.h"
#include "wrappers/descriptor_pool.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/descriptor_set_layout_manager.h"
#include "wrappers/device.h"
#include "wrappers/pipeline_layout.h"
#include <map>

/* Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetGroup(const Anvil::BaseDevice*                             in_device_ptr,
                                              std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> in_ds_create_info_ptrs,
                                              bool                                                 in_releaseable_sets,
                                              MTSafety                                             in_mt_safety,
                                              const std::vector<OverheadAllocation>&               in_opt_overhead_allocations,
                                              const Anvil::DescriptorPoolCreateFlags&              in_opt_pool_extra_flags)
    :MTSafetySupportProvider    (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                 in_device_ptr) ),
     m_device_ptr               (in_device_ptr),
     m_n_unique_dses            (0),
     m_parent_dsg_ptr           (nullptr),
     m_releaseable_sets         (in_releaseable_sets),
     m_user_specified_pool_flags(in_opt_pool_extra_flags)
{
    auto ds_layout_manager_ptr = m_device_ptr->get_descriptor_set_layout_manager();

    for (const auto& overhead_alloc : in_opt_overhead_allocations)
    {
        m_descriptor_type_properties[overhead_alloc.descriptor_type].n_overhead_allocations = overhead_alloc.n_overhead_allocations;
    }

    /* Initialize descriptor pool */
    m_n_unique_dses = static_cast<uint32_t>(in_ds_create_info_ptrs.size() );

    for (uint32_t n_layout_info_ptr = 0;
                  n_layout_info_ptr < static_cast<uint32_t>(in_ds_create_info_ptrs.size() );
                ++n_layout_info_ptr)
    {
        auto& current_layout_info_ptr = in_ds_create_info_ptrs.at(n_layout_info_ptr);

        m_descriptor_sets[n_layout_info_ptr].reset(
            new DescriptorSetInfoContainer()
        );

        if (current_layout_info_ptr != nullptr)
        {
            if (ds_layout_manager_ptr != nullptr)
            {
                ds_layout_manager_ptr->get_layout(current_layout_info_ptr.get(),
                                                 &m_descriptor_sets.at(n_layout_info_ptr)->layout_ptr);

                anvil_assert(m_descriptor_sets.at(n_layout_info_ptr)->layout_ptr != nullptr);
            }
            else
            {
                /* Required at device bring-up time. */
                m_descriptor_sets.at(n_layout_info_ptr)->layout_ptr = Anvil::DescriptorSetLayout::create(std::move(current_layout_info_ptr),
                                                                                                         in_device_ptr,
                                                                                                         in_mt_safety);
            }

            m_ds_create_info_ptrs.push_back(m_descriptor_sets.at(n_layout_info_ptr)->layout_ptr->get_create_info() );
        }
        else
        {
            m_ds_create_info_ptrs.push_back(nullptr);
        }
    }

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::ANVIL_DESCRIPTOR_SET_GROUP,
                                                 this);
}

/* Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetGroup(const DescriptorSetGroup* in_parent_dsg_ptr,
                                              bool                      in_releaseable_sets)
    :MTSafetySupportProvider    (in_parent_dsg_ptr->is_mt_safe() ),
     m_device_ptr               (in_parent_dsg_ptr->m_device_ptr),
     m_parent_dsg_ptr           (in_parent_dsg_ptr),
     m_releaseable_sets         (in_releaseable_sets),
     m_user_specified_pool_flags(in_parent_dsg_ptr->m_user_specified_pool_flags)
{
    auto descriptor_set_layout_manager_ptr = m_device_ptr->get_descriptor_set_layout_manager();

    anvil_assert(  in_parent_dsg_ptr->m_parent_dsg_ptr                                                                                                                       == nullptr);
    anvil_assert(((in_parent_dsg_ptr->m_descriptor_pool_ptr->get_create_info_ptr()->get_create_flags() & Anvil::DescriptorPoolCreateFlagBits::FREE_DESCRIPTOR_SET_BIT) != 0) == in_releaseable_sets);

    m_descriptor_type_properties = in_parent_dsg_ptr->m_descriptor_type_properties;

    for (auto& current_descriptor_type_props : m_descriptor_type_properties)
    {
        current_descriptor_type_props.second.n_overhead_allocations = 0;
    }

    /* Initialize descriptor pool */
    {
        auto     dp_create_info_ptr = Anvil::DescriptorPoolCreateInfo::create(in_parent_dsg_ptr->m_device_ptr,
                                                                              in_parent_dsg_ptr->m_descriptor_pool_ptr->get_create_info_ptr()->get_n_maximum_sets(),
                                                                              in_parent_dsg_ptr->m_descriptor_pool_ptr->get_create_info_ptr()->get_create_flags  (),
                                                                              Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ));
        uint32_t total_pool_size    = 0;

        for (const auto& current_descriptor_type_props : m_descriptor_type_properties)
        {
            dp_create_info_ptr->set_n_descriptors_for_descriptor_type(current_descriptor_type_props.first,
                                                                      current_descriptor_type_props.second.pool_size);

            total_pool_size += current_descriptor_type_props.second.pool_size;
        }

        if (total_pool_size == 0)
        {
            /* Request space for anything. This is required for correct dummy DSG support, as zero-sized pools are forbidden by the spec. */
            anvil_assert(dp_create_info_ptr->get_n_descriptors_for_descriptor_type(Anvil::DescriptorType::SAMPLER) == 0);

            dp_create_info_ptr->set_n_descriptors_for_descriptor_type(Anvil::DescriptorType::SAMPLER,
                                                                      1);
        }

        m_descriptor_pool_ptr = Anvil::DescriptorPool::create(std::move(dp_create_info_ptr) );
    }

    /* Configure the new DSG instance to use the specified parent DSG */
    for (const auto& ds : in_parent_dsg_ptr->m_descriptor_sets)
    {
        m_descriptor_sets[ds.first].reset(
            new DescriptorSetInfoContainer()
        );

        if (ds.second->layout_ptr != nullptr)
        {
            /* NOTE: We must use pipeline layout manager to acquire a clone of the input layout, instead of wrapping
             *       the raw pointer inside a customized std::unique_ptr. Reason for this is the descriptor set layout manager
             *       manually reference-counts users of each instantiated layout, and the counter is only bumped at get_layout()
             *       call time.
             */
            descriptor_set_layout_manager_ptr->get_layout(ds.second->layout_ptr->get_create_info(),
                                                         &m_descriptor_sets.at(ds.first)->layout_ptr);

            anvil_assert(m_descriptor_sets.at(ds.first)->layout_ptr.get() == ds.second->layout_ptr.get() );
        }
    }

    m_n_unique_dses = in_parent_dsg_ptr->m_n_unique_dses;

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::ANVIL_DESCRIPTOR_SET_GROUP,
                                                 this);
}

/** Releases the internally managed descriptor pool. */
Anvil::DescriptorSetGroup::~DescriptorSetGroup()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::ANVIL_DESCRIPTOR_SET_GROUP,
                                                    this);
}

/** Re-creates internally-maintained descriptor pool. **/
bool Anvil::DescriptorSetGroup::bake_descriptor_pool()
{
    Anvil::DescriptorPoolCreateFlags                                                                    flags                    = ((m_releaseable_sets) ? Anvil::DescriptorPoolCreateFlagBits::FREE_DESCRIPTOR_SET_BIT : Anvil::DescriptorPoolCreateFlagBits::NONE);
    std::unique_lock<std::recursive_mutex>                                                              mutex_lock;
    auto                                                                                                mutex_ptr                = get_mutex();
    std::unordered_map<Anvil::DescriptorType, uint32_t, Anvil::EnumClassHasher<Anvil::DescriptorType> > n_descriptors_needed_map;
    bool                                                                                                result                   = false;

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
    for (auto& current_ds : m_descriptor_sets)
    {
        const Anvil::DescriptorSetCreateInfo* current_ds_create_info_ptr        = nullptr;
        uint32_t                              n_ds_bindings;
        uint32_t                              variable_descriptor_binding_index = UINT32_MAX;
        uint32_t                              variable_descriptor_binding_size  = 0;

        if (current_ds.second->layout_ptr == nullptr)
        {
            current_ds_create_info_ptr = m_device_ptr->get_dummy_descriptor_set_layout()->get_create_info();
        }
        else
        {
            current_ds_create_info_ptr = current_ds.second->layout_ptr->get_create_info();
        }

        n_ds_bindings = static_cast<uint32_t>(current_ds_create_info_ptr->get_n_bindings() );

        current_ds_create_info_ptr->contains_variable_descriptor_count_binding(&variable_descriptor_binding_index,
                                                                               &variable_descriptor_binding_size);

        for (uint32_t n_ds_binding = 0;
                      n_ds_binding < n_ds_bindings;
                    ++n_ds_binding)
        {
            uint32_t                      ds_binding_array_size;
            Anvil::DescriptorBindingFlags ds_binding_flags;
            uint32_t                      ds_binding_index     = UINT32_MAX;
            Anvil::DescriptorType         ds_binding_type      = Anvil::DescriptorType::UNKNOWN;

            current_ds_create_info_ptr->get_binding_properties_by_index_number(n_ds_binding,
                                                                              &ds_binding_index,
                                                                              &ds_binding_type,
                                                                              &ds_binding_array_size,
                                                                               nullptr,  /* out_opt_stage_flags_ptr                */
                                                                               nullptr,  /* out_opt_immutable_samplers_enabled_ptr */
                                                                              &ds_binding_flags);

            if (ds_binding_index == variable_descriptor_binding_index)
            {
                ds_binding_array_size = variable_descriptor_binding_size;
            }

            if ((ds_binding_flags & Anvil::DescriptorBindingFlagBits::UPDATE_AFTER_BIND_BIT) != 0)
            {
                flags |= Anvil::DescriptorPoolCreateFlagBits::UPDATE_AFTER_BIND_BIT;
            }

            n_descriptors_needed_map[ds_binding_type] += ds_binding_array_size;
        }
    }

    for (auto& current_map_entry : n_descriptors_needed_map)
    {
        current_map_entry.second += m_descriptor_type_properties[current_map_entry.first].n_overhead_allocations;
    }

    /* Verify we can actually create the pool.. */
    if ((flags & Anvil::DescriptorPoolCreateFlagBits::UPDATE_AFTER_BIND_BIT) != 0)
    {
        if (!m_device_ptr->get_extension_info()->ext_descriptor_indexing() )
        {
            anvil_assert(m_device_ptr->get_extension_info()->ext_descriptor_indexing() );

            goto end;
        }
    }

    /* Create the pool */
    {
        auto dp_create_info_ptr = Anvil::DescriptorPoolCreateInfo::create(m_device_ptr,
                                                                          m_n_unique_dses,
                                                                          flags | m_user_specified_pool_flags,
                                                                          Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ));

        for (const auto& current_n_descriptors_needed_map_entry : n_descriptors_needed_map)
        {
            dp_create_info_ptr->set_n_descriptors_for_descriptor_type(current_n_descriptors_needed_map_entry.first,
                                                                      current_n_descriptors_needed_map_entry.second);
        }

        m_descriptor_pool_ptr = Anvil::DescriptorPool::create(std::move(dp_create_info_ptr) );
    }

    if (m_descriptor_pool_ptr == nullptr)
    {
        anvil_assert(m_descriptor_pool_ptr != nullptr);

        goto end;
    }

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSetGroup::bake_descriptor_sets()
{
    std::vector<Anvil::DescriptorSetAllocation> allocations;
    std::vector<DescriptorSetUniquePtr>         dses;
    const Anvil::DescriptorSetGroup*            layout_vk_owner_ptr = (m_parent_dsg_ptr != nullptr) ? m_parent_dsg_ptr
                                                                                                    : this;
    std::unique_lock<std::recursive_mutex>      mutex_lock;
    auto                                        mutex_ptr           = get_mutex();
    bool                                        result              = false;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    anvil_assert(m_descriptor_pool_ptr != nullptr);


    /* Copy layout descriptors to the helper vector.. */
    const auto n_sets = static_cast<uint32_t>(m_descriptor_sets.size() );

    for (const auto& ds_data : layout_vk_owner_ptr->m_descriptor_sets)
    {
        const auto& ds_ptr = ds_data.second;

        if (ds_data.second->layout_ptr == nullptr)
        {
            allocations.push_back(
                Anvil::DescriptorSetAllocation(m_device_ptr->get_dummy_descriptor_set_layout() )
            );
        }
        else
        {
            bool     has_variable_descriptor_count_binding   = false;
            uint32_t variable_descriptor_count_binding_index = 0;
            uint32_t variable_descriptor_count_binding_size  = 0;

            has_variable_descriptor_count_binding = ds_ptr->layout_ptr->get_create_info()->contains_variable_descriptor_count_binding(&variable_descriptor_count_binding_index,
                                                                                                                                      &variable_descriptor_count_binding_size);

            if (has_variable_descriptor_count_binding)
            {
                anvil_assert(variable_descriptor_count_binding_size != 0);

                allocations.push_back(
                    Anvil::DescriptorSetAllocation(ds_ptr->layout_ptr.get(),
                                                   variable_descriptor_count_binding_size)
                );
            }
            else
            {
                allocations.push_back(
                    Anvil::DescriptorSetAllocation(ds_ptr->layout_ptr.get() )
                );
            }
        }
    }

    /* Reset all previous allocations */
    m_descriptor_pool_ptr->reset();

    /* Grab descriptor sets from the pool. */
    auto ds_iterator = m_descriptor_sets.begin();

    dses.resize(n_sets);

    /* Allocate everything from scratch */
    result = m_descriptor_pool_ptr->alloc_descriptor_sets(n_sets,
                                                         &allocations.at(0),
                                                         &dses.at       (0) );
    anvil_assert(result);

    for (uint32_t n_set = 0;
                  n_set < n_sets;
                ++n_set, ++ds_iterator)
    {
        anvil_assert(dses[n_set]                             != nullptr);
        anvil_assert(ds_iterator->second->descriptor_set_ptr == nullptr);

        ds_iterator->second->descriptor_set_ptr = std::move(dses.at(n_set) );
    }

    /* All done */
    result = true;

    return result;
}

/* Please see header for specification */
Anvil::DescriptorSetGroupUniquePtr Anvil::DescriptorSetGroup::create(const Anvil::BaseDevice*                              in_device_ptr,
                                                                     std::vector<Anvil::DescriptorSetCreateInfoUniquePtr>& in_ds_create_info_ptrs,
                                                                     bool                                                  in_releaseable_sets,
                                                                     MTSafety                                              in_mt_safety,
                                                                     const std::vector<OverheadAllocation>&                in_opt_overhead_allocations,
                                                                     const Anvil::DescriptorPoolCreateFlags&               in_opt_pool_extra_flags)
{
    Anvil::DescriptorSetGroupUniquePtr result_ptr(nullptr,
                                                  std::default_delete<Anvil::DescriptorSetGroup>() );

    result_ptr.reset(
        new Anvil::DescriptorSetGroup(in_device_ptr,
                                      std::move(in_ds_create_info_ptrs),
                                      in_releaseable_sets,
                                      in_mt_safety,
                                      in_opt_overhead_allocations,
                                      in_opt_pool_extra_flags)
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
Anvil::DescriptorSetGroupUniquePtr Anvil::DescriptorSetGroup::create(const Anvil::DescriptorSetGroup* in_parent_dsg_ptr,
                                                                     bool                             in_releaseable_sets)
{
    Anvil::DescriptorSetGroupUniquePtr result_ptr(nullptr,
                                                  std::default_delete<Anvil::DescriptorSetGroup>() );

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
Anvil::DescriptorSet* Anvil::DescriptorSetGroup::get_descriptor_set(uint32_t in_n_set)
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

    return ds_iterator->second->descriptor_set_ptr.get();
}

const std::vector<const Anvil::DescriptorSetCreateInfo*>* Anvil::DescriptorSetGroup::get_descriptor_set_create_info() const
{
    std::unique_lock<std::recursive_mutex>                    mutex_lock;
    auto                                                      mutex_ptr  = get_mutex();
    const std::vector<const Anvil::DescriptorSetCreateInfo*>* result_ptr = nullptr;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    result_ptr = &m_ds_create_info_ptrs;

    return result_ptr;
}

/* Please see header for specification */
const Anvil::DescriptorSetCreateInfo* Anvil::DescriptorSetGroup::get_descriptor_set_create_info(uint32_t in_n_set) const
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr   = get_mutex();
    const Anvil::DescriptorSetCreateInfo*  result_ptr  = nullptr;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    if (m_ds_create_info_ptrs.size() < in_n_set)
    {
        anvil_assert_fail();

        goto end;
    }

    result_ptr = m_ds_create_info_ptrs.at(in_n_set);

end:
    return result_ptr;
}

/* Please see header for specification */
Anvil::DescriptorSetLayout* Anvil::DescriptorSetGroup::get_descriptor_set_layout(uint32_t in_n_set) const
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
            anvil_assert(ds_iterator->second->layout_ptr != nullptr)

            return ds_iterator->second->layout_ptr.get();
        }
        else
        {
            return nullptr;
        }
    }
}

/** Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetInfoContainer::~DescriptorSetInfoContainer()
{
    /* Stub */
}