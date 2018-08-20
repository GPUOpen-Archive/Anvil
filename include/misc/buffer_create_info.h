//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef MISC_BUFFER_CREATE_INFO_H
#define MISC_BUFFER_CREATE_INFO_H

#include "misc/types.h"


namespace Anvil
{
    class BufferCreateInfo
    {
    public:
        /* Public functions */

        /** Creates a create info for a NON-SPARSE buffer object.
         *
         *  A buffer instance created using the returned info instance WILL ALLOCATE and have a unique memory block BOUND to the object.
         *  Do NOT call Buffer::set_memory() to configure the binding.
         *
         *  The constructor can optionally upload data to the initialized memory.
         *
         *  The following default values are assumed, unless specified with separate set_..() invocations issued against the result instance:
         *
         *  - Client data to fill the result bullfer with after mem alloc is bound to the buffer: none
         *  - External memory handle types:                                                       none
         *  - MT safety:                                                                          MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         *
         *  @param in_device_ptr       Device to use.
         *  @param in_size             Size of the buffer object to be initialized.
         *  @param in_queue_families   Queue families which the buffer object is going to be used with.
         *                             One or more user queue family bits can be enabled.
         *  @param in_sharing_mode     VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                             call.
         *  @param in_usage_flags      Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                             to the vkCreateBuffer() call.
         *  @param in_memory_freatures Required memory features.
         **/
        static Anvil::BufferCreateInfoUniquePtr create_nonsparse_alloc(const Anvil::BaseDevice*  in_device_ptr,
                                                                       VkDeviceSize              in_size,
                                                                       QueueFamilyBits           in_queue_families,
                                                                       VkSharingMode             in_sharing_mode,
                                                                       VkBufferUsageFlags        in_usage_flags,
                                                                       Anvil::MemoryFeatureFlags in_memory_features);

        /** Creates a create info for a NON-SPARSE buffer object.
         *
         *  A buffer instance created using the returned info instance will NOT allocate or have any memory blocks bound
         *  to itself. It is user's responsibility to call Buffer::set_memory() to configure the binding.
         *
         *  The following default values are assumed, unless specified with separate set_..() invocations issued against the result instance:
         *
         *  - External memory handle types: none
         *  - MT safety:                    MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         *
         *  @param in_device_ptr     Device to use.
         *  @param in_size           Size of the buffer object to be initialized.
         *  @param in_queue_families Queue families which the buffer object is going to be used with.
         *                           One or more user queue family bits can be enabled.
         *  @param in_sharing_mode   VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                           call.
         *  @param in_usage_flags    Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                           to the vkCreateBuffer() call.
         **/
        static Anvil::BufferCreateInfoUniquePtr create_nonsparse_no_alloc(const Anvil::BaseDevice* in_device_ptr,
                                                                          VkDeviceSize             in_size,
                                                                          QueueFamilyBits          in_queue_families,
                                                                          VkSharingMode            in_sharing_mode,
                                                                          VkBufferUsageFlags       in_usage_flags);

        /** Creates a create info for a NON-SPARSE buffer object.
         *
         *  The new NON-SPARSE buffer will reuse a region of the specified buffer's storage, instead of creating one's own.
         *
         *  It is user's responsibility to ensure memory aliasing or synchronization is used, according
         *  to the spec rules.
         *
         *  @param in_parent_buffer_ptr Specifies the buffer, whose memory block should be used. Must not be
         *                              nullptr. MUST BE NON-SPARSE.
         *  @param in_start_offset      Memory region's start offset.
         *  @param in_size              Size of the memory region to "claim".
         **/
        static Anvil::BufferCreateInfoUniquePtr create_nonsparse_no_alloc_child(Anvil::Buffer* in_parent_nonsparse_buffer_ptr,
                                                                                VkDeviceSize   in_start_offset,
                                                                                VkDeviceSize   in_size);

        /** Creates a create info for a SPARSE buffer object.
         *
         *  A buffer instance created using the returned info instance will NOT allocate or have any memory blocks bound
         *  to itself. It is user's responsibility to call Queue::bind_sparse_memory() afterward to update page configuration.
         *
         *  @param in_device_ptr                   Device to use.
         *  @param in_size                         Size of the buffer object to be initialized.
         *  @param in_queue_families               Queue families which the buffer object is going to be used with.
         *                                         One or more user queue family bits can be enabled.
         *  @param in_sharing_mode                 VkSharingMode value, which is going to be passed to the vkCreateBuffer()
         *                                         call.
         *  @param in_usage_flags                  Usage flags to set in the VkBufferCreateInfo descriptor, passed to
         *                                         to the vkCreateBuffer() call.
         *  @param in_residency_scope              Scope of residency to support for the buffer.
         *  @param in_external_memory_handle_types If the buffer is going to be exported to another process or Vulkan instance, use
         *                                         this field to specify which handle types the allocation needs to support. Please see
         *                                         documentation of Anvil::ExternalMemoryHandleTypeBits for more details.
         **/
        static Anvil::BufferCreateInfoUniquePtr create_sparse_no_alloc(const Anvil::BaseDevice*            in_device_ptr,
                                                                       VkDeviceSize                        in_size,
                                                                       QueueFamilyBits                     in_queue_families,
                                                                       VkSharingMode                       in_sharing_mode,
                                                                       VkBufferUsageFlags                  in_usage_flags,
                                                                       Anvil::SparseResidencyScope         in_residency_scope,
                                                                       MTSafety                            in_mt_safety                    = MT_SAFETY_INHERIT_FROM_PARENT_DEVICE,
                                                                       Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types = 0);

        const void* const get_client_data() const
        {
            return m_client_data_ptr;
        }

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        const Anvil::ExternalMemoryHandleTypeBits& get_exportable_external_memory_handle_types() const
        {
            return m_exportable_external_memory_handle_types;
        }

        const Anvil::MemoryFeatureFlags& get_memory_features() const
        {
            return m_memory_features;
        }

        const Anvil::MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }

        /** Returns a pointer to the parent buffer, if one was specified at creation time */
        Anvil::Buffer* get_parent_buffer_ptr() const
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
            anvil_assert(m_type == Anvil::BufferType::SPARSE_NO_ALLOC);

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
        VkDeviceSize get_size() const
        {
            anvil_assert(m_size != 0);

            return m_size;
        }

        /** Returns start offset of the encapsulated Vulkan buffer memory region.
         *
         *  @return >= 0 if successful, UINT64_MAX otherwise */
        VkDeviceSize get_start_offset() const
        {
            return m_start_offset;
        }

        const Anvil::BufferType& get_type() const
        {
            return m_type;
        }

        const VkBufferUsageFlags get_usage_flags() const
        {
            return m_usage_flags;
        }

        /** Use to specify contents which should be uploaded to a buffer at memory block assignment time.
         *
         *  Note that this setting will be ignored for partially-resident buffers.
         *
         *  The specified pointer must remain valid until Buffer::set_nonsparse_memory() call time.
         *
         *  @param in_client_data_ptr Pointer to data storage holding contents the created buffer should be filled with.
         *                            Must remain valid until memory block assignment time.
         */
        void set_client_data(const void* const in_client_data_ptr)
        {
            m_client_data_ptr = in_client_data_ptr;
        }

        void set_device(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        void set_exportable_external_memory_handle_types(const Anvil::ExternalMemoryHandleTypeBits& in_external_memory_handle_types)
        {
            m_exportable_external_memory_handle_types = in_external_memory_handle_types;
        }

        void set_memory_features(const Anvil::MemoryFeatureFlags& in_memory_features)
        {
            m_memory_features = in_memory_features;
        }

        void set_mt_safety(const Anvil::MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        void get_queue_families(const QueueFamilyBits& in_queue_families)
        {
            m_queue_families = in_queue_families;
        }

        void set_sharing_mode(const VkSharingMode& in_sharing_mode)
        {
            m_sharing_mode = in_sharing_mode;
        }

        void set_size(const VkDeviceSize& in_size)
        {
            m_size = in_size;
        }

        void set_start_offset(const VkDeviceSize& in_start_offset)
        {
            m_start_offset = in_start_offset;
        }

        void set_usage_flags(const VkBufferUsageFlags& in_usage_flags)
        {
            m_usage_flags = in_usage_flags;
        }

    private:
        /* Private type definitions */

        /* Private functions */

        BufferCreateInfo(const Anvil::BaseDevice*            in_device_ptr,
                         VkDeviceSize                        in_size,
                         QueueFamilyBits                     in_queue_families,
                         VkSharingMode                       in_sharing_mode,
                         VkBufferUsageFlags                  in_usage_flags,
                         Anvil::SparseResidencyScope         in_residency_scope,
                         MTSafety                            in_mt_safety,
                         Anvil::ExternalMemoryHandleTypeBits in_exportable_external_memory_handle_types);
        BufferCreateInfo(const Anvil::BufferType&            in_buffer_type,
                         const Anvil::BaseDevice*            in_device_ptr,
                         VkDeviceSize                        in_size,
                         QueueFamilyBits                     in_queue_families,
                         VkSharingMode                       in_sharing_mode,
                         VkBufferUsageFlags                  in_usage_flags,
                         MemoryFeatureFlags                  in_memory_features,
                         MTSafety                            in_mt_safety,
                         Anvil::ExternalMemoryHandleTypeBits in_exportable_external_memory_handle_types,
                         const void*                         in_opt_client_data_ptr);
        BufferCreateInfo(Anvil::Buffer*                      in_parent_buffer_ptr,
                         VkDeviceSize                        in_start_offset,
                         VkDeviceSize                        in_size);


        /* Private variables */
        const void*                         m_client_data_ptr;
        const Anvil::BaseDevice*            m_device_ptr;
        Anvil::ExternalMemoryHandleTypeBits m_exportable_external_memory_handle_types;
        Anvil::MemoryFeatureFlags           m_memory_features;
        MTSafety                            m_mt_safety;
        Anvil::Buffer* const                m_parent_buffer_ptr;
        Anvil::QueueFamilyBits              m_queue_families;
        const Anvil::SparseResidencyScope   m_residency_scope;
        VkSharingMode                       m_sharing_mode;
        VkDeviceSize                        m_size;
        VkDeviceSize                        m_start_offset;
        const BufferType                    m_type;
        VkBufferUsageFlags                  m_usage_flags;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(BufferCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(BufferCreateInfo);
    };
}; /* namespace Anvil */
#endif /* MISC_BUFFER_CREATE_INFO_H */