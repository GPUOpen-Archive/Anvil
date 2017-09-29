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

#ifndef MISC_PAGE_TRACKER_H
#define MISC_PAGE_TRACKER_H

#include "misc/types.h"


namespace Anvil
{
    /** Tracks memory page bindings for sparse images & sparse buffers */
    class PageTracker
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  @param in_region_size Size of the memory region to track. Must not be 0.
         *  @param in_page_size   Size of a single memory page. Must not be 0.
         **/
        explicit PageTracker(VkDeviceSize in_region_size,
                             VkDeviceSize in_page_size);

        /** Retrieves a memory block assigned to the region <in_start_offset_page_aligned, in_start_offset_page_aligned + (page size)>.
         *
         *  NOTE: The caller should NOT assume subsequent pages are assigned the same memory block, unless the application never binds
         *        more than one memory block to a sparsely-bound buffer / image.
         *
         *  @param in_start_offset_page_aligned       Start offset of the page. Must be page size-aligned.
         *  @param out_memory_region_start_offset_ptr Deref will be set to the start offset, which corresponds to @param in_start_offset_page_aligned
         *                                            from the returned memory object's PoV. Must not be NULL.
         *
         *  @return Memory block instance bound to the specified page OR null, if no memory block has been assigned to the memory region.
         */
        std::shared_ptr<Anvil::MemoryBlock> get_memory_block(VkDeviceSize  in_start_offset_page_aligned,
                                                             VkDeviceSize* out_memory_region_start_offset_ptr);

        /** The same memory block is often bound to more than just one page. PageTracker
         *  coalesces such occurences into a single descriptor.
         *
         *  This function can be used to retrieve a memory block, bound to a descriptor
         *  at a given index (@param in_n_memory_block).
         *
         *  @param in_n_memory_block See above. Must not be equal or larger than value returned
         *                           by get_n_memory_blocks().
         *
         *  @return The requested memory block.
         */
        std::shared_ptr<Anvil::MemoryBlock> get_memory_block(uint32_t in_n_memory_block) const
        {
            anvil_assert(in_n_memory_block < m_memory_blocks.size() );

            return m_memory_blocks.at(in_n_memory_block).memory_block_ptr;
        }

        /** Returns the number of disjoint memory blocks */
        uint32_t get_n_memory_blocks() const
        {
            return static_cast<uint32_t>(m_memory_blocks.size() );
        }

        /** Returns total number of pages */
        uint32_t get_n_pages() const
        {
            return m_n_total_pages;
        }

        /** Returns the number of pages that have been assigned non-null memory blocks */
        uint32_t get_n_pages_with_memory_backing() const
        {
            return m_n_pages_with_memory_backing;
        }

        /* Returns page size, as recognized by the page tracker */
        VkDeviceSize get_page_size() const
        {
            return m_page_size;
        }

        /** Updates a locally tracked memory binding.
         *
         *  @param in_memory_block_ptr          Memory block that is going to be bound. May be null,
         *                                      in which case it is assumed no physical memory backing
         *                                      is now associated with the specified memory region.
         *  @param in_memory_block_start_offset Start offset relative to @param in_memory_block_ptr's buffer
         *                                      memory, from which physical memory should be assigned.
         *
         *  @param in_start_offset              Start offset, relative to the tracked memory region,
         *                                      from which @param in_memory_block_ptr is to be bound.
         *  @param in_size                      Size of the memory region which should used for the binding.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_binding(std::shared_ptr<MemoryBlock> in_memory_block_ptr,
                         VkDeviceSize                 in_memory_block_start_offset,
                         VkDeviceSize                 in_start_offset,
                         VkDeviceSize                 in_size);

    private:
        /* Private type definitions */
        typedef struct MemoryBlockBinding
        {
            std::shared_ptr<MemoryBlock> memory_block_ptr;
            VkDeviceSize                 memory_block_start_offset;
            VkDeviceSize                 size;
            VkDeviceSize                 start_offset;

            MemoryBlockBinding(std::shared_ptr<MemoryBlock> in_memory_block_ptr,
                               VkDeviceSize                 in_memory_block_start_offset,
                               VkDeviceSize                 in_size,
                               VkDeviceSize                 in_start_offset)
            {
                memory_block_ptr          = in_memory_block_ptr;
                memory_block_start_offset = in_memory_block_start_offset;
                size                      = in_size;
                start_offset              = in_start_offset;
            }
        } MemoryBlockBinding;

        /* Private functions */

        /* Private variables */
        std::vector<MemoryBlockBinding>  m_memory_blocks;
        uint32_t                         m_n_memory_blocks_with_memory_backing;
        uint32_t                         m_n_pages_with_memory_backing;
        uint32_t                         m_n_total_pages;
        VkDeviceSize                     m_page_size;
        VkDeviceSize                     m_region_size;
        std::vector<PageOccupancyStatus> m_sparse_page_occupancy;
    };
}; /* namespace Anvil */

#endif /* MISC_PAGE_TRACKER_H */