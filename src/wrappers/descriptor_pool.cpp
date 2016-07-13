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
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/device.h"


/* Please see header for specification */
Anvil::DescriptorPool::DescriptorPool(Anvil::Device* device_ptr,
                                      uint32_t       n_max_sets,
                                      bool           releaseable_sets)
    :CallbacksSupportProvider(DESCRIPTOR_POOL_CALLBACK_ID_COUNT),
     m_baked           (false),
     m_device_ptr      (device_ptr),
     m_n_max_sets      (n_max_sets),
     m_pool            (VK_NULL_HANDLE),
     m_releaseable_sets(releaseable_sets)
{
    anvil_assert(device_ptr != nullptr);

    memset(m_descriptor_count,
           0,
           sizeof(m_descriptor_count) );

    /* Register the object in the Object Tracker */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_DESCRIPTOR_POOL,
                                                  this);
}

/* Please see header for specification */
Anvil::DescriptorPool::~DescriptorPool()
{
    if (m_pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_device_ptr->get_device_vk(),
                                m_pool,
                                nullptr /* pAllocator */);

        m_pool = VK_NULL_HANDLE;
    }

    /* Unregister the instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_DESCRIPTOR_POOL,
                                                    this);
}

/* Please see header for specification */
bool Anvil::DescriptorPool::alloc_descriptor_sets(uint32_t                     n_sets,
                                                  Anvil::DescriptorSetLayout** descriptor_set_layouts_ptr,
                                                  Anvil::DescriptorSet**       out_descriptor_sets_ptr)
{
    bool result = false;

    m_ds_cache.resize(n_sets);

    result = alloc_descriptor_sets(n_sets,
                                   descriptor_set_layouts_ptr,
                                  &m_ds_cache[0]);

    if (result)
    {
        for (uint32_t n_set = 0;
                      n_set < n_sets;
                    ++n_set)
        {
            out_descriptor_sets_ptr[n_set] = new Anvil::DescriptorSet(m_device_ptr,
                                                                       this, /* parent_pool_ptr */
                                                                       descriptor_set_layouts_ptr[n_set],
                                                                       m_ds_cache[n_set]);
        }
    }

    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorPool::alloc_descriptor_sets(uint32_t                     n_sets,
                                                  Anvil::DescriptorSetLayout** descriptor_set_layouts_ptr,
                                                  VkDescriptorSet*             out_descriptor_sets_vk_ptr)
{
    VkDescriptorSetAllocateInfo ds_alloc_info;
    VkResult                    result_vk;

    m_ds_layout_cache.resize(n_sets);

    for (uint32_t n_set = 0;
                  n_set < n_sets;
                ++n_set)
    {
        m_ds_layout_cache[n_set] = descriptor_set_layouts_ptr[n_set]->get_layout();
    }

    ds_alloc_info.descriptorPool     = m_pool;
    ds_alloc_info.descriptorSetCount = n_sets;
    ds_alloc_info.pNext              = nullptr;
    ds_alloc_info.pSetLayouts        = &m_ds_layout_cache[0];
    ds_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

    result_vk = vkAllocateDescriptorSets(m_device_ptr->get_device_vk(),
                                        &ds_alloc_info,
                                         out_descriptor_sets_vk_ptr);

    anvil_assert_vk_call_succeeded(result_vk);
    return is_vk_call_successful(result_vk);
}

/* Please see header for specification */
void Anvil::DescriptorPool::bake()
{
    VkDescriptorPoolCreateInfo descriptor_pool_create_info;
    VkDescriptorPoolSize       descriptor_pool_sizes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
    uint32_t                   n_descriptor_types_used = 0;
    VkResult                   result_vk;

    if (m_pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_device_ptr->get_device_vk(),
                                m_pool,
                                nullptr /* pAllocator */);

        m_pool = VK_NULL_HANDLE;
    }

    /* Convert the counters to an arrayed, linear representation */
    for (uint32_t n_descriptor_type = 0;
                  n_descriptor_type < VK_DESCRIPTOR_TYPE_RANGE_SIZE;
                ++n_descriptor_type)
    {
        if (m_descriptor_count[n_descriptor_type] > 0)
        {
            uint32_t current_index = n_descriptor_types_used;

            descriptor_pool_sizes[current_index].descriptorCount = m_descriptor_count[n_descriptor_type];
            descriptor_pool_sizes[current_index].type            = (VkDescriptorType) n_descriptor_type;

            n_descriptor_types_used++;
        }
    }

    if (n_descriptor_types_used == 0)
    {
        /* If an empty pool is needed, request space for a single dummy descriptor. */
        descriptor_pool_sizes[0].descriptorCount = 1;
        descriptor_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_SAMPLER;

        n_descriptor_types_used = 1;
    }

    /* Set up the descriptor pool instance */
    descriptor_pool_create_info.flags         = (m_releaseable_sets) ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
                                                                     : 0;
    descriptor_pool_create_info.maxSets       = m_n_max_sets;
    descriptor_pool_create_info.pNext         = nullptr;
    descriptor_pool_create_info.poolSizeCount = n_descriptor_types_used;
    descriptor_pool_create_info.pPoolSizes    = descriptor_pool_sizes;
    descriptor_pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

    result_vk = vkCreateDescriptorPool(m_device_ptr->get_device_vk(),
                                      &descriptor_pool_create_info,
                                       nullptr, /* pAllocator */
                                      &m_pool);

    anvil_assert_vk_call_succeeded(result_vk);

    m_baked = true;
}

/* Please see header for specification */
bool Anvil::DescriptorPool::reset()
{
    VkResult result_vk;

    if (m_pool != VK_NULL_HANDLE)
    {
        result_vk = vkResetDescriptorPool(m_device_ptr->get_device_vk(),
                                          m_pool,
                                          0 /* flags */);
        anvil_assert_vk_call_succeeded(result_vk);

        if (is_vk_call_successful(result_vk) )
        {
            /* Alloced descriptor sets went out of scope. Send out a call-back, so that descriptor set
             * wrapper instances can mark themselves as unusable */
            callback(DESCRIPTOR_POOL_CALLBACK_ID_POOL_RESET,
                     this);
        }
    }
    else
    {
        result_vk = VK_SUCCESS;
    }

    return is_vk_call_successful(result_vk); 
}