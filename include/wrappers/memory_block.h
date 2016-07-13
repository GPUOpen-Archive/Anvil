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

/** Defines a MemoryBlock class, which is a wrapper for Vulkan device memory objects.
 *
 *  Additionally, the class:
 *
 *  - provides a read() function which works for buffer objects with coherent & non-coherent
 *    memory backing.
 *  - provides a write() function which works just as read().
 *  - if more than one read() or write() calls is necessary, the class exposes a function which
 *    lets its users map the block's storage into process space. Then, the user should issue
 *    a number of read & write ops, after which the object can be unmapped.
 *  - provides a way to create derivative memory blocks, whose storage is "carved out" of the
 *    parent memory block's. Note that only two-level hierarchy is supported (but could be
 *    extended if necessary).
 **/
#ifndef WRAPPERS_MEMORY_BLOCK_H
#define WRAPPERS_MEMORY_BLOCK_H

#include "misc/ref_counter.h"
#include "misc/types.h"


namespace Anvil
{
    /** Wrapper class for memory objects. Please see header for more details */
    class MemoryBlock : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor which should be used to create and bind a new device memory object
         *  to the instantiated MemoryBlock object.
         *
         *  @param device_ptr          Device to use.
         *  @param allowed_memory_bits Memory type bits which meet the allocation requirements.
         *  @param size                Required allocation size.
         *  @param should_be_mappable  true if the underlying memory object should come from a heap
         *                             which is host-visible. False, if the memory block is never going
         *                             to be mapped into process space.
         *  @param should_be_coherent  true if the underlying memory backing should come from a heap
         *                             which support coherent accesses. false otherwise.
         **/
         MemoryBlock(Anvil::Device* device_ptr,
                     uint32_t       allowed_memory_bits,
                     VkDeviceSize   size,
                     bool           should_be_mappable,
                     bool           should_be_coherent);

        /** Constructor which should be used to create a memory block whose storage space is
         *  maintained by another MemoryBlock instance.
         *
         *  The specified parent memory block is going to be retained.
         *
         *  @param parent_memory_block_ptr MemoryBlock instance to use as a parent. Must not be nullptr.
         *                                 Parent memory block must not have any parent on its own.
         *                                 Parent memory block must not be mapped.
         *  @param start_offset            Start offset of the storage maintained by the specified parent memory block,
         *                                 from which the new MemoryBlock instance's storage should start.
         *                                 Must not be equal to or larger than parent object's storage size.
         *                                 When added to @param size, the result value must not be be larger than the remaining
         *                                 storage size.
         *  @param size
         **/
        MemoryBlock(MemoryBlock* parent_memory_block_ptr,
                    VkDeviceSize start_offset,
                    VkDeviceSize size);

        /* Returns the underlying raw Vulkan VkDeviceMemory handle. */
        const VkDeviceMemory& get_memory() const
        {
            if (m_parent_memory_block_ptr != nullptr)
            {
                return m_parent_memory_block_ptr->m_memory;
            }
            else
            {
                return m_memory;
            }
        }

        /** Returns the memory type index the memory block was allocated from */
        uint32_t get_memory_type_index() const
        {
            if (m_parent_memory_block_ptr != nullptr)
            {
                return m_parent_memory_block_ptr->get_memory_type_index();
            }
            else
            {
                return m_memory_type_index;
            }
        }

        /* Returns the size of the memory block. */
        VkDeviceSize get_size() const
        {
            return m_size;
        }

        /* Returns the start offset of the memory block.
         *
         * If the memory block has a parent, the returned start offset is relative to the parent memory block's
         * start offset (in other words: the returned value doesn't include it)
         */
        VkDeviceSize get_start_offset() const
        {
            return m_start_offset;
        }

        /** Tells whether the underlying memory region is coherent */
        bool is_coherent() const
        {
            if (m_parent_memory_block_ptr != nullptr)
            {
                return m_parent_memory_block_ptr->is_coherent();
            }
            else
            {
                return m_is_coherent;
            }
        }

        /** Tells whether the underlying memory region is mappable */
        bool is_mappable() const
        {
            if (m_parent_memory_block_ptr != nullptr)
            {
                return m_parent_memory_block_ptr->is_mappable();
            }
            else
            {
                return m_is_mappable;
            }
        }

        /** Maps the specified region of the underlying memory object to the process space.
         *
         *  Neither the object, nor its parent (if one is defined) is allowed to be mapped
         *  at the time of the call.
         *
         *  The specified memory region to be mapped must be fully located within the
         *  boundaries of maintained storage space.
         *
         *  @param start_offset     Offset, from which the mapped region should start.
         *  @param size             Size of the region to be mapped. Must not be 0.
         *  @param opt_out_data_ptr If not null, deref will be set to the result pointer.
         *                          It is recommended to use memory block's read() & write()
         *                          functions to access GPU memory, although in some cases
         *                          a raw pointer may be useful. May be nullptr.
         **/
        bool map(VkDeviceSize start_offset,
                 VkDeviceSize size,
                 void**       opt_out_data_ptr = nullptr);

        /** Reads data from the specified region of the underlying memory object after mapping
         *  it into process space and copies it to the user-specified location.
         *  If the buffer object uses non-coherent memory backing, the region will first be
         *  invalidated to ensure the reads return valid data.
         *
         *  This function does not require the caller to issue a map() call, prior to being called.
         *  However, making that call in advance will skip map()+unmap() invocations, wnich would
         *  otherwise have to be done for each read() call.
         *
         *  @param start_offset   Start offset of the region to be mapped.
         *  @param size           Size of the region to be mapped.
         *  @param out_result_ptr The read data will be copied to the location specified by this
         *                        argument. Must not be nullptr.
         *
         *  @return true if the call was successful, false otherwise.
         **/
        bool read(VkDeviceSize start_offset,
                  VkDeviceSize size,
                  void*        out_result_ptr);

        /** Unmaps the mapped storage from the process space.
         *
         *  The call should only be made after a map() call.
         */
        bool unmap();

        /** Writes user data to the specified region of the underlying memory object after mapping
         *  it into process space.
         *  If the buffer object uses non-coherent memory backing, the modified regions will be
         *  flushed to ensure GPU can access the latest data after this call finishes.
         *
         *  This function does not require the caller to issue a map() call, prior to being called.
         *  However, making that call in advance will skip map()+unmap() invocations, wnich would
         *  otherwise have to be done for each write() call.
         *
         *  @param start_offset Start offset of the region to modify
         *  @param size         Size of the region to be modified.
         *  @param data         Data to be copied to the specified GPU memory region.
         *
         *  @return true if the call was successful, false otherwise.
         **/
        bool write(VkDeviceSize start_offset,
                   VkDeviceSize size,
                   const void*  data);

    private:
        /* Private functions */
        MemoryBlock           (const MemoryBlock&);
        MemoryBlock& operator=(const MemoryBlock&);

        virtual ~MemoryBlock();

        void     close_gpu_memory_access     ();
        uint32_t get_device_memory_type_index(uint32_t     memory_type_bits,
                                              bool         mappable_memory_required,
                                              bool         coherent_memory_required);
        bool     open_gpu_memory_access      (VkDeviceSize start_offset,
                                              VkDeviceSize size);

        /* Private members */
        void*                m_gpu_data_ptr;
        bool                 m_gpu_data_user_mapped;
        VkDeviceSize         m_gpu_data_user_size;
        VkDeviceSize         m_gpu_data_user_start_offset;

        uint32_t            m_allowed_memory_bits;
        Anvil::Device*      m_device_ptr;
        bool                m_is_coherent;
        bool                m_is_mappable;
        VkDeviceMemory      m_memory;
        uint32_t            m_memory_type_index;
        Anvil::MemoryBlock* m_parent_memory_block_ptr;
        VkDeviceSize        m_size;
        VkDeviceSize        m_start_offset;
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_MEMORY_BLOCK_H */
