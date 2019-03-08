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
 *  Object Tracker is thread-safe.
 **/
#ifndef MISC_OBJECT_TRACKER_H
#define MISC_OBJECT_TRACKER_H

#include <unordered_map>
#include <vector>
#include "misc/callbacks.h"
#include "misc/types.h"

namespace Anvil
{
    typedef enum
    {
        /* Callback issued when a new GLSLShaderToSPIRVGenerator object is instantiated
         *
         * @param callback_arg OnObjectRegisteredCallbackArgument structure instance
         */
        OBJECT_TRACKER_CALLBACK_ID_ON_GLSL_SHADER_TO_SPIRV_GENERATOR_OBJECT_REGISTERED,

        /* Callback issued when a new ShaderModule object is instantiated
         *
         * @param callback_arg OnObjectRegisteredCallbackArgument structure instance
         */
        OBJECT_TRACKER_CALLBACK_ID_ON_SHADER_MODULE_OBJECT_REGISTERED,

        /* Callback issued when an existing Device object instance is about to go out of scope.
         *
         * This callback IS issued BEFORE a corresponding Vulkan handle is destroyed.
         *
         * This callback MAY be issued FROM WITHIN the object's destructor, implying all WEAK POINTERS pointing
         * to the wrapper instance will have been expired at the time of the callback.
         *
         * @param callback_arg OnObjectAboutToBeUnregisteredCallbackArgument structure instance
         **/
        OBJECT_TRACKER_CALLBACK_ID_ON_DEVICE_OBJECT_ABOUT_TO_BE_UNREGISTERED,

        /* Callback issued when an existing GLSLShaderToSPIRVGenerator object instance is about to go out of scope.
         *
         * This callback IS issued BEFORE a corresponding Vulkan handle is destroyed.
         *
         * This callback MAY be issued FROM WITHIN the object's destructor, implying all WEAK POINTERS pointing
         * to the wrapper instance will have been expired at the time of the callback.
         *
         * @param callback_arg OnObjectAboutToBeUnregisteredCallbackArgument structure instance
         **/
        OBJECT_TRACKER_CALLBACK_ID_ON_GLSL_SHADER_TO_SPIRV_GENERATOR_OBJECT_ABOUT_TO_BE_UNREGISTERED,

        /* Callback issued when an existing PipelineLayout object instance is about to go out of scope.
         *
         * This callback IS issued BEFORE a corresponding Vulkan handle is destroyed.
         *
         * This callback MAY be issued FROM WITHIN the object's destructor, implying all WEAK POINTERS pointing
         * to the wrapper instance will have been expired at the time of the callback.
         *
         * @param callback_arg OnObjectAboutToBeUnregisteredCallbackArgument structure instance
         **/
        OBJECT_TRACKER_CALLBACK_ID_ON_PIPELINE_LAYOUT_OBJECT_ABOUT_TO_BE_UNREGISTERED,

        /* Callback issued when an existing ShaderModule object instance is about to go out of scope.
         *
         * This callback IS issued BEFORE a corresponding Vulkan handle is destroyed.
         *
         * This callback MAY be issued FROM WITHIN the object's destructor, implying all WEAK POINTERS pointing
         * to the wrapper instance will have been expired at the time of the callback.
         *
         * @param callback_arg OnObjectAboutToBeUnregisteredCallbackArgument structure instance
         **/
        OBJECT_TRACKER_CALLBACK_ID_ON_SHADER_MODULE_OBJECT_ABOUT_TO_BE_UNREGISTERED,

        OBJECT_TRACKER_CALLBACK_ID_COUNT,
    } ObjectTrackerCallbackID;

    class ObjectTracker : public CallbacksSupportProvider
    {
    public:
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

        /** Retrieves an alive object of user-specified type at given index. */
        void* get_object_at_index(const ObjectType& in_object_type,
                                  uint32_t          in_alloc_index) const;

        /** Registers a new object of the specified type.
         *
         *  @param in_object_type Wrapper object type.
         *  @param in_object_ptr  Object instance. The object is NOT retained.
         **/
        void register_object(const ObjectType& in_object_type,
                             void*             in_object_ptr);

        /** Stops tracking the specified object.
         *
         *  For Vulkan object wrappers, this function MUST be called prior to actual release of the Vulkan object!
         *
         *  @param in_object_type Wrapper object type.
         *  @param in_object_ptr  Object instance. The object is NOT released. The object must have
         *                        been registered earlier with a register_object() call, or else an
         *                        assertion failure will occur.
         **/
        void unregister_object(const ObjectType& in_object_type,
                               void*             in_object_ptr);

    private:
        /* Private type declarations */
        typedef uint64_t ObjectVkHandle;

        typedef struct ObjectAllocation
        {
            uint32_t n_allocation;
            void*    object_ptr;

            /** Dummy constructor. Should only be used by STL */
            ObjectAllocation()
            {
                n_allocation = UINT32_MAX;
                object_ptr   = nullptr;
            }

            /** Constructor.
             *
             *  @param in_n_allocation Index of the memory allocation.
             *  @param in_object_ptr   Pointer to the object.
             */
            ObjectAllocation(uint32_t in_n_allocation,
                             void*    in_object_ptr)
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

        const char* get_object_type_name(const ObjectType& in_object_type) const;

        /* Private members */
        mutable std::mutex m_cs;

        mutable std::map<Anvil::ObjectType, ObjectAllocations>  m_object_allocations;
        std::map<Anvil::ObjectType, uint32_t>                   m_n_objects_allocated_array;
    };
}; /* namespace Anvil */

#endif /* MISC_OBJECT_TRACKER_H */