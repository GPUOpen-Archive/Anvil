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
    VmaAllocatorCreateInfo create_info                        = {};
    const bool             khr_dedicated_allocation_supported = m_device_ptr->get_extension_info()->khr_dedicated_allocation();
    VkResult               result                             = VK_ERROR_DEVICE_LOST;

    /* Prepare VK func ptr array */
    m_vma_func_ptrs.reset(
        new VmaVulkanFunctions()
    );

    if (m_vma_func_ptrs == nullptr)
    {
        anvil_assert(m_vma_func_ptrs != nullptr);

        goto end;
    }

    m_vma_func_ptrs->vkAllocateMemory                    = Vulkan::vkAllocateMemory;
    m_vma_func_ptrs->vkBindBufferMemory                  = Vulkan::vkBindBufferMemory;
    m_vma_func_ptrs->vkBindImageMemory                   = Vulkan::vkBindImageMemory;
    m_vma_func_ptrs->vkCreateBuffer                      = Vulkan::vkCreateBuffer;
    m_vma_func_ptrs->vkCreateImage                       = Vulkan::vkCreateImage;
    m_vma_func_ptrs->vkDestroyBuffer                     = Vulkan::vkDestroyBuffer;
    m_vma_func_ptrs->vkDestroyImage                      = Vulkan::vkDestroyImage;
    m_vma_func_ptrs->vkFreeMemory                        = Vulkan::vkFreeMemory;
    m_vma_func_ptrs->vkGetBufferMemoryRequirements       = Vulkan::vkGetBufferMemoryRequirements;
    m_vma_func_ptrs->vkGetImageMemoryRequirements        = Vulkan::vkGetImageMemoryRequirements;
    m_vma_func_ptrs->vkGetPhysicalDeviceMemoryProperties = Vulkan::vkGetPhysicalDeviceMemoryProperties;
    m_vma_func_ptrs->vkGetPhysicalDeviceProperties       = Vulkan::vkGetPhysicalDeviceProperties;
    m_vma_func_ptrs->vkMapMemory                         = Vulkan::vkMapMemory;
    m_vma_func_ptrs->vkUnmapMemory                       = Vulkan::vkUnmapMemory;

    if (m_device_ptr->get_extension_info()->khr_get_memory_requirements2() )
    {
        m_vma_func_ptrs->vkGetBufferMemoryRequirements2KHR = m_device_ptr->get_extension_khr_get_memory_requirements2_entrypoints().vkGetBufferMemoryRequirements2KHR;
        m_vma_func_ptrs->vkGetImageMemoryRequirements2KHR  = m_device_ptr->get_extension_khr_get_memory_requirements2_entrypoints().vkGetImageMemoryRequirements2KHR;
    }
    else
    {
        m_vma_func_ptrs->vkGetBufferMemoryRequirements2KHR = nullptr;
        m_vma_func_ptrs->vkGetImageMemoryRequirements2KHR  = nullptr;
    }

    /* Prepare VMA create info struct */
    switch (m_device_ptr->get_type() )
    {
        case Anvil::DeviceType::MULTI_GPU:
        {
            /* VMA library takes a physical device handle to extract info regarding supported
             * memory types and the like. As VK_KHR_device_group provide explicit mGPU support,
             * it is guaranteed all physical devices within a logical device offer exactly the
             * same capabilities. This means we're safe to pass zeroth physical device to the
             * library, and everything will still be OK.
             */
            const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );

            create_info.physicalDevice = mgpu_device_ptr->get_physical_device(0)->get_physical_device();
            break;
        }

        case Anvil::DeviceType::SINGLE_GPU:
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

    create_info.flags                       = (khr_dedicated_allocation_supported) ? VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT : 0;
    create_info.device                      = m_device_ptr->get_device_vk();
    create_info.pAllocationCallbacks        = nullptr;
    create_info.preferredLargeHeapBlockSize = 0;
    create_info.pVulkanFunctions            = m_vma_func_ptrs.get();

    result = vmaCreateAllocator(&create_info,
                                &m_allocator);

    anvil_assert_vk_call_succeeded(result);
end:
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
        anvil_assert(current_item_ptr->memory_priority == FLT_MAX); /* VMA doesn't support memory_priority */

        MemoryBlockUniquePtr new_memory_block_ptr(nullptr,
                                                  std::default_delete<Anvil::MemoryBlock>() );

        VmaAllocation                               allocation                  = VK_NULL_HANDLE;
        VmaAllocationCreateInfo                     allocation_create_info      = {};
        VmaAllocationInfo                           allocation_info             = {};
        bool                                        is_dedicated_alloc          = false;
        VkMemoryRequirements                        memory_requirements_vk;
        Anvil::OnMemoryBlockReleaseCallbackFunction release_callback_function;
        Anvil::MemoryHeapFlags                      required_mem_heap_flags;
        Anvil::MemoryPropertyFlags                  required_mem_property_flags;

        Anvil::Utils::get_vk_property_flags_from_memory_feature_flags(current_item_ptr->alloc_memory_required_features,
                                                                     &required_mem_property_flags,
                                                                     &required_mem_heap_flags);

        /* NOTE: VMA does not take required memory heap flags at the moment. Adding this is on their radar. */
        anvil_assert(required_mem_heap_flags == Anvil::MemoryHeapFlagBits::NONE);

        memory_requirements_vk.alignment      = current_item_ptr->alloc_memory_required_alignment;
        memory_requirements_vk.memoryTypeBits = current_item_ptr->alloc_memory_supported_memory_types;
        memory_requirements_vk.size           = current_item_ptr->alloc_size;

        allocation_create_info.flags         = (current_item_ptr->alloc_is_dedicated_memory) ? VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
                                                                                             : 0;
        allocation_create_info.requiredFlags = required_mem_property_flags.get_vk();

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
        else
        {
            is_dedicated_alloc = (allocation->GetType() == VmaAllocation_T::ALLOCATION_TYPE_DEDICATED);
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

            if (is_dedicated_alloc)
            {
                if (current_item_ptr->type == Anvil::MemoryAllocator::ITEM_TYPE_BUFFER               ||
                    current_item_ptr->type == Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_BUFFER_REGION)
                {
                    anvil_assert(current_item_ptr->buffer_ptr != nullptr);

                    create_info_ptr->use_dedicated_allocation(current_item_ptr->buffer_ptr,
                                                              nullptr); /* in_opt_image_ptr */
                }
                else
                if (current_item_ptr->type == Anvil::MemoryAllocator::ITEM_TYPE_IMAGE_WHOLE              ||
                    current_item_ptr->type == Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_IMAGE_MIPTAIL     ||
                    current_item_ptr->type == Anvil::MemoryAllocator::ITEM_TYPE_SPARSE_IMAGE_SUBRESOURCE)
                {
                    anvil_assert(current_item_ptr->image_ptr != nullptr);

                    create_info_ptr->use_dedicated_allocation(nullptr, /* in_opt_buffer_ptr */
                                                              current_item_ptr->image_ptr);
                }
            }

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
                                                  VkDeviceSize in_memory_block_start_offset,
                                                  VkDeviceSize in_size,
                                                  void**       out_result_ptr)
{
    VkResult result;
    void*    result_ptr = nullptr;

    ANVIL_REDUNDANT_ARGUMENT(in_size);
    ANVIL_REDUNDANT_ARGUMENT(in_start_offset);

    anvil_assert(in_start_offset == 0);

    result = vmaMapMemory(m_vma_allocator_ptr->get_handle(),
                          static_cast<VmaAllocation>(in_memory_object),
                         &result_ptr);

    result_ptr = reinterpret_cast<uint8_t*>(result_ptr) - in_memory_block_start_offset;

    *out_result_ptr = result_ptr;
    return result;
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

bool Anvil::MemoryAllocatorBackends::VMA::supports_device_masks() const
{
    /* The version of VMA we currently use does not provide support for device groups. */
    return false;
}

bool Anvil::MemoryAllocatorBackends::VMA::supports_external_memory_handles(const Anvil::ExternalMemoryHandleTypeFlags& in_external_memory_handle_types) const
{
    /* Vulkan Memory Allocator does NOT support external memory handles */
    ANVIL_REDUNDANT_VARIABLE_CONST(in_external_memory_handle_types);

    return false;
}

bool Anvil::MemoryAllocatorBackends::VMA::supports_protected_memory() const
{
    /* Vulkan Memory Allocator does NOT support VK 1.1 features */
    return false;
}

void Anvil::MemoryAllocatorBackends::VMA::unmap(void* in_memory_object)
{
    vmaUnmapMemory(m_vma_allocator_ptr->get_handle(),
                   static_cast<VmaAllocation>(in_memory_object) );
}
