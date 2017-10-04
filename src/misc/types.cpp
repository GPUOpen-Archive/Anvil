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
#include "wrappers/physical_device.h"
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

    /* NOTE: For an image barrier to work correctly, the underlying subresource range must be assigned memory.
     *       Query for a memory block in order to force any listening memory allocators to bake */
    auto memory_block_ptr = buffer_ptr->get_memory_block(0 /* in_n_memory_block */);

    ANVIL_REDUNDANT_VARIABLE(memory_block_ptr);
}

/** Please see header for specification */
Anvil::BufferBarrier::~BufferBarrier()
{
    /* Stub */
}

/** Please see header for specification */
Anvil::DeviceExtensionConfiguration::DeviceExtensionConfiguration()
{
    amd_draw_indirect_count              = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_gcn_shader                       = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_gpu_shader_half_float            = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_gpu_shader_int16                 = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_rasterization_order              = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_shader_ballot                    = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_shader_explicit_vertex_parameter = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_shader_trinary_minmax            = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_texture_gather_bias_lod          = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    ext_shader_subgroup_ballot           = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    ext_shader_subgroup_vote             = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    khr_16bit_storage                    = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    khr_maintenance1                     = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    khr_storage_buffer_storage_class     = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    khr_surface                          = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    khr_swapchain                        = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;

    /* VK_AMD_negative_viewport_height interacts with KHR_maintenance1, hence it needs
     * to be enabled manually.
     */
    amd_negative_viewport_height = EXTENSION_AVAILABILITY_IGNORE;

    /* VK_EXT_debug_marker is only useful for debugging. */
    #if defined(_DEBUG)
    {
        ext_debug_marker = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    }
    #else
    {
        ext_debug_marker = EXTENSION_AVAILABILITY_IGNORE;
    }
    #endif

}

/** Please see header for specification */
bool Anvil::DeviceExtensionConfiguration::is_supported_by_physical_device(std::weak_ptr<const Anvil::PhysicalDevice> in_physical_device_ptr,
                                                                          std::vector<std::string>*                  out_opt_unsupported_extensions_ptr) const
{
    typedef struct ExtensionItem
    {
        const char* extension_name;
        bool        is_required;

        ExtensionItem(const char* in_extension_name,
                      const bool& in_is_required)
        {
            extension_name = in_extension_name;
            is_required    = in_is_required;
        }
    } ExtensionItem;

    std::shared_ptr<const Anvil::PhysicalDevice> physical_device_locked_ptr(in_physical_device_ptr);
    bool                                         result                    (true);
    std::vector<ExtensionItem>                   extensions =
    {
        ExtensionItem(VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME,              amd_draw_indirect_count              == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_GCN_SHADER_EXTENSION_NAME,                       amd_gcn_shader                       == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_GPU_SHADER_HALF_FLOAT_EXTENSION_NAME,            amd_gpu_shader_half_float            == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_GPU_SHADER_INT16_EXTENSION_NAME,                 amd_gpu_shader_int16                 == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME,         amd_negative_viewport_height         == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME,              amd_rasterization_order              == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_SHADER_BALLOT_EXTENSION_NAME,                    amd_shader_ballot                    == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER_EXTENSION_NAME, amd_shader_explicit_vertex_parameter == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_SHADER_TRINARY_MINMAX_EXTENSION_NAME,            amd_shader_trinary_minmax            == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_TEXTURE_GATHER_BIAS_LOD_EXTENSION_NAME,          amd_texture_gather_bias_lod          == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_EXT_DEBUG_MARKER_EXTENSION_NAME,                     ext_debug_marker                     == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME,           ext_shader_subgroup_vote             == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME,             ext_shader_subgroup_vote             == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_KHR_16BIT_STORAGE_EXTENSION_NAME,                    khr_16bit_storage                    == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_KHR_MAINTENANCE1_EXTENSION_NAME,                     khr_maintenance1                     == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem("VK_KHR_storage_buffer_storage_class",                  khr_storage_buffer_storage_class     == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_KHR_SURFACE_EXTENSION_NAME,                          khr_surface                          == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_KHR_SWAPCHAIN_EXTENSION_NAME,                        khr_swapchain                        == Anvil::EXTENSION_AVAILABILITY_REQUIRE)
    };

    if (out_opt_unsupported_extensions_ptr != nullptr)
    {
        out_opt_unsupported_extensions_ptr->clear();
    }

    for (const auto& extension : other_extensions)
    {
        if (extension.second == Anvil::EXTENSION_AVAILABILITY_REQUIRE)
        {
            extensions.push_back(
                ExtensionItem(extension.first.c_str(),
                              true)
            );
        }
    }

    for (const auto& current_extension : extensions)
    {
        if (!physical_device_locked_ptr->is_device_extension_supported(current_extension.extension_name) &&
             current_extension.is_required)
        {
            result = false;

            if (out_opt_unsupported_extensions_ptr == nullptr)
            {
                break;
            }
            else
            {
                out_opt_unsupported_extensions_ptr->push_back(current_extension.extension_name);
            }
        }
    }

    return result;
}

bool Anvil::DeviceExtensionConfiguration::operator==(const Anvil::DeviceExtensionConfiguration& in_config) const
{
    bool result;

    result = (amd_draw_indirect_count              == in_config.amd_draw_indirect_count)              &&
             (amd_gcn_shader                       == in_config.amd_gcn_shader)                       &&
             (amd_gpu_shader_half_float            == in_config.amd_gpu_shader_half_float)            &&
             (amd_gpu_shader_int16                 == in_config.amd_gpu_shader_int16)                 &&
             (amd_negative_viewport_height         == in_config.amd_negative_viewport_height)         &&
             (amd_rasterization_order              == in_config.amd_rasterization_order)              &&
             (amd_shader_ballot                    == in_config.amd_shader_ballot)                    &&
             (amd_shader_explicit_vertex_parameter == in_config.amd_shader_explicit_vertex_parameter) &&
             (amd_shader_trinary_minmax            == in_config.amd_shader_trinary_minmax)            &&
             (amd_texture_gather_bias_lod          == in_config.amd_texture_gather_bias_lod)          &&
             (ext_debug_marker                     == in_config.ext_debug_marker)                     &&
             (ext_shader_subgroup_ballot           == in_config.ext_shader_subgroup_ballot)           &&
             (ext_shader_subgroup_vote             == in_config.ext_shader_subgroup_vote)             &&
             (khr_16bit_storage                    == in_config.khr_16bit_storage)                    &&
             (khr_storage_buffer_storage_class     == in_config.khr_storage_buffer_storage_class)     &&
             (khr_surface                          == in_config.khr_surface)                          &&
             (khr_swapchain                        == in_config.khr_swapchain);

    if (result)
    {
        for (const auto& current_other_extension : other_extensions)
        {
            auto iterator = std::find(in_config.other_extensions.begin(),
                                      in_config.other_extensions.end(),
                                      current_other_extension);

            if (iterator == in_config.other_extensions.end() )
            {
                result = false;

                break;
            }
        }
    }

    return result;
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

    /* NOTE: For an image barrier to work correctly, the underlying subresource range must be assigned memory.
     *       Query for a memory block in order to force any listening memory allocators to bake */
    auto memory_block_ptr = image_ptr->get_memory_block();

    ANVIL_REDUNDANT_VARIABLE(memory_block_ptr);
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
void Anvil::MemoryProperties::init(const VkPhysicalDeviceMemoryProperties& in_mem_properties)
{
    n_heaps = in_mem_properties.memoryHeapCount;

    heaps   = new Anvil::MemoryHeap[n_heaps];
    anvil_assert(heaps != nullptr);

    for (unsigned int n_heap = 0;
                      n_heap < in_mem_properties.memoryHeapCount;
                    ++n_heap)
    {
        heaps[n_heap].flags = static_cast<VkMemoryHeapFlagBits>(in_mem_properties.memoryHeaps[n_heap].flags);
        heaps[n_heap].size  = in_mem_properties.memoryHeaps[n_heap].size;
    }

    types.reserve(in_mem_properties.memoryTypeCount);

    for (unsigned int n_type = 0;
                      n_type < in_mem_properties.memoryTypeCount;
                    ++n_type)
    {
        types.push_back(MemoryType(in_mem_properties.memoryTypes[n_type],
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
Anvil::MemoryType::MemoryType(const VkMemoryType&      in_type,
                              struct MemoryProperties* in_memory_props_ptr)
{
    flags    = static_cast<VkMemoryPropertyFlagBits>(in_type.propertyFlags);
    heap_ptr = &in_memory_props_ptr->heaps[in_type.heapIndex];

    features = Anvil::Utils::get_memory_feature_flags_from_vk_property_flags(flags,
                                                                             heap_ptr->flags);
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D(VkImageAspectFlagBits in_aspect,
                                                     uint32_t              in_n_mipmap,
                                                     uint32_t              in_row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect    = in_aspect;
    result.data_size = in_row_size;
    result.row_size  = in_row_size;
    result.n_layers  = 1;
    result.n_slices  = 1;
    result.n_mipmap  = in_n_mipmap;

    return result;
}

/** Returns a filled MipmapRawData structure for a 1D Array mip.
 *
 *  NOTE: It is caller's responsibility to configure one of the data storage pointer members.
 */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array(VkImageAspectFlagBits in_aspect,
                                                           uint32_t              in_n_layer,
                                                           uint32_t              in_n_layers,
                                                           uint32_t              in_n_mipmap,
                                                           uint32_t              in_row_size,
                                                           uint32_t              in_data_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect    = in_aspect;
    result.data_size = in_data_size;
    result.n_layer   = in_n_layer;
    result.n_layers  = in_n_layers;
    result.n_mipmap  = in_n_mipmap;
    result.n_slices  = 1;
    result.row_size  = in_row_size;

    return result;
}

/** Returns a filled MipmapRawData structure for a 2D mip.
 *
 *  NOTE: It is caller's responsibility to configure one of the data storage pointer members.
 */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D(VkImageAspectFlagBits in_aspect,
                                                     uint32_t              in_n_mipmap,
                                                     uint32_t              in_data_size,
                                                     uint32_t              in_row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect    = in_aspect;
    result.data_size = in_data_size;
    result.n_layers  = 1;
    result.n_mipmap  = in_n_mipmap;
    result.n_slices  = 1;
    result.row_size  = in_row_size;

    return result;
}

/** Returns a filled MipmapRawData structure for a 2D array mip.
 *
 *  NOTE: It is caller's responsibility to configure one of the data storage pointer members.
 */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array(VkImageAspectFlagBits in_aspect,
                                                           uint32_t              in_n_layer,
                                                           uint32_t              in_n_layers,
                                                           uint32_t              in_n_mipmap,
                                                           uint32_t              in_data_size,
                                                           uint32_t              in_row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect    = in_aspect;
    result.data_size = in_data_size;
    result.n_layer   = in_n_layer;
    result.n_layers  = in_n_layers;
    result.n_mipmap  = in_n_mipmap;
    result.n_slices  = 1;
    result.row_size  = in_row_size;

    return result;
}

/** Returns a filled MipmapRawData structure for a 3D mip.
 *
 *  NOTE: It is caller's responsibility to configure one of the data storage pointer members.
 */
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D(VkImageAspectFlagBits in_aspect,
                                                     uint32_t              in_n_layer,
                                                     uint32_t              in_n_slices,
                                                     uint32_t              in_n_mipmap,
                                                     uint32_t              in_data_size,
                                                     uint32_t              in_row_size)
{
    MipmapRawData result;

    memset(&result,
            0,
            sizeof(result) );

    result.aspect    = in_aspect;
    result.data_size = in_data_size;
    result.n_layers  = 1;
    result.n_layer   = in_n_layer;
    result.n_slices  = in_n_slices;
    result.n_mipmap  = in_n_mipmap;
    result.row_size  = in_row_size;

    return result;
}


/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_from_uchar_ptr(VkImageAspectFlagBits          in_aspect,
                                                                    uint32_t                       in_n_mipmap,
                                                                    std::shared_ptr<unsigned char> in_linear_tightly_packed_data_ptr,
                                                                    uint32_t                       in_row_size)
{
    MipmapRawData result = create_1D(in_aspect,
                                     in_n_mipmap,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_from_uchar_ptr(VkImageAspectFlagBits in_aspect,
                                                                    uint32_t              in_n_mipmap,
                                                                    const unsigned char*  in_linear_tightly_packed_data_ptr,
                                                                    uint32_t              in_row_size)
{
    MipmapRawData result = create_1D(in_aspect,
                                     in_n_mipmap,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                           uint32_t                                     in_n_mipmap,
                                                                           std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                           uint32_t                                     in_row_size)
{
    MipmapRawData result = create_1D(in_aspect,
                                     in_n_mipmap,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array_from_uchar_ptr(VkImageAspectFlagBits          in_aspect,
                                                                          uint32_t                       in_n_layer,
                                                                          uint32_t                       in_n_layers,
                                                                          uint32_t                       in_n_mipmap,
                                                                          std::shared_ptr<unsigned char> in_linear_tightly_packed_data_ptr,
                                                                          uint32_t                       in_row_size,
                                                                          uint32_t                       in_data_size)
{
    MipmapRawData result = create_1D_array(in_aspect,
                                           in_n_layer,
                                           in_n_layers,
                                           in_n_mipmap,
                                           in_row_size,
                                           in_data_size);

    result.linear_tightly_packed_data_uchar_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array_from_uchar_ptr(VkImageAspectFlagBits in_aspect,
                                                                          uint32_t              in_n_layer,
                                                                          uint32_t              in_n_layers,
                                                                          uint32_t              in_n_mipmap,
                                                                          const unsigned char*  in_linear_tightly_packed_data_ptr,
                                                                          uint32_t              in_row_size,
                                                                          uint32_t              in_data_size)
{
    MipmapRawData result = create_1D_array(in_aspect,
                                           in_n_layer,
                                           in_n_layers,
                                           in_n_mipmap,
                                           in_row_size,
                                           in_data_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                                 uint32_t                                     in_n_layer,
                                                                                 uint32_t                                     in_n_layers,
                                                                                 uint32_t                                     in_n_mipmap,
                                                                                 std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                                 uint32_t                                     in_row_size,
                                                                                 uint32_t                                     in_data_size)
{
    MipmapRawData result = create_1D_array(in_aspect,
                                           in_n_layer,
                                           in_n_layers,
                                           in_n_mipmap,
                                           in_row_size,
                                           in_data_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_from_uchar_ptr(VkImageAspectFlagBits          in_aspect,
                                                                    uint32_t                       in_n_mipmap,
                                                                    std::shared_ptr<unsigned char> in_linear_tightly_packed_data_ptr,
                                                                    uint32_t                       in_data_size,
                                                                    uint32_t                       in_row_size)
{
    MipmapRawData result = create_2D(in_aspect,
                                     in_n_mipmap,
                                     in_data_size,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_from_uchar_ptr(VkImageAspectFlagBits in_aspect,
                                                                    uint32_t              in_n_mipmap,
                                                                    const unsigned char*  in_linear_tightly_packed_data_ptr,
                                                                    uint32_t              in_data_size,
                                                                    uint32_t              in_row_size)
{
    MipmapRawData result = create_2D(in_aspect,
                                     in_n_mipmap,
                                     in_data_size,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                           uint32_t                                     in_n_mipmap,
                                                                           std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                           uint32_t                                     in_data_size,
                                                                           uint32_t                                     in_row_size)
{
    MipmapRawData result = create_2D(in_aspect,
                                     in_n_mipmap,
                                     in_data_size,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array_from_uchar_ptr(VkImageAspectFlagBits          in_aspect,
                                                                          uint32_t                       in_n_layer,
                                                                          uint32_t                       in_n_layers,
                                                                          uint32_t                       in_n_mipmap,
                                                                          std::shared_ptr<unsigned char> in_linear_tightly_packed_data_ptr,
                                                                          uint32_t                       in_data_size,
                                                                          uint32_t                       in_row_size)
{
    MipmapRawData result = create_2D_array(in_aspect,
                                           in_n_layer,
                                           in_n_layers,
                                           in_n_mipmap,
                                           in_data_size,
                                           in_row_size);

    result.linear_tightly_packed_data_uchar_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array_from_uchar_ptr(VkImageAspectFlagBits in_aspect,
                                                                          uint32_t              in_n_layer,
                                                                          uint32_t              in_n_layers,
                                                                          uint32_t              in_n_mipmap,
                                                                          const unsigned char*  in_linear_tightly_packed_data_ptr,
                                                                          uint32_t              in_data_size,
                                                                          uint32_t              in_row_size)
{
    MipmapRawData result = create_2D_array(in_aspect,
                                           in_n_layer,
                                           in_n_layers,
                                           in_n_mipmap,
                                           in_data_size,
                                           in_row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                                 uint32_t                                     in_n_layer,
                                                                                 uint32_t                                     in_n_layers,
                                                                                 uint32_t                                     in_n_mipmap,
                                                                                 std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                                 uint32_t                                     in_data_size,
                                                                                 uint32_t                                     in_row_size)
{
    MipmapRawData result = create_2D_array(in_aspect,
                                           in_n_layer,
                                           in_n_layers,
                                           in_n_mipmap,
                                           in_data_size,
                                           in_row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}



/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D_from_uchar_ptr(VkImageAspectFlagBits          in_aspect,
                                                                    uint32_t                       in_n_layer,
                                                                    uint32_t                       in_n_layer_slices,
                                                                    uint32_t                       in_n_mipmap,
                                                                    std::shared_ptr<unsigned char> in_linear_tightly_packed_data_ptr,
                                                                    uint32_t                       in_slice_data_size,
                                                                    uint32_t                       in_row_size)
{
    MipmapRawData result = create_3D(in_aspect,
                                     in_n_layer,
                                     in_n_layer_slices,
                                     in_n_mipmap,
                                     in_slice_data_size,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D_from_uchar_ptr(VkImageAspectFlagBits in_aspect,
                                                                    uint32_t              in_n_layer,
                                                                    uint32_t              in_n_layer_slices,
                                                                    uint32_t              in_n_mipmap,
                                                                    const unsigned char*  in_linear_tightly_packed_data_ptr,
                                                                    uint32_t              in_slice_data_size,
                                                                    uint32_t              in_row_size)
{
    MipmapRawData result = create_3D(in_aspect,
                                     in_n_layer,
                                     in_n_layer_slices,
                                     in_n_mipmap,
                                     in_slice_data_size,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                           uint32_t                                     in_n_layer,
                                                                           uint32_t                                     in_n_layer_slices,
                                                                           uint32_t                                     in_n_mipmap,
                                                                           std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                           uint32_t                                     in_slice_data_size,
                                                                           uint32_t                                     in_row_size)
{
    MipmapRawData result = create_3D(in_aspect,
                                     in_n_layer,
                                     in_n_layer_slices,
                                     in_n_mipmap,
                                     in_slice_data_size,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}


/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_from_uchar_ptr(VkImageAspectFlagBits          in_aspect,
                                                                          uint32_t                       in_n_layer,
                                                                          uint32_t                       in_n_mipmap,
                                                                          std::shared_ptr<unsigned char> in_linear_tightly_packed_data_ptr,
                                                                          uint32_t                       in_data_size,
                                                                          uint32_t                       in_row_size)
{
    anvil_assert(in_n_layer < 6);

    MipmapRawData result = create_2D_array(in_aspect,
                                           in_n_layer,
                                           1, /* n_layer_slices */
                                           in_n_mipmap,
                                           in_data_size,
                                           in_row_size);

    result.linear_tightly_packed_data_uchar_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_from_uchar_ptr(VkImageAspectFlagBits in_aspect,
                                                                          uint32_t              in_n_layer,
                                                                          uint32_t              in_n_mipmap,
                                                                          const unsigned char*  in_linear_tightly_packed_data_ptr,
                                                                          uint32_t              in_data_size,
                                                                          uint32_t              in_row_size)
{
    anvil_assert(in_n_layer < 6);

    MipmapRawData result = create_2D_array(in_aspect,
                                           in_n_layer,
                                           1, /* n_layer_slices */
                                           in_n_mipmap,
                                           in_data_size,
                                           in_row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                                 uint32_t                                     in_n_layer,
                                                                                 uint32_t                                     in_n_mipmap,
                                                                                 std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                                 uint32_t                                     in_data_size,
                                                                                 uint32_t                                     in_row_size)
{
    anvil_assert(in_n_layer < 6);

    MipmapRawData result = create_2D_array(in_aspect,
                                           in_n_layer,
                                           1, /* n_layer_slices */
                                           in_n_mipmap,
                                           in_data_size,
                                           in_row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}


/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_array_from_uchar_ptr(VkImageAspectFlagBits          in_aspect,
                                                                                uint32_t                       in_n_layer,
                                                                                uint32_t                       in_n_layers,
                                                                                uint32_t                       in_n_mipmap,
                                                                                std::shared_ptr<unsigned char> in_linear_tightly_packed_data_ptr,
                                                                                uint32_t                       in_data_size,
                                                                                uint32_t                       in_row_size)
{
    MipmapRawData result = create_2D_array(in_aspect,
                                           in_n_layer,
                                           in_n_layers,
                                           in_n_mipmap,
                                           in_data_size,
                                           in_row_size);

    result.linear_tightly_packed_data_uchar_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_array_from_uchar_ptr(VkImageAspectFlagBits in_aspect,
                                                                                uint32_t              in_n_layer,
                                                                                uint32_t              in_n_layers,
                                                                                uint32_t              in_n_mipmap,
                                                                                const unsigned char*  in_linear_tightly_packed_data_ptr,
                                                                                uint32_t              in_data_size,
                                                                                uint32_t              in_row_size)
{
    MipmapRawData result = create_2D_array(in_aspect,
                                           in_n_layer,
                                           in_n_layers,
                                           in_n_mipmap,
                                           in_data_size,
                                           in_row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_array_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                                       uint32_t                                     in_n_layer,
                                                                                       uint32_t                                     in_n_layers,
                                                                                       uint32_t                                     in_n_mipmap,
                                                                                       std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                                       uint32_t                                     in_data_size,
                                                                                       uint32_t                                     in_row_size)
{
    MipmapRawData result = create_2D_array(in_aspect,
                                           in_n_layer,
                                           in_n_layers,
                                           in_n_mipmap,
                                           in_data_size,
                                           in_row_size);

    result.linear_tightly_packed_data_uchar_vec_ptr = in_linear_tightly_packed_data_ptr;

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
    stage = SHADER_STAGE_UNKNOWN;
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const std::string&            in_name,
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
Anvil::SparseMemoryBindInfoID Anvil::Utils::SparseMemoryBindingUpdateInfo::add_bind_info(uint32_t                            in_n_signal_semaphores,
                                                                                         std::shared_ptr<Anvil::Semaphore>*  in_opt_signal_semaphores_ptr,
                                                                                         uint32_t                            in_n_wait_semaphores,
                                                                                         std::shared_ptr<Anvil::Semaphore>*  in_opt_wait_semaphores_ptr)
{
    Anvil::SparseMemoryBindInfoID result_id = static_cast<Anvil::SparseMemoryBindInfoID>(m_bindings.size() );
    BindingInfo                   new_binding;

    for (uint32_t n_signal_sem = 0;
                  n_signal_sem < in_n_signal_semaphores;
                ++n_signal_sem)
    {
        new_binding.signal_semaphores.push_back(in_opt_signal_semaphores_ptr[n_signal_sem]);
    }

    for (uint32_t n_wait_sem = 0;
                  n_wait_sem < in_n_wait_semaphores;
                ++n_wait_sem)
    {
        new_binding.wait_semaphores.push_back(in_opt_wait_semaphores_ptr[n_wait_sem]);
    }

    m_bindings.push_back(new_binding);

    return result_id;
}

/** Please see header for specification */
void Anvil::Utils::SparseMemoryBindingUpdateInfo::append_buffer_memory_update(SparseMemoryBindInfoID              in_bind_info_id,
                                                                              std::shared_ptr<Anvil::Buffer>      in_buffer_ptr,
                                                                              VkDeviceSize                        in_buffer_memory_start_offset,
                                                                              std::shared_ptr<Anvil::MemoryBlock> in_memory_block_ptr,
                                                                              VkDeviceSize                        in_memory_block_start_offset,
                                                                              VkDeviceSize                        in_size)
{
    /* Sanity checks */
    anvil_assert(in_buffer_ptr                                 != nullptr);
    anvil_assert(m_bindings.size()                             >  in_bind_info_id);
    anvil_assert(in_buffer_ptr->get_memory_requirements().size >= in_buffer_memory_start_offset + in_size);

    if (in_memory_block_ptr != nullptr)
    {
        anvil_assert(in_memory_block_ptr->get_size() >= in_memory_block_start_offset + in_size);
    }

    /* Cache the update */
    auto&              binding = m_bindings.at(in_bind_info_id);
    GeneralBindInfo    update;
    VkSparseMemoryBind update_vk;

    update.memory_block_ptr          = in_memory_block_ptr;
    update.memory_block_start_offset = in_memory_block_start_offset;
    update.size                      = in_size;
    update.start_offset              = in_buffer_memory_start_offset;

    update_vk.flags                  = 0;
    update_vk.memory                 = (in_memory_block_ptr != nullptr) ? in_memory_block_ptr->get_memory()
                                                                        : VK_NULL_HANDLE;
    update_vk.memoryOffset           = (in_memory_block_ptr != nullptr) ? (in_memory_block_ptr->get_start_offset() + in_memory_block_start_offset)
                                                                        : UINT32_MAX;
    update_vk.resourceOffset         = (in_buffer_ptr->get_start_offset() + in_buffer_memory_start_offset);
    update_vk.size                   = in_size;

    binding.buffer_updates[in_buffer_ptr].first.push_back (update);
    binding.buffer_updates[in_buffer_ptr].second.push_back(update_vk);
}

/** Please see header for specification */
void Anvil::Utils::SparseMemoryBindingUpdateInfo::append_image_memory_update(SparseMemoryBindInfoID              in_bind_info_id,
                                                                             std::shared_ptr<Anvil::Image>       in_image_ptr,
                                                                             const VkImageSubresource&           in_subresource,
                                                                             const VkOffset3D&                   in_offset,
                                                                             const VkExtent3D&                   in_extent,
                                                                             VkSparseMemoryBindFlags             in_flags,
                                                                             std::shared_ptr<Anvil::MemoryBlock> in_opt_memory_block_ptr,
                                                                             VkDeviceSize                        in_opt_memory_block_start_offset)
{
    /* Sanity checks .. */
    anvil_assert(in_image_ptr      != nullptr);
    anvil_assert(in_flags          == 0);
    anvil_assert(m_bindings.size() > in_bind_info_id);

    anvil_assert(in_image_ptr->get_image_n_layers()                > in_subresource.arrayLayer);
    anvil_assert(in_image_ptr->get_image_n_mipmaps()               > in_subresource.mipLevel);
    anvil_assert(in_image_ptr->has_aspects(in_subresource.aspectMask) );

    if (in_opt_memory_block_ptr != nullptr)
    {
        anvil_assert(in_opt_memory_block_ptr->get_size() > in_opt_memory_block_start_offset);
    }

    /* Cache the update */
    auto&                   binding = m_bindings.at(in_bind_info_id);
    ImageBindInfo           update;
    VkSparseImageMemoryBind update_vk;

    update.extent                    = in_extent;
    update.flags                     = in_flags;
    update.memory_block_ptr          = in_opt_memory_block_ptr;
    update.memory_block_start_offset = in_opt_memory_block_start_offset;
    update.offset                    = in_offset;
    update.subresource               = in_subresource;

    update_vk.extent       = in_extent;
    update_vk.flags        = in_flags;
    update_vk.memory       = (in_opt_memory_block_ptr != nullptr) ? in_opt_memory_block_ptr->get_memory()
                                                                  : VK_NULL_HANDLE;
    update_vk.memoryOffset = (in_opt_memory_block_ptr != nullptr) ? in_opt_memory_block_ptr->get_start_offset() + in_opt_memory_block_start_offset
                                                                  : UINT32_MAX;
    update_vk.offset       = in_offset;
    update_vk.subresource  = in_subresource;

    binding.image_updates[in_image_ptr].first.push_back (update);
    binding.image_updates[in_image_ptr].second.push_back(update_vk);
}

/** Please see header for specification */
void Anvil::Utils::SparseMemoryBindingUpdateInfo::append_opaque_image_memory_update(SparseMemoryBindInfoID              in_bind_info_id,
                                                                                    std::shared_ptr<Anvil::Image>       in_image_ptr,
                                                                                    VkDeviceSize                        in_resource_offset,
                                                                                    VkDeviceSize                        in_size,
                                                                                    VkSparseMemoryBindFlags             in_flags,
                                                                                    std::shared_ptr<Anvil::MemoryBlock> in_opt_memory_block_ptr,
                                                                                    VkDeviceSize                        in_opt_memory_block_start_offset)
{
    /* Sanity checks */
    anvil_assert(in_image_ptr                                 != nullptr);
    anvil_assert(m_bindings.size()                            >  in_bind_info_id);
    anvil_assert(in_image_ptr->get_memory_requirements().size >= in_resource_offset + in_size);

    if (in_opt_memory_block_ptr != nullptr)
    {
        anvil_assert(in_opt_memory_block_ptr->get_size() >= in_opt_memory_block_start_offset + in_size);
    }

    /* Cache the update */
    auto&              binding = m_bindings.at(in_bind_info_id);
    GeneralBindInfo    update;
    VkSparseMemoryBind update_vk;

    update.flags                     = in_flags;
    update.memory_block_ptr          = in_opt_memory_block_ptr;
    update.memory_block_start_offset = in_opt_memory_block_start_offset;
    update.size                      = in_size;
    update.start_offset              = in_resource_offset;

    update_vk.flags                  = in_flags;
    update_vk.memory                 = (in_opt_memory_block_ptr != nullptr) ? in_opt_memory_block_ptr->get_memory()
                                                                            : VK_NULL_HANDLE;
    update_vk.memoryOffset           = (in_opt_memory_block_ptr != nullptr) ? (in_opt_memory_block_ptr->get_start_offset() + in_opt_memory_block_start_offset)
                                                                            : UINT32_MAX;
    update_vk.resourceOffset         = in_resource_offset;
    update_vk.size                   = in_size;

    binding.image_opaque_updates[in_image_ptr].first.push_back (update);
    binding.image_opaque_updates[in_image_ptr].second.push_back(update_vk);
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
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_bind_info_properties(SparseMemoryBindInfoID                    in_bind_info_id,
                                                                           uint32_t* const                           out_opt_n_buffer_memory_updates_ptr,
                                                                           uint32_t* const                           out_opt_n_image_memory_updates_ptr,
                                                                           uint32_t* const                           out_opt_n_image_opaque_memory_updates_ptr,
                                                                           uint32_t* const                           out_opt_n_signal_semaphores_ptr,
                                                                           const std::shared_ptr<Anvil::Semaphore>** out_opt_signal_semaphores_ptr_ptr,
                                                                           uint32_t* const                           out_opt_n_wait_semaphores_ptr,
                                                                           const std::shared_ptr<Anvil::Semaphore>** out_opt_wait_semaphores_ptr_ptr) const
{
    decltype(m_bindings)::const_iterator binding_iterator;
          bool                           result = false;

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(m_bindings.size() > in_bind_info_id);

        goto end;
    }

    binding_iterator = m_bindings.begin() + static_cast<int>(in_bind_info_id);

    if (out_opt_n_buffer_memory_updates_ptr != nullptr)
    {
        uint32_t n_buffer_mem_updates = 0;

        for (const auto& buffer_update_iterator : binding_iterator->buffer_updates)
        {
            n_buffer_mem_updates += static_cast<uint32_t>(buffer_update_iterator.second.first.size() );
        }

        *out_opt_n_buffer_memory_updates_ptr = n_buffer_mem_updates;
    }

    if (out_opt_n_image_memory_updates_ptr != nullptr)
    {
        uint32_t n_image_mem_updates = 0;

        for (const auto& image_update_iterator : binding_iterator->image_updates)
        {
            n_image_mem_updates += static_cast<uint32_t>(image_update_iterator.second.first.size() );
        }

        *out_opt_n_image_memory_updates_ptr = n_image_mem_updates;
    }

    if (out_opt_n_image_opaque_memory_updates_ptr != nullptr)
    {
        uint32_t n_image_opaque_mem_updates = 0;

        for (const auto& image_opaque_update_iterator : binding_iterator->image_opaque_updates)
        {
            n_image_opaque_mem_updates += static_cast<uint32_t>(image_opaque_update_iterator.second.first.size() );
        }

        *out_opt_n_image_opaque_memory_updates_ptr = n_image_opaque_mem_updates;
    }

    if (out_opt_n_signal_semaphores_ptr != nullptr)
    {
        *out_opt_n_signal_semaphores_ptr = static_cast<uint32_t>(binding_iterator->signal_semaphores.size() );
    }

    if (out_opt_signal_semaphores_ptr_ptr != nullptr)
    {
        *out_opt_signal_semaphores_ptr_ptr = &binding_iterator->signal_semaphores[0];
    }

    if (out_opt_n_wait_semaphores_ptr != nullptr)
    {
        *out_opt_n_wait_semaphores_ptr = static_cast<uint32_t>(binding_iterator->wait_semaphores.size() );
    }

    if (out_opt_wait_semaphores_ptr_ptr != nullptr)
    {
        *out_opt_wait_semaphores_ptr_ptr = &binding_iterator->wait_semaphores[0];
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
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_buffer_memory_update_properties(SparseMemoryBindInfoID               in_bind_info_id,
                                                                                      uint32_t                             in_n_update,
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

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= in_bind_info_id) );

        goto end;
    }

    binding_iterator            = m_bindings.cbegin() + static_cast<int>(in_bind_info_id);
    buffer_binding_map_iterator = binding_iterator->buffer_updates.begin();

    while (buffer_binding_map_iterator != binding_iterator->buffer_updates.end() )
    {
        const uint32_t n_buffer_bindings = static_cast<uint32_t>(buffer_binding_map_iterator->second.first.size() );

        if (n_current_update + n_buffer_bindings > in_n_update)
        {
            buffer_bind = buffer_binding_map_iterator->second.first.at(in_n_update - n_current_update);

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
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_image_memory_update_properties(SparseMemoryBindInfoID               in_bind_info_id,
                                                                                     uint32_t                             in_n_update,
                                                                                     std::shared_ptr<Anvil::Image>*       out_opt_image_ptr_ptr,
                                                                                     VkImageSubresource*                  out_opt_subresource_ptr,
                                                                                     VkOffset3D*                          out_opt_offset_ptr,
                                                                                     VkExtent3D*                          out_opt_extent_ptr,
                                                                                     VkSparseMemoryBindFlags*             out_opt_flags_ptr,
                                                                                     std::shared_ptr<Anvil::MemoryBlock>* out_opt_memory_block_ptr_ptr,
                                                                                     VkDeviceSize*                        out_opt_memory_block_start_offset_ptr) const
{
    decltype(m_bindings)::const_iterator binding_iterator;
    ImageBindInfo                        image_bind;
    ImageBindUpdateMap::const_iterator   image_binding_map_iterator;
    uint32_t                             n_current_update           = 0;
    bool                                 result                     = false;

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= in_bind_info_id) );

        goto end;
    }

    binding_iterator           = m_bindings.cbegin() + static_cast<int>(in_bind_info_id);
    image_binding_map_iterator = binding_iterator->image_updates.begin();

    while (image_binding_map_iterator != binding_iterator->image_updates.end() )
    {
        const uint32_t n_image_bindings = static_cast<uint32_t>(image_binding_map_iterator->second.first.size() );

        if (n_current_update + n_image_bindings > in_n_update)
        {
            image_bind = image_binding_map_iterator->second.first.at(in_n_update - n_current_update);

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

    if (out_opt_image_ptr_ptr != nullptr)
    {
        *out_opt_image_ptr_ptr = image_binding_map_iterator->first;
    }

    if (out_opt_subresource_ptr != nullptr)
    {
        *out_opt_subresource_ptr = image_bind.subresource;
    }

    if (out_opt_offset_ptr != nullptr)
    {
        *out_opt_offset_ptr = image_bind.offset;
    }

    if (out_opt_extent_ptr != nullptr)
    {
        *out_opt_extent_ptr = image_bind.extent;
    }

    if (out_opt_flags_ptr != nullptr)
    {
        *out_opt_flags_ptr = image_bind.flags;
    }

    if (out_opt_memory_block_ptr_ptr != nullptr)
    {
        *out_opt_memory_block_ptr_ptr = image_bind.memory_block_ptr;
    }

    if (out_opt_memory_block_start_offset_ptr != nullptr)
    {
        *out_opt_memory_block_start_offset_ptr = image_bind.memory_block_start_offset;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_image_opaque_memory_update_properties(SparseMemoryBindInfoID               in_bind_info_id,
                                                                                            uint32_t                             in_n_update,
                                                                                            std::shared_ptr<Anvil::Image>*       out_opt_image_ptr_ptr,
                                                                                            VkDeviceSize*                        out_opt_resource_offset_ptr,
                                                                                            VkDeviceSize*                        out_opt_size_ptr,
                                                                                            VkSparseMemoryBindFlags*             out_opt_flags_ptr,
                                                                                            std::shared_ptr<Anvil::MemoryBlock>* out_opt_memory_block_ptr_ptr,
                                                                                            VkDeviceSize*                        out_opt_memory_block_start_offset_ptr) const
{
    decltype(m_bindings)::const_iterator     binding_iterator;
    GeneralBindInfo                          image_opaque_bind;
    ImageOpaqueBindUpdateMap::const_iterator image_opaque_binding_map_iterator;
    uint32_t                                 n_current_update           = 0;
    bool                                     result                     = false;

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= in_bind_info_id) );

        goto end;
    }

    binding_iterator                  = m_bindings.cbegin() + static_cast<int>(in_bind_info_id);
    image_opaque_binding_map_iterator = binding_iterator->image_opaque_updates.begin();

    while (image_opaque_binding_map_iterator != binding_iterator->image_opaque_updates.end() )
    {
        const uint32_t n_image_opaque_bindings = static_cast<uint32_t>(image_opaque_binding_map_iterator->second.first.size() );

        if (n_current_update + n_image_opaque_bindings > in_n_update)
        {
            image_opaque_bind = image_opaque_binding_map_iterator->second.first.at(in_n_update - n_current_update);

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

    if (out_opt_image_ptr_ptr != nullptr)
    {
        *out_opt_image_ptr_ptr = image_opaque_binding_map_iterator->first;
    }

    if (out_opt_resource_offset_ptr != nullptr)
    {
        *out_opt_resource_offset_ptr = image_opaque_bind.start_offset;
    }

    if (out_opt_size_ptr != nullptr)
    {
        *out_opt_size_ptr = image_opaque_bind.size;
    }

    if (out_opt_flags_ptr != nullptr)
    {
        *out_opt_flags_ptr = image_opaque_bind.flags;
    }

    if (out_opt_memory_block_ptr_ptr != nullptr)
    {
        *out_opt_memory_block_ptr_ptr = image_opaque_bind.memory_block_ptr;
    }

    if (out_opt_memory_block_start_offset_ptr != nullptr)
    {
        *out_opt_memory_block_start_offset_ptr = image_opaque_bind.memory_block_start_offset;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
Anvil::MemoryFeatureFlags Anvil::Utils::get_memory_feature_flags_from_vk_property_flags(VkMemoryPropertyFlags in_mem_type_flags,
                                                                                        VkMemoryHeapFlags     in_mem_heap_flags)
{
    Anvil::MemoryFeatureFlags result = 0;

    ANVIL_REDUNDANT_ARGUMENT(in_mem_heap_flags);

    if ((in_mem_type_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
    {
        result |= MEMORY_FEATURE_FLAG_DEVICE_LOCAL;
    }

    if ((in_mem_type_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
    {
        result |= MEMORY_FEATURE_FLAG_MAPPABLE;
    }

    if ((in_mem_type_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0)
    {
        result |= MEMORY_FEATURE_FLAG_HOST_COHERENT;
    }

    if ((in_mem_type_flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0)
    {
        result |= MEMORY_FEATURE_FLAG_HOST_CACHED;
    }

    if ((in_mem_type_flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) != 0)
    {
        result |= MEMORY_FEATURE_FLAG_LAZILY_ALLOCATED;
    }

    return result;
}

/** Please see header for specification */
void Anvil::Utils::convert_queue_family_bits_to_family_indices(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                               Anvil::QueueFamilyBits           in_queue_families,
                                                               uint32_t*                        out_opt_queue_family_indices_ptr,
                                                               uint32_t*                        out_opt_n_queue_family_indices_ptr)
{
    std::shared_ptr<BaseDevice> device_locked_ptr       (in_device_ptr);
    uint32_t                    n_queue_family_indices  (0);
    bool                        universal_queue_included(false);

    if ((in_queue_families & QUEUE_FAMILY_COMPUTE_BIT) != 0)
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

    if ((in_queue_families & QUEUE_FAMILY_DMA_BIT) != 0)
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

    if (((in_queue_families & QUEUE_FAMILY_GRAPHICS_BIT) != 0) &&
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
VkAccessFlags Anvil::Utils::get_access_mask_from_image_layout(VkImageLayout in_layout)
{
    VkAccessFlags result = 0;

    switch (in_layout)
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
            anvil_assert_fail();
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
const char* Anvil::Utils::get_raw_string(VkBlendFactor in_blend_factor)
{
    const char* result = "?";

    switch (in_blend_factor)
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
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkBlendOp in_blend_op)
{
    const char* result = "?";

    switch (in_blend_op)
    {
        case VK_BLEND_OP_ADD:              result = "VK_BLEND_OP_ADD";              break;
        case VK_BLEND_OP_SUBTRACT:         result = "VK_BLEND_OP_SUBTRACT";         break;
        case VK_BLEND_OP_REVERSE_SUBTRACT: result = "VK_BLEND_OP_REVERSE_SUBTRACT"; break;
        case VK_BLEND_OP_MIN:              result = "VK_BLEND_OP_MIN";              break;
        case VK_BLEND_OP_MAX:              result = "VK_BLEND_OP_MAX";              break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkCompareOp in_compare_op)
{
    const char* result = "?";

    switch (in_compare_op)
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
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkCullModeFlagBits in_cull_mode)
{
    const char* result = "?";

    switch (in_cull_mode)
    {
        case VK_CULL_MODE_NONE:           result = "VK_CULL_MODE_NONE";           break;
        case VK_CULL_MODE_FRONT_BIT:      result = "VK_CULL_MODE_FRONT_BIT";      break;
        case VK_CULL_MODE_BACK_BIT:       result = "VK_CULL_MODE_BACK_BIT";       break;
        case VK_CULL_MODE_FRONT_AND_BACK: result = "VK_CULL_MODE_FRONT_AND_BACK"; break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkDescriptorType in_descriptor_type)
{
    const char* result = "?";

    switch (in_descriptor_type)
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
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkFrontFace in_front_face)
{
    const char* result = "?";

    switch (in_front_face)
    {
        case VK_FRONT_FACE_COUNTER_CLOCKWISE: result = "VK_FRONT_FACE_COUNTER_CLOCKWISE"; break;
        case VK_FRONT_FACE_CLOCKWISE:         result = "VK_FRONT_FACE_CLOCKWISE";         break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkImageAspectFlagBits in_image_aspect_flag)
{
    const char* result = "?";

    switch (in_image_aspect_flag)
    {
        case VK_IMAGE_ASPECT_COLOR_BIT:    result = "VK_IMAGE_ASPECT_COLOR_BIT";    break;
        case VK_IMAGE_ASPECT_DEPTH_BIT:    result = "VK_IMAGE_ASPECT_DEPTH_BIT";    break;
        case VK_IMAGE_ASPECT_STENCIL_BIT:  result = "VK_IMAGE_ASPECT_STENCIL_BIT";  break;
        case VK_IMAGE_ASPECT_METADATA_BIT: result = "VK_IMAGE_ASPECT_METADATA_BIT"; break;

        default:
        {
            anvil_assert_fail();
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
            anvil_assert_fail();

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
const char* Anvil::Utils::get_raw_string(VkLogicOp in_logic_op)
{
    const char* result = "?";

    switch (in_logic_op)
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
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkPolygonMode in_polygon_mode)
{
    const char* result = "?";

    switch (in_polygon_mode)
    {
        case VK_POLYGON_MODE_FILL:  result = "VK_POLYGON_MODE_FILL";  break;
        case VK_POLYGON_MODE_LINE:  result = "VK_POLYGON_MODE_LINE";  break;
        case VK_POLYGON_MODE_POINT: result = "VK_POLYGON_MODE_POINT"; break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkPrimitiveTopology in_topology)
{
    const char* result = "?";

    switch (in_topology)
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
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkSampleCountFlagBits in_sample_count)
{
    const char* result = "?";

    switch (in_sample_count)
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
            anvil_assert_fail();
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
const char* Anvil::Utils::get_raw_string(VkStencilOp in_stencil_op)
{
    const char* result = "?";

    switch (in_stencil_op)
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
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
void Anvil::Utils::get_vk_property_flags_from_memory_feature_flags(Anvil::MemoryFeatureFlags in_mem_feature_flags,
                                                                   VkMemoryPropertyFlags*    out_mem_type_flags_ptr,
                                                                   VkMemoryHeapFlags*        out_mem_heap_flags_ptr)
{
    VkMemoryHeapFlags     result_mem_heap_flags = 0;
    VkMemoryPropertyFlags result_mem_type_flags = 0;

    if ((in_mem_feature_flags & MEMORY_FEATURE_FLAG_DEVICE_LOCAL) != 0)
    {
        result_mem_type_flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    if ((in_mem_feature_flags & MEMORY_FEATURE_FLAG_MAPPABLE) != 0)
    {
        result_mem_type_flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    if ((in_mem_feature_flags & MEMORY_FEATURE_FLAG_HOST_COHERENT) != 0)
    {
        result_mem_type_flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    if ((in_mem_feature_flags & MEMORY_FEATURE_FLAG_HOST_CACHED) != 0)
    {
        result_mem_type_flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    }

    if ((in_mem_feature_flags & MEMORY_FEATURE_FLAG_LAZILY_ALLOCATED) != 0)
    {
        result_mem_type_flags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    }

    *out_mem_heap_flags_ptr = result_mem_heap_flags;
    *out_mem_type_flags_ptr = result_mem_type_flags;
}
