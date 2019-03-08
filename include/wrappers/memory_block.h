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
 *    parent memory block's.
 **/
#ifndef WRAPPERS_MEMORY_BLOCK_H
#define WRAPPERS_MEMORY_BLOCK_H

#include "misc/mt_safety.h"
#include "misc/types.h"
#include "misc/memory_block_create_info.h"

namespace Anvil
{
    typedef class IMemoryBlockBackendSupport
    {
    public:
        virtual ~IMemoryBlockBackendSupport()
        {
            /* Stub */
        }

        virtual void set_parent_memory_allocator_backend_ptr(std::shared_ptr<Anvil::IMemoryAllocatorBackendBase> in_backend_ptr,
                                                             void*                                               in_backend_object) = 0;
    } IMemoryBlockBackendSupport;

    /** Wrapper class for memory objects. Please see header for more details */
    class MemoryBlock : public IMemoryBlockBackendSupport,
                        public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /* TODO
         *
         * @param in_create_info_ptr TODO
         * @param out_opt_result_ptr If not null, deref will be set to the error code reported by the API function
         *                           used to allocate the memory.
         *
         * @return New memory block instance if successful, null otherwise.
         */
        static MemoryBlockUniquePtr create(Anvil::MemoryBlockCreateInfoUniquePtr in_create_info_ptr,
                                           VkResult*                             out_opt_result_ptr  = nullptr);

        /* Creates a new external memory handle of the user-specified type.
         *
         * For NT handles, if one has been already created for this memory block instance & handle type, a cached instance
         * of the handle will be returned instead. Otherwise, each create() call will return a new handle.
         *
         * Cached external memory handles will be destroyed & released at MemoryBlock's destruction time.
         *
         * Supports DERIVED and REGULAR memory blocks. The latter case is only supported if memory region covered by
         * the derived region completely encapsulates the underlying Vulkan allocation.
         *
         * Returns nullptr if unsuccessful.
         *
         * Requires VK_KHR_external_memory_fd    under Linux.
         * Requires VK_KHR_external_memory_win32 under Windows.
         */
        ExternalHandleUniquePtr export_to_external_memory_handle(const Anvil::ExternalMemoryHandleTypeFlagBits& in_memory_handle_type);

        /** Releases the Vulkan counterpart and unregisters the wrapper instance from the object tracker */
        virtual ~MemoryBlock();

        const Anvil::MemoryBlockCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        /* Returns the underlying raw Vulkan VkDeviceMemory handle. */
        const VkDeviceMemory& get_memory() const
        {
            auto parent_mem_block_ptr = m_create_info_ptr->get_parent_memory_block();

            if (parent_mem_block_ptr != nullptr)
            {
                return parent_mem_block_ptr->m_memory;
            }
            else
            {
                return m_memory;
            }
        }

        const VkDeviceSize& get_start_offset() const
        {
            return m_start_offset;
        }

        /** Checks if the memory range covered by this memory block intersects with memory range covered
         *  by the user-specified memory block
         *
         *  @param in_memory_block_ptr
         *
         *  @return true if intersection has been detected, false otherwise. */
        bool intersects(const Anvil::MemoryBlock* in_memory_block_ptr) const;

        /** Maps the specified region of the underlying memory object to the process space.
         *
         *  Neither the object, nor its parent(s) is allowed to be mapped
         *  at the time of the call.
         *
         *  The specified memory region to be mapped must be fully located within the
         *  boundaries of maintained storage space.
         *
         *  @param in_start_offset  Offset, from which the mapped region should start.
         *  @param in_size          Size of the region to be mapped. Must not be 0.
         *  @param out_opt_data_ptr If not null, deref will be set to the result pointer.
         *                          It is recommended to use memory block's read() & write()
         *                          functions to access GPU memory, although in some cases
         *                          a raw pointer may be useful. May be nullptr.
         **/
        bool map(VkDeviceSize in_start_offset,
                 VkDeviceSize in_size,
                 void**       out_opt_data_ptr = nullptr);

        /** Reads data from the specified region of the underlying memory object after mapping
         *  it into process space and copies it to the user-specified location.
         *  If the buffer object uses non-coherent memory backing, the region will first be
         *  invalidated to ensure the reads return valid data.
         *
         *  This function does not require the caller to issue a map() call, prior to being called.
         *  However, making that call in advance will skip map()+unmap() invocations, wnich would
         *  otherwise have to be done for each read() call.
         *
         *  Note that reading from multi_instance memory heaps is not permitted by VK_KHR_device_group.
         *  Any attempt to do so will result in an assertion failure and an error being reported by
         *  this function.
         *
         *  Since this function is device-agnostic, it doesn't matter if the parent device is a single-
         *  or a multi-GPU instance.
         *
         *  @param in_start_offset Start offset of the region to be mapped.
         *  @param in_size         Size of the region to be mapped.
         *  @param out_result_ptr  The read data will be copied to the location specified by this
         *                         argument. Must not be nullptr.
         *
         *  @return true if the call was successful, false otherwise.
         **/
        bool read(VkDeviceSize in_start_offset,
                  VkDeviceSize in_size,
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
         *  Note that writing to multi_instance memory heaps is not permitted by VK_KHR_device_group.
         *  Any attempt to do so will result in an assertion failure and an error being reported by
         *  this function.
         *
         *  Since this function is device-agnostic, it doesn't matter if the parent device is a single-
         *  or a multi-GPU instance.
         *
         *  @param in_start_offset Start offset of the region to modify
         *  @param in_size         Size of the region to be modified.
         *  @param in_data         Data to be copied to the specified GPU memory region.
         *
         *  @return true if the call was successful, false otherwise.
         **/
        bool write(VkDeviceSize in_start_offset,
                   VkDeviceSize in_size,
                   const void*  in_data);

    private:
        /* Private functions */

        /* TODO
         *
         * @param out_opt_result_ptr If not null, deref will be set to the error code reported by the API function
         *                           used to allocate the memory.
         *
         * @return True if successful, false otherwise.
         */
        bool init(VkResult* out_opt_result_ptr = nullptr);

        MemoryBlock(Anvil::MemoryBlockCreateInfoUniquePtr in_create_info_ptr);

        MemoryBlock           (const MemoryBlock&);
        MemoryBlock& operator=(const MemoryBlock&);

        void     close_gpu_memory_access     ();
        uint32_t get_device_memory_type_index(uint32_t                  in_memory_type_bits,
                                              Anvil::MemoryFeatureFlags in_memory_features);
        bool     open_gpu_memory_access      ();

        /* IMemoryBlockBackendSupport */
        void set_parent_memory_allocator_backend_ptr(std::shared_ptr<Anvil::IMemoryAllocatorBackendBase> in_backend_ptr,
                                                     void*                                               in_backend_object)
        {
            anvil_assert(m_owned_parent_memory_allocator_backend_ptr == nullptr);

            m_backend_object                            = in_backend_object;
            m_owned_parent_memory_allocator_backend_ptr = in_backend_ptr;
            m_parent_memory_allocator_backend_ptr       = m_owned_parent_memory_allocator_backend_ptr.get();

            {
                auto parent_memory_block_ptr = m_create_info_ptr->get_parent_memory_block();

                while (parent_memory_block_ptr != nullptr)
                {
                    anvil_assert(parent_memory_block_ptr->m_parent_memory_allocator_backend_ptr == nullptr              ||
                                 parent_memory_block_ptr->m_parent_memory_allocator_backend_ptr == in_backend_ptr.get() );

                    parent_memory_block_ptr->m_backend_object                      = in_backend_object;
                    parent_memory_block_ptr->m_parent_memory_allocator_backend_ptr = in_backend_ptr.get();

                    parent_memory_block_ptr = parent_memory_block_ptr->get_create_info_ptr()->get_parent_memory_block();
                }
            }
        }

        /* Private members */
        std::atomic<uint32_t> m_gpu_data_map_count; /* Only set for root memory blocks */
        void*                 m_gpu_data_ptr;       /* Only set for root memory blocks */

        void*                                 m_backend_object;
        Anvil::MemoryBlockCreateInfoUniquePtr m_create_info_ptr;
        VkDeviceMemory                        m_memory;
        const Anvil::MemoryType*              m_memory_type_props_ptr; /* keep for simplified debugging */
        VkDeviceSize                          m_start_offset;

        std::vector<const Anvil::PhysicalDevice*>           m_mgpu_physical_devices;
        std::shared_ptr<Anvil::IMemoryAllocatorBackendBase> m_owned_parent_memory_allocator_backend_ptr;
        Anvil::IMemoryAllocatorBackendBase*                 m_parent_memory_allocator_backend_ptr;

        std::map<Anvil::ExternalMemoryHandleTypeFlagBits, Anvil::ExternalHandleUniquePtr> m_external_handle_type_to_external_handle;
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_MEMORY_BLOCK_H */
