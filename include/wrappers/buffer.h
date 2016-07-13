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

#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
    class Buffer : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Initializes a new buffer object using user-specified parameters.
         *
         *  This constructor does NOT allocate and bind a memory block to the object. It is
         *  user's responsibility to call Buffer::set_memory() afterward to configure the binding.
         *
         *  @param device_ptr              Device to use.
         *  @param size                    Size of the buffer object to be initialized.
         *  @param queue_families          Queue families which the buffer object is going to be used with.
         *                                 One or more user queue family bits can be enabled.
         *  @param queue_sharing_mode      VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                                 call.
         *  @param usage_flags             Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                                 to the vkCreateBuffer() call.
         *  @param should_be_mappable      true if the new buffer object should use a memory type which is
         *                                 host-visible; false otherwise. Note that passing non-null @param opt_client_data
         *                                 argument value forces this argument value to be true.
         *  @param should_be_coherent      true if the new buffer object should use a memory type which supports
         *                                 coherent memory access; false otherwise.
         *  @param opt_client_data         if not nullptr, exactly @param size bytes will be copied to the allocated
         *                                 buffer memory.
         **/
        explicit Buffer(Anvil::Device*     device_ptr,
                        VkDeviceSize       size,
                        QueueFamilyBits    queue_families,
                        VkSharingMode      queue_sharing_mode,
                        VkBufferUsageFlags usage_flags);

        /** Initializes a new buffer object using user-specified parameters.
         *
         *  This constructor ALLOCATES and BINDS a unique memory block to the object. Do NOT
         *  call Buffer::set_memory() to configure the binding.
         *
         *  The constructor can optionally upload data to the initialized memory.
         *
         *  @param device_ptr              Device to use.
         *  @param size                    Size of the buffer object to be initialized.
         *  @param queue_families          Queue families which the buffer object is going to be used with.
         *                                 One or more user queue family bits can be enabled.
         *  @param queue_sharing_mode      VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                                 call.
         *  @param usage_flags             Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                                 to the vkCreateBuffer() call.
         *  @param should_be_mappable      true if the new buffer object should use a memory type which is
         *                                 host-visible; false otherwise. Note that passing non-null @param opt_client_data
         *                                 argument value forces this argument value to be true.
         *  @param should_be_coherent      true if the new buffer object should use a memory type which supports
         *                                 coherent memory access; false otherwise.
         *  @param opt_client_data         if not nullptr, exactly @param size bytes will be copied to the allocated
         *                                 buffer memory.
         **/
        explicit Buffer(Anvil::Device*     device_ptr,
                        VkDeviceSize       size,
                        QueueFamilyBits    queue_families,
                        VkSharingMode      queue_sharing_mode,
                        VkBufferUsageFlags usage_flags,
                        bool               should_be_mappable,
                        bool               should_be_coherent,
                        const void*        opt_client_data);

        /** Creates a new Buffer wrapper instance. The new instance will reuse a region of the specified
         *  buffer's storage, instead of creating one's own.
         *
         *  It is user's responsibility to ensure memory aliasing or synchronization is used, according
         *  to the spec rules.
         *
         *  @param parent_buffer_ptr Specifies the buffer, whose memory block should be used. Must not be
         *                           nullptr. The specified Buffer instance will be retained.
         *  @param start_offset      Memory region's start offset.
         *  @param size              Size of the memory region to "claim".
         **/
        explicit Buffer(Anvil::Buffer* parent_buffer_ptr,
                        VkDeviceSize   start_offset,
                        VkDeviceSize   size);

        /** Returns the lowest-level Buffer instance which stores the data exposed by this Buffer instance. */
        Anvil::Buffer* get_base_buffer();

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
         *  Under normal circumstances, you should never need to access it.
         **/
        Anvil::MemoryBlock* get_memory()
        {
            return m_memory_block_ptr;
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

        /** Returns a pointer to the parent buffer, if one was specified at creation time */
        Anvil::Buffer* get_parent_buffer_ptr()
        {
            return m_parent_buffer_ptr;
        }

        /** Returns size of the encapsulated Vulkan buffer memory region.
         *
         *  @return >= 0 if successful, -1 otherwise */
        VkDeviceSize get_size() const;

        /** Returns start offset of the encapsulated Vulkan buffer memory region.
         *
         *  @return >= 0 if successful, -1 otherwise */
        VkDeviceSize get_start_offset() const;

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
         *  @param memory_block_ptr Memory block to attach to the buffer object. Must not be NULL.
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_memory(MemoryBlock* memory_block_ptr);

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
        Buffer           (const Buffer&);
        Buffer& operator=(const Buffer&);

        virtual ~Buffer();

        void convert_queue_family_bits_to_family_indices(Anvil::QueueFamilyBits queue_families,
                                                         uint32_t*              out_opt_queue_family_indices_ptr,
                                                         uint32_t*              out_opt_n_queue_family_indices_ptr) const;
        void create_buffer                              (Anvil::QueueFamilyBits queue_families,
                                                         VkSharingMode          sharing_mode,
                                                         VkDeviceSize           size);

        /* Private members */
        VkBuffer              m_buffer;
        VkMemoryRequirements  m_buffer_memory_reqs;
        VkDeviceSize          m_buffer_size;
        Anvil::Device*        m_device_ptr;
        Anvil::MemoryBlock*   m_memory_block_ptr;
        Anvil::Buffer*        m_parent_buffer_ptr;
        VkDeviceSize          m_start_offset;
        VkBufferUsageFlagBits m_usage_flags;
    };

    /** Delete functor. Useful if you need to wrap the buffer instance in an auto pointer */
    struct BufferDeleter
    {
        void operator()(Buffer* buffer_ptr) const
        {
            buffer_ptr->release();
        }
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_BUFFER_H */