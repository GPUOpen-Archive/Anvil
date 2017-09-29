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

#include "misc/debug.h"
#include "misc/page_tracker.h"

/** Please see header for specification */
Anvil::PageTracker::PageTracker(VkDeviceSize in_region_size,
                                VkDeviceSize in_page_size)
    :m_n_total_pages              (static_cast<uint32_t>(in_region_size / in_page_size) ),
     m_page_size                  (in_page_size),
     m_n_pages_with_memory_backing(0),
     m_region_size                (in_region_size)
{
    m_sparse_page_occupancy.resize(
        1 + m_n_total_pages / (sizeof(uint32_t) * 8 /* bits in byte */)
    );

}

/** Please see header for specification */
std::shared_ptr<Anvil::MemoryBlock> Anvil::PageTracker::get_memory_block(VkDeviceSize  in_start_offset_page_aligned,
                                                                         VkDeviceSize* out_memory_region_start_offset_ptr)
{
    std::shared_ptr<Anvil::MemoryBlock> result_ptr;

    /* Sanity checks */
    if ((in_start_offset_page_aligned % m_page_size) != 0)
    {
        anvil_assert( (in_start_offset_page_aligned % m_page_size) == 0);

        goto end;
    }

    if (in_start_offset_page_aligned + m_page_size > m_n_total_pages * m_page_size)
    {
        anvil_assert(!(in_start_offset_page_aligned + m_page_size > m_n_total_pages * m_page_size) );

        goto end;
    }

    /* Handle the request */
    for (const auto& current_memory_block : m_memory_blocks)
    {
        if (current_memory_block.start_offset                             <= in_start_offset_page_aligned               &&
            current_memory_block.start_offset + current_memory_block.size >= in_start_offset_page_aligned + m_page_size)
        {
            result_ptr                          = current_memory_block.memory_block_ptr;
            *out_memory_region_start_offset_ptr = current_memory_block.start_offset + in_start_offset_page_aligned;

            break;
        }
    }

end:
    return result_ptr;
}

/** Please see header for specification */
bool Anvil::PageTracker::set_binding(std::shared_ptr<MemoryBlock> in_memory_block_ptr,
                                     VkDeviceSize                 in_memory_block_start_offset,
                                     VkDeviceSize                 in_start_offset,
                                     VkDeviceSize                 in_size)
{
    uint32_t n_pages;
    uint32_t occupancy_item_start_index;
    uint32_t occupancy_vec_start_index;
    bool     result                     = false;

    /* Sanity checks */
    if (in_start_offset + in_size > m_region_size)
    {
        anvil_assert(!(in_start_offset + in_size > m_region_size) );

        goto end;
    }

    if ((in_start_offset % m_page_size) != 0)
    {
        anvil_assert(!(in_start_offset % m_page_size) != 0);

        goto end;
    }

    if ((in_size % m_page_size) != 0)
    {
        anvil_assert(!(in_size % m_page_size) != 0);

        goto end;
    }

    /* Store the memory block binding */
    n_pages                    = static_cast<uint32_t>(in_size         / m_page_size);
    occupancy_item_start_index = static_cast<uint32_t>(in_start_offset % m_page_size);
    occupancy_vec_start_index  = static_cast<uint32_t>(in_start_offset / m_page_size / 32 /* pages in a single vec item */);

    for (auto mem_binding_iterator  = m_memory_blocks.begin();
              mem_binding_iterator != m_memory_blocks.end();
            ++mem_binding_iterator)
    {
        auto& mem_binding = *mem_binding_iterator;

        const bool does_not_overlap = (in_start_offset + in_size -1                    < mem_binding.start_offset || /* XXYY , X = new binding, Y = this binding */
                                       mem_binding.start_offset + mem_binding.size - 1 < in_start_offset);           /* YYXX */

        if (does_not_overlap)
        {
            continue;
        }

        /* There are four cases we need to handle separately :
         *
         * 1)  ###      - The new block _ interleaves with current memory block # from the left side.
         *    __          Need to truncate # from the left side.
         *
         * 2)  ###      - The new block _ splits current memory block # in half.
         *      _         Need to split # to two separate memory blocks.
         *
         * 3)  ###      - The new block _ interleaves with current memory block # from the right side.
         *       __       Need to truncate # from the right side.
         *
         * 4)  ###      - The new block _ is going to be completely overwritten by _
         *    _____       Need to remove it.
         */
        if ( in_start_offset            <=  mem_binding.start_offset                     &&
             in_start_offset            <  (mem_binding.start_offset + mem_binding.size) &&
            (in_start_offset + in_size) >=  mem_binding.start_offset                     &&
            (in_start_offset + in_size) <  (mem_binding.start_offset + mem_binding.size) )
        {
            /* Case 1 */
            mem_binding.size         -= in_start_offset + in_size - (mem_binding.start_offset);
            mem_binding.start_offset  = in_start_offset + in_size;
        }
        else
        if ( in_start_offset            >  mem_binding.start_offset                     &&
             in_start_offset            < (mem_binding.start_offset + mem_binding.size) &&
            (in_start_offset + in_size) >  mem_binding.start_offset                     &&
            (in_start_offset + in_size) < (mem_binding.start_offset + mem_binding.size))
        {
            /* Case 2 */
            MemoryBlockBinding left_half (mem_binding.memory_block_ptr,
                                          mem_binding.memory_block_start_offset,
                                          in_start_offset - mem_binding.start_offset, /* in_size         */
                                          mem_binding.start_offset);                  /* in_start_offset */
            MemoryBlockBinding right_half(mem_binding.memory_block_ptr,
                                          mem_binding.memory_block_start_offset,
                                          mem_binding.start_offset + mem_binding.size - (in_start_offset + in_size) + 1, /* in_size         */
                                          in_start_offset + in_size);                                                    /* in_start_offset */

            m_memory_blocks.push_back(left_half);
            m_memory_blocks.push_back(right_half);

            m_memory_blocks.erase(mem_binding_iterator);
        }
        else
        if ( in_start_offset            >  mem_binding.start_offset                     &&
             in_start_offset            > (mem_binding.start_offset + mem_binding.size) &&
            (in_start_offset + in_size) >  mem_binding.start_offset                     &&
            (in_start_offset + in_size) > (mem_binding.start_offset + mem_binding.size) )
        {
            /* Case 3 */
            mem_binding.size = (mem_binding.start_offset + mem_binding.size) - in_start_offset + 1;
        }
        else
        {
            /* Must be case 4 */
            anvil_assert( in_start_offset            <=  mem_binding.start_offset                     &&
                         (in_start_offset + in_size) >= (mem_binding.start_offset + mem_binding.size) );

            m_memory_blocks.erase(mem_binding_iterator);
        }

        /* No more than one memory binding can be assigned to the same memory region */
        break;
    }

    m_memory_blocks.push_back(
        MemoryBlockBinding(
            in_memory_block_ptr,
            in_memory_block_start_offset,
            in_size,
            in_start_offset)
        );

    /* Update page occupancy info
     *
     * TODO: Perf of the code below could definitely be improved
     */
    for (uint32_t n_page = 0;
                  n_page < n_pages;
                ++n_page)
    {
        uint32_t occupancy_item_index = occupancy_item_start_index + n_page;
        uint32_t occupancy_vec_index  = occupancy_vec_start_index;

        while (occupancy_item_index >= 32)
        {
            occupancy_item_index -= 32;
            occupancy_vec_index  ++;
        }

        if (in_memory_block_ptr != nullptr)
        {
            m_sparse_page_occupancy[occupancy_vec_index].raw |= (1 << occupancy_item_index);

            ++m_n_pages_with_memory_backing;
        }
        else
        {
            m_sparse_page_occupancy[occupancy_vec_index].raw &= ~(1 << occupancy_item_index);

            --m_n_pages_with_memory_backing;
        }
    }

    anvil_assert(m_n_pages_with_memory_backing <= m_n_total_pages);
    result = true;
end:
    return result;
}