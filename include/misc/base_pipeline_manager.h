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

/** Base pipeline object manager. An abstract class which:
 *
 *  - simplifies the process of creating pipeline & derivative pipeline objects.
 *  - relies on PipelineLayoutManager to automatically re-use pipeline layout objects
 *    if the same layout is used for more than one pipeline object.
 *  - tracks life-time of baked Vulkan pipeline objects.
 *  - optionally defers the process of baking these objects until they're needed.
 *
 *  Any number of push constant ranges, as well as specialization constants can be assigned
 *  to the created pipeline objects.
 *
 *  This class is inherited by compute & graphics pipeline managers to provide common
 *  implementation for shared areas of functionality.
 **/
#ifndef MISC_BASE_PIPELINE_MANAGER_H
#define MISC_BASE_PIPELINE_MANAGER_H

#include "misc/base_pipeline_create_info.h"
#include "misc/callbacks.h"
#include "misc/debug.h"
#include "misc/mt_safety.h"
#include "misc/types.h"
#include <memory>
#include <vector>

namespace Anvil
{
    enum BasePipelineManagerCallbackID
    {
        /* Call-back issued whenever a new pipeline is created.
         *
         * NOTE: Only base pipeline-level properties are available for querying at the time of the call-back!
         *
         * callback_arg: OnNewPipelineCreatedCallbackData instance.
         */
        BASE_PIPELINE_MANAGER_CALLBACK_ID_ON_NEW_PIPELINE_CREATED,

        /* Always last */
        BASE_PIPELINE_MANAGER_CALLBACK_ID_COUNT
    };

    class BasePipelineManager : public CallbacksSupportProvider,
                                public MTSafetySupportProvider
    {
    public:
       /* Public functions */

       /** Destructor. Releases internally managed objects. */
       virtual ~BasePipelineManager();

        /** TODO */
        bool add_pipeline(Anvil::BasePipelineCreateInfoUniquePtr in_pipeline_create_info_ptr,
                          PipelineID*                            out_pipeline_id_ptr);

       /** TODO */
       virtual bool bake() = 0;

       /** Deletes an existing pipeline.
        *
        *  @param in_pipeline_id ID of a pipeline to delete.
        *
        *  @return true if successful, false otherwise.
        **/
       bool delete_pipeline(PipelineID in_pipeline_id);

       /** Retrieves a VkPipeline instance associated with the specified pipeline ID.
        *
        *  The function will bake a pipeline object (and, possibly, a pipeline layout object, too) if
        *  the specified pipeline is marked as dirty.
        *
        *  @param in_pipeline_id ID of the pipeline to return the raw Vulkan pipeline handle for. Must not
        *                        describe a proxy pipeline.
        *
        *  @return VkPipeline handle or nullptr if the function failed.
        **/
       VkPipeline get_pipeline(PipelineID in_pipeline_id);

       const Anvil::BasePipelineCreateInfo* get_pipeline_create_info(PipelineID in_pipeline_id) const;

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
       Anvil::PipelineLayout* get_pipeline_layout(PipelineID in_pipeline_id);

       /** Returns various post-compile information about compute and graphics pipeline shaders like
        *  compiled binary or optionally shader disassembly.
        *  Requires support for VK_AMD_shader_info extension.
        *
        *  @param in_pipeline_id    ID of the pipeline.
        *  @param in_shader_stage   The shader stage to collect post-compile information for.
        *  @param in_info_type      the type of information to collect - compiled binary code or shader disassembly.
        *  @param out_data_ptr      pointer to a vector of bytes where the post-compile information
        *                           is going to be stored. Pointer must be null. If the vector is empty
        *                           it is going to be resized to match the size required to store
        *                           the post-compile information.
        *
        *  @return true if successful and *out_data_ptr has been updated to store the data returned by the
        *          device, false otherwise.
        **/
       bool get_shader_info(PipelineID                  in_pipeline_id,
                            Anvil::ShaderStage          in_shader_stage,
                            Anvil::ShaderInfoType       in_info_type,
                            std::vector<unsigned char>* out_data_ptr);

       /** Returns post-compile GPU statistics about compute and graphics pipeline shaders like GPU register usage.
        *  Requires support for VK_AMD_shader_info extension.
        *
        *  @param in_pipeline_id            ID of the pipeline.
        *  @param in_shader_stage           The shader stage to collect post-compile information for.
        *  @param out_shader_statistics_ptr Pointer to VkShaderStatisticsInfoAMD struct to be filled in
        *                                   with statistics about requested shader stage.
        *                                   Pointer cannot be null.
        *
        *  @return true if successful, false otherwise.
        **/
       bool get_shader_statistics(PipelineID                 in_pipeline_id,
                                  Anvil::ShaderStage         in_shader_stage,
                                  VkShaderStatisticsInfoAMD* out_shader_statistics_ptr);

    protected:
       /* Protected type declarations */

       /** Internal pipeline object descriptor */
       typedef struct Pipeline : public MTSafetySupportProvider
       {
           VkPipeline                             baked_pipeline;
           const BaseDevice*                      device_ptr;
           Anvil::PipelineLayoutUniquePtr         layout_ptr;
           Anvil::BasePipelineCreateInfoUniquePtr pipeline_create_info_ptr;

           Pipeline(const Anvil::BaseDevice*               in_device_ptr,
                    Anvil::BasePipelineCreateInfoUniquePtr in_pipeline_create_info_ptr,
                    bool                                   in_mt_safe)
               :MTSafetySupportProvider(in_mt_safe)
           {
               baked_pipeline           = VK_NULL_HANDLE;
               device_ptr               = in_device_ptr;
               pipeline_create_info_ptr = std::move(in_pipeline_create_info_ptr);
           }

           /** Destrutor. Releases the internally managed Vulkan objects. */
           ~Pipeline()
           {
               release_pipeline();
           }

       private:
           Pipeline();

           /** Releases pipeline & pipeline layout instances **/
           void release_pipeline();
       } Pipeline;

       typedef std::map<PipelineID, std::unique_ptr<Pipeline> > Pipelines;

       /* Protected functions */

       /** Constructor. Initializes base layer of a pipeline manager.
        *
        *  @param in_device_ptr                  Device to use.
        *  @param in_mt_safe                     True if more than one thread at a time is going to be issuing calls against the pipeline manager.
        *  @param in_use_pipeline_cache          true if a pipeline cache should be used to spawn new pipeline objects.
        *                                        What pipeline cache ends up being used depends on @param in_pipeline_cache_to_reuse_ptr -
        *                                        if a nullptr object is passed via this argument, a new pipeline cache instance will
        *                                        be created, and later released by the destructor. If a non-nullptr object is passed,
        *                                        it will be used instead.
        *  @param in_pipeline_cache_to_reuse_ptr Please see above.
        **/
       explicit BasePipelineManager(const Anvil::BaseDevice* in_device_ptr,
                                    bool                     in_mt_safe,
                                    bool                     in_use_pipeline_cache,
                                    Anvil::PipelineCache*    in_pipeline_cache_to_reuse_ptr);

       /** Fills & returns a VkSpecializationInfo descriptor. Any sub-descriptors, to which the baked descriptor
        *  is going to point at, are stored in a vector provided by the caller. It is caller's responsibility to
        *  ensure the vector is not released before pipeline baking occurs.
        *
        *  @param in_specialization_constants            Vector of internal specialization constant descriptors, which should be
        *                                                baked into the Vulkan descriptor.
        *  @param in_specialization_constant_data_ptr    Buffer which holds specialization constant data.
        *  @param out_specialization_map_entry_vk_vector As per description. Must not be nullptr.
        *  @param out_specialization_info_ptr            Deref will be set to the baked Vulkan descriptor. Must not be nullptr.
        **/
       void bake_specialization_info_vk(const SpecializationConstants&         in_specialization_constants,
                                        const unsigned char*                   in_specialization_constant_data_ptr,
                                        std::vector<VkSpecializationMapEntry>* out_specialization_map_entry_vk_vector,
                                        VkSpecializationInfo*                  out_specialization_info_ptr) const;

       /* Protected members */
       const Anvil::BaseDevice* m_device_ptr;
       std::atomic<uint32_t>    m_pipeline_counter;

       Pipelines                             m_baked_pipelines;
       Pipelines                             m_outstanding_pipelines;

       Anvil::PipelineCache*  m_pipeline_cache_ptr;
       PipelineCacheUniquePtr m_pipeline_cache_owned_ptr;
       PipelineLayoutManager* m_pipeline_layout_manager_ptr;
       bool                   m_use_pipeline_cache;

private:
       /* Private functions */
       BasePipelineManager& operator=(const BasePipelineManager&);
       BasePipelineManager           (const BasePipelineManager&);
    };
}; /* Vulkan namespace */

#endif /* MISC_BASE_PIPELINE_MANAGER_H */