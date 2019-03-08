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
#include "misc/descriptor_set_create_info.h"
#include "misc/object_tracker.h"
#include "misc/struct_chainer.h"
#include "wrappers/descriptor_pool.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/device.h"


/* Please see header for specification */
Anvil::DescriptorPool::DescriptorPool(Anvil::DescriptorPoolCreateInfoUniquePtr in_create_info_ptr,
                                      bool                                     in_mt_safe)
    :CallbacksSupportProvider  (DESCRIPTOR_POOL_CALLBACK_ID_COUNT),
     DebugMarkerSupportProvider(in_create_info_ptr->get_device_ptr(),
                                Anvil::ObjectType::DESCRIPTOR_POOL),
     MTSafetySupportProvider   (in_mt_safe),
     m_pool                    (VK_NULL_HANDLE)
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    /* Register the object in the Object Tracker */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::DESCRIPTOR_POOL,
                                                 this);
}

/* Please see header for specification */
Anvil::DescriptorPool::~DescriptorPool()
{
    /* Unregister the instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::DESCRIPTOR_POOL,
                                                    this);

    if (m_pool != VK_NULL_HANDLE)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyDescriptorPool(m_device_ptr->get_device_vk(),
                                                   m_pool,
                                                   nullptr /* pAllocator */);
        }
        unlock();

        m_pool = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
bool Anvil::DescriptorPool::alloc_descriptor_sets(uint32_t                       in_n_sets,
                                                  const DescriptorSetAllocation* in_ds_allocations_ptr,
                                                  DescriptorSetUniquePtr*        out_descriptor_sets_ptr,
                                                  VkResult*                      out_opt_result_ptr)
{
    bool result = false;

    m_ds_cache.resize(in_n_sets);

    result = alloc_descriptor_sets(in_n_sets,
                                   in_ds_allocations_ptr,
                                  &m_ds_cache.at(0),
                                   out_opt_result_ptr);

    if (result)
    {
        for (uint32_t n_set = 0;
                      n_set < in_n_sets;
                    ++n_set)
        {
            out_descriptor_sets_ptr[n_set] = Anvil::DescriptorSet::create(m_device_ptr,
                                                                          this,
                                                                          in_ds_allocations_ptr[n_set].ds_layout_ptr,
                                                                          m_ds_cache[n_set],
                                                                          Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe()) );

            if (out_descriptor_sets_ptr[n_set] == nullptr)
            {
                anvil_assert_fail();

                result = false;
                goto end;
            }
        }
    }

end:
    if (!result)
    {
        for (uint32_t n_set = 0;
                      n_set < in_n_sets;
                    ++n_set)
        {
            out_descriptor_sets_ptr[n_set].reset();
        }
    }

    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorPool::alloc_descriptor_sets(uint32_t                       in_n_sets,
                                                  const DescriptorSetAllocation* in_ds_allocations_ptr,
                                                  VkDescriptorSet*               out_descriptor_sets_vk_ptr,
                                                  VkResult*                      out_opt_result_ptr)
{
    const auto&                                       dp_create_flags                              (m_create_info_ptr->get_create_flags() );
    bool                                              result                                       (false);
    VkResult                                          result_vk;
    bool                                              should_chain_variable_descriptor_count_struct(false);
    Anvil::StructChainer<VkDescriptorSetAllocateInfo> struct_chainer;
    std::vector<uint32_t>                             variable_descriptor_counts;

    lock();
    {
        m_ds_layout_cache.resize(in_n_sets);

        for (uint32_t n_set = 0;
                      n_set < in_n_sets;
                    ++n_set)
        {
            if (in_ds_allocations_ptr[n_set].ds_layout_ptr != nullptr)
            {
                auto ds_create_info_ptr = in_ds_allocations_ptr[n_set].ds_layout_ptr->get_create_info();

                if (ds_create_info_ptr->contains_variable_descriptor_count_binding() )
                {
                    if ((dp_create_flags & Anvil::DescriptorPoolCreateFlagBits::UPDATE_AFTER_BIND_BIT) != 0)
                    {
                        anvil_assert_fail();

                        result = false;
                        goto end;
                    }

                    should_chain_variable_descriptor_count_struct = true;

                    variable_descriptor_counts.push_back(in_ds_allocations_ptr[n_set].n_variable_descriptor_bindings);
                }
                else
                {
                    variable_descriptor_counts.push_back(0);
                }

                m_ds_layout_cache[n_set] = in_ds_allocations_ptr[n_set].ds_layout_ptr->get_layout();
            }
            else
            {
                /* This is a "gap" set. */
                m_ds_layout_cache[n_set] = m_device_ptr->get_dummy_descriptor_set_layout()->get_layout();
            }
        }

        {
            VkDescriptorSetAllocateInfo ds_alloc_info;

            ds_alloc_info.descriptorPool     = m_pool;
            ds_alloc_info.descriptorSetCount = in_n_sets;
            ds_alloc_info.pNext              = nullptr;
            ds_alloc_info.pSetLayouts        = &m_ds_layout_cache.at(0);
            ds_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

            struct_chainer.append_struct(ds_alloc_info);
        }

        if (should_chain_variable_descriptor_count_struct)
        {
            VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variable_descriptor_count_struct;

            anvil_assert(variable_descriptor_counts.size() == in_n_sets);

            variable_descriptor_count_struct.descriptorSetCount = in_n_sets;
            variable_descriptor_count_struct.pDescriptorCounts  = &variable_descriptor_counts.at(0);
            variable_descriptor_count_struct.pNext              = nullptr;
            variable_descriptor_count_struct.sType              = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT);

            struct_chainer.append_struct(variable_descriptor_count_struct);
        }

        {
            auto chain_ptr = struct_chainer.create_chain();

            result_vk = Anvil::Vulkan::vkAllocateDescriptorSets(m_device_ptr->get_device_vk(),
                                                                chain_ptr->get_root_struct(),
                                                                out_descriptor_sets_vk_ptr);
        }
    }
    unlock();

    if (out_opt_result_ptr != nullptr)
    {
        *out_opt_result_ptr = result_vk;
    }

    result = is_vk_call_successful(result_vk);
end:
    return result;
}

/* Please see header for specification */
Anvil::DescriptorPoolUniquePtr Anvil::DescriptorPool::create(Anvil::DescriptorPoolCreateInfoUniquePtr in_create_info_ptr)
{
    const bool              is_mt_safe = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety (),
                                                                                         in_create_info_ptr->get_device_ptr() );
    DescriptorPoolUniquePtr result_ptr(nullptr,
                                       std::default_delete<Anvil::DescriptorPool>() );

    result_ptr.reset(
        new Anvil::DescriptorPool(std::move(in_create_info_ptr),
                                  is_mt_safe)
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::DescriptorPool::init()
{
    static const Anvil::DescriptorType supported_descriptor_types[] =
    {
        Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
        Anvil::DescriptorType::INLINE_UNIFORM_BLOCK,
        Anvil::DescriptorType::INPUT_ATTACHMENT,
        Anvil::DescriptorType::SAMPLED_IMAGE,
        Anvil::DescriptorType::SAMPLER,
        Anvil::DescriptorType::STORAGE_BUFFER,
        Anvil::DescriptorType::STORAGE_BUFFER_DYNAMIC,
        Anvil::DescriptorType::STORAGE_IMAGE,
        Anvil::DescriptorType::STORAGE_TEXEL_BUFFER,
        Anvil::DescriptorType::UNIFORM_BUFFER,
        Anvil::DescriptorType::UNIFORM_BUFFER_DYNAMIC,
        Anvil::DescriptorType::UNIFORM_TEXEL_BUFFER,
    };
    const uint32_t n_supported_descriptor_types = sizeof(supported_descriptor_types) / sizeof(supported_descriptor_types[0]);

    VkDescriptorPoolSize                             descriptor_pool_sizes[n_supported_descriptor_types];
    const auto&                                      dp_create_flags                                     (m_create_info_ptr->get_create_flags() );
    uint32_t                                         n_descriptor_types_used                             (0);
    bool                                             result                                              (false);
    VkResult                                         result_vk                                           (VK_ERROR_INITIALIZATION_FAILED);
    Anvil::StructChainer<VkDescriptorPoolCreateInfo> struct_chainer;

    if ((dp_create_flags & Anvil::DescriptorPoolCreateFlagBits::UPDATE_AFTER_BIND_BIT) != 0)
    {
        if (!m_device_ptr->get_extension_info()->ext_descriptor_indexing() )
        {
            anvil_assert_fail();

            goto end;
        }
    }

    /* Convert the counters to an arrayed, linear representation */
    for (const auto& current_descriptor_type : supported_descriptor_types)
    {
        const auto n_descriptors_needed = m_create_info_ptr->get_n_descriptors_for_descriptor_type(current_descriptor_type);

        if (n_descriptors_needed > 0)
        {
            uint32_t current_index = n_descriptor_types_used;

            descriptor_pool_sizes[current_index].descriptorCount = n_descriptors_needed;
            descriptor_pool_sizes[current_index].type            = static_cast<VkDescriptorType>(current_descriptor_type);

            n_descriptor_types_used++;
        }
    }

    /* Set up the descriptor pool instance */
    {
        VkDescriptorPoolCreateInfo create_info;

        create_info.flags         = m_create_info_ptr->get_create_flags().get_vk();
        create_info.maxSets       = m_create_info_ptr->get_n_maximum_sets();
        create_info.pNext         = nullptr;
        create_info.poolSizeCount = n_descriptor_types_used;
        create_info.pPoolSizes    = descriptor_pool_sizes;
        create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

        struct_chainer.append_struct(create_info);
    }

    {
        const auto& n_max_inline_uniform_block_bindings = m_create_info_ptr->get_n_maximum_inline_uniform_block_bindings();

        if (n_max_inline_uniform_block_bindings > 0)
        {
            VkDescriptorPoolInlineUniformBlockCreateInfoEXT create_info;

            anvil_assert(m_device_ptr->get_extension_info()->ext_inline_uniform_block() );

            create_info.maxInlineUniformBlockBindings = n_max_inline_uniform_block_bindings;
            create_info.pNext                         = nullptr;
            create_info.sType                         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO_EXT;

            struct_chainer.append_struct(create_info);
        }
    }

    {
        auto chain_ptr = struct_chainer.create_chain();

        result_vk = Anvil::Vulkan::vkCreateDescriptorPool(m_device_ptr->get_device_vk(),
                                                          chain_ptr->get_root_struct (),
                                                          nullptr, /* pAllocator */
                                                         &m_pool);
    }

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk) )
    {
        set_vk_handle(m_pool);
    }
    else
    {
        anvil_assert(result);

        goto end;
    }

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorPool::reset()
{
    VkResult result_vk;

    if (m_pool != VK_NULL_HANDLE)
    {
        /* TODO: Host synchronization to VkDescriptorSetObjects alloc'ed from the pool. */
        lock();
        {
            result_vk = Anvil::Vulkan::vkResetDescriptorPool(m_device_ptr->get_device_vk(),
                                                             m_pool,
                                                             0 /* flags */);
        }
        unlock();

        anvil_assert_vk_call_succeeded(result_vk);

        if (is_vk_call_successful(result_vk) )
        {
            /* Alloced descriptor sets went out of scope. Send out a call-back, so that descriptor set
             * wrapper instances can mark themselves as unusable */
            OnDescriptorPoolResetCallbackArgument callback_argument(this);

            callback(DESCRIPTOR_POOL_CALLBACK_ID_POOL_RESET,
                    &callback_argument);
        }
    }
    else
    {
        result_vk = VK_SUCCESS;
    }

    return is_vk_call_successful(result_vk); 
}