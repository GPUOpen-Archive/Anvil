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

/* Defines a MemoryAllocator class which allocates & maintains a memory block for all registered
 * objects. At baking time, non-overlapping regions of memory storage are distributed to the objects,
 * respect to object-specific alignment requirements.
 *
 * The allocator uses a single memory heap for all allocations, so it may not work in all cases.
 * This will be improved at some point in the future.
 **/
#ifndef MISC_MEMORY_ALLOCATOR_H
#define MISC_MEMORY_ALLOCATOR_H

#include "misc/debug.h"
#include "misc/types.h"
#include <vector>


namespace Anvil
{
    typedef void (*PFNMEMORYALLOCATORBAKECALLBACKPROC)(Anvil::MemoryAllocator* in_memory_allocator_ptr,
                                                       void*                   in_user_arg);

    /** Implements a simple memory allocator. For more details, please see the header. */
    class MemoryAllocator : public std::enable_shared_from_this<MemoryAllocator>
    {
    public:
        /* Public type definitions */
        typedef enum
        {
            ITEM_TYPE_BUFFER,
            ITEM_TYPE_IMAGE_WHOLE,

            ITEM_TYPE_SPARSE_IMAGE_MIPTAIL,
            ITEM_TYPE_SPARSE_IMAGE_SUBRESOURCE,
        } ItemType;

        typedef struct Item
        {
            std::shared_ptr<Anvil::Buffer>               buffer_ptr;
            std::shared_ptr<float>                       buffer_ref_float_data_ptr;
            std::shared_ptr<std::vector<float> >         buffer_ref_float_vector_data_ptr;
            std::shared_ptr<unsigned char>               buffer_ref_uchar8_data_ptr;
            std::shared_ptr<std::vector<unsigned char> > buffer_ref_uchar8_vector_data_ptr;
            std::shared_ptr<uint32_t>                    buffer_ref_uint32_data_ptr;
            std::shared_ptr<std::vector<uint32_t> >      buffer_ref_uint32_vector_data_ptr;
            std::shared_ptr<Anvil::Image>                image_ptr;

            std::shared_ptr<Anvil::MemoryAllocator> memory_allocator_ptr;

            ItemType type;

            std::shared_ptr<Anvil::MemoryBlock> alloc_memory_block_ptr;
            uint32_t                            alloc_memory_final_type;
            VkDeviceSize                        alloc_memory_required_alignment;
            MemoryFeatureFlags                  alloc_memory_required_features;
            uint32_t                            alloc_memory_supported_memory_types;
            uint32_t                            alloc_memory_types;
            VkDeviceSize                        alloc_size;

            VkExtent3D         extent;
            bool               is_baked;
            VkDeviceSize       miptail_offset;
            uint32_t           n_layer;
            VkOffset3D         offset;
            VkImageSubresource subresource;

            Item(std::shared_ptr<Anvil::MemoryAllocator> in_memory_allocator_ptr,
                 std::shared_ptr<Anvil::Buffer>          in_buffer_ptr,
                 VkDeviceSize                            in_alloc_size,
                 uint32_t                                in_alloc_memory_types,
                 VkDeviceSize                            in_alloc_alignment,
                 MemoryFeatureFlags                      in_alloc_required_memory_features,
                 uint32_t                                in_alloc_supported_memory_types);

            Item(std::shared_ptr<Anvil::MemoryAllocator> in_memory_allocator_ptr,
                 std::shared_ptr<Anvil::Image>           in_image_ptr,
                 uint32_t                                in_n_layer,
                 VkDeviceSize                            in_alloc_size,
                 uint32_t                                in_alloc_memory_types,
                 VkDeviceSize                            in_miptail_offset,
                 VkDeviceSize                            in_alloc_alignment,
                 MemoryFeatureFlags                      in_alloc_required_memory_features,
                uint32_t                                 in_alloc_supported_memory_types);

            Item(std::shared_ptr<Anvil::MemoryAllocator> in_memory_allocator_ptr,
                 std::shared_ptr<Anvil::Image>           in_image_ptr,
                 const VkImageSubresource&               in_subresource,
                 const VkOffset3D&                       in_offset,
                 const VkExtent3D&                       in_extent,
                 VkDeviceSize                            in_alloc_size,
                 uint32_t                                in_alloc_memory_types,
                 VkDeviceSize                            in_alloc_alignment,
                 MemoryFeatureFlags                      in_alloc_required_memory_features,
                 uint32_t                                in_alloc_supported_memory_types);

            Item(std::shared_ptr<Anvil::MemoryAllocator> in_memory_allocator_ptr,
                 std::shared_ptr<Anvil::Image>           in_image_ptr,
                 VkDeviceSize                            in_alloc_size,
                 uint32_t                                in_alloc_memory_types,
                 VkDeviceSize                            in_alloc_alignment,
                 MemoryFeatureFlags                      in_alloc_required_memory_features,
                 uint32_t                                in_alloc_supported_memory_types);

            Item& operator=(const Item&);
            Item           (const Item&);

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

        typedef std::vector<std::shared_ptr<Item> > Items;

        class IMemoryAllocatorBackend
        {
        public:
            virtual ~IMemoryAllocatorBackend()
            {
                /* Stub */
            }

            virtual bool bake           (Items& in_items)       = 0;
            virtual bool supports_baking()                const = 0;
        };

        /* Public functions */

        /** Adds a new Buffer object which should use storage coming from the buffer memory
         *  maintained by the Memory Allocator.
         *
         *  @param in_buffer_ptr               Buffer to configure storage for at bake() call time. Must not
         *                                     be nullptr.
         *  @param in_data_ptr                 The buffer will be filled with data extracted from the specified
         *                                     location. The number of bytes which will be stored is defined by
         *                                     buffer size.
         *  @param in_data_vector_ptr          The buffer will be filled with data extracted from the specified
         *                                     vector. Total number of bytes defined in the vector must match
         *                                     buffer size.
         *  @param in_required_memory_features Memory features the assigned memory must support.
         *                                     See MemoryFeatureFlagBits for more details.
         *
         *  @return true if the buffer has been successfully scheduled for baking, false otherwise.
         **/
        bool add_buffer                                            (std::shared_ptr<Anvil::Buffer>               in_buffer_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features);
        bool add_buffer_with_float_data_ptr_based_post_fill        (std::shared_ptr<Anvil::Buffer>               in_buffer_ptr,
                                                                    std::shared_ptr<float>                       in_data_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features);
        bool add_buffer_with_float_data_vector_ptr_based_post_fill (std::shared_ptr<Anvil::Buffer>               in_buffer_ptr,
                                                                    std::shared_ptr<std::vector<float> >         in_data_vector_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features);
        bool add_buffer_with_uchar8_data_ptr_based_post_fill       (std::shared_ptr<Anvil::Buffer>               in_buffer_ptr,
                                                                    std::shared_ptr<unsigned char>               in_data_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features);
        bool add_buffer_with_uchar8_data_vector_ptr_based_post_fill(std::shared_ptr<Anvil::Buffer>               in_buffer_ptr,
                                                                    std::shared_ptr<std::vector<unsigned char> > in_data_vector_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features);
        bool add_buffer_with_uint32_data_ptr_based_post_fill       (std::shared_ptr<Anvil::Buffer>               in_buffer_ptr,
                                                                    std::shared_ptr<uint32_t>                    in_data_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features);
        bool add_buffer_with_uint32_data_vector_ptr_based_post_fill(std::shared_ptr<Anvil::Buffer>               in_buffer_ptr,
                                                                    std::shared_ptr<std::vector<uint32_t> >      in_data_vector_ptr,
                                                                    MemoryFeatureFlags                           in_required_memory_features);

        /** Adds an Image object which should be assigned storage coming from memory objects
         *  maintained by the Memory Allocator. At baking time, all subresources of the image,
         *  as well as all miptails (in case of resident images) will be assigned memory regions.
         *
         *  This function can be used against both non-sparse and sparse images.
         *
         *  @param image_ptr                   Image to configure storage for at bake() call time. Must not
         *                                     be nullptr.
         *  @param in_required_memory_features Memory features the assigned memory must support.
         *                                     See MemoryFeatureFlagBits for more details.
         *
         *  @return true if the image has been successfully scheduled for baking, false otherwise.
         **/
        bool add_image_whole(std::shared_ptr<Anvil::Image> in_image_ptr,
                             MemoryFeatureFlags            in_required_memory_features);

        /** Adds a new Image object whose layer @param in_n_layer 's miptail for @param in_aspect
         *  aspect should be assigned a physical memory backing. The miptail will be bound a memory
         *  region at baking time.
         *
         *  If the image needs to be assigned just a single miptail, @param in_n_layer should be
         *  set to 0.
         *
         *  This function can only be used for sparse resident images.
         *
         *  @param in_image_ptr                Image to use for the request. Must not be null.
         *  @param in_aspect                   Aspect to be used for the request.
         *  @param in_n_layer                  Index of the layer to attach the miptail to.
         *  @param in_required_memory_features Memory features the assigned memory must support.
         *                                     See MemoryFeatureFlagBits for more details.
         *
         *  @return true if the miptail has been successfully scheduled for baking, false otherwise.
         */
        bool add_sparse_image_miptail(std::shared_ptr<Anvil::Image> in_image_ptr,
                                      VkImageAspectFlagBits         in_aspect,
                                      uint32_t                      in_n_layer,
                                      MemoryFeatureFlags            in_required_memory_features);

        /** Adds a single subresource which should be assigned memory backing.
         *
         *  This function does NOT alloc memory for the miptail. It is user's responsibilty to call
         *  add_sparse_image_miptail() for any layers which require a miptail.
         *
         *  @param in_image_ptr                Image to use for the request. Must not be null.
         *  @param in_subresource              Specifies details of the subresource to attach memory backing to.
         *  @param in_offset                   XYZ offset, from which the memory backing should be attached. Must
         *                                     be rounded up to tile size of the aspect @param in_subresource
         *                                     refers to.
         *  @param in_extent                   Size of the region to assign memory backing to. Must be rounded up
         *                                     to tile size of the aspect @param in_subresource refers to, UNLESS
         *                                     @param in_offset + @param in_extent touches the subresource border.
         *  @param in_required_memory_features Memory features the assigned memory must support.
         *                                     See MemoryFeatureFlagBits for more details.
         *
         *  @return true if the subresource has been successfully scheduled for baking, false otherwise.
         **/
        bool add_sparse_image_subresource(std::shared_ptr<Anvil::Image> in_image_ptr,
                                          const VkImageSubresource&     in_subresource,
                                          const VkOffset3D&             in_offset,
                                          VkExtent3D                    in_extent,
                                          MemoryFeatureFlags            in_required_memory_features);

        /** TODO */
        bool bake();

        /** Creates a new one-shot memory allocator instance.
         *
         *  This type of allocator only supports a single explicit (or implicit) bake invocation.
         *
         *  @param in_device_ptr Device to use.
         **/
        static std::shared_ptr<MemoryAllocator> create_oneshot(std::weak_ptr<Anvil::BaseDevice> in_device_ptr);

        /** Creates a new VMA memory allocator instance.
         *
         *  This type of allocator supports an arbitrary number of implicit or explicit bake invocations.
         *
         *  @param in_device_ptr Device to use.
         **/
        static std::shared_ptr<MemoryAllocator> create_vma(std::weak_ptr<Anvil::BaseDevice> in_device_ptr);

        /** Assigns a func pointer which will be called by the allocator after all added objects
         *  have been assigned memory blocks.
         *
         *  Calling this function more than once for the same MemoryAllocator instance will trigger
         *  an assertion failure.
         *
         *  @param in_pfn_post_bake_callback_ptr Function pointer to assign. Must not be null.
         *  @param in_callback_user_arg          User argument to pass with the callback. Can be null.
         *
         */
        void set_post_bake_callback(PFNMEMORYALLOCATORBAKECALLBACKPROC in_pfn_post_bake_callback,
                                    void*                              in_callback_user_arg);

         /** Destructor.
          *
          *  Releases the underlying MemoryBlock instance
          **/
        ~MemoryAllocator();

    private:

        /* Private functions */
        bool add_buffer_internal(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                 MemoryFeatureFlags             in_required_memory_features);
        bool is_alloc_supported (uint32_t                       in_memory_types,
                                 Anvil::MemoryFeatureFlags      in_memory_features,
                                 uint32_t*                      out_opt_filtered_memory_types_ptr) const;

        static void on_is_alloc_pending_for_buffer_query(void* in_callback_arg,
                                                         void* in_user_arg);
        static void on_is_alloc_pending_for_image_query (void* in_callback_arg,
                                                         void* in_user_arg);
        static void on_implicit_bake_needed             (void* in_callback_arg,
                                                         void* in_user_arg);

        /** Constructor.
         *
         *  Please see create() documentation for specification. */
        MemoryAllocator(std::weak_ptr<Anvil::BaseDevice>         in_device_ptr,
                        std::shared_ptr<IMemoryAllocatorBackend> in_backend_ptr);

        MemoryAllocator           (const MemoryAllocator&);
        MemoryAllocator& operator=(const MemoryAllocator&);

        /* Private members */
        std::shared_ptr<IMemoryAllocatorBackend> m_backend_ptr;
        std::weak_ptr<Anvil::BaseDevice>         m_device_ptr;
        Items                                    m_items;
        std::map<const void*, bool>              m_per_object_pending_alloc_status;

        PFNMEMORYALLOCATORBAKECALLBACKPROC m_pfn_post_bake_callback_ptr;
        void*                              m_post_bake_callback_user_arg;
    };
};

#endif /* WRAPPERS_MEMORY_ALLOCATOR_H */