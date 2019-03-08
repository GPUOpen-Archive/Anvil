//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef MISC_DESCRIPTOR_POOL_CREATE_INFO_H
#define MISC_DESCRIPTOR_POOL_CREATE_INFO_H

#include "misc/mt_safety.h"
#include "misc/types.h"
#include <unordered_map>

namespace Anvil
{
    class DescriptorPoolCreateInfo
    {
    public:
        /* Public functions */

        /*  Creates a new create info structure which should be fed to Anvil::DescriptorPool::create() function.
         *
         *  By default, zero descriptors are associated with each descriptor type. You need to specify the number of descriptors
         *  the pool should allocate space for by calling set_n_descriptors_for_descriptor_type().
         *
         *  @param in_device_ptr Device to use.
         *  @param in_n_max_sets Maximum number of sets to be allocable from the pool. Must be at
         *                       least 1.
         *  @param in_flags      See DescriptorPoolFlagBits documentation for more details.
         *  @param in_mt_safety  MT safety setting to use for the pool to be spawned.
         */
        static DescriptorPoolCreateInfoUniquePtr create(const Anvil::BaseDevice*                in_device_ptr,
                                                        const uint32_t&                         in_n_max_sets,
                                                        const Anvil::DescriptorPoolCreateFlags& in_create_flags,
                                                        const MTSafety&                         in_mt_safety = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE);

        const Anvil::DescriptorPoolCreateFlags& get_create_flags() const
        {
            return m_create_flags;
        }

        const Anvil::BaseDevice* get_device_ptr() const
        {
            return m_device_ptr;
        }

        const MTSafety& get_mt_safety() const 
        {
            return m_mt_safety;
        }

        uint32_t get_n_descriptors_for_descriptor_type(const Anvil::DescriptorType& in_descriptor_type) const
        {
            auto     iterator = m_descriptor_count.find(in_descriptor_type);
            uint32_t result   = 0;

            if (iterator != m_descriptor_count.end() )
            {
                result = m_descriptor_count.at(in_descriptor_type);
            }

            return result;
        }

        const uint32_t& get_n_maximum_inline_uniform_block_bindings() const
        {
            return m_n_max_inline_uniform_block_bindings;
        }

        const uint32_t& get_n_maximum_sets() const
        {
            return m_n_max_sets;
        }

        void set_create_flags(const Anvil::DescriptorPoolCreateFlags& in_create_flags)
        {
            m_create_flags = in_create_flags;
        }

        void set_mt_safety(const MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        void set_n_descriptors_for_descriptor_type(const Anvil::DescriptorType& in_descriptor_type,
                                                   const uint32_t&              in_n_descriptors)
        {
            m_descriptor_count[in_descriptor_type] = in_n_descriptors;
        }

        /* Configures the maximum number of inline uniform block bindings that descriptor sets spawned using this descriptor pool
         * will ever use at once.
         *
         * NOTE: Requires VK_EXT_inline_uniform_block support.
         */
        void set_n_maximum_inline_uniform_block_bindings(const uint32_t& in_n_max_inline_uniform_block_bindings)
        {
            m_n_max_inline_uniform_block_bindings = in_n_max_inline_uniform_block_bindings;
        }

        void set_n_maximum_sets(const uint32_t& in_n_maximum_sets)
        {
            m_n_max_sets = in_n_maximum_sets;
        }

    private:

        /* Private functions */

        /** Constructor */
        DescriptorPoolCreateInfo(const Anvil::BaseDevice*                in_device_ptr,
                                 const uint32_t&                         in_n_max_sets,
                                 const Anvil::DescriptorPoolCreateFlags& in_create_flags,
                                 const MTSafety&                         in_mt_safety);

        DescriptorPoolCreateInfo           (const DescriptorPoolCreateInfo&);
        DescriptorPoolCreateInfo& operator=(const DescriptorPoolCreateInfo&);

        /* Private variables */
        Anvil::DescriptorPoolCreateFlags                                                             m_create_flags;
        const Anvil::BaseDevice*                                                                     m_device_ptr;
        std::unordered_map<Anvil::DescriptorType, uint32_t, EnumClassHasher<Anvil::DescriptorType> > m_descriptor_count;
        Anvil::MTSafety                                                                              m_mt_safety;
        uint32_t                                                                                     m_n_max_inline_uniform_block_bindings;
        uint32_t                                                                                     m_n_max_sets;
    };

}; /* namespace Anvil */

#endif /* MISC_DESCRIPTOR_POOL_CREATE_INFO_H */