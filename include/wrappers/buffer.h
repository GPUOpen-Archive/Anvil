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
#include "misc/types.h"
#include "misc/page_tracker.h"

namespace Anvil
{
    typedef struct BufferCallbackIsAllocPendingQueryData
    {
        explicit BufferCallbackIsAllocPendingQueryData(std::shared_ptr<Anvil::Buffer> in_buffer_ptr)
            :buffer_ptr(in_buffer_ptr),
             result    (false)
        {
            /* Stub */
        }

        BufferCallbackIsAllocPendingQueryData& operator=(const BufferCallbackIsAllocPendingQueryData&) = delete;

        const std::shared_ptr<const Anvil::Buffer> buffer_ptr;
        bool                                       result;
    } BufferCallbackIsAllocPendingQueryData;

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
         * callback_arg: BufferCallbackIsAllocPendingQueryData*
         **/
        BUFFER_CALLBACK_ID_IS_ALLOC_PENDING,

        /* Call-back issued when no memory block is assigned to the buffer wrapper instance and
         * someone has just requested it.
         *
         * This call-back is needed by memory allocator in order to support implicit bake operations.
         *
         * callback_arg: Calling back buffer instance;
         **/
        BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED,

        /* Always last */
        BUFFER_CALLBACK_ID_COUNT
    };

    class Buffer : public std::enable_shared_from_this<Buffer>,
                   public CallbacksSupportProvider,
                   public DebugMarkerSupportProvider<Buffer>
    {
    public:
        /* Public functions */

        /** Initializes a new NON-SPARSE buffer object using user-specified parameters.
         *
         *  Does NOT allocate and bind any memory blocks to the object. It is user's responsibility
         *  to call Buffer::set_memory() to configure the binding.
         *
         *  @param in_device_ptr         Device to use.
         *  @param in_size               Size of the buffer object to be initialized.
         *  @param in_queue_families     Queue families which the buffer object is going to be used with.
         *                               One or more user queue family bits can be enabled.
         *  @param in_queue_sharing_mode VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                               call.
         *  @param in_usage_flags        Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                               to the vkCreateBuffer() call.
         **/
        static std::shared_ptr<Anvil::Buffer> create_nonsparse(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                               VkDeviceSize                     in_size,
                                                               QueueFamilyBits                  in_queue_families,
                                                               VkSharingMode                    in_queue_sharing_mode,
                                                               VkBufferUsageFlags               in_usage_flags);

        /** Initializes a new NON-SPARSE buffer object using user-specified parameters.
         *
         *  This version of create() ALLOCATES and BINDS a unique memory block to the object. Do NOT
         *  call Buffer::set_memory() to configure the binding.
         *
         *  The constructor can optionally upload data to the initialized memory.
         *
         *  @param in_device_ptr               Device to use.
         *  @param in_size                     Size of the buffer object to be initialized.
         *  @param in_queue_families           Queue families which the buffer object is going to be used with.
         *                                     One or more user queue family bits can be enabled.
         *  @param in_queue_sharing_mode       VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                                     call.
         *  @param in_usage_flags              Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                                     to the vkCreateBuffer() call.
         *  @param in_memory_freatures         Required memory features.
         *  @param in_opt_client_data          if not nullptr, exactly @param in_size bytes will be copied to the allocated
         *                                     buffer memory.
         **/
        static std::shared_ptr<Anvil::Buffer> create_nonsparse(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                               VkDeviceSize                     in_size,
                                                               QueueFamilyBits                  in_queue_families,
                                                               VkSharingMode                    in_queue_sharing_mode,
                                                               VkBufferUsageFlags               in_usage_flags,
                                                               Anvil::MemoryFeatureFlags        in_memory_features,
                                                               const void*                      in_opt_client_data);

        /** Creates a new Buffer wrapper instance. The new NON-SPARSE buffer will reuse a region of the specified
         *  buffer's storage, instead of creating one's own.
         *
         *  It is user's responsibility to ensure memory aliasing or synchronization is used, according
         *  to the spec rules.
         *
         *  @param in_parent_buffer_ptr Specifies the buffer, whose memory block should be used. Must not be
         *                              nullptr. MUST BE NON-SPARSE.
         *  @param in_start_offset      Memory region's start offset.
         *  @param in_size              Size of the memory region to "claim".
         **/
        static std::shared_ptr<Anvil::Buffer> create_nonsparse(std::shared_ptr<Anvil::Buffer> in_parent_nonsparse_buffer_ptr,
                                                               VkDeviceSize                   in_start_offset,
                                                               VkDeviceSize                   in_size);

        /** Initializes a new SPARSE buffer object using user-specified parameters.
         *
         *  Does NOT bind any memory regions to the object. It is user's responsibility to call
         *  Queue::bind_sparse_memory() afterward to update page configuration.
         *
         *  @param in_device_ptr         Device to use.
         *  @param in_size               Size of the buffer object to be initialized.
         *  @param in_queue_families     Queue families which the buffer object is going to be used with.
         *                               One or more user queue family bits can be enabled.
         *  @param in_queue_sharing_mode VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                               call.
         *  @param in_usage_flags        Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                               to the vkCreateBuffer() call.
         *  @param in_residency_scope    Scope of residency to support for the buffer.
         **/
        static std::shared_ptr<Anvil::Buffer> create_sparse(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                            VkDeviceSize                     in_size,
                                                            QueueFamilyBits                  in_queue_families,
                                                            VkSharingMode                    in_queue_sharing_mode,
                                                            VkBufferUsageFlags               in_usage_flags,
                                                            Anvil::SparseResidencyScope      in_residency_scope);

        /** Destroys the Vulkan objects and unregister the Buffer instance from Object Tracker. */
        virtual ~Buffer();

        /** Returns the lowest-level Buffer instance which stores the data exposed by this Buffer instance. */
        std::shared_ptr<Anvil::Buffer> get_base_buffer();

        /** Returns the encapsulated raw Vulkan buffer handle
         *
         *  For non-sparse buffers, in case no memory block has been assigned to the buffer,
         *  the function will issue a BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED call-back, so that
         *  any memory allocator, which has this buffer scheduled for deferred memory allocation,
         *  gets a chance to allocate & bind a memory block to the instance. A non-sparse buffer instance
         *  without any memory block bound msut not be used for any GPU-side operation.
         */
        VkBuffer get_buffer();

        /** Returns a pointer to the encapsulated raw Vulkan buffer handle */
        const VkBuffer* get_buffer_ptr() const
        {
            return &m_buffer;
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
        std::shared_ptr<Anvil::MemoryBlock> get_memory_block(uint32_t in_n_memory_block);

        /** Returns memory requirements for the buffer */
        VkMemoryRequirements get_memory_requirements() const
        {
            if (m_parent_buffer_ptr != nullptr)
            {
                return m_parent_buffer_ptr->get_memory_requirements();
            }
            else
            {
                return m_buffer_memory_reqs;
            }
        }

        /** Returns the number of memory blocks assigned to the buffer. */
        uint32_t get_n_memory_blocks() const
        {
            if (m_is_sparse)
            {
                return m_page_tracker_ptr->get_n_memory_blocks();
            }
            else
            {
                return 1;
            }
        }

        /** Returns a pointer to the parent buffer, if one was specified at creation time */
        std::shared_ptr<Anvil::Buffer> get_parent_buffer_ptr() const
        {
            return m_parent_buffer_ptr;
        }

        /** Returns info about queue families this buffer has been created for */
        QueueFamilyBits get_queue_families() const
        {
            return m_queue_families;
        }

        /** Returns the residency scope.
         *
         *  Triggers an assertion failure if called for non-sparse images
         */
        Anvil::SparseResidencyScope get_residency_scope() const
        {
            anvil_assert(m_is_sparse);

            return m_residency_scope;
        }

        /** Returns sharing mode of the buffer */
        VkSharingMode get_sharing_mode() const
        {
            return m_sharing_mode;
        }

        /** Returns size of the encapsulated Vulkan buffer memory region.
         *
         *  @return >= 0 if successful, UINT64_MAX otherwise */
        VkDeviceSize get_size() const;

        /** Returns start offset of the encapsulated Vulkan buffer memory region.
         *
         *  @return >= 0 if successful, UINT64_MAX otherwise */
        VkDeviceSize get_start_offset() const;

        /** Tells whether or not the buffer is sparse */
        bool is_sparse() const
        {
            return m_is_sparse;
        }

        /** Returns usage defined for the buffer */
        VkBufferUsageFlags get_usage() const
        {
            return m_usage_flags;
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
         *  This function must not be used to read data from buffers, whose memory backing comes from a multi-instance heap.
         *
         *  This function blocks until the transfer completes.
         *
         *  @param in_start_offset   As per description. Must be smaller than the underlying memory object's size.
         *  @param in_size           As per description. @param in_start_offset + @param in_size must be lower than or
         *                           equal to the underlying memory object's size.
         *  @param in_out_result_ptr Retrieved data will be stored under this location. Must not be nullptr.
         *
         *  @return true if the operation was successful, false otherwise.
         **/
        bool read(VkDeviceSize in_start_offset,
                  VkDeviceSize in_size,
                  void*        out_result_ptr);

        /** Attaches a memory block to the buffer object.
         *
         *  This function can only be called ONCE, after the object has been created with the constructor which
         *  does not allocate the memory automatically.
         *
         *  This function can only be used for NON-SPARSE buffers. Calling this function for sparse buffers will
         *  result in an assertion failure.
         *
         *  @param in_memory_block_ptr Memory block to attach to the buffer object. Must not be NULL.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_nonsparse_memory(std::shared_ptr<MemoryBlock> in_memory_block_ptr);

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
         *  @param in_opt_queue_ptr  See documentation above. May be nullptr.

         *  @return true if the operation was successful, false otherwise.
         **/
        bool write(VkDeviceSize                  in_start_offset,
                   VkDeviceSize                  in_size,
                   const void*                   in_data,
                   std::shared_ptr<Anvil::Queue> in_opt_queue_ptr = nullptr);

    private:
        /* Private functions */
        explicit Buffer(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                        VkDeviceSize                     in_size,
                        QueueFamilyBits                  in_queue_families,
                        VkSharingMode                    in_queue_sharing_mode,
                        VkBufferUsageFlags               in_usage_flags,
                        Anvil::SparseResidencyScope      in_residency_scope);
        explicit Buffer(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                        VkDeviceSize                     in_size,
                        QueueFamilyBits                  in_queue_families,
                        VkSharingMode                    in_queue_sharing_mode,
                        VkBufferUsageFlags               in_usage_flags,
                        MemoryFeatureFlags               in_memory_features);
        explicit Buffer(std::shared_ptr<Anvil::Buffer>   in_parent_buffer_ptr,
                        VkDeviceSize                     in_start_offset,
                        VkDeviceSize                     in_size);

        Buffer           (const Buffer&);
        Buffer& operator=(const Buffer&);

        void create_buffer    (Anvil::QueueFamilyBits       in_queue_families,
                               VkSharingMode                in_sharing_mode,
                               VkDeviceSize                 in_size);
        bool set_memory_sparse(std::shared_ptr<MemoryBlock> in_memory_block_ptr,
                               VkDeviceSize                 in_memory_start_offset,
                               VkDeviceSize                 in_start_offset,
                               VkDeviceSize                 in_size);

        /* Private members */
        VkBuffer                            m_buffer;
        VkMemoryRequirements                m_buffer_memory_reqs;
        VkDeviceSize                        m_buffer_size;
        std::weak_ptr<Anvil::BaseDevice>    m_device_ptr;
        bool                                m_is_sparse;
        std::shared_ptr<Anvil::MemoryBlock> m_memory_block_ptr; // only used by non-sparse buffers
        std::unique_ptr<Anvil::PageTracker> m_page_tracker_ptr; // only used by sparse buffers
        std::shared_ptr<Anvil::Buffer>      m_parent_buffer_ptr;
        Anvil::QueueFamilyBits              m_queue_families;
        Anvil::SparseResidencyScope         m_residency_scope;
        VkSharingMode                       m_sharing_mode;
        VkDeviceSize                        m_start_offset;

        VkBufferCreateFlagsVariable(m_create_flags);
        VkBufferUsageFlagsVariable (m_usage_flags);

        friend class Anvil::Queue; /* set_sparse_memory() */
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_BUFFER_H */