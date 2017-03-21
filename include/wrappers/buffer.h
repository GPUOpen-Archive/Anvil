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

#include "misc/types.h"
#include "misc/page_tracker.h"

namespace Anvil
{
    class Buffer : public std::enable_shared_from_this<Buffer>
    {
    public:
        /* Public functions */

        /** Initializes a new NON-SPARSE buffer object using user-specified parameters.
         *
         *  Does NOT allocate and bind any memory blocks to the object. It is user's responsibility
         *  to call Buffer::set_memory() to configure the binding.
         *
         *  @param device_ptr         Device to use.
         *  @param size               Size of the buffer object to be initialized.
         *  @param queue_families     Queue families which the buffer object is going to be used with.
         *                            One or more user queue family bits can be enabled.
         *  @param queue_sharing_mode VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                            call.
         *  @param usage_flags        Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                            to the vkCreateBuffer() call.
         **/
        static std::shared_ptr<Anvil::Buffer> create_nonsparse(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                               VkDeviceSize                     size,
                                                               QueueFamilyBits                  queue_families,
                                                               VkSharingMode                    queue_sharing_mode,
                                                               VkBufferUsageFlags               usage_flags);

        /** Initializes a new NON-SPARSE buffer object using user-specified parameters.
         *
         *  This version of create() ALLOCATES and BINDS a unique memory block to the object. Do NOT
         *  call Buffer::set_memory() to configure the binding.
         *
         *  The constructor can optionally upload data to the initialized memory.
         *
         *  @param device_ptr               Device to use.
         *  @param size                     Size of the buffer object to be initialized.
         *  @param queue_families           Queue families which the buffer object is going to be used with.
         *                                  One or more user queue family bits can be enabled.
         *  @param queue_sharing_mode       VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                                  call.
         *  @param usage_flags              Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                                  to the vkCreateBuffer() call.
         *  @param should_be_mappable       true if the new buffer object should use a memory type which is
         *                                  host-visible; false otherwise. Note that passing non-null @param opt_client_data
         *                                  argument value forces this argument value to be true.
         *  @param should_be_coherent       true if the new buffer object should use a memory type which supports
         *                                  coherent memory access; false otherwise.
         *  @param opt_client_data          if not nullptr, exactly @param size bytes will be copied to the allocated
         *                                  buffer memory.
         **/
        static std::shared_ptr<Anvil::Buffer> create_nonsparse(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                               VkDeviceSize                     size,
                                                               QueueFamilyBits                  queue_families,
                                                               VkSharingMode                    queue_sharing_mode,
                                                               VkBufferUsageFlags               usage_flags,
                                                               bool                             should_be_mappable,
                                                               bool                             should_be_coherent,
                                                               const void*                      opt_client_data);

        /** Creates a new Buffer wrapper instance. The new NON-SPARSE buffer will reuse a region of the specified
         *  buffer's storage, instead of creating one's own.
         *
         *  It is user's responsibility to ensure memory aliasing or synchronization is used, according
         *  to the spec rules.
         *
         *  @param parent_buffer_ptr Specifies the buffer, whose memory block should be used. Must not be
         *                           nullptr. MUST BE NON-SPARSE.
         *  @param start_offset      Memory region's start offset.
         *  @param size              Size of the memory region to "claim".
         **/
        static std::shared_ptr<Anvil::Buffer> create_nonsparse(std::shared_ptr<Anvil::Buffer> parent_nonsparse_buffer_ptr,
                                                               VkDeviceSize                   start_offset,
                                                               VkDeviceSize                   size);

        /** Initializes a new SPARSE buffer object using user-specified parameters.
         *
         *  Does NOT bind any memory regions to the object. It is user's responsibility to call
         *  Queue::bind_sparse_memory() afterward to update page configuration.
         *
         *  @param device_ptr         Device to use.
         *  @param size               Size of the buffer object to be initialized.
         *  @param queue_families     Queue families which the buffer object is going to be used with.
         *                            One or more user queue family bits can be enabled.
         *  @param queue_sharing_mode VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                            call.
         *  @param usage_flags        Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                            to the vkCreateBuffer() call.
         *  @param residency_scope    Scope of residency to support for the buffer.
         **/
        static std::shared_ptr<Anvil::Buffer> create_sparse(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                            VkDeviceSize                     size,
                                                            QueueFamilyBits                  queue_families,
                                                            VkSharingMode                    queue_sharing_mode,
                                                            VkBufferUsageFlags               usage_flags,
                                                            Anvil::SparseResidencyScope      residency_scope);

        /** Destroys the Vulkan objects and unregister the Buffer instance from Object Tracker. */
        virtual ~Buffer();

        /** Returns the lowest-level Buffer instance which stores the data exposed by this Buffer instance. */
        std::shared_ptr<Anvil::Buffer> get_base_buffer();

        /** Returns the encapsulated raw Vulkan buffer handle */
        VkBuffer get_buffer() const
        {
            return m_buffer;
        }

        /** Returns a pointer to the encapsulated raw Vulkan buffer handle */
        const VkBuffer* get_buffer_ptr() const
        {
            return &m_buffer;
        }

        /** Returns a pointer to the underlying memory block wrapper instance.
         *
         *  Note that resident sparse buffers may have multiple memory blocks assigned.
         **/
        std::shared_ptr<Anvil::MemoryBlock> get_memory_block(uint32_t n_memory_block) const
        {
            if (m_is_sparse)
            {
                return m_page_tracker_ptr->get_memory_block(n_memory_block);
            }
            else
            {
                return m_memory_block_ptr;
            }
        }

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

        /** Returns name assigned to the buffer instance */
        std::string get_name() const
        {
            return m_name;
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

        /** Reads @param size bytes, starting from @param start_offset, from the wrapped memory object.
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
         *  @param start_offset   As per description. Must be smaller than the underlying memory object's size.
         *  @param size           As per description. @param start_offset + @param size must be lower than or
         *                        equal to the underlying memory object's size.
         *  @param out_result_ptr Retrieved data will be stored under this location. Must not be nullptr.
         *
         *  @return true if the operation was successful, false otherwise.
         **/
        bool read(VkDeviceSize start_offset,
                  VkDeviceSize size,
                  void*        out_result_ptr);

        /** Attaches a memory block to the buffer object.
         *
         *  This function can only be called ONCE, after the object has been created with the constructor which
         *  does not allocate the memory automatically.
         *
         *  This function can only be used for NON-SPARSE buffers. Calling this function for sparse buffers will
         *  result in an assertion failure.
         *
         *  @param memory_block_ptr     Memory block to attach to the buffer object. Must not be NULL.
         *  @param n_physical_devices   Describes the number of physical devices available under @param opt_physical_devices_ptr.
         *  @param physical_devices_ptr Physical devices to use to form the device mask.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_nonsparse_memory(std::shared_ptr<MemoryBlock> memory_block_ptr);

        /** Assigns a new name to the buffer instance */
        void set_name(std::string new_name)
        {
            m_name = new_name;
        }

        /** Writes @param size bytes, starting from @param start_offset, into the wrapped memory object.
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
         *  This function blocks until the transfer completes.
         *
         *  @param start_offset   As per description. Must be smaller than the underlying memory object's size.
         *  @param size           As per description. @param start_offset + @param size must be lower than or
         *                        equal to the underlying memory object's size.
         *  @param data           Data to store. Must not be nullptr.
         *
         *  @return true if the operation was successful, false otherwise.
         **/
        bool write(VkDeviceSize start_offset,
                   VkDeviceSize size,
                   const void*  data);

    private:
        /* Private functions */
        explicit Buffer(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                        VkDeviceSize                     size,
                        QueueFamilyBits                  queue_families,
                        VkSharingMode                    queue_sharing_mode,
                        VkBufferUsageFlags               usage_flags,
                        Anvil::SparseResidencyScope      residency_scope);
        explicit Buffer(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                        VkDeviceSize                     size,
                        QueueFamilyBits                  queue_families,
                        VkSharingMode                    queue_sharing_mode,
                        VkBufferUsageFlags               usage_flags,
                        bool                             should_be_mappable,
                        bool                             should_be_coherent);
        explicit Buffer(std::shared_ptr<Anvil::Buffer>   parent_buffer_ptr,
                        VkDeviceSize                     start_offset,
                        VkDeviceSize                     size);

        Buffer           (const Buffer&);
        Buffer& operator=(const Buffer&);

        void create_buffer    (Anvil::QueueFamilyBits       queue_families,
                               VkSharingMode                sharing_mode,
                               VkDeviceSize                 size);
        bool set_memory_sparse(std::shared_ptr<MemoryBlock> memory_block_ptr,
                               VkDeviceSize                 memory_start_offset,
                               VkDeviceSize                 start_offset,
                               VkDeviceSize                 size);

        /* Private members */
        VkBuffer                            m_buffer;
        VkMemoryRequirements                m_buffer_memory_reqs;
        VkDeviceSize                        m_buffer_size;
        std::weak_ptr<Anvil::BaseDevice>    m_device_ptr;
        bool                                m_is_sparse;
        std::string                         m_name;
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