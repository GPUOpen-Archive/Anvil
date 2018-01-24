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
#include "misc/descriptor_set_info.h"
#include "misc/object_tracker.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/device.h"
#include "wrappers/sampler.h"

/** Please see header for specification */
Anvil::DescriptorSetLayout::DescriptorSetLayout(std::unique_ptr<Anvil::DescriptorSetInfo> in_ds_info_ptr,
                                                std::weak_ptr<Anvil::BaseDevice>          in_device_ptr,
                                                bool                                      in_mt_safe)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT),
     MTSafetySupportProvider   (in_mt_safe),
     m_device_ptr              (in_device_ptr),
     m_info_ptr                (std::move(in_ds_info_ptr) ),
     m_layout                  (VK_NULL_HANDLE)
{
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                                  this);
}

/** Please see header for specification */
Anvil::DescriptorSetLayout::~DescriptorSetLayout()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                                    this);

    if (m_layout != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        lock();
        {
            vkDestroyDescriptorSetLayout(device_locked_ptr->get_device_vk(),
                                         m_layout,
                                         nullptr /* pAllocator */);
        }
        unlock();

        m_layout = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
std::shared_ptr<Anvil::DescriptorSetLayout> Anvil::DescriptorSetLayout::create(std::unique_ptr<Anvil::DescriptorSetInfo> in_ds_info_ptr,
                                                                               std::weak_ptr<Anvil::BaseDevice>          in_device_ptr,
                                                                               MTSafety                                  in_mt_safety)
{
    const bool                                  mt_safe = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                                          in_device_ptr);
    std::shared_ptr<Anvil::DescriptorSetLayout> result_ptr;

    result_ptr.reset(
        new Anvil::DescriptorSetLayout(std::move(in_ds_info_ptr),
                                       in_device_ptr,
                                       mt_safe)
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

/** Please see header for specification */
bool Anvil::DescriptorSetLayout::init()
{
    std::vector<VkDescriptorSetLayoutBinding> binding_info_items;
    VkDescriptorSetLayoutCreateInfo           create_info;
    std::shared_ptr<Anvil::BaseDevice>        device_locked_ptr(m_device_ptr);
    uint32_t                                  n_binding          = 0;
    uint32_t                                  n_bindings_defined = 0;
    uint32_t                                  n_samplers_defined = 0;
    bool                                      result             = false;
    VkResult                                  result_vk;
    std::vector<VkSampler>                    sampler_items;

    anvil_assert(m_layout == VK_NULL_HANDLE);

    /* Count the number of immutable samplers defined. This is needed because if sampler_items is reallocated
     * after we start building the VkSampler array contents, all previously initialized VkDescriptorSetLayoutBinding
     * instances will start referring to invalid sampler arrays. */
    for (auto binding_iterator  = m_info_ptr->m_bindings.begin();
              binding_iterator != m_info_ptr->m_bindings.end();
            ++binding_iterator)
    {
        const auto& binding_data = binding_iterator->second;

        n_samplers_defined += static_cast<uint32_t>(binding_data.immutable_samplers.size() );
    }

    sampler_items.reserve(n_samplers_defined);

    /* Determine the number of non-dummy bindings. */
    if (m_info_ptr->m_bindings.size() > 1)
    {
        n_bindings_defined = static_cast<uint32_t>(m_info_ptr->m_bindings.size() );
    }
    else
    for (auto binding : m_info_ptr->m_bindings)
    {
        if (binding.second.descriptor_array_size > 0)
        {
            ++n_bindings_defined;
        }
    }

    /* Convert internal representation to Vulkan structure(s) */
    binding_info_items.resize(n_bindings_defined);

    create_info.bindingCount = n_bindings_defined;
    create_info.flags        = 0;
    create_info.pNext        = nullptr;
    create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    if (n_bindings_defined > 0)
    {
        for (auto binding_iterator  = m_info_ptr->m_bindings.begin();
                  binding_iterator != m_info_ptr->m_bindings.end();
                ++binding_iterator, ++n_binding)
        {
            const auto&                   binding_data  = binding_iterator->second;
            const BindingIndex&           binding_index = binding_iterator->first;
            VkDescriptorSetLayoutBinding& binding_vk    = binding_info_items[n_binding];

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
                const uint32_t sampler_array_start_index = static_cast<uint32_t>(sampler_items.size() );

                anvil_assert(binding_data.immutable_samplers.size() == binding_data.descriptor_array_size);

                for (uint32_t n_sampler = 0;
                              n_sampler < binding_data.descriptor_array_size;
                            ++n_sampler)
                {
                    sampler_items.push_back(binding_data.immutable_samplers[n_sampler]->get_sampler() );
                }

                binding_vk.pImmutableSamplers = &sampler_items[sampler_array_start_index];
            }
            else
            {
                binding_vk.pImmutableSamplers = nullptr;
            }
        }
    }

    if (binding_info_items.size() > 0)
    {
        create_info.pBindings = &binding_info_items.at(0);
    }
    else
    {
        create_info.pBindings = nullptr;
    }

    /* Bake the Vulkan object */
    result_vk = vkCreateDescriptorSetLayout(device_locked_ptr->get_device_vk(),
                                           &create_info,
                                            nullptr, /* pAllocator */
                                           &m_layout);

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk) )
    {
        set_vk_handle(m_layout);
    }

    result = is_vk_call_successful(result_vk);
    return result;
}
