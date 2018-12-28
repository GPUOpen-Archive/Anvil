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
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/descriptor_set_layout_manager.h"
#include <algorithm>


/** Constructor. */
Anvil::DescriptorSetLayoutManager::DescriptorSetLayoutManager(const Anvil::BaseDevice* in_device_ptr,
                                                              bool                     in_mt_safe)
    :MTSafetySupportProvider(in_mt_safe),
     m_device_ptr           (in_device_ptr)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::ANVIL_DESCRIPTOR_SET_LAYOUT_MANAGER,
                                                  this);
}

/** Destructor */
Anvil::DescriptorSetLayoutManager::~DescriptorSetLayoutManager()
{
    anvil_assert(m_descriptor_set_layouts.size() == 0);

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::ANVIL_DESCRIPTOR_SET_LAYOUT_MANAGER,
                                                    this);
}

/* Please see header for specification */
Anvil::DescriptorSetLayoutManagerUniquePtr Anvil::DescriptorSetLayoutManager::create(const Anvil::BaseDevice* in_device_ptr,
                                                                                     bool                     in_mt_safe)
{
    DescriptorSetLayoutManagerUniquePtr result_ptr(nullptr,
                                                   std::default_delete<DescriptorSetLayoutManager>() );

    result_ptr.reset(
        new Anvil::DescriptorSetLayoutManager(in_device_ptr,
                                              in_mt_safe)
    );

    anvil_assert(result_ptr != nullptr);
    return result_ptr;
}

/* Please see header for specification */
bool Anvil::DescriptorSetLayoutManager::get_layout(const DescriptorSetCreateInfo*       in_ds_create_info_ptr,
                                                   Anvil::DescriptorSetLayoutUniquePtr* out_ds_layout_ptr_ptr)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr            = get_mutex();
    bool                                   result               = false;
    Anvil::DescriptorSetLayout*            result_ds_layout_ptr = nullptr;

    anvil_assert(in_ds_create_info_ptr != nullptr);

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    for (auto layout_iterator  = m_descriptor_set_layouts.begin();
              layout_iterator != m_descriptor_set_layouts.end();
            ++layout_iterator)
    {
        auto&  current_ds_layout_container_ptr  = *layout_iterator;
        auto&  current_ds_layout_ptr            = current_ds_layout_container_ptr->ds_layout_ptr;
        auto   current_ds_create_info_ptr       = current_ds_layout_ptr->get_create_info();

        if (*in_ds_create_info_ptr == *current_ds_create_info_ptr)
        {
            result               = true;
            result_ds_layout_ptr = current_ds_layout_ptr.get();

            current_ds_layout_container_ptr->n_references.fetch_add(1);

            break;
        }
    }

    if (!result)
    {
        auto ds_create_info_clone_ptr    = DescriptorSetCreateInfoUniquePtr             (new DescriptorSetCreateInfo(*in_ds_create_info_ptr),
                                                                                         std::default_delete<Anvil::DescriptorSetCreateInfo>() );
        auto new_ds_layout_ptr           = Anvil::DescriptorSetLayout::create           (std::move(ds_create_info_clone_ptr),
                                                                                         m_device_ptr,
                                                                                         Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ));
        auto new_ds_layout_container_ptr = std::unique_ptr<DescriptorSetLayoutContainer>(new DescriptorSetLayoutContainer() );

        result                                     = true;
        result_ds_layout_ptr                       = new_ds_layout_ptr.get();
        new_ds_layout_container_ptr->ds_layout_ptr = std::move(new_ds_layout_ptr);

        m_descriptor_set_layouts.push_back(
            std::move(new_ds_layout_container_ptr)
        );
    }

    if (result)
    {
        anvil_assert(result_ds_layout_ptr != nullptr);

        *out_ds_layout_ptr_ptr = Anvil::DescriptorSetLayoutUniquePtr(result_ds_layout_ptr,
                                                                     std::bind(&DescriptorSetLayoutManager::on_descriptor_set_layout_dereferenced,
                                                                               this,
                                                                               result_ds_layout_ptr)
        );
    }

    return result;
}

void Anvil::DescriptorSetLayoutManager::on_descriptor_set_layout_dereferenced(Anvil::DescriptorSetLayout* in_layout_ptr)
{
    bool                                   has_found  = false;
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    for (auto layout_iterator  = m_descriptor_set_layouts.begin();
              layout_iterator != m_descriptor_set_layouts.end()    && !has_found;
            ++layout_iterator)
    {
        auto& current_ds_layout_container_ptr = *layout_iterator;
        auto& current_ds_layout_ptr           = current_ds_layout_container_ptr->ds_layout_ptr;

        if (current_ds_layout_ptr.get() == in_layout_ptr)
        {
            has_found = true;

            if (current_ds_layout_container_ptr->n_references.fetch_sub(1) == 1)
            {
                m_descriptor_set_layouts.erase(layout_iterator);
            }

            break;
        }
    }

    anvil_assert(has_found);
}