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

/** Pipeline layout object manager. A ref-counted singleton which:
 *
 *  - caches all pipeline layout wrappers and re-uses already instantiated wrappers,
 *    if user-requested one is already available.
 *
 *  Opt-in MT-safety available.
 **/
#ifndef PIPELINE_LAYOUT_MANAGER_H
#define PIPELINE_LAYOUT_MANAGER_H

#include "misc/mt_safety.h"
#include "misc/types.h"
#include <memory>

namespace Anvil
{
    class PipelineLayoutManager : public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Destructor */
        ~PipelineLayoutManager();

        /** Returns a pipeline layout wrapper matching the specified DSG + push constant range configuration.
         *  If such pipeline layout has never been defined before, it will be created at the call time.
         *
         *  @param in_ds_create_info_items_ptr TODO.
         *  @param in_push_constant_ranges     A vector of PushConstantRange descriptor, describing the push constant ranges
         *                                     the layout should define.
         *  @param out_pipeline_layout_ptr_ptr Deref will be set to a ptr to the pipeline layout wrapper instance, matching the described
         *                                     requirements. Must not be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_layout(const std::vector<DescriptorSetCreateInfoUniquePtr>* in_ds_create_info_items_ptr,
                        const PushConstantRanges&                            in_push_constant_ranges,
                        Anvil::PipelineLayoutUniquePtr*                      out_pipeline_layout_ptr_ptr);

    protected:
        /* Protected functions */

    private:
        /* Private type declarations */
        typedef struct PipelineLayoutContainer
        {
            std::atomic<uint32_t>   n_references;
            PipelineLayoutUniquePtr pipeline_layout_ptr;

            PipelineLayoutContainer()
                :n_references(1)
            {
                /* Stub */
            }
        } PipelineLayoutContainer;

        typedef std::vector<std::unique_ptr<PipelineLayoutContainer> > PipelineLayouts;

        /* Private functions */
        PipelineLayoutManager(const Anvil::BaseDevice* in_device_ptr,
                              bool                     in_mt_safe);

        PipelineLayoutManager           (const PipelineLayoutManager&);
        PipelineLayoutManager& operator=(const PipelineLayoutManager&);

        void on_pipeline_layout_dereferenced(Anvil::PipelineLayout* in_layout_ptr);

        /** Instantiates a new PipelineLayoutManager instance.
         *
         *  NOTE: This function should only be used by Device.
         *
         *  @param in_device_ptr Device to initialize the manager for.
         *  @param in_mt_safe    Set to true if the instance should provide multi-threaded access safety.
         *
         *  @return PipelineLayoutManager instance, or nullptr if the function failed.
         **/
        static Anvil::PipelineLayoutManagerUniquePtr create(const Anvil::BaseDevice* in_device_ptr,
                                                            bool                     in_mt_safe);

        /* Private members */
        const Anvil::BaseDevice* m_device_ptr;
        PipelineLayouts          m_pipeline_layouts;

        friend class BaseDevice;
    };
}; /* Vulkan namespace */

#endif /* PIPELINE_LAYOUT_MANAGER_H */