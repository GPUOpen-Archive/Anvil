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
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/device.h"


/* Please see header for specification */
Anvil::DescriptorPool::DescriptorPool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                      uint32_t                         in_n_max_sets,
                                      bool                             in_releaseable_sets)
    :CallbacksSupportProvider  (DESCRIPTOR_POOL_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT),
     m_baked                   (false),
     m_device_ptr              (in_device_ptr),
     m_n_max_sets              (in_n_max_sets),
     m_pool                    (VK_NULL_HANDLE),
     m_releaseable_sets        (in_releaseable_sets)
{
    memset(m_descriptor_count,
           0,
           sizeof(m_descriptor_count) );

    /* Register the object in the Object Tracker */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_DESCRIPTOR_POOL,
                                                 this);
}

/* Please see header for specification */
Anvil::DescriptorPool::~DescriptorPool()
{
    /* Unregister the instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_DESCRIPTOR_POOL,
                                                    this);

    if (m_pool != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        vkDestroyDescriptorPool(device_locked_ptr->get_device_vk(),
                                m_pool,
                                nullptr /* pAllocator */);

        m_pool = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
bool Anvil::DescriptorPool::alloc_descriptor_sets(uint32_t                                     in_n_sets,
                                                  std::shared_ptr<Anvil::DescriptorSetLayout>* in_descriptor_set_layouts_ptr,
                                                  std::shared_ptr<Anvil::DescriptorSet>*       out_descriptor_sets_ptr,
                                                  VkResult*                                    out_opt_result_ptr)
{
    bool result = false;

    m_ds_cache.resize(in_n_sets);

    result = alloc_descriptor_sets(in_n_sets,
                                   in_descriptor_set_layouts_ptr,
                                  &m_ds_cache.at(0),
                                   out_opt_result_ptr);

    if (result)
    {
        for (uint32_t n_set = 0;
                      n_set < in_n_sets;
                    ++n_set)
        {
            out_descriptor_sets_ptr[n_set] = Anvil::DescriptorSet::create(m_device_ptr,
                                                                          shared_from_this(),
                                                                          in_descriptor_set_layouts_ptr[n_set],
                                                                          m_ds_cache[n_set]);
        }
    }

    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorPool::alloc_descriptor_sets(uint32_t                                     in_n_sets,
                                                  std::shared_ptr<Anvil::DescriptorSetLayout>* in_descriptor_set_layouts_ptr,
                                                  VkDescriptorSet*                             out_descriptor_sets_vk_ptr,
                                                  VkResult*                                    out_opt_result_ptr)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkDescriptorSetAllocateInfo        ds_alloc_info;
    VkResult                           result_vk;

    if (!m_baked)
    {
        bake();

        anvil_assert(m_baked);
    }

    m_ds_layout_cache.resize(in_n_sets);

    for (uint32_t n_set = 0;
                  n_set < in_n_sets;
                ++n_set)
    {
        m_ds_layout_cache[n_set] = in_descriptor_set_layouts_ptr[n_set]->get_layout();
    }

    ds_alloc_info.descriptorPool     = m_pool;
    ds_alloc_info.descriptorSetCount = in_n_sets;
    ds_alloc_info.pNext              = nullptr;
    ds_alloc_info.pSetLayouts        = &m_ds_layout_cache[0];
    ds_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

    result_vk = vkAllocateDescriptorSets(device_locked_ptr->get_device_vk(),
                                        &ds_alloc_info,
                                         out_descriptor_sets_vk_ptr);

    if (out_opt_result_ptr != nullptr)
    {
        *out_opt_result_ptr = result_vk;
    }

    return is_vk_call_successful(result_vk);
}

/* Please see header for specification */
void Anvil::DescriptorPool::bake()
{
    VkDescriptorPoolCreateInfo         descriptor_pool_create_info;
    VkDescriptorPoolSize               descriptor_pool_sizes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr      (m_device_ptr);
    uint32_t                           n_descriptor_types_used(0);
    VkResult                           result_vk              (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    if (m_pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device_locked_ptr->get_device_vk(),
                                m_pool,
                                nullptr /* pAllocator */);

        set_vk_handle(VK_NULL_HANDLE);
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
                                                                     : 0u;
    descriptor_pool_create_info.maxSets       = m_n_max_sets;
    descriptor_pool_create_info.pNext         = nullptr;
    descriptor_pool_create_info.poolSizeCount = n_descriptor_types_used;
    descriptor_pool_create_info.pPoolSizes    = descriptor_pool_sizes;
    descriptor_pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

    result_vk = vkCreateDescriptorPool(device_locked_ptr->get_device_vk(),
                                      &descriptor_pool_create_info,
                                       nullptr, /* pAllocator */
                                      &m_pool);

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk) )
    {
        set_vk_handle(m_pool);
    }

    m_baked = true;
}

/* Please see header for specification */
std::shared_ptr<Anvil::DescriptorPool> Anvil::DescriptorPool::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                     uint32_t                         in_n_max_sets,
                                                                     bool                             in_releaseable_sets)
{
    std::shared_ptr<Anvil::DescriptorPool> result_ptr;

    result_ptr.reset(
        new Anvil::DescriptorPool(in_device_ptr,
                                  in_n_max_sets,
                                  in_releaseable_sets)
    );

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::DescriptorPool::reset()
{
    VkResult result_vk;

    if (m_pool != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        result_vk = vkResetDescriptorPool(device_locked_ptr->get_device_vk(),
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