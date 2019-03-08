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

#include "misc/descriptor_set_create_info.h"
#include "misc/struct_chainer.h"
#include "wrappers/device.h"
#include "wrappers/sampler.h"

/** Please see header for specification */
Anvil::DescriptorSetCreateInfo::DescriptorSetCreateInfo()
    :m_n_variable_descriptor_count_binding   (UINT32_MAX),
     m_variable_descriptor_count_binding_size(0)
{
    /* Stub */
}

/** Please see header for specification */
Anvil::DescriptorSetCreateInfo::~DescriptorSetCreateInfo()
{
    /* Stub */
}

/** Please see header for specification */
bool Anvil::DescriptorSetCreateInfo::add_binding(uint32_t                             in_binding_index,
                                                 Anvil::DescriptorType                in_descriptor_type,
                                                 uint32_t                             in_descriptor_array_size,
                                                 Anvil::ShaderStageFlags              in_stage_flags,
                                                 const Anvil::DescriptorBindingFlags& in_flags,
                                                 const Anvil::Sampler* const*         in_immutable_sampler_ptrs)
{
    bool result = false;

    /* Make sure the binding is not already defined */
    if (m_bindings.find(in_binding_index) != m_bindings.end() )
    {
        anvil_assert_fail();

        goto end;
    }

    /* Make sure the sampler array can actually be specified for this descriptor type. */
    if (in_immutable_sampler_ptrs != nullptr)
    {
        if (in_descriptor_type != Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER &&
            in_descriptor_type != Anvil::DescriptorType::SAMPLER)
        {
            anvil_assert_fail();

            goto end;
        }
    }

    if (in_descriptor_type == Anvil::DescriptorType::INLINE_UNIFORM_BLOCK)
    {
        anvil_assert((in_descriptor_array_size % 4) == 0);
    }

    if ((in_flags & Anvil::DescriptorBindingFlagBits::VARIABLE_DESCRIPTOR_COUNT_BIT) != 0)
    {
        if (m_n_variable_descriptor_count_binding != UINT32_MAX)
        {
            /* If this assertion check fails, you're attempting to add more than 1 variable descriptor count binding
             * which is illegal! */
            anvil_assert(m_n_variable_descriptor_count_binding == UINT32_MAX);

            goto end;
        }

        m_n_variable_descriptor_count_binding = in_binding_index;
    }

    /* Add a new binding entry and mark the layout as dirty, so that it is re-baked next time
     * the user calls the getter func */
    m_bindings[in_binding_index] = Binding(in_descriptor_array_size,
                                           in_descriptor_type,
                                           in_stage_flags,
                                           in_immutable_sampler_ptrs,
                                           in_flags);

    result  = true;
end:
    return result;
}

/** Please see header for specification */
Anvil::DescriptorSetCreateInfoUniquePtr Anvil::DescriptorSetCreateInfo::create()
{
    Anvil::DescriptorSetCreateInfoUniquePtr result_ptr(nullptr,
                                                       std::default_delete<Anvil::DescriptorSetCreateInfo>() );

    result_ptr.reset(
        new Anvil::DescriptorSetCreateInfo()
    );

    return result_ptr;
}

/** Please see header for specification */
std::unique_ptr<Anvil::DescriptorSetLayoutCreateInfoContainer> Anvil::DescriptorSetCreateInfo::create_descriptor_set_layout_create_info(const Anvil::BaseDevice* in_device_ptr) const
{
    uint32_t                                                       n_binding                         = 0;
    uint32_t                                                       n_bindings_defined                = 0;
    uint32_t                                                       n_samplers_defined                = 0;
    std::unique_ptr<Anvil::DescriptorSetLayoutCreateInfoContainer> result_ptr;
    bool                                                           should_chain_binding_flags_struct = false;
    Anvil::StructChainer<VkDescriptorSetLayoutCreateInfo>          struct_chainer;

    result_ptr.reset(
        new Anvil::DescriptorSetLayoutCreateInfoContainer()
    );

    if (result_ptr == nullptr)
    {
        anvil_assert(result_ptr != nullptr);

        goto end;
    }

    /* Count the number of immutable samplers defined. This is needed because if sampler_items is reallocated
     * after we start building the VkSampler array contents, all previously initialized VkDescriptorSetLayoutBinding
     * instances will start referring to invalid sampler arrays.
     */
    for (auto binding_iterator  = m_bindings.begin();
              binding_iterator != m_bindings.end();
            ++binding_iterator)
    {
        const auto& binding_data = binding_iterator->second;

        n_samplers_defined += static_cast<uint32_t>(binding_data.immutable_samplers.size() );
    }

    result_ptr->sampler_items.reserve(n_samplers_defined);

    /* Determine the number of non-dummy bindings. */
    if (m_bindings.size() > 1)
    {
        n_bindings_defined = static_cast<uint32_t>(m_bindings.size() );
    }
    else
    for (auto binding : m_bindings)
    {
        if (binding.second.descriptor_array_size > 0)
        {
            ++n_bindings_defined;
        }
    }

    /* Convert internal representation to Vulkan structure(s) */
    result_ptr->binding_info_items.resize(n_bindings_defined);

    {
        VkDescriptorSetLayoutCreateInfo create_info;

        create_info.bindingCount = n_bindings_defined;
        create_info.flags        = 0;
        create_info.pNext        = nullptr;
        create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        if (n_bindings_defined > 0)
        {
            for (auto binding_iterator  = m_bindings.begin();
                      binding_iterator != m_bindings.end();
                    ++binding_iterator, ++n_binding)
            {
                const auto&                   binding_data  = binding_iterator->second;
                VkDescriptorBindingFlagsEXT   binding_flags = 0;
                const BindingIndex&           binding_index = binding_iterator->first;
                VkDescriptorSetLayoutBinding& binding_vk    = result_ptr->binding_info_items.at(n_binding);

                if (binding_data.descriptor_array_size == 0)
                {
                    /* Dummy binding, ignore. */
                    continue;
                }

                binding_vk.binding         = binding_index;
                binding_vk.descriptorCount = binding_data.descriptor_array_size;
                binding_vk.descriptorType  = static_cast<VkDescriptorType>(binding_data.descriptor_type);
                binding_vk.stageFlags      = binding_data.stage_flags.get_vk();

                if (binding_data.immutable_samplers.size() > 0)
                {
                    const uint32_t sampler_array_start_index = static_cast<uint32_t>(result_ptr->sampler_items.size() );

                    anvil_assert(binding_data.immutable_samplers.size() == binding_data.descriptor_array_size);

                    for (uint32_t n_sampler = 0;
                                  n_sampler < binding_data.descriptor_array_size;
                                ++n_sampler)
                    {
                        result_ptr->sampler_items.push_back(binding_data.immutable_samplers[n_sampler]->get_sampler() );
                    }

                    binding_vk.pImmutableSamplers = &result_ptr->sampler_items[sampler_array_start_index];
                }
                else
                {
                    binding_vk.pImmutableSamplers = nullptr;
                }

                if ((binding_iterator->second.flags & Anvil::DescriptorBindingFlagBits::UPDATE_AFTER_BIND_BIT) != 0)
                {
                    binding_flags                     |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
                    create_info.flags                 |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
                    should_chain_binding_flags_struct  = true;
                }

                if ((binding_iterator->second.flags & Anvil::DescriptorBindingFlagBits::UPDATE_UNUSED_WHILE_PENDING_BIT) != 0)
                {
                    binding_flags                     |= VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;
                    should_chain_binding_flags_struct  = true;
                }

                if ((binding_iterator->second.flags & Anvil::DescriptorBindingFlagBits::PARTIALLY_BOUND_BIT) != 0)
                {
                    binding_flags                     |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT;
                    should_chain_binding_flags_struct  = true;
                }

                if ((binding_iterator->second.flags & Anvil::DescriptorBindingFlagBits::VARIABLE_DESCRIPTOR_COUNT_BIT) != 0)
                {
                    binding_flags                     |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;
                    should_chain_binding_flags_struct  = true;
                }

                result_ptr->binding_flags_vec.push_back(binding_flags);
            }
        }

        if (result_ptr->binding_info_items.size() > 0)
        {
            create_info.pBindings = &result_ptr->binding_info_items.at(0);
        }
        else
        {
            create_info.pBindings = nullptr;
        }

        struct_chainer.append_struct(create_info);
    }

    if (should_chain_binding_flags_struct)
    {
        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags_struct;

        if (!in_device_ptr->get_extension_info()->ext_descriptor_indexing() )
        {
            /* The bindings have been assigned flags which are only available on implementations
             * that report support for VK_EXT_descriptor_indexing extension!
             */
            anvil_assert(in_device_ptr->get_extension_info()->ext_descriptor_indexing() );

            result_ptr.reset();
            goto end;
        }

        anvil_assert(result_ptr->binding_flags_vec.size() >  0);
        anvil_assert(result_ptr->binding_flags_vec.size() == struct_chainer.get_root_struct()->bindingCount);

        binding_flags_struct.bindingCount  = static_cast<uint32_t>(result_ptr->binding_flags_vec.size() );
        binding_flags_struct.pBindingFlags = &result_ptr->binding_flags_vec.at(0);
        binding_flags_struct.pNext         = nullptr;
        binding_flags_struct.sType         = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT);

        struct_chainer.append_struct(binding_flags_struct);
    }

    result_ptr->struct_chain_ptr = struct_chainer.create_chain();

end:
    return result_ptr;
}

/** Please see header for specification */
bool Anvil::DescriptorSetCreateInfo::get_binding_properties_by_binding_index(uint32_t                       in_binding_index,
                                                                             Anvil::DescriptorType*         out_opt_descriptor_type_ptr,
                                                                             uint32_t*                      out_opt_descriptor_array_size_ptr,
                                                                             Anvil::ShaderStageFlags*       out_opt_stage_flags_ptr,
                                                                             bool*                          out_opt_immutable_samplers_enabled_ptr,
                                                                             Anvil::DescriptorBindingFlags* out_opt_flags_ptr) const
{
    auto binding_iterator = m_bindings.find(in_binding_index);
    bool result           = false;

    if (binding_iterator == m_bindings.end() )
    {
        goto end;
    }

    if (out_opt_descriptor_array_size_ptr != nullptr)
    {
        *out_opt_descriptor_array_size_ptr = binding_iterator->second.descriptor_array_size;
    }

    if (out_opt_descriptor_type_ptr != nullptr)
    {
        *out_opt_descriptor_type_ptr = binding_iterator->second.descriptor_type;
    }

    if (out_opt_immutable_samplers_enabled_ptr != nullptr)
    {
        *out_opt_immutable_samplers_enabled_ptr = (binding_iterator->second.immutable_samplers.size() != 0);
    }

    if (out_opt_stage_flags_ptr != nullptr)
    {
        *out_opt_stage_flags_ptr = binding_iterator->second.stage_flags;
    }

    if (out_opt_flags_ptr != nullptr)
    {
        *out_opt_flags_ptr = binding_iterator->second.flags;
    }

    result = true;

end:
    return result;
}

/** Please see header for specification */
bool Anvil::DescriptorSetCreateInfo::get_binding_properties_by_index_number(uint32_t                       in_n_binding,
                                                                            uint32_t*                      out_opt_binding_index_ptr,
                                                                            Anvil::DescriptorType*         out_opt_descriptor_type_ptr,
                                                                            uint32_t*                      out_opt_descriptor_array_size_ptr,
                                                                            Anvil::ShaderStageFlags*       out_opt_stage_flags_ptr,
                                                                            bool*                          out_opt_immutable_samplers_enabled_ptr,
                                                                            Anvil::DescriptorBindingFlags* out_opt_flags_ptr) const
{
    auto binding_iterator = m_bindings.begin();
    bool result           = false;

    if (m_bindings.size() <= in_n_binding)
    {
        goto end;
    }

    for (uint32_t n_current_binding = 0;
                  n_current_binding < in_n_binding;
                ++n_current_binding)
    {
        binding_iterator ++;
    }

    if (binding_iterator != m_bindings.end() )
    {
        if (out_opt_binding_index_ptr != nullptr)
        {
            *out_opt_binding_index_ptr = binding_iterator->first;
        }

        if (out_opt_descriptor_array_size_ptr != nullptr)
        {
            *out_opt_descriptor_array_size_ptr = binding_iterator->second.descriptor_array_size;
        }

        if (out_opt_descriptor_type_ptr != nullptr)
        {
            *out_opt_descriptor_type_ptr = binding_iterator->second.descriptor_type;
        }

        if (out_opt_immutable_samplers_enabled_ptr != nullptr)
        {
            *out_opt_immutable_samplers_enabled_ptr = (binding_iterator->second.immutable_samplers.size() != 0);
        }

        if (out_opt_stage_flags_ptr != nullptr)
        {
            *out_opt_stage_flags_ptr = binding_iterator->second.stage_flags;
        }

        if (out_opt_flags_ptr != nullptr)
        {
            *out_opt_flags_ptr = binding_iterator->second.flags;
        }

        result = true;
    }

end:
    return result;
}

bool Anvil::DescriptorSetCreateInfo::set_binding_variable_descriptor_count(const uint32_t& in_count)
{
    uint32_t binding_index = UINT32_MAX;
    bool     result        = false;

    if (!contains_variable_descriptor_count_binding(&binding_index) )
    {
        /* This descriptor set info instance does not define a variable descriptor count binding! */
        anvil_assert_fail();

        goto end;
    }

    m_variable_descriptor_count_binding_size = in_count;
    result                                   = true;

end:
    return result;
}

/** Please see header for specification */
bool Anvil::DescriptorSetCreateInfo::operator==(const Anvil::DescriptorSetCreateInfo& in_ds) const
{
    return (m_bindings                               == in_ds.m_bindings                               &&
            m_n_variable_descriptor_count_binding    == in_ds.m_n_variable_descriptor_count_binding    &&
            m_variable_descriptor_count_binding_size == in_ds.m_variable_descriptor_count_binding_size);
}