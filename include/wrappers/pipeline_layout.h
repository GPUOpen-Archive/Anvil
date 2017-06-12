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
#include "misc/debug_marker.h"
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

        PIPELINE_LAYOUT_CALLBACK_ID_COUNT
    } PipelineLayoutCallbackID;

    /** Vulkan Pipeline Layout wrapper */
    class PipelineLayout : public CallbacksSupportProvider,
                           public DebugMarkerSupportProvider<PipelineLayout>
    {
    public:
        /* Public functions */

        /** Initializes a new wrapper instance with no descriptor sets or push constant ranges
         *  defined.
         *
         *  Layouts initialized with this constructor are mutable - dsgs and new PC ranges can
         *  be attached anytime.
         *
         *  @param in_device_ptr Device the layout is being created for. Must not be nullptr.
         */
        static std::shared_ptr<PipelineLayout> create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr);

        /** Initializes a new wrapper instance with user-specified descriptor set groups (appended
         *  one after another, in the user-defined order) defined at creation time.
         *
         *  This constructor can be used to initialize immutable pipeline layouts. If @param in_is_immutable
         *  is set to true, attach_dsg() and attach_push_constant_range() calls invoked for such object
         *  will result in a failure.
         *
         *  @param in_device_ptr           Device to use. Must not be nullptr.
         *  @param in_dsg_ptr              Descriptor set group to use for the pipeline layout.
         *  @param in_push_constant_ranges Push constant ranges to define for the pipeline layout.
         *  @param in_is_immutable         true if the wrapper instance should be made immutable; false otherwise.
         *
         **/
        static std::shared_ptr<PipelineLayout> create(std::weak_ptr<Anvil::BaseDevice>           in_device_ptr,
                                                      std::shared_ptr<Anvil::DescriptorSetGroup> in_dsg_ptr,
                                                      const PushConstantRanges&                  in_push_constant_ranges,
                                                      bool                                       in_is_immutable);

        /** Destructor. Releases all attached descriptor set groups, as well as
         *  the Vulkan pipeline layout object.
         **/
        virtual ~PipelineLayout();

        /** Appends a new push constant range to the list of push constant ranges that will be used
         *  when baking the layout object.
         *
         *  This function will fail if the instance is defined as immutable.
         *
         *  @param in_layout_id ID of the pipeline layout to perform the operation on. The ID must have
         *                      been returned by a preceding create_layout() call.
         *  @param in_offset    Start offset of the new range.
         *  @param in_size      Size of the new range.
         *
         *  @return true if the function succeeded, false otherwise.
         **/
        bool attach_push_constant_range(uint32_t           in_offset,
                                        uint32_t           in_size,
                                        VkShaderStageFlags in_stages);

        /** Bakes a Vulkan VkPipelineLayout instance from the object.
         *
         *  Bake requests for wrappers not marked as dirty will be ignored.
         *
         *  @return true if successful, false otherwise.
         **/
        bool bake();

        /** Retrieves a descriptor set group, as assigned to the pipeline layout. */
        std::shared_ptr<Anvil::DescriptorSetGroup> get_attached_dsg() const
        {
            return m_dsg_ptr;
        }

        /** Retrieves a vector of push constant ranges, attached to the pipeline layout. */
        const PushConstantRanges& get_attached_push_constant_ranges() const
        {
            return m_push_constant_ranges;
        }

        /** Returns unique ID assigned to the pipeline layout instance */
        PipelineLayoutID get_id() const
        {
            return m_id;
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

        /** Assigns the specified Descriptor Set Group to the pipeline layout.
         *
         *  This function will fail if the instance is defined as immutable.
         *
         *  This function marks the pipeline layout as dirty, meaning it will be re-baked at
         *  the next get_() call.
         *
         *  @param in_dsg_ptr Pointer to the DescriptorSetGroup instance to use for the operation.
         *                    This object will be retained.
         *
         *  @return true if the operation was successful, false otherwise.
         **/
        bool set_dsg(std::shared_ptr<DescriptorSetGroup> in_dsg_ptr);

    private:
        /* Private functions */

        /* Constructor. Please see create() for specification */
        PipelineLayout(std::weak_ptr<Anvil::BaseDevice> in_device_ptr);

        /* Constructor. Please see create() for specification */
        PipelineLayout(std::weak_ptr<Anvil::BaseDevice>           in_device_ptr,
                       std::shared_ptr<Anvil::DescriptorSetGroup> in_dsg_ptr,
                       const PushConstantRanges&                  in_push_constant_ranges,
                       bool                                       in_is_immutable);

        PipelineLayout           (const PipelineLayout&);
        PipelineLayout& operator=(const PipelineLayout&);

        /* Private variables */
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        bool                             m_is_immutable;

        bool                                       m_dirty;
        std::shared_ptr<Anvil::DescriptorSetGroup> m_dsg_ptr;
        Anvil::PipelineLayoutID                    m_id;
        VkPipelineLayout                           m_layout_vk;
        PushConstantRanges                         m_push_constant_ranges;
    };

}; /* namespace Anvil */

#endif /* WRAPPERS_PIPELINE_LAYOUT_H */