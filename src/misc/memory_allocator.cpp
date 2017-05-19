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

#include "misc/debug.h"
#include "misc/formats.h"
#include "misc/memory_allocator.h"
#include "wrappers/buffer.h"
#include "wrappers/device.h"
#include "wrappers/fence.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include "wrappers/queue.h"
#include <algorithm>


/* Please see header for specification */
Anvil::MemoryAllocator::MemoryAllocator(std::weak_ptr<Anvil::BaseDevice> in_device_ptr)
    :m_device_ptr                 (in_device_ptr),
     m_is_baked                   (false),
     m_pfn_post_bake_callback_ptr (nullptr),
     m_post_bake_callback_user_arg(nullptr) 
{
}

/* Please see header for specification */
Anvil::MemoryAllocator::~MemoryAllocator()
{
    if (!m_is_baked         &&
         m_items.size() > 0)
    {
        bake();
    }
}


/** Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                        MemoryFeatureFlags             in_required_memory_features)
{
    return add_buffer_internal(in_buffer_ptr,
                               in_required_memory_features);
}

/** Determines the amount of memory, supported memory type and required alignment for the specified
 *  buffer, and caches all this data in the m_items for further processing at baking time.
 *
 *  @param buffer_ptr Buffer instance to assign a memory block at baking time.
 **/
bool Anvil::MemoryAllocator::add_buffer_internal(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                                 MemoryFeatureFlags             in_required_memory_features)
{
    VkDeviceSize buffer_alignment    = 0;
    uint32_t     buffer_memory_types = 0;
    VkDeviceSize buffer_storage_size = 0;
    bool         result              = true;

    /* Sanity checks */
    anvil_assert(!m_is_baked);
    anvil_assert(in_buffer_ptr != nullptr);

    /* Determine how much space we're going to need, what alignment we need
     * to consider, and so on. */
    const VkMemoryRequirements memory_reqs = in_buffer_ptr->get_memory_requirements();

    buffer_alignment    = memory_reqs.alignment;
    buffer_memory_types = memory_reqs.memoryTypeBits;
    buffer_storage_size = memory_reqs.size;

    if (!is_alloc_supported(buffer_memory_types,
                            in_required_memory_features,
                            nullptr) ) /* opt_out_filtered_memory_types_ptr */
    {
        result = false;

        goto end;
    }

    /* Store a new block item descriptor. */
    m_items.push_back(Item(in_buffer_ptr,
                           buffer_storage_size,
                           buffer_memory_types,
                           buffer_alignment,
                           in_required_memory_features) );

end:
    anvil_assert(result);

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_float_data_ptr_based_post_fill(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                                                            std::shared_ptr<float>         in_data_ptr,
                                                                            MemoryFeatureFlags             in_required_memory_features)
{
    bool result = add_buffer_internal(in_buffer_ptr,
                                      in_required_memory_features);

    if (result)
    {
        m_items.back().buffer_ref_float_data_ptr = in_data_ptr;
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_float_data_vector_ptr_based_post_fill(std::shared_ptr<Anvil::Buffer>       in_buffer_ptr,
                                                                                   std::shared_ptr<std::vector<float> > in_data_vector_ptr,
                                                                                   MemoryFeatureFlags                   in_required_memory_features)
{
    anvil_assert(in_data_vector_ptr->size() * sizeof(float) == in_buffer_ptr->get_size() );

    bool result = add_buffer_internal(in_buffer_ptr,
                                      in_required_memory_features);

    if (result)
    {
        m_items.back().buffer_ref_float_vector_data_ptr = in_data_vector_ptr;
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_uchar8_data_ptr_based_post_fill(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                                                             std::shared_ptr<unsigned char> in_data_ptr,
                                                                             MemoryFeatureFlags             in_required_memory_features)
{
    bool result = add_buffer_internal(in_buffer_ptr,
                                      in_required_memory_features);

    if (result)
    {
        m_items.back().buffer_ref_uchar8_data_ptr = in_data_ptr;
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_uchar8_data_vector_ptr_based_post_fill(std::shared_ptr<Anvil::Buffer>               in_buffer_ptr,
                                                                                    std::shared_ptr<std::vector<unsigned char> > in_data_vector_ptr,
                                                                                    MemoryFeatureFlags                           in_required_memory_features)
{
    anvil_assert(in_data_vector_ptr->size() == in_buffer_ptr->get_size() );

    bool result = add_buffer_internal(in_buffer_ptr,
                                      in_required_memory_features);

    if (result)
    {
        m_items.back().buffer_ref_uchar8_vector_data_ptr = in_data_vector_ptr;
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_uint32_data_ptr_based_post_fill(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                                                             std::shared_ptr<uint32_t>      in_data_ptr,
                                                                             MemoryFeatureFlags             in_required_memory_features)
{
    bool result = add_buffer_internal(in_buffer_ptr,
                                      in_required_memory_features);

    if (result)
    {
        m_items.back().buffer_ref_uint32_data_ptr = in_data_ptr;
    }

    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_buffer_with_uint32_data_vector_ptr_based_post_fill(std::shared_ptr<Anvil::Buffer>          in_buffer_ptr,
                                                                                    std::shared_ptr<std::vector<uint32_t> > in_data_vector_ptr,
                                                                                    MemoryFeatureFlags                      in_required_memory_features)
{
    anvil_assert(in_data_vector_ptr->size() * sizeof(uint32_t) == in_buffer_ptr->get_size() );

    bool result = add_buffer_internal(in_buffer_ptr,
                                      in_required_memory_features);

    if (result)
    {
        m_items.back().buffer_ref_uint32_vector_data_ptr = in_data_vector_ptr;
    }

    return result;
}

/** Please see header for specification */
bool Anvil::MemoryAllocator::add_image_whole(std::shared_ptr<Anvil::Image> in_image_ptr,
                                             MemoryFeatureFlags            in_required_memory_features)
{
    VkDeviceSize image_alignment    = 0;
    uint32_t     image_memory_types = 0;
    VkDeviceSize image_storage_size = 0;
    bool         result             = true;

    /* Sanity checks */
    anvil_assert(!m_is_baked);
    anvil_assert(in_image_ptr != nullptr);

    /* Determine how much size is needed for the image's storage, as well as what
     * the allocation requirements are */
    image_alignment    = in_image_ptr->get_image_alignment();
    image_memory_types = in_image_ptr->get_image_memory_types();
    image_storage_size = in_image_ptr->get_image_storage_size();

    if (!is_alloc_supported(image_memory_types,
                            in_required_memory_features,
                            nullptr) ) /* opt_out_filtered_memory_types_ptr */
    {
        result = false;

        goto end;
    }

    /* Store a new block item descriptor */
    m_items.push_back(Item(in_image_ptr,
                           image_storage_size,
                           image_memory_types,
                           image_alignment,
                           in_required_memory_features) );

end:
    return result;
}

/** Please see header for specification */
bool Anvil::MemoryAllocator::add_sparse_image_miptail(std::shared_ptr<Anvil::Image> in_image_ptr,
                                                      VkImageAspectFlagBits         in_aspect,
                                                      uint32_t                      in_n_layer,
                                                      MemoryFeatureFlags            in_required_memory_features)
{
    const Anvil::SparseImageAspectProperties* aspect_props_ptr     = nullptr;
    uint32_t                                  miptail_memory_types = 0;
    VkDeviceSize                              miptail_offset       = static_cast<VkDeviceSize>(UINT64_MAX);
    VkDeviceSize                              miptail_size         = 0;
    bool                                      result               = true;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Sanity checks */
    anvil_assert(!m_is_baked);
    anvil_assert(in_image_ptr                                  != nullptr);
    anvil_assert(in_image_ptr->is_sparse         () );
    anvil_assert(in_image_ptr->get_image_n_layers()            >  in_n_layer);
    anvil_assert(in_image_ptr->has_aspects       (in_aspect) );

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
                            nullptr) ) /* opt_out_filtered_memory_types_ptr */
    {
        result = false;

        goto end;
    }

    /* Store a new block item descriptor */
    m_items.push_back(Item(in_image_ptr,
                           in_n_layer,
                           miptail_size,
                           miptail_memory_types,
                           miptail_offset,
                           in_image_ptr->get_image_alignment(),
                           in_required_memory_features));

end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::add_sparse_image_subresource(std::shared_ptr<Anvil::Image> in_image_ptr,
                                                          const VkImageSubresource&     in_subresource,
                                                          const VkOffset3D&             in_offset,
                                                          VkExtent3D                    in_extent,
                                                          MemoryFeatureFlags            in_required_memory_features)
{
    const Anvil::SparseImageAspectProperties* aspect_props_ptr       = nullptr;
    uint32_t                                  component_size_bits[4] = {0};
    uint32_t                                  mip_size[3];
    bool                                      result                     = true;
    VkDeviceSize                              total_region_size_in_bytes = 0;

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Sanity checks */
    anvil_assert(!m_is_baked);
    anvil_assert(in_image_ptr                                         != nullptr);
    anvil_assert(in_image_ptr->is_sparse() );
    anvil_assert(in_image_ptr->has_aspects(in_subresource.aspectMask) );
    anvil_assert(in_image_ptr->get_image_n_mipmaps()                  >  in_subresource.mipLevel);
    anvil_assert(in_image_ptr->get_image_n_layers()                   >  in_subresource.arrayLayer);

    anvil_assert(in_extent.depth  >= 1);
    anvil_assert(in_extent.height >= 1);
    anvil_assert(in_extent.width  >= 1);

    anvil_assert(!Anvil::Formats::is_format_compressed(in_image_ptr->get_image_format() ));                     // TODO
    anvil_assert((Anvil::Utils::is_pow2               (static_cast<int32_t>(in_subresource.aspectMask)) != 0)); // only permit a single aspect


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
    Anvil::Formats::get_format_n_component_bits(in_image_ptr->get_image_format(),
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

    /* The region size may be smaller than the required page size. Round it up if that's the case */
    const VkDeviceSize tile_size = in_image_ptr->get_memory_requirements().alignment;

    total_region_size_in_bytes = Anvil::Utils::round_up(total_region_size_in_bytes,
                                                        tile_size);

    if (!is_alloc_supported(in_image_ptr->get_image_memory_types(),
                            in_required_memory_features,
                            nullptr) ) /* opt_out_filtered_memory_types_ptr */
    {
        result = false;

        goto end;
    }

    /* Store a new block item descriptor */
    m_items.push_back(Item(in_image_ptr,
                           in_subresource,
                           in_offset,
                           in_extent,
                           total_region_size_in_bytes,
                           in_image_ptr->get_image_memory_types(),
                           tile_size,
                           in_required_memory_features) );

end:
    return result;
}

/* Please see header for specification */
bool Anvil::MemoryAllocator::bake()
{
    std::shared_ptr<Anvil::BaseDevice>          device_locked_ptr          (m_device_ptr);
    const auto&                                 memory_props               (device_locked_ptr->get_physical_device_memory_properties() );
    const uint32_t                              n_memory_types             (static_cast<uint32_t>(memory_props.types.size() ));
    bool                                        needs_sparse_memory_binding(false);
    std::vector<std::vector<Item*> >            per_mem_type_items_vector  (n_memory_types);
    bool                                        result                     (false);
    Anvil::SparseMemoryBindInfoID               sparse_memory_bind_info_id (UINT32_MAX);
    Anvil::Utils::SparseMemoryBindingUpdateInfo sparse_memory_binding;

    if (m_is_baked)
    {
        result = true;

        goto end;
    }

    /* Sanity checks */
    anvil_assert(m_items.size() > 0);

    /* Iterate over all block items and determine what memory types we can use.
     *
     * In certain cases, we may need to suballocate from more than one memory block,
     * due to the fact not all memory heaps may support features requested at
     * creation time.
     */
    for (auto item_iterator  = m_items.begin();
              item_iterator != m_items.end();
            ++item_iterator)
    {
        uint32_t allowed_memory_types = item_iterator->alloc_memory_types;

        if (!is_alloc_supported(allowed_memory_types,
                                item_iterator->alloc_memory_required_features,
                               &allowed_memory_types))
        {
            /* This should never happen */
            anvil_assert(false);

            goto end;
        }

        /* Assign the item to supported memory types */
        for (uint32_t n_memory_type = 0;
                      (1u << n_memory_type) <= allowed_memory_types;
                     ++n_memory_type)
        {
            if (!(allowed_memory_types & (1 << n_memory_type)) )
            {
                continue;
            }

            per_mem_type_items_vector.at(n_memory_type).push_back(&(*item_iterator) );
            break;
        }
    }

    /* For each memory type, for each there's at least one item, bake a memory block */
    {
        uint32_t current_memory_type_index(0);

        for (auto mem_type_to_item_vector_iterator  = per_mem_type_items_vector.begin();
                  mem_type_to_item_vector_iterator != per_mem_type_items_vector.end();
                ++mem_type_to_item_vector_iterator, ++current_memory_type_index)
        {
            auto& current_item_vector = *mem_type_to_item_vector_iterator;

            if (current_item_vector.size() > 0)
            {
                std::shared_ptr<Anvil::MemoryBlock> new_memory_block_ptr;
                VkDeviceSize                        n_bytes_required      = 0;

                /* Go through the items, calculate offsets and the total amount of memory we're going
                 * to need to alloc off the heap */
                for (auto& current_item_ptr : current_item_vector)
                {
                    n_bytes_required = Anvil::Utils::round_up(n_bytes_required,
                                                              current_item_ptr->alloc_memory_required_alignment);

                    current_item_ptr->alloc_offset  = n_bytes_required;
                    n_bytes_required               += current_item_ptr->alloc_size;
                }

                /* Bake the block and stash it */
                new_memory_block_ptr = Anvil::MemoryBlock::create(m_device_ptr,
                                                                  1u << current_memory_type_index,
                                                                  n_bytes_required,
                                                                  ((memory_props.types[current_memory_type_index].flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)  != 0),   /* should_be_mappable */
                                                                  ((memory_props.types[current_memory_type_index].flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0) ); /* should_be_coherent */

                anvil_assert(new_memory_block_ptr != nullptr);

                /* Go through the items again and assign the result memory block */
                for (auto& current_item_ptr : current_item_vector)
                {
                    current_item_ptr->alloc_memory_block_ptr = new_memory_block_ptr;
                }
            }
        }
    }

    /* Prepare a sparse memory binding structure, if we're going to need one */
    for (auto item_iterator  = m_items.begin();
              item_iterator != m_items.end() && !needs_sparse_memory_binding;
            ++item_iterator)
    {
        switch (item_iterator->type)
        {
            case ITEM_TYPE_BUFFER:
            {
                needs_sparse_memory_binding |= item_iterator->buffer_ptr->is_sparse(); 

                break;
            }

            case ITEM_TYPE_IMAGE_WHOLE:
            case ITEM_TYPE_SPARSE_IMAGE_MIPTAIL:
            case ITEM_TYPE_SPARSE_IMAGE_SUBRESOURCE:
            {
                needs_sparse_memory_binding |= item_iterator->image_ptr->is_sparse();

                break;
            }

            default:
            {
                anvil_assert(false);
            }
        }
    }

    if (needs_sparse_memory_binding)
    {
        std::shared_ptr<Anvil::Fence> wait_fence_ptr = Anvil::Fence::create(m_device_ptr,
                                                                            false); /* create_signalled */

        sparse_memory_binding.set_fence(wait_fence_ptr);

        sparse_memory_bind_info_id = sparse_memory_binding.add_bind_info(0,        /* n_signal_semaphores       */
                                                                         nullptr,  /* opt_signal_semaphores_ptr */
                                                                         0,        /* n_wait_semaphores         */
                                                                         nullptr); /* opt_wait_semaphores_ptr   */

        anvil_assert(sparse_memory_bind_info_id  != UINT32_MAX);
    }

    /* Distribute memory regions to the registered objects */
    for (auto item_iterator  = m_items.begin();
              item_iterator != m_items.end();
            ++item_iterator)
    {
        std::shared_ptr<Anvil::MemoryBlock> memory_block_ptr;

        memory_block_ptr = Anvil::MemoryBlock::create_derived(item_iterator->alloc_memory_block_ptr,
                                                              item_iterator->alloc_offset,
                                                              item_iterator->alloc_size);

        switch (item_iterator->type)
        {
            case ITEM_TYPE_BUFFER:
            {
                if (!item_iterator->buffer_ptr->is_sparse() )
                {
                    item_iterator->buffer_ptr->set_nonsparse_memory(memory_block_ptr);
                }
                else
                {
                    sparse_memory_binding.append_buffer_memory_update(sparse_memory_bind_info_id,
                                                                      item_iterator->buffer_ptr,
                                                                      0, /* buffer_memory_start_offset */
                                                                      memory_block_ptr,
                                                                      0, /* opt_memory_block_start_offset */
                                                                      item_iterator->alloc_size);
                }

                break;
            }

            case ITEM_TYPE_IMAGE_WHOLE:
            {
                if (!item_iterator->image_ptr->is_sparse() )
                {
                    item_iterator->image_ptr->set_memory(memory_block_ptr);
                }
                else
                {
                    sparse_memory_binding.append_opaque_image_memory_update(sparse_memory_bind_info_id,
                                                                            item_iterator->image_ptr,
                                                                            0, /* resource_offset */
                                                                            item_iterator->alloc_size,
                                                                            0, /* flags */
                                                                            memory_block_ptr,
                                                                            0); /* opt_memory_block_start_offset */
                }

                break;
            }

            case ITEM_TYPE_SPARSE_IMAGE_MIPTAIL:
            {
                sparse_memory_binding.append_opaque_image_memory_update(sparse_memory_bind_info_id,
                                                                        item_iterator->image_ptr,
                                                                        item_iterator->miptail_offset,
                                                                        item_iterator->alloc_size,
                                                                        0, /* flags */
                                                                        memory_block_ptr,
                                                                        0); /* opt_memory_block_start_offset */

                break;
            }

            case ITEM_TYPE_SPARSE_IMAGE_SUBRESOURCE:
            {
                sparse_memory_binding.append_image_memory_update(sparse_memory_bind_info_id,
                                                                 item_iterator->image_ptr,
                                                                 item_iterator->subresource,
                                                                 item_iterator->offset,
                                                                 item_iterator->extent,
                                                                 0, /* flags */
                                                                 memory_block_ptr,
                                                                 0); /* opt_memory_block_start_offset */

                break;
            }

            default:
            {
                anvil_assert(false);
            }
        }
    }

    result = true;

    /* If memory backing is needed for one or more sparse resources, bind these now */
    if (sparse_memory_bind_info_id != UINT32_MAX)
    {
        std::shared_ptr<Anvil::Queue> sparse_queue_ptr(device_locked_ptr->get_sparse_binding_queue(0) );

        result = sparse_queue_ptr->bind_sparse_memory(sparse_memory_binding);
        anvil_assert(result);

        /* Block until the sparse memory bindings are in place */
        vkWaitForFences(device_locked_ptr->get_device_vk(),
                        1, /* fenceCount */
                        sparse_memory_binding.get_fence()->get_fence_ptr(),
                        VK_FALSE, /* waitAll */
                        UINT64_MAX);
    }

    /* Perform post-fill actions */
    for (auto item_iterator  = m_items.begin();
              item_iterator != m_items.end();
            ++item_iterator)
    {
        VkDeviceSize buffer_size = 0;

        if (item_iterator->type != ITEM_TYPE_BUFFER)
        {
            continue;
        }

        buffer_size = item_iterator->buffer_ptr->get_size();

        if (item_iterator->buffer_ref_float_data_ptr != nullptr)
        {
            item_iterator->buffer_ptr->write(0, /* start_offset */
                                             buffer_size,
                                             item_iterator->buffer_ref_float_data_ptr.get() );
        }
        else
        if (item_iterator->buffer_ref_float_vector_data_ptr != nullptr)
        {
            item_iterator->buffer_ptr->write(0, /* start_offset */
                                             buffer_size,
                                            &(*item_iterator->buffer_ref_float_vector_data_ptr)[0]);
        }
        else
        if (item_iterator->buffer_ref_uchar8_data_ptr != nullptr)
        {
            item_iterator->buffer_ptr->write(0, /* start_offset */
                                             buffer_size,
                                             item_iterator->buffer_ref_uchar8_data_ptr.get() );
        }
        else
        if (item_iterator->buffer_ref_uchar8_vector_data_ptr != nullptr)
        {
            item_iterator->buffer_ptr->write(0, /* start_offset */
                                             buffer_size,
                                            &(*item_iterator->buffer_ref_uchar8_vector_data_ptr)[0]);
        }
        else
        if (item_iterator->buffer_ref_uint32_data_ptr != nullptr)
        {
            item_iterator->buffer_ptr->write(0, /* start_offset */
                                             buffer_size,
                                             item_iterator->buffer_ref_uint32_data_ptr.get() );
        }
        else
        if (item_iterator->buffer_ref_uint32_vector_data_ptr != nullptr)
        {
            item_iterator->buffer_ptr->write(0, /* start_offset */
                                             buffer_size,
                                            &(*item_iterator->buffer_ref_uint32_vector_data_ptr)[0]);
        }
    }

    if (m_pfn_post_bake_callback_ptr != nullptr)
    {
        m_pfn_post_bake_callback_ptr(this,
                                     m_post_bake_callback_user_arg);
    }

    m_items.clear();
    m_is_baked = true;
end:
    return result;
}

/* Please see header for specification */
std::shared_ptr<Anvil::MemoryAllocator> Anvil::MemoryAllocator::create(std::weak_ptr<Anvil::BaseDevice> device_ptr)
{
    std::shared_ptr<MemoryAllocator> result_ptr;

    result_ptr.reset(
        new Anvil::MemoryAllocator(device_ptr)
    );

    return result_ptr;
}

/** Tells whether or not a given set of memory types supports the requested memory features. */
bool Anvil::MemoryAllocator::is_alloc_supported(uint32_t           in_memory_types,
                                                MemoryFeatureFlags in_memory_features,
                                                uint32_t*          opt_out_filtered_memory_types_ptr) const
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr                 (m_device_ptr);
    const bool                         is_coherent_memory_required       (((in_memory_features & MEMORY_FEATURE_FLAG_COHERENT)       != 0) );
    const bool                         is_mappable_memory_required       (((in_memory_features & MEMORY_FEATURE_FLAG_MAPPABLE)       != 0) );
    const auto&                        memory_props                      (device_locked_ptr->get_physical_device_memory_properties() );
    bool                               result                            (true);

    /* Filter out memory types that do not support features requested at creation time */
    for (uint32_t n_memory_type = 0;
                  (1u << n_memory_type) <= in_memory_types;
                ++n_memory_type)
    {
        if ((is_coherent_memory_required && !(memory_props.types[n_memory_type].flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ||
            (is_mappable_memory_required && !(memory_props.types[n_memory_type].flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))  )
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

    if (opt_out_filtered_memory_types_ptr != nullptr)
    {
        *opt_out_filtered_memory_types_ptr = in_memory_types;
    }

end:
    return result;
}

/* Please see header for specification */
void Anvil::MemoryAllocator::set_post_bake_callback(PFNMEMORYALLOCATORBAKECALLBACKPROC pfn_post_bake_callback,
                                                    void*                              callback_user_arg)
{
    anvil_assert(m_pfn_post_bake_callback_ptr == nullptr);

    if (m_pfn_post_bake_callback_ptr == nullptr)
    {
        m_pfn_post_bake_callback_ptr  = pfn_post_bake_callback;
        m_post_bake_callback_user_arg = callback_user_arg;
    }
}
