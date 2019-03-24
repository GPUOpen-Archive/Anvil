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

/* Implements a memory allocator backend which acts as an adapter to AMD Vulkan
 * Memory Allocator library, allowing the applications to request for memory
 * allocations as many times as they need.
 *
 * This class should only be used internally by MemoryAllocator.
 **/
#ifndef MISC_MEMORY_ALLOCATOR_BACKEND_VMA_H
#define MISC_MEMORY_ALLOCATOR_BACKEND_VMA_H

#include "misc/types.h"
#include "misc/memory_allocator.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"


namespace Anvil
{
    namespace MemoryAllocatorBackends
    {
        /* VMA memory allocator backend implementation.
         *
         * Should only be used by Anvil::MemoryAllocator
         */
        class VMA : public Anvil::MemoryAllocator::IMemoryAllocatorBackend,
                    public std::enable_shared_from_this<VMA>
        {
        public:
            /* Public functions */

            /** Creates a new VMA memory allocator backend instance.
             *
             *  Should only be used internally by MemoryAllocator.
             *
             *  @param in_device_ptr Vulkan device the memory allocations are going to be made for.
             **/
            static std::unique_ptr<VMA> create(const Anvil::BaseDevice* in_device_ptr);

            /** Destructor. */
            virtual ~VMA();

        private:
            /* Private type definitions */

            /** Wrapper for the Vulkan Memory Allocator handle. Also includes additional code required
             *  to prolong the destruction of the allocator only until all memory blocks which have been
             *  assigned memory backing by the VMA allocator have gone out of scope.
             */
            class VMAAllocator : public std::enable_shared_from_this<VMAAllocator>
            {
            public:
                /* Public functions */

                /** Constructor.
                 *
                 *  @param in_device_ptr Device the allocator should be initialized for.
                 **/
                static std::shared_ptr<VMAAllocator> create(const Anvil::BaseDevice* in_device_ptr);

                /** Destructor */
                virtual ~VMAAllocator();

                /** Returns the raw VMA allocator handle. */
                VmaAllocator get_handle() const
                {
                    return m_allocator;
                }

                /** Entry-point which should be called by VMA class every time a new memory block instance is created
                 *  from a memory region returned by the VMA library.
                 */
                void on_new_vma_mem_block_alloced();

                /** Entry-point which should be called by memory blocks when they are about to be released.
                 *
                 *  The function returns the underlying memory region back to the VMA library.
                 *
                 *  @param in_memory_block_ptr Raw pointer to the memory block which is about to be destroyed.
                 *                             Must NOT be null. Must be valid at call time.
                 *  @param in_vma_allocation   TODO.
                 */
                void on_vma_alloced_mem_block_gone_out_of_scope(Anvil::MemoryBlock* in_memory_block_ptr,
                                                                VmaAllocation       in_vma_allocation);

            private:
                /* Private functions */

                VMAAllocator(const Anvil::BaseDevice* in_device_ptr);

                bool init();

                /* Private variables */
                VmaAllocator                        m_allocator;
                const Anvil::BaseDevice*            m_device_ptr;
                std::unique_ptr<VmaVulkanFunctions> m_vma_func_ptrs;

                std::vector<std::shared_ptr<VMAAllocator> > m_refcount_helper;
            };

            /* Private functions */

            VMA(const Anvil::BaseDevice* in_device_ptr);

            bool init();

            /* IMemoryAllocatorBackend functions */

            bool     bake                            (Anvil::MemoryAllocator::Items&              in_items) final;
            VkResult map                             (void*                                       in_memory_object,
                                                      VkDeviceSize                                in_start_offset,
                                                      VkDeviceSize                                in_memory_block_start_offset,
                                                      VkDeviceSize                                in_size,
                                                      void**                                      out_result_ptr);
            bool     supports_baking                 () const final;
            bool     supports_device_masks           ()                                                                            const final;
            bool     supports_external_memory_handles(const Anvil::ExternalMemoryHandleTypeFlags& in_external_memory_handle_types) const final;
            bool     supports_protected_memory       ()                                                                            const final;
            void     unmap                           (void*                                       in_memory_object);

            /* Private variables */
            const Anvil::BaseDevice*            m_device_ptr;
            std::shared_ptr<VMAAllocator>       m_vma_allocator_ptr;
        };
    };
};

#endif /* MISC_MEMORY_ALLOCATOR_BACKEND_VMA_H */