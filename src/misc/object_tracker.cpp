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
#include <algorithm>


static Anvil::ObjectTracker* object_tracker_ptr = nullptr;


/** Constructor. */
Anvil::ObjectTracker::ObjectTracker()
{
    memset(m_n_objects_allocated_array,
           0,
           sizeof(m_n_objects_allocated_array) );
}

/* Please see header for specification */
void Anvil::ObjectTracker::destroy()
{
    if (object_tracker_ptr != nullptr)
    {
        delete object_tracker_ptr;

        object_tracker_ptr = nullptr;
    }
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
    for (ObjectType current_object_type = OBJECT_TYPE_FIRST;
                    current_object_type < OBJECT_TYPE_COUNT;
            ++(int&)current_object_type)
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
const char* Anvil::ObjectTracker::get_object_type_name(ObjectType object_type) const
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
        "Swapchain"
    };

    static_assert(sizeof(result_array) / sizeof(result_array[0]) == OBJECT_TYPE_COUNT,
                  "Number of object types change detected - update result_array.");

    return result_array[object_type];
}

/* Please see header for specification */
void Anvil::ObjectTracker::register_object(ObjectType object_type,
                                           void*      object_ptr)
{
    anvil_assert(object_ptr  != nullptr);
    anvil_assert(object_type <  OBJECT_TYPE_COUNT);

    m_object_allocations[object_type].push_back(ObjectAllocation(m_n_objects_allocated_array[object_type]++,
                                                                 object_ptr) );
}

/* Please see header for specification */
void Anvil::ObjectTracker::unregister_object(ObjectType object_type,
                                             void*      object_ptr)
{
    auto object_allocation_iterator = std::find(m_object_allocations[object_type].begin(),
                                                m_object_allocations[object_type].end(),
                                                object_ptr);

    if (object_allocation_iterator == m_object_allocations[object_type].end() )
    {
        anvil_assert(false);

        goto end;
    }

    m_object_allocations[object_type].erase(object_allocation_iterator);

end:
    ;
}