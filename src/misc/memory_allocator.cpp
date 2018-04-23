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

#include "misc/buffer_create_info.h"
#include "misc/fence_create_info.h"
#include "misc/formats.h"
#include "misc/image_create_info.h"
#include "misc/memory_allocator.h"
#include "misc/memalloc_backends/backend_oneshot.h"
#include "misc/memalloc_backends/backend_vma.h"
#include "wrappers/buffer.h"
#include "wrappers/device.h"
#include "wrappers/fence.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include "wrappers/queue.h"

/* Please see header for specification */
Anvil::MemoryAllocator::Item::Item(Anvil::MemoryAllocator*             in_memory_allocator_ptr,
                                   Anvil::Buffer*                      in_buffer_ptr,
                                   VkDeviceSize                        in_alloc_size,
                                   uint32_t                            in_alloc_memory_types,
                                   VkDeviceSize                        in_alloc_alignment,
                                   MemoryFeatureFlags                  in_alloc_required_memory_features,
                                   uint32_t                            in_alloc_supported_memory_types,
                                   Anvil::ExternalMemoryHandleTypeBits in_alloc_external_memory_handle_types)
{
    anvil_assert(in_alloc_supported_memory_types != 0);
    anvil_assert(in_memory_allocator_ptr         != nullptr);

    alloc_external_memory_handle_types  = in_alloc_external_memory_handle_types;
    alloc_memory_final_type             = UINT32_MAX;
    alloc_memory_required_alignment     = in_alloc_alignment;
    alloc_memory_required_features      = in_alloc_required_memory_features;
    alloc_memory_supported_memory_types = in_alloc_supported_memory_types;
    alloc_memory_types                  = in_alloc_memory_types;
    alloc_size                          = in_alloc_size;
    buffer_ptr                          = in_buffer_ptr;
    image_ptr                           = nullptr;
    is_baked                            = false;
    memory_allocator_ptr                = in_memory_allocator_ptr;
    type                                = ITEM_TYPE_BUFFER;

    register_for_callbacks();
}

Anvil::MemoryAllocator::Item::Item(Anvil::MemoryAllocator*             in_memory_allocator_ptr,
                                   Anvil::Buffer*                      in_buffer_ptr,
                                   VkDeviceSize                        in_alloc_offset,
                                   VkDeviceSize                        in_alloc_size,
                                   uint32_t                            in_alloc_memory_types,
                                   VkDeviceSize                        in_alloc_alignment,
                                   MemoryFeatureFlags                  in_alloc_required_memory_features,
                                   uint32_t                            in_alloc_supported_memory_types,
                                   Anvil::ExternalMemoryHandleTypeBits in_alloc_external_memory_handle_types)
{
    anvil_assert(in_alloc_supported_memory_types != 0);
    anvil_assert(in_memory_allocator_ptr         != nullptr);

    alloc_external_memory_handle_types  = in_alloc_external_memory_handle_types;
    alloc_memory_final_type             = UINT32_MAX;
    alloc_memory_required_alignment     = in_alloc_alignment;
    alloc_memory_required_features      = in_alloc_required_memory_features;
    alloc_memory_supported_memory_types = in_alloc_supported_memory_types;
    alloc_memory_types                  = in_alloc_memory_types;
    alloc_offset                        = in_alloc_offset;
    alloc_size                          = in_alloc_size;
    buffer_ptr                          = in_buffer_ptr;
    image_ptr                           = nullptr;
    is_baked                            = false;
    memory_allocator_ptr                = in_memory_allocator_ptr;
    type                                = ITEM_TYPE_SPARSE_BUFFER_REGION;

    register_for_callbacks();
}

/* Please see header for specification */
Anvil::MemoryAllocator::Item::Item(Anvil::MemoryAllocator*             in_memory_allocator_ptr,
                                   Anvil::Image*                       in_image_ptr,
                                   uint32_t                            in_n_layer,
                                   VkDeviceSize                        in_alloc_size,
                                   uint32_t                            in_alloc_memory_types,
                                   VkDeviceSize                        in_miptail_offset,
                                   VkDeviceSize                        in_alloc_alignment,
                                   MemoryFeatureFlags                  in_alloc_required_memory_features,
                                   uint32_t                            in_alloc_supported_memory_types,
                                   Anvil::ExternalMemoryHandleTypeBits in_alloc_external_memory_handle_types)
{
    anvil_assert(in_alloc_supported_memory_types != 0);
    anvil_assert(in_memory_allocator_ptr         != nullptr);

    alloc_external_memory_handle_types  = in_alloc_external_memory_handle_types;
    alloc_memory_final_type             = UINT32_MAX;
    alloc_memory_required_alignment     = in_alloc_alignment;
    alloc_memory_required_features      = in_alloc_required_memory_features;
    alloc_memory_supported_memory_types = in_alloc_supported_memory_types;
    alloc_memory_types                  = in_alloc_memory_types;
    alloc_offset                        = UINT64_MAX;
    alloc_size                          = in_alloc_size;
    buffer_ptr                          = nullptr;
    image_ptr                           = in_image_ptr;
    is_baked                            = false;
    memory_allocator_ptr                = in_memory_allocator_ptr;
    miptail_offset                      = in_miptail_offset;
    n_layer                             = in_n_layer;
    type                                = ITEM_TYPE_SPARSE_IMAGE_MIPTAIL;

    register_for_callbacks();
}

/* Please see header for specification */
Anvil::MemoryAllocator::Item::Item(Anvil::MemoryAllocator*             in_memory_allocator_ptr,
                                   Anvil::Image*                       in_image_ptr,
                                   const VkImageSubresource&           in_subresource,
                                   const VkOffset3D&                   in_offset,
                                   const VkExtent3D&                   in_extent,
                                   VkDeviceSize                        in_alloc_size,
                                   uint32_t                            in_alloc_memory_types,
                                   VkDeviceSize                        in_alloc_alignment,
                                   MemoryFeatureFlags                  in_alloc_required_memory_features,
                                   uint32_t                            in_alloc_supported_memory_types,
                                   Anvil::ExternalMemoryHandleTypeBits in_alloc_external_memory_handle_types)
{
    anvil_assert(in_alloc_supported_memory_types != 0);
    anvil_assert(in_memory_allocator_ptr         != nullptr);

    alloc_external_memory_handle_types  = in_alloc_external_memory_handle_types;
    alloc_memory_final_type             = UINT32_MAX;
    alloc_memory_types                  = in_alloc_memory_types;
    alloc_memory_required_alignment     = in_alloc_alignment;
    alloc_memory_required_features      = in_alloc_required_memory_features;
    alloc_memory_supported_memory_types = in_alloc_supported_memory_types;
    alloc_offset                        = UINT64_MAX;
    alloc_size                          = in_alloc_size;
    buffer_ptr                          = nullptr;
    extent                              = in_extent;
    image_ptr                           = in_image_ptr;
    is_baked                            = false;
    memory_allocator_ptr                = in_memory_allocator_ptr;
    offset                              = in_offset;
    subresource                         = in_subresource;
    type                                = ITEM_TYPE_SPARSE_IMAGE_SUBRESOURCE;

    register_for_callbacks();
}

/* Please see header for specification */
Anvil::MemoryAllocator::Item::Item(Anvil::MemoryAllocator*             in_memory_allocator_ptr,
                                   Anvil::Image*                       in_image_ptr,
                                   VkDeviceSize                        in_alloc_size,
                                   uint32_t                            in_alloc_memory_types,
                                   VkDeviceSize                        in_alloc_alignment,
                                   MemoryFeatureFlags                  in_alloc_required_memory_features,
                                   uint32_t                            in_alloc_supported_memory_types,
                                   Anvil::ExternalMemoryHandleTypeBits in_alloc_external_memory_handle_types)
{
    anvil_assert(in_alloc_supported_memory_types != 0);
    anvil_assert(in_memory_allocator_ptr         != nullptr);

    alloc_external_memory_handle_types  = in_alloc_external_memory_handle_types;
    alloc_memory_final_type             = UINT32_MAX;
    alloc_memory_required_alignment     = in_alloc_alignment;
    alloc_memory_required_features      = in_alloc_required_memory_features;
    alloc_memory_supported_memory_types = in_alloc_supported_memory_types;
    alloc_memory_types                  = in_alloc_memory_types;
    alloc_offset                        = UINT64_MAX;
    alloc_size                          = in_alloc_size;
    buffer_ptr                          = nullptr;
    image_ptr                           = in_image_ptr;
    is_baked                            = false;
    memory_allocator_ptr                = in_memory_allocator_ptr;
    type                                = ITEM_TYPE_IMAGE_WHOLE;

    register_for_callbacks();
}

/* Please see header for specification */
Anvil::MemoryAllocator::Item::~Item()
{
    unregister_from_callbacks();
}

/* Please see header for specification */
void Anvil::MemoryAllocator::Item::register_for_callbacks()
{
    auto on_implicit_bake_needed_callback_func              = std::bind(&Anvil::MemoryAllocator::on_implicit_bake_needed,
                                                                        memory_allocator_ptr);
    auto on_is_alloc_pending_for_buffer_query_callback_func = std::bind(&Anvil::MemoryAllocator::on_is_alloc_pending_for_buffer_query,
                                                                        memory_allocator_ptr,
                                                                        std::placeholders::_1);
    auto on_is_alloc_pending_for_image_query_callback_func  = std::bind(&Anvil::MemoryAllocator::on_is_alloc_pending_for_image_query,
                                                                        memory_allocator_ptr,
                                                                        std::placeholders::_1);

    if (buffer_ptr != nullptr)
    {
        /* Sign up for "is alloc pending" callback in order to support sparsely bound/sparse buffers */
        if (!buffer_ptr->is_callback_registered(BUFFER_CALLBACK_ID_IS_ALLOC_PENDING,
                                                on_is_alloc_pending_for_buffer_query_callback_func,
                                                buffer_ptr))
        {
            buffer_ptr->register_for_callbacks(BUFFER_CALLBACK_ID_IS_ALLOC_PENDING,
                                               on_is_alloc_pending_for_buffer_query_callback_func,
                                               buffer_ptr);

            buffer_has_is_alloc_pending_callback_registered = true;
        }
        else
        {
            buffer_has_is_alloc_pending_callback_registered = false;
        }

        /* Sign up for "memory needed" callback so that we can trigger an implicit bake operation */
        if (!buffer_ptr->is_callback_registered(BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED,
                                                on_implicit_bake_needed_callback_func,
                                                buffer_ptr))
        {
            buffer_ptr->register_for_callbacks(BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED,
                                               on_implicit_bake_needed_callback_func,
                                               buffer_ptr);

            buffer_has_memory_block_needed_callback_registered = true;
        }
        else
        {
            buffer_has_memory_block_needed_callback_registered = false;
        }
    }

    if (image_ptr != nullptr)
    {
        /* Sign up for "is alloc pending" callback in order to support sparse images */
        if (!image_ptr->is_callback_registered(IMAGE_CALLBACK_ID_IS_ALLOC_PENDING,
                                               on_is_alloc_pending_for_image_query_callback_func,
                                               image_ptr))
        {
            image_ptr->register_for_callbacks(IMAGE_CALLBACK_ID_IS_ALLOC_PENDING,
                                              on_is_alloc_pending_for_image_query_callback_func,
                                              image_ptr);

            image_has_is_alloc_pending_callback_registered = true;
        }
        else
        {
            image_has_is_alloc_pending_callback_registered = false;
        }

        /* Sign up for "memory needed" callback so that we can trigger an implicit bake operation */
        if (!image_ptr->is_callback_registered(IMAGE_CALLBACK_ID_MEMORY_BLOCK_NEEDED,
                                               on_implicit_bake_needed_callback_func,
                                               image_ptr))
        {
            image_ptr->register_for_callbacks(IMAGE_CALLBACK_ID_MEMORY_BLOCK_NEEDED,
                                              on_implicit_bake_needed_callback_func,
                                              image_ptr);

            image_has_memory_block_needed_callback_registered = true;
        }
        else
        {
            image_has_memory_block_needed_callback_registered = false;
        }

    }
}

/* Please see header for specification */
void Anvil::MemoryAllocator::Item::unregister_from_callbacks()
{
    auto on_implicit_bake_needed_callback_func              = std::bind(&Anvil::MemoryAllocator::on_implicit_bake_needed,
                                                                        memory_allocator_ptr);
    auto on_is_alloc_pending_for_buffer_query_callback_func = std::bind(&Anvil::MemoryAllocator::on_is_alloc_pending_for_buffer_query,
                                                                        memory_allocator_ptr,
                                                                        std::placeholders::_1);
    auto on_is_alloc_pending_for_image_query_callback_func  = std::bind(&Anvil::MemoryAllocator::on_is_alloc_pending_for_image_query,
                                                                        memory_allocator_ptr,
                                                                        std::placeholders::_1);

    if (buffer_ptr != nullptr)
    {
        if (buffer_has_is_alloc_pending_callback_registered)
        {
            buffer_ptr->unregister_from_callbacks(BUFFER_CALLBACK_ID_IS_ALLOC_PENDING,
                                                  on_is_alloc_pending_for_buffer_query_callback_func,
                                                  buffer_ptr);
        }

        if (buffer_has_memory_block_needed_callback_registered)
        {
            buffer_ptr->unregister_from_callbacks(BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED,
                                                  on_implicit_bake_needed_callback_func,
                                                  buffer_ptr);
        }
    }

    if (image_ptr != nullptr)
    {
        if (image_has_is_alloc_pending_callback_registered)
        {
            image_ptr->unregister_from_callbacks(IMAGE_CALLBACK_ID_IS_ALLOC_PENDING,
                                                 on_is_alloc_pending_for_image_query_callback_func,
                                                 image_ptr);
        }

        if (image_has_memory_block_needed_callback_registered)
        {
            image_ptr->unregister_from_callbacks(IMAGE_CALLBACK_ID_MEMORY_BLOCK_NEEDED,
                                                 on_implicit_bake_needed_callback_func,
                                                 image_ptr);
        }
    }
}


/* Please see header for specification */
Anvil::MemoryAllocator::MemoryAllocator(const Anvil::BaseDevice*                 in_device_ptr,
                                        std::shared_ptr<IMemoryAllocatorBackend> in_backend_ptr,
                                        bool                                     in_mt_safe)
    :MTSafetySupportProvider(in_mt_safe),
     m_backend_ptr          (std::move(in_backend_ptr) ),
     m_device_ptr           (in_device_ptr)
{
    /* Stub */
}

/* Please see header for specification */
Anvil::MemoryAllocator::~MemoryAllocator()
{
    if (m_items.size()                   > 0 &&
        m_backend_ptr->supports_baking() )
    {
        bake();
    }
}


/** Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer(Anvil::Buffer*                      in_buffer_ptr,
                                        MemoryFeatureFlags                  in_required_memory_features,
                                        Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    return add_buffer_internal(in_buffer_ptr,
                               in_required_memory_features,
                               in_external_memory_handle_types);
}

/** Determines the amount of memory, supported memory type and required alignment for the specified
 *  buffer, and caches all this data in the m_items for further processing at baking time.
 *
 *  @param buffer_ptr Buffer instance to assign a memory block at baking time.
 **/
bool Anvil::MemoryAllocator::add_buffer_internal(Anvil::Buffer*                      in_buffer_ptr,
                                                 MemoryFeatureFlags                  in_required_memory_features,
                                                 Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    IMemoryAllocatorBackend*   backend_interface_ptr = dynamic_cast<IMemoryAllocatorBackend*>(m_backend_ptr.get() );
    VkDeviceSize               buffer_alignment      = 0;
    uint32_t                   buffer_memory_types   = 0;
    VkDeviceSize               buffer_storage_size   = 0;
    uint32_t                   filtered_memory_types = 0;
    const VkMemoryRequirements memory_reqs           = in_buffer_ptr->get_memory_requirements();
    std::unique_ptr<Item>      new_item_ptr;
    bool                       result                = true;

    ANVIL_REDUNDANT_VARIABLE(backend_interface_ptr);

    /* Sanity checks */
    anvil_assert(backend_interface_ptr->supports_baking() );
    anvil_assert(in_buffer_ptr                            != nullptr);

    /* Extract external memory handle types from the specified buffer, if none were specified. */
    if (in_external_memory_handle_types == 0)
    {
        in_external_memory_handle_types = in_buffer_ptr->get_create_info_ptr()->get_external_memory_handle_types();
    }

    if (!do_external_memory_handle_type_sanity_checks(in_external_memory_handle_types) )
    {
        result = false;

        goto end;
    }

    /* Determine how much space we're going to need, what alignment we need
     * to consider, and so on. */
    buffer_alignment    = memory_reqs.alignment;
    buffer_memory_types = memory_reqs.memoryTypeBits;
    buffer_storage_size = memory_reqs.size;

    if (!is_alloc_supported(buffer_memory_types,
                            in_required_memory_features,
                           &filtered_memory_types) )
    {
        result = false;

        goto end;
    }

    /* Store a new block item descriptor. */
    new_item_ptr.reset(
        new Item(this,
                 in_buffer_ptr,
                 buffer_storage_size,
                 buffer_memory_types,
                 buffer_alignment,
                 in_required_memory_features,
                 filtered_memory_types,
                 in_external_memory_handle_types)
    );

    m_items.push_back(
        std::move(new_item_ptr)
    );

    m_per_object_pending_alloc_status[in_buffer_ptr] = true;

end:
    anvil_assert(result);

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_float_data_ptr_based_post_fill(Anvil::Buffer*                      in_buffer_ptr,
                                                                            std::unique_ptr<float[]>            in_data_ptr,
                                                                            MemoryFeatureFlags                  in_required_memory_features,
                                                                            Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();
    bool                                   result;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    result = add_buffer_internal(in_buffer_ptr,
                                 in_required_memory_features,
                                 in_external_memory_handle_types);

    if (result)
    {
        m_items.back()->buffer_ref_float_data_ptr = std::move(in_data_ptr);
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_float_data_vector_ptr_based_post_fill(Anvil::Buffer*                       in_buffer_ptr,
                                                                                   std::unique_ptr<std::vector<float> > in_data_vector_ptr,
                                                                                   MemoryFeatureFlags                   in_required_memory_features,
                                                                                   Anvil::ExternalMemoryHandleTypeBits  in_external_memory_handle_types)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();
    bool                                   result;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    anvil_assert(in_data_vector_ptr->size() * sizeof(float) == in_buffer_ptr->get_create_info_ptr()->get_size() );

    result = add_buffer_internal(in_buffer_ptr,
                                 in_required_memory_features,
                                 in_external_memory_handle_types);

    if (result)
    {
        m_items.back()->buffer_ref_float_vector_data_ptr = std::move(in_data_vector_ptr);
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_float_data_vector_ptr_based_post_fill(Anvil::Buffer*                      in_buffer_ptr,
                                                                                   const std::vector<float>*           in_data_vector_ptr,
                                                                                   MemoryFeatureFlags                  in_required_memory_features,
                                                                                   Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();
    auto                                   ptr        = std::unique_ptr<std::vector<float>, std::function<void (std::vector<float>*) > >(const_cast<std::vector<float>* >(in_data_vector_ptr),
                                                                                                                                         [](const std::vector<float>*)
                                                                                                                                         {
                                                                                                                                             /* Stub */
                                                                                                                                         });
    bool                                   result;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    anvil_assert(in_data_vector_ptr->size() * sizeof(uint32_t) == in_buffer_ptr->get_create_info_ptr()->get_size() );

    result = add_buffer_internal(in_buffer_ptr,
                                 in_required_memory_features,
                                 in_external_memory_handle_types);

    if (result)
    {
        m_items.back()->buffer_ref_float_vector_data_ptr = std::move(ptr);
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_uchar8_data_ptr_based_post_fill(Anvil::Buffer*                      in_buffer_ptr,
                                                                             std::unique_ptr<uint8_t[]>          in_data_ptr,
                                                                             MemoryFeatureFlags                  in_required_memory_features,
                                                                             Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();
    bool                                   result;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    result = add_buffer_internal(in_buffer_ptr,
                                 in_required_memory_features,
                                 in_external_memory_handle_types);

    if (result)
    {
        m_items.back()->buffer_ref_uchar8_data_ptr = std::move(in_data_ptr);
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_uchar8_data_vector_ptr_based_post_fill(Anvil::Buffer*                               in_buffer_ptr,
                                                                                    std::unique_ptr<std::vector<unsigned char> > in_data_vector_ptr,
                                                                                    MemoryFeatureFlags                           in_required_memory_features,
                                                                                    Anvil::ExternalMemoryHandleTypeBits          in_external_memory_handle_types)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();
    bool                                   result;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    anvil_assert(in_data_vector_ptr->size() == in_buffer_ptr->get_create_info_ptr()->get_size() );

    result = add_buffer_internal(in_buffer_ptr,
                                 in_required_memory_features,
                                 in_external_memory_handle_types);

    if (result)
    {
        m_items.back()->buffer_ref_uchar8_vector_data_ptr = std::move(in_data_vector_ptr);
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_uint32_data_ptr_based_post_fill(Anvil::Buffer*                      in_buffer_ptr,
                                                                             std::unique_ptr<uint32_t[]>         in_data_ptr,
                                                                             MemoryFeatureFlags                  in_required_memory_features,
                                                                             Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();
    bool                                   result;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    result = add_buffer_internal(in_buffer_ptr,
                                 in_required_memory_features,
                                 in_external_memory_handle_types);

    if (result)
    {
        m_items.back()->buffer_ref_uint32_data_ptr = std::move(in_data_ptr);
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_uint32_data_vector_ptr_based_post_fill(Anvil::Buffer*                          in_buffer_ptr,
                                                                                    std::unique_ptr<std::vector<uint32_t> > in_data_vector_ptr,
                                                                                    MemoryFeatureFlags                      in_required_memory_features,
                                                                                    Anvil::ExternalMemoryHandleTypeBits     in_external_memory_handle_types)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();
    bool                                   result;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    anvil_assert(in_data_vector_ptr->size() * sizeof(uint32_t) == in_buffer_ptr->get_create_info_ptr()->get_size() );

    result = add_buffer_internal(in_buffer_ptr,
                                 in_required_memory_features,
                                 in_external_memory_handle_types);

    if (result)
    {
        m_items.back()->buffer_ref_uint32_vector_data_ptr = std::move(in_data_vector_ptr);
    }

    return result;
}

/** Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_uint32_data_vector_ptr_based_post_fill(Anvil::Buffer*                      in_buffer_ptr,
                                                                                    const std::vector<uint32_t>*        in_data_vector_ptr,
                                                                                    MemoryFeatureFlags                  in_required_memory_features,
                                                                                    Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr  = get_mutex();
    auto                                   ptr        = std::unique_ptr<std::vector<uint32_t>, std::function<void (std::vector<uint32_t>*) > >(const_cast<std::vector<uint32_t>* >(in_data_vector_ptr),
                                                                                                                                              [](const std::vector<uint32_t>*)
                                                                                                                                              {
                                                                                                                                                  /* Stub */
                                                                                                                                              });
    bool                                   result;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    anvil_assert(in_data_vector_ptr->size() * sizeof(uint32_t) == in_buffer_ptr->get_create_info_ptr()->get_size() );

    result = add_buffer_internal(in_buffer_ptr,
                                 in_required_memory_features,
                                 in_external_memory_handle_types);

    if (result)
    {
        m_items.back()->buffer_ref_uint32_vector_data_ptr = std::move(ptr);
    }

    return result;
}

/** Please see header for specification */
bool Anvil::MemoryAllocator::add_image_whole(Anvil::Image*                       in_image_ptr,
                                             MemoryFeatureFlags                  in_required_memory_features,
                                             Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    uint32_t                               filtered_memory_types = 0;
    VkDeviceSize                           image_alignment       = 0;
    uint32_t                               image_memory_types    = 0;
    VkDeviceSize                           image_storage_size    = 0;
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr             = get_mutex();
    std::unique_ptr<Item>                  new_item_ptr;
    bool                                   result                = true;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    /* Sanity checks */
    anvil_assert(m_backend_ptr->supports_baking() );
    anvil_assert(in_image_ptr                     != nullptr);

    /* Extract external memory handle types from the specified image, if none were specified. */
    if (in_external_memory_handle_types == 0)
    {
        in_external_memory_handle_types = in_image_ptr->get_create_info_ptr()->get_external_memory_handle_types();
    }

    if (!do_external_memory_handle_type_sanity_checks(in_external_memory_handle_types) )
    {
        result = false;

        goto end;
    }

    /* Determine how much size is needed for the image's storage, as well as what
     * the allocation requirements are */
    image_alignment    = in_image_ptr->get_image_alignment();
    image_memory_types = in_image_ptr->get_image_memory_types();
    image_storage_size = in_image_ptr->get_image_storage_size();

    if (!is_alloc_supported(image_memory_types,
                            in_required_memory_features,
                           &filtered_memory_types) )
    {
        result = false;

        goto end;
    }

    /* Store a new block item descriptor */
    new_item_ptr.reset(
        new Item(this,
                 in_image_ptr,
                 image_storage_size,
                 image_memory_types,
                 image_alignment,
                 in_required_memory_features,
                 filtered_memory_types,
                 in_external_memory_handle_types)
    );

    m_items.push_back(
        std::move(new_item_ptr)
    );

    m_per_object_pending_alloc_status[in_image_ptr] = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::MemoryAllocator::add_sparse_buffer_region(Anvil::Buffer*                      in_buffer_ptr,
                                                      VkDeviceSize                        in_offset,
                                                      VkDeviceSize                        in_size,
                                                      MemoryFeatureFlags                  in_required_memory_features,
                                                      Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    uint32_t                               filtered_memory_types = 0;
    const auto&                            memory_reqs           = in_buffer_ptr->get_memory_requirements();
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr             = get_mutex();
    std::unique_ptr<Item>                  new_item_ptr;
    bool                                   result                = true;

    /* Sanity checks */
    anvil_assert(in_buffer_ptr                                    != nullptr);
    anvil_assert(m_backend_ptr->supports_baking                () );
    anvil_assert(in_buffer_ptr->get_create_info_ptr()->get_type() == Anvil::BufferType::SPARSE_NO_ALLOC);
    anvil_assert(in_buffer_ptr->get_memory_requirements().size    >= in_offset + in_size);

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    /* Extract external memory handle types from the specified buffer, if none were specified. */
    if (in_external_memory_handle_types == 0)
    {
        in_external_memory_handle_types = in_buffer_ptr->get_create_info_ptr()->get_external_memory_handle_types();
    }

    if (!do_external_memory_handle_type_sanity_checks(in_external_memory_handle_types) )
    {
        result = false;

        goto end;
    }

    /* Determine how much space we're going to need, what alignment we need
     * to consider, and so on. */
    anvil_assert((in_offset % memory_reqs.alignment) == 0);

    if (!is_alloc_supported(memory_reqs.memoryTypeBits,
                            in_required_memory_features,
                           &filtered_memory_types) )
    {
        result = false;

        goto end;
    }

    /* Store a new block item descriptor. */
    new_item_ptr.reset(
        new Item(this,
                 in_buffer_ptr,
                 in_offset,
                 in_size,
                 memory_reqs.memoryTypeBits,
                 memory_reqs.alignment,
                 in_required_memory_features,
                 filtered_memory_types,
                 in_external_memory_handle_types)
    );

    m_items.push_back(
        std::move(new_item_ptr)
    );

    m_per_object_pending_alloc_status[in_buffer_ptr] = true;

end:
    anvil_assert(result);

    return result;
}

/** Please see header for specification */
bool Anvil::MemoryAllocator::add_sparse_image_miptail(Anvil::Image*                       in_image_ptr,
                                                      VkImageAspectFlagBits               in_aspect,
                                                      uint32_t                            in_n_layer,
                                                      MemoryFeatureFlags                  in_required_memory_features,
                                                      Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    const Anvil::SparseImageAspectProperties* aspect_props_ptr      = nullptr;
    uint32_t                                  filtered_memory_types = 0;
    std::unique_lock<std::recursive_mutex>    mutex_lock;
    auto                                      mutex_ptr             = get_mutex();
    std::unique_ptr<Item>                     new_item_ptr;
    uint32_t                                  miptail_memory_types  = 0;
    VkDeviceSize                              miptail_offset        = static_cast<VkDeviceSize>(UINT64_MAX);
    VkDeviceSize                              miptail_size          = 0;
    bool                                      result                = true;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Sanity checks */
    anvil_assert(in_image_ptr                                                        != nullptr);
    anvil_assert(m_backend_ptr->supports_baking                          () );
    anvil_assert(in_image_ptr->get_create_info_ptr()->get_residency_scope()          != Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED);
    anvil_assert(in_image_ptr->get_create_info_ptr()->get_n_layers       ()          >  in_n_layer);
    anvil_assert(in_image_ptr->has_aspects                               (in_aspect) );

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    /* Extract external memory handle types from the specified image, if none were specified. */
    if (in_external_memory_handle_types == 0)
    {
        in_external_memory_handle_types = in_image_ptr->get_create_info_ptr()->get_external_memory_handle_types();
    }

    if (!do_external_memory_handle_type_sanity_checks(in_external_memory_handle_types) )
    {
        result = false;

        goto end;
    }

    /* Extract aspect-specific properties which includes all the miptail data we're going to need */
    result = in_image_ptr->get_sparse_image_aspect_properties(in_aspect,
                                                             &aspect_props_ptr);
    anvil_assert(result);

    /* Even more sanity checks */
    anvil_assert((aspect_props_ptr->flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) != 0 &&
                  in_n_layer                                                           == 0 ||
                 (aspect_props_ptr->flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) == 0);

    /* Determine allocation properties */ 
    miptail_memory_types = in_image_ptr->get_image_memory_types();
    miptail_offset       = aspect_props_ptr->mip_tail_offset + aspect_props_ptr->mip_tail_stride * in_n_layer;
    miptail_size         = aspect_props_ptr->mip_tail_size;

    anvil_assert(miptail_size != 0);

    if (!is_alloc_supported(miptail_memory_types,
                            in_required_memory_features,
                           &filtered_memory_types) )
    {
        result = false;

        goto end;
    }

    /* Store a new block item descriptor */
    new_item_ptr.reset(
        new Item(this,
                 in_image_ptr,
                 in_n_layer,
                 miptail_size,
                 miptail_memory_types,
                 miptail_offset,
                 in_image_ptr->get_image_alignment(),
                 in_required_memory_features,
                 filtered_memory_types,
                 in_external_memory_handle_types)
    );

    m_items.push_back(
        std::move(new_item_ptr)
    );

    m_per_object_pending_alloc_status[in_image_ptr] = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_sparse_image_subresource(Anvil::Image*                       in_image_ptr,
                                                          const VkImageSubresource&           in_subresource,
                                                          const VkOffset3D&                   in_offset,
                                                          VkExtent3D                          in_extent,
                                                          MemoryFeatureFlags                  in_required_memory_features,
                                                          Anvil::ExternalMemoryHandleTypeBits in_external_memory_handle_types)
{
    const Anvil::SparseImageAspectProperties* aspect_props_ptr           = nullptr;
    uint32_t                                  component_size_bits[4]     = {0};
    uint32_t                                  filtered_memory_types      = 0;
    const auto                                image_format               = in_image_ptr->get_create_info_ptr()->get_format();
    uint32_t                                  mip_size[3];
    std::unique_lock<std::recursive_mutex>    mutex_lock;
    auto                                      mutex_ptr                  = get_mutex();
    std::unique_ptr<Item>                     new_item_ptr;
    bool                                      result                     = true;
    const VkDeviceSize                        tile_size                  = in_image_ptr->get_memory_requirements().alignment;
    VkDeviceSize                              total_region_size_in_bytes = 0;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Sanity checks */
    anvil_assert(in_image_ptr != nullptr);
    anvil_assert(m_backend_ptr->supports_baking                          () );
    anvil_assert(in_image_ptr->get_create_info_ptr()->get_residency_scope()                          != Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED);
    anvil_assert(in_image_ptr->has_aspects                               (in_subresource.aspectMask) );
    anvil_assert(in_image_ptr->get_n_mipmaps                             ()                          >  in_subresource.mipLevel);
    anvil_assert(in_image_ptr->get_create_info_ptr()->get_n_layers       ()                          >  in_subresource.arrayLayer);

    anvil_assert(in_extent.depth  >= 1);
    anvil_assert(in_extent.height >= 1);
    anvil_assert(in_extent.width  >= 1);

    anvil_assert((Anvil::Utils::is_pow2(static_cast<int32_t>(in_subresource.aspectMask)) != 0)); // only permit a single aspect

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    /* Extract external memory handle types from the specified image, if none were specified. */
    if (in_external_memory_handle_types == 0)
    {
        in_external_memory_handle_types = in_image_ptr->get_create_info_ptr()->get_external_memory_handle_types();
    }

    if (!do_external_memory_handle_type_sanity_checks(in_external_memory_handle_types) )
    {
        result = false;

        goto end;
    }

    /* Extract image properties needed for calculations below. */
    result = in_image_ptr->get_sparse_image_aspect_properties(static_cast<VkImageAspectFlagBits>(in_subresource.aspectMask),
                                                             &aspect_props_ptr);
    anvil_assert(result);

    result = in_image_ptr->get_image_mipmap_size(in_subresource.mipLevel,
                                                 mip_size + 0,
                                                 mip_size + 1,
                                                 mip_size + 2);
    anvil_assert(result);

    /* Even more sanity checks .. */
    anvil_assert(in_offset.x + in_extent.width  <= Anvil::Utils::round_up(mip_size[0], aspect_props_ptr->granularity.width) );
    anvil_assert(in_offset.y + in_extent.height <= Anvil::Utils::round_up(mip_size[1], aspect_props_ptr->granularity.height) );
    anvil_assert(in_offset.z + in_extent.depth  <= Anvil::Utils::round_up(mip_size[2], aspect_props_ptr->granularity.depth) );

    if (in_offset.x + in_extent.width != mip_size[0])
    {
        anvil_assert((in_offset.x     % aspect_props_ptr->granularity.width) == 0);
        anvil_assert((in_extent.width % aspect_props_ptr->granularity.width) == 0);
    }
    else
    {
        /* Image::set_memory_sparse() expects all subresources to be rounded up to tile size. */
        in_extent.width = Anvil::Utils::round_up(in_extent.width, aspect_props_ptr->granularity.width);
    }

    if (in_offset.y + in_extent.height != mip_size[1])
    {
        anvil_assert((in_offset.y      % aspect_props_ptr->granularity.height) == 0);
        anvil_assert((in_extent.height % aspect_props_ptr->granularity.height) == 0);
    }
    else
    {
        /* Image::set_memory_sparse() expects all subresources to be rounded up to tile size. */
        in_extent.height = Anvil::Utils::round_up(in_extent.height, aspect_props_ptr->granularity.height);
    }

    if (in_offset.z + in_extent.depth != mip_size[2])
    {
        anvil_assert((in_offset.z     % aspect_props_ptr->granularity.depth) == 0);
        anvil_assert((in_extent.depth % aspect_props_ptr->granularity.depth) == 0);
    }
    else
    {
        /* Image::set_memory_sparse() expects all subresources to be rounded up to tile size. */
        in_extent.depth = Anvil::Utils::round_up(in_extent.depth, aspect_props_ptr->granularity.depth);
    }

    /* Determine allocation properties */
    if (!Anvil::Formats::is_format_compressed(image_format))
    {
        Anvil::Formats::get_format_n_component_bits(image_format,
                                                    component_size_bits + 0,
                                                    component_size_bits + 1,
                                                    component_size_bits + 2,
                                                    component_size_bits + 3);

        anvil_assert(component_size_bits[0] != 0 ||
                     component_size_bits[1] != 0 ||
                     component_size_bits[2] != 0 ||
                     component_size_bits[3] != 0);
        anvil_assert(((component_size_bits[0] + component_size_bits[1] +
                       component_size_bits[2] + component_size_bits[3]) % 8) == 0);

        total_region_size_in_bytes = (component_size_bits[0] + component_size_bits[1] + component_size_bits[2] + component_size_bits[3]) / 8 /* bits in byte */
                                   * in_extent.width
                                   * in_extent.height
                                   * in_extent.depth;
    }
    else
    {
        uint32_t compressed_block_size[3] =
        {
            1,
            1,
            1
        };
        uint32_t n_bytes_per_block        = 0;

        if (!Anvil::Formats::get_compressed_format_block_size(image_format,
                                                              compressed_block_size,
                                                             &n_bytes_per_block) )
        {
            result = false;

            goto end;
        }
        else
        {
            anvil_assert(compressed_block_size[0] != 0 &&
                         compressed_block_size[1] != 0 &&
                         compressed_block_size[2] != 0);
            anvil_assert(n_bytes_per_block        != 0);

            anvil_assert( (in_extent.width  % compressed_block_size[0]) == 0);
            anvil_assert( (in_extent.height % compressed_block_size[1]) == 0);
            anvil_assert( (in_extent.depth  % compressed_block_size[2]) == 0);

            total_region_size_in_bytes = (in_extent.width  / compressed_block_size[0]) *
                                         (in_extent.height / compressed_block_size[1]) *
                                         (in_extent.depth  / compressed_block_size[2]) *
                                         n_bytes_per_block;
        }
    }

    /* The region size may be smaller than the required page size. Round it up if that's the case */
    total_region_size_in_bytes = Anvil::Utils::round_up(total_region_size_in_bytes,
                                                        tile_size);

    if (!is_alloc_supported(in_image_ptr->get_image_memory_types(),
                            in_required_memory_features,
                           &filtered_memory_types) )
    {
        result = false;

        goto end;
    }

    /* Store a new block item descriptor */
    new_item_ptr.reset(
        new Item(this,
                 in_image_ptr,
                 in_subresource,
                 in_offset,
                 in_extent,
                 total_region_size_in_bytes,
                 in_image_ptr->get_image_memory_types(),
                 tile_size,
                 in_required_memory_features,
                 filtered_memory_types,
                 in_external_memory_handle_types)
    );

    m_items.push_back(
        std::move(new_item_ptr)
    );

    m_per_object_pending_alloc_status[in_image_ptr] = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::bake()
{
    std::vector<Anvil::FenceUniquePtr>      fences;
    std::unique_lock<std::recursive_mutex>  mutex_lock;
    auto                                    mutex_ptr                   = get_mutex();
    bool                                    needs_sparse_memory_binding = false;
    bool                                    result                      = false;
    Anvil::SparseMemoryBindInfoID           sparse_memory_bind_info_id  = UINT32_MAX;
    Anvil::SparseMemoryBindingUpdateInfo    sparse_memory_binding;
    std::shared_ptr<Anvil::MemoryAllocator> this_ptr;

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    if (!m_backend_ptr->supports_baking() )
    {
        result = (m_items.size() == 0);

        anvil_assert(result);
        goto end;
    }

    if (m_items.size() == 0)
    {
        result = true;

        goto end;
    }

    result = m_backend_ptr->bake(m_items);
    anvil_assert(result);

    /* Prepare a sparse memory binding structure, if we're going to need one */
    for (auto item_iterator  = m_items.begin();
              item_iterator != m_items.end() && !needs_sparse_memory_binding;
            ++item_iterator)
    {
        switch ((*item_iterator)->type)
        {
            case Anvil::MemoryAllocator::ITEM_TYPE_BUFFER:
            case Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_BUFFER_REGION:
            {
                needs_sparse_memory_binding |= ((*item_iterator)->buffer_ptr->get_create_info_ptr()->get_type() == Anvil::BufferType::SPARSE_NO_ALLOC);

                break;
            }

            case Anvil::MemoryAllocator::ITEM_TYPE_IMAGE_WHOLE:
            case Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_IMAGE_MIPTAIL:
            case Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_IMAGE_SUBRESOURCE:
            {
                needs_sparse_memory_binding |= ((*item_iterator)->image_ptr->get_create_info_ptr()->get_residency_scope() != Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED);

                break;
            }

            default:
            {
                anvil_assert_fail();
            }
        }
    }

    if (needs_sparse_memory_binding)
    {
        Anvil::FenceUniquePtr wait_fence_ptr;

        {
            auto create_info_ptr = Anvil::FenceCreateInfo::create(m_device_ptr,
                                                                  false); /* create_signalled */

            create_info_ptr->set_mt_safety(MT_SAFETY_DISABLED);

            wait_fence_ptr = Anvil::Fence::create(std::move(create_info_ptr) );
        }

        sparse_memory_binding.set_fence(wait_fence_ptr.get() );
        fences.push_back               (std::move(wait_fence_ptr) );

        sparse_memory_bind_info_id = sparse_memory_binding.add_bind_info(0,        /* n_signal_semaphores       */
                                                                         nullptr,  /* opt_signal_semaphores_ptr */
                                                                         0,        /* n_wait_semaphores         */
                                                                         nullptr); /* opt_wait_semaphores_ptr   */

        anvil_assert(sparse_memory_bind_info_id  != UINT32_MAX);
    }

    result = true;

    /* Distribute memory regions to the registered objects */
    for (auto item_iterator  = m_items.begin();
              item_iterator != m_items.end();
            ++item_iterator)
    {
        auto item_ptr = item_iterator->get();

        if (item_ptr->alloc_memory_block_ptr)
        {
            anvil_assert(item_ptr->is_baked);

            switch (item_ptr->type)
            {
                case Anvil::MemoryAllocator::ITEM_TYPE_BUFFER:
                {
                    if (item_ptr->buffer_ptr->get_create_info_ptr()->get_type() != Anvil::BufferType::SPARSE_NO_ALLOC)
                    {
                        item_ptr->buffer_ptr->set_nonsparse_memory(
                            std::move(item_ptr->alloc_memory_block_ptr)
                        );
                    }
                    else
                    {
                        sparse_memory_binding.append_buffer_memory_update(sparse_memory_bind_info_id,
                                                                          item_ptr->buffer_ptr,
                                                                          0, /* buffer_memory_start_offset */
                                                                          item_ptr->alloc_memory_block_ptr.release(),
                                                                          0,    /* opt_memory_block_start_offset       */
                                                                          true, /* in_opt_memory_block_owned_by_buffer */
                                                                          item_ptr->alloc_size);
                    }

                    break;
                }

                case Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_BUFFER_REGION:
                {
                    anvil_assert(item_ptr->buffer_ptr->get_create_info_ptr()->get_type() == Anvil::BufferType::SPARSE_NO_ALLOC);

                    sparse_memory_binding.append_buffer_memory_update(sparse_memory_bind_info_id,
                                                                      item_ptr->buffer_ptr,
                                                                      item_ptr->alloc_offset,
                                                                      item_ptr->alloc_memory_block_ptr.release(),
                                                                      0,    /* opt_memory_block_start_offset       */
                                                                      true, /* in_opt_memory_block_owned_by_buffer */
                                                                      item_ptr->alloc_size);

                    break;
                }

                case Anvil::MemoryAllocator::ITEM_TYPE_IMAGE_WHOLE:
                {
                    if (item_ptr->image_ptr->get_create_info_ptr()->get_residency_scope() == Anvil::SPARSE_RESIDENCY_SCOPE_UNDEFINED)
                    {
                        item_ptr->image_ptr->set_memory(
                            std::move(item_ptr->alloc_memory_block_ptr)
                        );
                    }
                    else
                    {
                        sparse_memory_binding.append_opaque_image_memory_update(sparse_memory_bind_info_id,
                                                                                item_ptr->image_ptr,
                                                                                0, /* resource_offset */
                                                                                item_ptr->alloc_size,
                                                                                0, /* flags */
                                                                                item_ptr->alloc_memory_block_ptr.release(),
                                                                                0,     /* opt_memory_block_start_offset      */
                                                                                true); /* in_opt_memory_block_owned_by_image */
                    }

                    break;
                }

                case Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_IMAGE_MIPTAIL:
                {
                    sparse_memory_binding.append_opaque_image_memory_update(sparse_memory_bind_info_id,
                                                                            item_ptr->image_ptr,
                                                                            item_ptr->miptail_offset,
                                                                            item_ptr->alloc_size,
                                                                            0, /* flags */
                                                                            item_ptr->alloc_memory_block_ptr.release(),
                                                                            0,     /* opt_memory_block_start_offset      */
                                                                            true); /* in_opt_memory_block_owned_by_image */

                    break;
                }

                case Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_IMAGE_SUBRESOURCE:
                {
                    sparse_memory_binding.append_image_memory_update(sparse_memory_bind_info_id,
                                                                     item_ptr->image_ptr,
                                                                     item_ptr->subresource,
                                                                     item_ptr->offset,
                                                                     item_ptr->extent,
                                                                     0, /* flags */
                                                                     item_ptr->alloc_memory_block_ptr.release(),
                                                                     0,     /* opt_memory_block_start_offset      */
                                                                     true); /* in_opt_memory_block_owned_by_image */

                    break;
                }

                default:
                {
                    anvil_assert_fail();
                }
            }
        }
    }

    /* If memory backing is needed for one or more sparse resources, bind these now */
    if (sparse_memory_bind_info_id != UINT32_MAX)
    {
        Anvil::Queue* sparse_queue_ptr(m_device_ptr->get_sparse_binding_queue(0) );

        result = sparse_queue_ptr->bind_sparse_memory(sparse_memory_binding);
        anvil_assert(result);

        /* Block until the sparse memory bindings are in place */
        vkWaitForFences(m_device_ptr->get_device_vk(),
                        1, /* fenceCount */
                        sparse_memory_binding.get_fence()->get_fence_ptr(),
                        VK_FALSE, /* waitAll */
                        UINT64_MAX);
    }

    /* If the user does not keep the memory allocator around and all items are assigned memory backing,
     * the m_items.erase() call we do close to the end of this func may invoke destruction of this object.
     * There's a post-alloc call-back that we still need to do after all items are traversed, so, in order
     * to prevent the premature destruction of the allocator, cache a shared ptr to this instance, so that
     * the allocator only goes out of scope when this function leaves.
     */
    for (uint32_t n_item = 0;
                  n_item < m_items.size();
                ++n_item
        )
    {
        auto  item_iterator = m_items.begin() + n_item;
        auto& item_ptr      = *item_iterator;

        if (item_ptr->is_baked)
        {
            decltype(m_per_object_pending_alloc_status)::iterator alloc_status_map_iterator;

            switch (item_ptr->type)
            {
                case ITEM_TYPE_BUFFER:                   /* fall-through */
                case ITEM_TYPE_SPARSE_BUFFER_REGION:     alloc_status_map_iterator = m_per_object_pending_alloc_status.find(item_ptr->buffer_ptr); break;

                case ITEM_TYPE_IMAGE_WHOLE:              /* fall-through */
                case ITEM_TYPE_SPARSE_IMAGE_MIPTAIL:     /* fall-through */
                case ITEM_TYPE_SPARSE_IMAGE_SUBRESOURCE: alloc_status_map_iterator = m_per_object_pending_alloc_status.find(item_ptr->image_ptr); break;

                default:
                {
                    anvil_assert_fail();
                }
            }

            if (alloc_status_map_iterator != m_per_object_pending_alloc_status.end() )
            {
                m_per_object_pending_alloc_status.erase(alloc_status_map_iterator);
            }
        }
    }

    /* Perform post-alloc fill actions */
    for (const auto& current_item_ptr : m_items)
    {
        VkDeviceSize buffer_size   = 0;

        if (current_item_ptr->type != Anvil::MemoryAllocator::ITEM_TYPE_BUFFER)
        {
            continue;
        }

        if (!current_item_ptr->is_baked)
        {
            continue;
        }

        buffer_size = current_item_ptr->buffer_ptr->get_create_info_ptr()->get_size();

        if (current_item_ptr->buffer_ref_float_data_ptr != nullptr)
        {
            current_item_ptr->buffer_ptr->write(0, /* start_offset */
                                                buffer_size,
                                                current_item_ptr->buffer_ref_float_data_ptr.get() );
        }
        else
        if (current_item_ptr->buffer_ref_float_vector_data_ptr != nullptr)
        {
            current_item_ptr->buffer_ptr->write(0, /* start_offset */
                                               buffer_size,
                                              &(*current_item_ptr->buffer_ref_float_vector_data_ptr)[0]);
        }
        else
        if (current_item_ptr->buffer_ref_uchar8_data_ptr != nullptr)
        {
            current_item_ptr->buffer_ptr->write(0, /* start_offset */
                                                buffer_size,
                                                current_item_ptr->buffer_ref_uchar8_data_ptr.get() );
        }
        else
        if (current_item_ptr->buffer_ref_uchar8_vector_data_ptr != nullptr)
        {
            current_item_ptr->buffer_ptr->write(0, /* start_offset */
                                                buffer_size,
                                               &(*current_item_ptr->buffer_ref_uchar8_vector_data_ptr)[0]);
        }
        else
        if (current_item_ptr->buffer_ref_uint32_data_ptr != nullptr)
        {
            current_item_ptr->buffer_ptr->write(0, /* start_offset */
                                                buffer_size,
                                                current_item_ptr->buffer_ref_uint32_data_ptr.get() );
        }
        else
        if (current_item_ptr->buffer_ref_uint32_vector_data_ptr != nullptr)
        {
            current_item_ptr->buffer_ptr->write(0, /* start_offset */
                                                buffer_size,
                                               &(*current_item_ptr->buffer_ref_uint32_vector_data_ptr)[0]);
        }
    }

    m_items.clear();

    if (m_post_bake_callback_function != nullptr)
    {
        m_post_bake_callback_function(this);
    }

end:
    if (mutex_lock.owns_lock() )
    {
        mutex_lock.unlock();
    }

    return result;
}

/* Please see header for specification */
Anvil::MemoryAllocatorUniquePtr Anvil::MemoryAllocator::create_oneshot(const Anvil::BaseDevice* in_device_ptr,
                                                                       MTSafety                 in_mt_safety)
{
    std::shared_ptr<IMemoryAllocatorBackend> backend_ptr;
    const bool                               mt_safe    (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                                         in_device_ptr) );
    std::unique_ptr<MemoryAllocator>         result_ptr (nullptr,
                                                         std::default_delete<MemoryAllocator>() );

    backend_ptr.reset(
        new Anvil::MemoryAllocatorBackends::OneShot(in_device_ptr)
    );

    if (backend_ptr != nullptr)
    {
        result_ptr.reset(
            new Anvil::MemoryAllocator(in_device_ptr,
                                       backend_ptr,
                                       mt_safe)
        );
    }

    return std::move(result_ptr);
}

/* Please see header for specification */
Anvil::MemoryAllocatorUniquePtr Anvil::MemoryAllocator::create_vma(const Anvil::BaseDevice* in_device_ptr,
                                                                   MTSafety                 in_mt_safety)
{
    std::shared_ptr<IMemoryAllocatorBackend> backend_ptr;
    const bool                               mt_safe    (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                                         in_device_ptr) );
    std::unique_ptr<MemoryAllocator>         result_ptr(nullptr,
                                                        std::default_delete<MemoryAllocator>() );

    backend_ptr = Anvil::MemoryAllocatorBackends::VMA::create(in_device_ptr);

    if (backend_ptr != nullptr)
    {
        result_ptr.reset(
            new Anvil::MemoryAllocator(in_device_ptr,
                                       std::move(backend_ptr),
                                       mt_safe)
        );
    }

    return std::move(result_ptr);
}

bool Anvil::MemoryAllocator::do_external_memory_handle_type_sanity_checks(const Anvil::ExternalMemoryHandleTypeBits& in_external_memory_handle_types) const
{
    bool result = true;

    if (in_external_memory_handle_types != 0)
    {
        if (!m_backend_ptr->supports_external_memory_handles(in_external_memory_handle_types) )
        {
            anvil_assert(m_backend_ptr->supports_external_memory_handles(in_external_memory_handle_types) );

            result = false;
        }
    }

    return result;
}

/** Tells whether or not a given set of memory types supports the requested memory features. */
bool Anvil::MemoryAllocator::is_alloc_supported(uint32_t                  in_memory_types,
                                                Anvil::MemoryFeatureFlags in_memory_features,
                                                uint32_t*                 out_opt_filtered_memory_types_ptr) const
{
    const bool  is_coherent_memory_required        (((in_memory_features & MEMORY_FEATURE_FLAG_HOST_COHERENT)    != 0) );
    const bool  is_device_local_memory_required    (((in_memory_features & MEMORY_FEATURE_FLAG_DEVICE_LOCAL)     != 0) );
    const bool  is_host_cached_memory_required     (((in_memory_features & MEMORY_FEATURE_FLAG_HOST_CACHED)      != 0) );
    const bool  is_lazily_allocated_memory_required(((in_memory_features & MEMORY_FEATURE_FLAG_LAZILY_ALLOCATED) != 0) );
    const bool  is_mappable_memory_required        (((in_memory_features & MEMORY_FEATURE_FLAG_MAPPABLE)         != 0) );
    const auto& memory_props                       (m_device_ptr->get_physical_device_memory_properties()  );
    bool        result                             (true);

    /* Filter out memory types that do not support features requested at creation time */
    for (uint32_t n_memory_type = 0;
                  (1u << n_memory_type) <= in_memory_types;
                ++n_memory_type)
    {
        if ((is_coherent_memory_required         && !(memory_props.types[n_memory_type].flags           & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))    ||
            (is_device_local_memory_required     && !(memory_props.types[n_memory_type].flags           & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))     ||
            (is_host_cached_memory_required      && !(memory_props.types[n_memory_type].flags           & VK_MEMORY_PROPERTY_HOST_CACHED_BIT))      ||
            (is_lazily_allocated_memory_required && !(memory_props.types[n_memory_type].flags           & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)) ||
            (is_mappable_memory_required         && !(memory_props.types[n_memory_type].flags           & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) )
        {
            in_memory_types &= ~(1 << n_memory_type);
        }

        if (in_memory_types == 0)
        {
            /* None of the available memory heaps support the requested set of features ! */
            result = false;

            goto end;
        }
    }

    if (out_opt_filtered_memory_types_ptr != nullptr)
    {
        *out_opt_filtered_memory_types_ptr = in_memory_types;
    }

end:
    return result;
}

/* Please see header for specification */
void Anvil::MemoryAllocator::on_is_alloc_pending_for_buffer_query(CallbackArgument* in_callback_arg_ptr)
{
    IsBufferMemoryAllocPendingQueryCallbackArgument* query_ptr                 = dynamic_cast<IsBufferMemoryAllocPendingQueryCallbackArgument*>(in_callback_arg_ptr);
    auto                                             alloc_status_map_iterator = m_per_object_pending_alloc_status.find                        (query_ptr->buffer_ptr);
    std::unique_lock<std::recursive_mutex>           mutex_lock;
    auto                                             mutex_ptr                 = get_mutex();

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    if (alloc_status_map_iterator != m_per_object_pending_alloc_status.end() )
    {
        query_ptr->result = true;
    }
}

/* Please see header for specification */
void Anvil::MemoryAllocator::on_is_alloc_pending_for_image_query(CallbackArgument* in_callback_arg_ptr)
{
    IsImageMemoryAllocPendingQueryCallbackArgument* query_ptr                 = dynamic_cast<IsImageMemoryAllocPendingQueryCallbackArgument*>(in_callback_arg_ptr);
    auto                                            alloc_status_map_iterator = m_per_object_pending_alloc_status.find                       (query_ptr->image_ptr);
    std::unique_lock<std::recursive_mutex>          mutex_lock;
    auto                                            mutex_ptr                 = get_mutex();

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    if (alloc_status_map_iterator != m_per_object_pending_alloc_status.end() )
    {
        query_ptr->result = true;
    }
}

/* Please see header for specification */
void Anvil::MemoryAllocator::on_implicit_bake_needed()
{
    /* Sanity checks */
    anvil_assert(m_items.size() >= 1);

    bake();
}

/* Please see header for specification */
void Anvil::MemoryAllocator::set_post_bake_callback(MemoryAllocatorBakeCallbackFunction in_post_bake_callback_function)
{
    std::unique_lock<std::recursive_mutex> mutex_lock;
    auto                                   mutex_ptr = get_mutex();

    if (mutex_ptr != nullptr)
    {
        mutex_lock = std::move(
            std::unique_lock<std::recursive_mutex>(*mutex_ptr)
        );
    }

    anvil_assert(m_post_bake_callback_function == nullptr);

    if (m_post_bake_callback_function == nullptr)
    {
        m_post_bake_callback_function = in_post_bake_callback_function;
    }
}
