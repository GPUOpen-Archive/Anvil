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

#include "misc/types.h"


namespace Anvil
{
    /** "About to be deleted" call-back function prototype. */
    typedef void (*PFNMEMORYBLOCKDESTRUCTIONCALLBACKPROC)(Anvil::MemoryBlock* in_memory_block_ptr,
                                                          void*               in_user_arg);

    /** Wrapper class for memory objects. Please see header for more details */
    class MemoryBlock : public std::enable_shared_from_this<MemoryBlock>
    {
    public:
        /* Public functions */

        /** Create and bind a new device memory object to the instantiated MemoryBlock object.
         *
         *  @param in_device_ptr          Device to use.
         *  @param in_allowed_memory_bits Memory type bits which meet the allocation requirements.
         *  @param in_size                Required allocation size.
         *  @param in_memory_features     Required memory features.
         **/
         static std::shared_ptr<MemoryBlock> create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                    uint32_t                         in_allowed_memory_bits,
                                                    VkDeviceSize                     in_size,
                                                    Anvil::MemoryFeatureFlags        in_memory_features);

        /** Create a memory block whose storage space is maintained by another MemoryBlock instance.
         *
         *  @param in_parent_memory_block_ptr MemoryBlock instance to use as a parent. Must not be nullptr.
         *                                    Parent memory block must not be mapped.
         *  @param in_start_offset            Start offset of the storage maintained by the specified parent memory block,
         *                                    from which the new MemoryBlock instance's storage should start.
         *                                    Must not be equal to or larger than parent object's storage size.
         *                                    When added to @param in_size, the result value must not be be larger than the remaining
         *                                    storage size.
         *  @param in_size                    Region size to use for the derived memory block.
         **/
        static std::shared_ptr<MemoryBlock> create_derived(std::shared_ptr<MemoryBlock> in_parent_memory_block_ptr,
                                                           VkDeviceSize                 in_start_offset,
                                                           VkDeviceSize                 in_size);

        /** Create a memory block whose lifetime is maintained by a separate entity. While the memory block remains reference-counted
         *  as usual, the destruction process is carried out by an external party via the specified call-back.
         *
         *  This implements a special case required for support of Vulkan Memory Allocator memory allocator backend. Applications
         *  are very unlikely to ever need to use this create() function.
         *
         *  TODO
         */
        static std::shared_ptr<MemoryBlock> create_derived_with_custom_delete_proc(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                                                                                   VkDeviceMemory                        in_memory,
                                                                                   uint32_t                              in_allowed_memory_bits,
                                                                                   Anvil::MemoryFeatureFlags             in_memory_features,
                                                                                   uint32_t                              in_memory_type_index,
                                                                                   VkDeviceSize                          in_size,
                                                                                   VkDeviceSize                          in_start_offset,
                                                                                   PFNMEMORYBLOCKDESTRUCTIONCALLBACKPROC in_pfn_destroy_memory_block_proc,
                                                                                   void*                                 in_destroy_memory_block_proc_user_arg);

        /** Releases the Vulkan counterpart and unregisters the wrapper instance from the object tracker */
        virtual ~MemoryBlock();

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

        /** Returns memory features of the underlying memory region */
        Anvil::MemoryFeatureFlags get_memory_features() const
        {
            if (m_parent_memory_block_ptr != nullptr)
            {
                return m_parent_memory_block_ptr->get_memory_features();
            }
            else
            {
                return m_memory_features;
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

        /* Returns parent memory block, if one has been defined for this instance */
        std::shared_ptr<Anvil::MemoryBlock> get_parent_memory_block() const
        {
            return m_parent_memory_block_ptr;
        }

        /* Returns the size of the memory block. */
        VkDeviceSize get_size() const
        {
            return m_size;
        }

        /* Returns the start offset of the memory block.
         *
         * If the memory block has a parent, the returned start offset is NOT relative to the parent memory block's
         * start offset (in other words: the returned value is an absolute offset which can be directly used against
         * the memory block instance)
         */
        VkDeviceSize get_start_offset() const
        {
            return m_start_offset;
        }

        /** Checks if the memory range covered by this memory block intersects with memory range covered
         *  by the user-specified memory block
         *
         *  @param in_memory_block_ptr
         *
         *  @return true if intersection has been detected, false otherwise. */
        bool intersects(std::shared_ptr<const Anvil::MemoryBlock> in_memory_block_ptr) const;

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
        bool init();

        /** Please see create() for documentation */
        MemoryBlock(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                    uint32_t                              in_allowed_memory_bits,
                    VkDeviceSize                          in_size,
                    Anvil::MemoryFeatureFlags             in_memory_features);

        /** Please see create() for documentation */
        MemoryBlock(std::shared_ptr<MemoryBlock> in_parent_memory_block_ptr,
                    VkDeviceSize                 in_start_offset,
                    VkDeviceSize                 in_size);

        /** Please see create_derived_with_custom_delete_proc() for documentation */
        MemoryBlock(std::weak_ptr<Anvil::BaseDevice>      in_device_ptr,
                    VkDeviceMemory                        in_memory,
                    uint32_t                              in_allowed_memory_bits,
                    Anvil::MemoryFeatureFlags             in_memory_features,
                    uint32_t                              in_memory_type_index,
                    VkDeviceSize                          in_size,
                    VkDeviceSize                          in_start_offset,
                    PFNMEMORYBLOCKDESTRUCTIONCALLBACKPROC in_pfn_destroy_memory_block_proc,
                    void*                                 in_destroy_memory_block_proc_user_arg);

        MemoryBlock           (const MemoryBlock&);
        MemoryBlock& operator=(const MemoryBlock&);

        void     close_gpu_memory_access     ();
        uint32_t get_device_memory_type_index(uint32_t                  in_memory_type_bits,
                                              Anvil::MemoryFeatureFlags in_memory_features);
        bool     open_gpu_memory_access      (VkDeviceSize              in_start_offset,
                                              VkDeviceSize              in_size);

        /* Private members */
        void*                                 m_destroy_memory_block_proc_user_arg;
        PFNMEMORYBLOCKDESTRUCTIONCALLBACKPROC m_pfn_destroy_memory_block_proc;

        void*        m_gpu_data_ptr;
        bool         m_gpu_data_user_mapped;
        VkDeviceSize m_gpu_data_user_size;
        VkDeviceSize m_gpu_data_user_start_offset;

        uint32_t                            m_allowed_memory_bits;
        std::weak_ptr<Anvil::BaseDevice>    m_device_ptr;
        VkDeviceMemory                      m_memory;
        Anvil::MemoryFeatureFlags           m_memory_features;
        uint32_t                            m_memory_type_index;
        std::shared_ptr<Anvil::MemoryBlock> m_parent_memory_block_ptr;
        VkDeviceSize                        m_size;
        VkDeviceSize                        m_start_offset;
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_MEMORY_BLOCK_H */
