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

    typedef PipelineID ComputePipelineID;

    class ComputePipelineManager : public BasePipelineManager
    {
    public:
        /* Public functions */
       static std::shared_ptr<ComputePipelineManager> create(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                                                             bool                                  in_use_pipeline_cache          = false,
                                                             std::shared_ptr<Anvil::PipelineCache> in_pipeline_cache_to_reuse_ptr = nullptr);

       virtual ~ComputePipelineManager();


       /** Registers a new derivative pipeline which is going to inherit state from another compute pipeline object,
         *  which has already been added to the compute pipeline manager.
         *
         *  The function does not automatically bake the new pipeline. This action can either be explicitly
         *  requested by calling bake(), or by calling one of the get_*() functions.
         *
         *  @param in_disable_optimizations                If true, the pipeline will be created with the 
         *                                                 VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT flag.
         *  @param in_allow_derivatives                    If true, the pipeline will be created with the
         *                                                 VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT flag.
         *  @param in_compute_shader_stage_entrypoint_info Compute shader stage entrypoint info.
         *  @param in_base_pipeline_id                     ID of the pipeline, which should be used as base for the new pipeline
         *                                                 object. Must be an ID earlier returned by one of the other add_() functions.
         *  @param out_pipeline_id_ptr                     Deref will be set to the ID of the result pipeline object. Must not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
       bool add_derivative_pipeline_from_sibling_pipeline(bool                               in_disable_optimizations,
                                                          bool                               in_allow_derivatives,
                                                          const ShaderModuleStageEntryPoint& in_compute_shader_stage_entrypoint_info,
                                                          ComputePipelineID                  in_base_pipeline_id,
                                                          ComputePipelineID*                 out_pipeline_id_ptr)
       {
           return BasePipelineManager::add_derivative_pipeline_from_sibling_pipeline(in_disable_optimizations,
                                                                                     in_allow_derivatives,
                                                                                     1, /* n_pipeline_stage_shader_info_items */
                                                                                    &in_compute_shader_stage_entrypoint_info,
                                                                                     in_base_pipeline_id,
                                                                                     out_pipeline_id_ptr);
       }

       /** Registers a new derivative pipeline which is going to inherit its state from another Vulkan pipeline object.
        *
        *  The function does not automatically bake the new pipeline. This action can either be explicitly
        *  requested by calling bake(), or by calling one of the get_*() functions.
        *
        *  @param in_disable_optimizations     If true, the pipeline will be created with the 
        *                                      VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT flag.
        *  @param in_allow_derivatives         If true, the pipeline will be created with the
        *                                      VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT flag.
        *  @param in_compute_shader_stage_info Compute shader stage entrypoint info.
        *  @param in_base_pipeline             Handle of the pipeline, which should be used as base for the new pipeline
        *                                      object. Must not be nullptr.
        *  @param out_pipeline_id_ptr          Deref will be set to the ID of the result pipeline object. Must not be nullptr.
        *
        *  @return true if the function executed successfully, false otherwise.
        **/
       bool add_derivative_pipeline_from_pipeline(bool                               in_disable_optimizations,
                                                  bool                               in_allow_derivatives,
                                                  const ShaderModuleStageEntryPoint& in_compute_shader_stage_entrypoint_info,
                                                  VkPipeline                         in_base_pipeline,
                                                  ComputePipelineID*                 out_compute_pipeline_id_ptr)
       {
           return BasePipelineManager::add_derivative_pipeline_from_pipeline(in_disable_optimizations,
                                                                             in_allow_derivatives,
                                                                             1, /* n_pipeline_stage_shader_info_items */
                                                                            &in_compute_shader_stage_entrypoint_info,
                                                                             in_base_pipeline,
                                                                             out_compute_pipeline_id_ptr);
       }

       /** Registers a new pipeline object.
        *
        *  The function does not automatically bake the new pipeline. This action can either be explicitly
        *  requested by calling bake(), or by calling one of the get_*() functions.
        *
        *  @param in_disable_optimizations                If true, the pipeline will be created with the 
        *                                                 VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT flag.
        *  @param in_allow_derivatives                    If true, the pipeline will be created with the
        *                                                 VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT flag.
        *  @param in_compute_shader_stage_entrypoint_info Compute shader stage info.
        *  @param out_compute_pipeline_id_ptr             Deref will be set to the ID of the result pipeline object. Must not be nullptr.
        *
        *  @return true if the function executed successfully, false otherwise.
        **/
       bool add_regular_pipeline(bool                               in_disable_optimizations,
                                 bool                               in_allow_derivatives,
                                 const ShaderModuleStageEntryPoint& in_compute_shader_stage_entrypoint_info,
                                 ComputePipelineID*                 out_compute_pipeline_id_ptr)
       {
           return BasePipelineManager::add_regular_pipeline(in_disable_optimizations,
                                                            in_allow_derivatives,
                                                            1, /* n_pipeline_stage_shader_info_items */
                                                           &in_compute_shader_stage_entrypoint_info,
                                                            out_compute_pipeline_id_ptr);
       }

       /** Adds a new specialization constant to the specified pipeline object.
         *
         *  This function marks the pipeline as dirty, meaning it will be rebaked at the next get_() call.
         *
         *  @param in_pipeline_id  ID of the pipeline to add the constant to. Must be an ID returned
         *                         by one of the add_() functions.
         *  @param in_constant_id  ID of the specialization constant to assign data for.
         *  @param in_n_data_bytes Number of bytes under @param in_data_ptr to assign to the specialization constant.
         *  @param in_data_ptr     A buffer holding the data to be assigned to the constant. Must hold at least
         *                         @param in_n_data_bytes bytes that will be read by the function.
         *
         *  @return true if successful, false otherwise.
         **/
       bool add_specialization_constant_to_pipeline(ComputePipelineID in_pipeline_id,
                                                    uint32_t          in_constant_id,
                                                    uint32_t          in_n_data_bytes,
                                                    const void*       in_data_ptr)
       {
           return BasePipelineManager::add_specialization_constant_to_pipeline(in_pipeline_id,
                                                                               0, /* shader_index */
                                                                               in_constant_id,
                                                                               in_n_data_bytes,
                                                                               in_data_ptr);
       }

       bool bake();

       /** Deletes an existing pipeline.
        *
        *  @param in_pipeline_id ID of a pipeline to delete.
        *
        *  @return true if successful, false otherwise.
        **/
       bool delete_pipeline(ComputePipelineID in_pipeline_id)
       {
           return BasePipelineManager::delete_pipeline(in_pipeline_id);
       }

       /** Retrieves a VkPipeline instance associated with the specified pipeline ID.
        *
        *  The function will bake a pipeline object (and, possibly, a pipeline layout object, too) if
        *  the specified pipeline is marked as dirty.
        *
        *  @param in_pipeline_id ID of the pipeline to return the raw Vulkan pipeline handle for.
        *
        *  @return VkPipeline handle or nullptr if the function failed.
        **/
       VkPipeline get_compute_pipeline(ComputePipelineID in_pipeline_id)
       {
           return BasePipelineManager::get_pipeline(in_pipeline_id);
       }

       /** Retrieves a PipelineLayout instance associated with the specified pipeline ID.
        *
        *  The function will bake a pipeline object (and, possibly, a pipeline layout object, too) if
        *  the specified pipeline is marked as dirty.
        *
        *  @param in_pipeline_id ID of the pipeline to return the wrapper instance for.
        *                        Must not describe a proxy pipeline.
        *
        *  @return Ptr to a PipelineLayout instance or nullptr if the function failed.
        **/
       std::shared_ptr<Anvil::PipelineLayout> get_compute_pipeline_layout(ComputePipelineID in_pipeline_id)
       {
           return BasePipelineManager::get_pipeline_layout(in_pipeline_id);
       }

       private:
           /* Constructor */
           explicit ComputePipelineManager(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                                           bool                                  in_use_pipeline_cache          = false,
                                           std::shared_ptr<Anvil::PipelineCache> in_pipeline_cache_to_reuse_ptr = nullptr);


           ANVIL_DISABLE_ASSIGNMENT_OPERATOR(ComputePipelineManager);
           ANVIL_DISABLE_COPY_CONSTRUCTOR   (ComputePipelineManager);
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_COMPUTE_PIPELINE_MANAGER_H */