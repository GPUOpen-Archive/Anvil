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
#include "misc/memory_allocator.h"
#include "misc/memory_block_create_info.h"
#include "wrappers/buffer.h"
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include <algorithm>

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
    const auto&                                              memory_props               (m_device_ptr->get_physical_device_memory_properties() );
    const uint32_t                                           n_memory_types             (static_cast<uint32_t>(memory_props.types.size() ));
    std::vector<std::vector<Anvil::MemoryAllocator::Item*> > per_mem_type_items_vector  (n_memory_types);
    bool                                                     result                     (true);

    /* Iterate over all block items and determine what memory types we can use.
     *
     * In certain cases, we may need to suballocate from more than one memory block,
     * due to the fact not all memory heaps may support features requested at
     * creation time.
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

            if ((memory_props.types.at(n_memory_type).features & required_memory_features) != required_memory_features)
            {
                continue;
            }

            per_mem_type_items_vector.at(n_memory_type).push_back((item_iterator->get() ) );
            break;
        }
    }

    /* For each memory type, for each there's at least one item, bake a memory block */
    {
        std::map<Anvil::MemoryAllocator::Item*, VkDeviceSize> alloc_offset_map;
        uint32_t                                              current_memory_type_index(0);

        for (auto mem_type_to_item_vector_iterator  = per_mem_type_items_vector.begin();
                  mem_type_to_item_vector_iterator != per_mem_type_items_vector.end();
                ++mem_type_to_item_vector_iterator, ++current_memory_type_index)
        {
            auto& current_item_vector = *mem_type_to_item_vector_iterator;

            if (current_item_vector.size() > 0)
            {
                Anvil::MemoryBlockUniquePtr new_memory_block_ptr(nullptr,
                                                                 std::default_delete<Anvil::MemoryBlock>() );
                VkDeviceSize                n_bytes_required    (0);

                /* Go through the items, calculate offsets and the total amount of memory we're going
                 * to need to alloc off the heap */
                for (auto& current_item_ptr : current_item_vector)
                {
                    n_bytes_required = Anvil::Utils::round_up(n_bytes_required,
                                                              current_item_ptr->alloc_memory_required_alignment);

                    alloc_offset_map[current_item_ptr]  = n_bytes_required;
                    n_bytes_required                   += current_item_ptr->alloc_size;
                }

                /* Bake the block and stash it */
                {
                    auto create_info_ptr = Anvil::MemoryBlockCreateInfo::create_regular(m_device_ptr,
                                                                                        1u << current_memory_type_index,
                                                                                        n_bytes_required,
                                                                                        (memory_props.types[current_memory_type_index].features) );

                    create_info_ptr->set_mt_safety(Anvil::Utils::convert_boolean_to_mt_safety_enum(m_device_ptr->is_mt_safe()) );

                    new_memory_block_ptr = Anvil::MemoryBlock::create(std::move(create_info_ptr) );
                }

                if (new_memory_block_ptr == nullptr)
                {
                    anvil_assert(new_memory_block_ptr != nullptr);

                    result = false;
                    continue;
                }

                /* Go through the items again and assign the result memory block */
                for (auto& current_item_ptr : current_item_vector)
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
        }
    }

    m_is_baked = true;

    /* One-shot backend is not able to handle cases where only a portion of scheduled
     * items was successfully assigned memory backing.
     */
    anvil_assert(result);

    return result;
}

VkResult Anvil::MemoryAllocatorBackends::OneShot::map(void*        in_memory_object,
                                                      VkDeviceSize in_start_offset,
                                                      VkDeviceSize in_size,
                                                      void**       out_result_ptr)
{
    return vkMapMemory(m_device_ptr->get_device_vk(),
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

bool Anvil::MemoryAllocatorBackends::OneShot::supports_external_memory_handles(const Anvil::ExternalMemoryHandleTypeBits& in_external_memory_handle_types) const
{
    return (in_external_memory_handle_types == 0);
}

void Anvil::MemoryAllocatorBackends::OneShot::unmap(void* in_memory_object)
{
    vkUnmapMemory(m_device_ptr->get_device_vk(),
                  reinterpret_cast<VkDeviceMemory>(in_memory_object) );
}
