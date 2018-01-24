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
#include "wrappers/descriptor_set_group.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/device.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/pipeline_layout_manager.h"

/** Please see header for specification */
Anvil::PipelineLayout::PipelineLayout(std::weak_ptr<Anvil::BaseDevice>                 in_device_ptr,
                                      std::shared_ptr<const Anvil::DescriptorSetGroup> in_dsg_ptr,
                                      const Anvil::PushConstantRanges&                 in_push_constant_ranges,
                                      bool                                             in_mt_safe)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT),
     MTSafetySupportProvider   (in_mt_safe)
{
    m_device_ptr           = in_device_ptr;
    m_dirty                = true;
    m_dsg_ptr              = in_dsg_ptr;
    m_layout_vk            = VK_NULL_HANDLE;
    m_push_constant_ranges = in_push_constant_ranges;
}

/** Please see header for specification */
Anvil::PipelineLayout::~PipelineLayout()
{
    m_dsg_ptr = nullptr;

    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_PIPELINE_LAYOUT,
                                                    this);

    if (m_layout_vk != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        lock();
        {
            vkDestroyPipelineLayout(device_locked_ptr->get_device_vk(),
                                    m_layout_vk,
                                    nullptr /* pAllocator */);
        }
        unlock();

        m_layout_vk = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
bool Anvil::PipelineLayout::bake()
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    std::vector<VkDescriptorSetLayout> ds_layouts_vk;
    VkPipelineLayoutCreateInfo         pipeline_layout_create_info;
    std::vector<VkPushConstantRange>   push_constant_ranges_vk;
    VkResult                           result_vk;

    /* Convert descriptor set layouts to Vulkan equivalents */
    const VkDescriptorSetLayout dummy_ds_layout      = device_locked_ptr->get_dummy_descriptor_set_layout()->get_layout();
    uint32_t                    max_ds_binding_index = 0;

    if (!m_dirty)
    {
        result_vk = VK_SUCCESS;

        goto end;
    }

    if (m_layout_vk != VK_NULL_HANDLE)
    {
        lock();
        {
            vkDestroyPipelineLayout(device_locked_ptr->get_device_vk(),
                                    m_layout_vk,
                                    nullptr /* pAllocator */);
        }
        unlock();

        m_layout_vk = VK_NULL_HANDLE;
        set_vk_handle(VK_NULL_HANDLE);
    }

    if (m_dsg_ptr != nullptr)
    {
        const uint32_t n_dses = m_dsg_ptr->get_n_descriptor_sets();

        for (uint32_t n_ds = 0;
                      n_ds < n_dses;
                    ++n_ds)
        {
            auto ds_layout_ptr = m_dsg_ptr->get_descriptor_set_layout(n_ds);

            if (ds_layout_ptr == nullptr)
            {
                continue;
            }

            max_ds_binding_index = n_ds;
        }
    }

    ds_layouts_vk.resize(max_ds_binding_index + 1,
                         dummy_ds_layout);

    if (m_dsg_ptr != nullptr)
    {
        const uint32_t n_current_descriptor_sets = m_dsg_ptr->get_n_descriptor_sets();

        for (uint32_t n_current_descriptor_set = 0;
                      n_current_descriptor_set < n_current_descriptor_sets;
                    ++n_current_descriptor_set)
        {
            auto ds_layout_ptr = m_dsg_ptr->get_descriptor_set_layout(n_current_descriptor_set);

            if (ds_layout_ptr == nullptr)
            {
                continue;
            }

            ds_layouts_vk[n_current_descriptor_set] = ds_layout_ptr->get_layout();
         }
    }

    /* Convert push range descriptors to Vulkan equivalents */
    push_constant_ranges_vk.clear();

    for (auto push_constant_range_iterator  = m_push_constant_ranges.begin();
              push_constant_range_iterator != m_push_constant_ranges.end();
            ++push_constant_range_iterator)
    {
        VkPushConstantRange new_push_constant_range;

        new_push_constant_range.offset     = push_constant_range_iterator->offset;
        new_push_constant_range.size       = push_constant_range_iterator->size;
        new_push_constant_range.stageFlags = push_constant_range_iterator->stages;

        push_constant_ranges_vk.push_back(new_push_constant_range);
    }

    /* Re-create the pipeline layout. */
    pipeline_layout_create_info.flags                  = 0;
    pipeline_layout_create_info.setLayoutCount         = static_cast<uint32_t>(ds_layouts_vk.size() );
    pipeline_layout_create_info.pNext                  = nullptr;
    pipeline_layout_create_info.pPushConstantRanges    = (push_constant_ranges_vk.size() > 0) ? &push_constant_ranges_vk.at(0)
                                                                                              : nullptr;
    pipeline_layout_create_info.pSetLayouts            = (ds_layouts_vk.size() > 0)           ? &ds_layouts_vk.at(0)
                                                                                              : nullptr;
    pipeline_layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(m_push_constant_ranges.size() );
    pipeline_layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    result_vk = vkCreatePipelineLayout(device_locked_ptr->get_device_vk(),
                                      &pipeline_layout_create_info,
                                       nullptr, /* pAllocator */
                                      &m_layout_vk);

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk))
    {
        set_vk_handle(m_layout_vk);

        m_dirty = false;
    }

end:
    return is_vk_call_successful(result_vk);
}

/* Please see header for specification */
std::shared_ptr<Anvil::PipelineLayout> Anvil::PipelineLayout::create(std::weak_ptr<Anvil::BaseDevice>                 in_device_ptr,
                                                                     std::shared_ptr<const Anvil::DescriptorSetGroup> in_dsg_ptr,
                                                                     const PushConstantRanges&                        in_push_constant_ranges,
                                                                     bool                                             in_mt_safe)
{
    std::shared_ptr<Anvil::PipelineLayout> result_ptr;

    result_ptr.reset(
        new Anvil::PipelineLayout(in_device_ptr,
                                  in_dsg_ptr,
                                  in_push_constant_ranges,
                                  in_mt_safe)
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->bake() )
        {
            result_ptr.reset();
        }
        else
        {
            Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_PIPELINE_LAYOUT,
                                                         result_ptr.get() );
        }
    }

    return result_ptr;
}

