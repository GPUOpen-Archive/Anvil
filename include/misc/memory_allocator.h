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

/* Defines a MemoryAllocator class which allocates & maintains a memory block for all registered
 * objects. At baking time, non-overlapping regions of memory storage are distributed to the objects,
 * respect to object-specific alignment requirements.
 *
 * MT-safe at an opt-in basis.
 **/
#ifndef MISC_MEMORY_ALLOCATOR_H
#define MISC_MEMORY_ALLOCATOR_H

#include "misc/debug.h"
#include "misc/mt_safety.h"
#include "misc/types.h"
#include <functional>
#include <vector>
#include <cfloat>

namespace Anvil
{
    typedef std::pair<uint32_t, uint32_t>                                            LocalRemoteDeviceIndexPair;
    typedef std::pair<uint32_t, uint32_t>                                            ResourceMemoryDeviceIndexPair;
    typedef std::function<void (Anvil::MemoryAllocator*) >                           MemoryAllocatorBakeCallbackFunction;
    typedef std::function<void (Anvil::Buffer*,       Anvil::MemoryBlockUniquePtr) > MemoryAllocatorPostBakePerNonSparseBufferItemMemAssignmentCallback;
    typedef std::function<void (Anvil::Image*,        Anvil::MemoryBlockUniquePtr) > MemoryAllocatorPostBakePerNonSparseImageItemMemAssignmentCallback;
    typedef std::map<LocalRemoteDeviceIndexPair, Anvil::PeerMemoryFeatureFlags>      MGPUPeerMemoryRequirements;
    typedef std::vector<ResourceMemoryDeviceIndexPair>                               MGPUBindSparseDeviceIndices;

    class MemoryAllocator : public MTSafetySupportProvider
    {
    public:
        /* Public type definitions */
        typedef enum
        {
            ITEM_TYPE_BUFFER,
            ITEM_TYPE_IMAGE_WHOLE,

            ITEM_TYPE_SPARSE_BUFFER_REGION,
            ITEM_TYPE_SPARSE_IMAGE_MIPTAIL,
            ITEM_TYPE_SPARSE_IMAGE_SUBRESOURCE,
        } ItemType;

        typedef struct Item
        {
            Anvil::Buffer*                                                                        buffer_ptr;
            std::unique_ptr<float[]>                                                              buffer_ref_float_data_ptr;
            std::unique_ptr<std::vector<float>, std::function<void (std::vector<float>*)> >       buffer_ref_float_vector_data_ptr;
            std::unique_ptr<uint8_t[]>                                                            buffer_ref_uchar8_data_ptr;
            std::unique_ptr<std::vector<unsigned char> >                                          buffer_ref_uchar8_vector_data_ptr;
            std::unique_ptr<uint32_t[]>                                                           buffer_ref_uint32_data_ptr;
            std::unique_ptr<std::vector<uint32_t>, std::function<void (std::vector<uint32_t>*)> > buffer_ref_uint32_vector_data_ptr;
            Anvil::Image*                                                                         image_ptr;

            Anvil::MemoryAllocator* memory_allocator_ptr;

            ItemType type;

            #if defined(_WIN32)
                const Anvil::ExternalNTHandleInfo* alloc_external_nt_handle_info_ptr;
            #endif

            uint32_t                             alloc_device_mask;
            Anvil::ExternalMemoryHandleTypeFlags alloc_exportable_external_handle_types;
            Anvil::ImageAspectFlagBits           alloc_image_aspect;
            bool                                 alloc_is_dedicated_memory;
            bool                                 alloc_is_scratch_buffer_alloc;
            MemoryBlockUniquePtr                 alloc_memory_block_ptr;
            uint32_t                             alloc_memory_final_type;
            VkDeviceSize                         alloc_memory_required_alignment;
            MemoryFeatureFlags                   alloc_memory_required_features;
            uint32_t                             alloc_memory_supported_memory_types;
            uint32_t                             alloc_memory_types;
            MGPUBindSparseDeviceIndices          alloc_mgpu_bind_sparse_device_indices;
            MGPUPeerMemoryRequirements           alloc_mgpu_peer_memory_reqs;
            VkDeviceSize                         alloc_offset;
            VkDeviceSize                         alloc_size;
            float                                memory_priority;

            VkExtent3D              extent;
            bool                    is_baked;
            VkDeviceSize            miptail_offset;
            uint32_t                n_layer;
            uint32_t                n_plane;
            VkOffset3D              offset;
            Anvil::ImageSubresource subresource;

            Item(Anvil::MemoryAllocator*                     in_memory_allocator_ptr,
                 Anvil::Buffer*                              in_buffer_ptr,
                 VkDeviceSize                                in_alloc_size,
                 uint32_t                                    in_alloc_memory_types,
                 VkDeviceSize                                in_alloc_alignment,
                 MemoryFeatureFlags                          in_alloc_required_memory_features,
                 uint32_t                                    in_alloc_supported_memory_types,
                 const Anvil::ExternalMemoryHandleTypeFlags& in_opt_exportable_external_handle_types,

#if defined(_WIN32)
                 const Anvil::ExternalNTHandleInfo*          in_alloc_external_nt_handle_info_ptr,
#endif
                 const bool&                                 in_alloc_is_dedicated,
                 const uint32_t&                             in_device_mask,
                 const MGPUPeerMemoryRequirements&           in_mgpu_peer_memory_reqs,
                 const MGPUBindSparseDeviceIndices&          in_mgpu_bind_sparse_device_indices,
                 const float&                                in_memory_priority);

            Item(Anvil::MemoryAllocator*                     in_memory_allocator_ptr,
                 Anvil::Buffer*                              in_buffer_ptr,
                 VkDeviceSize                                in_alloc_offset,
                 VkDeviceSize                                in_alloc_size,
                 uint32_t                                    in_alloc_memory_types,
                 VkDeviceSize                                in_alloc_alignment,
                 MemoryFeatureFlags                          in_alloc_required_memory_features,
                 uint32_t                                    in_alloc_supported_memory_types,
                 const Anvil::ExternalMemoryHandleTypeFlags& in_opt_exportable_external_handle_types,
#if defined(_WIN32)
                 const Anvil::ExternalNTHandleInfo*          in_alloc_external_nt_handle_info_ptr,
#endif
                 const bool&                                 in_alloc_is_dedicated,
                 const uint32_t&                             in_device_mask,
                 const MGPUPeerMemoryRequirements&           in_mgpu_peer_memory_reqs,
                 const MGPUBindSparseDeviceIndices&          in_mgpu_bind_sparse_device_indices,
                 const float&                                in_memory_priority);

            Item(Anvil::MemoryAllocator*                     in_memory_allocator_ptr,
                 Anvil::Image*                               in_image_ptr,
                 uint32_t                                    in_n_layer,
                 VkDeviceSize                                in_alloc_size,
                 uint32_t                                    in_alloc_memory_types,
                 VkDeviceSize                                in_miptail_offset,
                 const Anvil::ImageAspectFlagBits&           in_alloc_aspect,
                 VkDeviceSize                                in_alloc_alignment,
                 MemoryFeatureFlags                          in_alloc_required_memory_features,
                 uint32_t                                    in_alloc_supported_memory_types,
                 const Anvil::ExternalMemoryHandleTypeFlags& in_opt_exportable_external_handle_types,
#if defined(_WIN32)
                 const Anvil::ExternalNTHandleInfo*          in_alloc_external_nt_handle_info_ptr,
#endif
                 const bool&                                 in_alloc_is_dedicated,
                 const uint32_t&                             in_device_mask,
                 const MGPUPeerMemoryRequirements&           in_mgpu_peer_memory_reqs,
                 const MGPUBindSparseDeviceIndices&          in_mgpu_bind_sparse_device_indices,
                 const float&                                in_memory_priority);

            Item(Anvil::MemoryAllocator*                     in_memory_allocator_ptr,
                 Anvil::Image*                               in_image_ptr,
                 const Anvil::ImageSubresource&              in_subresource,
                 const VkOffset3D&                           in_offset,
                 const VkExtent3D&                           in_extent,
                 VkDeviceSize                                in_alloc_size,
                 uint32_t                                    in_alloc_memory_types,
                 VkDeviceSize                                in_alloc_alignment,
                 MemoryFeatureFlags                          in_alloc_required_memory_features,
                 uint32_t                                    in_alloc_supported_memory_types,
                 const Anvil::ExternalMemoryHandleTypeFlags& in_opt_exportable_external_handle_types,
#if defined(_WIN32)
                 const Anvil::ExternalNTHandleInfo*          in_alloc_external_nt_handle_info_ptr,
#endif
                 const bool&                                 in_alloc_is_dedicated,
                 const uint32_t&                             in_device_mask,
                 const MGPUPeerMemoryRequirements&           in_mgpu_peer_memory_reqs,
                 const MGPUBindSparseDeviceIndices&          in_mgpu_bind_sparse_device_indices,
                 const float&                                in_memory_priority);

            Item(Anvil::MemoryAllocator*                     in_memory_allocator_ptr,
                 Anvil::Image*                               in_image_ptr,
                 VkDeviceSize                                in_alloc_size,
                 uint32_t                                    in_alloc_memory_types,
                 VkDeviceSize                                in_alloc_alignment,
                 MemoryFeatureFlags                          in_alloc_required_memory_features,
                 uint32_t                                    in_alloc_supported_memory_types,
                 const Anvil::ExternalMemoryHandleTypeFlags& in_opt_exportable_external_handle_types,
#if defined(_WIN32)
                 const Anvil::ExternalNTHandleInfo*          in_alloc_external_nt_handle_info_ptr,
#endif
                 const bool&                                 in_alloc_is_dedicated,
                 const uint32_t&                             in_device_mask,
                 const MGPUPeerMemoryRequirements&           in_mgpu_peer_memory_reqs,
                 const MGPUBindSparseDeviceIndices&          in_mgpu_bind_sparse_device_indices,
                 const float&                                in_memory_priority,
                 const uint32_t&                             in_n_plane);

            /** TODO */
            ~Item();

        private:
            void register_for_callbacks   ();
            void unregister_from_callbacks();

            bool buffer_has_is_alloc_pending_callback_registered;
            bool buffer_has_memory_block_needed_callback_registered;
            bool image_has_is_alloc_pending_callback_registered;
            bool image_has_memory_block_needed_callback_registered;
        } Item;

        typedef std::vector<std::unique_ptr<Item> > Items;

        class IMemoryAllocatorBackend : public IMemoryAllocatorBackendBase
        {
        public:
            virtual ~IMemoryAllocatorBackend()
            {
                /* Stub */
            }

            virtual bool bake                            (Items&                                      in_items)                              = 0;
            virtual bool supports_device_masks           ()                                                                            const = 0;
            virtual bool supports_external_memory_handles(const Anvil::ExternalMemoryHandleTypeFlags& in_external_memory_handle_types) const = 0;
            virtual bool supports_protected_memory       ()                                                                            const = 0;
        };

        /* Public functions */

        /** Adds a new Buffer object which should use storage coming from the buffer memory
         *  maintained by the Memory Allocator.
         *
         *  @param in_buffer_ptr                              Buffer to configure storage for at bake() call time. Must not
         *                                                    be nullptr.
         *  @param in_data_ptr                                The buffer will be filled with data extracted from the specified
         *                                                    location. The number of bytes which will be stored is defined by
         *                                                    buffer size.
         *  @param in_data_vector_ptr                         The buffer will be filled with data extracted from the specified
         *                                                    vector. Total number of bytes defined in the vector must match
         *                                                    buffer size.
         *  @param in_required_memory_features                Memory features the assigned memory must support.
         *                                                    See MemoryFeatureFlagBits for more details.
         *  @param in_opt_external_nt_handle_info_ptr         TODO. Pointer must remain valid till baking time.
         *  @param in_opt_device_mask_ptr                     If not null, deref should contain a valid device mask to be used for
         *                                                    the allocation. Specifying a device mask annotates the allocation request
         *                                                    with VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT flag. Value from deref is copied
         *                                                    at call time, the pointer may be released when the func leaves.
         *  @param in_opt_mgpu_bind_sparse_device_indices_ptr If not null, deref may contain up to N pairs of resource and memory device indices, where N is the number of devices
         *                                                    in the device group. The indices will be used during sparse bind operation, binding the buffer on the device with
         *                                                    resource index to a memory allocation instance on the device with memory index. If null or empty, the binding will work
         *                                                    as if both indices were zero.
         *  @param in_opt_memory_priority                     Memory priority to use for the allocation. Valid values must be within range [0.0f, 1.0f].
         *                                                    Ignored if VK_EXT_memory_priority is unavailable or if VMA backend is used or if FLT_MAX is passed.
         *
         *  @return true if the buffer has been successfully scheduled for baking, false otherwise.
         **/
        bool add_buffer                                            (Anvil::Buffer*                               in_buffer_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features,
                                                                    const Anvil::ExternalMemoryHandleTypeFlags&  in_opt_exportable_external_handle_types    = Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
        #if defined(_WIN32)
                                                                    const Anvil::ExternalNTHandleInfo*           in_opt_external_nt_handle_info_ptr         = nullptr,
        #endif
                                                                    const uint32_t*                              in_opt_device_mask_ptr                     = nullptr,
                                                                    const MGPUPeerMemoryRequirements*            in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                                                    const MGPUBindSparseDeviceIndices*           in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                                                    const float&                                 in_opt_memory_priority                     = FLT_MAX);
        bool add_buffer_with_float_data_ptr_based_post_fill        (Anvil::Buffer*                               in_buffer_ptr,
                                                                    std::unique_ptr<float[]>                     in_data_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features,
                                                                    const Anvil::ExternalMemoryHandleTypeFlags&  in_opt_exportable_external_handle_types    = Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
        #if defined(_WIN32)
                                                                    const Anvil::ExternalNTHandleInfo*           in_opt_external_nt_handle_info_ptr         = nullptr,
        #endif
                                                                    const uint32_t*                              in_opt_device_mask_ptr                     = nullptr,
                                                                    const MGPUPeerMemoryRequirements*            in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                                                    const MGPUBindSparseDeviceIndices*           in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                                                    const float&                                 in_opt_memory_priority                     = FLT_MAX);
        bool add_buffer_with_float_data_vector_ptr_based_post_fill (Anvil::Buffer*                               in_buffer_ptr,
                                                                    std::unique_ptr<std::vector<float> >         in_data_vector_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features,
                                                                    const Anvil::ExternalMemoryHandleTypeFlags&  in_opt_exportable_external_handle_types    = Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
        #if defined(_WIN32)
                                                                    const Anvil::ExternalNTHandleInfo*           in_opt_external_nt_handle_info_ptr         = nullptr,
        #endif
                                                                    const uint32_t*                              in_opt_device_mask_ptr                     = nullptr,
                                                                    const MGPUPeerMemoryRequirements*            in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                                                    const MGPUBindSparseDeviceIndices*           in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                                                    const float&                                 in_opt_memory_priority                     = FLT_MAX);
        bool add_buffer_with_float_data_vector_ptr_based_post_fill (Anvil::Buffer*                               in_buffer_ptr,
                                                                    const std::vector<float>*                    in_data_vector_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features,
                                                                    const Anvil::ExternalMemoryHandleTypeFlags&  in_opt_exportable_external_handle_types    = Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
        #if defined(_WIN32)
                                                                    const Anvil::ExternalNTHandleInfo*           in_opt_external_nt_handle_info_ptr         = nullptr,
        #endif
                                                                    const uint32_t*                              in_opt_device_mask_ptr                     = nullptr,
                                                                    const MGPUPeerMemoryRequirements*            in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                                                    const MGPUBindSparseDeviceIndices*           in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                                                    const float&                                 in_opt_memory_priority                     = FLT_MAX);
        bool add_buffer_with_uchar8_data_ptr_based_post_fill       (Anvil::Buffer*                               in_buffer_ptr,
                                                                    std::unique_ptr<uint8_t[]>                   in_data_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features,
                                                                    const Anvil::ExternalMemoryHandleTypeFlags&  in_opt_exportable_external_handle_types    = Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
        #if defined(_WIN32)
                                                                    const Anvil::ExternalNTHandleInfo*           in_opt_external_nt_handle_info_ptr         = nullptr,
        #endif
                                                                    const uint32_t*                              in_opt_device_mask_ptr                     = nullptr,
                                                                    const MGPUPeerMemoryRequirements*            in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                                                    const MGPUBindSparseDeviceIndices*           in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                                                    const float&                                 in_opt_memory_priority                     = FLT_MAX);
        bool add_buffer_with_uchar8_data_vector_ptr_based_post_fill(Anvil::Buffer*                               in_buffer_ptr,
                                                                    std::unique_ptr<std::vector<unsigned char> > in_data_vector_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features,
                                                                    const Anvil::ExternalMemoryHandleTypeFlags&  in_opt_exportable_external_handle_types    = Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
        #if defined(_WIN32)
                                                                    const Anvil::ExternalNTHandleInfo*           in_opt_external_nt_handle_info_ptr         = nullptr,
        #endif
                                                                    const uint32_t*                              in_opt_device_mask_ptr                     = nullptr,
                                                                    const MGPUPeerMemoryRequirements*            in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                                                    const MGPUBindSparseDeviceIndices*           in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                                                    const float&                                 in_opt_memory_priority                     = FLT_MAX);
        bool add_buffer_with_uint32_data_ptr_based_post_fill       (Anvil::Buffer*                               in_buffer_ptr,
                                                                    std::unique_ptr<uint32_t[]>                  in_data_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features,
                                                                    const Anvil::ExternalMemoryHandleTypeFlags&  in_opt_exportable_external_handle_types    = Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
        #if defined(_WIN32)
                                                                    const Anvil::ExternalNTHandleInfo*           in_opt_external_nt_handle_info_ptr         = nullptr,
        #endif
                                                                    const uint32_t*                              in_opt_device_mask_ptr                     = nullptr,
                                                                    const MGPUPeerMemoryRequirements*            in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                                                    const MGPUBindSparseDeviceIndices*           in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                                                    const float&                                 in_opt_memory_priority                     = FLT_MAX);
        bool add_buffer_with_uint32_data_vector_ptr_based_post_fill(Anvil::Buffer*                               in_buffer_ptr,
                                                                    std::unique_ptr<std::vector<uint32_t> >      in_data_vector_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features,
                                                                    const Anvil::ExternalMemoryHandleTypeFlags&  in_opt_exportable_external_handle_types    = Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
        #if defined(_WIN32)
                                                                    const Anvil::ExternalNTHandleInfo*           in_opt_external_nt_handle_info_ptr         = nullptr,
        #endif
                                                                    const uint32_t*                              in_opt_device_mask_ptr                     = nullptr,
                                                                    const MGPUPeerMemoryRequirements*            in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                                                    const MGPUBindSparseDeviceIndices*           in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                                                    const float&                                 in_opt_memory_priority                     = FLT_MAX);
        bool add_buffer_with_uint32_data_vector_ptr_based_post_fill(Anvil::Buffer*                               in_buffer_ptr,
                                                                    const std::vector<uint32_t>*                 in_data_vector_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features,
                                                                    const Anvil::ExternalMemoryHandleTypeFlags&  in_opt_exportable_external_handle_types    = Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
        #if defined(_WIN32)
                                                                    const Anvil::ExternalNTHandleInfo*           in_opt_external_nt_handle_info_ptr         = nullptr,
        #endif
                                                                    const uint32_t*                              in_opt_device_mask_ptr                     = nullptr,
                                                                    const MGPUPeerMemoryRequirements*            in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                                                    const MGPUBindSparseDeviceIndices*           in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                                                    const float&                                 in_opt_memory_priority                     = FLT_MAX);

        /** TODO
         *
         *  @param in_buffer_ptr                              TODO
         *  @param in_offset                                  TODO. Must be divisible by VkMemoryRequirements::alignment
         *  @param in_size                                    TODO.
         *  @param in_required_memory_features                TODO
         *  @param in_opt_device_mask_ptr                     If not null, deref should contain a valid device mask to be used for
         *                                                    the allocation. Specifying a device mask annotates the allocation request
         *                                                    with VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT flag. Value from deref is copied
         *                                                    at call time, the pointer may be released when the func leaves.
         *  @param in_opt_memory_reqs_ptr                     If not null, deref contains information of peer memory requirements which
         *                                                    should be taken into account when determining which memory heap allocations
         *                                                    should be made off. Specified device indices must be included in @param in_opt_device_mask_ptr.
         *  @param in_opt_mgpu_bind_sparse_device_indices_ptr If not null, deref may contain up to N pairs of resource and memory device indices, where N is the number of devices
         *                                                    in the device group. The indices will be used during sparse bind operation, binding the buffer on the device with
         *                                                    resource index to a memory allocation instance on the device with memory index. If null or empty, the binding will work
         *                                                    as if both indices were zero.
         *  @param in_opt_memory_priority                     Memory priority to use for the allocation. Valid values must be within range [0.0f, 1.0f].
         *                                                    Ignored if VK_EXT_memory_priority is unavailable or if VMA backend is used or if FLT_MAX is passed.
         *
         *  @return TODO
         */
        bool add_sparse_buffer_region(Anvil::Buffer*                        in_buffer_ptr,
                                      VkDeviceSize                          in_offset,
                                      VkDeviceSize                          in_size,
                                      MemoryFeatureFlags                    in_required_memory_features,
                                      const uint32_t*                       in_opt_device_mask_ptr                     = nullptr,
                                      const MGPUPeerMemoryRequirements*     in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                      const MGPUBindSparseDeviceIndices*    in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                      const float&                          in_opt_memory_priority                     = FLT_MAX);


        /** Adds an Image object which should be assigned storage coming from memory objects
         *  maintained by the Memory Allocator. At baking time, all subresources of the image,
         *  as well as all miptails (in case of resident images) will be assigned memory regions.
         *
         *  This function can be used against both non-sparse and sparse images.
         *
         *  @param image_ptr                                  Image to configure storage for at bake() call time. Must not
         *                                                    be nullptr.
         *  @param in_required_memory_features                Memory features the assigned memory must support.
         *                                                    See MemoryFeatureFlagBits for more details.
         *  @param in_opt_external_nt_handle_info_ptr         TODO. Pointer must remain valid till baking time.
         *  @param in_opt_device_mask_ptr                     If not null, deref should contain a valid device mask to be used for
         *                                                    the allocation. Specifying a device mask annotates the allocation request
         *                                                    with VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT flag. Value from deref is copied
         *                                                    at call time, the pointer may be released when the func leaves.
         *  @param in_opt_mgpu_bind_sparse_device_indices_ptr If not null, deref may contain up to N pairs of resource and memory device indices, where N is the number of devices
         *                                                    in the device group. The indices will be used during sparse bind operation, binding the image on the device with
         *                                                    resource index to a memory allocation instance on the device with memory index. If null or empty, the binding will work
         *                                                    as if both indices were zero.
         *  @param in_opt_memory_priority                     Memory priority to use for the allocation. Valid values must be within range [0.0f, 1.0f].
         *                                                    Ignored if VK_EXT_memory_priority is unavailable or if VMA backend is used or if FLT_MAX is passed.
         *
         *  @return true if the image has been successfully scheduled for baking, false otherwise.
         **/
        bool add_image_whole(Anvil::Image*                               in_image_ptr,
                             MemoryFeatureFlags                          in_required_memory_features,
                             const Anvil::ExternalMemoryHandleTypeFlags& in_opt_exportable_external_handle_types    = Anvil::ExternalMemoryHandleTypeFlagBits::NONE,
#if defined(_WIN32)
                             const Anvil::ExternalNTHandleInfo*          in_opt_external_nt_handle_info_ptr         = nullptr,
        #endif
                             const uint32_t*                             in_opt_device_mask_ptr                     = nullptr,
                             const MGPUPeerMemoryRequirements*           in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                             const MGPUBindSparseDeviceIndices*          in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                             const float&                                in_opt_memory_priority                     = FLT_MAX);


        /** Adds a new Image object whose layer @param in_n_layer 's miptail for @param in_aspect
         *  aspect should be assigned a physical memory backing. The miptail will be bound a memory
         *  region at baking time.
         *
         *  If the image needs to be assigned just a single miptail, @param in_n_layer should be
         *  set to 0.
         *
         *  This function can only be used for sparse resident images.
         *
         *  @param in_image_ptr                               Image to use for the request. Must not be null.
         *  @param in_aspect                                  Aspect to be used for the request.
         *  @param in_n_layer                                 Index of the layer to attach the miptail to.
         *  @param in_required_memory_features                Memory features the assigned memory must support.
         *                                                    See MemoryFeatureFlagBits for more details.
         *  @param in_opt_device_mask_ptr                     If not null, deref should contain a valid device mask to be used for
         *                                                    the allocation. Specifying a device mask annotates the allocation request
         *                                                    with VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT flag. Value from deref is copied
         *                                                    at call time, the pointer may be released when the func leaves.
         *  @param in_opt_mgpu_bind_sparse_device_indices_ptr If not null, deref may contain up to N pairs of resource and memory device indices, where N is the number of devices
         *                                                    in the device group. The indices will be used during sparse bind operation, binding the image on the device with
         *                                                    resource index to a memory allocation instance on the device with memory index. If null or empty, the binding will work
         *                                                    as if both indices were zero.
         *  @param in_opt_memory_priority                     Memory priority to use for the allocation. Valid values must be within range [0.0f, 1.0f].
         *                                                    Ignored if VK_EXT_memory_priority is unavailable or if VMA backend is used or if FLT_MAX is passed.
         *
         *  @return true if the miptail has been successfully scheduled for baking, false otherwise.
         */
        bool add_sparse_image_miptail(Anvil::Image*                         in_image_ptr,
                                      Anvil::ImageAspectFlagBits            in_aspect,
                                      uint32_t                              in_n_layer,
                                      MemoryFeatureFlags                    in_required_memory_features,
                                      const uint32_t*                       in_opt_device_mask_ptr                     = nullptr,
                                      const MGPUPeerMemoryRequirements*     in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                      const MGPUBindSparseDeviceIndices*    in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                      const float&                          in_opt_memory_priority                     = FLT_MAX);


        /** Adds a single subresource which should be assigned memory backing.
         *
         *  This function does NOT alloc memory for the miptail. It is user's responsibilty to call
         *  add_sparse_image_miptail() for any layers which require a miptail.
         *
         *  @param in_image_ptr                               Image to use for the request. Must not be null.
         *  @param in_subresource                             Specifies details of the subresource to attach memory backing to.
         *  @param in_offset                                  XYZ offset, from which the memory backing should be attached. Must
         *                                                    be rounded up to tile size of the aspect @param in_subresource
         *                                                    refers to.
         *  @param in_extent                                  Size of the region to assign memory backing to. Must be rounded up
         *                                                    to tile size of the aspect @param in_subresource refers to, UNLESS
         *                                                    @param in_offset + @param in_extent touches the subresource border.
         *  @param in_required_memory_features                Memory features the assigned memory must support.
         *                                                    See MemoryFeatureFlagBits for more details.
         *  @param in_opt_device_mask_ptr                     If not null, deref should contain a valid device mask to be used for
         *                                                    the allocation. Specifying a device mask annotates the allocation request
         *                                                    with VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT flag. Value from deref is copied
         *                                                    at call time, the pointer may be released when the func leaves.
         *  @param in_opt_mgpu_bind_sparse_device_indices_ptr If not null, deref may contain up to N pairs of resource and memory device indices, where N is the number of devices
         *                                                    in the device group. The indices will be used during sparse bind operation, binding the image on the device with
         *                                                    resource index to a memory allocation instance on the device with memory index. If null or empty, the binding will work
         *                                                    as if both indices were zero.
         *  @param in_opt_memory_priority                     Memory priority to use for the allocation. Valid values must be within range [0.0f, 1.0f].
         *                                                    Ignored if VK_EXT_memory_priority is unavailable or if VMA backend is used or if FLT_MAX is passed.
         *
         *  @return true if the subresource has been successfully scheduled for baking, false otherwise.
         **/
        bool add_sparse_image_subresource(Anvil::Image*                         in_image_ptr,
                                          const Anvil::ImageSubresource&        in_subresource,
                                          const VkOffset3D&                     in_offset,
                                          VkExtent3D                            in_extent,
                                          MemoryFeatureFlags                    in_required_memory_features,
                                          const uint32_t*                       in_opt_device_mask_ptr                     = nullptr,
                                          const MGPUPeerMemoryRequirements*     in_opt_mgpu_peer_memory_reqs_ptr           = nullptr,
                                          const MGPUBindSparseDeviceIndices*    in_opt_mgpu_bind_sparse_device_indices_ptr = nullptr,
                                          const float&                          in_opt_memory_priority                     = FLT_MAX);

        /** TODO */
        bool bake();

        /** Creates a new one-shot memory allocator instance.
         *
         *  This type of allocator only supports a single explicit (or implicit) bake invocation.
         *
         *  @param in_device_ptr Device to use.
         **/
        static Anvil::MemoryAllocatorUniquePtr create_oneshot(const Anvil::BaseDevice* in_device_ptr,
                                                              MTSafety                 in_mt_safety = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE);

        /** Creates a new VMA memory allocator instance.
         *
         *  This type of allocator supports an arbitrary number of implicit or explicit bake invocations.
         *  This type of allocator does NOT support external handles of any type.
         *  This type of allocator does NOT support device masks.
         *
         *  @param in_device_ptr Device to use.
         **/
        static Anvil::MemoryAllocatorUniquePtr create_vma(const Anvil::BaseDevice* in_device_ptr,
                                                          MTSafety                 in_mt_safety = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE);

        static bool get_mem_types_supporting_mem_features(const Anvil::BaseDevice*         in_device_ptr,
                                                          uint32_t                         in_memory_types,
                                                          const Anvil::MemoryFeatureFlags& in_memory_features,
                                                          uint32_t*                        out_opt_filtered_memory_types_ptr);

        /** By default, once memory regions are baked, memory allocator will bind them to objects specified
         *  at add_*() call time. Use cases exist where apps may prefer to handle this action on their own.
         *
         *  When a post-bake per-item mem assignment callback is specified, the allocator will NO LONGER:
         *
         *  - (NON-SPARSE OBJECTS): call set_(nonsparse_)memory() functions and fill the regions with user-specified data,
         *                          if such was provided at add_*() call time.
         *  - (SPARSE OBJECTS):     bind allocated memory to user-specified sparse memory regions.
         *
         *  The allocator WILL still allocate physical memory as determined to be required for the specified list of objects
         *  and use the callback specified in this call to let the application do whatever it needs to with the allocated memory.
         *
         *  All callback functions must be specified for all types of objects supported by the allocator.
         *
         *  Do not use this function unless you have a strong understanding of the implications.
         *
         *  NOTE: Support for sparse buffers & images remains a TODO.
         */
        void set_post_bake_per_item_mem_assignment_callbacks(MemoryAllocatorPostBakePerNonSparseBufferItemMemAssignmentCallback in_callback_function_for_buffers,
                                                             MemoryAllocatorPostBakePerNonSparseImageItemMemAssignmentCallback  in_callback_function_for_images)
        {
            anvil_assert(in_callback_function_for_buffers        != nullptr);
            anvil_assert(in_callback_function_for_images         != nullptr);

            m_post_bake_per_buffer_item_mem_assignment_callback_function = in_callback_function_for_buffers;
            m_post_bake_per_image_item_mem_assignment_callback_function  = in_callback_function_for_images;
        }

        /** Assigns a func pointer which will be called by the allocator after all added objects
         *  have been assigned memory blocks.
         *
         *  Calling this function more than once for the same MemoryAllocator instance will trigger
         *  an assertion failure.
         *
         *  @param in_post_bake_callback_function Function pointer to assign. Must not be null.
         *
         */
        void set_post_bake_callback(MemoryAllocatorBakeCallbackFunction in_post_bake_callback_function);

         /** Destructor.
          *
          *  Releases the underlying MemoryBlock instance
          **/
        ~MemoryAllocator();

    private:
        /* Private functions */
        bool add_buffer_internal(Anvil::Buffer*                              in_buffer_ptr,
                                 MemoryFeatureFlags                          in_required_memory_features,
                                 const Anvil::ExternalMemoryHandleTypeFlags& in_opt_exportable_external_handle_types,
#if defined(_WIN32)
                                 const Anvil::ExternalNTHandleInfo*          in_opt_external_nt_handle_info_ptr,
#endif
                                 const uint32_t*                             in_opt_device_mask_ptr,
                                 const MGPUPeerMemoryRequirements*           in_opt_mgpu_peer_memory_reqs_ptr,
                                 const MGPUBindSparseDeviceIndices*          in_opt_mgpu_bind_sparse_device_indices_ptr,
                                 const float&                                in_opt_memory_priority);

        bool do_bind_sparse_device_indices_sanity_check  (const MGPUBindSparseDeviceIndices*          in_opt_mgpu_bind_sparse_device_indices_ptr) const;
        bool do_external_memory_handle_type_sanity_checks(const Anvil::ExternalMemoryHandleTypeFlags& in_external_memory_handle_types) const;

        void on_is_alloc_pending_for_buffer_query(CallbackArgument* in_callback_arg_ptr);
        void on_is_alloc_pending_for_image_query (CallbackArgument* in_callback_arg_ptr);
        void on_implicit_bake_needed             ();

        /** Constructor.
         *
         *  Please see create() documentation for specification. */
        MemoryAllocator(const Anvil::BaseDevice*                 in_device_ptr,
                        std::shared_ptr<IMemoryAllocatorBackend> in_backend_ptr,
                        bool                                     in_mt_safe);

        MemoryAllocator           (const MemoryAllocator&);
        MemoryAllocator& operator=(const MemoryAllocator&);

        /* Private members */
        std::shared_ptr<IMemoryAllocatorBackend> m_backend_ptr;
        const Anvil::BaseDevice*                 m_device_ptr;
        Items                                    m_items;
        std::map<const void*, bool>              m_per_object_pending_alloc_status;

        MemoryAllocatorBakeCallbackFunction                                m_post_bake_callback_function;
        MemoryAllocatorPostBakePerNonSparseBufferItemMemAssignmentCallback m_post_bake_per_buffer_item_mem_assignment_callback_function;
        MemoryAllocatorPostBakePerNonSparseImageItemMemAssignmentCallback  m_post_bake_per_image_item_mem_assignment_callback_function;
    };
};

#endif /* WRAPPERS_MEMORY_ALLOCATOR_H */