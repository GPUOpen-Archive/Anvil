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
#include "wrappers/descriptor_pool.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/pipeline_layout.h"
#include <map>

#define N_PREALLOCATED_IMMUTABLE_SAMPLERS (16)


/* Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetGroup(Anvil::Device* device_ptr,
                                              bool           releaseable_sets,
                                              uint32_t       n_sets)
    :m_descriptor_pool_dirty       (false),
     m_device_ptr                  (device_ptr),
     m_layout_modifications_blocked(false),
     m_n_instantiated_sets         (0),
     m_n_sets                      (n_sets),
     m_parent_dsg_ptr              (nullptr),
     m_releaseable_sets            (releaseable_sets)
{
    anvil_assert(n_sets >= 1);

    memset(m_overhead_allocations,
           0,
           sizeof(m_overhead_allocations) );

    /* Preallocate memory for various containers */
    m_cached_immutable_samplers.resize(N_PREALLOCATED_IMMUTABLE_SAMPLERS);

    /* Initialize descriptor pool */
    m_descriptor_pool_ptr = new Anvil::DescriptorPool(device_ptr,
                                                      n_sets,
                                                      releaseable_sets);

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_DESCRIPTOR_SET_GROUP,
                                                 this);
}

/* Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetGroup(DescriptorSetGroup* parent_dsg_ptr,
                                              bool                releaseable_sets)
    :m_descriptor_pool_dirty       (true),
     m_descriptor_pool_ptr         (nullptr),
     m_device_ptr                  (parent_dsg_ptr->m_device_ptr),
     m_layout_modifications_blocked(true),
     m_n_sets                      (-1),
     m_parent_dsg_ptr              (parent_dsg_ptr),
     m_releaseable_sets            (releaseable_sets)
{
    anvil_assert(parent_dsg_ptr                                                != nullptr);
    anvil_assert(parent_dsg_ptr->m_parent_dsg_ptr                              == nullptr);
    anvil_assert(parent_dsg_ptr->m_descriptor_pool_ptr->are_sets_releaseable() == releaseable_sets);

    memset(m_overhead_allocations,
           0,
           sizeof(m_overhead_allocations) );

    /* Initialize descriptor pool */
    m_descriptor_pool_ptr = new Anvil::DescriptorPool(parent_dsg_ptr->m_device_ptr,
                                                      parent_dsg_ptr->m_n_sets,
                                                      releaseable_sets);

    /* Preallocate memory for descriptor containers */
    m_cached_immutable_samplers.resize(N_PREALLOCATED_IMMUTABLE_SAMPLERS);

    /* Configure the new DSG instance to use the specified parent DSG */
    const uint32_t n_dses = (uint32_t) parent_dsg_ptr->m_descriptor_sets.size();

    m_descriptor_sets = parent_dsg_ptr->m_descriptor_sets;

    for (auto ds : m_descriptor_sets)
    {
        m_descriptor_sets[ds.first].descriptor_set_ptr = nullptr;
    }

    /* The parent descriptor set group should be locked, so that it is no longer possible to modify
     * its descriptor set layout. Introducing support for such behavior would require significant
     * work and testing. */
    m_parent_dsg_ptr->m_layout_modifications_blocked = true;

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_DESCRIPTOR_SET_GROUP,
                                                 this);
}

/** Releases the internally managed descriptor pool. */
Anvil::DescriptorSetGroup::~DescriptorSetGroup()
{
    if (m_descriptor_pool_ptr != nullptr)
    {
        m_descriptor_pool_ptr->release();

        m_descriptor_pool_ptr = nullptr;
    }

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_DESCRIPTOR_SET_GROUP,
                                                    this);
}

/* Please see header for specification */
bool Anvil::DescriptorSetGroup::add_binding(uint32_t           n_set,
                                            uint32_t           binding,
                                            VkDescriptorType   type,
                                            uint32_t           n_elements,
                                            VkShaderStageFlags shader_stages)
{
    bool result = false;

    /* Sanity check: The DSG must not be locked. If you run into this assertion failure, you are trying
     *               to add a new binding to a descriptor set group which shares its layout with other
     *               DSGs. This modification would have invalidated layouts used by children DSGs, and
     *               currently there's no mechanism implemented to inform them about such event. */
    anvil_assert(!m_layout_modifications_blocked);

    /* Sanity check: make sure no more than the number of descriptor sets specified at creation time is
     *               used
     */
    if (m_descriptor_sets.find(n_set) == m_descriptor_sets.end() )
    {
        if (m_descriptor_sets.size() > (m_n_instantiated_sets + 1) )
        {
            anvil_assert(!(m_descriptor_sets.size() > (m_n_instantiated_sets + 1)) );

            goto end;
        }

        m_descriptor_sets[n_set].descriptor_set_ptr = nullptr;
        m_descriptor_sets[n_set].layout_ptr         = new Anvil::DescriptorSetLayout(m_device_ptr);
    }

    /* Pass the call down to DescriptorSet instance */
    result = m_descriptor_sets[n_set].layout_ptr->add_binding(binding,
                                                              type,
                                                              n_elements,
                                                              shader_stages);

    m_descriptor_pool_dirty = true;
end:
    return result;
}

/** Re-creates internally-maintained descriptor pool. **/
void Anvil::DescriptorSetGroup::bake_descriptor_pool()
{
    const uint32_t n_descriptor_sets = (uint32_t) m_descriptor_sets.size();

    anvil_assert(m_descriptor_pool_dirty);
    anvil_assert(n_descriptor_sets != 0);

    /* Count how many descriptor of what types need to have pool space allocated */
    uint32_t n_descriptors_needed_array[VK_DESCRIPTOR_TYPE_RANGE_SIZE];

    memset(n_descriptors_needed_array,
           0,
           sizeof(n_descriptors_needed_array) );

    for (auto current_ds : m_descriptor_sets)
    {
        uint32_t n_ds_bindings;

        if (current_ds.second.layout_ptr == nullptr)
        {
            continue;
        }

        n_ds_bindings = static_cast<uint32_t>(current_ds.second.layout_ptr->get_n_bindings() );

        for (uint32_t n_ds_binding = 0;
                      n_ds_binding < n_ds_bindings;
                    ++n_ds_binding)
        {
            uint32_t         ds_binding_array_size;
            VkDescriptorType ds_binding_type;

            current_ds.second.layout_ptr->get_binding_properties(n_ds_binding,
                                                                 nullptr, /* opt_out_binding_index_ptr               */
                                                                &ds_binding_type,
                                                                &ds_binding_array_size,
                                                                 nullptr,  /* opt_out_stage_flags_ptr                */
                                                                 nullptr); /* opt_out_immutable_samplers_enabled_ptr */

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

    /* The descriptor pool now matches the layout's configuration */
    m_descriptor_pool_dirty = false;
}

/* Please see header for specification */
bool Anvil::DescriptorSetGroup::bake_descriptor_sets()
{
    Anvil::DescriptorSetGroup* layout_vk_owner_ptr = (m_parent_dsg_ptr != nullptr) ? m_parent_dsg_ptr
                                                                                : this;
    uint32_t                    n_descriptors       = 0;
    const uint32_t              n_sets              = (uint32_t) m_descriptor_sets.size();
    bool                        result              = false;

    /* Copy layout descriptors to the helper vector.. */
    m_cached_ds_layouts.clear();

    for (auto ds : layout_vk_owner_ptr->m_descriptor_sets)
    {
        m_cached_ds_layouts.push_back(ds.second.layout_ptr);
    }

    /* Allocate the descriptor pool, if necessary */
    if (m_descriptor_pool_dirty)
    {
        bake_descriptor_pool();

        anvil_assert(!m_descriptor_pool_dirty);
    }

    /* Reset all previous allocations */
    m_descriptor_pool_ptr->reset();

    /* Grab descriptor sets from the pool.
     *
     * Note that baking can only occur if 1 or more bindings or sets were added. If we already have a number of
     * DescriptorSet instances cached, we need to re-use existing instances in order to retain the bindings,
     * which are cached internally by DescriptorSet instances.
     */
    if (m_descriptor_sets.begin()->second.descriptor_set_ptr != nullptr)
    {
        const uint32_t n_dses_to_assign_handles = m_n_instantiated_sets;

        anvil_assert(n_dses_to_assign_handles != 0);

        m_cached_ds.resize   (n_sets);
        m_cached_ds_vk.resize(n_dses_to_assign_handles);

        if (n_sets - m_n_instantiated_sets > 0)
        {
            auto ds = layout_vk_owner_ptr->m_descriptor_sets.begin();

            result = m_descriptor_pool_ptr->alloc_descriptor_sets(n_sets - m_n_instantiated_sets,
                                                                 &m_cached_ds_layouts[m_n_instantiated_sets],
                                                                 &m_cached_ds        [m_n_instantiated_sets]);

            for (uint32_t n_skipped_ds = 0;
                          n_skipped_ds < m_n_instantiated_sets;
                        ++n_skipped_ds)
            {
                ds ++;
            }

            for (uint32_t n_set = m_n_instantiated_sets;
                          n_set < n_sets;
                        ++n_set, ++ds)
            {
                ds->second.descriptor_set_ptr = m_cached_ds[n_set];
            }
        }
        else
        {
            result = true;
        }

        result &= m_descriptor_pool_ptr->alloc_descriptor_sets(n_dses_to_assign_handles,
                                                              &m_cached_ds_layouts[0],
                                                              &m_cached_ds_vk[0]);
        anvil_assert(result);

        /* Assign the allocated DSes to relevant descriptor set descriptors */
        auto ds_iterator = m_descriptor_sets.begin();

        for (uint32_t n_set = 0;
                      n_set < m_n_instantiated_sets;
                    ++n_set, ++ds_iterator)
        {
            anvil_assert(m_cached_ds[n_set]                     != nullptr);
            anvil_assert(ds_iterator->second.descriptor_set_ptr != nullptr);

            ds_iterator->second.descriptor_set_ptr->set_new_vk_handle(m_cached_ds_vk[n_set]);
        }
    }
    else
    {
        auto ds_iterator = m_descriptor_sets.begin();

        m_cached_ds.resize(n_sets);

        /* Allocate everything from scratch */
        result = m_descriptor_pool_ptr->alloc_descriptor_sets(n_sets,
                                                             &m_cached_ds_layouts[0],
                                                             &m_cached_ds[0]);
        anvil_assert(result);

        for (uint32_t n_set = 0;
                      n_set < n_sets;
                    ++n_set, ++ds_iterator)
        {
            anvil_assert(m_cached_ds[n_set]                     != nullptr);
            anvil_assert(ds_iterator->second.descriptor_set_ptr == nullptr);

            ds_iterator->second.descriptor_set_ptr = m_cached_ds[n_set];
        }

        m_n_instantiated_sets = n_sets;
    }

    /* All done */
    result = true;

    return result;
}

/* Please see header for specification */
Anvil::DescriptorSet* Anvil::DescriptorSetGroup::get_descriptor_set(uint32_t n_set)
{
    bool pool_rebaked = false;

    anvil_assert(m_descriptor_sets.find(n_set) != m_descriptor_sets.end() );

    if (m_descriptor_pool_dirty)
    {
        bake_descriptor_pool();

        anvil_assert(!m_descriptor_pool_dirty);

        pool_rebaked = true;
    }

    if (pool_rebaked                                           ||
        m_descriptor_sets[n_set].descriptor_set_ptr == nullptr)
    {
        bake_descriptor_sets();

        anvil_assert(m_descriptor_sets[n_set].descriptor_set_ptr != nullptr);
    }

    return m_descriptor_sets[n_set].descriptor_set_ptr;
}

/* Please see header for specification */
uint32_t Anvil::DescriptorSetGroup::get_descriptor_set_binding_index(uint32_t n_set) const
{
    std::map<uint32_t, DescriptorSetInfo>::const_iterator dsg_iterator = m_descriptor_sets.begin();

    anvil_assert(m_descriptor_sets.size() > n_set);

    for (uint32_t n_current_set = 0;
                  n_current_set < n_set;
                ++n_current_set, ++dsg_iterator)
    {
        /* Stub */
    }

    return dsg_iterator->first;
}

/* Please see header for specification */
Anvil::DescriptorSetLayout* Anvil::DescriptorSetGroup::get_descriptor_set_layout(uint32_t n_set)
{
    if (m_parent_dsg_ptr != nullptr)
    {
        return m_parent_dsg_ptr->get_descriptor_set_layout(n_set);
    }
    else
    {
        anvil_assert(m_descriptor_sets.find(n_set)       != m_descriptor_sets.end() );
        anvil_assert(m_descriptor_sets[n_set].layout_ptr != nullptr)

        return m_descriptor_sets[n_set].layout_ptr;
    }

}

/* Please see header for specification */
void Anvil::DescriptorSetGroup::set_descriptor_pool_overhead_allocations(VkDescriptorType descriptor_type,
                                                                          uint32_t         n_overhead_allocations)
{
    anvil_assert(descriptor_type < VK_DESCRIPTOR_TYPE_RANGE_SIZE);

    if (m_overhead_allocations[descriptor_type] != n_overhead_allocations)
    {
        m_overhead_allocations[descriptor_type] = n_overhead_allocations;
        m_descriptor_pool_dirty                 = true;
    }
}

/** Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetInfo::DescriptorSetInfo(const DescriptorSetInfo& in)
{
    descriptor_set_ptr = in.descriptor_set_ptr;
    layout_ptr         = in.layout_ptr;

    if (descriptor_set_ptr != nullptr)
    {
        descriptor_set_ptr->retain();
    }

    if (layout_ptr != nullptr)
    {
        layout_ptr->retain();
    }
}

/** Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetInfo& Anvil::DescriptorSetGroup::DescriptorSetInfo::operator=(const DescriptorSetInfo& in)
{
    descriptor_set_ptr = in.descriptor_set_ptr;
    layout_ptr         = in.layout_ptr;

    if (descriptor_set_ptr != nullptr)
    {
        descriptor_set_ptr->retain();
    }

    if (layout_ptr != nullptr)
    {
        layout_ptr->retain();
    }

    return *this;
}

/** Please see header for specification */
Anvil::DescriptorSetGroup::DescriptorSetInfo::~DescriptorSetInfo()
{
    if (descriptor_set_ptr != nullptr)
    {
        descriptor_set_ptr->release();

        descriptor_set_ptr = nullptr;
    }

    if (layout_ptr != nullptr)
    {
        layout_ptr->release();

        layout_ptr = nullptr;
    }
}