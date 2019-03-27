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
#ifndef TYPES_CLASSES_H
#define TYPES_CLASSES_H

#include "misc/types_struct.h"

namespace Anvil
{
    class IMemoryAllocatorBackendBase
    {
    public:
        virtual ~IMemoryAllocatorBackendBase()
        {
            /* Stub */
        }

        virtual VkResult map            (void*        in_memory_object,
                                         VkDeviceSize in_start_offset,
                                         VkDeviceSize in_memory_block_start_offset,
                                         VkDeviceSize in_size,
                                         void**       out_result_ptr)   = 0;
        virtual bool     supports_baking() const                        = 0;
        virtual void     unmap          (void*        in_memory_object) = 0;
    };

    /** Container for sparse memory binding updates */
    class SparseMemoryBindingUpdateInfo
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  Marks the container as dirty by default.
         */
        SparseMemoryBindingUpdateInfo();

        /** Adds a new bind info to the container. The application can then append buffer memory updates
         *  to the bind info by calling append_buffer_memory_update().
         *
         *  For mGPU devices, zeroth device index is used by default for both memory and resource device indices.
         *  You can adjust these default values by calling set_memory_device_index() and set_resource_device_index().
         *
         *  @param in_n_signal_semaphores            Number of semaphores to signal after the bind info is processed. Can be 0.
         *  @param in_opt_signal_semaphores_ptrs_ptr Ptr to an array of semaphores (sized @param in_n_signal_semaphores) to signal.
         *                                           Should be null if @param in_n_signal_semaphores is 0.
         *  @param in_n_wait_semaphores              Number of semaphores to wait on before the bind info should start being
         *                                           processed. Can be 0.
         *  @param in_opt_wait_semaphores_ptrs_ptr   Ptr to an array of semaphores (sized @param in_n_wait_semaphores) to wait on,
         *                                           before processing the bind info. Should be null if @param in_n_wait_semaphores
         *                                           is 0.
         *
         *  @return ID of the new bind info.
         **/
        SparseMemoryBindInfoID add_bind_info(uint32_t                 in_n_signal_semaphores,
                                             Anvil::Semaphore* const* in_opt_signal_semaphores_ptrs_ptr,
                                             uint32_t                 in_n_wait_semaphores,
                                             Anvil::Semaphore* const* in_opt_wait_semaphores_ptrs_ptr);

        /** Appends a new buffer memory block update to the bind info.
         *
         *  @param in_bind_info_id                     ID of the bind info to append the update to.
         *  @param in_buffer_ptr                       Buffer instance to update. Must not be NULL.
         *  @param in_buffer_memory_start_offset       Start offset of the target memory region.
         *  @param in_opt_memory_block_ptr             Memory block to use for the binding. May be NULL.
         *  @param in_opt_memory_block_start_offset    Start offset of the source memory region. Ignored
         *                                             if @param in_memory_block_ptr is NULL.
         *  @param in_opt_memory_block_owned_by_buffer TODO.
         *  @param in_size                             Size of the memory block to update.
         **/
        void append_buffer_memory_update(SparseMemoryBindInfoID in_bind_info_id,
                                         Anvil::Buffer*         in_buffer_ptr,
                                         VkDeviceSize           in_buffer_memory_start_offset,
                                         Anvil::MemoryBlock*    in_opt_memory_block_ptr,
                                         VkDeviceSize           in_opt_memory_block_start_offset,
                                         bool                   in_opt_memory_block_owned_by_buffer,
                                         VkDeviceSize           in_size);

        /** Appends a new non-opaque image memory update to the bind info.
         *
         *  @param in_bind_info_id                    ID of the bind info to append the update to.
         *  @param in_image_ptr                       Image instance to update. Must not be NULL.
         *  @param in_subresource                     Subresource which should be used for the update operation.
         *  @param in_offset                          Image region offset for the update operation.
         *  @param in_extent                          Extent of the update operation.
         *  @param in_flags                           Flags to use for the update.
         *  @param in_opt_memory_block_ptr            Memory block to use for the update operation. May be NULL.
         *  @param in_opt_memory_block_start_offset   Start offset of the source memory region. ignored if
         *                                            @param in_opt_memory_block_ptr is NULL.
         *  @param in_opt_memory_block_owned_by_image TODO
         **/
        void append_image_memory_update(SparseMemoryBindInfoID         in_bind_info_id,
                                        Anvil::Image*                  in_image_ptr,
                                        const Anvil::ImageSubresource& in_subresource,
                                        const VkOffset3D&              in_offset,
                                        const VkExtent3D&              in_extent,
                                        Anvil::SparseMemoryBindFlags   in_flags,
                                        Anvil::MemoryBlock*            in_opt_memory_block_ptr,
                                        VkDeviceSize                   in_opt_memory_block_start_offset,
                                        bool                           in_opt_memory_block_owned_by_image);

        /** Appends a new opaque image memory update to the bind info.
         *
         *  @param in_bind_info_id                    ID of the bind info to append the update to.
         *  @param in_image_ptr                       Image instance to update. Must not be NULL.
         *  @param in_resource_offset                 Raw memory image start offset to use for the update.
         *  @param in_size                            Number of bytes to update.
         *  @param in_flags                           Flags to use for the update.
         *  @param in_opt_memory_block_ptr            Memory block to use for the update operation. May be NULL.
         *  @param in_opt_memory_block_start_offset   Start offset of the source memory region. Ignored if
         *                                            @param in_opt_memory_block_ptr is NULL.
         *  @param in_opt_memory_block_owned_by_image TODO
         *  @param in_n_plane                         Index of the image plane to use for the binding. Must be 0 for joint YUV
         *                                            and non-YUV images. For disjoint YUV images, must be a value between 0 and 2
         *                                            inclusive.
         **/
        void append_opaque_image_memory_update(SparseMemoryBindInfoID       in_bind_info_id,
                                               Anvil::Image*                in_image_ptr,
                                               VkDeviceSize                 in_resource_offset,
                                               VkDeviceSize                 in_size,
                                               Anvil::SparseMemoryBindFlags in_flags,
                                               Anvil::MemoryBlock*          in_opt_memory_block_ptr,
                                               VkDeviceSize                 in_opt_memory_block_start_offset,
                                               bool                         in_opt_memory_block_owned_by_image,
                                               uint32_t                     in_n_plane);

        /** Retrieves bind info properties.
         *
         *  @param in_bind_info_id                           ID of the bind info to retrieve properties of.
         *  @param out_opt_n_buffer_memory_updates_ptr       Deref will be set to the number of buffer memory updates, assigned
         *                                                   to the specified bind info item. May be NULL.
         *  @param out_opt_n_image_memory_updates_ptr        Deref will be set to the number of non-opaque image memory updates, assigned
         *                                                   to the specified bind info item. May be NULL.
         *  @param out_opt_n_image_opaque_memory_updates_ptr Deref will be set to the number of image opaque memory updates, assigned
         *                                                   to the specified bind info item. May be NULL.
         *  @param out_opt_fence_to_set_ptr                  Deref will be set to the fence, which is going to be set once all
         *                                                   updates assigned to the bind info item are executed. May be NULL.
         *  @param out_opt_n_signal_semaphores_ptr           Deref will be set to the number of semaphores, which should be
         *                                                   signalled after bindings are applied. May be NULL.
         *  @param out_opt_signal_semaphores_ptr_ptr_ptr     Deref will be set to a ptr to an array of signal semaphores. May be NULL.
         *  @param out_opt_n_wait_semaphores_ptr             Deref will be set to the number of semaphores, which should be
         *                                                   waited on before bindings are applied. May be NULL.
         *  @param out_opt_wait_semaphores_ptr_ptr_ptr       Deref will be set to a ptr to an array of wait semaphores. May be NULL.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_bind_info_properties(SparseMemoryBindInfoID in_bind_info_id,
                                      uint32_t* const        out_opt_n_buffer_memory_updates_ptr,
                                      uint32_t* const        out_opt_n_image_memory_updates_ptr,
                                      uint32_t* const        out_opt_n_image_opaque_memory_updates_ptr,
                                      uint32_t* const        out_opt_n_signal_semaphores_ptr,
                                      Anvil::Semaphore***    out_opt_signal_semaphores_ptr_ptr_ptr,
                                      uint32_t* const        out_opt_n_wait_semaphores_ptr,
                                      Anvil::Semaphore***    out_opt_wait_semaphores_ptr_ptr_ptr);

        /** Retrieves Vulkan descriptors which should be used for the vkQueueBindSparse() call.
         *
         *  This call will trigger baking, if the container is marked as dirty.
         *
         *  @param out_bind_info_count_ptr   Deref will be set to the value which should be passed in the
         *                                   <bindInfoCount> argument of the call. Must not be NULL.
         *  @param out_bind_info_ptrs_ptr    Deref will be set to a pointer to an array, which should be
         *                                   passed in the <pBindInfo> argument of the call. Must not be NULL.
         *  @param out_fence_to_set_ptrs_ptr Deref will be set to the fence, which should be set by the implementation
         *                                   after all bindings are in place. Note that the fence itself is optional
         *                                   and may be null.
         **/
        void get_bind_sparse_call_args(uint32_t*                out_bind_info_count_ptr,
                                       const VkBindSparseInfo** out_bind_info_ptrs_ptr,
                                       Anvil::Fence**           out_fence_to_set_ptr_ptr);

        /** Retrieves details of buffer memory binding updates, cached for user-specified bind info.
         *
         *  @param in_bind_info_id                          ID of the bind info, which owns the update, whose properties are
         *                                                  being queried.
         *  @param in_n_update                              Index of the buffer memory update to retrieve properties of.
         *  @param out_opt_buffer_ptr                       If not NULL, deref will be set to the buffer, whose sparse memory
         *                                                  binding should be updated. Otherwise ignored.
         *  @param out_opt_buffer_memory_start_offset_ptr   If not NULL, deref will be set to the start offset of the buffer,
         *                                                  at which the memory block should be bound. Otherwise ignored.
         *  @param out_opt_memory_block_ptr                 If not NULL, deref will be set to the memory block, which should
         *                                                  be used for the binding. Otherwise ignored.
         *  @param out_opt_memory_block_start_offset_ptr    If not NULL, deref will be set to the start offset of the memory block,
         *                                                  from which the memory region, which should be used for the binding,
         *                                                  starts. Otherwise ignored.
         *  @param out_opt_memory_block_owned_by_buffer_ptr TODO.
         *  @param out_opt_size_ptr                         If not NULL, deref will be set to the size of the memory region,
         *                                                  which should be used for the binding. Otherwise ignored.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_buffer_memory_update_properties(SparseMemoryBindInfoID in_bind_info_id,
                                                 uint32_t               in_n_update,
                                                 Anvil::Buffer**        out_opt_buffer_ptr_ptr,
                                                 VkDeviceSize*          out_opt_buffer_memory_start_offset_ptr,
                                                 Anvil::MemoryBlock**   out_opt_memory_block_ptr_ptr,
                                                 VkDeviceSize*          out_opt_memory_block_start_offset_ptr,
                                                 bool*                  out_opt_memory_block_owned_by_buffer_ptr,
                                                 VkDeviceSize*          out_opt_size_ptr) const;

        /** Returns memory & resource device indices assigned to this batch */
        bool get_device_indices(SparseMemoryBindInfoID in_bind_info_id,
                                uint32_t*              out_opt_resource_device_index_ptr,
                                uint32_t*              out_opt_memory_device_index_ptr) const;

        /** Retrieves the fence, if one was earlier assigned to the instance */
        const Anvil::Fence* get_fence() const
        {
            return m_fence_ptr;
        }

        /** Retrieves properties of a non-opaque image memory update with a given ID.
         *
         *  @param in_bind_info_id                         ID of the bind info, which owns the update, and whose properties are
         *                                                 being queried.
         *  @param in_n_update                             Index of the image memory update to retrieve properties of.
         *  @param out_opt_image_ptr_ptr                   If not NULL, deref will be set to the image which should be updated.
         *                                                 Otherwise ignored.
         *  @param out_opt_subresouce_ptr                  If not NULL, deref will be set to the subresource to be used for the
         *                                                 update. Otherwise ignored.
         *  @param out_opt_offset_ptr                      If not NULL, deref will be set to image start offset, at which
         *                                                 the memory block should be bound. Otherwise ignored.
         *  @param out_opt_extent_ptr                      If not NULL, deref will be set to the extent of the update. Otherwise
         *                                                 ignored.
         *  @param out_opt_flags_ptr                       If not NULL, deref will be set to Anvil::SparseMemoryBindFlags value which
         *                                                 is going to be used for the update. Otherwise ignored.
         *  @param out_opt_memory_block_ptr_ptr            If not NULL, deref will be set to pointer to the memory block, which
         *                                                 is going to be used for the bind operation. Otherwise ignored.
         *  @param out_opt_memory_block_start_offset_ptr   If not NULL, deref will be set to the start offset of the memory block,
         *                                                 which should be used for the binding operation. Otherwise ignored.
         *  @param out_opt_memory_block_owned_by_image_ptr TODO
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_image_memory_update_properties(SparseMemoryBindInfoID        in_bind_info_id,
                                                uint32_t                      in_n_update,
                                                Anvil::Image**                out_opt_image_ptr_ptr,
                                                Anvil::ImageSubresource*      out_opt_subresource_ptr,
                                                VkOffset3D*                   out_opt_offset_ptr,
                                                VkExtent3D*                   out_opt_extent_ptr,
                                                Anvil::SparseMemoryBindFlags* out_opt_flags_ptr,
                                                Anvil::MemoryBlock**          out_opt_memory_block_ptr_ptr,
                                                VkDeviceSize*                 out_opt_memory_block_start_offset_ptr,
                                                bool*                         out_opt_memory_block_owned_by_image_ptr) const;

        /** Retrieves properties of an opaque image memory updated with a given ID.
         *
         *  @param in_bind_info_id                         ID of the bind info, which owns the update, and whose properties are being
         *                                                 queried.
         *  @param in_n_update                             Index of the opaque image memory update to retrieve properties of.
         *  @param out_opt_image_ptr_ptr                   If not NULL, deref will be set to the image which should be updated. Otherwise
         *                                                 ignored.
         *  @param out_opt_resource_offset_ptr             If not NULL, deref will be set to the raw image memory offset, which should
         *                                                 be used for the update. Otherwise ignored.
         *  @param out_opt_size_ptr                        If not NULL, deref will be set to the size of the image memory which should
         *                                                 be used for the update. Otherwise ignored.
         *  @param out_opt_flags_ptr                       If not NULL, deref will be set to the Anvil::SparseMemoryBindFlags value which is
         *                                                 going to be used for the update. Otherwise igfnored.
         *  @param out_opt_memory_block_ptr_ptr            If not NULL, deref will be set to pointer to the memory block, which is going
         *                                                 to be used for the bind operation. Otherwise ignored.
         *  @param out_opt_memory_block_start_offset_ptr   If not NULL, deref will be set to the start offset of the memory block, which
         *                                                 should be used for the binding operation. Otherwise ignored.
         *  @param out_opt_memory_block_owned_by_image_ptr TODO
         *  @param out_opt_n_plane_ptr                     If not NULL, deref will be set to index of the image plane to use for the binding
         *                                                 operation. Otherwise ignored.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_image_opaque_memory_update_properties(SparseMemoryBindInfoID        in_bind_info_id,
                                                       uint32_t                      in_n_update,
                                                       Anvil::Image**                out_opt_image_ptr_ptr,
                                                       VkDeviceSize*                 out_opt_resource_offset_ptr,
                                                       VkDeviceSize*                 out_opt_size_ptr,
                                                       Anvil::SparseMemoryBindFlags* out_opt_flags_ptr,
                                                       Anvil::MemoryBlock**          out_opt_memory_block_ptr_ptr,
                                                       VkDeviceSize*                 out_opt_memory_block_start_offset_ptr,
                                                       bool*                         out_opt_memory_block_owned_by_image_ptr,
                                                       uint32_t*                     out_opt_n_plane_ptr) const;

        /** Tells how many bind info items have been assigned to the descriptor */
        uint32_t get_n_bind_info_items() const
        {
            return static_cast<uint32_t>(m_bindings.size() );
        }

        /* Tells whether this instance requires VK_KHR_device_group extension in order for the vkQueueBindSparse()
         * invocation. */
        bool is_device_group_support_required() const;

        /* Changes the fence (null by default), which should be set by the Vulkan implementation after it finishes
         * updating the bindings.
        **/
        void set_fence(Anvil::Fence* in_fence_ptr)
        {
            m_fence_ptr = in_fence_ptr;
        }

        /* Updates memory device index, associated with this batch.
         *
         * Do not modify unless VK_KHR_device_group is supported
         *
         * @param in_memory_device_index New memory device index to use.
         *
         **/
        void set_memory_device_index(SparseMemoryBindInfoID in_bind_info_id,
                                     const uint32_t&        in_memory_device_index);

        /* Updates resource device index, associated with this batch.
         *
         * Do not modify unless VK_KHR_device_group is supported
         *
         * @param in_resource_device_index New resource device index to use.
         *
         **/
        void set_resource_device_index(SparseMemoryBindInfoID in_bind_info_id,
                                       const uint32_t&        in_resource_device_index);

    private:
        /* Private type definitions */
        typedef struct GeneralBindInfo
        {
            bool                memory_block_owned_by_target;
            Anvil::MemoryBlock* memory_block_ptr;
            VkDeviceSize        memory_block_start_offset;
            uint32_t            n_plane;
            VkDeviceSize        size;
            VkDeviceSize        start_offset;

            SparseMemoryBindFlags flags;

            GeneralBindInfo()
            {
                memory_block_owned_by_target = false;
                memory_block_ptr             = nullptr;
                memory_block_start_offset    = 0;
                n_plane                      = 0;
                size                         = 0;
                start_offset                 = 0;
            }
        } GeneralBindInfo;

        typedef struct ImageBindInfo
        {
            VkExtent3D              extent;
            bool                    memory_block_owned_by_image;
            Anvil::MemoryBlock*     memory_block_ptr;
            VkDeviceSize            memory_block_start_offset;
            VkOffset3D              offset;
            Anvil::ImageSubresource subresource;

            SparseMemoryBindFlags flags;

            ImageBindInfo()
            {
                memory_block_owned_by_image = false;
                memory_block_ptr            = nullptr;
                memory_block_start_offset   = UINT32_MAX;
                offset.x                    = 0;
                offset.y                    = 0;
                offset.z                    = 0;
            }
        } ImageBindInfo;

        typedef std::map<Anvil::Buffer*, std::pair<std::vector<GeneralBindInfo>, std::vector<VkSparseMemoryBind>      >> BufferBindUpdateMap;
        typedef std::map<Anvil::Image*,  std::pair<std::vector<ImageBindInfo>,   std::vector<VkSparseImageMemoryBind> >> ImageBindUpdateMap;
        typedef std::map<Anvil::Image*,  std::pair<std::vector<GeneralBindInfo>, std::vector<VkSparseMemoryBind>      >> ImageOpaqueBindUpdateMap;

        typedef struct BindingInfo
        {
            BufferBindUpdateMap      buffer_updates;
            ImageOpaqueBindUpdateMap image_opaque_updates;
            ImageBindUpdateMap       image_updates;

            uint32_t memory_device_index;
            uint32_t resource_device_index;

            std::vector<Anvil::Semaphore*> signal_semaphores;
            std::vector<VkSemaphore>       signal_semaphores_vk;
            std::vector<Anvil::Semaphore*> wait_semaphores;
            std::vector<VkSemaphore>       wait_semaphores_vk;

            BindingInfo()
            {
                memory_device_index   = 0;
                resource_device_index = 0;
            }
        } BindingInfo;

        std::vector<BindingInfo> m_bindings;
        bool                     m_dirty;
        Anvil::Fence*            m_fence_ptr;

        std::vector<VkBindSparseInfo>                  m_bindings_vk;
        std::vector<VkSparseBufferMemoryBindInfo>      m_buffer_bindings_vk;
        std::vector<VkDeviceGroupBindSparseInfoKHR>    m_device_group_bindings_vk;
        std::vector<VkSparseImageMemoryBindInfo>       m_image_bindings_vk;
        std::vector<VkSparseImageOpaqueMemoryBindInfo> m_image_opaque_bindings_vk;

        /* Private functions */
        SparseMemoryBindingUpdateInfo          (const SparseMemoryBindingUpdateInfo&);
        SparseMemoryBindingUpdateInfo operator=(const SparseMemoryBindingUpdateInfo&);

        void bake();
    };
}; /* namespace Anvil */

#endif /* TYPES_CLASSES_H */