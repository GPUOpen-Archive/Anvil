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
#ifndef ANVIL_BASE_PIPELINE_CREATE_INFO_H
#define ANVIL_BASE_PIPELINE_CREATE_INFO_H

#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    class BasePipelineCreateInfo
    {
    public:
        /* Public functions */

        virtual ~BasePipelineCreateInfo();

        bool allows_derivatives() const
        {
            return m_allow_derivatives;
        }

        bool attach_push_constant_range(uint32_t           in_offset,
                                        uint32_t           in_size,
                                        VkShaderStageFlags in_stages);

        /* Returns != UINT32_MAX if this pipeline is a derivative pipeline, or UINT32_MAX otherwise. */
        Anvil::PipelineID get_base_pipeline_id() const
        {
            return m_base_pipeline_id;
        }

        const std::vector<DescriptorSetCreateInfoUniquePtr>* get_ds_create_info_items() const
        {
            return &m_ds_create_info_items;
        }

        const PushConstantRanges& get_push_constant_ranges() const
        {
            return m_push_constant_ranges;
        }

        /** Returns shader stage information for each stage, as specified at creation time */
        bool get_shader_stage_properties(Anvil::ShaderStage                  in_shader_stage,
                                         const ShaderModuleStageEntryPoint** out_opt_result_ptr_ptr) const;

        bool get_specialization_constants(Anvil::ShaderStage              in_shader_stage,
                                          const SpecializationConstants** out_opt_spec_constants_ptr,
                                          const unsigned char**           out_opt_spec_constants_data_buffer_ptr) const;

        bool has_optimizations_disabled() const
        {
            return m_disable_optimizations;
        }

        bool is_proxy() const
        {
            return m_is_proxy;
        }

        void set_descriptor_set_create_info(const std::vector<const Anvil::DescriptorSetCreateInfo*>* in_ds_create_info_vec_ptr);

    protected:
        /* Protected functions */

        BasePipelineCreateInfo();

        /** Adds a new specialization constant.
         *
         *  @param in_shader_stage Shader stage, with which the new specialization constant should be
         *                         associated.
         *  @param in_constant_id  ID of the specialization constant to assign data for.
         *  @param in_n_data_bytes Number of bytes under @param in_data_ptr to assign to the specialization constant.
         *  @param in_data_ptr     A buffer holding the data to be assigned to the constant. Must hold at least
         *                         @param in_n_data_bytes bytes that will be read by the function.
         *
         *  @return true if successful, false otherwise.
         **/
        virtual bool add_specialization_constant(Anvil::ShaderStage in_shader_stage,
                                                 uint32_t           in_constant_id,
                                                 uint32_t           in_n_data_bytes,
                                                 const void*        in_data_ptr);

        void init_derivative(bool                                                      in_disable_optimizations,
                             bool                                                      in_allow_derivatives,
                             uint32_t                                                  in_n_shader_module_stage_entrypoints,
                             const ShaderModuleStageEntryPoint*                        in_shader_module_stage_entrypoint_ptrs,
                             Anvil::PipelineID                                         in_base_pipeline_id,
                             const std::vector<const Anvil::DescriptorSetCreateInfo*>* in_opt_ds_create_info_vec_ptr = nullptr);
        void init_proxy     ();
        void init_regular   (bool                                                      in_disable_optimizations,
                             bool                                                      in_allow_derivatives,
                             uint32_t                                                  in_n_shader_module_stage_entrypoints,
                             const ShaderModuleStageEntryPoint*                        in_shader_module_stage_entrypoint_ptrs,
                             const std::vector<const Anvil::DescriptorSetCreateInfo*>* in_opt_ds_create_info_vec_ptr = nullptr);

        void copy_state_from(const BasePipelineCreateInfo* in_src_pipeline_create_info_ptr);


        /* Protected type definitions */
        typedef std::map<Anvil::ShaderStage, SpecializationConstants> ShaderStageToSpecializationConstantsMap;

        /* Protected variables */
        Anvil::PipelineID m_base_pipeline_id;

        std::vector<DescriptorSetCreateInfoUniquePtr> m_ds_create_info_items;
        PushConstantRanges                            m_push_constant_ranges;
        std::vector<unsigned char>                    m_specialization_constants_data_buffer;
        ShaderStageToSpecializationConstantsMap       m_specialization_constants_map;

        bool                                               m_allow_derivatives;
        bool                                               m_disable_optimizations;
        bool                                               m_is_proxy;
        std::map<ShaderStage, ShaderModuleStageEntryPoint> m_shader_stages;

    private:
        /* Private type definitions */

        /* Private functions */
        void init_shader_modules(uint32_t                                  in_n_shader_module_stage_entrypoints,
                                 const Anvil::ShaderModuleStageEntryPoint* in_shader_module_stage_entrypoint_ptrs);

        /* Private variables */
    };

};

#endif /* ANVIL_BASE_PIPELINE_CREATE_INFO_H */

