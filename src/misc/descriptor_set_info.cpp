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
bool Anvil::DescriptorSetInfo::add_binding(uint32_t                               in_binding_index,
                                           VkDescriptorType                       in_descriptor_type,
                                           uint32_t                               in_descriptor_array_size,
                                           VkShaderStageFlags                     in_stage_flags,
                                           const std::shared_ptr<Anvil::Sampler>* in_immutable_sampler_ptrs)
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
std::unique_ptr<Anvil::DescriptorSetInfo> Anvil::DescriptorSetInfo::create()
{
    std::unique_ptr<Anvil::DescriptorSetInfo> result_ptr;

    result_ptr.reset(
        new Anvil::DescriptorSetInfo()
    );

    return result_ptr;
}

/** Please see header for specification */
bool Anvil::DescriptorSetInfo::get_binding_properties(uint32_t            in_n_binding,
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
