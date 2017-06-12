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
#include "wrappers/pipeline_layout.h"
#include "wrappers/pipeline_layout_manager.h"
#include <algorithm>


/** Constructor. */
Anvil::PipelineLayoutManager::PipelineLayoutManager(std::weak_ptr<Anvil::BaseDevice> in_device_ptr)
    :m_device_ptr              (in_device_ptr),
     m_pipeline_layouts_created(0)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_PIPELINE_LAYOUT_MANAGER,
                                                  this);
}

/** Destructor */
Anvil::PipelineLayoutManager::~PipelineLayoutManager()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_PIPELINE_LAYOUT_MANAGER,
                                                    this);
}

/* Please see header for specification */
std::shared_ptr<Anvil::PipelineLayoutManager> Anvil::PipelineLayoutManager::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr)
{
    std::shared_ptr<Anvil::BaseDevice>            device_locked_ptr(in_device_ptr);
    std::shared_ptr<Anvil::PipelineLayoutManager> result_ptr;

    result_ptr.reset(new Anvil::PipelineLayoutManager(in_device_ptr) );
    anvil_assert(result_ptr != nullptr);

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::PipelineLayoutManager::get_layout(std::shared_ptr<DescriptorSetGroup>     in_dsg_ptr,
                                              const PushConstantRanges&               in_push_constant_ranges,
                                              std::shared_ptr<Anvil::PipelineLayout>* out_pipeline_layout_ptr_ptr)
{
    bool result = false;

    for (auto layout_iterator  = m_pipeline_layouts.begin();
              layout_iterator != m_pipeline_layouts.end();
            ++layout_iterator)
    {
        if (layout_iterator->second.expired() )
        {
            continue;
        }

        std::shared_ptr<Anvil::PipelineLayout> current_pipeline_layout_ptr = layout_iterator->second.lock();

        if (current_pipeline_layout_ptr->get_attached_dsg()                  == in_dsg_ptr              &&
            current_pipeline_layout_ptr->get_attached_push_constant_ranges() == in_push_constant_ranges)
        {
            *out_pipeline_layout_ptr_ptr = current_pipeline_layout_ptr;
            result                       = true;

            break;
        }
    }

    if (!result)
    {
        result = true;

        /* Try to create a new layout for the specified DSG + push constant range set */
        std::shared_ptr<Anvil::PipelineLayout> new_layout_ptr = Anvil::PipelineLayout::create(m_device_ptr);

        result = new_layout_ptr->set_dsg(in_dsg_ptr);

        if (!result)
        {
            goto end;
        }

        for (auto push_constant_range_iterator  = in_push_constant_ranges.begin();
                  push_constant_range_iterator != in_push_constant_ranges.end();
                ++push_constant_range_iterator)
        {
            result = new_layout_ptr->attach_push_constant_range((*push_constant_range_iterator).offset,
                                                                (*push_constant_range_iterator).size,
                                                                (*push_constant_range_iterator).stages);

            if (!result)
            {
                goto end;
            }
        }

        new_layout_ptr->register_for_callbacks(PIPELINE_LAYOUT_CALLBACK_ID_OBJECT_ABOUT_TO_BE_DELETED,
                                               on_pipeline_layout_dropped,
                                               this);

        if (result)
        {
            const PipelineLayoutID pipeline_layout_id = new_layout_ptr->get_id();

            anvil_assert(m_pipeline_layouts.find(pipeline_layout_id) == m_pipeline_layouts.end() );

            new_layout_ptr->bake();

            m_pipeline_layouts[pipeline_layout_id] = new_layout_ptr;
            *out_pipeline_layout_ptr_ptr           = new_layout_ptr;
        }
    }
end:
    return result;
}

/* Please see header for specification */
std::shared_ptr<Anvil::PipelineLayout> Anvil::PipelineLayoutManager::get_layout_by_id(Anvil::PipelineLayoutID in_id) const
{
    if (m_pipeline_layouts.find(in_id) == m_pipeline_layouts.end() ||
        m_pipeline_layouts.at  (in_id).expired() )
    {
        return std::shared_ptr<Anvil::PipelineLayout>();
    }
    else
    {
        return m_pipeline_layouts.at(in_id).lock();
    }
}

/** Called back whenever a pipeline layout is released **/
void Anvil::PipelineLayoutManager::on_pipeline_layout_dropped(void* in_callback_arg,
                                                              void* in_user_arg)
{
    PipelineLayouts::iterator     layout_iterator;
    Anvil::PipelineLayoutManager* layout_manager_ptr = static_cast<PipelineLayoutManager*>(in_user_arg);

    ANVIL_REDUNDANT_ARGUMENT(in_callback_arg);

    /* Are we the last standing layout user? */
    for (layout_iterator  = layout_manager_ptr->m_pipeline_layouts.begin();
         layout_iterator != layout_manager_ptr->m_pipeline_layouts.end();
        )
    {
        if (layout_iterator->second.expired() )
        {
            layout_manager_ptr->m_pipeline_layouts.erase(layout_iterator);

            layout_iterator = layout_manager_ptr->m_pipeline_layouts.begin();
        }
        else
        {
            ++layout_iterator;
        }
    }
}

/* Please see header for specification */
Anvil::PipelineLayoutID Anvil::PipelineLayoutManager::reserve_pipeline_layout_id()
{
    return m_pipeline_layouts_created++;
}