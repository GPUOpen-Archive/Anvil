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

#include "misc/debug.h"
#include "misc/memory_allocator.h"
#include "wrappers/buffer.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"


/* Please see header for specification */
Anvil::MemoryAllocator::MemoryAllocator(Anvil::Device* device_ptr,
                                        bool           mappable_memory_required,
                                        bool           coherent_memory_required)
    :m_device_ptr              (device_ptr),
     m_is_baked                (false),
     m_coherent_memory_required(coherent_memory_required),
     m_mappable_memory_required(mappable_memory_required),
     m_memory_block_ptr        (nullptr),
     m_needed_memory_size      (0)
{
    /* Stub */
}

/* Please see header for specification */
Anvil::MemoryAllocator::~MemoryAllocator()
{
    if (m_memory_block_ptr != nullptr)
    {
        m_memory_block_ptr->release();

        m_memory_block_ptr = nullptr;
    }
}


/* Please see header for specification */
void Anvil::MemoryAllocator::add_buffer(Anvil::Buffer* buffer_ptr)
{
    VkDeviceSize buffer_alignment    = 0;
    uint32_t     buffer_memory_types = 0;
    VkDeviceSize buffer_storage_size = 0;

    /* Sanity checks */
    anvil_assert(!m_is_baked);
    anvil_assert(buffer_ptr != nullptr);

    /* Determine how much space we're going to need, what alignment we need
     * to consider, and so on. */
    const VkMemoryRequirements memory_reqs = buffer_ptr->get_memory_requirements();

    buffer_alignment    = memory_reqs.alignment;
    buffer_memory_types = memory_reqs.memoryTypeBits;
    buffer_storage_size = memory_reqs.size;

    m_needed_memory_size += (buffer_alignment - m_needed_memory_size % buffer_alignment) % buffer_alignment;

    anvil_assert((m_needed_memory_size % buffer_alignment) == 0);

    /* Store a new block item descriptor */
    m_items.push_back(Item(buffer_ptr,
                           m_needed_memory_size,
                           buffer_storage_size,
                           buffer_memory_types) );

    m_needed_memory_size += buffer_storage_size;
}

/* Please see header for specification */
void Anvil::MemoryAllocator::add_image(Anvil::Image* image_ptr)
{
    VkDeviceSize image_alignment    = 0;
    uint32_t     image_memory_types = 0;
    VkDeviceSize image_storage_size = 0;

    /* Sanity checks */
    anvil_assert(!m_is_baked);
    anvil_assert(image_ptr != nullptr);

    /* Determine how much size is needed for the image's storage, as well as what
     * the allocation requirements are */
    image_alignment    = image_ptr->get_image_alignment();
    image_memory_types = image_ptr->get_image_memory_types();
    image_storage_size = image_ptr->get_image_storage_size();

    /* Determine the offset, from which the image storage can legally start from */
    m_needed_memory_size += (image_alignment - m_needed_memory_size % image_alignment) % image_alignment;

    anvil_assert((m_needed_memory_size % image_alignment) == 0);

    /* Store a new block item descriptor */
    m_items.push_back(Item(image_ptr,
                           m_needed_memory_size,
                           image_storage_size,
                           image_memory_types) );

    m_needed_memory_size += image_storage_size;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::bake()
{
    uint32_t allowed_memory_types = 0xFFFFFFFF;
    bool     result               = false;

    /* Sanity checks */
    anvil_assert(!m_is_baked);
    anvil_assert(m_items.size() > 0);
    anvil_assert(m_needed_memory_size > 0);

    /* Iterate over all block items and determine what memory types we can use. */
    for (auto item_iterator  = m_items.cbegin();
              item_iterator != m_items.cend();
            ++item_iterator)
    {
        allowed_memory_types &= item_iterator->alloc_memory_types;
    }

    if (allowed_memory_types == 0)
    {
        /* A single memory block cannot be used for this set of allocations on the running
         * platform.. */
        anvil_assert(allowed_memory_types != 0);

        goto end;
    }

    /* Allocate the required memory region .. */
    m_memory_block_ptr = new Anvil::MemoryBlock(m_device_ptr,
                                                allowed_memory_types,
                                                m_needed_memory_size,
                                                m_mappable_memory_required,
                                                m_coherent_memory_required);

    /* Distribute memory regions to the registered objects */
    for (auto item_iterator  = m_items.cbegin();
              item_iterator != m_items.cend();
            ++item_iterator)
    {
        Anvil::MemoryBlock* memory_block_ptr = new Anvil::MemoryBlock(m_memory_block_ptr,
                                                                      item_iterator->alloc_offset,
                                                                      item_iterator->alloc_size);

        switch (item_iterator->type)
        {
            case ITEM_TYPE_BUFFER:
            {
                item_iterator->buffer_ptr->set_memory(memory_block_ptr);

                memory_block_ptr->release();
                break;
            }

            case ITEM_TYPE_IMAGE:
            {
                item_iterator->image_ptr->set_memory(memory_block_ptr);

                memory_block_ptr->release();
                break;
            }

            default:
            {
                anvil_assert(false);
            }
        }
    }

    m_is_baked = true;
end:
    return result;
}

