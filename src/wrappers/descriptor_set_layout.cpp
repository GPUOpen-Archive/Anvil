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
#include "misc/descriptor_set_create_info.h"
#include "misc/object_tracker.h"
#include "misc/struct_chainer.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/device.h"
#include "wrappers/sampler.h"

/** Please see header for specification */
Anvil::DescriptorSetLayout::DescriptorSetLayout(Anvil::DescriptorSetCreateInfoUniquePtr in_ds_create_info_ptr,
                                                const Anvil::BaseDevice*                in_device_ptr,
                                                bool                                    in_mt_safe)
    :DebugMarkerSupportProvider(in_device_ptr,
                                Anvil::ObjectType::DESCRIPTOR_SET_LAYOUT),
     MTSafetySupportProvider   (in_mt_safe),
     m_create_info_ptr         (std::move(in_ds_create_info_ptr) ),
     m_device_ptr              (in_device_ptr),
     m_layout                  (VK_NULL_HANDLE)
{
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::DESCRIPTOR_SET_LAYOUT,
                                                  this);
}

/** Please see header for specification */
Anvil::DescriptorSetLayout::~DescriptorSetLayout()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::DESCRIPTOR_SET_LAYOUT,
                                                    this);

    if (m_layout != VK_NULL_HANDLE)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyDescriptorSetLayout(m_device_ptr->get_device_vk(),
                                                        m_layout,
                                                        nullptr /* pAllocator */);
        }
        unlock();

        m_layout = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
Anvil::DescriptorSetLayoutUniquePtr Anvil::DescriptorSetLayout::create(Anvil::DescriptorSetCreateInfoUniquePtr in_ds_create_info_ptr,
                                                                       const Anvil::BaseDevice*                in_device_ptr,
                                                                       MTSafety                                in_mt_safety)
{
    const bool                   mt_safe = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                           in_device_ptr);
    DescriptorSetLayoutUniquePtr result_ptr(nullptr,
                                            std::default_delete<Anvil::DescriptorSetLayout>() );

    result_ptr.reset(
        new Anvil::DescriptorSetLayout(std::move(in_ds_create_info_ptr),
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

uint32_t Anvil::DescriptorSetLayout::get_maximum_variable_descriptor_count(const DescriptorSetLayoutCreateInfoContainer* in_ds_create_info_ptr,
                                                                           const Anvil::BaseDevice*                      in_device_ptr)
{
    const Anvil::ExtensionKHRMaintenance3Entrypoints*             entrypoints_ptr  = nullptr;
    Anvil::StructID                                               query_struct_id  = UINT32_MAX;
    const VkDescriptorSetVariableDescriptorCountLayoutSupportEXT* query_struct_ptr = nullptr;
    uint32_t                                                      result           = 0;
    Anvil::StructChainUniquePtr<VkDescriptorSetLayoutSupportKHR>  struct_chain_ptr;
    Anvil::StructChainer<VkDescriptorSetLayoutSupportKHR>         struct_chainer;

    if (!in_device_ptr->get_extension_info()->ext_descriptor_indexing() )
    {
        anvil_assert(in_device_ptr->get_extension_info()->ext_descriptor_indexing() );

        goto end;
    }

    if (!in_device_ptr->get_extension_info()->khr_maintenance3() )
    {
        anvil_assert(in_device_ptr->get_extension_info()->khr_maintenance3());

        goto end;
    }
    else
    {
        entrypoints_ptr = &in_device_ptr->get_extension_khr_maintenance3_entrypoints();
    }

    {
        VkDescriptorSetLayoutSupportKHR root;

        root.pNext     = nullptr;
        root.sType     = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT_KHR);
        root.supported = VK_FALSE;

        struct_chainer.append_struct(root);
    }

    {
        VkDescriptorSetVariableDescriptorCountLayoutSupportEXT query;

        query.maxVariableDescriptorCount = 0;
        query.pNext                      = nullptr;
        query.sType                      = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT_EXT);

        query_struct_id = struct_chainer.append_struct(query);
    }

    struct_chain_ptr = struct_chainer.create_chain                                                                 ();
    query_struct_ptr = struct_chain_ptr->get_struct_with_id<VkDescriptorSetVariableDescriptorCountLayoutSupportEXT>(query_struct_id);

    if (query_struct_ptr == nullptr)
    {
        anvil_assert(query_struct_ptr != nullptr);

        goto end;
    }

    entrypoints_ptr->vkGetDescriptorSetLayoutSupportKHR(in_device_ptr->get_device_vk(),
                                                        in_ds_create_info_ptr->struct_chain_ptr->get_root_struct(),
                                                        struct_chain_ptr->get_root_struct() );

    result = query_struct_ptr->maxVariableDescriptorCount;

end:
    return result;
}

bool Anvil::DescriptorSetLayout::meets_max_per_set_descriptors_limit(const DescriptorSetLayoutCreateInfoContainer* in_ds_create_info_ptr,
                                                                     const Anvil::BaseDevice*                      in_device_ptr)
{
    const Anvil::ExtensionKHRMaintenance3Entrypoints* entrypoints_ptr = nullptr;
    VkDescriptorSetLayoutSupportKHR                   query;
    bool                                              result          = false;

    if (!in_device_ptr->get_extension_info()->khr_maintenance3() )
    {
        anvil_assert(in_device_ptr->get_extension_info()->khr_maintenance3());

        goto end;
    }
    else
    {
        entrypoints_ptr = &in_device_ptr->get_extension_khr_maintenance3_entrypoints();
    }

    query.pNext     = nullptr;
    query.sType     = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT_KHR);
    query.supported = VK_FALSE;

    entrypoints_ptr->vkGetDescriptorSetLayoutSupportKHR(in_device_ptr->get_device_vk(),
                                                        in_ds_create_info_ptr->struct_chain_ptr->get_root_struct(),
                                                       &query);

    result = (query.supported == VK_TRUE);
end:
    return result;
}

/** Please see header for specification */
bool Anvil::DescriptorSetLayout::init()
{
    bool     result    = false;
    VkResult result_vk;

    anvil_assert(m_layout == VK_NULL_HANDLE);

    /* Bake the Vulkan object */
    auto create_info_ptr = m_create_info_ptr->create_descriptor_set_layout_create_info(m_device_ptr);

    if (create_info_ptr == nullptr)
    {
        anvil_assert(create_info_ptr != nullptr);

        goto end;
    }

    result_vk = Anvil::Vulkan::vkCreateDescriptorSetLayout(m_device_ptr->get_device_vk(),
                                                           create_info_ptr->struct_chain_ptr->get_root_struct(),
                                                           nullptr, /* pAllocator */
                                                          &m_layout);

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk) )
    {
        set_vk_handle(m_layout);
    }

    result = is_vk_call_successful(result_vk);

end:
    return result;
}
