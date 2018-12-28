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
    /* Stub */
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

    for (const auto& current_object_type_alloc_data : m_object_allocations)
    {
        const uint32_t n_object_allocations = static_cast<uint32_t>(current_object_type_alloc_data.second.size() );

        if (n_object_allocations > 0)
        {
            fprintf(stdout,
                    "The following %s instances have not been released:\n",
                    get_object_type_name(current_object_type_alloc_data.first) );

            for (const auto& current_alloc : current_object_type_alloc_data.second)
            {
                fprintf(stdout,
                        "[%d]. %p\n",
                        current_alloc.n_allocation,
                        current_alloc.object_ptr);
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
const char* Anvil::ObjectTracker::get_object_type_name(const Anvil::ObjectType& in_object_type) const
{
    const char* result_ptr = "?!";

    switch (in_object_type)
    {
        case Anvil::ObjectType::BUFFER:                     result_ptr = "Buffer";                     break;
        case Anvil::ObjectType::BUFFER_VIEW:                result_ptr = "Buffer View";                break;
        case Anvil::ObjectType::COMMAND_BUFFER:             result_ptr = "Command Buffer";             break;
        case Anvil::ObjectType::COMMAND_POOL:               result_ptr = "Command Pool";               break;
        case Anvil::ObjectType::DESCRIPTOR_POOL:            result_ptr = "Descriptor Pool";            break;
        case Anvil::ObjectType::DESCRIPTOR_SET:             result_ptr = "Descriptor Set";             break;
        case Anvil::ObjectType::DESCRIPTOR_SET_LAYOUT:      result_ptr = "Descriptor Set Layout";      break;
        case Anvil::ObjectType::DESCRIPTOR_UPDATE_TEMPLATE: result_ptr = "Descriptor Update Template"; break;
        case Anvil::ObjectType::DEVICE:                     result_ptr = "Device";                     break;
        case Anvil::ObjectType::EVENT:                      result_ptr = "Event";                      break;
        case Anvil::ObjectType::FENCE:                      result_ptr = "Fence";                      break;
        case Anvil::ObjectType::FRAMEBUFFER:                result_ptr = "Framebuffer";                break;
        case Anvil::ObjectType::IMAGE:                      result_ptr = "Image";                      break;
        case Anvil::ObjectType::IMAGE_VIEW:                 result_ptr = "Image View";                 break;
        case Anvil::ObjectType::INSTANCE:                   result_ptr = "Instance";                   break;
        case Anvil::ObjectType::PHYSICAL_DEVICE:            result_ptr = "Physical Device";            break;
        case Anvil::ObjectType::PIPELINE_CACHE:             result_ptr = "Pipeline Cache";             break;
        case Anvil::ObjectType::PIPELINE_LAYOUT:            result_ptr = "Pipeline Layout";            break;
        case Anvil::ObjectType::QUERY_POOL:                 result_ptr = "Query Pool";                 break;
        case Anvil::ObjectType::QUEUE:                      result_ptr = "Queue";                      break;
        case Anvil::ObjectType::RENDER_PASS:                result_ptr = "Render Pass";                break;
        case Anvil::ObjectType::RENDERING_SURFACE:          result_ptr = "Rendering Surface";          break;
        case Anvil::ObjectType::SAMPLER:                    result_ptr = "Sampler";                    break;
        case Anvil::ObjectType::SEMAPHORE:                  result_ptr = "Semaphore";                  break;
        case Anvil::ObjectType::SHADER_MODULE:              result_ptr = "Shader Module";              break;
        case Anvil::ObjectType::SWAPCHAIN:                  result_ptr = "Swapchain";                  break;

        case Anvil::ObjectType::ANVIL_COMPUTE_PIPELINE_MANAGER:       result_ptr = "Anvil Compute Pipeline Manager";      break;
        case Anvil::ObjectType::ANVIL_DESCRIPTOR_SET_GROUP:           result_ptr = "Anvil Descriptor Set Group";          break;
        case Anvil::ObjectType::ANVIL_DESCRIPTOR_SET_LAYOUT_MANAGER:  result_ptr = "Anvil Descriptor Set Layout Manager"; break;
        case Anvil::ObjectType::ANVIL_GLSL_SHADER_TO_SPIRV_GENERATOR: result_ptr = "Anvil GLSL Shader->SPIRV Generator";  break;
        case Anvil::ObjectType::ANVIL_GRAPHICS_PIPELINE_MANAGER:      result_ptr = "Anvil Graphics Pipeline Manager";     break;
        case Anvil::ObjectType::ANVIL_MEMORY_BLOCK:                   result_ptr = "Anvil Memory Block";                  break;
        case Anvil::ObjectType::ANVIL_PIPELINE_LAYOUT_MANAGER:        result_ptr = "Anvil Pipeline Layout Manager";       break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result_ptr;
}

/* Please see header for specification */
void* Anvil::ObjectTracker::get_object_at_index(const ObjectType& in_object_type,
                                                uint32_t          in_alloc_index) const
{
    std::unique_lock<std::mutex> lock (m_cs);
    void*                        result(nullptr);

    if (m_object_allocations[in_object_type].size() > in_alloc_index)
    {
        result = m_object_allocations[in_object_type][in_alloc_index].object_ptr;
    }

    return result;
}

/* Please see header for specification */
void Anvil::ObjectTracker::register_object(const ObjectType& in_object_type,
                                           void*             in_object_ptr)
{
    anvil_assert(in_object_ptr != nullptr);

    {
        std::unique_lock<std::mutex> lock(m_cs);

        m_object_allocations[in_object_type].push_back(ObjectAllocation(m_n_objects_allocated_array[in_object_type]++,
                                                                        in_object_ptr) );
    }

    /* Notify any observers about the new object */
    OnObjectRegisteredCallbackArgument callback_arg(in_object_type,
                                                    in_object_ptr);

    if (in_object_type == Anvil::ObjectType::ANVIL_GLSL_SHADER_TO_SPIRV_GENERATOR)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_GLSL_SHADER_TO_SPIRV_GENERATOR_OBJECT_REGISTERED,
                     &callback_arg);
    }
    else
    if (in_object_type == Anvil::ObjectType::SHADER_MODULE)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_SHADER_MODULE_OBJECT_REGISTERED,
                     &callback_arg);
    }
}

/* Please see header for specification */
void Anvil::ObjectTracker::unregister_object(const ObjectType& in_object_type,
                                             void*             in_object_ptr)
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
    if (in_object_type == Anvil::ObjectType::DEVICE)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_DEVICE_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                     &callback_arg);
    }
    else
    if (in_object_type == Anvil::ObjectType::ANVIL_GLSL_SHADER_TO_SPIRV_GENERATOR)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_GLSL_SHADER_TO_SPIRV_GENERATOR_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                     &callback_arg);
    }
    else
    if (in_object_type == Anvil::ObjectType::PIPELINE_LAYOUT)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_PIPELINE_LAYOUT_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                     &callback_arg);
    }
    else
    if (in_object_type == Anvil::ObjectType::SHADER_MODULE)
    {
        callback_safe(OBJECT_TRACKER_CALLBACK_ID_ON_SHADER_MODULE_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                     &callback_arg);
    }

end:
    ;
}