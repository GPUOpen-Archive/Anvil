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

/** Implements a wrapper for a single Vulkan pipeline layout. Implemented in order to:
 *
 *  - encapsulate all state related to a single ipeline layout.
 *  - let ObjectTracker detect leaking pipeline layout wrapper instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_PIPELINE_LAYOUT_H
#define WRAPPERS_PIPELINE_LAYOUT_H

#include "misc/callbacks.h"
#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
    /* Forward declarations */
    class DescriptorSetGroup;

    typedef std::vector<PushConstantRange> PushConstantRanges;

    typedef enum
    {
        /* Notification fired when a pipeline layout instance is about to be deleted.
         *
         * callback_arg: originating PipelineLayout instance ptr.
         */
        PIPELINE_LAYOUT_CALLBACK_ID_OBJECT_ABOUT_TO_BE_DELETED,

        /* Notification fired when a pipeline layout instance is about to be released,
         * but there are objects still referring to the object, so the object will not
         * be deleted at callback time.
         *
         * This call-back occurs BEFORE the reference counter is decremented.
         *
         * callback_arg: originating PipelineLayout instance ptr
         */
        PIPELINE_LAYOUT_CALLBACK_ID_OBJECT_ABOUT_TO_BE_RELEASED_BUT_NOT_DELETED,

        PIPELINE_LAYOUT_CALLBACK_ID_COUNT
    } PipelineLayoutCallbackID;

    /** Vulkan Pipeline Layout wrapper */
    class PipelineLayout : public CallbacksSupportProvider,
                           public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Initializes a new wrapper instance with no descriptor sets or push constant ranges
         *  defined.
         *
         *  Layouts initialized with this constructor are mutable - dsgs and new PC ranges can
         *  be attached anytime.
         *
         *  @param device_ptr Device the layout is being created for. Must not be nullptr.
         */
        PipelineLayout(Anvil::Device* device_ptr);

        /** Initializes a new wrapper instance with user-specified descriptor set groups (appended
         *  one after another, in the user-defined order) defined at creation time.
         *
         *  This constructor can be used to initialize immutable pipeline layouts. If @param is_immutable
         *  is set to true, attach_dsg() and attach_push_constant_range() calls invoked for such object
         *  will result in a failure.
         *
         *  @param device_ptr           Device to use. Must not be nullptr.
         *  @param dsgs                 Descriptor set groups to define for the pipeline layout.
         *  @param push_constant_ranges Push constant ranges to define for the pipeline layout.
         *  @param is_immutable         true if the wrapper instance should be made immutable; false otherwise.
         *
         **/
        PipelineLayout(Anvil::Device*             device_ptr,
                       const DescriptorSetGroups& dsgs,
                       const PushConstantRanges&  push_constant_ranges,
                       bool                       is_immutable);

        /** Appends the specified Descriptor Set Group to the list of Descriptor Sets that will
         *  be used to generate the descriptor set layout.
         *
         *  This function will fail if the instance is defined as immutable.
         *
         *  This function marks the pipeline layout as dirty, meaning it will be re-baked at
         *  the next get_() call.
         *
         *  @param layout_id ID of the pipeline layout to perform the operation on. This
         *                   ID must have been returned by a preceding create_layout() call.
         *  @param dsg_ptr   Pointer to the DescriptorSetGroup instance to use for the operation.
         *                   This object will be retained.
         *
         *  @return true if the operation was successful, false otherwise.
         **/
        bool attach_dsg(DescriptorSetGroup* dsg_ptr);

        /** Appends a new push constant range to the list of push constant ranges that will be used
         *  when baking the layout object.
         *
         *  This function will fail if the instance is defined as immutable.
         *
         *  @param layout_id ID of the pipeline layout to perform the operation on. The ID must have
         *                   been returned by a preceding create_layout() call.
         *  @param offset    Start offset of the new range.
         *  @param size      Size of the new range.
         *
         *  @return true if the function succeeded, false otherwise.
         **/
        bool attach_push_constant_range(uint32_t           offset,
                                        uint32_t           size,
                                        VkShaderStageFlags stages);

        /** Bakes a Vulkan VkPipelineLayout instance from the object.
         *
         *  Bake requests for wrappers not marked as dirty will be ignored.
         *
         *  @return true if successful, false otherwise.
         **/
        bool bake();

        /** Retrieves a vector of descriptor set groups, attached to the pipeline layout. **/
        const DescriptorSetGroups& get_attached_dsgs() const
        {
            return m_dsgs;
        }

        /** Retrieves a vector of push constant ranges, attached to the pipeline layout. */
        const PushConstantRanges& get_attached_push_constant_ranges() const
        {
            return m_push_constant_ranges;
        }

        /** Retrieves a raw Vulkan pipeline layout handle.
         *
         *  This getter will automatically invoke bake(), if the wrapper instance is marked as dirty.
         *
         *  @return Requested handle.
         **/
        VkPipelineLayout get_pipeline_layout()
        {
            if (m_dirty)
            {
                bake();
            }

            return m_layout_vk;
        }

        /** Decrements internal reference counter and releases the object, once the counter drops to zero. */
        virtual void release();

    private:
        /* Private functions */
        PipelineLayout           (const PipelineLayout&);
        PipelineLayout& operator=(const PipelineLayout&);

        /** Destructor. Releases all attached descriptor set groups, as well as
         *  the Vulkan pipeline layout object.
         **/
        virtual ~PipelineLayout();

        /* Private variables */
        Anvil::Device* m_device_ptr;
        bool           m_is_immutable;

        bool                m_dirty;
        DescriptorSetGroups m_dsgs;
        VkPipelineLayout    m_layout_vk;
        PushConstantRanges  m_push_constant_ranges;
    };

}; /* namespace Anvil */

#endif /* WRAPPERS_PIPELINE_LAYOUT_H */