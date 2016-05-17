//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
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


std::map<Anvil::Device*, Anvil::PipelineLayoutManager::PipelineLayoutManagerInfo> Anvil::PipelineLayoutManager::m_instance_map;


/** Constructor. */
Anvil::PipelineLayoutManager::PipelineLayoutManager(Anvil::Device* device_ptr)
    :m_device_ptr(device_ptr)
{
    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_PIPELINE_LAYOUT_MANAGER,
                                                  this);
}

/** Destructor */
Anvil::PipelineLayoutManager::~PipelineLayoutManager()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_PIPELINE_LAYOUT_MANAGER,
                                                    this);
}

/* Please see header for specification */
Anvil::PipelineLayoutManager* Anvil::PipelineLayoutManager::acquire(Anvil::Device* device_ptr)
{
    auto                           iterator   = m_instance_map.find(device_ptr);
    Anvil::PipelineLayoutManager* result_ptr = nullptr;

    if (iterator == m_instance_map.end() )
    {
        PipelineLayoutManagerInfo new_manager;

        new_manager.instance_ptr = new Anvil::PipelineLayoutManager(device_ptr);
        new_manager.ref_counter  = 1;

        m_instance_map[device_ptr] = new_manager;
        iterator                   = m_instance_map.find(device_ptr);
    }
    else
    {
        ++iterator->second.ref_counter;
    }

    result_ptr = iterator->second.instance_ptr;
    anvil_assert(result_ptr != nullptr);

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::PipelineLayoutManager::get_retained_layout(const DescriptorSetGroups& dsgs,
                                                       const PushConstantRanges&  push_constant_ranges,
                                                       Anvil::PipelineLayout**    out_pipeline_layout_ptr_ptr)
{
    bool result = false;

    for (auto layout_iterator  = m_pipeline_layouts.begin();
              layout_iterator != m_pipeline_layouts.end();
            ++layout_iterator)
    {
        Anvil::PipelineLayout* current_pipeline_layout_ptr = *layout_iterator;

        if (current_pipeline_layout_ptr->get_attached_dsgs()                 == dsgs                 &&
            current_pipeline_layout_ptr->get_attached_push_constant_ranges() == push_constant_ranges)
        {
            *out_pipeline_layout_ptr_ptr = current_pipeline_layout_ptr;
            result                       = true;

            current_pipeline_layout_ptr->retain();
            break;
        }
    }

    if (!result)
    {
        result = true;

        /* Try to create a new layout for the specified DSG + push constant range set */
        Anvil::PipelineLayout* new_layout_ptr = new Anvil::PipelineLayout(m_device_ptr);

        for (auto dsg_iterator  = dsgs.begin();
                  dsg_iterator != dsgs.end();
                ++dsg_iterator)
        {
            result = new_layout_ptr->attach_dsg(*dsg_iterator);

            if (!result)
            {
                goto end;
            }
        }

        for (auto push_constant_range_iterator  = push_constant_ranges.begin();
                  push_constant_range_iterator != push_constant_ranges.end();
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

        /* Sign up for call-backs so we can release the layout instance when the last user of the layout
         * releases the layout instance.
         **/
        new_layout_ptr->register_for_callbacks(PIPELINE_LAYOUT_CALLBACK_ID_OBJECT_ABOUT_TO_BE_RELEASED_BUT_NOT_DELETED,
                                               on_pipeline_layout_dropped,
                                               this);

        if (result)
        {
            new_layout_ptr->bake();
            new_layout_ptr->retain();

            m_pipeline_layouts.push_back(new_layout_ptr);

            *out_pipeline_layout_ptr_ptr = new_layout_ptr;
        }
    }
end:
    return result;
}

/** Called back whenever a pipeline layout is released, but its reference counter is larger than 2, which indicates
 *  there are other users of the layout. As a result, the pipeline layout instance is not going to be physically
 *  released yet.
 **/
void Anvil::PipelineLayoutManager::on_pipeline_layout_dropped(void* callback_arg,
                                                              void* user_arg)
{
    PipelineLayouts::iterator     layout_iterator;
    PipelineLayout*               layout_ptr         = static_cast<PipelineLayout*>       (callback_arg);
    Anvil::PipelineLayoutManager* layout_manager_ptr = static_cast<PipelineLayoutManager*>(user_arg);

    /* Are we the last standing layout user?
     *
     * NOTE: Reason we're not comparing against 1 is because the callback occurs prior to the reference counter
     *       of the object getting decremented.
     */
    if (layout_ptr->get_ref_counter() != 2)
    {
        goto end;
    }

    /* Yeah. Wipe out the instance from all records. */
    layout_iterator = std::find(layout_manager_ptr->m_pipeline_layouts.begin(),
                                layout_manager_ptr->m_pipeline_layouts.end(),
                                layout_ptr);

    if (layout_iterator != layout_manager_ptr->m_pipeline_layouts.end() )
    {
        layout_manager_ptr->m_pipeline_layouts.erase(layout_iterator);

        layout_ptr->release();
    }

end:
    ;
}

/* Please see header for specification */
void Anvil::PipelineLayoutManager::release()
{
    auto iterator = m_instance_map.find(m_device_ptr);

    anvil_assert(iterator                      != m_instance_map.end() );
    anvil_assert(iterator->second.ref_counter  != 0 &&
                iterator->second.instance_ptr != nullptr);

    --iterator->second.ref_counter;

    if (iterator->second.ref_counter == 0)
    {
        delete iterator->second.instance_ptr;

        m_instance_map.erase(iterator);
    }
}
