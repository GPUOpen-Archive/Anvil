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

/* Defines a MemoryAllocator class which allocates & maintains a memory block for all registered
 * objects. At baking time, non-overlapping regions of memory storage are distributed to the objects,
 * respect to object-specific alignment requirements.
 *
 * The allocator uses a single memory heap for all allocations, so it may not work in all cases.
 * This will be improved at some point in the future.
 **/
#ifndef MISC_MEMORY_ALLOCATOR_H
#define MISC_MEMORY_ALLOCATOR_H

#include "misc/types.h"

#include <vector>


namespace Anvil
{
    /** Implements a simple memory allocator. For more details, please see the header. */
    class MemoryAllocator
    {
    public:
        /* Public functions */

        /** Adds a new Buffer object which should use storage coming from the buffer memory
         *  maintained by the Memory Allocator.
         *
         *  @param buffer_ptr Buffer to configure storage for at bake() call time. Must not
         *                    be nullptr.
         **/
        void add_buffer(Anvil::Buffer* buffer_ptr);

        /** Adds a new Image object which should use storage coming from the buffer memory
         *  maintained by the Memory Allocator.
         *
         *  @param image_ptr Image to configure storage for at bake() call time. Must not
         *                   be nullptr.
         **/
        void add_image(Anvil::Image* image_ptr);

        /** Tries to create a memory object of size large enough to capacitate all added objects,
         *  given their alignment, size, and other requirements.
         *
         *  If the call is successful, each added object will have its set_memory() entry-point
         *  called with more details about what memory object they should use, along with
         *  a start offset and size of the granted allocation.
         *
         *  The allocations are guaranteed not to overlap.
         *
         *  @return true if successful, false otherwise.
         **/
        bool bake();

        /** Constructor.
         *
         *  @param device_ptr               Device to use.
         *  @param mappable_memory_required true if the allocated buffer storage should come
         *                                  from a host-visible memory backing; false otherwise.
         *  @param coherent_memory_required true if the allocated buffer storage should come
         *                                  from a coherent memory backing; false otherwise.
         **/
         MemoryAllocator(Anvil::Device* device_ptr,
                         bool           mappable_memory_required,
                         bool           coherent_memory_required);

         /** Destructor.
          *
          *  Releases the underlying MemoryBlock instance
          **/
        ~MemoryAllocator();

    private:
        /* Private type declarations */
        typedef enum
        {
            ITEM_TYPE_BUFFER,
            ITEM_TYPE_IMAGE
        } ItemType;

        typedef struct Item
        {
            union
            {
                Anvil::Buffer* buffer_ptr;
                Anvil::Image*  image_ptr;
            };

            ItemType type;

            VkDeviceSize alloc_offset;
            uint32_t     alloc_memory_types;
            VkDeviceSize alloc_size;

            Item(Anvil::Buffer* in_buffer_ptr,
                 VkDeviceSize   in_alloc_offset,
                 VkDeviceSize   in_alloc_size,
                 uint32_t       in_alloc_memory_types)
            {
                alloc_memory_types = in_alloc_memory_types;
                alloc_offset       = in_alloc_offset;
                alloc_size         = in_alloc_size;
                buffer_ptr         = in_buffer_ptr;
                type               = ITEM_TYPE_BUFFER;
            }

            Item(Anvil::Image* in_image_ptr,
                 VkDeviceSize  in_alloc_offset,
                 VkDeviceSize  in_alloc_size,
                 uint32_t      in_alloc_memory_types)
            {
                alloc_memory_types = in_alloc_memory_types;
                alloc_offset       = in_alloc_offset;
                alloc_size         = in_alloc_size;
                image_ptr          = in_image_ptr;
                type               = ITEM_TYPE_IMAGE;
            }
        } Item;

        typedef std::vector<Item> Items;

        /* Private functions */
        MemoryAllocator           (const MemoryAllocator&);
        MemoryAllocator& operator=(const MemoryAllocator&);

        /* Private members */
        bool           m_coherent_memory_required;
        bool           m_mappable_memory_required;

        Anvil::Device*      m_device_ptr;
        bool                m_is_baked;
        Items               m_items;
        Anvil::MemoryBlock* m_memory_block_ptr;
        VkDeviceSize        m_needed_memory_size;
    };
};

#endif /* WRAPPERS_MEMORY_ALLOCATOR_H */