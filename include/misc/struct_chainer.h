//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef MISC_STRUCT_CHAINER_H
#define MISC_STRUCT_CHAINER_H

#include "types.h"

namespace Anvil
{
    typedef uint32_t StructID;

    template <typename StructType>
    struct StructChain
    {
        std::vector<uint8_t> raw_data;
        StructType*          root_struct_ptr;

        StructChain(const uint32_t& in_raw_data_size)
            :raw_data(in_raw_data_size)
        {
            root_struct_ptr = nullptr;
        }

        const StructType* get_struct_with_id(const StructID& in_id) const
        {
            anvil_assert(in_id < raw_data.size() );

            return reinterpret_cast<const StructType*>(&raw_data.at(0) + in_id);
        }

        template<typename StructType2>
        StructType2* get_struct_with_id(const StructID& in_id)
        {
            anvil_assert(in_id < raw_data.size() );

            return reinterpret_cast<StructType2*>(&raw_data.at(0) + in_id);
        }

        StructType* get_root_struct()
        {
            return reinterpret_cast<StructType*>(&raw_data.at(0) );
        }

        const StructType* get_root_struct() const
        {
            return reinterpret_cast<const StructType*>(&raw_data.at(0) );
        }
    };

    template <typename StructType>
    using StructChainUniquePtr = std::unique_ptr<StructChain<StructType> >;

    template <typename StructType>
    struct StructChainVector
    {
        void append_struct_chain(StructChainUniquePtr<StructType> inout_struct_chain_ptr)
        {
            root_structs.push_back     (*inout_struct_chain_ptr->get_root_struct() );
            struct_chain_ptrs.push_back(std::move(inout_struct_chain_ptr) );
        }

        uint32_t get_n_structs() const
        {
            return static_cast<uint32_t>(root_structs.size() );
        }
        const StructType* get_root_structs() const
        {
            return &root_structs.at(0);
        }


    private:
        std::vector<StructType>                        root_structs;
        std::vector<StructChainUniquePtr<StructType> > struct_chain_ptrs;
    };

    template<typename StructType>
    class StructChainer
    {
    public:
        /* Public functions */
         StructChainer()
             :m_helper_structs_size(0),
              m_structs_size       (0)
         {
             /* Stub */
         }

        ~StructChainer()
        {
            /* Stub */
        }

        template<typename ChainedStructType>
        StructID append_struct(const ChainedStructType& in_struct)
        {
            const uint32_t       pre_call_structs_size(m_structs_size);
            std::vector<uint8_t> struct_raw_data      (sizeof(in_struct) );

            anvil_assert(in_struct.pNext == nullptr);

            /* Zeroth item appended to the chain must be of StructType type! */
            if (m_structs_size    == 0                  &&
                sizeof(in_struct) != sizeof(StructType) )
            {
                anvil_assert_fail();
            }

            memcpy(struct_raw_data.data(),
                   &in_struct,
                   sizeof(in_struct) );

            m_structs.push_back(
                std::move(struct_raw_data)
            );

            m_structs_size += sizeof(in_struct);

            return pre_call_structs_size;
        }

        template<typename HelperStructType>
        void store_helper_structure(const HelperStructType& in_helper_struct,
                                    const StructID&         in_referring_struct,
                                    const uint32_t&         in_referring_struct_pnext_ptr_offset)
        {
            anvil_assert(m_structs.size() >  in_referring_struct);
            anvil_assert(m_structs.size() >= in_referring_struct + in_referring_struct_pnext_ptr_offset + sizeof(void*) );

            m_helper_structs.push_back(
                HelperStruct(&in_helper_struct,
                             sizeof(in_helper_struct),
                             in_referring_struct,
                            in_referring_struct_pnext_ptr_offset)
            );

            m_helper_structs_size += static_cast<uint32_t>(sizeof(in_helper_struct) );
        }

        template<typename HelperStructType>
        void store_helper_structure_vector(const std::vector<HelperStructType>& in_helper_struct_vec,
                                           const StructID&                      in_referring_struct,
                                           const uint32_t&                      in_referring_struct_pnext_ptr_offset)
        {
            /* Convert the input vector to a single entry */
            anvil_assert(in_helper_struct_vec.size() > 0);

            m_helper_structs.push_back(
                HelperStruct(&in_helper_struct_vec.at(0),
                              static_cast<uint32_t>(in_helper_struct_vec.size() ) * sizeof(HelperStructType),
                              in_referring_struct,
                              in_referring_struct_pnext_ptr_offset)
            );

            m_helper_structs_size += static_cast<uint32_t>(in_helper_struct_vec.size() ) * sizeof(HelperStructType);
        }

        std::unique_ptr<StructChain<StructType> > create_chain() const
        {
            size_t                                    n_bytes_used     = 0;
            const uint32_t                            n_helper_structs = static_cast<uint32_t>(m_helper_structs.size() );
            const uint32_t                            n_structs        = static_cast<uint32_t>(m_structs.size       () );
            std::unique_ptr<StructChain<StructType> > result_ptr;

            /* Sanity checks */
            if (m_structs.size() == 0)
            {
                anvil_assert(m_structs.size() > 0);

                goto end;
            }

            /* Allocate the result instance */
            result_ptr.reset(
                new StructChain<StructType>(m_structs_size + m_helper_structs_size)
            );

            if (result_ptr == nullptr)
            {
                anvil_assert(result_ptr != nullptr);

                goto end;
            }

            /* Form the struct chain.. */
            for (uint32_t n_struct = 0;
                          n_struct < n_structs;
                        ++n_struct)
            {
                /* Copy struct contents to the final vector */
                const auto& current_struct_data      = m_structs.at(n_struct);
                const auto  current_struct_data_size = current_struct_data.size();

                memcpy(&result_ptr->raw_data.at(n_bytes_used),
                       &current_struct_data.at(0),
                        current_struct_data_size);

                /* Adjust pNext pointer to point at the next struct, if defined. */
                if (n_struct != (n_structs - 1) )
                {
                    VkStructHeader* header_ptr = reinterpret_cast<VkStructHeader*>(&result_ptr->raw_data.at(n_bytes_used) );

                    header_ptr->next_ptr = &result_ptr->raw_data.at(0) + n_bytes_used + current_struct_data_size;
                }

                n_bytes_used += current_struct_data_size;
            }

            for (uint32_t n_helper_struct = 0;
                          n_helper_struct < n_helper_structs;
                        ++n_helper_struct)
            {
                const auto& current_helper_struct_props     = m_helper_structs.at(n_helper_struct);
                const auto& current_helper_struct_data      = current_helper_struct_props.data;
                const auto  current_helper_struct_data_size = current_helper_struct_data.size();

                /* Cache helper structure data */
                memcpy(&result_ptr->raw_data.at(n_bytes_used),
                       &current_helper_struct_data.at(0),
                        current_helper_struct_data_size);

                /* Patch the field of the referring struct so that it points to the helper structure we cache locally */
                *reinterpret_cast<void**>(&result_ptr->raw_data.at(current_helper_struct_props.referring_struct_id + current_helper_struct_props.referring_struct_ptr_offset) ) = &result_ptr->raw_data.at(n_bytes_used);

                n_bytes_used += current_helper_struct_data_size;
            }

            result_ptr->root_struct_ptr = reinterpret_cast<StructType*>(&result_ptr->raw_data.at(0) );

            /* Done. */
        end:
            return result_ptr;
        }

        StructType* get_last_struct()
        {
            anvil_assert(m_structs.size() > 0);

            return reinterpret_cast<StructType*>(&m_structs.at(m_structs.size() - 1).at(0) );
        }

        const StructType* get_last_struct() const
        {
            anvil_assert(m_structs.size() > 0);

            return reinterpret_cast<const StructType*>(&m_structs.at(m_structs.size() - 1).at(0) );
        }

        uint32_t get_n_structs() const
        {
            return static_cast<uint32_t>(m_structs.size() );
        }

        StructType* get_root_struct()
        {
            anvil_assert(m_structs.size() > 0);

            return reinterpret_cast<StructType*>(&m_structs.at(0).at(0) );
        }

        const StructType* get_root_struct() const
        {
            anvil_assert(m_structs.size() > 0);

            return reinterpret_cast<const StructType*>(&m_structs.at(0).at(0) );
        }

        VkStructHeader* get_struct_at_index(const uint32_t& in_index)
        {
            VkStructHeader* result_ptr = nullptr;

            if (m_structs.size() > in_index)
            {
                result_ptr = reinterpret_cast<VkStructHeader*>(&m_structs.at(in_index).at(0) );
            }

            return result_ptr;
        }
    private:
        /* Private type definitions */
        typedef struct HelperStruct
        {
            std::vector<uint8_t> data;
            StructID             referring_struct_id;
            uint32_t             referring_struct_ptr_offset;

            HelperStruct(const void*     in_data_ptr,
                         const uint32_t& in_data_size,
                         const StructID& in_referring_struct_id,
                         const uint32_t& in_referring_struct_ptr_offset)
                :referring_struct_id        (in_referring_struct_id),
                 referring_struct_ptr_offset(in_referring_struct_ptr_offset)
            {
                anvil_assert(in_data_ptr  != nullptr);
                anvil_assert(in_data_size != 0);

                data.resize(in_data_size);

                memcpy(&data.at(0),
                       in_data_ptr,
                       in_data_size);
            }
        } HelperStruct;

        /* Private functions */
        std::vector<HelperStruct>          m_helper_structs;
        uint32_t                           m_helper_structs_size;
        std::vector<std::vector<uint8_t> > m_structs;
        uint32_t                           m_structs_size;
    };
};

#endif /* MISC_STRUCT_CHAINER_H */
