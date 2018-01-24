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

#ifndef MISC_DESCRIPTOR_SET_INFO_H
#define MISC_DESCRIPTOR_SET_INFO_H

#include "misc/types.h"

namespace Anvil
{
    class DescriptorSetInfo
    {
    public:
        /* Public functions */

        /** Destructor. */
        ~DescriptorSetInfo();

        /** Adds a new binding.
         *
         *  It is an error to attempt to add a binding at an index, for which another binding has
         *  already been specified.
         *
         *  It is an error to attempt to define immutable samplers for descriptors of type other than
         *  sampler or combined image+sampler.
         *
         *  @param in_binding_index              Index of the binding to configure.
         *  @param in_descriptor_type            Type of the descriptor to use for the binding.
         *  @param in_descriptor_array_size      Size of the descriptor array to use for the binding.
         *  @param in_stage_flags                Rendering stages which are going to use the binding.
         *  @param in_opt_immutable_sampler_ptrs If not nullptr, an array of @param in_descriptor_array_size samplers should
         *                                       be passed. The binding will then be considered immutable, as per spec language.
         *                                       May be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool add_binding(uint32_t                               in_binding_index,
                         VkDescriptorType                       in_descriptor_type,
                         uint32_t                               in_descriptor_array_size,
                         VkShaderStageFlags                     in_stage_flags,
                         const std::shared_ptr<Anvil::Sampler>* in_opt_immutable_sampler_ptrs = nullptr);

        /** Creates a new DescriptorSetInfo instance. **/
        static std::unique_ptr<DescriptorSetInfo> create();

        /** Retrieves properties of a single defined binding.
         *
         *  @param in_n_binding                           Index number of the binding to retrieve properties of.
         *  @param out_opt_binding_index_ptr              May be nullptr. If not, deref will be set to the index of the
         *                                                binding. This does NOT need to be equal to @param in_n_binding.
         *  @param out_opt_descriptor_type_ptr            May be nullptr. If not, deref will be set to the descriptor type
         *                                                for the specified binding.
         *  @param out_opt_descriptor_array_size_ptr      May be nullptr. If not, deref will be set to size of the descriptor
         *                                                array, associated with the specified binding.
         *  @param out_opt_stage_flags_ptr                May be nullptr. If not, deref will be set to stage flags,
         *                                                as configured for the specified binding.
         *  @param out_opt_immutable_samplers_enabled_ptr May be nullptr. If not, deref will be set to true if immutable samplers
         *                                                have been defined for the specified binding; otherwise, it will be
         *                                                set to false.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_binding_properties(uint32_t            in_n_binding,
                                    uint32_t*           out_opt_binding_index_ptr,
                                    VkDescriptorType*   out_opt_descriptor_type_ptr,
                                    uint32_t*           out_opt_descriptor_array_size_ptr,
                                    VkShaderStageFlags* out_opt_stage_flags_ptr,
                                    bool*               out_opt_immutable_samplers_enabled_ptr) const;

        /** Returns the number of bindings defined for the layout. */
        uint32_t get_n_bindings() const
        {
            return static_cast<uint32_t>(m_bindings.size() );
        }

    private:
        /* Private type definitions */

        /** Describes a single descriptor set layout binding */
        typedef struct Binding
        {
            uint32_t                                      descriptor_array_size;
            VkDescriptorType                              descriptor_type;
            std::vector<std::shared_ptr<Anvil::Sampler> > immutable_samplers;

            VkShaderStageFlagsVariable(stage_flags);

            /** Dummy constructor. Do not use. */
            Binding()
            {
                descriptor_array_size = 0;
                descriptor_type       = VK_DESCRIPTOR_TYPE_MAX_ENUM;
                stage_flags           = static_cast<VkShaderStageFlagBits>(0);
            }

            /** Constructor.
             *
             *  For argument discussion, please see Anvil::DescriptorSetLayout::add_binding() documentation.
             **/
            Binding(uint32_t                               in_descriptor_array_size,
                    VkDescriptorType                       in_descriptor_type,
                    VkShaderStageFlags                     in_stage_flags,
                    const std::shared_ptr<Anvil::Sampler>* in_immutable_sampler_ptrs)
            {
                descriptor_array_size = in_descriptor_array_size;
                descriptor_type       = in_descriptor_type;
                stage_flags           = static_cast<VkShaderStageFlagBits>(in_stage_flags);

                if (in_immutable_sampler_ptrs != nullptr)
                {
                    for (uint32_t n_sampler = 0;
                                  n_sampler < descriptor_array_size;
                                ++n_sampler)
                    {
                        immutable_samplers.push_back(in_immutable_sampler_ptrs[n_sampler]);
                    }
                }
            }
        } Binding;

        typedef std::map<BindingIndex, Binding> BindingIndexToBindingMap;

        /* Private functions */

        /* Please see create() documentation for more details */
        DescriptorSetInfo();

        /* Private variables */
        BindingIndexToBindingMap m_bindings;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(DescriptorSetInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(DescriptorSetInfo);

        friend class Anvil::DescriptorSetLayout;
    };
}; /* namespace Anvil */

#endif /* MISC_DESCRIPTOR_SET_INFO */