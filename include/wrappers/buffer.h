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

/** Defines a Buffer class, which is a wrapper for buffer object API in Vulkan.
 *
 *  Additionally, the class:
 *
 *  - provides a read() function which works for buffer objects with coherent & non-coherent
 *    memory backing.
 *  - provides a write() function which works just as read().
 *
 *  Buffer instances are reference-counted.
 **/
#ifndef WRAPPERS_BUFFER_H
#define WRAPPERS_BUFFER_H

#include "misc/callbacks.h"
#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"
#include "misc/page_tracker.h"

namespace Anvil
{
    /* Enumerates available buffer call-back types.*/
    enum BufferCallbackID
    {
        /* Call-back issued by sparse buffer instances whenever the buffer needs to check if
         * there are any pending alloc operations for this instance. Any recipient should set
         * callback_arg::result to true in case a bake operation *would* assign new pages to
         * the buffer instance. If no allocs are scheduled, the bool value MUST be left
         * untouched.
         *
         * This call-back is needed for memory allocator to support implicit bake operations
         * for sparse images.
         *
         * callback_arg: Pointer to IsBufferMemoryAllocPendingQueryCallbackArgument instance.
         **/
        BUFFER_CALLBACK_ID_IS_ALLOC_PENDING,

        /* Call-back issued when no memory block is assigned to the buffer wrapper instance and
         * someone has just requested it.
         *
         * This call-back is needed by memory allocator in order to support implicit bake operations.
         *
         * callback_arg: Pointer to OnMemoryBlockNeededForBufferCallbackArgument instance.
         **/
        BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED,

        /* Always last */
        BUFFER_CALLBACK_ID_COUNT
    };

    class Buffer : public CallbacksSupportProvider,
                   public DebugMarkerSupportProvider<Buffer>,
                   public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Destroys the Vulkan objects and unregister the Buffer instance from Object Tracker. */
        virtual ~Buffer();

        static Anvil::BufferUniquePtr create(Anvil::BufferCreateInfoUniquePtr in_create_info_ptr);

        /** Returns the lowest-level Buffer instance which stores the data exposed by this Buffer instance. */
        const Anvil::Buffer* get_base_buffer();

        /** Returns the encapsulated raw Vulkan buffer handle
         *
         *  For non-sparse buffers, in case no memory block has been assigned to the buffer,
         *  the function will issue a BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED call-back, so that
         *  any memory allocator, which has this buffer scheduled for deferred memory allocation,
         *  gets a chance to allocate & bind a memory block to the instance. A non-sparse buffer instance
         *  without any memory block bound msut not be used for any GPU-side operation.
         *
         *  This behavior may optionally be disabled by setting @param in_bake_memory_if_necessary to false.
         *  Should only be done in special circumstances.
         */
        VkBuffer get_buffer(const bool& in_bake_memory_if_necessary = true);

        /** Returns a pointer to the encapsulated raw Vulkan buffer handle */
        const VkBuffer* get_buffer_ptr() const
        {
            return &m_buffer;
        }

        const Anvil::BufferCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        /** Returns a pointer to the underlying memory block wrapper instance.
         *
         *  For non-sparse buffers, in case no memory block has been assigned to the buffer,
         *  the function will issue a BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED call-back, so that
         *  any memory allocator, which has this buffer scheduled for deferred memory allocation,
         *  gets a chance to allocate & bind a memory block to the instance.
         *
         *  Sparse buffers do not support implicit bake operations yet.
         *
         *  Note that resident sparse buffers may have multiple memory blocks assigned.
         **/
        Anvil::MemoryBlock* get_memory_block(uint32_t in_n_memory_block);

        /** Returns memory requirements for the buffer */
        VkMemoryRequirements get_memory_requirements() const;

        /** Returns the number of memory blocks assigned to the buffer. */
        uint32_t get_n_memory_blocks() const;

        const Anvil::PageTracker* get_page_tracker() const
        {
            return m_page_tracker_ptr.get();
        }

        bool prefers_dedicated_allocation() const
        {
            return m_prefers_dedicated_allocation;
        }

        /** Reads @param in_size bytes, starting from @param in_start_offset, from the wrapped memory object.
         *
         *  If the buffer object uses mappable storage memory, the affected region will be mapped into process space,
         *  read from, and then unmapped. If the memory region comes from a non-coherent memory heap, it will be
         *  invalidated before the CPU read operation.
         *
         *  If the buffer object uses non-mappable storage memory, a staging buffer using mappable memory will be created
         *  instead. User-specified region of the source buffer will then be copied into it by submitting a copy operation,
         *  executed either on the transfer queue (if available), or on the universal queue. Afterward, the staging buffer
         *  will be released.
         *
         *  The function prototype without @param in_device_mask argument should be used for single-GPU devices only.
         *  The function prototype with @param in_device_mask argument should be used for multi-GPU devices only.
         *  The mask must contain only one bit set.
         *
         *  This function must not be used to read data from buffers, whose memory backing comes from a multi-instance heap.
         *
         *  This function blocks until the transfer completes.
         *
         *  @param in_start_offset As per description. Must be smaller than the underlying memory object's size.
         *  @param in_size         As per description. @param in_start_offset + @param in_size must be lower than or
         *                         equal to the underlying memory object's size.
         *  @param out_result_ptr  Retrieved data will be stored under this location. Must not be nullptr.
         *
         *  @return true if the operation was successful, false otherwise.
         **/
        bool read(VkDeviceSize in_start_offset,
                  VkDeviceSize in_size,
                  void*        out_result_ptr);
        bool read(VkDeviceSize in_start_offset,
                  VkDeviceSize in_size,
                  uint32_t     in_device_mask,
                  void*        out_result_ptr);

        bool requires_dedicated_allocation() const
        {
            return m_requires_dedicated_allocation;
        }

        /** Attaches a memory block to the buffer object.
         *
         *  This function can only be called ONCE, after the object has been created with the constructor which
         *  does not allocate the memory automatically.
         *
         *  This function can only be used for NON-SPARSE buffers. Calling this function for sparse buffers will
         *  result in an assertion failure.
         *
         *  @param in_memory_block_ptr             Memory block to attach to the buffer object. Must not be NULL.
         *  @param in_memory_block_owned_by_buffer TODO
         *  @param in_n_device_group_indices       Describes the number of device indices available under @param in_device_group_indices_ptr.
         *  @param in_device_group_indices_ptr     Device group indices to use to form the device mask.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_nonsparse_memory(MemoryBlockUniquePtr in_memory_block_ptr);
        bool set_nonsparse_memory(MemoryBlock*         in_memory_block_ptr,
                                  bool                 in_memory_block_owned_by_buffer);

        bool set_nonsparse_memory(MemoryBlockUniquePtr in_memory_block_ptr,
                                  uint32_t             in_n_device_group_indices,
                                  const uint32_t*      in_device_group_indices_ptr);
        bool set_nonsparse_memory(MemoryBlock*         in_memory_block_ptr,
                                  bool                 in_memory_block_owned_by_buffer,
                                  uint32_t             in_n_device_group_indices,
                                  const uint32_t*      in_device_group_indices_ptr);

        /** See set_nonsparse_memory() for general documentation.
         *
         *  This static function can be used to set buffer memory bindings in a batched manner.
         *
         *  Can be used for both single- and multi-GPU devices.
         *  Requires VK_KHR_device_group to be supported by & enabled for the device.
         *
         *  TODO
         **/
        static bool set_nonsparse_memory_multi(uint32_t                   in_n_buffer_memory_binding_updates,
                                               BufferMemoryBindingUpdate* in_updates_ptr);

        /** Writes @param in_size bytes, starting from @param in_start_offset, into the wrapped memory object.
         *
         *  If the buffer object uses mappable storage memory, the affected region will be mapped into process space,
         *  updated, and then unmapped. If the memory region comes from a non-coherent memory heap, it will be
         *  flushed after the CPU write operation.
         *
         *  If the buffer object uses non-mappable storage memory, a staging buffer using mappable memory will be created
         *  instead. It will then be filled with user-specified data and used as a source for a copy operation which will
         *  transfer the new contents to the target buffer. The operation will be submitted via a transfer queue, if one
         *  is available, or a universal queue otherwise.
         *
         *  This function must not be used to read data from buffers, whose memory backing comes from a multi-instance heap.
         *
         *  The function prototype without @param in_device_mask argument should be used for single-GPU devices only.
         *
         *  The function prototype with @param in_device_mask argument should be used for multi-GPU devices only.
         *  The mask must contain only one bit set.
         *
         *  If the buffer instance uses an exclusive sharing mode and supports more than just one queue family type AND memory
         *  backing the buffer is not mappable, you MUST specify a queue instance that should be used to perform a buffer->buffer
         *  copy op. The queue MUST support transfer ops.
         *
         *  This function blocks until the transfer completes.
         *
         *  @param in_start_offset   As per description. Must be smaller than the underlying memory object's size.
         *  @param in_size           As per description. @param in_start_offset + @param in_size must be lower than or
         *                           equal to the underlying memory object's size.
         *  @param in_data           Data to store. Must not be nullptr.
         *
         *  @return true if the operation was successful, false otherwise.
         **/
        bool write(VkDeviceSize                         in_start_offset,
                   VkDeviceSize                         in_size,
                   const void*                          in_data,
                   Anvil::Queue*                        in_opt_queue_ptr = nullptr);
        bool write(VkDeviceSize                         in_start_offset,
                   VkDeviceSize                         in_size,
                   const void*                          in_data,
                   uint32_t                             in_device_mask,
                   Anvil::Queue*                        in_opt_queue_ptr = nullptr);

    private:
        /* Private functions */

        Buffer(Anvil::BufferCreateInfoUniquePtr in_create_info_ptr);

        bool init               ();
        bool init_staging_buffer(const VkDeviceSize& in_size,
                                 Anvil::Queue*       in_opt_queue_ptr);
        bool set_memory_sparse  (MemoryBlock*        in_memory_block_ptr,
                                 bool                in_memory_block_owned_by_buffer,
                                 VkDeviceSize        in_memory_start_offset,
                                 VkDeviceSize        in_start_offset,
                                 VkDeviceSize        in_size);

        bool set_memory_nonsparse_internal(MemoryBlockUniquePtr in_memory_block_ptr,
                                           uint32_t             in_n_device_group_indices,
                                           const uint32_t*      in_device_group_indices_ptr);

        bool is_memory_block_owned(const MemoryBlock* in_memory_block_ptr) const;

        /* Private members */
        VkBuffer                                 m_buffer;
        VkMemoryRequirements                     m_buffer_memory_reqs;
        std::unique_ptr<Anvil::BufferCreateInfo> m_create_info_ptr;

        Anvil::MemoryBlock*                  m_memory_block_ptr; // only used by non-sparse buffers
        std::unique_ptr<Anvil::PageTracker>  m_page_tracker_ptr; // only used by sparse buffers
        Anvil::BufferUniquePtr               m_staging_buffer_ptr;
        Anvil::Queue*                        m_staging_buffer_queue_ptr;

        std::vector<MemoryBlockUniquePtr> m_owned_memory_blocks;
        bool                              m_prefers_dedicated_allocation;
        bool                              m_requires_dedicated_allocation;

        friend class Anvil::Queue; /* set_memory_sparse() */

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(Buffer);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(Buffer);
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_BUFFER_H */