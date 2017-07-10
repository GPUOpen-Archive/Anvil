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
#include "wrappers/device.h"
#include "wrappers/memory_block.h"
#include "wrappers/physical_device.h"

/* Inject Vulkan Memory Allocator impl (ignore any warnings reported for the library) ==> */
#define VMA_IMPLEMENTATION 

#ifdef max
    #undef max
#endif
#ifdef min
    #undef min
#endif

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4127) /* conditional expression is constant */
    #pragma warning(disable : 4239) /* nonstandard extension used: 'argument': conversion from 'X' to 'X&' */
#endif

#include "misc/memalloc_backends/backend_vma.h"

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

/* <== */


/* Please see header for specification */
Anvil::MemoryAllocatorBackends::VMA::VMAAllocator::VMAAllocator(std::weak_ptr<Anvil::BaseDevice> in_device_ptr)
    :m_allocator (nullptr),
     m_device_ptr(in_device_ptr)
{
    /* Stub */
}

/* Please see header for specification */
Anvil::MemoryAllocatorBackends::VMA::VMAAllocator::~VMAAllocator()
{
    if (m_allocator != nullptr)
    {
        vmaDestroyAllocator(m_allocator);

        m_allocator = nullptr;
    }
}

/* Please see header for specifications */
std::shared_ptr<Anvil::MemoryAllocatorBackends::VMA::VMAAllocator> Anvil::MemoryAllocatorBackends::VMA::VMAAllocator::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr)
{
    std::shared_ptr<Anvil::MemoryAllocatorBackends::VMA::VMAAllocator> result_ptr;

    result_ptr.reset(
        new VMAAllocator(in_device_ptr)
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init())
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/** Creates a new VMA allocator instance.
 *
 *  @return true if successful, false otherwise.
 **/
bool Anvil::MemoryAllocatorBackends::VMA::VMAAllocator::init()
{
    VmaAllocatorCreateInfo             create_info;
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkResult                           result           (VK_ERROR_DEVICE_LOST);

    switch (device_locked_ptr->get_type() )
    {
        case Anvil::DEVICE_TYPE_SINGLE_GPU:
        {
            std::shared_ptr<Anvil::SGPUDevice> sgpu_device_locked_ptr(std::dynamic_pointer_cast<Anvil::SGPUDevice>(device_locked_ptr) );

            create_info.physicalDevice = sgpu_device_locked_ptr->get_physical_device().lock()->get_physical_device();
            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }

    create_info.device                      = device_locked_ptr->get_device_vk();
    create_info.pAllocationCallbacks        = nullptr;
    create_info.preferredLargeHeapBlockSize = 0;
    create_info.preferredSmallHeapBlockSize = 0;

    result = vmaCreateAllocator(&create_info,
                                &m_allocator);

    anvil_assert_vk_call_succeeded(result);
    return is_vk_call_successful(result);
}

/** Please see header for specification */
Anvil::MemoryAllocatorBackends::VMA::VMA(std::weak_ptr<Anvil::BaseDevice> in_device_ptr)
    :m_device_ptr(in_device_ptr)
{
    /* Stub */
}

/** Please see header for specification */
Anvil::MemoryAllocatorBackends::VMA::~VMA()
{
    /* Stub */
}

/** For each specified Memory Allocator's Item, the function asks VMA for a memory region that
 *  can be assigned to corresponding wrapper instance. For each successfully handled request,
 *  a MemoryBlock instance is created, using the feedback provided by the library.
 *
 *  This function can be called multiple times.
 *
 *  @return true if all allocations have been handled successfully, false if there was at least
 *               one failure.
 **/
bool Anvil::MemoryAllocatorBackends::VMA::bake(Anvil::MemoryAllocator::Items& in_items)
{
    bool     result    = true;
    VkResult result_vk = VK_ERROR_DEVICE_LOST;

    /* Go through all scheduled items and call the underlying library API to handle the request.
     *
     * For each successful allocation, wrap it with a MemoryBlock wrapper with a custom delete
     * handler, so that VMA is notified whenever a memory block it has provided memory backing for
     * has gone out of scope.
     */
    for (auto& current_item_ptr : in_items)
    {
        VkMemoryRequirements                memory_requirements_vk;
        VmaMemoryRequirements               memory_requirements_vma;
        std::shared_ptr<Anvil::MemoryBlock> new_memory_block_ptr;
        VkMemoryHeapFlags                   required_mem_heap_flags     = 0;
        VkMemoryPropertyFlags               required_mem_property_flags = 0;
        uint32_t                            result_mem_type_index       = UINT32_MAX;
        VkMappedMemoryRange                 result_mem_range;

        Anvil::Utils::get_vk_property_flags_from_memory_feature_flags(current_item_ptr->alloc_memory_required_features,
                                                                     &required_mem_property_flags,
                                                                     &required_mem_heap_flags);

        /* NOTE: VMA does not take required memory heap flags at the moment. Adding this is on their radar. */
        anvil_assert(required_mem_heap_flags == 0);

        memory_requirements_vk.alignment      = current_item_ptr->alloc_memory_required_alignment;
        memory_requirements_vk.memoryTypeBits = current_item_ptr->alloc_memory_supported_memory_types;
        memory_requirements_vk.size           = current_item_ptr->alloc_size;

        memory_requirements_vma.neverAllocate  = false;
        memory_requirements_vma.ownMemory      = false;
        memory_requirements_vma.preferredFlags = 0;
        memory_requirements_vma.requiredFlags  = required_mem_property_flags;
        memory_requirements_vma.usage          = VMA_MEMORY_USAGE_UNKNOWN;

        result_vk = vmaAllocateMemory(m_vma_allocator_ptr->get_handle(),
                                     &memory_requirements_vk,
                                     &memory_requirements_vma,
                                     &result_mem_range,
                                     &result_mem_type_index);

        if (!is_vk_call_successful(result_vk) )
        {
            result = false;

            continue;
        }

        anvil_assert(result_mem_range.pNext == nullptr);
        anvil_assert(result_mem_range.sType == VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE);

        /* Bake the block and stash it */
        new_memory_block_ptr = Anvil::MemoryBlock::create_derived_with_custom_delete_proc(m_device_ptr,
                                                                                          result_mem_range.memory,
                                                                                          memory_requirements_vk.memoryTypeBits,
                                                                                          current_item_ptr->alloc_memory_required_features,
                                                                                          result_mem_type_index,
                                                                                          memory_requirements_vk.size,
                                                                                          result_mem_range.offset,
                                                                                          VMAAllocator::on_vma_alloced_mem_block_gone_out_of_scope,
                                                                                          m_vma_allocator_ptr.get()
        );

        if (new_memory_block_ptr == nullptr)
        {
            anvil_assert(new_memory_block_ptr != nullptr);

            result = false;
            continue;
        }

        current_item_ptr->alloc_memory_block_ptr = new_memory_block_ptr;
        current_item_ptr->alloc_size             = memory_requirements_vk.size;
        current_item_ptr->is_baked               = true;

        m_vma_allocator_ptr->on_new_vma_mem_block_alloced();
    }

    return result;
}

/** Please see header for specification */
std::shared_ptr<Anvil::MemoryAllocatorBackends::VMA> Anvil::MemoryAllocatorBackends::VMA::create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr)
{
    std::shared_ptr<Anvil::MemoryAllocatorBackends::VMA> result_ptr;

    result_ptr.reset(
        new VMA(in_device_ptr)
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/** Creates and stores a new VMAAllocator instance.
 *
 *  @return true if successful, false otherwise.
 */
bool Anvil::MemoryAllocatorBackends::VMA::init()
{
    m_vma_allocator_ptr = VMAAllocator::create(m_device_ptr);

    return (m_vma_allocator_ptr != nullptr);
}

/** Please see header for specification */
void Anvil::MemoryAllocatorBackends::VMA::VMAAllocator::on_new_vma_mem_block_alloced()
{
    /* The VMA class instance must remain alive as long as there are memory blocks created from blocks
     * managed by the Vulkan Memory Allocator library.
     *
     * For each alloc, we cache a copy of our own in order to increment the use count of the instance.
     * Every time the alloc is released back to the library, we remove a pointer off the vector.
     *
     * This prevents from premature release of the VMA wrapper instance if the user did not care to keep
     * a copy of a pointer to the allocator throughout Vulkan instance lifetime. */
    m_refcount_helper.push_back(shared_from_this() );
}

/** Please see header for specification */
void Anvil::MemoryAllocatorBackends::VMA::VMAAllocator::on_vma_alloced_mem_block_gone_out_of_scope(Anvil::MemoryBlock*              in_memory_block_ptr,
                                                                                                   void*                            in_user_arg)
{
    VkMappedMemoryRange mem_range;
    VMAAllocator*       vma_allocator_ptr = reinterpret_cast<VMAAllocator*>(in_user_arg);

    anvil_assert(vma_allocator_ptr != nullptr);

    /* Only physically deallocate those memory blocks that are not derivatives of another memory blocks! */
    if (in_memory_block_ptr->get_parent_memory_block() == nullptr)
    {
        mem_range.memory = in_memory_block_ptr->get_memory      ();
        mem_range.offset = in_memory_block_ptr->get_start_offset();
        mem_range.pNext  = nullptr;
        mem_range.size   = in_memory_block_ptr->get_size();
        mem_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

        vmaFreeMemory(vma_allocator_ptr->get_handle(),
                     &mem_range);

        /* Remove one cached pointer to the VMA wrapper class instance. This means that VMA instance
         * is going to be destroyed in case the vector's size reaches zero!
         */
        anvil_assert(vma_allocator_ptr->m_refcount_helper.size() >= 1);

        vma_allocator_ptr->m_refcount_helper.pop_back();

        /* WARNING: vma_allocator_ptr is potentially nullptr from this point onward!: */
    }
}

/** Always returns true */
bool Anvil::MemoryAllocatorBackends::VMA::supports_baking() const
{
    return true;
}