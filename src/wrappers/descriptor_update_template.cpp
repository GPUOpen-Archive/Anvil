//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/descriptor_set_create_info.h"
#include "misc/object_tracker.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/descriptor_update_template.h"
#include "wrappers/device.h"
#include "wrappers/physical_device.h"

Anvil::DescriptorUpdateTemplate::DescriptorUpdateTemplate(const Anvil::BaseDevice* in_device_ptr,
                                                          bool                     in_mt_safe)
    :DebugMarkerSupportProvider(in_device_ptr,
                                Anvil::ObjectType::DESCRIPTOR_UPDATE_TEMPLATE),
     MTSafetySupportProvider   (in_mt_safe),
     m_device_ptr              (in_device_ptr),
     m_vk_object               (VK_NULL_HANDLE)
{
    /* Register this instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::DESCRIPTOR_UPDATE_TEMPLATE,
                                                 this);

}

Anvil::DescriptorUpdateTemplate::~DescriptorUpdateTemplate()
{
    if (m_vk_object != VK_NULL_HANDLE)
    {
        const auto& entrypoints = m_device_ptr->get_extension_khr_descriptor_update_template_entrypoints();

        entrypoints.vkDestroyDescriptorUpdateTemplateKHR(m_device_ptr->get_device_vk(),
                                                         m_vk_object,
                                                         nullptr); /* pAllocator */

        m_vk_object = VK_NULL_HANDLE;
    }

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::DESCRIPTOR_UPDATE_TEMPLATE,
                                                   this);

}

Anvil::DescriptorUpdateTemplateUniquePtr Anvil::DescriptorUpdateTemplate::create_for_descriptor_set_updates(const Anvil::BaseDevice*                    in_device_ptr,
                                                                                                            const Anvil::DescriptorSetLayout*           in_descriptor_set_layout_ptr,
                                                                                                            const Anvil::DescriptorUpdateTemplateEntry* in_update_entries_ptr,
                                                                                                            const uint32_t&                             in_n_update_entries,
                                                                                                            MTSafety                                    in_mt_safety)
{
    DescriptorUpdateTemplateUniquePtr result_ptr(nullptr,
                                                 std::default_delete<Anvil::DescriptorUpdateTemplate>() );

    result_ptr.reset(
        new DescriptorUpdateTemplate(in_device_ptr,
                                     Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                     in_device_ptr) )
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init(in_descriptor_set_layout_ptr,
                              in_update_entries_ptr,
                              in_n_update_entries) )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool Anvil::DescriptorUpdateTemplate::init(const Anvil::DescriptorSetLayout*           in_descriptor_set_layout_ptr,
                                           const Anvil::DescriptorUpdateTemplateEntry* in_update_entries_ptr,
                                           const uint32_t&                             in_n_update_entries)
{
    const Anvil::ExtensionKHRDescriptorUpdateTemplateEntrypoints* entrypoints_ptr = nullptr;
    bool                                                          result          = true;

    if (!m_device_ptr->get_extension_info()->khr_descriptor_update_template() )
    {
        anvil_assert(m_device_ptr->get_extension_info()->khr_descriptor_update_template() );

        result = false;
        goto end;
    }

    if (in_descriptor_set_layout_ptr == nullptr)
    {
        anvil_assert(in_descriptor_set_layout_ptr != nullptr);

        result = false;
        goto end;
    }

    if (in_n_update_entries == 0)
    {
        anvil_assert(in_n_update_entries != 0);

        result = false;
        goto end;
    }

    if (in_update_entries_ptr == nullptr)
    {
        anvil_assert(in_update_entries_ptr != nullptr);

        result = false;
        goto end;
    }

    entrypoints_ptr = &m_device_ptr->get_extension_khr_descriptor_update_template_entrypoints();

    if (entrypoints_ptr == nullptr)
    {
        anvil_assert(entrypoints_ptr != nullptr);

        result = false;
        goto end;
    }

    {
        VkDescriptorUpdateTemplateCreateInfoKHR         create_info;
        std::vector<VkDescriptorUpdateTemplateEntryKHR> update_entries(in_n_update_entries);

        for (uint32_t n_update_entry = 0;
                      n_update_entry < in_n_update_entries;
                    ++n_update_entry)
        {
            update_entries.at(n_update_entry) = in_update_entries_ptr[n_update_entry].get_vk_descriptor_update_template_entry_khr();
        }

        create_info.descriptorSetLayout        = in_descriptor_set_layout_ptr->get_layout();
        create_info.descriptorUpdateEntryCount = in_n_update_entries;
        create_info.flags                      = 0;
        create_info.pDescriptorUpdateEntries   = &update_entries.at(0);
        create_info.pipelineBindPoint          = VK_PIPELINE_BIND_POINT_MAX_ENUM;
        create_info.pipelineLayout             = VK_NULL_HANDLE;
        create_info.pNext                      = nullptr;
        create_info.set                        = VK_NULL_HANDLE;
        create_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO_KHR;
        create_info.templateType               = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET_KHR;

        in_descriptor_set_layout_ptr->lock();
        {
            result = is_vk_call_successful(entrypoints_ptr->vkCreateDescriptorUpdateTemplateKHR(m_device_ptr->get_device_vk(),
                                                                                               &create_info,
                                                                                                nullptr, /* pAllocator */
                                                                                               &m_vk_object) );
        }
        in_descriptor_set_layout_ptr->unlock();
    }

    if (!result                        ||
         m_vk_object == VK_NULL_HANDLE)
    {
        anvil_assert(result);
        anvil_assert(m_vk_object != VK_NULL_HANDLE);

        result = false;
        goto end;
    }

    set_vk_handle(m_vk_object);

    /* Finally, also cache descriptor set create info using the user-specified DS layout */
    m_ds_create_info_ptr.reset(
        new Anvil::DescriptorSetCreateInfo(*in_descriptor_set_layout_ptr->get_create_info() )
    );

    if (m_ds_create_info_ptr == nullptr)
    {
        anvil_assert(m_ds_create_info_ptr != nullptr);

        result = false;
        goto end;
    }

end:
    return result;
}

void Anvil::DescriptorUpdateTemplate::update_descriptor_set(const Anvil::DescriptorSet* inout_ds_ptr,
                                                            const void*                 in_data_ptr) const
{
    const auto& entrypoints = m_device_ptr->get_extension_khr_descriptor_update_template_entrypoints();

    if (in_data_ptr == nullptr)
    {
        anvil_assert(in_data_ptr != nullptr);

        goto end;
    }

    #ifdef _DEBUG
    {
        if (!(*inout_ds_ptr->get_descriptor_set_layout()->get_create_info() == *m_ds_create_info_ptr) )
        {
            anvil_assert_fail();

            goto end;
        }
    }
    #endif

    inout_ds_ptr->lock();
    lock();
    {
        entrypoints.vkUpdateDescriptorSetWithTemplateKHR(m_device_ptr->get_device_vk(),
                                                         inout_ds_ptr->get_descriptor_set_vk(),
                                                         m_vk_object,
                                                         in_data_ptr);
    }
    unlock();
    inout_ds_ptr->unlock();

end:
    ;
}

