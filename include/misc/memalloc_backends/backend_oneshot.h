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

/* Implements a memory allocator backend which allocates & maintains one or more memory blocks for all
 * registered objects. At baking time, non-overlapping regions of memory storage are distributed to the objects,
 * with respect to object-specific alignment requirements.
 *
 * The allocator can only handle one bake request throughout its life-time.
 *
 * This class should only be used internally by MemoryAllocator.
 **/
#ifndef MISC_MEMORY_ALLOCATOR_BACKEND_ONESHOT_H
#define MISC_MEMORY_ALLOCATOR_BACKEND_ONESHOT_H

#include "misc/types.h"
#include "misc/memory_allocator.h"

namespace Anvil
{
    namespace MemoryAllocatorBackends
    {
        /* One-shot memory allocator backend implementation.
         *
         * Should only be used by Anvil::MemoryAllocator
         */
        class OneShot : public Anvil::MemoryAllocator::IMemoryAllocatorBackend,
                        public std::enable_shared_from_this<OneShot>
        {
        public:
            /* Public functions */

            /** Creates a new one-shot memory allocator backend instance.
             *
             *  Should only be used internally by MemoryAllocator.
             *
             *  @param in_device_ptr Vulkan device the memory allocations are going to be made for.
             **/
            OneShot(const Anvil::BaseDevice* in_device_ptr);

            /** Destructor. */
            virtual ~OneShot();

        private:
            /* IMemoryAllocatorBackend functions */

            bool     bake                            (Anvil::MemoryAllocator::Items&              in_items) final;
            VkResult map                             (void*                                       in_memory_object,
                                                      VkDeviceSize                                in_start_offset,
                                                      VkDeviceSize                                in_memory_block_start_offset,
                                                      VkDeviceSize                                in_size,
                                                      void**                                      out_result_ptr) final;
            bool     supports_baking                 () const final;
            bool     supports_external_memory_handles(const Anvil::ExternalMemoryHandleTypeFlags& in_external_memory_handle_types) const final;
            bool     supports_device_masks           ()                                                                            const final;
            bool     supports_protected_memory       ()                                                                            const final;
            void     unmap                           (void*                                       in_memory_object) final;

            /* Private functions */

            /* Private variables */
            const Anvil::BaseDevice*          m_device_ptr;
            bool                              m_is_baked;
            std::vector<MemoryBlockUniquePtr> m_memory_blocks;
        };
    };
};

#endif /* MISC_MEMORY_ALLOCATOR_BACKEND_ONESHOT_H */