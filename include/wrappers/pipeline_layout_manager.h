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

/** Pipeline layout object manager. A ref-counted singleton which:
 *
 *  - caches all pipeline layout wrappers and re-uses already instantiated wrappers,
 *    if user-requested one is already available.
 *
 **/
#ifndef PIPELINE_LAYOUT_MANAGER_H
#define PIPELINE_LAYOUT_MANAGER_H

#include "misc/types.h"
#include <memory>

namespace Anvil
{
    class PipelineLayoutManager
    {
    public:
        /* Public functions */

        /** Destructor */
        ~PipelineLayoutManager();

        /** Returns a pipeline layout wrapper matching the specified DSG + push constant range configuration.
         *  If such pipeline layout has never been defined before, it will be created at the call time.
         *
         *  If the function returns a Anvil::PipelineLayout instance, it is caller's responsibility to release it
         *  in order for the object to be correctly deleted when its reference counter drops to zero.
         *
         *  @param in_dsg_ptr              A DescriptorSetGroup instance, holding the set of descriptor sets which the
         *                                 layout should describe.
         *  @param in_push_constant_ranges A vector of PushConstantRange descriptor, describing the push constant ranges
         *                                 the layout should define.
         *  @param out_pipeline_layout_ptr Deref will be set to a ptr to the pipeline layout wrapper instance, matching the described
         *                                 requirements. Must not be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_layout(std::shared_ptr<DescriptorSetGroup>     in_dsg_ptr,
                        const PushConstantRanges&               in_push_constant_ranges,
                        std::shared_ptr<Anvil::PipelineLayout>* out_pipeline_layout_ptr);

        /** Retrieves a PipelineLayout instance, assigned to the specific pipeline layout ID */
        std::shared_ptr<Anvil::PipelineLayout> get_layout_by_id(Anvil::PipelineLayoutID in_id) const;

    protected:
        /* Protected functions */

        /** Marks specified pipeline layout ID as used.
         *
         *  NOTE: This function should only be used by PipelineLayout.
         **/
        PipelineLayoutID reserve_pipeline_layout_id();

    private:
        /* Private type declarations */

        /* NOTE: We do NOT own pipeline layouts. As soon as all wrapper instances are out of scope,
         *       we drop the ID.
         */
        typedef std::map<Anvil::PipelineLayoutID, std::weak_ptr<Anvil::PipelineLayout> > PipelineLayouts;

        /* Private functions */
        PipelineLayoutManager(std::weak_ptr<Anvil::BaseDevice> in_device_ptr);

        PipelineLayoutManager           (const PipelineLayoutManager&);
        PipelineLayoutManager& operator=(const PipelineLayoutManager&);

        /** Instantiates a new PipelineLayoutManager instance.
         *
         *  NOTE: This function should only be used by Device.
         *
         *  @param in_device_ptr Device to initialize the manager for.
         *
         *  @return PipelineLayoutManager instance, or nullptr if the function failed.
         **/
        static std::shared_ptr<PipelineLayoutManager> create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr);

        static void on_pipeline_layout_dropped(void* in_callback_arg,
                                               void* in_user_arg);

        /* Private members */
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        PipelineLayouts                  m_pipeline_layouts;
        Anvil::PipelineLayoutID          m_pipeline_layouts_created;

        friend class BaseDevice;
        friend class PipelineLayout;
    };
}; /* Vulkan namespace */

#endif /* PIPELINE_LAYOUT_MANAGER_H */