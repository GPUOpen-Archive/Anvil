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
#include "wrappers/buffer.h"
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include "wrappers/semaphore.h"
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
}

/** Please see header for specification */
Anvil::BufferBarrier::BufferBarrier(VkAccessFlags                  in_source_access_mask,
                                    VkAccessFlags                  in_destination_access_mask,
                                    uint32_t                       in_src_queue_family_index,
                                    uint32_t                       in_dst_queue_family_index,
                                    std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                    VkDeviceSize                   in_offset,
                                    VkDeviceSize                   in_size)
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
}

/** Please see header for specification */
Anvil::BufferBarrier::~BufferBarrier()
{
    /* Stub */
}

/** Please see header for specification */
bool Anvil::operator==(const Anvil::FormatProperties& in1,
                       const Anvil::FormatProperties& in2)
{
    return memcmp(&in1,
                  &in2,
                  sizeof(Anvil::FormatProperties) ) == 0;
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
}

/** Please see header for specification */
Anvil::ImageBarrier::ImageBarrier(VkAccessFlags                 in_source_access_mask,
                                  VkAccessFlags                 in_destination_access_mask,
                                  bool                          in_by_region_barrier,
                                  VkImageLayout                 in_old_layout,
                                  VkImageLayout                 in_new_layout,
                                  uint32_t                      in_src_queue_family_index,
                                  uint32_t                      in_dst_queue_family_index,
                                  std::shared_ptr<Anvil::Image> in_image_ptr,
                                  VkImageSubresourceRange       in_image_subresource_range)
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
}

/** Please see header for specification */
Anvil::ImageBarrier::~ImageBarrier()
{
    /* Stub */
}

/* Please see header for specification */
bool Anvil::operator==(const MemoryProperties& in1,
                       const MemoryProperties& in2)
{
    bool result = (in1.types.size() == in2.types.size() );

    if (result)
    {
        for (uint32_t n_type = 0;
                      n_type < static_cast<uint32_t>(in1.types.size() ) && result;
                    ++n_type)
        {
            const auto& current_type_in1 = in1.types.at(n_type);
            const auto& current_type_in2 = in2.types.at(n_type);

            result &= (current_type_in1 == current_type_in2);
        }
    }

    return result;
}

/* Please see header for specification */
void Anvil::MemoryProperties::init(const VkPhysicalDeviceMemoryProperties& mem_properties)
{
    n_heaps = mem_properties.memoryHeapCount;

    heaps   = new Anvil::MemoryHeap[n_heaps];
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

/** Please see header for specification */
bool Anvil::operator==(const MemoryHeap& in1,
                       const MemoryHeap& in2)
{
    return (in1.flags == in2.flags &&
            in1.size  == in2.size);
}

/* Please see header for specification */
Anvil::MemoryType::MemoryType(const VkMemoryType&      type,
                              struct MemoryProperties* memory_props_ptr)
{
    flags    = static_cast<VkMemoryPropertyFlagBits>(type.propertyFlags);
    heap_ptr = &memory_props_ptr->heaps[type.heapIndex];
}

/* Please see header for specification */
bool Anvil::operator==(const Anvil::MemoryType& in1,
                       const Anvil::MemoryType& in2)
{
    return  ( in1.flags    ==  in2.flags     &&
             *in1.heap_ptr == *in2.heap_ptr);
}

                              /** Returns a filled MipmapRawData structure for a 1D mip.
 *
 *  NOTE: It is caller's responsibility to configure one of the data storage pointer members.
 */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D(VkImageAspectFlagBits aspect,
                                                     uint32_t              n_mipmap,
                                                     uint32_t              row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect    = aspect;
    result.data_size = row_size;
    result.row_size  = row_size;
    result.n_layers  = 1;
    result.n_slices  = 1;
    result.n_mipmap  = n_mipmap;

    return result;
}

/** Returns a filled MipmapRawData structure for a 1D Array mip.
 *
 *  NOTE: It is caller's responsibility to configure one of the data storage pointer members.
 */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array(VkImageAspectFlagBits aspect,
                                                           uint32_t              n_layer,
                                                           uint32_t              n_layers,
                                                           uint32_t              n_mipmap,
                                                           uint32_t              row_size,
                                                           uint32_t              data_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect    = aspect;
    result.data_size = data_size;
    result.n_layer   = n_layer;
    result.n_layers  = n_layers;
    result.n_mipmap  = n_mipmap;
    result.n_slices  = 1;
    result.row_size  = row_size;

    return result;
}

/** Returns a filled MipmapRawData structure for a 2D mip.
 *
 *  NOTE: It is caller's responsibility to configure one of the data storage pointer members.
 */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D(VkImageAspectFlagBits aspect,
                                                     uint32_t              n_mipmap,
                                                     uint32_t              data_size,
                                                     uint32_t              row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect    = aspect;
    result.data_size = data_size;
    result.n_layers  = 1;
    result.n_mipmap  = n_mipmap;
    result.n_slices  = 1;
    result.row_size  = row_size;

    return result;
}

/** Returns a filled MipmapRawData structure for a 2D array mip.
 *
 *  NOTE: It is caller's responsibility to configure one of the data storage pointer members.
 */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array(VkImageAspectFlagBits aspect,
                                                           uint32_t              n_layer,
                                                           uint32_t              n_layers,
                                                           uint32_t              n_mipmap,
                                                           uint32_t              data_size,
                                                           uint32_t              row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect    = aspect;
    result.data_size = data_size;
    result.n_layer   = n_layer;
    result.n_layers  = n_layers;
    result.n_mipmap  = n_mipmap;
    result.n_slices  = 1;
    result.row_size  = row_size;

    return result;
}

/** Returns a filled MipmapRawData structure for a 3D mip.
 *
 *  NOTE: It is caller's responsibility to configure one of the data storage pointer members.
 */
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D(VkImageAspectFlagBits aspect,
                                                     uint32_t              n_layer,
                                                     uint32_t              n_slices,
                                                     uint32_t              n_mipmap,
                                                     uint32_t              data_size,
                                                     uint32_t              row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect    = aspect;
    result.data_size = data_size;
    result.n_layers  = 1;
    result.n_layer   = n_layer;
    result.n_slices  = n_slices;
    result.n_mipmap  = n_mipmap;
    result.row_size  = row_size;

    return result;
}


/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_from_uchar_ptr(VkImageAspectFlagBits          aspect,
                                                                    uint32_t                       n_mipmap,
                                                                    std::shared_ptr<unsigned char> linear_tightly_packed_data_ptr,
                                                                    uint32_t                       row_size)
{
    MipmapRawData result = create_1D(aspect,
                                     n_mipmap,
                                     row_size);

    result.linear_tightly_packed_data_uchar_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_from_uchar_ptr(VkImageAspectFlagBits aspect,
                                                                    uint32_t              n_mipmap,
                                                                    const unsigned char*  linear_tightly_packed_data_ptr,
                                                                    uint32_t              row_size)
{
    MipmapRawData result = create_1D(aspect,
                                     n_mipmap,
                                     row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_from_uchar_vector_ptr(VkImageAspectFlagBits                        aspect,
                                                                           uint32_t                                     n_mipmap,
                                                                           std::shared_ptr<std::vector<unsigned char> > linear_tightly_packed_data_ptr,
                                                                           uint32_t                                     row_size)
{
    MipmapRawData result = create_1D(aspect,
                                     n_mipmap,
                                     row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array_from_uchar_ptr(VkImageAspectFlagBits          aspect,
                                                                          uint32_t                       n_layer,
                                                                          uint32_t                       n_layers,
                                                                          uint32_t                       n_mipmap,
                                                                          std::shared_ptr<unsigned char> linear_tightly_packed_data_ptr,
                                                                          uint32_t                       row_size,
                                                                          uint32_t                       data_size)
{
    MipmapRawData result = create_1D_array(aspect,
                                           n_layer,
                                           n_layers,
                                           n_mipmap,
                                           row_size,
                                           data_size);

    result.linear_tightly_packed_data_uchar_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array_from_uchar_ptr(VkImageAspectFlagBits aspect,
                                                                          uint32_t              n_layer,
                                                                          uint32_t              n_layers,
                                                                          uint32_t              n_mipmap,
                                                                          const unsigned char*  linear_tightly_packed_data_ptr,
                                                                          uint32_t              row_size,
                                                                          uint32_t              data_size)
{
    MipmapRawData result = create_1D_array(aspect,
                                           n_layer,
                                           n_layers,
                                           n_mipmap,
                                           row_size,
                                           data_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array_from_uchar_vector_ptr(VkImageAspectFlagBits                        aspect,
                                                                                 uint32_t                                     n_layer,
                                                                                 uint32_t                                     n_layers,
                                                                                 uint32_t                                     n_mipmap,
                                                                                 std::shared_ptr<std::vector<unsigned char> > linear_tightly_packed_data_ptr,
                                                                                 uint32_t                                     row_size,
                                                                                 uint32_t                                     data_size)
{
    MipmapRawData result = create_1D_array(aspect,
                                           n_layer,
                                           n_layers,
                                           n_mipmap,
                                           row_size,
                                           data_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_from_uchar_ptr(VkImageAspectFlagBits          aspect,
                                                                    uint32_t                       n_mipmap,
                                                                    std::shared_ptr<unsigned char> linear_tightly_packed_data_ptr,
                                                                    uint32_t                       data_size,
                                                                    uint32_t                       row_size)
{
    MipmapRawData result = create_2D(aspect,
                                     n_mipmap,
                                     data_size,
                                     row_size);

    result.linear_tightly_packed_data_uchar_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_from_uchar_ptr(VkImageAspectFlagBits aspect,
                                                                    uint32_t              n_mipmap,
                                                                    const unsigned char*  linear_tightly_packed_data_ptr,
                                                                    uint32_t              data_size,
                                                                    uint32_t              row_size)
{
    MipmapRawData result = create_2D(aspect,
                                     n_mipmap,
                                     data_size,
                                     row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_from_uchar_vector_ptr(VkImageAspectFlagBits                        aspect,
                                                                           uint32_t                                     n_mipmap,
                                                                           std::shared_ptr<std::vector<unsigned char> > linear_tightly_packed_data_ptr,
                                                                           uint32_t                                     data_size,
                                                                           uint32_t                                     row_size)
{
    MipmapRawData result = create_2D(aspect,
                                     n_mipmap,
                                     data_size,
                                     row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array_from_uchar_ptr(VkImageAspectFlagBits                        aspect,
                                                                          uint32_t                                     n_layer,
                                                                          uint32_t                                     n_layers,
                                                                          uint32_t                                     n_mipmap,
                                                                          std::shared_ptr<unsigned char>               linear_tightly_packed_data_ptr,
                                                                          uint32_t                                     data_size,
                                                                          uint32_t                                     row_size)
{
    MipmapRawData result = create_2D_array(aspect,
                                           n_layer,
                                           n_layers,
                                           n_mipmap,
                                           data_size,
                                           row_size);

    result.linear_tightly_packed_data_uchar_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array_from_uchar_ptr(VkImageAspectFlagBits                        aspect,
                                                                          uint32_t                                     n_layer,
                                                                          uint32_t                                     n_layers,
                                                                          uint32_t                                     n_mipmap,
                                                                          const unsigned char*                         linear_tightly_packed_data_ptr,
                                                                          uint32_t                                     data_size,
                                                                          uint32_t                                     row_size)
{
    MipmapRawData result = create_2D_array(aspect,
                                           n_layer,
                                           n_layers,
                                           n_mipmap,
                                           data_size,
                                           row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array_from_uchar_vector_ptr(VkImageAspectFlagBits                        aspect,
                                                                                 uint32_t                                     n_layer,
                                                                                 uint32_t                                     n_layers,
                                                                                 uint32_t                                     n_mipmap,
                                                                                 std::shared_ptr<std::vector<unsigned char> > linear_tightly_packed_data_ptr,
                                                                                 uint32_t                                     data_size,
                                                                                 uint32_t                                     row_size)
{
    MipmapRawData result = create_2D_array(aspect,
                                           n_layer,
                                           n_layers,
                                           n_mipmap,
                                           data_size,
                                           row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = linear_tightly_packed_data_ptr;

    return result;
}



/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D_from_uchar_ptr(VkImageAspectFlagBits          aspect,
                                                                    uint32_t                       n_layer,
                                                                    uint32_t                       n_layer_slices,
                                                                    uint32_t                       n_mipmap,
                                                                    std::shared_ptr<unsigned char> linear_tightly_packed_data_ptr,
                                                                    uint32_t                       data_size,
                                                                    uint32_t                       row_size)
{
    MipmapRawData result = create_3D(aspect,
                                     n_layer,
                                     n_layer_slices,
                                     n_mipmap,
                                     data_size,
                                     row_size);

    result.linear_tightly_packed_data_uchar_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D_from_uchar_ptr(VkImageAspectFlagBits aspect,
                                                                    uint32_t              n_layer,
                                                                    uint32_t              n_layer_slices,
                                                                    uint32_t              n_mipmap,
                                                                    const unsigned char*  linear_tightly_packed_data_ptr,
                                                                    uint32_t              data_size,
                                                                    uint32_t              row_size)
{
    MipmapRawData result = create_3D(aspect,
                                     n_layer,
                                     n_layer_slices,
                                     n_mipmap,
                                     data_size,
                                     row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D_from_uchar_vector_ptr(VkImageAspectFlagBits                        aspect,
                                                                           uint32_t                                     n_layer,
                                                                           uint32_t                                     n_layer_slices,
                                                                           uint32_t                                     n_mipmap,
                                                                           std::shared_ptr<std::vector<unsigned char> > linear_tightly_packed_data_ptr,
                                                                           uint32_t                                     data_size,
                                                                           uint32_t                                     row_size)
{
    MipmapRawData result = create_3D(aspect,
                                     n_layer,
                                     n_layer_slices,
                                     n_mipmap,
                                     data_size,
                                     row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = linear_tightly_packed_data_ptr;

    return result;
}


/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_from_uchar_ptr(VkImageAspectFlagBits          aspect,
                                                                          uint32_t                       n_layer,
                                                                          uint32_t                       n_mipmap,
                                                                          std::shared_ptr<unsigned char> linear_tightly_packed_data_ptr,
                                                                          uint32_t                       data_size,
                                                                          uint32_t                       row_size)
{
    anvil_assert(n_layer < 6);

    MipmapRawData result = create_2D_array(aspect,
                                           n_layer,
                                           1, /* n_layer_slices */
                                           n_mipmap,
                                           data_size,
                                           row_size);

    result.linear_tightly_packed_data_uchar_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_from_uchar_ptr(VkImageAspectFlagBits aspect,
                                                                          uint32_t              n_layer,
                                                                          uint32_t              n_mipmap,
                                                                          const unsigned char*  linear_tightly_packed_data_ptr,
                                                                          uint32_t              data_size,
                                                                          uint32_t              row_size)
{
    anvil_assert(n_layer < 6);

    MipmapRawData result = create_2D_array(aspect,
                                           n_layer,
                                           1, /* n_layer_slices */
                                           n_mipmap,
                                           data_size,
                                           row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_from_uchar_vector_ptr(VkImageAspectFlagBits                        aspect,
                                                                                 uint32_t                                     n_layer,
                                                                                 uint32_t                                     n_mipmap,
                                                                                 std::shared_ptr<std::vector<unsigned char> > linear_tightly_packed_data_ptr,
                                                                                 uint32_t                                     data_size,
                                                                                 uint32_t                                     row_size)
{
    anvil_assert(n_layer < 6);

    MipmapRawData result = create_2D_array(aspect,
                                           n_layer,
                                           1, /* n_layer_slices */
                                           n_mipmap,
                                           data_size,
                                           row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = linear_tightly_packed_data_ptr;

    return result;
}


/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_array_from_uchar_ptr(VkImageAspectFlagBits          aspect,
                                                                                uint32_t                       n_layer,
                                                                                uint32_t                       n_layers,
                                                                                uint32_t                       n_mipmap,
                                                                                std::shared_ptr<unsigned char> linear_tightly_packed_data_ptr,
                                                                                uint32_t                       data_size,
                                                                                uint32_t                       row_size)
{
    MipmapRawData result = create_2D_array(aspect,
                                           n_layer,
                                           n_layers,
                                           n_mipmap,
                                           data_size,
                                           row_size);

    result.linear_tightly_packed_data_uchar_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_array_from_uchar_ptr(VkImageAspectFlagBits aspect,
                                                                                uint32_t              n_layer,
                                                                                uint32_t              n_layers,
                                                                                uint32_t              n_mipmap,
                                                                                const unsigned char*  linear_tightly_packed_data_ptr,
                                                                                uint32_t              data_size,
                                                                                uint32_t              row_size)
{
    MipmapRawData result = create_2D_array(aspect,
                                           n_layer,
                                           n_layers,
                                           n_mipmap,
                                           data_size,
                                           row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_array_from_uchar_vector_ptr(VkImageAspectFlagBits                        aspect,
                                                                                       uint32_t                                     n_layer,
                                                                                       uint32_t                                     n_layers,
                                                                                       uint32_t                                     n_mipmap,
                                                                                       std::shared_ptr<std::vector<unsigned char> > linear_tightly_packed_data_ptr,
                                                                                       uint32_t                                     data_size,
                                                                                       uint32_t                                     row_size)
{
    MipmapRawData result = create_2D_array(aspect,
                                           n_layer,
                                           n_layers,
                                           n_mipmap,
                                           data_size,
                                           row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = linear_tightly_packed_data_ptr;

    return result;
}

/** Please see header for specification */
bool Anvil::operator==(const Anvil::QueueFamilyInfo& in1,
                       const Anvil::QueueFamilyInfo& in2)
{
    return (in1.flags                                 == in2.flags                                 &&
            in1.min_image_transfer_granularity.depth  == in2.min_image_transfer_granularity.depth  &&
            in1.min_image_transfer_granularity.height == in2.min_image_transfer_granularity.height &&
            in1.min_image_transfer_granularity.width  == in2.min_image_transfer_granularity.width  &&
            in1.n_queues                              == in2.n_queues                              &&
            in1.n_timestamp_bits                      == in2.n_timestamp_bits);
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint()
{
    name  = nullptr;
    stage = SHADER_STAGE_UNKNOWN;
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const char*                   in_name,
                                                                std::shared_ptr<ShaderModule> in_shader_module_ptr,
                                                                ShaderStage                   in_stage)
{
    anvil_assert(in_shader_module_ptr != nullptr);

    name              = in_name;
    shader_module_ptr = in_shader_module_ptr;
    stage             = in_stage;
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const ShaderModuleStageEntryPoint& in)
{
    name              = in.name;
    shader_module_ptr = in.shader_module_ptr;
    stage             = in.stage;
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::~ShaderModuleStageEntryPoint()
{
    /* Stub */
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint& Anvil::ShaderModuleStageEntryPoint::operator=(const Anvil::ShaderModuleStageEntryPoint& in)
{
    name              = in.name;
    shader_module_ptr = in.shader_module_ptr;
    stage             = in.stage;

    return *this;
}

/** Please see header for specification */
Anvil::Utils::SparseMemoryBindingUpdateInfo::SparseMemoryBindingUpdateInfo()
{
    m_dirty = true;
}

/** Please see header for specification */
Anvil::SparseMemoryBindInfoID Anvil::Utils::SparseMemoryBindingUpdateInfo::add_bind_info(uint32_t                            n_signal_semaphores,
                                                                                         std::shared_ptr<Anvil::Semaphore>*  opt_signal_semaphores_ptr,
                                                                                         uint32_t                            n_wait_semaphores,
                                                                                         std::shared_ptr<Anvil::Semaphore>*  opt_wait_semaphores_ptr)
{
    Anvil::SparseMemoryBindInfoID result_id = static_cast<Anvil::SparseMemoryBindInfoID>(m_bindings.size() );
    BindingInfo                   new_binding;

    for (uint32_t n_signal_sem = 0;
                  n_signal_sem < n_signal_semaphores;
                ++n_signal_sem)
    {
        new_binding.signal_semaphores.push_back(opt_signal_semaphores_ptr[n_signal_sem]);
    }

    for (uint32_t n_wait_sem = 0;
                  n_wait_sem < n_wait_semaphores;
                ++n_wait_sem)
    {
        new_binding.wait_semaphores.push_back(opt_wait_semaphores_ptr[n_wait_sem]);
    }

    m_bindings.push_back(new_binding);

    return result_id;
}

/** Please see header for specification */
void Anvil::Utils::SparseMemoryBindingUpdateInfo::append_buffer_memory_update(SparseMemoryBindInfoID              bind_info_id,
                                                                              std::shared_ptr<Anvil::Buffer>      buffer_ptr,
                                                                              VkDeviceSize                        buffer_memory_start_offset,
                                                                              std::shared_ptr<Anvil::MemoryBlock> memory_block_ptr,
                                                                              VkDeviceSize                        memory_block_start_offset,
                                                                              VkDeviceSize                        size)
{
    /* Sanity checks */
    anvil_assert(buffer_ptr                                 != nullptr);
    anvil_assert(m_bindings.size()                          >  bind_info_id);
    anvil_assert(buffer_ptr->get_memory_requirements().size >= buffer_memory_start_offset + size);

    if (memory_block_ptr != nullptr)
    {
        anvil_assert(memory_block_ptr->get_size() >= memory_block_start_offset  + size);
    }

    /* Cache the update */
    auto&              binding = m_bindings.at(bind_info_id);
    GeneralBindInfo    update;
    VkSparseMemoryBind update_vk;

    update.memory_block_ptr          = memory_block_ptr;
    update.memory_block_start_offset = memory_block_start_offset;
    update.size                      = size;
    update.start_offset              = buffer_memory_start_offset;

    update_vk.flags                  = 0;
    update_vk.memory                 = (memory_block_ptr != nullptr) ? memory_block_ptr->get_memory()
                                                                     : VK_NULL_HANDLE;
    update_vk.memoryOffset           = (memory_block_ptr != nullptr) ? (memory_block_ptr->get_start_offset() + memory_block_start_offset)
                                                                     : UINT32_MAX;
    update_vk.resourceOffset         = (buffer_ptr->get_start_offset() + buffer_memory_start_offset);
    update_vk.size                   = size;

    binding.buffer_updates[buffer_ptr].first.push_back (update);
    binding.buffer_updates[buffer_ptr].second.push_back(update_vk);
}

/** Please see header for specification */
void Anvil::Utils::SparseMemoryBindingUpdateInfo::append_image_memory_update(SparseMemoryBindInfoID              bind_info_id,
                                                                             std::shared_ptr<Anvil::Image>       image_ptr,
                                                                             const VkImageSubresource&           subresource,
                                                                             const VkOffset3D&                   offset,
                                                                             const VkExtent3D&                   extent,
                                                                             VkSparseMemoryBindFlags             flags,
                                                                             std::shared_ptr<Anvil::MemoryBlock> opt_memory_block_ptr,
                                                                             VkDeviceSize                        opt_memory_block_start_offset)
{
    /* Sanity checks .. */
    anvil_assert(image_ptr != nullptr);
    anvil_assert(flags     == 0);
    anvil_assert(m_bindings.size() > bind_info_id);

    anvil_assert(image_ptr->get_image_n_layers()  > subresource.arrayLayer);
    anvil_assert(image_ptr->get_image_n_mipmaps() > subresource.mipLevel);
    anvil_assert(image_ptr->has_aspects(subresource.aspectMask) );

    if (opt_memory_block_ptr != nullptr)
    {
        anvil_assert(opt_memory_block_ptr->get_size() > opt_memory_block_start_offset);
    }

    /* Cache the update */
    auto&                   binding = m_bindings.at(bind_info_id);
    ImageBindInfo           update;
    VkSparseImageMemoryBind update_vk;

    update.extent                    = extent;
    update.flags                     = flags;
    update.memory_block_ptr          = opt_memory_block_ptr;
    update.memory_block_start_offset = opt_memory_block_start_offset;
    update.offset                    = offset;
    update.subresource               = subresource;

    update_vk.extent       = extent;
    update_vk.flags        = flags;
    update_vk.memory       = (opt_memory_block_ptr != nullptr) ? opt_memory_block_ptr->get_memory()
                                                               : VK_NULL_HANDLE;
    update_vk.memoryOffset = (opt_memory_block_ptr != nullptr) ? opt_memory_block_ptr->get_start_offset() + opt_memory_block_start_offset
                                                               : UINT32_MAX;
    update_vk.offset       = offset;
    update_vk.subresource  = subresource;

    binding.image_updates[image_ptr].first.push_back (update);
    binding.image_updates[image_ptr].second.push_back(update_vk);
}

/** Please see header for specification */
void Anvil::Utils::SparseMemoryBindingUpdateInfo::append_opaque_image_memory_update(SparseMemoryBindInfoID              bind_info_id,
                                                                                    std::shared_ptr<Anvil::Image>       image_ptr,
                                                                                    VkDeviceSize                        resource_offset,
                                                                                    VkDeviceSize                        size,
                                                                                    VkSparseMemoryBindFlags             flags,
                                                                                    std::shared_ptr<Anvil::MemoryBlock> opt_memory_block_ptr,
                                                                                    VkDeviceSize                        opt_memory_block_start_offset)
{
    /* Sanity checks */
    anvil_assert(image_ptr                                 != nullptr);
    anvil_assert(m_bindings.size()                         >  bind_info_id);
    anvil_assert(image_ptr->get_memory_requirements().size >= resource_offset + size);

    if (opt_memory_block_ptr != nullptr)
    {
        anvil_assert(opt_memory_block_ptr->get_size() >= opt_memory_block_start_offset + size);
    }

    /* Cache the update */
    auto&              binding = m_bindings.at(bind_info_id);
    GeneralBindInfo    update;
    VkSparseMemoryBind update_vk;

    update.flags                     = flags;
    update.memory_block_ptr          = opt_memory_block_ptr;
    update.memory_block_start_offset = opt_memory_block_start_offset;
    update.size                      = size;
    update.start_offset              = resource_offset;

    update_vk.flags                  = flags;
    update_vk.memory                 = (opt_memory_block_ptr != nullptr) ? opt_memory_block_ptr->get_memory()
                                                                         : VK_NULL_HANDLE;
    update_vk.memoryOffset           = (opt_memory_block_ptr != nullptr) ? (opt_memory_block_ptr->get_start_offset() + opt_memory_block_start_offset)
                                                                         : UINT32_MAX;
    update_vk.resourceOffset         = resource_offset;
    update_vk.size                   = size;

    binding.image_opaque_updates[image_ptr].first.push_back (update);
    binding.image_opaque_updates[image_ptr].second.push_back(update_vk);
}

/** Please see header for specification */
void Anvil::Utils::SparseMemoryBindingUpdateInfo::bake()
{
    const uint32_t n_bindings = static_cast<uint32_t>(m_bindings.size() );
    
    anvil_assert(m_dirty);

    if (n_bindings > 0)
    {
        m_bindings_vk.resize(n_bindings);

        m_buffer_bindings_vk.clear();

        for (uint32_t n_binding = 0;
                      n_binding < n_bindings;
                    ++n_binding)
        {
            auto&    bind_info                           = m_bindings   [n_binding];
            uint32_t n_buffer_bindings_start_index       = ~0u;
            uint32_t n_image_bindings_start_index        = ~0u;
            uint32_t n_image_opaque_bindings_start_index = ~0u;
            auto&    vk_binding                          = m_bindings_vk[n_binding];

            bind_info.signal_semaphores_vk.clear();
            bind_info.wait_semaphores_vk.clear  ();

            bind_info.signal_semaphores_vk.reserve(bind_info.signal_semaphores.size() );
            bind_info.wait_semaphores_vk.reserve  (bind_info.wait_semaphores.size  () );

            for (auto& signal_semaphore_ptr : bind_info.signal_semaphores)
            {
                bind_info.signal_semaphores_vk.push_back(signal_semaphore_ptr->get_semaphore() );
            }

            for (auto& wait_semaphore_ptr : bind_info.wait_semaphores)
            {
                bind_info.wait_semaphores_vk.push_back(wait_semaphore_ptr->get_semaphore() );
            }

            vk_binding.bufferBindCount      = static_cast<uint32_t>(bind_info.buffer_updates.size() );
            vk_binding.imageBindCount       = static_cast<uint32_t>(bind_info.image_updates.size() );
            vk_binding.imageOpaqueBindCount = static_cast<uint32_t>(bind_info.image_opaque_updates.size() );
            vk_binding.pNext                = nullptr;
            vk_binding.pSignalSemaphores    = (bind_info.signal_semaphores_vk.size() > 0) ? &bind_info.signal_semaphores_vk[0] : nullptr;
            vk_binding.pWaitSemaphores      = (bind_info.wait_semaphores_vk.size()   > 0) ? &bind_info.wait_semaphores_vk  [0] : nullptr;
            vk_binding.signalSemaphoreCount = static_cast<uint32_t>(bind_info.signal_semaphores_vk.size() );
            vk_binding.sType                = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
            vk_binding.waitSemaphoreCount   = static_cast<uint32_t>(bind_info.wait_semaphores_vk.size() );

            n_buffer_bindings_start_index       = static_cast<uint32_t>(m_buffer_bindings_vk.size() );
            n_image_bindings_start_index        = static_cast<uint32_t>(m_image_bindings_vk.size() );
            n_image_opaque_bindings_start_index = static_cast<uint32_t>(m_image_opaque_bindings_vk.size() );

            for (auto& buffer_update : bind_info.buffer_updates)
            {
                const VkBuffer               current_buffer_vk = buffer_update.first->get_buffer();
                VkSparseBufferMemoryBindInfo buffer_bind_info;

                anvil_assert(buffer_update.second.second.size() > 0);

                buffer_bind_info.bindCount = static_cast<uint32_t>(buffer_update.second.second.size() );
                buffer_bind_info.buffer    = current_buffer_vk;
                buffer_bind_info.pBinds    = &buffer_update.second.second[0];

                m_buffer_bindings_vk.push_back(buffer_bind_info);
            }

            for (auto& image_update : bind_info.image_updates)
            {
                const VkImage               current_image_vk = image_update.first->get_image();
                VkSparseImageMemoryBindInfo image_bind_info;

                anvil_assert(image_update.second.second.size() > 0);

                image_bind_info.bindCount = static_cast<uint32_t>(image_update.second.second.size() );
                image_bind_info.image     = current_image_vk;
                image_bind_info.pBinds    = &image_update.second.second[0];

                m_image_bindings_vk.push_back(image_bind_info);
            }

            for (auto& image_opaque_update : bind_info.image_opaque_updates)
            {
                const VkImage                     current_image_vk = image_opaque_update.first->get_image();
                VkSparseImageOpaqueMemoryBindInfo image_opaque_bind_info;

                anvil_assert(image_opaque_update.second.second.size() > 0);

                image_opaque_bind_info.bindCount = static_cast<uint32_t>(image_opaque_update.second.second.size() );
                image_opaque_bind_info.image     = current_image_vk;
                image_opaque_bind_info.pBinds    = &image_opaque_update.second.second[0];

                m_image_opaque_bindings_vk.push_back(image_opaque_bind_info);
            }

            vk_binding.pBufferBinds      = (bind_info.buffer_updates.size()       > 0) ? &m_buffer_bindings_vk      [n_buffer_bindings_start_index]       : nullptr;
            vk_binding.pImageBinds       = (bind_info.image_updates.size()        > 0) ? &m_image_bindings_vk       [n_image_bindings_start_index]        : nullptr;
            vk_binding.pImageOpaqueBinds = (bind_info.image_opaque_updates.size() > 0) ? &m_image_opaque_bindings_vk[n_image_opaque_bindings_start_index] : nullptr;
        }
    }
    else
    {
        m_bindings_vk.clear();
    }

    m_dirty = false;
}

/** Please see header for specification */
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_bind_info_properties(SparseMemoryBindInfoID                    bind_info_id,
                                                                           uint32_t* const                           opt_out_n_buffer_memory_updates_ptr,
                                                                           uint32_t* const                           opt_out_n_image_memory_updates_ptr,
                                                                           uint32_t* const                           opt_out_n_image_opaque_memory_updates_ptr,
                                                                           uint32_t* const                           opt_out_n_signal_semaphores_ptr,
                                                                           const std::shared_ptr<Anvil::Semaphore>** opt_out_signal_semaphores_ptr_ptr,
                                                                           uint32_t* const                           opt_out_n_wait_semaphores_ptr,
                                                                           const std::shared_ptr<Anvil::Semaphore>** opt_out_wait_semaphores_ptr_ptr) const
{
    decltype(m_bindings)::const_iterator binding_iterator;
          bool                           result = false;

    if (m_bindings.size() <= bind_info_id)
    {
        anvil_assert(m_bindings.size() > bind_info_id);

        goto end;
    }

    binding_iterator = m_bindings.begin() + static_cast<int>(bind_info_id);

    if (opt_out_n_buffer_memory_updates_ptr != nullptr)
    {
        uint32_t n_buffer_mem_updates = 0;

        for (const auto& buffer_update_iterator : binding_iterator->buffer_updates)
        {
            n_buffer_mem_updates += static_cast<uint32_t>(buffer_update_iterator.second.first.size() );
        }

        *opt_out_n_buffer_memory_updates_ptr = n_buffer_mem_updates;
    }

    if (opt_out_n_image_memory_updates_ptr != nullptr)
    {
        uint32_t n_image_mem_updates = 0;

        for (const auto& image_update_iterator : binding_iterator->image_updates)
        {
            n_image_mem_updates += static_cast<uint32_t>(image_update_iterator.second.first.size() );
        }

        *opt_out_n_image_memory_updates_ptr = n_image_mem_updates;
    }

    if (opt_out_n_image_opaque_memory_updates_ptr != nullptr)
    {
        uint32_t n_image_opaque_mem_updates = 0;

        for (const auto& image_opaque_update_iterator : binding_iterator->image_opaque_updates)
        {
            n_image_opaque_mem_updates += static_cast<uint32_t>(image_opaque_update_iterator.second.first.size() );
        }

        *opt_out_n_image_opaque_memory_updates_ptr = n_image_opaque_mem_updates;
    }

    if (opt_out_n_signal_semaphores_ptr != nullptr)
    {
        *opt_out_n_signal_semaphores_ptr = static_cast<uint32_t>(binding_iterator->signal_semaphores.size() );
    }

    if (opt_out_signal_semaphores_ptr_ptr != nullptr)
    {
        *opt_out_signal_semaphores_ptr_ptr = &binding_iterator->signal_semaphores[0];
    }

    if (opt_out_n_wait_semaphores_ptr != nullptr)
    {
        *opt_out_n_wait_semaphores_ptr = static_cast<uint32_t>(binding_iterator->wait_semaphores.size() );
    }

    if (opt_out_wait_semaphores_ptr_ptr != nullptr)
    {
        *opt_out_wait_semaphores_ptr_ptr = &binding_iterator->wait_semaphores[0];
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
void Anvil::Utils::SparseMemoryBindingUpdateInfo::get_bind_sparse_call_args(uint32_t*                      out_bind_info_count_ptr,
                                                                            const VkBindSparseInfo**       out_bind_info_ptr,
                                                                            std::shared_ptr<Anvil::Fence>* out_fence_to_set_ptr)
{
    if (m_dirty)
    {
        bake();

        anvil_assert(!m_dirty);
    }

    *out_bind_info_count_ptr = static_cast<uint32_t>(m_bindings.size() );
    *out_bind_info_ptr       = (m_bindings_vk.size() > 0) ? &m_bindings_vk[0] : nullptr;
    *out_fence_to_set_ptr    = m_fence_ptr;
}

/** Please see header for specification */
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_buffer_memory_update_properties(SparseMemoryBindInfoID               bind_info_id,
                                                                                      uint32_t                             n_update,
                                                                                      std::shared_ptr<Anvil::Buffer>*      out_opt_buffer_ptr,
                                                                                      VkDeviceSize*                        out_opt_buffer_memory_start_offset_ptr,
                                                                                      std::shared_ptr<Anvil::MemoryBlock>* out_opt_memory_block_ptr,
                                                                                      VkDeviceSize*                        out_opt_memory_block_start_offset_ptr,
                                                                                      VkDeviceSize*                        out_opt_size_ptr) const
{
    GeneralBindInfo                      buffer_bind;
    BufferBindUpdateMap::const_iterator  buffer_binding_map_iterator;
    decltype(m_bindings)::const_iterator binding_iterator;
    uint32_t                             n_current_update = 0;
    bool                                 result           = false;

    if (m_bindings.size() <= bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= bind_info_id) );

        goto end;
    }

    binding_iterator            = m_bindings.cbegin() + static_cast<int>(bind_info_id);
    buffer_binding_map_iterator = binding_iterator->buffer_updates.begin();

    while (buffer_binding_map_iterator != binding_iterator->buffer_updates.end() )
    {
        const uint32_t n_buffer_bindings = static_cast<uint32_t>(buffer_binding_map_iterator->second.first.size() );

        if (n_current_update + n_buffer_bindings > n_update)
        {
            buffer_bind = buffer_binding_map_iterator->second.first.at(n_update - n_current_update);

            break;
        }
        else
        {
            n_current_update            += n_buffer_bindings;
            buffer_binding_map_iterator ++;
        }
    }

    if (buffer_binding_map_iterator == binding_iterator->buffer_updates.end() )
    {
        anvil_assert(!(buffer_binding_map_iterator == binding_iterator->buffer_updates.end()) );

        goto end;
    }

    if (out_opt_buffer_ptr != nullptr)
    {
        *out_opt_buffer_ptr = buffer_binding_map_iterator->first;
    }

    if (out_opt_buffer_memory_start_offset_ptr != nullptr)
    {
        *out_opt_buffer_memory_start_offset_ptr = buffer_bind.start_offset;
    }

    if (out_opt_memory_block_ptr != nullptr)
    {
        *out_opt_memory_block_ptr = buffer_bind.memory_block_ptr;
    }

    if (out_opt_memory_block_start_offset_ptr != nullptr)
    {
        *out_opt_memory_block_start_offset_ptr = buffer_bind.memory_block_start_offset;
    }

    if (out_opt_size_ptr != nullptr)
    {
        *out_opt_size_ptr = buffer_bind.size;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_image_memory_update_properties(SparseMemoryBindInfoID               bind_info_id,
                                                                                     uint32_t                             n_update,
                                                                                     std::shared_ptr<Anvil::Image>*       opt_out_image_ptr_ptr,
                                                                                     VkImageSubresource*                  opt_out_subresource_ptr,
                                                                                     VkOffset3D*                          opt_out_offset_ptr,
                                                                                     VkExtent3D*                          opt_out_extent_ptr,
                                                                                     VkSparseMemoryBindFlags*             opt_out_flags_ptr,
                                                                                     std::shared_ptr<Anvil::MemoryBlock>* opt_out_memory_block_ptr_ptr,
                                                                                     VkDeviceSize*                        opt_out_memory_block_start_offset_ptr) const
{
    decltype(m_bindings)::const_iterator binding_iterator;
    ImageBindInfo                        image_bind;
    ImageBindUpdateMap::const_iterator   image_binding_map_iterator;
    uint32_t                             n_current_update           = 0;
    bool                                 result                     = false;

    if (m_bindings.size() <= bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= bind_info_id) );

        goto end;
    }

    binding_iterator           = m_bindings.cbegin() + static_cast<int>(bind_info_id);
    image_binding_map_iterator = binding_iterator->image_updates.begin();

    while (image_binding_map_iterator != binding_iterator->image_updates.end() )
    {
        const uint32_t n_image_bindings = static_cast<uint32_t>(image_binding_map_iterator->second.first.size() );

        if (n_current_update + n_image_bindings > n_update)
        {
            image_bind = image_binding_map_iterator->second.first.at(n_update - n_current_update);

            break;
        }
        else
        {
            n_current_update           += n_image_bindings;
            image_binding_map_iterator ++;
        }
    }

    if (image_binding_map_iterator == binding_iterator->image_updates.end() )
    {
        anvil_assert(!(image_binding_map_iterator == binding_iterator->image_updates.end()) );

        goto end;
    }

    if (opt_out_image_ptr_ptr != nullptr)
    {
        *opt_out_image_ptr_ptr = image_binding_map_iterator->first;
    }

    if (opt_out_subresource_ptr != nullptr)
    {
        *opt_out_subresource_ptr = image_bind.subresource;
    }

    if (opt_out_offset_ptr != nullptr)
    {
        *opt_out_offset_ptr = image_bind.offset;
    }

    if (opt_out_extent_ptr != nullptr)
    {
        *opt_out_extent_ptr = image_bind.extent;
    }

    if (opt_out_flags_ptr != nullptr)
    {
        *opt_out_flags_ptr = image_bind.flags;
    }

    if (opt_out_memory_block_ptr_ptr != nullptr)
    {
        *opt_out_memory_block_ptr_ptr = image_bind.memory_block_ptr;
    }

    if (opt_out_memory_block_start_offset_ptr != nullptr)
    {
        *opt_out_memory_block_start_offset_ptr = image_bind.memory_block_start_offset;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_image_opaque_memory_update_properties(SparseMemoryBindInfoID               bind_info_id,
                                                                                            uint32_t                             n_update,
                                                                                            std::shared_ptr<Anvil::Image>*       opt_out_image_ptr_ptr,
                                                                                            VkDeviceSize*                        opt_out_resource_offset_ptr,
                                                                                            VkDeviceSize*                        opt_out_size_ptr,
                                                                                            VkSparseMemoryBindFlags*             opt_out_flags_ptr,
                                                                                            std::shared_ptr<Anvil::MemoryBlock>* opt_out_memory_block_ptr_ptr,
                                                                                            VkDeviceSize*                        opt_out_memory_block_start_offset_ptr) const
{
    decltype(m_bindings)::const_iterator     binding_iterator;
    GeneralBindInfo                          image_opaque_bind;
    ImageOpaqueBindUpdateMap::const_iterator image_opaque_binding_map_iterator;
    uint32_t                                 n_current_update           = 0;
    bool                                     result                     = false;

    if (m_bindings.size() <= bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= bind_info_id) );

        goto end;
    }

    binding_iterator                  = m_bindings.cbegin() + static_cast<int>(bind_info_id);
    image_opaque_binding_map_iterator = binding_iterator->image_opaque_updates.begin();

    while (image_opaque_binding_map_iterator != binding_iterator->image_opaque_updates.end() )
    {
        const uint32_t n_image_opaque_bindings = static_cast<uint32_t>(image_opaque_binding_map_iterator->second.first.size() );

        if (n_current_update + n_image_opaque_bindings > n_update)
        {
            image_opaque_bind = image_opaque_binding_map_iterator->second.first.at(n_update - n_current_update);

            break;
        }
        else
        {
            n_current_update                 += n_image_opaque_bindings;
            image_opaque_binding_map_iterator ++;
        }
    }

    if (image_opaque_binding_map_iterator == binding_iterator->image_opaque_updates.end() )
    {
        anvil_assert(!(image_opaque_binding_map_iterator == binding_iterator->image_opaque_updates.end()) );

        goto end;
    }

    if (opt_out_image_ptr_ptr != nullptr)
    {
        *opt_out_image_ptr_ptr = image_opaque_binding_map_iterator->first;
    }

    if (opt_out_resource_offset_ptr != nullptr)
    {
        *opt_out_resource_offset_ptr = image_opaque_bind.start_offset;
    }

    if (opt_out_size_ptr != nullptr)
    {
        *opt_out_size_ptr = image_opaque_bind.size;
    }

    if (opt_out_flags_ptr != nullptr)
    {
        *opt_out_flags_ptr = image_opaque_bind.flags;
    }

    if (opt_out_memory_block_ptr_ptr != nullptr)
    {
        *opt_out_memory_block_ptr_ptr = image_opaque_bind.memory_block_ptr;
    }

    if (opt_out_memory_block_start_offset_ptr != nullptr)
    {
        *opt_out_memory_block_start_offset_ptr = image_opaque_bind.memory_block_start_offset;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
void Anvil::Utils::convert_queue_family_bits_to_family_indices(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                               Anvil::QueueFamilyBits           queue_families,
                                                               uint32_t*                        out_opt_queue_family_indices_ptr,
                                                               uint32_t*                        out_opt_n_queue_family_indices_ptr)
{
    std::shared_ptr<BaseDevice> device_locked_ptr       (device_ptr);
    uint32_t                    n_queue_family_indices  (0);
    bool                        universal_queue_included(false);

    if ((queue_families & QUEUE_FAMILY_COMPUTE_BIT) != 0)
    {
        if (out_opt_queue_family_indices_ptr != nullptr)
        {
            out_opt_queue_family_indices_ptr[n_queue_family_indices] = device_locked_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_COMPUTE);

            /* If the compute queue family is not available, use universal instead */
            if (out_opt_queue_family_indices_ptr[n_queue_family_indices] == UINT32_MAX)
            {
                out_opt_queue_family_indices_ptr[n_queue_family_indices] = device_locked_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL);

                ++n_queue_family_indices;
                universal_queue_included = true;
            }
            else
            {
                ++n_queue_family_indices;
            }
        }
    }

    if ((queue_families & QUEUE_FAMILY_DMA_BIT) != 0)
    {
        if (out_opt_queue_family_indices_ptr != nullptr)
        {
            out_opt_queue_family_indices_ptr[n_queue_family_indices] = device_locked_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_TRANSFER);

            /* If the DMA queue family is unavailable, use universal instead */
            if (out_opt_queue_family_indices_ptr[n_queue_family_indices] == UINT32_MAX &&
                !universal_queue_included)
            {
                out_opt_queue_family_indices_ptr[n_queue_family_indices] = device_locked_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL);

                ++n_queue_family_indices;
                universal_queue_included = true;
            }
            else
            {
                ++n_queue_family_indices;
            }
        }
    }

    if (((queue_families & QUEUE_FAMILY_GRAPHICS_BIT) != 0) &&
        !universal_queue_included)
    {
        if (out_opt_queue_family_indices_ptr != nullptr)
        {
            out_opt_queue_family_indices_ptr[n_queue_family_indices] = device_locked_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL);
        }

        ++n_queue_family_indices;
        universal_queue_included = true;
    }

    anvil_assert(n_queue_family_indices > 0);

    if (out_opt_n_queue_family_indices_ptr != nullptr)
    {
        *out_opt_n_queue_family_indices_ptr = n_queue_family_indices;
    }
}

/** Please see header for specification */
VkAccessFlags Anvil::Utils::get_access_mask_from_image_layout(VkImageLayout layout)
{
    VkAccessFlags result = 0;

    switch (layout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        {
            result = 0;

            break;
        }

        case VK_IMAGE_LAYOUT_GENERAL:
        {
            result = VK_ACCESS_INDIRECT_COMMAND_READ_BIT          |
                     VK_ACCESS_INDEX_READ_BIT                     |
                     VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT          |
                     VK_ACCESS_UNIFORM_READ_BIT                   |
                     VK_ACCESS_INPUT_ATTACHMENT_READ_BIT          |
                     VK_ACCESS_SHADER_READ_BIT                    |
                     VK_ACCESS_SHADER_WRITE_BIT                   |
                     VK_ACCESS_COLOR_ATTACHMENT_READ_BIT          |
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT         |
                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT  |
                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                     VK_ACCESS_TRANSFER_READ_BIT                  |
                     VK_ACCESS_TRANSFER_WRITE_BIT                 |
                     VK_ACCESS_HOST_READ_BIT                      |
                     VK_ACCESS_HOST_WRITE_BIT                     |
                     VK_ACCESS_MEMORY_READ_BIT                    |
                     VK_ACCESS_MEMORY_WRITE_BIT;

            break;
        }

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        {
            result = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            break;
        }

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        {
            result = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT  |
                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            break;
        }

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        {
            result = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

            break;
        }

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        {
            result = VK_ACCESS_SHADER_READ_BIT;

            break;
        }

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        {
            result = VK_ACCESS_TRANSFER_READ_BIT;

            break;
        }

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        {
            result = VK_ACCESS_TRANSFER_WRITE_BIT;

            break;
        }

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
        {
            result = VK_ACCESS_SHADER_READ_BIT;

            break;
        }

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        {
            result = VK_ACCESS_MEMORY_READ_BIT;

            break;
        }

        default:
        {
            /* Invalid VkImageLayout argument value */
            anvil_assert(false);
        }
    }


    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkAttachmentLoadOp in_load_op)
{
    static const char* attachment_load_op_strings[] =
    {
        "VK_ATTACHMENT_LOAD_OP_LOAD",
        "VK_ATTACHMENT_LOAD_OP_CLEAR",
        "VK_ATTACHMENT_LOAD_OP_DONT_CARE",
    };
    static const uint32_t n_attachment_load_op_strings = sizeof(attachment_load_op_strings) / sizeof(attachment_load_op_strings[0]);

    static_assert(n_attachment_load_op_strings == VK_ATTACHMENT_LOAD_OP_RANGE_SIZE, "");
    anvil_assert (in_load_op                   <= VK_ATTACHMENT_LOAD_OP_END_RANGE);

    return attachment_load_op_strings[in_load_op];
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkAttachmentStoreOp in_store_op)
{
    static const char* attachment_store_op_strings[] =
    {
        "VK_ATTACHMENT_STORE_OP_STORE",
        "VK_ATTACHMENT_STORE_OP_DONT_CARE",
    };
    static const uint32_t n_attachment_store_op_strings = sizeof(attachment_store_op_strings) / sizeof(attachment_store_op_strings[0]);

    static_assert(n_attachment_store_op_strings == VK_ATTACHMENT_STORE_OP_RANGE_SIZE, "");
    anvil_assert (in_store_op                   <= VK_ATTACHMENT_STORE_OP_END_RANGE);

    return attachment_store_op_strings[in_store_op];
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkBlendFactor blend_factor)
{
    const char* result = "?";

    switch (blend_factor)
    {
        case VK_BLEND_FACTOR_ZERO:                     result = "VK_BLEND_FACTOR_ZERO";                     break;
        case VK_BLEND_FACTOR_ONE:                      result = "VK_BLEND_FACTOR_ONE";                      break;
        case VK_BLEND_FACTOR_SRC_COLOR:                result = "VK_BLEND_FACTOR_SRC_COLOR";                break;
        case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:      result = "VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR";      break;
        case VK_BLEND_FACTOR_DST_COLOR:                result = "VK_BLEND_FACTOR_DST_COLOR";                break;
        case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:      result = "VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR";      break;
        case VK_BLEND_FACTOR_SRC_ALPHA:                result = "VK_BLEND_FACTOR_SRC_ALPHA";                break;
        case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:      result = "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA";      break;
        case VK_BLEND_FACTOR_DST_ALPHA:                result = "VK_BLEND_FACTOR_DST_ALPHA";                break;
        case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:      result = "VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA";      break;
        case VK_BLEND_FACTOR_CONSTANT_COLOR:           result = "VK_BLEND_FACTOR_CONSTANT_COLOR";           break;
        case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: result = "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR"; break;
        case VK_BLEND_FACTOR_CONSTANT_ALPHA:           result = "VK_BLEND_FACTOR_CONSTANT_ALPHA";           break;
        case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: result = "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA"; break;
        case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:       result = "VK_BLEND_FACTOR_SRC_ALPHA_SATURATE";       break;
        case VK_BLEND_FACTOR_SRC1_COLOR:               result = "VK_BLEND_FACTOR_SRC1_COLOR";               break;
        case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:     result = "VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR";     break;
        case VK_BLEND_FACTOR_SRC1_ALPHA:               result = "VK_BLEND_FACTOR_SRC1_ALPHA";               break;
        case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:     result = "VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA";     break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkBlendOp blend_op)
{
    const char* result = "?";

    switch (blend_op)
    {
        case VK_BLEND_OP_ADD:              result = "VK_BLEND_OP_ADD";              break;
        case VK_BLEND_OP_SUBTRACT:         result = "VK_BLEND_OP_SUBTRACT";         break;
        case VK_BLEND_OP_REVERSE_SUBTRACT: result = "VK_BLEND_OP_REVERSE_SUBTRACT"; break;
        case VK_BLEND_OP_MIN:              result = "VK_BLEND_OP_MIN";              break;
        case VK_BLEND_OP_MAX:              result = "VK_BLEND_OP_MAX";              break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkCompareOp compare_op)
{
    const char* result = "?";

    switch (compare_op)
    {
        case VK_COMPARE_OP_NEVER:            result = "VK_COMPARE_OP_NEVER";            break;
        case VK_COMPARE_OP_LESS:             result = "VK_COMPARE_OP_LESS";             break;
        case VK_COMPARE_OP_EQUAL:            result = "VK_COMPARE_OP_EQUAL";            break;
        case VK_COMPARE_OP_LESS_OR_EQUAL:    result = "VK_COMPARE_OP_LESS_OR_EQUAL";    break;
        case VK_COMPARE_OP_GREATER:          result = "VK_COMPARE_OP_GREATER";          break;
        case VK_COMPARE_OP_NOT_EQUAL:        result = "VK_COMPARE_OP_NOT_EQUAL";        break;
        case VK_COMPARE_OP_GREATER_OR_EQUAL: result = "VK_COMPARE_OP_GREATER_OR_EQUAL"; break;
        case VK_COMPARE_OP_ALWAYS:           result = "VK_COMPARE_OP_ALWAYS";           break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkCullModeFlagBits cull_mode)
{
    const char* result = "?";

    switch (cull_mode)
    {
        case VK_CULL_MODE_NONE:           result = "VK_CULL_MODE_NONE";           break;
        case VK_CULL_MODE_FRONT_BIT:      result = "VK_CULL_MODE_FRONT_BIT";      break;
        case VK_CULL_MODE_BACK_BIT:       result = "VK_CULL_MODE_BACK_BIT";       break;
        case VK_CULL_MODE_FRONT_AND_BACK: result = "VK_CULL_MODE_FRONT_AND_BACK"; break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkDescriptorType descriptor_type)
{
    const char* result = "?";

    switch (descriptor_type)
    {
        case VK_DESCRIPTOR_TYPE_SAMPLER:                result = "VK_DESCRIPTOR_TYPE_SAMPLER";                break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: result = "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"; break;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:          result = "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";          break;
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:          result = "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";          break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:   result = "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";   break;
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:   result = "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";   break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:         result = "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";         break;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:         result = "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";         break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: result = "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC"; break;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: result = "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC"; break;
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:       result = "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";       break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkFrontFace front_face)
{
    const char* result = "?";

    switch (front_face)
    {
        case VK_FRONT_FACE_COUNTER_CLOCKWISE: result = "VK_FRONT_FACE_COUNTER_CLOCKWISE"; break;
        case VK_FRONT_FACE_CLOCKWISE:         result = "VK_FRONT_FACE_CLOCKWISE";         break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkImageLayout in_image_layout)
{
    const char* result = "?!";

    /* Note: we can't use an array-based solution here because of PRESENT_SRC_KHR */
    switch (in_image_layout)
    {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:         result = "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL";         break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: result = "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL"; break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:  result = "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL";  break;
        case VK_IMAGE_LAYOUT_GENERAL:                          result = "VK_IMAGE_LAYOUT_GENERAL";                          break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:                   result = "VK_IMAGE_LAYOUT_PREINITIALIZED";                   break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                  result = "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR";                  break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:         result = "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL";         break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:             result = "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL";             break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:             result = "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL";             break;
        case VK_IMAGE_LAYOUT_UNDEFINED:                        result = "VK_IMAGE_LAYOUT_UNDEFINED";                        break;

        default:
        {
            anvil_assert(false);

            break;
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkImageTiling in_image_tiling)
{
    static const char* image_tilings[] =
    {
        "VK_IMAGE_TILING_OPTIMAL",
        "VK_IMAGE_TILING_LINEAR"
    };
    static const int32_t n_image_tilings = sizeof(image_tilings) / sizeof(image_tilings[0]);

    static_assert(n_image_tilings == VK_IMAGE_TILING_RANGE_SIZE, "");
    anvil_assert (in_image_tiling <  n_image_tilings);

    return image_tilings[in_image_tiling];
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkImageType in_image_type)
{
    static const char* image_types[] =
    {
        "VK_IMAGE_TYPE_1D",
        "VK_IMAGE_TYPE_2D",
        "VK_IMAGE_TYPE_3D"
    };
    static const uint32_t n_image_types = sizeof(image_types) / sizeof(image_types[0]);

    static_assert(n_image_types == VK_IMAGE_TYPE_RANGE_SIZE, "");
    anvil_assert (in_image_type <  VK_IMAGE_TYPE_RANGE_SIZE);

    return image_types[in_image_type];
}

/** Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkImageViewType in_image_view_type)
{
    static const char* image_view_types[] =
    {
        "VK_IMAGE_VIEW_TYPE_1D",
        "VK_IMAGE_VIEW_TYPE_2D",
        "VK_IMAGE_VIEW_TYPE_3D",
        "VK_IMAGE_VIEW_TYPE_CUBE",
        "VK_IMAGE_VIEW_TYPE_1D_ARRAY",
        "VK_IMAGE_VIEW_TYPE_2D_ARRAY",
        "VK_IMAGE_VIEW_TYPE_CUBE_ARRAY",
    };
    static const uint32_t n_image_view_types = sizeof(image_view_types) / sizeof(image_view_types[0]);

    static_assert(n_image_view_types == VK_IMAGE_VIEW_TYPE_RANGE_SIZE, "");
    anvil_assert (in_image_view_type <  VK_IMAGE_VIEW_TYPE_RANGE_SIZE);

    return image_view_types[in_image_view_type];
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkLogicOp logic_op)
{
    const char* result = "?";

    switch (logic_op)
    {
        case VK_LOGIC_OP_CLEAR:         result = "VK_LOGIC_OP_CLEAR";         break;
        case VK_LOGIC_OP_AND:           result = "VK_LOGIC_OP_AND";           break;
        case VK_LOGIC_OP_AND_REVERSE:   result = "VK_LOGIC_OP_AND_REVERSE";   break;
        case VK_LOGIC_OP_COPY:          result = "VK_LOGIC_OP_COPY";          break;
        case VK_LOGIC_OP_AND_INVERTED:  result = "VK_LOGIC_OP_AND_INVERTED";  break;
        case VK_LOGIC_OP_NO_OP:         result = "VK_LOGIC_OP_NO_OP";         break;
        case VK_LOGIC_OP_XOR:           result = "VK_LOGIC_OP_XOR";           break;
        case VK_LOGIC_OP_OR:            result = "VK_LOGIC_OP_OR";            break;
        case VK_LOGIC_OP_NOR:           result = "VK_LOGIC_OP_NOR";           break;
        case VK_LOGIC_OP_EQUIVALENT:    result = "VK_LOGIC_OP_EQUIVALENT";    break;
        case VK_LOGIC_OP_INVERT:        result = "VK_LOGIC_OP_INVERT";        break;
        case VK_LOGIC_OP_OR_REVERSE:    result = "VK_LOGIC_OP_OR_REVERSE";    break;
        case VK_LOGIC_OP_COPY_INVERTED: result = "VK_LOGIC_OP_COPY_INVERTED"; break;
        case VK_LOGIC_OP_OR_INVERTED:   result = "VK_LOGIC_OP_OR_INVERTED";   break;
        case VK_LOGIC_OP_NAND:          result = "VK_LOGIC_OP_NAND";          break;
        case VK_LOGIC_OP_SET:           result = "VK_LOGIC_OP_SET";           break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkPolygonMode polygon_mode)
{
    const char* result = "?";

    switch (polygon_mode)
    {
        case VK_POLYGON_MODE_FILL:  result = "VK_POLYGON_MODE_FILL";  break;
        case VK_POLYGON_MODE_LINE:  result = "VK_POLYGON_MODE_LINE";  break;
        case VK_POLYGON_MODE_POINT: result = "VK_POLYGON_MODE_POINT"; break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkPrimitiveTopology topology)
{
    const char* result = "?";

    switch (topology)
    {
        case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:                    result = "VK_PRIMITIVE_TOPOLOGY_POINT_LIST";                    break;
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:                     result = "VK_PRIMITIVE_TOPOLOGY_LINE_LIST";                     break;
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:                    result = "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP";                    break;
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:                 result = "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST";                 break;
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:                result = "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP";                break;
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:                  result = "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN";                  break;
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:      result = "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY";      break;
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:     result = "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY";     break;
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:  result = "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY";  break;
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY: result = "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY"; break;
        case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:                    result = "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST";                    break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkSampleCountFlagBits sample_count)
{
    const char* result = "?";

    switch (sample_count)
    {
        case VK_SAMPLE_COUNT_1_BIT:  result = "VK_SAMPLE_COUNT_1_BIT";  break;
        case VK_SAMPLE_COUNT_2_BIT:  result = "VK_SAMPLE_COUNT_2_BIT";  break;
        case VK_SAMPLE_COUNT_4_BIT:  result = "VK_SAMPLE_COUNT_4_BIT";  break;
        case VK_SAMPLE_COUNT_8_BIT:  result = "VK_SAMPLE_COUNT_8_BIT";  break;
        case VK_SAMPLE_COUNT_16_BIT: result = "VK_SAMPLE_COUNT_16_BIT"; break;
        case VK_SAMPLE_COUNT_32_BIT: result = "VK_SAMPLE_COUNT_32_BIT"; break;
        case VK_SAMPLE_COUNT_64_BIT: result = "VK_SAMPLE_COUNT_64_BIT"; break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkSharingMode in_sharing_mode)
{
    static const char* sharing_modes[] =
    {
        "VK_SHARING_MODE_EXCLUSIVE",
        "VK_SHARING_MODE_CONCURRENT"
    };
    static const int32_t n_sharing_modes = sizeof(sharing_modes) / sizeof(sharing_modes[0]);

    static_assert(n_sharing_modes == VK_SHARING_MODE_RANGE_SIZE, "");
    anvil_assert (in_sharing_mode <  n_sharing_modes);

    return sharing_modes[in_sharing_mode];
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkStencilOp stencil_op)
{
    const char* result = "?";

    switch (stencil_op)
    {
        case VK_STENCIL_OP_KEEP:                result = "VK_STENCIL_OP_KEEP";                break;
        case VK_STENCIL_OP_ZERO:                result = "VK_STENCIL_OP_ZERO";                break;
        case VK_STENCIL_OP_REPLACE:             result = "VK_STENCIL_OP_REPLACE";             break;
        case VK_STENCIL_OP_INCREMENT_AND_CLAMP: result = "VK_STENCIL_OP_INCREMENT_AND_CLAMP"; break;
        case VK_STENCIL_OP_DECREMENT_AND_CLAMP: result = "VK_STENCIL_OP_DECREMENT_AND_CLAMP"; break;
        case VK_STENCIL_OP_INVERT:              result = "VK_STENCIL_OP_INVERT";              break;
        case VK_STENCIL_OP_INCREMENT_AND_WRAP:  result = "VK_STENCIL_OP_INCREMENT_AND_WRAP";  break;
        case VK_STENCIL_OP_DECREMENT_AND_WRAP:  result = "VK_STENCIL_OP_DECREMENT_AND_WRAP";  break;

        default:
        {
            anvil_assert(false);
        }
    }

    return result;
}

