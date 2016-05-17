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

        /** Returns a pointer to the global PipelineLayoutManager instance.
         *
         *  First call to the function will instantiate the object. The object needs to
         *  be released (by calling release() ) as many time as accquire() was called.
         *
         *  @param device_ptr Device to initialize the manager for.
         *
         *  @return PipelineLayoutManager instance, or nullptr if the function failed.
         **/
        static PipelineLayoutManager* acquire(Anvil::Device* device_ptr);

        /** Retains & returns a pipeline layout wrapper matching the specified DSG + push constant range configuration.
         *  If such pipeline layout has never been defined before, it will be created at the call time.
         *
         *  If the function returns a Anvil::PipelineLayout instance, it is caller's responsibility to release it
         *  in order for the object to be correctly deleted when its reference counter drops to zero.
         *
         *  @param dsgs                        A vector of DescriptorSetGroup instances, describing the set of descriptor sets which the
         *                                     layout should refer to.
         *  @param push_constant_ranges        A vector of PushConstantRange descriptor, describing the push constant ranges
         *                                     the layout should define.
         *  @param out_pipeline_layout_ptr_ptr Deref will be set to a ptr to the pipeline layout wrapper instance, matching the described
         *                                     requirements. The object will be retained, prior to being returned. Must not be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_retained_layout(const DescriptorSetGroups& dsgs,
                                 const PushConstantRanges&  push_constant_ranges,
                                 Anvil::PipelineLayout**    out_pipeline_layout_ptr_ptr);

        /** Decrements the internal reference counter and releases the manager instance, if the counter
         *  drops to zero.
         **/
        void release();

    private:
        /* Private type declarations */
        typedef std::vector<Anvil::PipelineLayout*> PipelineLayouts;

        typedef struct PipelineLayoutManagerInfo
        {
                     PipelineLayoutManager* instance_ptr;
            volatile uint32_t               ref_counter;

            PipelineLayoutManagerInfo()
            {
                instance_ptr = nullptr;
                ref_counter  = 0;
            }

        } PipelineLayoutManagerInfo;

        /* Private functions */
         PipelineLayoutManager(Anvil::Device* device_ptr);
        ~PipelineLayoutManager();

        PipelineLayoutManager           (const PipelineLayoutManager&);
        PipelineLayoutManager& operator=(const PipelineLayoutManager&);

        static void on_pipeline_layout_dropped(void* callback_arg,
                                               void* user_arg);

        /* Private members */
        Anvil::Device* m_device_ptr;
        PipelineLayouts m_pipeline_layouts;

        static std::map<Anvil::Device*, PipelineLayoutManagerInfo> m_instance_map;
    };
}; /* Vulkan namespace */

#endif /* PIPELINE_LAYOUT_MANAGER_H */