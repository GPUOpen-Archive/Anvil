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

#include "misc/memalloc_backends/backend_oneshot.h"
#include "misc/debug.h"
#include "misc/formats.h"
#include "misc/image_create_info.h"
#include "misc/memory_allocator.h"
#include "misc/memory_block_create_info.h"
#include "wrappers/buffer.h"
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include <algorithm>
#include <unordered_map>
#include <cmath>

/** Please see header for specification */
Anvil::MemoryAllocatorBackends::OneShot::OneShot(const Anvil::BaseDevice* in_device_ptr)
    :m_device_ptr(in_device_ptr),
     m_is_baked  (false)
{
    /* Stub */
}

/** Please see header for specification */
Anvil::MemoryAllocatorBackends::OneShot::~OneShot()
{
    /* Stub */
}


namespace
{
    typedef struct MemoryUniqueInfo
    {
        uint32_t  device_mask;
        float     memory_priority;

        MemoryUniqueInfo(const uint32_t& in_device_mask,
                         const float&    in_memory_priority)
            :device_mask    (in_device_mask),
             memory_priority(in_memory_priority)
        {
            /* Stub */
        }

        bool operator==(const MemoryUniqueInfo& in_info) const
        {
            if ((in_info.device_mask == device_mask) && (std::fabs(in_info.memory_priority - memory_priority) < 1e-4f))
            {
                return true;
            }
            return false;
        }

        bool operator<(const MemoryUniqueInfo& in_info) const
        {
            return (device_mask < in_info.device_mask) ? true
                                                       : ((device_mask == in_info.device_mask) && (memory_priority < in_info.memory_priority));
        }

    } MemoryUniqueInfo;
}

/** Tries to create a memory object of size large enough to capacitate all added objects,
 *  given their alignment, size, and other requirements.
 *
 *  If the call is successful, each added object will have its set_memory() entry-point
 *  called with more details about what memory object they should use, along with
 *  a start offset and size of the granted allocation.
 *
 *  The allocations are guaranteed not to overlap.
 *
 *  @param in_items Items to bake memory objects for.
 *
 *  @return true if successful, false otherwise.
 **/
bool Anvil::MemoryAllocatorBackends::OneShot::bake(Anvil::MemoryAllocator::Items& in_items)
{
    typedef struct UniqueAllocationInfo
    {
        Anvil::MemoryAllocator::Item* item_ptr;
        uint32_t                      n_memory_type;

        UniqueAllocationInfo(Anvil::MemoryAllocator::Item* in_item_ptr,
                             const uint32_t&               in_n_memory_type)
            :item_ptr     (in_item_ptr),
             n_memory_type(in_n_memory_type)
        {
            /* Stub */
        }
    } UniqueAllocationInfo;

    std::map<MemoryUniqueInfo, std::vector<std::vector<Anvil::MemoryAllocator::Item*> > > device_mask_to_mem_type_to_item_vec;
    const auto&                                                                           memory_props                       (m_device_ptr->get_physical_device_memory_properties() );
    const uint32_t                                                                        n_memory_types                     (static_cast<uint32_t>(memory_props.types.size()) );
    bool                                                                                  result                             (true);
    std::vector<UniqueAllocationInfo>                                                     unique_allocs;

    /* Iterate over all block items and determine what memory types we can use.
     *
     * In certain cases, we may need to suballocate from more than one memory block,
     * due to the fact not all memory heaps may support features requested at
     * creation time.
     *
     * Dedicated allocations need to get their own memory blocks, too.
     *
     * Allocations which can be exported via external handles are tricky. Drivers do NOT need to request such allocations to be
     * dedicated. However, each exportable memory allocation comes with a set of parameters (NT handle details, as an example)
     * which make merging memory blocks tricky. Hence, for exportable allocs, just put each such alloc in its own memory block.
     *
     * Since we can only specify one device mask per memory allocation, allocation need to be further subdivided by device masks
     * they have been associated with.
     *
     */
    for (auto item_iterator  = in_items.begin();
              item_iterator != in_items.end();
            ++item_iterator)
    {
        /* Assign the item to supported memory types */
        const auto& required_memory_features = ((*item_iterator)->alloc_memory_required_features);
        const auto& supported_memory_types   = (*item_iterator)->alloc_memory_supported_memory_types;

        for (uint32_t n_memory_type = 0;
                      (1u << n_memory_type) <= (*item_iterator)->alloc_memory_supported_memory_types;
                     ++n_memory_type)
        {
            if (!(supported_memory_types & (1 << n_memory_type)) )
            {
                continue;
            }

            if ((memory_props.types.at(n_memory_type).features & static_cast<Anvil::MemoryFeatureFlagBits>(required_memory_features.get_vk() )) != required_memory_features)
            {
                continue;
            }

            if ((*item_iterator)->alloc_mgpu_peer_memory_reqs.size() > 0)
            {
                /* Make sure current memory type supports all required peer memory requirements specified by the user */
                bool can_continue    = true;
                auto mgpu_device_ptr = dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr);

                anvil_assert(m_device_ptr->get_type() == Anvil::DeviceType::MULTI_GPU);
                anvil_assert(mgpu_device_ptr          != nullptr);

                for (const auto& current_req : (*item_iterator)->alloc_mgpu_peer_memory_reqs)
                {
                    Anvil::PeerMemoryFeatureFlags current_memory_type_peer_memory_features;
                    const auto&                   local_device_index                       = current_req.first.first;
                    const auto                    local_device_ptr                         = mgpu_device_ptr->get_physical_device(local_device_index);
                    const auto&                   remote_device_index                      = current_req.first.second;
                    const auto                    remote_device_ptr                        = mgpu_device_ptr->get_physical_device(remote_device_index);
                    const auto&                   required_peer_memory_features            = current_req.second;

                    if (!mgpu_device_ptr->get_peer_memory_features(local_device_ptr,
                                                                   remote_device_ptr,
                                                                   memory_props.types.at(n_memory_type).heap_ptr->index,
                                                                  &current_memory_type_peer_memory_features) )
                    {
                        anvil_assert_fail();

                        can_continue = false;
                        break;
                    }

                    if ((current_memory_type_peer_memory_features & required_peer_memory_features) != required_peer_memory_features)
                    {
                        can_continue = false;

                        break;
                    }
                }

                if (!can_continue)
                {
                    result = false;

                    goto end;
                }
            }

            if ((*item_iterator)->alloc_is_dedicated_memory                   ||
                (*item_iterator)->alloc_exportable_external_handle_types != 0)
            {
                unique_allocs.push_back(UniqueAllocationInfo(item_iterator->get(),
                                                             n_memory_type));
            }
            else
            {
                const MemoryUniqueInfo unique_info((*item_iterator)->alloc_device_mask,
                                                   (*item_iterator)->memory_priority);

                if (device_mask_to_mem_type_to_item_vec.find(unique_info) == device_mask_to_mem_type_to_item_vec.end() )
                {
                    device_mask_to_mem_type_to_item_vec[unique_info].resize(n_memory_types);
                }

                device_mask_to_mem_type_to_item_vec.at(unique_info).at(n_memory_type).push_back((item_iterator->get() ) );
            }

            break;
        }
    }

    /* For each dedicated allocation, create one and associate it with the parent object. */
    for (const auto& current_unique_alloc : unique_allocs)
    {
        Anvil::MemoryBlockUniquePtr new_memory_block_ptr_derived(nullptr,
                                                                 std::default_delete<Anvil::MemoryBlock>() );
        Anvil::MemoryBlockUniquePtr new_memory_block_ptr_regular(nullptr,
                                                                 std::default_delete<Anvil::MemoryBlock>() );

        /* Bake the block and stash it */
        {
            auto create_info_ptr = Anvil::MemoryBlockCreateInfo::create_regular(m_device_ptr,
                                                                                1u << current_unique_alloc.n_memory_type,
                                                                                current_unique_alloc.item_ptr->alloc_size,
                                                                                (memory_props.types[current_unique_alloc.n_memory_type].features) );

            create_info_ptr->set_memory_priority(current_unique_alloc.item_ptr->memory_priority);
            create_info_ptr->set_device_mask    (current_unique_alloc.item_ptr->alloc_device_mask);
            create_info_ptr->set_mt_safety      (Anvil::Utils::convert_boolean_to_mt_safety_enum(m_device_ptr->is_mt_safe()) );

            if (current_unique_alloc.item_ptr->alloc_is_dedicated_memory)
            {
                create_info_ptr->use_dedicated_allocation(current_unique_alloc.item_ptr->buffer_ptr,
                                                          current_unique_alloc.item_ptr->image_ptr);
            }

            if (current_unique_alloc.item_ptr->alloc_exportable_external_handle_types != 0)
            {
                create_info_ptr->set_exportable_external_memory_handle_types(current_unique_alloc.item_ptr->alloc_exportable_external_handle_types);
            }

            #if defined(_WIN32)
            {
                if (current_unique_alloc.item_ptr->alloc_external_nt_handle_info_ptr != nullptr)
                {
                    create_info_ptr->set_exportable_nt_handle_info(current_unique_alloc.item_ptr->alloc_external_nt_handle_info_ptr->attributes_ptr,
                                                                   current_unique_alloc.item_ptr->alloc_external_nt_handle_info_ptr->access,
                                                                   current_unique_alloc.item_ptr->alloc_external_nt_handle_info_ptr->name);
                }
            }
            #endif

            new_memory_block_ptr_regular = Anvil::MemoryBlock::create(std::move(create_info_ptr) );
        }

        if (new_memory_block_ptr_regular == nullptr)
        {
            anvil_assert(new_memory_block_ptr_regular != nullptr);

            result = false;
            continue;
        }

        {
            auto create_info_ptr = Anvil::MemoryBlockCreateInfo::create_derived(new_memory_block_ptr_regular.get(),
                                                                                0, /* in_start_offset */
                                                                                current_unique_alloc.item_ptr->alloc_size);

            new_memory_block_ptr_derived = Anvil::MemoryBlock::create(std::move(create_info_ptr) );

            current_unique_alloc.item_ptr->alloc_memory_block_ptr = std::move(new_memory_block_ptr_regular);
            current_unique_alloc.item_ptr->is_baked               = (current_unique_alloc.item_ptr->alloc_memory_block_ptr != nullptr);
        }

        dynamic_cast<IMemoryBlockBackendSupport*>(current_unique_alloc.item_ptr->alloc_memory_block_ptr.get() )->set_parent_memory_allocator_backend_ptr(shared_from_this(),
                                                                                                                                                         reinterpret_cast<void*>(new_memory_block_ptr_derived->get_memory() ));

        m_memory_blocks.push_back(
            std::move(new_memory_block_ptr_derived)
        );
    }

    /* For each memory type, for each there's at least one item, bake a memory block */
    {
        for (auto device_mask_to_mem_type_to_item_vector_iterator  = device_mask_to_mem_type_to_item_vec.begin();
                  device_mask_to_mem_type_to_item_vector_iterator != device_mask_to_mem_type_to_item_vec.end();
                ++device_mask_to_mem_type_to_item_vector_iterator)
        {
            std::map<Anvil::MemoryAllocator::Item*, VkDeviceSize> alloc_offset_map;
            auto&                                                 current_memory_info_to_item_vector_data = *device_mask_to_mem_type_to_item_vector_iterator;
            const auto&                                           current_memory_info                     = current_memory_info_to_item_vector_data.first;
            uint32_t                                              current_memory_type_index               = 0;

            for (const auto& current_items : current_memory_info_to_item_vector_data.second)
            {
                if (current_items.size() > 0)
                {
                    Anvil::MemoryBlockUniquePtr new_memory_block_ptr(nullptr,
                                                                     std::default_delete<Anvil::MemoryBlock>() );
                    VkDeviceSize                n_bytes_required    (0);

                    /* Go through the items, calculate offsets and the total amount of memory we're going
                     * to need to alloc off the heap */
                    {
                        bool                          is_prev_item_linear = false;
                        Anvil::MemoryAllocator::Item* prev_item_ptr       = nullptr;

                        for (auto& current_item_ptr : current_items)
                        {
                            const bool is_current_item_buffer  = (current_item_ptr->type == Anvil::MemoryAllocator::ITEM_TYPE_BUFFER                   ||
                                                                  current_item_ptr->type == Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_BUFFER_REGION);
                            const bool is_current_item_image   = (current_item_ptr->type == Anvil::MemoryAllocator::ITEM_TYPE_IMAGE_WHOLE              ||
                                                                  current_item_ptr->type == Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_IMAGE_MIPTAIL     ||
                                                                  current_item_ptr->type == Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_IMAGE_SUBRESOURCE);
                            const bool is_current_item_linear = (is_current_item_buffer)                                                                                                  ||
                                                                (is_current_item_image && current_item_ptr->image_ptr->get_create_info_ptr()->get_tiling() == Anvil::ImageTiling::LINEAR);

                            anvil_assert(current_item_ptr->alloc_exportable_external_handle_types == 0);

                            #if defined(_WIN32)
                                anvil_assert(current_item_ptr->alloc_external_nt_handle_info_ptr == nullptr);
                            #endif

                            n_bytes_required = Anvil::Utils::round_up(n_bytes_required,
                                                                      current_item_ptr->alloc_memory_required_alignment);

                            if (prev_item_ptr != nullptr)
                            {
                                /* Make sure to adhere to the buffer-image granularity requirement */
                                if (is_prev_item_linear != is_current_item_linear)
                                {
                                    n_bytes_required = Anvil::Utils::round_up(n_bytes_required,
                                                                              m_device_ptr->get_physical_device_properties().core_vk1_0_properties_ptr->limits.buffer_image_granularity);
                                }
                            }

                            alloc_offset_map[current_item_ptr]  = n_bytes_required;
                            n_bytes_required                   += current_item_ptr->alloc_size;

                            is_prev_item_linear = is_current_item_linear;
                            prev_item_ptr       = current_item_ptr;
                        }
                    }

                    /* Bake the block and stash it */
                    {
                        auto create_info_ptr = Anvil::MemoryBlockCreateInfo::create_regular(m_device_ptr,
                                                                                            1u << current_memory_type_index,
                                                                                            n_bytes_required,
                                                                                            (memory_props.types[current_memory_type_index].features) );

                        create_info_ptr->set_memory_priority(current_memory_info_to_item_vector_data.first.memory_priority);
                        create_info_ptr->set_device_mask    (current_memory_info.device_mask);
                        create_info_ptr->set_mt_safety      (Anvil::Utils::convert_boolean_to_mt_safety_enum(m_device_ptr->is_mt_safe()) );

                        new_memory_block_ptr = Anvil::MemoryBlock::create(std::move(create_info_ptr) );
                    }

                    if (new_memory_block_ptr == nullptr)
                    {
                        anvil_assert(new_memory_block_ptr != nullptr);

                        result = false;
                        continue;
                    }

                    /* Go through the items again and assign the result memory block */
                    for (auto& current_item_ptr : current_items)
                    {
                        {
                            auto create_info_ptr = Anvil::MemoryBlockCreateInfo::create_derived(new_memory_block_ptr.get(),
                                                                                                alloc_offset_map.at(current_item_ptr),
                                                                                                current_item_ptr->alloc_size);

                            current_item_ptr->alloc_memory_block_ptr = Anvil::MemoryBlock::create(std::move(create_info_ptr) );
                        }

                        if (current_item_ptr->alloc_memory_block_ptr != nullptr)
                        {
                            current_item_ptr->is_baked = true;
                        }

                        dynamic_cast<IMemoryBlockBackendSupport*>(current_item_ptr->alloc_memory_block_ptr.get() )->set_parent_memory_allocator_backend_ptr(shared_from_this(),
                                                                                                                                                            reinterpret_cast<void*>(new_memory_block_ptr->get_memory() ));
                    }

                    m_memory_blocks.push_back(
                        std::move(new_memory_block_ptr)
                    );
                }

                ++current_memory_type_index;
            }
        }
    }

    m_is_baked = true;

    /* One-shot backend is not able to handle cases where only a portion of scheduled
     * items was successfully assigned memory backing.
     */
    anvil_assert(result);

end:
    return result;
}

VkResult Anvil::MemoryAllocatorBackends::OneShot::map(void*        in_memory_object,
                                                      VkDeviceSize in_start_offset,
                                                      VkDeviceSize in_memory_block_start_offset,
                                                      VkDeviceSize in_size,
                                                      void**       out_result_ptr)
{
    ANVIL_REDUNDANT_VARIABLE(in_memory_block_start_offset);

    return Anvil::Vulkan::vkMapMemory(m_device_ptr->get_device_vk(),
                                      reinterpret_cast<VkDeviceMemory>(in_memory_object),
                                      in_start_offset,
                                      in_size,
                                      0, /* flags */
                                      out_result_ptr);
}

/** Tells whether or not the backend is ready to handle allocation request.
 *
 *  One-shot memory allocator backend will return true until the first bake() invocation,
  * after which it's always going to return false (even if not all items have been assigned
  * memory regions successfully
  **/
bool Anvil::MemoryAllocatorBackends::OneShot::supports_baking() const
{
    return !m_is_baked;
}

bool Anvil::MemoryAllocatorBackends::OneShot::supports_device_masks() const
{
    return true;
}

bool Anvil::MemoryAllocatorBackends::OneShot::supports_external_memory_handles(const Anvil::ExternalMemoryHandleTypeFlags&) const
{
    return true;
}

bool Anvil::MemoryAllocatorBackends::OneShot::supports_protected_memory() const
{
    return true;
}

void Anvil::MemoryAllocatorBackends::OneShot::unmap(void* in_memory_object)
{
    Anvil::Vulkan::vkUnmapMemory(m_device_ptr->get_device_vk(),
                                 reinterpret_cast<VkDeviceMemory>(in_memory_object) );
}
