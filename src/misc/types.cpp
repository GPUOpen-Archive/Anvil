//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
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
#include "wrappers/buffer.h"
#include "wrappers/image.h"
#include "wrappers/shader_module.h"

/** Please see header for specification */
Anvil::BufferBarrier::BufferBarrier(const BufferBarrier& in)
{
    buffer                 = in.buffer;
    buffer_barrier_vk      = in.buffer_barrier_vk;
    buffer_ptr             = in.buffer_ptr;
    dst_access_mask        = in.dst_access_mask;
    dst_queue_family_index = in.dst_queue_family_index;
    offset                 = in.offset;
    size                   = in.size;
    src_access_mask        = in.src_access_mask;
    src_queue_family_index = in.src_queue_family_index;

    if (buffer_ptr != nullptr)
    {
        buffer_ptr->retain();
    }
}

/** Please see header for specification */
Anvil::BufferBarrier::BufferBarrier(VkAccessFlags  in_source_access_mask,
                                    VkAccessFlags  in_destination_access_mask,
                                    uint32_t       in_src_queue_family_index,
                                    uint32_t       in_dst_queue_family_index,
                                    Anvil::Buffer* in_buffer_ptr,
                                    VkDeviceSize   in_offset,
                                    VkDeviceSize   in_size)
{
    buffer                 = in_buffer_ptr->get_buffer();
    buffer_ptr             = in_buffer_ptr;
    dst_access_mask        = static_cast<VkAccessFlagBits>(in_destination_access_mask);
    dst_queue_family_index = in_dst_queue_family_index;
    offset                 = in_offset;
    size                   = in_size;
    src_access_mask        = static_cast<VkAccessFlagBits>(in_source_access_mask);
    src_queue_family_index = in_src_queue_family_index;

    buffer_barrier_vk.buffer              = in_buffer_ptr->get_buffer();
    buffer_barrier_vk.dstAccessMask       = in_destination_access_mask;
    buffer_barrier_vk.dstQueueFamilyIndex = in_dst_queue_family_index;
    buffer_barrier_vk.offset              = in_offset;
    buffer_barrier_vk.pNext               = nullptr;
    buffer_barrier_vk.size                = in_size;
    buffer_barrier_vk.srcAccessMask       = in_source_access_mask,
    buffer_barrier_vk.srcQueueFamilyIndex = in_src_queue_family_index;
    buffer_barrier_vk.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

    if (buffer_ptr != nullptr)
    {
        buffer_ptr->retain();
    }
}

/** Please see header for specification */
Anvil::BufferBarrier::~BufferBarrier()
{
    if (buffer_ptr != nullptr)
    {
        buffer_ptr->release();
    }
}

/** Please see header for specification */
Anvil::ImageBarrier::ImageBarrier(const ImageBarrier& in)
{
    dst_access_mask        = in.dst_access_mask;
    dst_queue_family_index = in.dst_queue_family_index;
    image                  = in.image;
    image_barrier_vk       = in.image_barrier_vk;
    image_ptr              = in.image_ptr;
    new_layout             = in.new_layout;
    old_layout             = in.old_layout;
    src_access_mask        = in.src_access_mask;
    src_queue_family_index = in.src_queue_family_index;
    subresource_range      = in.subresource_range;

    if (image_ptr != nullptr)
    {
        image_ptr->retain();
    }
}

/** Please see header for specification */
Anvil::ImageBarrier::ImageBarrier(VkAccessFlags           in_source_access_mask,
                                  VkAccessFlags           in_destination_access_mask,
                                  bool                    in_by_region_barrier,
                                  VkImageLayout           in_old_layout,
                                  VkImageLayout           in_new_layout,
                                  uint32_t                in_src_queue_family_index,
                                  uint32_t                in_dst_queue_family_index,
                                  Anvil::Image*           in_image_ptr,
                                  VkImageSubresourceRange in_image_subresource_range)
{
    by_region              = in_by_region_barrier;
    dst_access_mask        = static_cast<VkAccessFlagBits>(in_destination_access_mask);
    dst_queue_family_index = in_dst_queue_family_index;
    image                  = (in_image_ptr != VK_NULL_HANDLE) ? in_image_ptr->get_image()
                                                              : VK_NULL_HANDLE;
    image_ptr              = in_image_ptr;
    new_layout             = in_new_layout;
    old_layout             = in_old_layout;
    src_access_mask        = static_cast<VkAccessFlagBits>(in_source_access_mask);
    src_queue_family_index = in_src_queue_family_index;
    subresource_range      = in_image_subresource_range;

    image_barrier_vk.dstAccessMask       = in_destination_access_mask;
    image_barrier_vk.dstQueueFamilyIndex = in_dst_queue_family_index;
    image_barrier_vk.image               = (in_image_ptr != nullptr) ? in_image_ptr->get_image()
                                                                     : VK_NULL_HANDLE;
    image_barrier_vk.newLayout           = in_new_layout;
    image_barrier_vk.oldLayout           = in_old_layout;
    image_barrier_vk.pNext               = nullptr;
    image_barrier_vk.srcAccessMask       = in_source_access_mask;
    image_barrier_vk.srcQueueFamilyIndex = in_src_queue_family_index;
    image_barrier_vk.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier_vk.subresourceRange    = in_image_subresource_range;

    if (image_ptr != nullptr)
    {
        image_ptr->retain();
    }
}

/** Please see header for specification */
Anvil::ImageBarrier::~ImageBarrier()
{
    if (image_ptr != nullptr)
    {
        image_ptr->release();
    }
}

/* Please see header for specification */
void Anvil::MemoryProperties::init(const VkPhysicalDeviceMemoryProperties& mem_properties)
{
    heaps = new Anvil::MemoryHeap[mem_properties.memoryHeapCount];
    anvil_assert(heaps != nullptr);

    for (unsigned int n_heap = 0;
                      n_heap < mem_properties.memoryHeapCount;
                    ++n_heap)
    {
        heaps[n_heap].flags = static_cast<VkMemoryHeapFlagBits>(mem_properties.memoryHeaps[n_heap].flags);
        heaps[n_heap].size  = mem_properties.memoryHeaps[n_heap].size;
    }

    types.reserve(mem_properties.memoryTypeCount);

    for (unsigned int n_type = 0;
                      n_type < mem_properties.memoryTypeCount;
                    ++n_type)
    {
        types.push_back(MemoryType(mem_properties.memoryTypes[n_type],
                                   this) );
    }
}

/* Please see header for specification */
Anvil::MemoryType::MemoryType(const VkMemoryType&      type,
                              struct MemoryProperties* memory_props_ptr)
{
    flags    = static_cast<VkMemoryPropertyFlagBits>(type.propertyFlags);
    heap_ptr = &memory_props_ptr->heaps[type.heapIndex];
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint()
{
    name              = nullptr;
    shader_module_ptr = nullptr;
    stage             = SHADER_STAGE_UNKNOWN;
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const char*   in_name,
                                                                ShaderModule* in_shader_module_ptr,
                                                                ShaderStage   in_stage)
{
    anvil_assert(in_shader_module_ptr != nullptr);

    name              = in_name;
    shader_module_ptr = in_shader_module_ptr;
    stage             = in_stage;

    shader_module_ptr->retain();
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const ShaderModuleStageEntryPoint& in)
{
    name              = in.name;
    shader_module_ptr = in.shader_module_ptr;
    stage             = in.stage;

    if (shader_module_ptr != nullptr)
    {
        shader_module_ptr->retain();
    }
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::~ShaderModuleStageEntryPoint()
{
    if (shader_module_ptr != nullptr)
    {
        shader_module_ptr->release();

        shader_module_ptr = nullptr;
    }
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint& Anvil::ShaderModuleStageEntryPoint::operator=(const Anvil::ShaderModuleStageEntryPoint& in)
{
    name              = in.name;
    shader_module_ptr = in.shader_module_ptr;
    stage             = in.stage;

    if (shader_module_ptr != nullptr)
    {
        shader_module_ptr->retain();
    }

    return *this;
}
