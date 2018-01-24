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
Anvil::PipelineLayoutManager::PipelineLayoutManager(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                    bool                             in_mt_safe)
    :MTSafetySupportProvider(in_mt_safe),
     m_device_ptr           (in_device_ptr)
{
    update_subscriptions(true);

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_PIPELINE_LAYOUT_MANAGER,
                                                  this);
}

/** Destructor */
Anvil::PipelineLayoutManager::~PipelineLayoutManager()
{
    anvil_assert(m_pipeline_layouts.size() == 0);

    update_subscriptions(false);

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_PIPELINE_LAYOUT_MANAGER,
                                                    this);
}

/* Please see header for specification */
std::shared_ptr<Anvil::PipelineLayoutManager> Anvil::PipelineLayoutManager::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                                   bool                             in_mt_safe)
{
    std::shared_ptr<Anvil::BaseDevice>            device_locked_ptr(in_device_ptr);
    std::shared_ptr<Anvil::PipelineLayoutManager> result_ptr;

    result_ptr.reset(
        new Anvil::PipelineLayoutManager(in_device_ptr,
                                         in_mt_safe)
    );

    anvil_assert(result_ptr != nullptr);

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::PipelineLayoutManager::get_layout(std::shared_ptr<const DescriptorSetGroup> in_dsg_ptr,
                                              const PushConstantRanges&                 in_push_constant_ranges,
                                              std::shared_ptr<Anvil::PipelineLayout>*   out_pipeline_layout_ptr_ptr)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();
    bool                                   result     = false;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    for (auto layout_iterator  = m_pipeline_layouts.begin();
              layout_iterator != m_pipeline_layouts.end();
            ++layout_iterator)
    {
        auto current_pipeline_layout_ptr = *layout_iterator;

        if (current_pipeline_layout_ptr->get_attached_dsg()                  == in_dsg_ptr              &&
            current_pipeline_layout_ptr->get_attached_push_constant_ranges() == in_push_constant_ranges)
        {
            *out_pipeline_layout_ptr_ptr = current_pipeline_layout_ptr->shared_from_this();
            result                       = true;

            break;
        }
    }

    if (!result)
    {
        result = true;

        std::shared_ptr<Anvil::PipelineLayout> new_layout_ptr = Anvil::PipelineLayout::create(m_device_ptr,
                                                                                              in_dsg_ptr,
                                                                                              in_push_constant_ranges,
                                                                                              is_mt_safe() );

        m_pipeline_layouts.push_back(new_layout_ptr.get() );

        *out_pipeline_layout_ptr_ptr = new_layout_ptr;
    }

    return result;
}

/** Called back whenever a pipeline layout is released **/
void Anvil::PipelineLayoutManager::on_pipeline_layout_dropped(CallbackArgument* in_callback_arg_raw_ptr)
{
    auto                                   callback_arg_ptr = dynamic_cast<Anvil::OnObjectAboutToBeUnregisteredCallbackArgument*>(in_callback_arg_raw_ptr);
    PipelineLayouts::iterator              layout_iterator;
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    layout_iterator = std::find(m_pipeline_layouts.begin(),
                                m_pipeline_layouts.end  (),
                                callback_arg_ptr->object_raw_ptr);

    anvil_assert(layout_iterator != m_pipeline_layouts.end() );
    if (layout_iterator != m_pipeline_layouts.end() )
    {
        m_pipeline_layouts.erase(layout_iterator);
    }
}

void Anvil::PipelineLayoutManager::update_subscriptions(bool in_should_init)
{
    const auto callback_func      = std::bind(&PipelineLayoutManager::on_pipeline_layout_dropped,
                                              this,
                                              std::placeholders::_1);
    void*      callback_owner     = this;
    auto       object_tracker_ptr = Anvil::ObjectTracker::get();

    if (in_should_init)
    {
        object_tracker_ptr->register_for_callbacks(OBJECT_TRACKER_CALLBACK_ID_ON_PIPELINE_LAYOUT_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                                                   callback_func,
                                                   callback_owner);
    }
    else
    {
        object_tracker_ptr->unregister_from_callbacks(OBJECT_TRACKER_CALLBACK_ID_ON_PIPELINE_LAYOUT_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                                                      callback_func,
                                                      callback_owner);
    }
}
