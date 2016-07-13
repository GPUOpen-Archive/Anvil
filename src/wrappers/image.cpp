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
#include "misc/object_tracker.h"
#include "wrappers/buffer.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include "wrappers/physical_device.h"
#include "wrappers/queue.h"
#include <math.h>

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1d_texture_mipmap_raw_data(VkImageAspectFlagBits in_aspect,
                                                                             uint32_t              in_n_mipmap,
                                                                             const void*           in_linear_tightly_packed_data_ptr,
                                                                             uint32_t              in_row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect                         = in_aspect;
    result.data_size                      = in_row_size;
    result.row_size                       = in_row_size;
    result.linear_tightly_packed_data_ptr = in_linear_tightly_packed_data_ptr;
    result.n_layers                       = 1;
    result.n_slices                       = 1;
    result.n_mipmap                       = in_n_mipmap;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1d_array_texture_mipmap_raw_data(VkImageAspectFlagBits in_aspect,
                                                                                   uint32_t              in_n_layer,
                                                                                   uint32_t              in_n_layers,
                                                                                   uint32_t              in_n_mipmap,
                                                                                   const void*           in_linear_tightly_packed_data_ptr,
                                                                                   uint32_t              in_row_size,
                                                                                   uint32_t              in_data_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect                         = in_aspect;
    result.data_size                      = in_data_size;
    result.linear_tightly_packed_data_ptr = in_linear_tightly_packed_data_ptr;
    result.n_layer                        = in_n_layer;
    result.n_layers                       = in_n_layers;
    result.n_mipmap                       = in_n_mipmap;
    result.n_slices                       = 1;
    result.row_size                       = in_row_size;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2d_texture_mipmap_raw_data(VkImageAspectFlagBits in_aspect,
                                                                             uint32_t              in_n_mipmap,
                                                                             const void*           in_linear_tightly_packed_data_ptr,
                                                                             uint32_t              in_data_size,
                                                                             uint32_t              in_row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect                         = in_aspect;
    result.data_size                      = in_data_size;
    result.linear_tightly_packed_data_ptr = in_linear_tightly_packed_data_ptr;
    result.n_layers                       = 1;
    result.n_mipmap                       = in_n_mipmap;
    result.n_slices                       = 1;
    result.row_size                       = in_row_size;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2d_array_texture_mipmap_raw_data(VkImageAspectFlagBits in_aspect,
                                                                                   uint32_t              in_n_layer,
                                                                                   uint32_t              in_n_layers,
                                                                                   uint32_t              in_n_mipmap,
                                                                                   const void*           in_linear_tightly_packed_data_ptr,
                                                                                   uint32_t              in_data_size,
                                                                                   uint32_t              in_row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect                         = in_aspect;
    result.data_size                      = in_data_size;
    result.linear_tightly_packed_data_ptr = in_linear_tightly_packed_data_ptr;
    result.n_layer                        = in_n_layer;
    result.n_layers                       = in_n_layers;
    result.n_mipmap                       = in_n_mipmap;
    result.n_slices                       = 1;
    result.row_size                       = in_row_size;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_3d_texture_mipmap_raw_data(VkImageAspectFlagBits in_aspect,
                                                                             uint32_t              in_n_layer,
                                                                             uint32_t              in_n_slices,
                                                                             uint32_t              in_n_mipmap,
                                                                             const void*           in_linear_tightly_packed_data_ptr,
                                                                             uint32_t              in_data_size,
                                                                             uint32_t              in_row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect                         = in_aspect;
    result.data_size                      = in_data_size;
    result.linear_tightly_packed_data_ptr = in_linear_tightly_packed_data_ptr;
    result.n_layers                       = 1;
    result.n_layer                        = in_n_layer;
    result.n_slices                       = in_n_slices;
    result.n_mipmap                       = in_n_mipmap;
    result.row_size                       = in_row_size;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_texture_mipmap_raw_data(VkImageAspectFlagBits aspect,
                                                                                   uint32_t              n_layer,
                                                                                   uint32_t              n_mipmap,
                                                                                   const void*           linear_tightly_packed_data_ptr,
                                                                                   uint32_t              data_size,
                                                                                   uint32_t              row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    anvil_assert(n_layer < 6);

    return Anvil::MipmapRawData::create_2d_array_texture_mipmap_raw_data(aspect,
                                                                          n_layer,
                                                                          1, /* n_layer_slices */
                                                                          n_mipmap,
                                                                          linear_tightly_packed_data_ptr,
                                                                          data_size,
                                                                          row_size);
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_array_texture_mipmap_raw_data(VkImageAspectFlagBits aspect,
                                                                                         uint32_t              n_layer,
                                                                                         uint32_t              n_layers,
                                                                                         uint32_t              n_mipmap,
                                                                                         const void*           linear_tightly_packed_data_ptr,
                                                                                         uint32_t              data_size,
                                                                                         uint32_t              row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    return Anvil::MipmapRawData::create_2d_array_texture_mipmap_raw_data(aspect,
                                                                          n_layer,
                                                                          n_layers,
                                                                          n_mipmap,
                                                                          linear_tightly_packed_data_ptr,
                                                                          data_size,
                                                                          row_size);
}


/* Please see header for specification */
Anvil::Image::Image(Anvil::Device*        device_ptr,
                    VkImageType           type,
                    VkFormat              format,
                    VkImageTiling         tiling,
                    VkImageUsageFlags     usage,
                    uint32_t              base_mipmap_width,
                    uint32_t              base_mipmap_height,
                    uint32_t              base_mipmap_depth,
                    uint32_t              n_layers,
                    VkSampleCountFlagBits sample_count,
                    bool                  use_full_mipmap_chain,
                    bool                  is_mutable)
    :m_device_ptr              (device_ptr),
     m_image                   (VK_NULL_HANDLE),
     m_image_alignment         (0),
     m_image_depth             (base_mipmap_depth),
     m_image_format            (format),
     m_image_height            (base_mipmap_height),
     m_image_is_mutable        (is_mutable),
     m_image_layout_at_creation(VK_IMAGE_LAYOUT_MAX_ENUM),
     m_image_memory_types      (0),
     m_image_n_mipmaps         (0),
     m_image_n_layers          (n_layers),
     m_image_n_slices          (0),
     m_image_owner             (true),
     m_image_storage_size      (0),
     m_image_tiling            (tiling),
     m_image_usage             (static_cast<VkImageUsageFlagBits>(usage)),
     m_image_width             (base_mipmap_width),
     m_memory_block_ptr        (nullptr),
     m_memory_owner            (false)
{
    init(type,
         format,
         tiling,
         usage,
         base_mipmap_width,
         base_mipmap_height,
         base_mipmap_depth,
         n_layers,
         sample_count,
         0,                 /* queue_families - irrelevant */
         (VkSharingMode) 0, /* sharing_mode   - irrelevant */
         use_full_mipmap_chain,
         false,             /* memory_mappable        - irrelevant */
         false,             /* memory_coherent        - irrelevant */
         nullptr,           /* start_image_layout_ptr - irrelevant */
         nullptr,           /* final_image_layout_ptr - irrelevant */
         nullptr);          /* mipmaps_raw_ptr        - irrelevant */
}

/* Please see header for specification */
Anvil::Image::Image(Anvil::Device*                    device_ptr,
                    VkImageType                       type,
                    VkFormat                          format,
                    VkImageTiling                     tiling,
                    VkImageUsageFlags                 usage,
                    uint32_t                          base_mipmap_width,
                    uint32_t                          base_mipmap_height,
                    uint32_t                          base_mipmap_depth,
                    uint32_t                          n_layers,
                    VkSampleCountFlagBits             sample_count,
                    Anvil::QueueFamilyBits            queue_families,
                    VkSharingMode                     sharing_mode,
                    bool                              use_full_mipmap_chain,
                    bool                              should_memory_backing_be_mappable,
                    bool                              should_memory_backing_be_coherent,
                    bool                              is_mutable,
                    VkImageLayout                     final_image_layout,
                    const std::vector<MipmapRawData>* mipmaps_ptr)
    :m_device_ptr              (device_ptr),
     m_image                   (VK_NULL_HANDLE),
     m_image_alignment         (0),
     m_image_depth             (base_mipmap_depth),
     m_image_format            (format),
     m_image_height            (base_mipmap_height),
     m_image_is_mutable        (is_mutable),
     m_image_layout_at_creation(final_image_layout),
     m_image_memory_types      (0),
     m_image_n_layers          (n_layers),
     m_image_n_mipmaps         (0),
     m_image_n_slices          (0),
     m_image_owner             (true),
     m_image_storage_size      (0),
     m_image_tiling            (tiling),
     m_image_usage             (static_cast<VkImageUsageFlagBits>(usage)),
     m_image_width             (base_mipmap_width),
     m_memory_block_ptr        (nullptr),
     m_memory_owner            (true)
{
    const VkImageLayout start_image_layout = (mipmaps_ptr != nullptr && mipmaps_ptr->size() > 0) ? VK_IMAGE_LAYOUT_PREINITIALIZED
                                                                                              : VK_IMAGE_LAYOUT_UNDEFINED;

    init(type,
         format,
         tiling,
         usage,
         base_mipmap_width,
         base_mipmap_height,
         base_mipmap_depth,
         n_layers,
         sample_count,
         queue_families,
         sharing_mode,
         use_full_mipmap_chain,
         should_memory_backing_be_mappable,
         should_memory_backing_be_coherent,
        &start_image_layout,
        &final_image_layout,
         mipmaps_ptr);
}

/* Please see header for specification */
Anvil::Image::Image(Anvil::Device*        device_ptr,
                    VkImage               image,
                    VkFormat              format,
                    VkImageTiling         tiling,
                    VkImageUsageFlags     usage,
                    uint32_t              base_mipmap_width,
                    uint32_t              base_mipmap_height,
                    uint32_t              base_mipmap_depth,
                    uint32_t              n_layers,
                    uint32_t              n_mipmaps,
                    VkSampleCountFlagBits sample_count,
                    uint32_t              n_slices,
                    bool                  is_mutable)
    :m_device_ptr              (device_ptr),
     m_image                   (image),
     m_image_alignment         (0),
     m_image_depth             (base_mipmap_depth),
     m_image_format            (format),
     m_image_height            (base_mipmap_height),
     m_image_is_mutable        (is_mutable),
     m_image_layout_at_creation(VK_IMAGE_LAYOUT_MAX_ENUM),
     m_image_memory_types      (-1),
     m_image_n_layers          (n_layers),
     m_image_n_mipmaps         (n_mipmaps),
     m_image_n_slices          (n_slices),
     m_image_owner             (false),
     m_image_sample_count      (sample_count),
     m_image_storage_size      (-1),
     m_image_tiling            (tiling),
     m_image_usage             (static_cast<VkImageUsageFlagBits>(usage)),
     m_image_width             (base_mipmap_width),
     m_memory_block_ptr        (nullptr),
     m_memory_owner            (false)
{
    init_mipmap_props();

    /* Register this instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_IMAGE,
                                                  this);
}

/** Please see header for specification */
VkAccessFlags Anvil::Image::get_access_mask_from_image_layout(VkImageLayout layout)
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
            anvil_assert(false && "Invalid VkImageLayout argument value");
        }
    }


    return result;
}

/** Private function which initializes the Image instance.
 *
 *  For argument discussion, please see documentation of the constructors.
 **/
void Anvil::Image::init(VkImageType                       type,
                        VkFormat                          format,
                        VkImageTiling                     tiling,
                        VkImageUsageFlags                 usage,
                        uint32_t                          base_mipmap_width,
                        uint32_t                          base_mipmap_height,
                        uint32_t                          base_mipmap_depth,
                        uint32_t                          n_layers,
                        VkSampleCountFlagBits             sample_count,
                        Anvil::QueueFamilyBits            queue_families,
                        VkSharingMode                     sharing_mode,
                        bool                              use_full_mipmap_chain,
                        bool                              memory_mappable,
                        bool                              memory_coherent,
                        const VkImageLayout*              start_image_layout_ptr,
                        const VkImageLayout*              final_image_layout_ptr,
                        const std::vector<MipmapRawData>* mipmaps_ptr)
{
    VkImageFormatProperties image_format_props;
    uint32_t                max_dimension;
    VkMemoryRequirements    memory_reqs;
    uint32_t                n_queue_family_indices  = 0;
    uint32_t                queue_family_indices[3];
    VkResult                result;

    if (m_memory_owner && !memory_mappable)
    {
        anvil_assert(!memory_coherent);
    }

    /* Determine the maximum dimension size. */
    max_dimension = base_mipmap_width;

    if (max_dimension < base_mipmap_height)
    {
        max_dimension = base_mipmap_height;
    }

    if (max_dimension < base_mipmap_depth)
    {
        max_dimension = base_mipmap_depth;
    }

    /* Form the queue family array */
    if (queue_families & Anvil::QUEUE_FAMILY_COMPUTE_BIT)
    {
        queue_family_indices[n_queue_family_indices] = m_device_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_COMPUTE);

        ++n_queue_family_indices;
    }

    if (queue_families & Anvil::QUEUE_FAMILY_DMA_BIT)
    {
        queue_family_indices[n_queue_family_indices] = m_device_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_TRANSFER);

        ++n_queue_family_indices;
    }

    if (queue_families & Anvil::QUEUE_FAMILY_GRAPHICS_BIT)
    {
        queue_family_indices[n_queue_family_indices] = m_device_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL);

        ++n_queue_family_indices;
    }

    /* Is the requested texture size valid? */
    result = vkGetPhysicalDeviceImageFormatProperties(m_device_ptr->get_physical_device()->get_physical_device(),
                                                      format,
                                                      type,
                                                      tiling,
                                                      usage,
                                                      0, /* flags */
                                                     &image_format_props);
    anvil_assert_vk_call_succeeded(result);

    anvil_assert(base_mipmap_width  >= 1 &&
                 base_mipmap_height >= 1 &&
                 base_mipmap_depth  >= 1);

    anvil_assert(base_mipmap_width <= (uint32_t) image_format_props.maxExtent.width);

    if (base_mipmap_height > 1)
    {
        anvil_assert(base_mipmap_height <= (uint32_t) image_format_props.maxExtent.height);
    }

    if (base_mipmap_depth > 1)
    {
        anvil_assert(base_mipmap_depth <= (uint32_t) image_format_props.maxExtent.depth);
    }

    anvil_assert(n_layers >= 1);

    if (n_layers > 1)
    {
        anvil_assert(n_layers <= (uint32_t) image_format_props.maxArrayLayers);
    }

    /* If multisample image is requested, make sure the number of samples is supported. */
    anvil_assert(sample_count >= 1);

    if (sample_count > 1)
    {
        anvil_assert((image_format_props.sampleCounts & sample_count) != 0);
    }

    /* Create the image object */
    VkImageLayout      current_image_layout;
    VkImageCreateInfo  image_create_info;
    VkImageCreateFlags image_flags = 0;

    if (type == VK_IMAGE_TYPE_2D)
    {
        if ((n_layers % 6) == 0)
        {
            image_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }
    }

    image_create_info.arrayLayers           = n_layers;
    image_create_info.extent.depth          = base_mipmap_depth;
    image_create_info.extent.height         = base_mipmap_height;
    image_create_info.extent.width          = base_mipmap_width;
    image_create_info.flags                 = image_flags | ((m_image_is_mutable) ? VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT
                                                                                  : 0);
    image_create_info.format                = format;
    image_create_info.imageType             = type;
    image_create_info.initialLayout         = (start_image_layout_ptr != nullptr) ? *start_image_layout_ptr
                                                                               : VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.mipLevels             = (uint32_t) ((use_full_mipmap_chain) ? (1 + log2(max_dimension)) : 1);
    image_create_info.pNext                 = nullptr;
    image_create_info.pQueueFamilyIndices   = queue_family_indices;
    image_create_info.queueFamilyIndexCount = n_queue_family_indices;
    image_create_info.samples               = sample_count;
    image_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.tiling                = tiling;
    image_create_info.usage                 = usage;

    result = vkCreateImage(m_device_ptr->get_device_vk(),
                          &image_create_info,
                           nullptr, /* pAllocator */
                          &m_image);
    anvil_assert_vk_call_succeeded(result);

    current_image_layout = image_create_info.initialLayout;
    m_image_n_mipmaps    = image_create_info.mipLevels;
    m_image_n_slices     = (type == VK_IMAGE_TYPE_3D) ? base_mipmap_depth : 1;
    m_image_sample_count = sample_count;

    /* If we are to own the memory backing, allocate it now. If not, this action will be handled by the MemoryBlock instance.
     * The object will then update the memory backing properties by firing a couple of set_() calls against this object. */
    vkGetImageMemoryRequirements(m_device_ptr->get_device_vk(),
                                 m_image,
                                &memory_reqs);

    m_image_alignment    = memory_reqs.alignment;
    m_image_memory_types = memory_reqs.memoryTypeBits;
    m_image_storage_size = memory_reqs.size;

    if (m_memory_owner)
    {
        /* Allocate memory for the image */
        m_memory_block_ptr = new Anvil::MemoryBlock(m_device_ptr,
                                                     memory_reqs.memoryTypeBits,
                                                     memory_reqs.size,
                                                     memory_mappable,
                                                     memory_coherent);

        set_memory(m_memory_block_ptr);

        m_memory_block_ptr->release();
    }

    /* Initialize mipmap props storage */
    init_mipmap_props();

    /* Fill the storage with mipmap contents, if mipmap data was specified at input */
    if (image_create_info.initialLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
    {
        anvil_assert(mipmaps_ptr != nullptr);

        upload_mipmaps(*mipmaps_ptr,
                       &current_image_layout);
    }

    /* Transition the image to the final layout, if one was requested by the caller */
    if ( final_image_layout_ptr != nullptr                   &&
        *final_image_layout_ptr != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        Anvil::PrimaryCommandBuffer* transition_command_buffer_ptr = nullptr;
        Anvil::Queue*                universal_queue_ptr           = m_device_ptr->get_universal_queue(0);

        transition_command_buffer_ptr = m_device_ptr->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)->alloc_primary_level_command_buffer();

        transition_command_buffer_ptr->start_recording(true,   /* one_time_submit          */
                                                       false); /* simultaneous_use_allowed */
        {
            Anvil::ImageBarrier image_barrier(((image_create_info.initialLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)       ? VK_ACCESS_HOST_WRITE_BIT     : 0) |
                                               ((current_image_layout           == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) ? VK_ACCESS_TRANSFER_WRITE_BIT : 0), /* source_access_mask */
                                               get_access_mask_from_image_layout(*final_image_layout_ptr),
                                               false,
                                               current_image_layout,
                                              *final_image_layout_ptr,
                                               universal_queue_ptr->get_queue_family_index(),
                                               universal_queue_ptr->get_queue_family_index(),
                                               this,
                                               get_subresource_range() );

            transition_command_buffer_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                                   VK_FALSE,       /* in_by_region                   */
                                                                   0,              /* in_memory_barrier_count        */
                                                                   nullptr,           /* in_memory_barrier_ptrs         */
                                                                   0,              /* in_buffer_memory_barrier_count */
                                                                   nullptr,           /* in_buffer_memory_barrier_ptrs  */
                                                                   1,              /* in_image_memory_barrier_count  */
                                                                  &image_barrier);
        }
        transition_command_buffer_ptr->stop_recording();

        universal_queue_ptr->submit_command_buffer(transition_command_buffer_ptr,
                                                   true /* should_block */);

        transition_command_buffer_ptr->release();
    }

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_IMAGE,
                                                  this);
}

/** Releases the Vulkan image object, as well as the memory object associated with the Image instance. */
Anvil::Image::~Image()
{
    if (m_image != VK_NULL_HANDLE &&
        m_image_owner)
    {
        vkDestroyImage(m_device_ptr->get_device_vk(),
                       m_image,
                       nullptr /* pAllocator */);

        m_image = VK_NULL_HANDLE;
    }

    if (m_memory_block_ptr != nullptr)
    {
        m_memory_block_ptr->release();

        m_memory_block_ptr = nullptr;
    }

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_IMAGE,
                                                    this);
}

/* Please see header for specification */
bool Anvil::Image::get_image_layout_at_creation_time(VkImageLayout* out_result_ptr) const
{
    bool result = (m_image_layout_at_creation != VK_IMAGE_LAYOUT_MAX_ENUM);

    if (result)
    {
        *out_result_ptr = m_image_layout_at_creation;
    }

    return result;
}

/* Please see header for specification */
bool Anvil::Image::get_image_mipmap_size(uint32_t  n_mipmap,
                                         uint32_t* opt_out_width_ptr,
                                         uint32_t* opt_out_height_ptr,
                                         uint32_t* opt_out_depth_ptr) const
{
    bool result = false;

    /* Is this a sensible mipmap index? */
    if (m_image_mipmaps.size() <= n_mipmap)
    {
        goto end;
    }

    /* Return the result data.. */
    if (opt_out_width_ptr != nullptr)
    {
        *opt_out_width_ptr = m_image_mipmaps[n_mipmap].width;
    }

    if (opt_out_height_ptr != nullptr)
    {
        *opt_out_height_ptr = m_image_mipmaps[n_mipmap].height;
    }

    if (opt_out_depth_ptr != nullptr)
    {
        *opt_out_depth_ptr = m_image_mipmaps[n_mipmap].depth;
    }

    /* All done */
    result = true;

end:
    return result;
}

/** Please see header for specification */
VkImageSubresourceRange Anvil::Image::get_subresource_range() const
{
    VkImageSubresourceRange result;

    switch (m_image_format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        {
            result.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            break;
        }

        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        {
            result.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

            break;
        }

        case VK_FORMAT_S8_UINT:
        {
            result.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;

            break;
        }

        default:
        {
            result.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            break;
        }
    }

    result.baseArrayLayer = 0;
    result.baseMipLevel   = 0;
    result.layerCount     = m_image_n_layers;
    result.levelCount     = m_image_n_mipmaps;

    return result;
}

/** Fills the m_mipmaps vector with mipmap size data. */
void Anvil::Image::init_mipmap_props()
{
    uint32_t current_mipmap_size[3] =
    {
        m_image_width,
        m_image_height,
        m_image_depth
    };

    anvil_assert(m_image_n_mipmaps      != 0);
    anvil_assert(m_image_mipmaps.size() == 0);

    for (uint32_t mipmap_level = 0;
                  mipmap_level < m_image_n_mipmaps;
                ++mipmap_level)
    {
        m_image_mipmaps.push_back(Mipmap(current_mipmap_size[0],
                                         current_mipmap_size[1],
                                         current_mipmap_size[2]) );

        current_mipmap_size[0] /= 2;
        current_mipmap_size[1] /= 2;
        current_mipmap_size[2] /= 2;

        if (current_mipmap_size[0] < 1)
        {
            current_mipmap_size[0] = 1;
        }

        if (current_mipmap_size[1] < 1)
        {
            current_mipmap_size[1] = 1;
        }

        if (current_mipmap_size[2] < 1)
        {
            current_mipmap_size[2] = 1;
        }
    }
}

/* Please see header for specification */
void Anvil::Image::set_creation_time_image_layout(VkImageLayout new_image_layout)
{
    anvil_assert(m_image_layout_at_creation == VK_IMAGE_LAYOUT_MAX_ENUM);
    
    if (m_image_layout_at_creation == VK_IMAGE_LAYOUT_MAX_ENUM)
    {
        m_image_layout_at_creation = new_image_layout;
    }
}

/* Please see header for specification */
void Anvil::Image::set_memory(Anvil::MemoryBlock* memory_block_ptr)
{
    VkResult result;

    /* Sanity checks */
    anvil_assert(memory_block_ptr   != nullptr);
    anvil_assert(m_memory_block_ptr == nullptr || m_memory_block_ptr == memory_block_ptr);

    /* Bind the memory object to the image object */
    m_memory_block_ptr = memory_block_ptr;
    result             = vkBindImageMemory(m_device_ptr->get_device_vk(),
                                           m_image,
                                           m_memory_block_ptr->get_memory(),
                                           m_memory_block_ptr->get_start_offset() );

    anvil_assert_vk_call_succeeded(result);

    memory_block_ptr->retain();
}

/** Uploads data of one or more mip-maps to the image instance. Handles both linear and optimal images.
 *
 *  This function assumes the image is in PREINITIALIZED layout.
 *
 *  @param mipmaps A vector of MipmapRawData descriptors, describing which mip-maps should be updated
 *                 with what data.
 *
 **/
void Anvil::Image::upload_mipmaps(const std::vector<Anvil::MipmapRawData>& mipmaps,
                                  VkImageLayout*                           out_new_image_layout_ptr)
{
    std::map<VkImageAspectFlagBits, std::vector<const Anvil::MipmapRawData*> > image_aspect_to_mipmap_raw_data_map;
    VkImageAspectFlags                                                          image_aspects_touched = 0;
    VkImageSubresourceRange                                                     image_subresource_range;
    uint32_t                                                                    max_layer_index  = -1;
    uint32_t                                                                    max_mipmap_index = -1;
    uint32_t                                                                    min_layer_index  = -1;
    uint32_t                                                                    min_mipmap_index = -1;
    bool                                                                        result_bool      = false;

    /* Each image aspect needs to be modified separately. Iterate over the input vector and move MipmapRawData
     * to separate vectors corresponding to which aspect they need to update. */
    for (auto mipmap_iterator =  mipmaps.begin();
              mipmap_iterator != mipmaps.end();
            ++mipmap_iterator)
    {
        image_aspect_to_mipmap_raw_data_map[mipmap_iterator->aspect].push_back(&(*mipmap_iterator));
    }

     /* Determine the max/min layer & mipmap indices */
    anvil_assert(mipmaps.size() > 0);

    for (auto mipmap_iterator =  mipmaps.begin();
              mipmap_iterator != mipmaps.end();
            ++mipmap_iterator)
    {
        image_aspects_touched |= mipmap_iterator->aspect;

        if (max_layer_index == -1                       ||
            max_layer_index <  mipmap_iterator->n_layer)
        {
            max_layer_index = mipmap_iterator->n_layer;
        }

        if (max_mipmap_index == -1                        ||
            max_mipmap_index <  mipmap_iterator->n_mipmap)
        {
            max_mipmap_index = mipmap_iterator->n_mipmap;
        }

        if (min_layer_index == -1                        ||
            min_layer_index >  mipmap_iterator->n_layer)
        {
            min_layer_index = mipmap_iterator->n_layer;
        }

        if (min_mipmap_index == -1 ||
            min_mipmap_index >  mipmap_iterator->n_mipmap)
        {
            min_mipmap_index = mipmap_iterator->n_mipmap;
        }
    }

    anvil_assert(max_layer_index  < m_image_n_layers);
    anvil_assert(max_mipmap_index < m_image_n_mipmaps);

    /* Fill the buffer memory with data, according to the specified layout requirements,
     * if linear tiling is used.
     *
     * For optimal tiling, we need to copy the raw data to temporary buffer
     * and use vkCmdCopyBufferToImage() to let the driver rearrange the data as needed.
     */
    image_subresource_range.aspectMask     = image_aspects_touched;
    image_subresource_range.baseArrayLayer = min_layer_index;
    image_subresource_range.baseMipLevel   = min_mipmap_index;
    image_subresource_range.layerCount     = max_layer_index  - min_layer_index  + 1;
    image_subresource_range.levelCount     = max_mipmap_index - min_mipmap_index + 1;

    if (m_image_tiling == VK_IMAGE_TILING_LINEAR)
    {
        unsigned char* image_memory_ptr = nullptr;

        result_bool = m_memory_block_ptr->map(0, /* start_offset */
                                              m_memory_block_ptr->get_size() );
        anvil_assert(result_bool);

        for (auto   aspect_to_mipmap_data_iterator  = image_aspect_to_mipmap_raw_data_map.begin();
                    aspect_to_mipmap_data_iterator != image_aspect_to_mipmap_raw_data_map.end();
                  ++aspect_to_mipmap_data_iterator)
        {
            VkImageAspectFlagBits current_aspect        = aspect_to_mipmap_data_iterator->first;
            const auto&           mipmap_raw_data_items = aspect_to_mipmap_data_iterator->second;

            for (auto mipmap_raw_data_item_iterator  = mipmap_raw_data_items.begin();
                      mipmap_raw_data_item_iterator != mipmap_raw_data_items.end();
                    ++mipmap_raw_data_item_iterator)
            {
                uint32_t             current_mipmap_height            = 0;
                const auto&          current_mipmap_raw_data_item_ptr = *mipmap_raw_data_item_iterator;
                uint32_t             current_mipmap_slices            = 0;
                uint32_t             current_row_size                 = 0;
                uint32_t             current_slice_size               = 0;
                VkDeviceSize         dst_slice_offset                 = 0;
                const unsigned char* src_slice_ptr                    = nullptr;
                VkImageSubresource   image_subresource;
                VkSubresourceLayout  image_subresource_layout;

                image_subresource.arrayLayer = current_mipmap_raw_data_item_ptr->n_layer;
                image_subresource.aspectMask = current_aspect;
                image_subresource.mipLevel   = current_mipmap_raw_data_item_ptr->n_mipmap;

                vkGetImageSubresourceLayout(m_device_ptr->get_device_vk(),
                                            m_image,
                                           &image_subresource,
                                           &image_subresource_layout);

                /* Determine row size for the mipmap.
                 *
                 * NOTE: Current implementation can only handle power-of-two textures.
                 */
                anvil_assert(m_image_height == 1 || (m_image_height % 2) == 0);

                current_mipmap_height = m_image_height                               / (1 << current_mipmap_raw_data_item_ptr->n_mipmap);
                current_mipmap_slices = current_mipmap_raw_data_item_ptr->n_slices   / (1 << current_mipmap_raw_data_item_ptr->n_mipmap);
                current_row_size      = current_mipmap_raw_data_item_ptr->row_size   / (1 << current_mipmap_raw_data_item_ptr->n_mipmap);
                current_slice_size    = (current_row_size * current_mipmap_height);

                if (current_mipmap_slices < 1)
                {
                    current_mipmap_slices = 1;
                }

                for (unsigned int n_slice = 0;
                                  n_slice < current_mipmap_slices;
                                ++n_slice)
                {
                    dst_slice_offset = image_subresource_layout.offset                                                                   +
                                       image_subresource_layout.depthPitch                                                     * n_slice;
                    src_slice_ptr    = (const unsigned char*) current_mipmap_raw_data_item_ptr->linear_tightly_packed_data_ptr           +
                                       current_slice_size                                                                      * n_slice;

                    for (unsigned int n_row = 0;
                                      n_row < current_mipmap_height;
                                    ++n_row, dst_slice_offset += image_subresource_layout.rowPitch)
                    {
                        m_memory_block_ptr->write(dst_slice_offset,
                                                  current_row_size,
                                                  src_slice_ptr + current_row_size * n_row);
                    }
                }
            }
        }

        m_memory_block_ptr->unmap();

        *out_new_image_layout_ptr = VK_IMAGE_LAYOUT_PREINITIALIZED;
    }
    else
    {
        anvil_assert(m_image_tiling == VK_IMAGE_TILING_OPTIMAL);

        Anvil::Buffer*               temp_buffer_ptr       = nullptr;
        Anvil::PrimaryCommandBuffer* temp_cmdbuf_ptr       = nullptr;
        VkDeviceSize                  total_raw_mips_size   = 0;
        Anvil::Queue*                universal_queue_ptr   = m_device_ptr->get_universal_queue(0);

        /* Count how much space all specified mipmaps take in raw format. */
        for (auto mipmap_iterator  = mipmaps.begin();
                  mipmap_iterator != mipmaps.end();
                ++mipmap_iterator)
        {
            total_raw_mips_size += mipmap_iterator->n_slices * mipmap_iterator->data_size;
        }

        /* Merge data of all mips into one buffer, cache the offsets and push the merged data
         * to the buffer memory. */
        std::vector<VkDeviceSize> mip_data_offsets;

        VkDeviceSize current_mip_offset = 0;
        char*        merged_mip_storage = nullptr;

        merged_mip_storage = new char[total_raw_mips_size];

        /* NOTE: The memcpy() call, as well as the way we implement copy op calls below, assume
         *       POT resolution of the base mipmap
         */
        anvil_assert(m_image_height < 2 || (m_image_height % 2) == 0);

        for (auto mipmap_iterator  = mipmaps.begin();
                  mipmap_iterator != mipmaps.end();
                ++mipmap_iterator)
        {
            const auto& current_mipmap = *mipmap_iterator;

            mip_data_offsets.push_back(current_mip_offset);

            memcpy(merged_mip_storage + current_mip_offset,
                   current_mipmap.linear_tightly_packed_data_ptr,
                   current_mipmap.n_slices * current_mipmap.data_size);

            current_mip_offset += current_mipmap.n_slices * current_mipmap.data_size;
        }

        temp_buffer_ptr = new Anvil::Buffer(m_device_ptr,
                                             total_raw_mips_size,
                                             Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
                                             VK_SHARING_MODE_EXCLUSIVE,
                                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                             false, /* should_be_mappable */
                                             false, /* should_be_coherent */
                                             merged_mip_storage);

        delete [] merged_mip_storage;
        merged_mip_storage = nullptr;

        /* Set up a command buffer we will use to copy the data to the image */
        temp_cmdbuf_ptr = m_device_ptr->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)->alloc_primary_level_command_buffer();
        anvil_assert(temp_cmdbuf_ptr != nullptr);

        temp_cmdbuf_ptr->start_recording(true, /* one_time_submit          */
                                         false /* simultaneous_use_allowed */);
        {
            std::vector<VkBufferImageCopy> copy_regions;

            /* Transfer the image to the transfer_destination layout */
            Anvil::ImageBarrier image_barrier(0, /* source_access_mask */
                                               VK_ACCESS_TRANSFER_WRITE_BIT,
                                               false,
                                               VK_IMAGE_LAYOUT_UNDEFINED,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               universal_queue_ptr->get_queue_family_index(),
                                               universal_queue_ptr->get_queue_family_index(),
                                               this,
                                               image_subresource_range);

            temp_cmdbuf_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                     VK_FALSE,       /* in_by_region                   */
                                                     0,              /* in_memory_barrier_count        */
                                                     nullptr,           /* in_memory_barrier_ptrs         */
                                                     0,              /* in_buffer_memory_barrier_count */
                                                     nullptr,           /* in_buffer_memory_barrier_ptrs  */
                                                     1,              /* in_image_memory_barrier_count  */
                                                    &image_barrier);

            /* Issue the buffer->image copy op */
            copy_regions.reserve(mipmaps.size() );

            for (auto mipmap_iterator  = mipmaps.begin();
                      mipmap_iterator != mipmaps.end();
                    ++mipmap_iterator)
            {
                VkBufferImageCopy current_copy_region;
                const auto&       current_mipmap = *mipmap_iterator;

                current_copy_region.bufferImageHeight               = m_image_height / (1 << current_mipmap.n_mipmap);
                current_copy_region.bufferOffset                    = mip_data_offsets[static_cast<uint32_t>(mipmap_iterator - mipmaps.begin()) ];
                current_copy_region.bufferRowLength                 = 0;
                current_copy_region.imageOffset.x                   = 0;
                current_copy_region.imageOffset.y                   = 0;
                current_copy_region.imageOffset.z                   = 0;
                current_copy_region.imageSubresource.baseArrayLayer = current_mipmap.n_layer;
                current_copy_region.imageSubresource.layerCount     = current_mipmap.n_layers;
                current_copy_region.imageSubresource.aspectMask     = current_mipmap.aspect;
                current_copy_region.imageSubresource.mipLevel       = current_mipmap.n_mipmap;
                current_copy_region.imageExtent.depth               = current_mipmap.n_slices;
                current_copy_region.imageExtent.height              = m_image_height          / (1 << current_mipmap.n_mipmap);
                current_copy_region.imageExtent.width               = m_image_width           / (1 << current_mipmap.n_mipmap);

                if (current_copy_region.imageExtent.depth < 1)
                {
                    current_copy_region.imageExtent.depth = 1;
                }

                if (current_copy_region.imageExtent.height < 1)
                {
                    current_copy_region.imageExtent.height = 1;
                }

                if (current_copy_region.imageExtent.width < 1)
                {
                    current_copy_region.imageExtent.width = 1;
                }

                copy_regions.push_back(current_copy_region);
            }

            temp_cmdbuf_ptr->record_copy_buffer_to_image(temp_buffer_ptr,
                                                         this,
                                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                         static_cast<uint32_t>(copy_regions.size() ),
                                                        &copy_regions[0]);
        }
        temp_cmdbuf_ptr->stop_recording();

        /* Execute the command buffer */
        universal_queue_ptr->submit_command_buffer(temp_cmdbuf_ptr,
                                                   true /* should_block */);

        *out_new_image_layout_ptr = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        /* Clean up */
        if (temp_buffer_ptr != nullptr)
        {
            temp_buffer_ptr->release();

            temp_buffer_ptr = nullptr;
        }

        if (temp_cmdbuf_ptr != nullptr)
        {
            temp_cmdbuf_ptr->release();

            temp_cmdbuf_ptr = nullptr;
        }
    }
}