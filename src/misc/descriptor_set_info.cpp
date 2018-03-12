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

#include "misc/descriptor_set_info.h"
#include "misc/struct_chainer.h"
#include "wrappers/device.h"
#include "wrappers/sampler.h"

/** Please see header for specification */
Anvil::DescriptorSetInfo::DescriptorSetInfo()
{
    /* Stub */
}

/** Please see header for specification */
Anvil::DescriptorSetInfo::~DescriptorSetInfo()
{
    /* Stub */
}

/** Please see header for specification */
bool Anvil::DescriptorSetInfo::add_binding(uint32_t                     in_binding_index,
                                           VkDescriptorType             in_descriptor_type,
                                           uint32_t                     in_descriptor_array_size,
                                           VkShaderStageFlags           in_stage_flags,
                                           const Anvil::Sampler* const* in_immutable_sampler_ptrs)
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
        if (in_descriptor_type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER &&
            in_descriptor_type != VK_DESCRIPTOR_TYPE_SAMPLER)
        {
            anvil_assert_fail();

            goto end;
        }
    }

    /* Add a new binding entry and mark the layout as dirty, so that it is re-baked next time
     * the user calls the getter func */
    m_bindings[in_binding_index] = Binding(in_descriptor_array_size,
                                           in_descriptor_type,
                                           in_stage_flags,
                                           in_immutable_sampler_ptrs);

    result  = true;
end:
    return result;
}

/** Please see header for specification */
Anvil::DescriptorSetInfoUniquePtr Anvil::DescriptorSetInfo::create()
{
    Anvil::DescriptorSetInfoUniquePtr result_ptr(nullptr,
                                                 std::default_delete<Anvil::DescriptorSetInfo>() );

    result_ptr.reset(
        new Anvil::DescriptorSetInfo()
    );

    return result_ptr;
}

/** Please see header for specification */
std::unique_ptr<Anvil::DescriptorSetLayoutCreateInfoContainer> Anvil::DescriptorSetInfo::create_descriptor_set_layout_create_info(const Anvil::BaseDevice* in_device_ptr) const
{
    uint32_t                                                       n_binding          = 0;
    uint32_t                                                       n_bindings_defined = 0;
    uint32_t                                                       n_samplers_defined = 0;
    std::unique_ptr<Anvil::DescriptorSetLayoutCreateInfoContainer> result_ptr;
    Anvil::StructChainer<VkDescriptorSetLayoutCreateInfo>          struct_chainer;

    ANVIL_REDUNDANT_ARGUMENT_CONST(in_device_ptr);

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
                const BindingIndex&           binding_index = binding_iterator->first;
                VkDescriptorSetLayoutBinding& binding_vk    = result_ptr->binding_info_items.at(n_binding);

                if (binding_data.descriptor_array_size == 0)
                {
                    /* Dummy binding, ignore. */
                    continue;
                }

                binding_vk.binding         = binding_index;
                binding_vk.descriptorCount = binding_data.descriptor_array_size;
                binding_vk.descriptorType  = binding_data.descriptor_type;
                binding_vk.stageFlags      = binding_data.stage_flags;

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

    result_ptr->struct_chain_ptr = struct_chainer.create_chain();

end:
    return result_ptr;
}

/** Please see header for specification */
bool Anvil::DescriptorSetInfo::get_binding_properties_by_binding_index(uint32_t            in_binding_index,
                                                                       VkDescriptorType*   out_opt_descriptor_type_ptr,
                                                                       uint32_t*           out_opt_descriptor_array_size_ptr,
                                                                       VkShaderStageFlags* out_opt_stage_flags_ptr,
                                                                       bool*               out_opt_immutable_samplers_enabled_ptr) const
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

    result = true;

end:
    return result;
}

/** Please see header for specification */
bool Anvil::DescriptorSetInfo::get_binding_properties_by_index_number(uint32_t            in_n_binding,
                                                                      uint32_t*           out_opt_binding_index_ptr,
                                                                      VkDescriptorType*   out_opt_descriptor_type_ptr,
                                                                      uint32_t*           out_opt_descriptor_array_size_ptr,
                                                                      VkShaderStageFlags* out_opt_stage_flags_ptr,
                                                                      bool*               out_opt_immutable_samplers_enabled_ptr) const
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

        result = true;
    }

end:
    return result;
}

/** Please see header for specification */
bool Anvil::DescriptorSetInfo::operator==(const Anvil::DescriptorSetInfo& in_ds) const
{
    return (m_bindings == in_ds.m_bindings);
}