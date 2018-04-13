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

/** Compute pipeline manager. A class which inherits from the base pipeline object manager.
 *
 *  Apart from exposing the functionality offered by the parent class under slightly
 *  renamed, pipeline-specific function names, this wrapper implements the baking process.
 *
 **/
#ifndef WRAPPERS_COMPUTE_PIPELINE_MANAGER_H
#define WRAPPERS_COMPUTE_PIPELINE_MANAGER_H

#include "misc/base_pipeline_manager.h"
#include "misc/types.h"
#include <memory>

namespace Anvil
{

    class ComputePipelineManager : public BasePipelineManager
    {
    public:
       using BasePipelineManager::bake;

        /* Public functions */
       static std::unique_ptr<ComputePipelineManager> create(Anvil::BaseDevice*    in_device_ptr,
                                                             bool                  in_mt_safe,
                                                             bool                  in_use_pipeline_cache,
                                                             Anvil::PipelineCache* in_pipeline_cache_to_reuse_ptr);

       virtual ~ComputePipelineManager();

       bool bake();

       private:
           /* Constructor */
           explicit ComputePipelineManager(Anvil::BaseDevice*    in_device_ptr,
                                           bool                  in_mt_safe,
                                           bool                  in_use_pipeline_cache          = false,
                                           Anvil::PipelineCache* in_pipeline_cache_to_reuse_ptr = nullptr);

           ANVIL_DISABLE_ASSIGNMENT_OPERATOR(ComputePipelineManager);
           ANVIL_DISABLE_COPY_CONSTRUCTOR   (ComputePipelineManager);
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_COMPUTE_PIPELINE_MANAGER_H */