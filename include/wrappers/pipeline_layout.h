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

/** Implements a wrapper for a single Vulkan pipeline layout. Implemented in order to:
 *
 *  - encapsulate all state related to a single ipeline layout.
 *  - let ObjectTracker detect leaking pipeline layout wrapper instances.
 *
 *  The wrapper is thread-safe on an opt-in basis.
 **/
#ifndef WRAPPERS_PIPELINE_LAYOUT_H
#define WRAPPERS_PIPELINE_LAYOUT_H

#include "misc/callbacks.h"
#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    typedef std::vector<PushConstantRange> PushConstantRanges;

    /** Vulkan Pipeline Layout wrapper */
    class PipelineLayout : public DebugMarkerSupportProvider<PipelineLayout>,
                           public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Destructor. Releases all attached descriptor set groups, as well as
         *  the Vulkan pipeline layout object.
         **/
        virtual ~PipelineLayout();

        /** Retrieves a vector of push constant ranges, attached to the pipeline layout. */
        const PushConstantRanges& get_attached_push_constant_ranges() const
        {
            return m_push_constant_ranges;
        }

        const std::vector<Anvil::DescriptorSetCreateInfoUniquePtr>* get_ds_create_info_ptrs() const
        {
            return &m_ds_create_info_ptrs;
        }

        /** Retrieves a raw Vulkan pipeline layout handle.
         *
         *  This getter will automatically invoke bake(), if the wrapper instance is marked as dirty.
         *
         *  @return Requested handle.
         **/
        VkPipelineLayout get_pipeline_layout() const
        {
            return m_layout_vk;
        }

    private:
        /* Private functions */

        /* Constructor. Please see create() for specification */
        PipelineLayout(const Anvil::BaseDevice*   in_device_ptr,
                       const PushConstantRanges&  in_push_constant_ranges,
                       bool                       in_mt_safe);

        PipelineLayout           (const PipelineLayout&);
        PipelineLayout& operator=(const PipelineLayout&);

        /** Bakes a Vulkan VkPipelineLayout instance from the object.
         *
         *  @return true if successful, false otherwise.
         **/
        bool bake(const std::vector<DescriptorSetCreateInfoUniquePtr>* in_ds_create_info_items_ptr);

        /** Initializes a new wrapper instance using information extracted from user-specified descriptor set groups (appended
         *  one after another, in the user-defined order).
         *
         *  This constructor can be used to initialize immutable pipeline layouts. If @param in_is_immutable
         *  is set to true, attach_dsg() and attach_push_constant_range() calls invoked for such object
         *  will result in a failure.
         *
         *  @param in_device_ptr               Device to use. Must not be nullptr.
         *  @param in_ds_create_info_items_ptr TODO
         *  @param in_push_constant_ranges     Push constant ranges to define for the pipeline layout.
         *
         **/
        static PipelineLayoutUniquePtr create(const Anvil::BaseDevice*                             in_device_ptr,
                                              const std::vector<DescriptorSetCreateInfoUniquePtr>* in_ds_create_info_items_ptr,
                                              const PushConstantRanges&                            in_push_constant_ranges,
                                              bool                                                 in_mt_safe);

        /* Private variables */
        const Anvil::BaseDevice*                             m_device_ptr;
        std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> m_ds_create_info_ptrs;
        std::vector<Anvil::DescriptorSetLayoutUniquePtr>     m_ds_layout_ptrs;
        VkPipelineLayout                                     m_layout_vk;
        PushConstantRanges                                   m_push_constant_ranges;

        friend class PipelineLayoutManager;
    };

}; /* namespace Anvil */

#endif /* WRAPPERS_PIPELINE_LAYOUT_H */