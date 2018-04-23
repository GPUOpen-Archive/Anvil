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

#include "misc/debug.h"
#include "misc/memory_block_create_info.h"
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
Anvil::MemoryAllocatorBackends::VMA::VMAAllocator::VMAAllocator(const Anvil::BaseDevice* in_device_ptr)
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
std::shared_ptr<Anvil::MemoryAllocatorBackends::VMA::VMAAllocator> Anvil::MemoryAllocatorBackends::VMA::VMAAllocator::create(const Anvil::BaseDevice* in_device_ptr)
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
    VmaAllocatorCreateInfo create_info = {};
    VkResult               result        (VK_ERROR_DEVICE_LOST);

    switch (m_device_ptr->get_type() )
    {
        case Anvil::DEVICE_TYPE_SINGLE_GPU:
        {
            const Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

            create_info.physicalDevice = sgpu_device_ptr->get_physical_device()->get_physical_device();
            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }

    create_info.device                      = m_device_ptr->get_device_vk();
    create_info.pAllocationCallbacks        = nullptr;
    create_info.preferredLargeHeapBlockSize = 0;

    result = vmaCreateAllocator(&create_info,
                                &m_allocator);

    anvil_assert_vk_call_succeeded(result);
    return is_vk_call_successful(result);
}

/** Please see header for specification */
Anvil::MemoryAllocatorBackends::VMA::VMA(const Anvil::BaseDevice* in_device_ptr)
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
        MemoryBlockUniquePtr new_memory_block_ptr(nullptr,
                                                  std::default_delete<Anvil::MemoryBlock>() );

        VmaAllocation                               allocation                  = VK_NULL_HANDLE;
        VmaAllocationCreateInfo                     allocation_create_info      = {};
        VmaAllocationInfo                           allocation_info             = {};
        VkMemoryRequirements                        memory_requirements_vk;
        Anvil::OnMemoryBlockReleaseCallbackFunction release_callback_function;
        VkMemoryHeapFlags                           required_mem_heap_flags     = 0;
        VkMemoryPropertyFlags                       required_mem_property_flags = 0;

        Anvil::Utils::get_vk_property_flags_from_memory_feature_flags(current_item_ptr->alloc_memory_required_features,
                                                                     &required_mem_property_flags,
                                                                     &required_mem_heap_flags);

        /* NOTE: VMA does not take required memory heap flags at the moment. Adding this is on their radar. */
        anvil_assert(required_mem_heap_flags == 0);

        memory_requirements_vk.alignment      = current_item_ptr->alloc_memory_required_alignment;
        memory_requirements_vk.memoryTypeBits = current_item_ptr->alloc_memory_supported_memory_types;
        memory_requirements_vk.size           = current_item_ptr->alloc_size;

        allocation_create_info.requiredFlags = required_mem_property_flags;

        result_vk = vmaAllocateMemory(m_vma_allocator_ptr->get_handle(),
                                     &memory_requirements_vk,
                                     &allocation_create_info,
                                     &allocation,
                                     &allocation_info);

        if (!is_vk_call_successful(result_vk) )
        {
            result = false;

            continue;
        }

        /* Bake the block and stash it */
        release_callback_function = std::bind(
            &VMAAllocator::on_vma_alloced_mem_block_gone_out_of_scope,
            m_vma_allocator_ptr,
            std::placeholders::_1,
            allocation
        );

        {
            auto create_info_ptr = Anvil::MemoryBlockCreateInfo::create_derived_with_custom_delete_proc(m_device_ptr,
                                                                                                        allocation_info.deviceMemory,
                                                                                                        memory_requirements_vk.memoryTypeBits,
                                                                                                        current_item_ptr->alloc_memory_required_features,
                                                                                                        allocation_info.memoryType,
                                                                                                        memory_requirements_vk.size,
                                                                                                        allocation_info.offset,
                                                                                                        release_callback_function);

            new_memory_block_ptr = Anvil::MemoryBlock::create(std::move(create_info_ptr) );
        }

        if (new_memory_block_ptr == nullptr)
        {
            anvil_assert(new_memory_block_ptr != nullptr);

            result = false;
            continue;
        }

        dynamic_cast<IMemoryBlockBackendSupport*>(new_memory_block_ptr.get() )->set_parent_memory_allocator_backend_ptr(shared_from_this(),
                                                                                                                        allocation);

        current_item_ptr->alloc_memory_block_ptr = std::move(new_memory_block_ptr);
        current_item_ptr->alloc_size             = memory_requirements_vk.size;
        current_item_ptr->is_baked               = true;

        m_vma_allocator_ptr->on_new_vma_mem_block_alloced();
    }

    return result;
}

/** Please see header for specification */
std::unique_ptr<Anvil::MemoryAllocatorBackends::VMA> Anvil::MemoryAllocatorBackends::VMA::create(const Anvil::BaseDevice* in_device_ptr)
{
    std::unique_ptr<Anvil::MemoryAllocatorBackends::VMA> result_ptr;

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

VkResult Anvil::MemoryAllocatorBackends::VMA::map(void*        in_memory_object,
                                                  VkDeviceSize in_start_offset,
                                                  VkDeviceSize in_size,
                                                  void**       out_result_ptr)
{
    ANVIL_REDUNDANT_ARGUMENT(in_size);
    ANVIL_REDUNDANT_ARGUMENT(in_start_offset);

    anvil_assert(in_start_offset == 0);

    return vmaMapMemory(m_vma_allocator_ptr->get_handle(),
                        static_cast<VmaAllocation>(in_memory_object),
                        out_result_ptr);
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
void Anvil::MemoryAllocatorBackends::VMA::VMAAllocator::on_vma_alloced_mem_block_gone_out_of_scope(Anvil::MemoryBlock* in_memory_block_ptr,
                                                                                                   VmaAllocation       in_vma_allocation)
{
    /* Only physically deallocate those memory blocks that are not derivatives of another memory blocks! */
    if (in_memory_block_ptr->get_create_info_ptr()->get_parent_memory_block() == nullptr)
    {
        vmaFreeMemory(get_handle(),
                      in_vma_allocation);

        /* Remove one cached pointer to the VMA wrapper class instance. This means that VMA instance
         * is going to be destroyed in case the vector's size reaches zero!
         */
        anvil_assert(m_refcount_helper.size() >= 1);

        m_refcount_helper.pop_back();

        /* WARNING: *this is potentially out of scope from this point onward!: */
    }
}

/** Always returns true */
bool Anvil::MemoryAllocatorBackends::VMA::supports_baking() const
{
    return true;
}

bool Anvil::MemoryAllocatorBackends::VMA::supports_external_memory_handles(const Anvil::ExternalMemoryHandleTypeBits& in_external_memory_handle_types) const
{
    /* Vulkan Memory Allocator does NOT support external memory handles */
    ANVIL_REDUNDANT_VARIABLE_CONST(in_external_memory_handle_types);

    return false;
}

void Anvil::MemoryAllocatorBackends::VMA::unmap(void* in_memory_object)
{
    vmaUnmapMemory(m_vma_allocator_ptr->get_handle(),
                   static_cast<VmaAllocation>(in_memory_object) );
}
