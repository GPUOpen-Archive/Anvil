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

/** A destroyable singleton which keeps track of Vulkan wrapper object allocations and helps
 *  track leaking of ref-countable object instances.
 *
 *  The first ObjectTracker::get() call will instantiate the singleton. Any further get() calls
 *  will return the same handle until a ObjectTracker::destroy() call is made, which releases
 *  the tracker. After the tracker is destroyed, any subsequent get() call will create a new
 *  tracker instance.
 *
 *  Each object allocation is registered by the tracker. At any time, the application can call
 *  ObjectTracker::check_for_leaks() to determine, if there are any wrapper objects alive. If so,
 *  brief info on each such instance will be printed out to stdout.
 *
 *  Object Tracker is not thread-safe at the moment.
 **/
#ifndef MISC_OBJECT_TRACKER_H
#define MISC_OBJECT_TRACKER_H

#include <vector>
#include "misc/types.h"

namespace Anvil
{
    class ObjectTracker
    {
    public:
        /* Public type declarations */
        typedef enum
        {
            /* NOTE: If new entries are added or existing entry order is modified, make sure to
             *       update get_object_type_name().
             */
            OBJECT_TYPE_FIRST,

            OBJECT_TYPE_BUFFER = OBJECT_TYPE_FIRST,
            OBJECT_TYPE_BUFFER_VIEW,
            OBJECT_TYPE_COMMAND_BUFFER,
            OBJECT_TYPE_COMMAND_POOL,
            OBJECT_TYPE_COMPUTE_PIPELINE_MANAGER,
            OBJECT_TYPE_DESCRIPTOR_POOL,
            OBJECT_TYPE_DESCRIPTOR_SET,
            OBJECT_TYPE_DESCRIPTOR_SET_GROUP,
            OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
            OBJECT_TYPE_DEVICE,
            OBJECT_TYPE_EVENT,
            OBJECT_TYPE_FENCE,
            OBJECT_TYPE_FRAMEBUFFER,
            OBJECT_TYPE_GRAPHICS_PIPELINE_MANAGER,
            OBJECT_TYPE_IMAGE,
            OBJECT_TYPE_IMAGE_VIEW,
            OBJECT_TYPE_INSTANCE,
            OBJECT_TYPE_MEMORY_BLOCK,
            OBJECT_TYPE_PHYSICAL_DEVICE,
            OBJECT_TYPE_PIPELINE_CACHE,
            OBJECT_TYPE_PIPELINE_LAYOUT,
            OBJECT_TYPE_PIPELINE_LAYOUT_MANAGER,
            OBJECT_TYPE_QUERY_POOL,
            OBJECT_TYPE_QUEUE,
            OBJECT_TYPE_RENDER_PASS,
            OBJECT_TYPE_RENDERING_SURFACE,
            OBJECT_TYPE_SAMPLER,
            OBJECT_TYPE_SEMAPHORE,
            OBJECT_TYPE_SHADER_MODULE,
            OBJECT_TYPE_SWAPCHAIN,

            /* Always last */
            OBJECT_TYPE_COUNT
        } ObjectType;

        /* Public functions */

        /** Destroys the ObjectTracker singleton, no matter how many preceding get() calls have been made. */
        static void destroy();

        /** When called for the first time, or following a destroy() call, the function will create
         *  a new ObjectTracker instance. Otherwise, it will return the cached pointer to a living
         *  tracker instance.
         *
         *  @return As per description.
         **/
        static ObjectTracker* get();

        /** Iterates over all wrapper object types and, for each type, prints out info about object instances
         *  which have not been released yet.
         **/
        void check_for_leaks() const;

        /** Registers a new object of the specified type.
         *
         *  @param object_type Wrapper object type.
         *  @param object_ptr  Object instance. The object is NOT retained.
         **/
        void register_object(ObjectType object_type,
                             void*      object_ptr);

        /** Stops tracking the specified object.
         *
         *  @param object_type Wrapper object type.
         *  @param object_ptr  Object instance. The object is NOT released. The object must have
         *                     been registered earlier with a register_object() call, or else an
         *                     assertion failure will occur.
         **/
        void unregister_object(ObjectType object_type,
                               void*      object_ptr);

    private:
        /* Private type declarations */
        typedef uint64_t ObjectVkHandle;

        typedef struct ObjectAllocation
        {
            uint32_t    n_allocation;
            const void* object_ptr;

            /** Dummy constructor. Should only be used by STL */
            ObjectAllocation()
            {
                n_allocation = -1;
                object_ptr   = nullptr;
            }

            /** Constructor.
             *
             *  @param in_n_allocation Index of the memory allocation.
             *  @param in_object_ptr   Pointer to the object.
             */
            ObjectAllocation(uint32_t    in_n_allocation,
                             const void* in_object_ptr)
            {
                n_allocation = in_n_allocation;
                object_ptr   = in_object_ptr;
            }

            /** Comparator. Will return true if the wrapped pointer matches @param in_object_ptr. */
            bool operator==(const void* in_object_ptr) const
            {
                return object_ptr == in_object_ptr;
            }
        } ObjectAllocation;

        typedef std::vector<ObjectAllocation> ObjectAllocations;

        /* Private functions */
        ObjectTracker           ();
        ObjectTracker           (const ObjectTracker&);
        ObjectTracker& operator=(const ObjectTracker&);

        const char* get_object_type_name(ObjectType object_type) const;

        /* Private members */
        ObjectAllocations m_object_allocations       [OBJECT_TYPE_COUNT];
        uint32_t          m_n_objects_allocated_array[OBJECT_TYPE_COUNT];
    };
}; /* namespace Anvil */

#endif /* MISC_OBJECT_TRACKER_H */