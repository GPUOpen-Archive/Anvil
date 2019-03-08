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

#ifndef MISC_DESCRIPTOR_SET_CREATE_INFO_H
#define MISC_DESCRIPTOR_SET_CREATE_INFO_H

#include "misc/types.h"
#include "misc/struct_chainer.h"

namespace Anvil
{
    struct DescriptorSetLayoutCreateInfoContainer
    {
        std::vector<VkDescriptorBindingFlagsEXT>                     binding_flags_vec;
        std::vector<VkDescriptorSetLayoutBinding>                    binding_info_items;
        std::vector<VkSampler>                                       sampler_items;
        Anvil::StructChainUniquePtr<VkDescriptorSetLayoutCreateInfo> struct_chain_ptr;
    };

    class DescriptorSetCreateInfo
    {
    public:
        /* Public functions */

        /** Destructor. */
        ~DescriptorSetCreateInfo();

        /** Adds a new binding.
         *
         *  If @param in_flags includes DESCRIPTOR_BINDING_FLAG_VARIABLE_DESCRIPTOR_COUNT_BIT, @param in_descriptor_array_size
         *  tells the maximum number of descriptors the binding can take. The actual number of descriptors which is going to be
         *  specified for the binding needs to be specified by separately calling set_binding_variable_descriptor_count().
         *
         *  It is an error to attempt to add a binding at an index, for which another binding has
         *  already been specified.
         *
         *  It is an error to attempt to define immutable samplers for descriptors of type other than
         *  sampler or combined image+sampler.
         *
         *  NOTE: For inline uniform block bindings, subsequent set_binding_item() call is NOT required.
         *
         *  @param in_binding_index                 Index of the binding to configure.
         *  @param in_descriptor_type               Type of the descriptor to use for the binding.
         *  @param in_descriptor_array_size         Size of the descriptor array to use for the binding.
         *
         *                                          For inline uniform blocks, this parameter corresponds to (number of bytes associated with the
         *                                          block). This value MUST be divisible by 4.
         *
         *  @param in_flags                         Please see documentation of Anvil::DescriptorBindingFlags for more details.
         *  @param in_opt_immutable_sampler_ptr_ptr If not nullptr, an array of @param in_descriptor_array_size samplers should
         *                                          be passed. The binding will then be considered immutable, as per spec language.
         *                                          May be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool add_binding(uint32_t                               in_binding_index,
                         Anvil::DescriptorType                  in_descriptor_type,
                         uint32_t                               in_descriptor_array_size,
                         Anvil::ShaderStageFlags                in_stage_flags,
                         const Anvil::DescriptorBindingFlags&   in_flags                         = Anvil::DescriptorBindingFlagBits::NONE,
                         const Anvil::Sampler* const*           in_opt_immutable_sampler_ptr_ptr = nullptr);

        /** Tells if the DS info structure contains a variable descriptor count binding.
         *
         *  @param out_opt_binding_index_ptr If not null, deref will be set to the index of the variable descriptor
         *                                   count binding.
         *  @param out_opt_binding_size_ptr  If not null, deref will be set to the variable descriptor count binding's size.
         *
         *  @return true if such a binding has been defined, false otherwise.
         */
        bool contains_variable_descriptor_count_binding(uint32_t* out_opt_binding_index_ptr = nullptr,
                                                        uint32_t* out_opt_binding_size_ptr  = nullptr) const
        {
            if (out_opt_binding_index_ptr != nullptr)
            {
                *out_opt_binding_index_ptr = m_n_variable_descriptor_count_binding;
            }

            if (out_opt_binding_size_ptr != nullptr)
            {
                *out_opt_binding_size_ptr = m_variable_descriptor_count_binding_size;
            }

            return (m_n_variable_descriptor_count_binding != UINT32_MAX);
        }

        static DescriptorSetCreateInfoUniquePtr create();

        /* Fills & returns a VkDescriptorSetLayoutCreateInfo structure holding all information necessary to spawn
         * a new descriptor set layout instance.
         *
         **/
        std::unique_ptr<DescriptorSetLayoutCreateInfoContainer> create_descriptor_set_layout_create_info(const Anvil::BaseDevice* in_device_ptr) const;

        /** Retrieves properties of a binding at a given index number.
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
         *  @param out_opt_flags_ptr                      May be nullptr. If not, deref will be set to the flags specified at binding
         *                                                addition time.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_binding_properties_by_binding_index(uint32_t                       in_binding_index,
                                                     Anvil::DescriptorType*         out_opt_descriptor_type_ptr            = nullptr,
                                                     uint32_t*                      out_opt_descriptor_array_size_ptr      = nullptr,
                                                     Anvil::ShaderStageFlags*       out_opt_stage_flags_ptr                = nullptr,
                                                     bool*                          out_opt_immutable_samplers_enabled_ptr = nullptr,
                                                     Anvil::DescriptorBindingFlags* out_opt_flags_ptr                      = nullptr) const;

        /** Retrieves properties of a binding at a given index number.
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
         *  @param out_opt_flags_ptr                      May be nullptr. If not, deref will be set to the flags specified at binding
         *                                                addition time.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_binding_properties_by_index_number(uint32_t                       in_n_binding,
                                                    uint32_t*                      out_opt_binding_index_ptr              = nullptr,
                                                    Anvil::DescriptorType*         out_opt_descriptor_type_ptr            = nullptr,
                                                    uint32_t*                      out_opt_descriptor_array_size_ptr      = nullptr,
                                                    Anvil::ShaderStageFlags*       out_opt_stage_flags_ptr                = nullptr,
                                                    bool*                          out_opt_immutable_samplers_enabled_ptr = nullptr,
                                                    Anvil::DescriptorBindingFlags* out_opt_flags_ptr                      = nullptr) const;

        /** Returns the number of bindings defined for the layout. */
        uint32_t get_n_bindings() const
        {
            return static_cast<uint32_t>(m_bindings.size() );
        }

        /* Sets the number of descriptors to be used for a variable descriptor count binding.
         *
         * A variable descriptor count binding must have been added to this DS info instance before this function
         * can be called.
         */
        bool set_binding_variable_descriptor_count(const uint32_t& in_count);

        bool operator==(const Anvil::DescriptorSetCreateInfo& in_ds) const;

    private:
        /* Private type definitions */

        /** Describes a single descriptor set layout binding */
        typedef struct Binding
        {
            uint32_t                           descriptor_array_size;
            Anvil::DescriptorType              descriptor_type;
            Anvil::DescriptorBindingFlags      flags;
            std::vector<const Anvil::Sampler*> immutable_samplers;

            Anvil::ShaderStageFlags stage_flags;

            /** Dummy constructor. Do not use. */
            Binding()
            {
                descriptor_array_size = 0;
                descriptor_type       = Anvil::DescriptorType::UNKNOWN;
            }

            /** Constructor.
             *
             *  For argument discussion, please see Anvil::DescriptorSetLayout::add_binding() documentation.
             **/
            Binding(uint32_t                      in_descriptor_array_size,
                    Anvil::DescriptorType         in_descriptor_type,
                    Anvil::ShaderStageFlags       in_stage_flags,
                    const Anvil::Sampler* const*  in_immutable_sampler_ptrs,
                    Anvil::DescriptorBindingFlags in_flags)
            {
                descriptor_array_size = in_descriptor_array_size;
                descriptor_type       = in_descriptor_type;
                flags                 = in_flags;
                stage_flags           = in_stage_flags;

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

            bool operator==(const Binding& in_binding) const
            {
                return (descriptor_array_size == in_binding.descriptor_array_size) &&
                       (descriptor_type       == in_binding.descriptor_type)       &&
                       (flags                 == in_binding.flags)                 &&
                       (immutable_samplers    == in_binding.immutable_samplers)    &&
                       (stage_flags           == in_binding.stage_flags);
            }
        } Binding;

        typedef std::map<BindingIndex, Binding> BindingIndexToBindingMap;

        /* Private functions */

        /* Please see create() documentation for more details */
        DescriptorSetCreateInfo();

        /* Private variables */
        BindingIndexToBindingMap m_bindings;

        uint32_t                 m_n_variable_descriptor_count_binding;
        uint32_t                 m_variable_descriptor_count_binding_size;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(DescriptorSetCreateInfo);
    };
}; /* namespace Anvil */

#endif /* MISC_DESCRIPTOR_SET_CREATE_INFO */