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
#include "misc/object_tracker.h"
#include <algorithm>


static Anvil::ObjectTracker* object_tracker_ptr = nullptr;


/** Constructor. */
Anvil::ObjectTracker::ObjectTracker()
    :CallbacksSupportProvider(OBJECT_TRACKER_CALLBACK_ID_COUNT)
{
    memset(m_n_objects_allocated_array,
           0,
           sizeof(m_n_objects_allocated_array) );
}

/* Please see header for specification */
void Anvil::ObjectTracker::destroy()
{
    delete object_tracker_ptr;

    object_tracker_ptr = nullptr;
}

/* Please see header for specification */
Anvil::ObjectTracker* Anvil::ObjectTracker::get()
{
    if (object_tracker_ptr == nullptr)
    {
        object_tracker_ptr = new ObjectTracker();
    }

    return object_tracker_ptr;
}

/* Please see header for specification */
void Anvil::ObjectTracker::check_for_leaks() const
{
    std::unique_lock<std::mutex> lock(m_cs);

    for (ObjectType current_object_type = OBJECT_TYPE_FIRST;
                    current_object_type < OBJECT_TYPE_COUNT;
                    current_object_type = static_cast<ObjectType>(current_object_type + 1))
    {
        const uint32_t n_object_allocations = (uint32_t) m_object_allocations[current_object_type].size();

        if (n_object_allocations > 0)
        {
            fprintf(stdout,
                    "The following %s instances have not been released:\n",
                    get_object_type_name(current_object_type) );

            for (uint32_t n_allocation = 0;
                          n_allocation < n_object_allocations;
                        ++n_allocation)
            {
                fprintf(stdout,
                        "[%d]. %p\n",
                        m_object_allocations[current_object_type][n_allocation].n_allocation,
                        m_object_allocations[current_object_type][n_allocation].object_ptr);
            }

            fprintf(stdout,
                    "\n");
        }
    }
}

/** Converts @param object_type enum to a null-terminated string.
 *
 *  @param object_type Internal wrapper object type to return the string for.
 *
 *  @return As per description.
 **/
const char* Anvil::ObjectTracker::get_object_type_name(ObjectType in_object_type) const
{
    static const char* result_array[] =
    {
        "Buffer",
        "Buffer View",
        "Command Buffer",
        "Command Pool",
        "Compute Pipeline Manager",
        "Descriptor Pool",
        "Descriptor Set",
        "Descriptor Set Group",
        "Descriptor Set Layout",
        "Descriptor Set Layout Manager",
        "Descriptor Update Template",
        "Device",
        "Event",
        "Fence",
        "Framebuffer",
        "Graphics Pipeline Manager",
        "Image",
        "Image View",
        "Instance",
        "Memory Block",
        "Physical Device",
        "Pipeline Cache",
        "Pipeline Layout",
        "Pipeline Layout Manager",
        "Query Pool",
        "Queue",
        "Render Pass",
        "Rendering Surface",
        "Sampler",
        "Semaphore",
        "Shader Module",
        "Swapchain",

        "GLSL shader -> SPIR-V generator",
        "Graphics Pipeline (fake)"
    };

    static_assert(sizeof(result_array) / sizeof(result_array[0]) == OBJECT_TYPE_COUNT,
                  "Number of object types change detected - update result_array.");

    return result_array[in_object_type];
}

/* Please see header for specification */
void* Anvil::ObjectTracker::get_object_at_index(ObjectType in_object_type,
                                                uint32_t   in_alloc_index) const
{
    std::unique_lock<std::mutex> lock (m_cs);
    void*                        result(nullptr);

    anvil_assert(in_object_type < OBJECT_TYPE_COUNT);

    if (m_object_allocations[in_object_type].size() > in_alloc_index)
    {
        result = m_object_allocations[in_object_type][in_alloc_index].object_ptr;
    }

    return result;
}

/* Please see header for specification */
void Anvil::ObjectTracker::register_object(ObjectType in_object_type,
                                           void*      in_object_ptr)
{
    anvil_assert(in_object_ptr  != nullptr);
    anvil_assert(in_object_type <  OBJECT_TYPE_COUNT);

    {
        std::unique_lock<std::mutex> lock(m_cs);

        m_object_allocations[in_object_type].push_back(ObjectAllocation(m_n_objects_allocated_array[in_object_type]++,
                                                                        in_object_ptr) );
    }

    /* Notify any observers about the new object */
    OnObjectRegisteredCallbackArgument callback_arg(in_object_type,
                                                    in_object_ptr);

    if (in_object_type == OBJECT_TYPE_GLSL_SHADER_TO_SPIRV_GENERATOR)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_GLSL_SHADER_TO_SPIRV_GENERATOR_OBJECT_REGISTERED,
                     &callback_arg);
    }
    else
    if (in_object_type == OBJECT_TYPE_SHADER_MODULE)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_SHADER_MODULE_OBJECT_REGISTERED,
                     &callback_arg);
    }
}

/* Please see header for specification */
void Anvil::ObjectTracker::unregister_object(ObjectType in_object_type,
                                             void*      in_object_ptr)
{
    OnObjectAboutToBeUnregisteredCallbackArgument callback_arg(in_object_type,
                                                               in_object_ptr);

    {
        std::unique_lock<std::mutex> lock                      (m_cs);
        auto                         object_allocation_iterator(std::find(m_object_allocations[in_object_type].begin(),
                                                                          m_object_allocations[in_object_type].end(),
                                                                          in_object_ptr) );

        if (object_allocation_iterator == m_object_allocations[in_object_type].end() )
        {
            anvil_assert_fail();

            goto end;
        }

        m_object_allocations[in_object_type].erase(object_allocation_iterator);
    }

    /* Notify any observers about the event. */
    if (in_object_type == OBJECT_TYPE_DEVICE)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_DEVICE_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                     &callback_arg);
    }
    else
    if (in_object_type == OBJECT_TYPE_GLSL_SHADER_TO_SPIRV_GENERATOR)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_GLSL_SHADER_TO_SPIRV_GENERATOR_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                     &callback_arg);
    }
    else
    if (in_object_type == OBJECT_TYPE_PIPELINE_LAYOUT)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_PIPELINE_LAYOUT_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                     &callback_arg);
    }
    else
    if (in_object_type == OBJECT_TYPE_SHADER_MODULE)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_SHADER_MODULE_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                     &callback_arg);
    }

end:
    ;
}