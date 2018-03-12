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

#include <cfloat>
#include <cmath>
#include "misc/debug.h"
#include "misc/descriptor_set_info.h"
#include "wrappers/buffer.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/device.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include "wrappers/physical_device.h"
#include "wrappers/semaphore.h"
#include "wrappers/shader_module.h"

#define BOOL_TO_VK_BOOL32(x) ((x)             ? VK_TRUE : VK_FALSE);
#define VK_BOOL32_TO_BOOL(x) ((x == VK_FALSE) ? false   : true)

#ifdef max
    #undef max
#endif

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

Anvil::DescriptorSetAllocation::DescriptorSetAllocation(const Anvil::DescriptorSetLayout* in_ds_layout_ptr)
{
    anvil_assert( in_ds_layout_ptr != nullptr);

    ds_layout_ptr = in_ds_layout_ptr;
}

/** Please see header for specification */
VkDescriptorUpdateTemplateEntryKHR Anvil::DescriptorUpdateTemplateEntry::get_vk_descriptor_update_template_entry_khr() const
{
    VkDescriptorUpdateTemplateEntryKHR result;

    result.descriptorCount = n_descriptors;
    result.descriptorType  = descriptor_type;
    result.dstArrayElement = n_destination_array_element;
    result.dstBinding      = n_destination_binding;
    result.offset          = offset;
    result.stride          = stride;

    return result;
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
    amd_shader_fragment_mask             = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_shader_image_load_store_lod      = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_shader_info                      = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_shader_trinary_minmax            = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    amd_texture_gather_bias_lod          = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    ext_shader_stencil_export            = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    ext_shader_subgroup_ballot           = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    ext_shader_subgroup_vote             = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    khr_16bit_storage                    = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    khr_bind_memory2                     = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    khr_descriptor_update_template       = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    khr_maintenance1                     = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
    khr_maintenance3                     = EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE;
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
bool Anvil::DeviceExtensionConfiguration::is_supported_by_physical_device(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                                                          std::vector<std::string>*    out_opt_unsupported_extensions_ptr) const
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

    bool                       result     = true;
    std::vector<ExtensionItem> extensions =
    {
        ExtensionItem(VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME,              amd_draw_indirect_count              == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_GCN_SHADER_EXTENSION_NAME,                       amd_gcn_shader                       == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_GPU_SHADER_HALF_FLOAT_EXTENSION_NAME,            amd_gpu_shader_half_float            == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_GPU_SHADER_INT16_EXTENSION_NAME,                 amd_gpu_shader_int16                 == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME,         amd_negative_viewport_height         == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME,              amd_rasterization_order              == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_SHADER_BALLOT_EXTENSION_NAME,                    amd_shader_ballot                    == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER_EXTENSION_NAME, amd_shader_explicit_vertex_parameter == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_SHADER_FRAGMENT_MASK_EXTENSION_NAME,             amd_shader_fragment_mask             == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_SHADER_IMAGE_LOAD_STORE_LOD_EXTENSION_NAME,      amd_shader_image_load_store_lod      == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_SHADER_INFO_EXTENSION_NAME,                      amd_shader_info                      == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_SHADER_TRINARY_MINMAX_EXTENSION_NAME,            amd_shader_trinary_minmax            == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_AMD_TEXTURE_GATHER_BIAS_LOD_EXTENSION_NAME,          amd_texture_gather_bias_lod          == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_EXT_DEBUG_MARKER_EXTENSION_NAME,                     ext_debug_marker                     == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem("VK_EXT_shader_stencil_export",                         ext_shader_stencil_export            == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME,           ext_shader_subgroup_ballot           == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME,             ext_shader_subgroup_vote             == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_KHR_16BIT_STORAGE_EXTENSION_NAME,                    khr_16bit_storage                    == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,                    khr_bind_memory2                     == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,       khr_descriptor_update_template       == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_KHR_MAINTENANCE1_EXTENSION_NAME,                     khr_maintenance1                     == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
        ExtensionItem(VK_KHR_MAINTENANCE3_EXTENSION_NAME,                     khr_maintenance3                     == Anvil::EXTENSION_AVAILABILITY_REQUIRE),
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
        if (!in_physical_device_ptr->is_device_extension_supported(current_extension.extension_name) &&
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
             (amd_shader_fragment_mask             == in_config.amd_shader_fragment_mask)             &&
             (amd_shader_image_load_store_lod      == in_config.amd_shader_image_load_store_lod)      &&
             (amd_shader_info                      == in_config.amd_shader_info)                      &&
             (amd_shader_trinary_minmax            == in_config.amd_shader_trinary_minmax)            &&
             (amd_texture_gather_bias_lod          == in_config.amd_texture_gather_bias_lod)          &&
             (ext_debug_marker                     == in_config.ext_debug_marker)                     &&
             (ext_shader_stencil_export            == in_config.ext_shader_stencil_export)            &&
             (ext_shader_subgroup_ballot           == in_config.ext_shader_subgroup_ballot)           &&
             (ext_shader_subgroup_vote             == in_config.ext_shader_subgroup_vote)             &&
             (khr_16bit_storage                    == in_config.khr_16bit_storage)                    &&
             (khr_bind_memory2                     == in_config.khr_bind_memory2)                     &&
             (khr_descriptor_update_template       == in_config.khr_descriptor_update_template)       &&
             (khr_maintenance1                     == in_config.khr_maintenance1)                     &&
             (khr_maintenance3                     == in_config.khr_maintenance3)                     &&
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

Anvil::KHR16BitStorageFeatures::KHR16BitStorageFeatures(const VkPhysicalDevice16BitStorageFeaturesKHR& in_features)
{
    is_input_output_storage_supported                     = VK_BOOL32_TO_BOOL(in_features.storageInputOutput16);
    is_push_constant_16_bit_storage_supported             = VK_BOOL32_TO_BOOL(in_features.storagePushConstant16);
    is_storage_buffer_16_bit_access_supported             = VK_BOOL32_TO_BOOL(in_features.storageBuffer16BitAccess);
    is_uniform_and_storage_buffer_16_bit_access_supported = VK_BOOL32_TO_BOOL(in_features.uniformAndStorageBuffer16BitAccess);
}

bool Anvil::KHR16BitStorageFeatures::operator==(const KHR16BitStorageFeatures& in_features) const
{
    return (in_features.is_input_output_storage_supported                     == is_input_output_storage_supported                      &&
            in_features.is_push_constant_16_bit_storage_supported             == is_push_constant_16_bit_storage_supported              &&
            in_features.is_storage_buffer_16_bit_access_supported             == is_storage_buffer_16_bit_access_supported              &&
            in_features.is_uniform_and_storage_buffer_16_bit_access_supported == is_uniform_and_storage_buffer_16_bit_access_supported);
}

Anvil::KHRMaintenance3Properties::KHRMaintenance3Properties()
    :max_memory_allocation_size(std::numeric_limits<VkDeviceSize>::max() ),
     max_per_set_descriptors   (UINT32_MAX)
{
    /* Stub */
}

Anvil::KHRMaintenance3Properties::KHRMaintenance3Properties(const VkPhysicalDeviceMaintenance3PropertiesKHR& in_props)
    :max_memory_allocation_size(in_props.maxMemoryAllocationSize),
     max_per_set_descriptors   (in_props.maxPerSetDescriptors)
{
    /* Stub */
}

bool Anvil::KHRMaintenance3Properties::operator==(const Anvil::KHRMaintenance3Properties& in_props) const
{
    return (max_memory_allocation_size == in_props.max_memory_allocation_size &&
            max_per_set_descriptors    == in_props.max_per_set_descriptors);
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

Anvil::PhysicalDeviceLimits::PhysicalDeviceLimits()
    :buffer_image_granularity                             (std::numeric_limits<VkDeviceSize>::max() ),
     discrete_queue_priorities                            (UINT32_MAX),
     framebuffer_color_sample_counts                      (VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM),
     framebuffer_depth_sample_counts                      (VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM),
     framebuffer_no_attachments_sample_counts             (VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM),
     framebuffer_stencil_sample_counts                    (VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM),
     line_width_granularity                               (FLT_MAX),
     max_bound_descriptor_sets                            (UINT32_MAX),
     max_clip_distances                                   (UINT32_MAX),
     max_color_attachments                                (UINT32_MAX),
     max_combined_clip_and_cull_distances                 (UINT32_MAX),
     max_compute_shared_memory_size                       (UINT32_MAX),
     max_compute_work_group_invocations                   (UINT32_MAX),
     max_cull_distances                                   (UINT32_MAX),
     max_descriptor_set_input_attachments                 (UINT32_MAX),
     max_descriptor_set_sampled_images                    (UINT32_MAX),
     max_descriptor_set_samplers                          (UINT32_MAX),
     max_descriptor_set_storage_buffers                   (UINT32_MAX),
     max_descriptor_set_storage_buffers_dynamic           (UINT32_MAX),
     max_descriptor_set_storage_images                    (UINT32_MAX),
     max_descriptor_set_uniform_buffers                   (UINT32_MAX),
     max_descriptor_set_uniform_buffers_dynamic           (UINT32_MAX),
     max_draw_indexed_index_value                         (UINT32_MAX),
     max_draw_indirect_count                              (UINT32_MAX),
     max_fragment_combined_output_resources               (UINT32_MAX),
     max_fragment_dual_src_attachments                    (UINT32_MAX),
     max_fragment_input_components                        (UINT32_MAX),
     max_fragment_output_attachments                      (UINT32_MAX),
     max_framebuffer_height                               (UINT32_MAX),
     max_framebuffer_layers                               (UINT32_MAX),
     max_framebuffer_width                                (UINT32_MAX),
     max_geometry_input_components                        (UINT32_MAX),
     max_geometry_output_components                       (UINT32_MAX),
     max_geometry_output_vertices                         (UINT32_MAX),
     max_geometry_shader_invocations                      (UINT32_MAX),
     max_geometry_total_output_components                 (UINT32_MAX),
     max_image_array_layers                               (UINT32_MAX),
     max_image_dimension_1D                               (UINT32_MAX),
     max_image_dimension_2D                               (UINT32_MAX),
     max_image_dimension_3D                               (UINT32_MAX),
     max_image_dimension_cube                             (UINT32_MAX),
     max_interpolation_offset                             (FLT_MAX),
     max_memory_allocation_count                          (UINT32_MAX),
     max_per_stage_descriptor_input_attachments           (UINT32_MAX),
     max_per_stage_descriptor_sampled_images              (UINT32_MAX),
     max_per_stage_descriptor_samplers                    (UINT32_MAX),
     max_per_stage_descriptor_storage_buffers             (UINT32_MAX),
     max_per_stage_descriptor_storage_images              (UINT32_MAX),
     max_per_stage_descriptor_uniform_buffers             (UINT32_MAX),
     max_per_stage_resources                              (UINT32_MAX),
     max_push_constants_size                              (UINT32_MAX),
     max_sample_mask_words                                (UINT32_MAX),
     max_sampler_allocation_count                         (UINT32_MAX),
     max_sampler_anisotropy                               (FLT_MAX),
     max_sampler_lod_bias                                 (FLT_MAX),
     max_storage_buffer_range                             (UINT32_MAX),
     max_viewports                                        (UINT32_MAX),
     max_tessellation_control_per_patch_output_components (UINT32_MAX),
     max_tessellation_control_per_vertex_input_components (UINT32_MAX),
     max_tessellation_control_per_vertex_output_components(UINT32_MAX),
     max_tessellation_control_total_output_components     (UINT32_MAX),
     max_tessellation_evaluation_input_components         (UINT32_MAX),
     max_tessellation_evaluation_output_components        (UINT32_MAX),
     max_tessellation_generation_level                    (UINT32_MAX),
     max_tessellation_patch_size                          (UINT32_MAX),
     max_texel_buffer_elements                            (UINT32_MAX),
     max_texel_gather_offset                              (UINT32_MAX),
     max_texel_offset                                     (UINT32_MAX),
     max_uniform_buffer_range                             (UINT32_MAX),
     max_vertex_input_attributes                          (UINT32_MAX),
     max_vertex_input_attribute_offset                    (UINT32_MAX),
     max_vertex_input_bindings                            (UINT32_MAX),
     max_vertex_input_binding_stride                      (UINT32_MAX),
     max_vertex_output_components                         (UINT32_MAX),
     min_interpolation_offset                             (FLT_MAX),
     min_memory_map_alignment                             (std::numeric_limits<size_t>::max      () ),
     min_storage_buffer_offset_alignment                  (std::numeric_limits<VkDeviceSize>::max() ),
     min_texel_buffer_offset_alignment                    (std::numeric_limits<VkDeviceSize>::max() ),
     min_texel_gather_offset                              (INT32_MAX),
     min_texel_offset                                     (INT32_MAX),
     min_uniform_buffer_offset_alignment                  (std::numeric_limits<VkDeviceSize>::max() ),
     mipmap_precision_bits                                (UINT32_MAX),
     non_coherent_atom_size                               (std::numeric_limits<VkDeviceSize>::max() ),
     optimal_buffer_copy_offset_alignment                 (std::numeric_limits<VkDeviceSize>::max() ),
     optimal_buffer_copy_row_pitch_alignment              (std::numeric_limits<VkDeviceSize>::max() ),
     point_size_granularity                               (FLT_MAX),
     sampled_image_color_sample_counts                    (VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM), 
     sampled_image_depth_sample_counts                    (VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM),
     sampled_image_integer_sample_counts                  (VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM),
     sampled_image_stencil_sample_counts                  (VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM),
     sparse_address_space_size                            (std::numeric_limits<VkDeviceSize>::max() ),
     standard_sample_locations                            (false),
     storage_image_sample_counts                          (VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM),
     strict_lines                                         (false),
     sub_pixel_interpolation_offset_bits                  (UINT32_MAX),
     sub_pixel_precision_bits                             (UINT32_MAX),
     sub_texel_precision_bits                             (UINT32_MAX),
     timestamp_compute_and_graphics                       (false),
     timestamp_period                                     (FLT_MAX),
     viewport_sub_pixel_bits                              (UINT32_MAX)
{
    line_width_range[0] = FLT_MAX;
    line_width_range[1] = FLT_MAX;

    max_compute_work_group_count[0] = UINT32_MAX;
    max_compute_work_group_count[1] = UINT32_MAX;
    max_compute_work_group_count[2] = UINT32_MAX;

    max_compute_work_group_size[0] = UINT32_MAX;
    max_compute_work_group_size[1] = UINT32_MAX;
    max_compute_work_group_size[2] = UINT32_MAX;

    max_viewport_dimensions[0] = UINT32_MAX;
    max_viewport_dimensions[1] = UINT32_MAX;

    point_size_range[0] = FLT_MAX;
    point_size_range[1] = FLT_MAX;

    viewport_bounds_range[0] = FLT_MAX;
    viewport_bounds_range[1] = FLT_MAX;
}

Anvil::PhysicalDeviceLimits::PhysicalDeviceLimits(const VkPhysicalDeviceLimits& in_device_limits)
    :buffer_image_granularity                             (in_device_limits.bufferImageGranularity),
     discrete_queue_priorities                            (in_device_limits.discreteQueuePriorities),
     framebuffer_color_sample_counts                      (in_device_limits.framebufferColorSampleCounts),
     framebuffer_depth_sample_counts                      (in_device_limits.framebufferDepthSampleCounts),
     framebuffer_no_attachments_sample_counts             (in_device_limits.framebufferNoAttachmentsSampleCounts),
     framebuffer_stencil_sample_counts                    (in_device_limits.framebufferStencilSampleCounts),
     line_width_granularity                               (in_device_limits.lineWidthGranularity),
     max_bound_descriptor_sets                            (in_device_limits.maxBoundDescriptorSets),
     max_clip_distances                                   (in_device_limits.maxClipDistances),
     max_color_attachments                                (in_device_limits.maxColorAttachments),
     max_combined_clip_and_cull_distances                 (in_device_limits.maxCombinedClipAndCullDistances),
     max_compute_shared_memory_size                       (in_device_limits.maxComputeSharedMemorySize),
     max_compute_work_group_invocations                   (in_device_limits.maxComputeWorkGroupInvocations),
     max_cull_distances                                   (in_device_limits.maxCullDistances),
     max_descriptor_set_input_attachments                 (in_device_limits.maxDescriptorSetInputAttachments),
     max_descriptor_set_sampled_images                    (in_device_limits.maxDescriptorSetSampledImages),
     max_descriptor_set_samplers                          (in_device_limits.maxDescriptorSetSamplers),
     max_descriptor_set_storage_buffers                   (in_device_limits.maxDescriptorSetStorageBuffers),
     max_descriptor_set_storage_buffers_dynamic           (in_device_limits.maxDescriptorSetStorageBuffersDynamic),
     max_descriptor_set_storage_images                    (in_device_limits.maxDescriptorSetStorageImages),
     max_descriptor_set_uniform_buffers                   (in_device_limits.maxDescriptorSetUniformBuffers),
     max_descriptor_set_uniform_buffers_dynamic           (in_device_limits.maxDescriptorSetUniformBuffersDynamic),
     max_draw_indexed_index_value                         (in_device_limits.maxDrawIndexedIndexValue),
     max_draw_indirect_count                              (in_device_limits.maxDrawIndirectCount),
     max_fragment_combined_output_resources               (in_device_limits.maxFragmentCombinedOutputResources),
     max_fragment_dual_src_attachments                    (in_device_limits.maxFragmentDualSrcAttachments),
     max_fragment_input_components                        (in_device_limits.maxFragmentInputComponents),
     max_fragment_output_attachments                      (in_device_limits.maxFragmentOutputAttachments),
     max_framebuffer_height                               (in_device_limits.maxFramebufferHeight),
     max_framebuffer_layers                               (in_device_limits.maxFramebufferLayers),
     max_framebuffer_width                                (in_device_limits.maxFramebufferWidth),
     max_geometry_input_components                        (in_device_limits.maxGeometryInputComponents),
     max_geometry_output_components                       (in_device_limits.maxGeometryOutputComponents),
     max_geometry_output_vertices                         (in_device_limits.maxGeometryOutputVertices),
     max_geometry_shader_invocations                      (in_device_limits.maxGeometryShaderInvocations),
     max_geometry_total_output_components                 (in_device_limits.maxGeometryTotalOutputComponents),
     max_image_array_layers                               (in_device_limits.maxImageArrayLayers),
     max_image_dimension_1D                               (in_device_limits.maxImageDimension1D),
     max_image_dimension_2D                               (in_device_limits.maxImageDimension2D),
     max_image_dimension_3D                               (in_device_limits.maxImageDimension3D),
     max_image_dimension_cube                             (in_device_limits.maxImageDimensionCube),
     max_interpolation_offset                             (in_device_limits.maxInterpolationOffset),
     max_memory_allocation_count                          (in_device_limits.maxMemoryAllocationCount),
     max_per_stage_descriptor_input_attachments           (in_device_limits.maxPerStageDescriptorInputAttachments),
     max_per_stage_descriptor_sampled_images              (in_device_limits.maxPerStageDescriptorSampledImages),
     max_per_stage_descriptor_samplers                    (in_device_limits.maxPerStageDescriptorSamplers),
     max_per_stage_descriptor_storage_buffers             (in_device_limits.maxPerStageDescriptorStorageBuffers),
     max_per_stage_descriptor_storage_images              (in_device_limits.maxPerStageDescriptorStorageImages),
     max_per_stage_descriptor_uniform_buffers             (in_device_limits.maxPerStageDescriptorUniformBuffers),
     max_per_stage_resources                              (in_device_limits.maxPerStageResources),
     max_push_constants_size                              (in_device_limits.maxPushConstantsSize),
     max_sample_mask_words                                (in_device_limits.maxSampleMaskWords),
     max_sampler_allocation_count                         (in_device_limits.maxSamplerAllocationCount),
     max_sampler_anisotropy                               (in_device_limits.maxSamplerAnisotropy),
     max_sampler_lod_bias                                 (in_device_limits.maxSamplerLodBias),
     max_storage_buffer_range                             (in_device_limits.maxStorageBufferRange),
     max_viewports                                        (in_device_limits.maxViewports),
     max_tessellation_control_per_patch_output_components (in_device_limits.maxTessellationControlPerPatchOutputComponents),
     max_tessellation_control_per_vertex_input_components (in_device_limits.maxTessellationControlPerVertexInputComponents),
     max_tessellation_control_per_vertex_output_components(in_device_limits.maxTessellationControlPerVertexOutputComponents),
     max_tessellation_control_total_output_components     (in_device_limits.maxTessellationControlTotalOutputComponents),
     max_tessellation_evaluation_input_components         (in_device_limits.maxTessellationEvaluationInputComponents),
     max_tessellation_evaluation_output_components        (in_device_limits.maxTessellationEvaluationOutputComponents),
     max_tessellation_generation_level                    (in_device_limits.maxTessellationGenerationLevel),
     max_tessellation_patch_size                          (in_device_limits.maxTessellationPatchSize),
     max_texel_buffer_elements                            (in_device_limits.maxTexelBufferElements),
     max_texel_gather_offset                              (in_device_limits.maxTexelGatherOffset),
     max_texel_offset                                     (in_device_limits.maxTexelOffset),
     max_uniform_buffer_range                             (in_device_limits.maxUniformBufferRange),
     max_vertex_input_attributes                          (in_device_limits.maxVertexInputAttributes),
     max_vertex_input_attribute_offset                    (in_device_limits.maxVertexInputAttributeOffset),
     max_vertex_input_bindings                            (in_device_limits.maxVertexInputBindings),
     max_vertex_input_binding_stride                      (in_device_limits.maxVertexInputBindingStride),
     max_vertex_output_components                         (in_device_limits.maxVertexOutputComponents),
     min_interpolation_offset                             (in_device_limits.minInterpolationOffset),
     min_memory_map_alignment                             (in_device_limits.minMemoryMapAlignment),
     min_storage_buffer_offset_alignment                  (in_device_limits.minStorageBufferOffsetAlignment),
     min_texel_buffer_offset_alignment                    (in_device_limits.minTexelBufferOffsetAlignment),
     min_texel_gather_offset                              (in_device_limits.minTexelGatherOffset),
     min_texel_offset                                     (in_device_limits.minTexelOffset),
     min_uniform_buffer_offset_alignment                  (in_device_limits.minUniformBufferOffsetAlignment),
     mipmap_precision_bits                                (in_device_limits.mipmapPrecisionBits),
     non_coherent_atom_size                               (in_device_limits.nonCoherentAtomSize),
     optimal_buffer_copy_offset_alignment                 (in_device_limits.optimalBufferCopyOffsetAlignment),
     optimal_buffer_copy_row_pitch_alignment              (in_device_limits.optimalBufferCopyRowPitchAlignment),
     point_size_granularity                               (in_device_limits.pointSizeGranularity),
     sampled_image_color_sample_counts                    (in_device_limits.sampledImageColorSampleCounts),
     sampled_image_depth_sample_counts                    (in_device_limits.sampledImageDepthSampleCounts),
     sampled_image_integer_sample_counts                  (in_device_limits.sampledImageIntegerSampleCounts),
     sampled_image_stencil_sample_counts                  (in_device_limits.sampledImageStencilSampleCounts),
     sparse_address_space_size                            (in_device_limits.sparseAddressSpaceSize),
     standard_sample_locations                            (VK_BOOL32_TO_BOOL(in_device_limits.standardSampleLocations) ),
     storage_image_sample_counts                          (in_device_limits.storageImageSampleCounts),
     strict_lines                                         (VK_BOOL32_TO_BOOL(in_device_limits.strictLines) ),
     sub_pixel_interpolation_offset_bits                  (in_device_limits.subPixelInterpolationOffsetBits),
     sub_pixel_precision_bits                             (in_device_limits.subPixelPrecisionBits),
     sub_texel_precision_bits                             (in_device_limits.subTexelPrecisionBits),
     timestamp_compute_and_graphics                       (VK_BOOL32_TO_BOOL(in_device_limits.timestampComputeAndGraphics) ),
     timestamp_period                                     (in_device_limits.timestampPeriod),
     viewport_sub_pixel_bits                              (in_device_limits.viewportSubPixelBits)
{
    memcpy(line_width_range,
           in_device_limits.lineWidthRange,
           sizeof(line_width_range) );

    memcpy(max_compute_work_group_count,
           in_device_limits.maxComputeWorkGroupCount,
           sizeof(max_compute_work_group_count) );

    memcpy(max_compute_work_group_size,
           in_device_limits.maxComputeWorkGroupSize,
           sizeof(max_compute_work_group_size) );

    memcpy(max_viewport_dimensions,
           in_device_limits.maxViewportDimensions,
           sizeof(max_viewport_dimensions) );

    memcpy(point_size_range,
           in_device_limits.pointSizeRange,
           sizeof(point_size_range) );

    memcpy(viewport_bounds_range,
           in_device_limits.viewportBoundsRange,
           sizeof(viewport_bounds_range) );
}

bool Anvil::PhysicalDeviceLimits::operator==(const Anvil::PhysicalDeviceLimits& in_device_limits) const
{
    bool result = false;

     if (buffer_image_granularity                              == in_device_limits.buffer_image_granularity                              &&
         discrete_queue_priorities                             == in_device_limits.discrete_queue_priorities                             &&
         framebuffer_color_sample_counts                       == in_device_limits.framebuffer_color_sample_counts                       &&
         framebuffer_depth_sample_counts                       == in_device_limits.framebuffer_depth_sample_counts                       &&
         framebuffer_no_attachments_sample_counts              == in_device_limits.framebuffer_no_attachments_sample_counts              &&
         framebuffer_stencil_sample_counts                     == in_device_limits.framebuffer_stencil_sample_counts                     &&
         max_bound_descriptor_sets                             == in_device_limits.max_bound_descriptor_sets                             &&
         max_clip_distances                                    == in_device_limits.max_clip_distances                                    &&
         max_color_attachments                                 == in_device_limits.max_color_attachments                                 &&
         max_combined_clip_and_cull_distances                  == in_device_limits.max_combined_clip_and_cull_distances                  &&
         max_compute_shared_memory_size                        == in_device_limits.max_compute_shared_memory_size                        &&
         max_compute_work_group_count[0]                       == in_device_limits.max_compute_work_group_count[0]                       &&
         max_compute_work_group_count[1]                       == in_device_limits.max_compute_work_group_count[1]                       &&
         max_compute_work_group_count[2]                       == in_device_limits.max_compute_work_group_count[2]                       &&
         max_compute_work_group_invocations                    == in_device_limits.max_compute_work_group_invocations                    &&
         max_compute_work_group_size[0]                        == in_device_limits.max_compute_work_group_size[0]                        &&
         max_compute_work_group_size[1]                        == in_device_limits.max_compute_work_group_size[1]                        &&
         max_compute_work_group_size[2]                        == in_device_limits.max_compute_work_group_size[2]                        &&
         max_cull_distances                                    == in_device_limits.max_cull_distances                                    &&
         max_descriptor_set_input_attachments                  == in_device_limits.max_descriptor_set_input_attachments                  &&
         max_descriptor_set_sampled_images                     == in_device_limits.max_descriptor_set_sampled_images                     &&
         max_descriptor_set_samplers                           == in_device_limits.max_descriptor_set_samplers                           &&
         max_descriptor_set_storage_buffers                    == in_device_limits.max_descriptor_set_storage_buffers                    &&
         max_descriptor_set_storage_buffers_dynamic            == in_device_limits.max_descriptor_set_storage_buffers_dynamic            &&
         max_descriptor_set_storage_images                     == in_device_limits.max_descriptor_set_storage_images                     &&
         max_descriptor_set_uniform_buffers                    == in_device_limits.max_descriptor_set_uniform_buffers                    &&
         max_descriptor_set_uniform_buffers_dynamic            == in_device_limits.max_descriptor_set_uniform_buffers_dynamic            &&
         max_draw_indexed_index_value                          == in_device_limits.max_draw_indexed_index_value                          &&
         max_draw_indirect_count                               == in_device_limits.max_draw_indirect_count                               &&
         max_fragment_combined_output_resources                == in_device_limits.max_fragment_combined_output_resources                &&
         max_fragment_dual_src_attachments                     == in_device_limits.max_fragment_dual_src_attachments                     &&
         max_fragment_input_components                         == in_device_limits.max_fragment_input_components                         &&
         max_fragment_output_attachments                       == in_device_limits.max_fragment_output_attachments                       &&
         max_framebuffer_height                                == in_device_limits.max_framebuffer_height                                &&
         max_framebuffer_layers                                == in_device_limits.max_framebuffer_layers                                &&
         max_framebuffer_width                                 == in_device_limits.max_framebuffer_width                                 &&
         max_geometry_input_components                         == in_device_limits.max_geometry_input_components                         &&
         max_geometry_output_components                        == in_device_limits.max_geometry_output_components                        &&
         max_geometry_output_vertices                          == in_device_limits.max_geometry_output_vertices                          &&
         max_geometry_shader_invocations                       == in_device_limits.max_geometry_shader_invocations                       &&
         max_geometry_total_output_components                  == in_device_limits.max_geometry_total_output_components                  &&
         max_image_array_layers                                == in_device_limits.max_image_array_layers                                &&
         max_image_dimension_1D                                == in_device_limits.max_image_dimension_1D                                &&
         max_image_dimension_2D                                == in_device_limits.max_image_dimension_2D                                &&
         max_image_dimension_3D                                == in_device_limits.max_image_dimension_3D                                &&
         max_image_dimension_cube                              == in_device_limits.max_image_dimension_cube                              &&
         max_memory_allocation_count                           == in_device_limits.max_memory_allocation_count                           &&
         max_per_stage_descriptor_input_attachments            == in_device_limits.max_per_stage_descriptor_input_attachments            &&
         max_per_stage_descriptor_sampled_images               == in_device_limits.max_per_stage_descriptor_sampled_images               &&
         max_per_stage_descriptor_samplers                     == in_device_limits.max_per_stage_descriptor_samplers                     &&
         max_per_stage_descriptor_storage_buffers              == in_device_limits.max_per_stage_descriptor_storage_buffers              &&
         max_per_stage_descriptor_storage_images               == in_device_limits.max_per_stage_descriptor_storage_images               &&
         max_per_stage_descriptor_uniform_buffers              == in_device_limits.max_per_stage_descriptor_uniform_buffers              &&
         max_per_stage_resources                               == in_device_limits.max_per_stage_resources                               &&
         max_push_constants_size                               == in_device_limits.max_push_constants_size                               &&
         max_sample_mask_words                                 == in_device_limits.max_sample_mask_words                                 &&
         max_sampler_allocation_count                          == in_device_limits.max_sampler_allocation_count                          &&
         max_storage_buffer_range                              == in_device_limits.max_storage_buffer_range                              &&
         max_viewport_dimensions[0]                            == in_device_limits.max_viewport_dimensions[0]                            &&
         max_viewport_dimensions[1]                            == in_device_limits.max_viewport_dimensions[1]                            &&
         max_viewports                                         == in_device_limits.max_viewports                                         &&
         max_tessellation_control_per_patch_output_components  == in_device_limits.max_tessellation_control_per_patch_output_components  &&
         max_tessellation_control_per_vertex_input_components  == in_device_limits.max_tessellation_control_per_vertex_input_components  &&
         max_tessellation_control_per_vertex_output_components == in_device_limits.max_tessellation_control_per_vertex_output_components &&
         max_tessellation_control_total_output_components      == in_device_limits.max_tessellation_control_total_output_components      &&
         max_tessellation_evaluation_input_components          == in_device_limits.max_tessellation_evaluation_input_components          &&
         max_tessellation_evaluation_output_components         == in_device_limits.max_tessellation_evaluation_output_components         &&
         max_tessellation_generation_level                     == in_device_limits.max_tessellation_generation_level                     &&
         max_tessellation_patch_size                           == in_device_limits.max_tessellation_patch_size                           &&
         max_texel_buffer_elements                             == in_device_limits.max_texel_buffer_elements                             &&
         max_texel_gather_offset                               == in_device_limits.max_texel_gather_offset                               &&
         max_texel_offset                                      == in_device_limits.max_texel_offset                                      &&
         max_uniform_buffer_range                              == in_device_limits.max_uniform_buffer_range                              &&
         max_vertex_input_attributes                           == in_device_limits.max_vertex_input_attributes                           &&
         max_vertex_input_attribute_offset                     == in_device_limits.max_vertex_input_attribute_offset                     &&
         max_vertex_input_bindings                             == in_device_limits.max_vertex_input_bindings                             &&
         max_vertex_input_binding_stride                       == in_device_limits.max_vertex_input_binding_stride                       &&
         max_vertex_output_components                          == in_device_limits.max_vertex_output_components                          &&
         min_memory_map_alignment                              == in_device_limits.min_memory_map_alignment                              &&
         min_storage_buffer_offset_alignment                   == in_device_limits.min_storage_buffer_offset_alignment                   &&
         min_texel_buffer_offset_alignment                     == in_device_limits.min_texel_buffer_offset_alignment                     &&
         min_texel_gather_offset                               == in_device_limits.min_texel_gather_offset                               &&
         min_texel_offset                                      == in_device_limits.min_texel_offset                                      &&
         min_uniform_buffer_offset_alignment                   == in_device_limits.min_uniform_buffer_offset_alignment                   &&
         mipmap_precision_bits                                 == in_device_limits.mipmap_precision_bits                                 &&
         non_coherent_atom_size                                == in_device_limits.non_coherent_atom_size                                &&
         optimal_buffer_copy_offset_alignment                  == in_device_limits.optimal_buffer_copy_offset_alignment                  &&
         optimal_buffer_copy_row_pitch_alignment               == in_device_limits.optimal_buffer_copy_row_pitch_alignment               &&
         sampled_image_color_sample_counts                     == in_device_limits.sampled_image_color_sample_counts                     &&
         sampled_image_depth_sample_counts                     == in_device_limits.sampled_image_depth_sample_counts                     &&
         sampled_image_integer_sample_counts                   == in_device_limits.sampled_image_integer_sample_counts                   &&
         sampled_image_stencil_sample_counts                   == in_device_limits.sampled_image_stencil_sample_counts                   &&
         sparse_address_space_size                             == in_device_limits.sparse_address_space_size                             &&
         standard_sample_locations                             == in_device_limits.standard_sample_locations                             &&
         storage_image_sample_counts                           == in_device_limits.storage_image_sample_counts                           &&
         strict_lines                                          == in_device_limits.strict_lines                                          &&
         sub_pixel_interpolation_offset_bits                   == in_device_limits.sub_pixel_interpolation_offset_bits                   &&
         sub_pixel_precision_bits                              == in_device_limits.sub_pixel_precision_bits                              &&
         sub_texel_precision_bits                              == in_device_limits.sub_texel_precision_bits                              &&
         timestamp_compute_and_graphics                        == in_device_limits.timestamp_compute_and_graphics                        &&
         viewport_sub_pixel_bits                               == in_device_limits.viewport_sub_pixel_bits)
     {
         /* Floats */
         if (fabs(line_width_range[0]      - in_device_limits.line_width_range[0])      < 1e-5f &&
             fabs(line_width_range[1]      - in_device_limits.line_width_range[1])      < 1e-5f &&
             fabs(line_width_granularity   - in_device_limits.line_width_granularity)   < 1e-5f &&
             fabs(max_interpolation_offset - in_device_limits.max_interpolation_offset) < 1e-5f &&
             fabs(max_sampler_anisotropy   - in_device_limits.max_sampler_anisotropy)   < 1e-5f &&
             fabs(max_sampler_lod_bias     - in_device_limits.max_sampler_lod_bias)     < 1e-5f &&
             fabs(min_interpolation_offset - in_device_limits.min_interpolation_offset) < 1e-5f &&
             fabs(point_size_granularity   - in_device_limits.point_size_granularity)   < 1e-5f &&
             fabs(point_size_range[0]      - in_device_limits.point_size_range[0])      < 1e-5f &&
             fabs(point_size_range[1]      - in_device_limits.point_size_range[1])      < 1e-5f &&
             fabs(timestamp_period         - in_device_limits.timestamp_period)         < 1e-5f &&
             fabs(viewport_bounds_range[0] - in_device_limits.viewport_bounds_range[0]) < 1e-5f &&
             fabs(viewport_bounds_range[1] - in_device_limits.viewport_bounds_range[1]) < 1e-5f)
         {
             result = true;
         }
     }

    return result;
}

Anvil::PhysicalDevicePropertiesCoreVK10::PhysicalDevicePropertiesCoreVK10(const VkPhysicalDeviceProperties& in_physical_device_properties)
     :api_version      (in_physical_device_properties.apiVersion),
      device_id        (in_physical_device_properties.deviceID),
      device_type      (in_physical_device_properties.deviceType),
      driver_version   (in_physical_device_properties.driverVersion),
      limits           (PhysicalDeviceLimits          (in_physical_device_properties.limits)           ),
      sparse_properties(PhysicalDeviceSparseProperties(in_physical_device_properties.sparseProperties) ),
      vendor_id        (in_physical_device_properties.vendorID)
{
    memcpy(device_name,
           in_physical_device_properties.deviceName,
           sizeof(device_name) );
    memcpy(pipeline_cache_uuid,
           in_physical_device_properties.pipelineCacheUUID,
           sizeof(pipeline_cache_uuid) );
}

bool Anvil::PhysicalDevicePropertiesCoreVK10::operator==(const PhysicalDevicePropertiesCoreVK10& in_props) const
{
    bool result = false;

    if (in_props.api_version       == api_version       &&
        in_props.device_id         == device_id         &&
        in_props.device_type       == device_type       &&
        in_props.driver_version    == driver_version    &&
        in_props.limits            == limits            &&
        in_props.sparse_properties == sparse_properties &&
        in_props.vendor_id         == vendor_id)
    {
        if (memcmp(device_name,
                   in_props.device_name,
                   sizeof(device_name) )         == 0 &&
            memcmp(pipeline_cache_uuid,
                   in_props.pipeline_cache_uuid,
                   sizeof(pipeline_cache_uuid) ) == 0)
        {
            result = true;
        }
    }

    return result;
}

Anvil::PhysicalDeviceSparseProperties::PhysicalDeviceSparseProperties()
    :residency_standard_2D_block_shape            (false),
     residency_standard_2D_multisample_block_shape(false),
     residency_standard_3D_block_shape            (false),
     residency_aligned_mip_size                   (false),
     residency_non_resident_strict                (false)
{
    /* Stub */
}

Anvil::PhysicalDeviceSparseProperties::PhysicalDeviceSparseProperties(const VkPhysicalDeviceSparseProperties& in_sparse_props)
    :residency_standard_2D_block_shape            (VK_BOOL32_TO_BOOL(in_sparse_props.residencyStandard2DBlockShape)            ),
     residency_standard_2D_multisample_block_shape(VK_BOOL32_TO_BOOL(in_sparse_props.residencyStandard2DMultisampleBlockShape) ),
     residency_standard_3D_block_shape            (VK_BOOL32_TO_BOOL(in_sparse_props.residencyStandard3DBlockShape)            ),
     residency_aligned_mip_size                   (VK_BOOL32_TO_BOOL(in_sparse_props.residencyAlignedMipSize)                  ),
     residency_non_resident_strict                (VK_BOOL32_TO_BOOL(in_sparse_props.residencyNonResidentStrict)               )
{
    /* Stub */
}

bool Anvil::PhysicalDeviceSparseProperties::operator==(const Anvil::PhysicalDeviceSparseProperties& in_props) const
{
    return (residency_standard_2D_block_shape             == in_props.residency_standard_2D_block_shape             &&
            residency_standard_2D_multisample_block_shape == in_props.residency_standard_2D_multisample_block_shape &&
            residency_standard_3D_block_shape             == in_props.residency_standard_3D_block_shape             &&
            residency_aligned_mip_size                    == in_props.residency_aligned_mip_size                    &&
            residency_non_resident_strict                 == in_props.residency_non_resident_strict);
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
    shader_module_ptr = nullptr;
    stage             = SHADER_STAGE_UNKNOWN;
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const std::string& in_name,
                                                                ShaderModule*      in_shader_module_ptr,
                                                                ShaderStage        in_stage)
{
    anvil_assert(in_shader_module_ptr != nullptr);

    name              = in_name;
    shader_module_ptr = in_shader_module_ptr;
    stage             = in_stage;
}

Anvil::ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const std::string&    in_name,
                                                                ShaderModuleUniquePtr in_shader_module_ptr,
                                                                ShaderStage           in_stage)
{
    anvil_assert(in_shader_module_ptr != nullptr);

    name              = in_name;
    shader_module_ptr = in_shader_module_ptr.get();
    stage             = in_stage;

    shader_module_owned_ptr = std::move(in_shader_module_ptr);
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

bool Anvil::PhysicalDeviceFeatures::operator==(const PhysicalDeviceFeatures& in_physical_device_features) const
{
    const bool core_vk1_0_features_match        = (*core_vk1_0_features_ptr == *in_physical_device_features.core_vk1_0_features_ptr);
    bool       khr_16bit_storage_features_match = false;

    if (khr_16bit_storage_features_ptr                             != nullptr &&
        in_physical_device_features.khr_16bit_storage_features_ptr != nullptr)
    {
        khr_16bit_storage_features_match = (*khr_16bit_storage_features_ptr == *in_physical_device_features.khr_16bit_storage_features_ptr);
    }
    else
    {
        khr_16bit_storage_features_match = (khr_16bit_storage_features_ptr                             == nullptr &&
                                            in_physical_device_features.khr_16bit_storage_features_ptr == nullptr);
    }

    return core_vk1_0_features_match        &&
           khr_16bit_storage_features_match;
}

Anvil::PhysicalDeviceFeaturesCoreVK10::PhysicalDeviceFeaturesCoreVK10()
    :alpha_to_one                                (false),
     depth_bias_clamp                            (false),
     depth_bounds                                (false),
     depth_clamp                                 (false),
     draw_indirect_first_instance                (false),
     dual_src_blend                              (false),
     fill_mode_non_solid                         (false),
     fragment_stores_and_atomics                 (false),
     full_draw_index_uint32                      (false),
     geometry_shader                             (false),
     image_cube_array                            (false),
     independent_blend                           (false),
     inherited_queries                           (false),
     large_points                                (false),
     logic_ip                                    (false),
     multi_draw_indirect                         (false),
     multi_viewport                              (false),
     occlusion_query_precise                     (false),
     pipeline_statistics_query                   (false),
     robust_buffer_access                        (false),
     sampler_anisotropy                          (false),
     sample_rate_shading                         (false),
     shader_clip_distance                        (false),
     shader_cull_distance                        (false),
     shader_float64                              (false),
     shader_image_gather_extended                (false),
     shader_int16                                (false),
     shader_int64                                (false),
     shader_resource_residency                   (false),
     shader_resource_min_lod                     (false),
     shader_sampled_image_array_dynamic_indexing (false),
     shader_storage_buffer_array_dynamic_indexing(false),
     shader_storage_image_array_dynamic_indexing (false),
     shader_storage_image_extended_formats       (false),
     shader_storage_image_multisample            (false),
     shader_storage_image_read_without_format    (false),
     shader_storage_image_write_without_format   (false),
     shader_tessellation_and_geometry_point_size (false),
     shader_uniform_buffer_array_dynamic_indexing(false),
     sparse_binding                              (false),
     sparse_residency_2_samples                  (false),
     sparse_residency_4_samples                  (false),
     sparse_residency_8_samples                  (false),
     sparse_residency_16_samples                 (false),
     sparse_residency_aliased                    (false),
     sparse_residency_buffer                     (false),
     sparse_residency_image_2D                   (false),
     sparse_residency_image_3D                   (false),
     tessellation_shader                         (false),
     texture_compression_ASTC_LDR                (false),
     texture_compression_BC                      (false),
     texture_compression_ETC2                    (false),
     variable_multisample_rate                   (false),
     vertex_pipeline_stores_and_atomics          (false),
     wide_lines                                  (false)
{
    /* Stub */
}

Anvil::PhysicalDeviceFeaturesCoreVK10::PhysicalDeviceFeaturesCoreVK10(const VkPhysicalDeviceFeatures& in_physical_device_features)
    :alpha_to_one                                (VK_BOOL32_TO_BOOL(in_physical_device_features.alphaToOne) ),
     depth_bias_clamp                            (VK_BOOL32_TO_BOOL(in_physical_device_features.depthBiasClamp) ),
     depth_bounds                                (VK_BOOL32_TO_BOOL(in_physical_device_features.depthBounds) ),
     depth_clamp                                 (VK_BOOL32_TO_BOOL(in_physical_device_features.depthClamp) ),
     draw_indirect_first_instance                (VK_BOOL32_TO_BOOL(in_physical_device_features.drawIndirectFirstInstance) ),
     dual_src_blend                              (VK_BOOL32_TO_BOOL(in_physical_device_features.dualSrcBlend) ),
     fill_mode_non_solid                         (VK_BOOL32_TO_BOOL(in_physical_device_features.fillModeNonSolid) ),
     fragment_stores_and_atomics                 (VK_BOOL32_TO_BOOL(in_physical_device_features.fragmentStoresAndAtomics) ),
     full_draw_index_uint32                      (VK_BOOL32_TO_BOOL(in_physical_device_features.fullDrawIndexUint32) ),
     geometry_shader                             (VK_BOOL32_TO_BOOL(in_physical_device_features.geometryShader) ),
     image_cube_array                            (VK_BOOL32_TO_BOOL(in_physical_device_features.imageCubeArray) ),
     independent_blend                           (VK_BOOL32_TO_BOOL(in_physical_device_features.independentBlend) ),
     inherited_queries                           (VK_BOOL32_TO_BOOL(in_physical_device_features.inheritedQueries) ),
     large_points                                (VK_BOOL32_TO_BOOL(in_physical_device_features.largePoints) ),
     logic_ip                                    (VK_BOOL32_TO_BOOL(in_physical_device_features.logicOp) ),
     multi_draw_indirect                         (VK_BOOL32_TO_BOOL(in_physical_device_features.multiDrawIndirect) ),
     multi_viewport                              (VK_BOOL32_TO_BOOL(in_physical_device_features.multiViewport) ),
     occlusion_query_precise                     (VK_BOOL32_TO_BOOL(in_physical_device_features.occlusionQueryPrecise) ),
     pipeline_statistics_query                   (VK_BOOL32_TO_BOOL(in_physical_device_features.pipelineStatisticsQuery) ),
     robust_buffer_access                        (VK_BOOL32_TO_BOOL(in_physical_device_features.robustBufferAccess) ),
     sampler_anisotropy                          (VK_BOOL32_TO_BOOL(in_physical_device_features.samplerAnisotropy) ),
     sample_rate_shading                         (VK_BOOL32_TO_BOOL(in_physical_device_features.sampleRateShading) ),
     shader_clip_distance                        (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderClipDistance) ),
     shader_cull_distance                        (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderCullDistance) ),
     shader_float64                              (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderFloat64) ),
     shader_image_gather_extended                (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderImageGatherExtended) ),
     shader_int16                                (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderInt16) ),
     shader_int64                                (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderInt64) ),
     shader_resource_residency                   (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderResourceResidency) ),
     shader_resource_min_lod                     (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderResourceMinLod) ),
     shader_sampled_image_array_dynamic_indexing (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderSampledImageArrayDynamicIndexing) ),
     shader_storage_buffer_array_dynamic_indexing(VK_BOOL32_TO_BOOL(in_physical_device_features.shaderStorageBufferArrayDynamicIndexing) ),
     shader_storage_image_array_dynamic_indexing (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderStorageImageArrayDynamicIndexing) ),
     shader_storage_image_extended_formats       (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderStorageImageExtendedFormats) ),
     shader_storage_image_multisample            (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderStorageImageMultisample) ),
     shader_storage_image_read_without_format    (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderStorageImageReadWithoutFormat) ),
     shader_storage_image_write_without_format   (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderStorageImageWriteWithoutFormat) ),
     shader_tessellation_and_geometry_point_size (VK_BOOL32_TO_BOOL(in_physical_device_features.shaderTessellationAndGeometryPointSize) ),
     shader_uniform_buffer_array_dynamic_indexing(VK_BOOL32_TO_BOOL(in_physical_device_features.shaderUniformBufferArrayDynamicIndexing) ),
     sparse_binding                              (VK_BOOL32_TO_BOOL(in_physical_device_features.sparseBinding) ),
     sparse_residency_2_samples                  (VK_BOOL32_TO_BOOL(in_physical_device_features.sparseResidency2Samples) ),
     sparse_residency_4_samples                  (VK_BOOL32_TO_BOOL(in_physical_device_features.sparseResidency4Samples) ),
     sparse_residency_8_samples                  (VK_BOOL32_TO_BOOL(in_physical_device_features.sparseResidency8Samples) ),
     sparse_residency_16_samples                 (VK_BOOL32_TO_BOOL(in_physical_device_features.sparseResidency16Samples) ),
     sparse_residency_aliased                    (VK_BOOL32_TO_BOOL(in_physical_device_features.sparseResidencyAliased) ),
     sparse_residency_buffer                     (VK_BOOL32_TO_BOOL(in_physical_device_features.sparseResidencyBuffer) ),
     sparse_residency_image_2D                   (VK_BOOL32_TO_BOOL(in_physical_device_features.sparseResidencyImage2D) ),
     sparse_residency_image_3D                   (VK_BOOL32_TO_BOOL(in_physical_device_features.sparseResidencyImage3D) ),
     tessellation_shader                         (VK_BOOL32_TO_BOOL(in_physical_device_features.tessellationShader) ),
     texture_compression_ASTC_LDR                (VK_BOOL32_TO_BOOL(in_physical_device_features.textureCompressionASTC_LDR) ),
     texture_compression_BC                      (VK_BOOL32_TO_BOOL(in_physical_device_features.textureCompressionBC) ),
     texture_compression_ETC2                    (VK_BOOL32_TO_BOOL(in_physical_device_features.textureCompressionETC2) ),
     variable_multisample_rate                   (VK_BOOL32_TO_BOOL(in_physical_device_features.variableMultisampleRate) ),
     vertex_pipeline_stores_and_atomics          (VK_BOOL32_TO_BOOL(in_physical_device_features.vertexPipelineStoresAndAtomics) ),
     wide_lines                                  (VK_BOOL32_TO_BOOL(in_physical_device_features.wideLines) )
{
    /* Stub */
}

VkPhysicalDeviceFeatures Anvil::PhysicalDeviceFeaturesCoreVK10::get_vk_physical_device_features() const
{
    VkPhysicalDeviceFeatures result;

    result.alphaToOne                              = BOOL_TO_VK_BOOL32(alpha_to_one);
    result.depthBiasClamp                          = BOOL_TO_VK_BOOL32(depth_bias_clamp);
    result.depthBounds                             = BOOL_TO_VK_BOOL32(depth_bounds);
    result.depthClamp                              = BOOL_TO_VK_BOOL32(depth_clamp);
    result.drawIndirectFirstInstance               = BOOL_TO_VK_BOOL32(draw_indirect_first_instance);
    result.dualSrcBlend                            = BOOL_TO_VK_BOOL32(dual_src_blend);
    result.fillModeNonSolid                        = BOOL_TO_VK_BOOL32(fill_mode_non_solid);
    result.fragmentStoresAndAtomics                = BOOL_TO_VK_BOOL32(fragment_stores_and_atomics);
    result.fullDrawIndexUint32                     = BOOL_TO_VK_BOOL32(full_draw_index_uint32);
    result.geometryShader                          = BOOL_TO_VK_BOOL32(geometry_shader);
    result.imageCubeArray                          = BOOL_TO_VK_BOOL32(image_cube_array);
    result.independentBlend                        = BOOL_TO_VK_BOOL32(independent_blend);
    result.inheritedQueries                        = BOOL_TO_VK_BOOL32(inherited_queries);
    result.largePoints                             = BOOL_TO_VK_BOOL32(large_points);
    result.logicOp                                 = BOOL_TO_VK_BOOL32(logic_ip);
    result.multiDrawIndirect                       = BOOL_TO_VK_BOOL32(multi_draw_indirect);
    result.multiViewport                           = BOOL_TO_VK_BOOL32(multi_viewport);
    result.occlusionQueryPrecise                   = BOOL_TO_VK_BOOL32(occlusion_query_precise);
    result.pipelineStatisticsQuery                 = BOOL_TO_VK_BOOL32(pipeline_statistics_query);
    result.robustBufferAccess                      = BOOL_TO_VK_BOOL32(robust_buffer_access);
    result.samplerAnisotropy                       = BOOL_TO_VK_BOOL32(sampler_anisotropy);
    result.sampleRateShading                       = BOOL_TO_VK_BOOL32(sample_rate_shading);
    result.shaderClipDistance                      = BOOL_TO_VK_BOOL32(shader_clip_distance);
    result.shaderCullDistance                      = BOOL_TO_VK_BOOL32(shader_cull_distance);
    result.shaderFloat64                           = BOOL_TO_VK_BOOL32(shader_float64);
    result.shaderImageGatherExtended               = BOOL_TO_VK_BOOL32(shader_image_gather_extended);
    result.shaderInt16                             = BOOL_TO_VK_BOOL32(shader_int16);
    result.shaderInt64                             = BOOL_TO_VK_BOOL32(shader_int64);
    result.shaderResourceResidency                 = BOOL_TO_VK_BOOL32(shader_resource_residency);
    result.shaderResourceMinLod                    = BOOL_TO_VK_BOOL32(shader_resource_min_lod);
    result.shaderSampledImageArrayDynamicIndexing  = BOOL_TO_VK_BOOL32(shader_sampled_image_array_dynamic_indexing);
    result.shaderStorageBufferArrayDynamicIndexing = BOOL_TO_VK_BOOL32(shader_storage_buffer_array_dynamic_indexing);
    result.shaderStorageImageArrayDynamicIndexing  = BOOL_TO_VK_BOOL32(shader_storage_image_array_dynamic_indexing);
    result.shaderStorageImageExtendedFormats       = BOOL_TO_VK_BOOL32(shader_storage_image_extended_formats);
    result.shaderStorageImageMultisample           = BOOL_TO_VK_BOOL32(shader_storage_image_multisample);
    result.shaderStorageImageReadWithoutFormat     = BOOL_TO_VK_BOOL32(shader_storage_image_read_without_format);
    result.shaderStorageImageWriteWithoutFormat    = BOOL_TO_VK_BOOL32(shader_storage_image_write_without_format);
    result.shaderTessellationAndGeometryPointSize  = BOOL_TO_VK_BOOL32(shader_tessellation_and_geometry_point_size);
    result.shaderUniformBufferArrayDynamicIndexing = BOOL_TO_VK_BOOL32(shader_uniform_buffer_array_dynamic_indexing);
    result.sparseBinding                           = BOOL_TO_VK_BOOL32(sparse_binding);
    result.sparseResidency2Samples                 = BOOL_TO_VK_BOOL32(sparse_residency_2_samples);
    result.sparseResidency4Samples                 = BOOL_TO_VK_BOOL32(sparse_residency_4_samples);
    result.sparseResidency8Samples                 = BOOL_TO_VK_BOOL32(sparse_residency_8_samples);
    result.sparseResidency16Samples                = BOOL_TO_VK_BOOL32(sparse_residency_16_samples);
    result.sparseResidencyAliased                  = BOOL_TO_VK_BOOL32(sparse_residency_aliased);
    result.sparseResidencyBuffer                   = BOOL_TO_VK_BOOL32(sparse_residency_buffer);
    result.sparseResidencyImage2D                  = BOOL_TO_VK_BOOL32(sparse_residency_image_2D);
    result.sparseResidencyImage3D                  = BOOL_TO_VK_BOOL32(sparse_residency_image_3D);
    result.tessellationShader                      = BOOL_TO_VK_BOOL32(tessellation_shader);
    result.textureCompressionASTC_LDR              = BOOL_TO_VK_BOOL32(texture_compression_ASTC_LDR);
    result.textureCompressionBC                    = BOOL_TO_VK_BOOL32(texture_compression_BC);
    result.textureCompressionETC2                  = BOOL_TO_VK_BOOL32(texture_compression_ETC2);
    result.variableMultisampleRate                 = BOOL_TO_VK_BOOL32(variable_multisample_rate);
    result.vertexPipelineStoresAndAtomics          = BOOL_TO_VK_BOOL32(vertex_pipeline_stores_and_atomics);
    result.wideLines                               = BOOL_TO_VK_BOOL32(wide_lines);

    return result;
}

bool Anvil::PhysicalDeviceFeaturesCoreVK10::operator==(const Anvil::PhysicalDeviceFeaturesCoreVK10& in_data) const
{
    return (alpha_to_one                                 == in_data.alpha_to_one)                                 &&
           (depth_bias_clamp                             == in_data.depth_bias_clamp)                             &&
           (depth_bounds                                 == in_data.depth_bounds)                                 &&
           (depth_clamp                                  == in_data.depth_clamp)                                  &&
           (draw_indirect_first_instance                 == in_data.draw_indirect_first_instance)                 &&
           (dual_src_blend                               == in_data.dual_src_blend)                               &&
           (fill_mode_non_solid                          == in_data.fill_mode_non_solid)                          &&
           (fragment_stores_and_atomics                  == in_data.fragment_stores_and_atomics)                  &&
           (full_draw_index_uint32                       == in_data.full_draw_index_uint32)                       &&
           (geometry_shader                              == in_data.geometry_shader)                              &&
           (image_cube_array                             == in_data.image_cube_array)                             &&
           (independent_blend                            == in_data.independent_blend)                            &&
           (inherited_queries                            == in_data.inherited_queries)                            &&
           (large_points                                 == in_data.large_points)                                 &&
           (logic_ip                                     == in_data.logic_ip)                                     &&
           (multi_draw_indirect                          == in_data.multi_draw_indirect)                          &&
           (multi_viewport                               == in_data.multi_viewport)                               &&
           (occlusion_query_precise                      == in_data.occlusion_query_precise)                      &&
           (pipeline_statistics_query                    == in_data.pipeline_statistics_query)                    &&
           (robust_buffer_access                         == in_data.robust_buffer_access)                         &&
           (sampler_anisotropy                           == in_data.sampler_anisotropy)                           &&
           (sample_rate_shading                          == in_data.sample_rate_shading)                          &&
           (shader_clip_distance                         == in_data.shader_clip_distance)                         &&
           (shader_cull_distance                         == in_data.shader_cull_distance)                         &&
           (shader_float64                               == in_data.shader_float64)                               &&
           (shader_image_gather_extended                 == in_data.shader_image_gather_extended)                 &&
           (shader_int16                                 == in_data.shader_int16)                                 &&
           (shader_int64                                 == in_data.shader_int64)                                 &&
           (shader_resource_residency                    == in_data.shader_resource_residency)                    &&
           (shader_resource_min_lod                      == in_data.shader_resource_min_lod)                      &&
           (shader_sampled_image_array_dynamic_indexing  == in_data.shader_sampled_image_array_dynamic_indexing)  &&
           (shader_storage_buffer_array_dynamic_indexing == in_data.shader_storage_buffer_array_dynamic_indexing) &&
           (shader_storage_image_array_dynamic_indexing  == in_data.shader_storage_image_array_dynamic_indexing)  &&
           (shader_storage_image_extended_formats        == in_data.shader_storage_image_extended_formats)        &&
           (shader_storage_image_multisample             == in_data.shader_storage_image_multisample)             &&
           (shader_storage_image_read_without_format     == in_data.shader_storage_image_read_without_format)     &&
           (shader_storage_image_write_without_format    == in_data.shader_storage_image_write_without_format)    &&
           (shader_tessellation_and_geometry_point_size  == in_data.shader_tessellation_and_geometry_point_size)  &&
           (shader_uniform_buffer_array_dynamic_indexing == in_data.shader_uniform_buffer_array_dynamic_indexing) &&
           (sparse_binding                               == in_data.sparse_binding)                               &&
           (sparse_residency_2_samples                   == in_data.sparse_residency_2_samples)                   &&
           (sparse_residency_4_samples                   == in_data.sparse_residency_4_samples)                   &&
           (sparse_residency_8_samples                   == in_data.sparse_residency_8_samples)                   &&
           (sparse_residency_16_samples                  == in_data.sparse_residency_16_samples)                  &&
           (sparse_residency_aliased                     == in_data.sparse_residency_aliased)                     &&
           (sparse_residency_buffer                      == in_data.sparse_residency_buffer)                      &&
           (sparse_residency_image_2D                    == in_data.sparse_residency_image_2D)                    &&
           (sparse_residency_image_3D                    == in_data.sparse_residency_image_3D)                    &&
           (tessellation_shader                          == in_data.tessellation_shader)                          &&
           (texture_compression_ASTC_LDR                 == in_data.texture_compression_ASTC_LDR)                 &&
           (texture_compression_BC                       == in_data.texture_compression_BC)                       &&
           (texture_compression_ETC2                     == in_data.texture_compression_ETC2)                     &&
           (variable_multisample_rate                    == in_data.variable_multisample_rate)                    &&
           (vertex_pipeline_stores_and_atomics           == in_data.vertex_pipeline_stores_and_atomics)           &&
           (wide_lines                                   == in_data.wide_lines);
}

bool Anvil::PhysicalDeviceProperties::operator==(const PhysicalDeviceProperties& in_props) const
{
    const bool core_vk1_0_features_match         = (*core_vk1_0_properties_ptr == *in_props.core_vk1_0_properties_ptr);
    bool       khr_maintenance3_properties_match = false;

    if (khr_maintenance3_properties_ptr          != nullptr &&
        in_props.khr_maintenance3_properties_ptr != nullptr)
    {
        khr_maintenance3_properties_match = (*khr_maintenance3_properties_ptr == *in_props.khr_maintenance3_properties_ptr);
    }
    else
    {
        khr_maintenance3_properties_match = (khr_maintenance3_properties_ptr          == nullptr &&
                                             in_props.khr_maintenance3_properties_ptr == nullptr);
    }

    return core_vk1_0_features_match         &&
           khr_maintenance3_properties_match;
}


/** Please see header for specification */
Anvil::SparseMemoryBindInfoID Anvil::Utils::SparseMemoryBindingUpdateInfo::add_bind_info(uint32_t                 in_n_signal_semaphores,
                                                                                         Anvil::Semaphore* const* in_opt_signal_semaphores_ptr,
                                                                                         uint32_t                 in_n_wait_semaphores,
                                                                                         Anvil::Semaphore* const* in_opt_wait_semaphores_ptr)
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
void Anvil::Utils::SparseMemoryBindingUpdateInfo::append_buffer_memory_update(SparseMemoryBindInfoID in_bind_info_id,
                                                                              Anvil::Buffer*         in_buffer_ptr,
                                                                              VkDeviceSize           in_buffer_memory_start_offset,
                                                                              Anvil::MemoryBlock*    in_memory_block_ptr,
                                                                              VkDeviceSize           in_memory_block_start_offset,
                                                                              bool                   in_memory_block_owned_by_buffer,
                                                                              VkDeviceSize           in_size)
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

    update.memory_block_owned_by_target = in_memory_block_owned_by_buffer;
    update.memory_block_ptr             = in_memory_block_ptr;
    update.memory_block_start_offset    = in_memory_block_start_offset;
    update.size                         = in_size;
    update.start_offset                 = in_buffer_memory_start_offset;

    update_vk.flags                     = 0;
    update_vk.memory                    = (in_memory_block_ptr != nullptr) ? in_memory_block_ptr->get_memory()
                                                                           : VK_NULL_HANDLE;
    update_vk.memoryOffset              = (in_memory_block_ptr != nullptr) ? (in_memory_block_ptr->get_start_offset() + in_memory_block_start_offset)
                                                                           : UINT32_MAX;
    update_vk.resourceOffset            = (in_buffer_ptr->get_start_offset() + in_buffer_memory_start_offset);
    update_vk.size                      = in_size;

    binding.buffer_updates[in_buffer_ptr].first.push_back (update);
    binding.buffer_updates[in_buffer_ptr].second.push_back(update_vk);
}

/** Please see header for specification */
void Anvil::Utils::SparseMemoryBindingUpdateInfo::append_image_memory_update(SparseMemoryBindInfoID    in_bind_info_id,
                                                                             Anvil::Image*             in_image_ptr,
                                                                             const VkImageSubresource& in_subresource,
                                                                             const VkOffset3D&         in_offset,
                                                                             const VkExtent3D&         in_extent,
                                                                             VkSparseMemoryBindFlags   in_flags,
                                                                             Anvil::MemoryBlock*       in_opt_memory_block_ptr,
                                                                             VkDeviceSize              in_opt_memory_block_start_offset,
                                                                             bool                      in_opt_memory_block_owned_by_image)
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

    update.extent                      = in_extent;
    update.flags                       = in_flags;
    update.memory_block_owned_by_image = in_opt_memory_block_owned_by_image;
    update.memory_block_ptr            = in_opt_memory_block_ptr;
    update.memory_block_start_offset   = in_opt_memory_block_start_offset;
    update.offset                      = in_offset;
    update.subresource                 = in_subresource;

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
void Anvil::Utils::SparseMemoryBindingUpdateInfo::append_opaque_image_memory_update(SparseMemoryBindInfoID  in_bind_info_id,
                                                                                    Anvil::Image*           in_image_ptr,
                                                                                    VkDeviceSize            in_resource_offset,
                                                                                    VkDeviceSize            in_size,
                                                                                    VkSparseMemoryBindFlags in_flags,
                                                                                    Anvil::MemoryBlock*     in_opt_memory_block_ptr,
                                                                                    VkDeviceSize            in_opt_memory_block_start_offset,
                                                                                    bool                    in_opt_memory_block_owned_by_image)
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

    update.flags                        = in_flags;
    update.memory_block_owned_by_target = in_opt_memory_block_owned_by_image;
    update.memory_block_ptr             = in_opt_memory_block_ptr;
    update.memory_block_start_offset    = in_opt_memory_block_start_offset;
    update.size                         = in_size;
    update.start_offset                 = in_resource_offset;

    update_vk.flags                     = in_flags;
    update_vk.memory                    = (in_opt_memory_block_ptr != nullptr) ? in_opt_memory_block_ptr->get_memory()
                                                                               : VK_NULL_HANDLE;
    update_vk.memoryOffset              = (in_opt_memory_block_ptr != nullptr) ? (in_opt_memory_block_ptr->get_start_offset() + in_opt_memory_block_start_offset)
                                                                               : UINT32_MAX;
    update_vk.resourceOffset            = in_resource_offset;
    update_vk.size                      = in_size;

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
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_bind_info_properties(SparseMemoryBindInfoID in_bind_info_id,
                                                                           uint32_t* const        out_opt_n_buffer_memory_updates_ptr,
                                                                           uint32_t* const        out_opt_n_image_memory_updates_ptr,
                                                                           uint32_t* const        out_opt_n_image_opaque_memory_updates_ptr,
                                                                           uint32_t* const        out_opt_n_signal_semaphores_ptr,
                                                                           Anvil::Semaphore***    out_opt_signal_semaphores_ptr_ptr_ptr,
                                                                           uint32_t* const        out_opt_n_wait_semaphores_ptr,
                                                                           Anvil::Semaphore***    out_opt_wait_semaphores_ptr_ptr_ptr)
{
    decltype(m_bindings)::iterator binding_iterator;
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

    if (out_opt_signal_semaphores_ptr_ptr_ptr      != nullptr &&
        binding_iterator->signal_semaphores.size() >  0)
    {
        *out_opt_signal_semaphores_ptr_ptr_ptr = &binding_iterator->signal_semaphores.at(0);
    }

    if (out_opt_n_wait_semaphores_ptr != nullptr)
    {
        *out_opt_n_wait_semaphores_ptr = static_cast<uint32_t>(binding_iterator->wait_semaphores.size() );
    }

    if (out_opt_wait_semaphores_ptr_ptr_ptr      != nullptr &&
        binding_iterator->wait_semaphores.size() >  0)
    {
        *out_opt_wait_semaphores_ptr_ptr_ptr = &binding_iterator->wait_semaphores.at(0);
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
void Anvil::Utils::SparseMemoryBindingUpdateInfo::get_bind_sparse_call_args(uint32_t*                out_bind_info_count_ptr,
                                                                            const VkBindSparseInfo** out_bind_info_ptr,
                                                                            Anvil::Fence**           out_fence_to_set_ptr)
{
    if (m_dirty)
    {
        bake();

        anvil_assert(!m_dirty);
    }

    *out_bind_info_count_ptr = static_cast<uint32_t>(m_bindings.size() );
    *out_bind_info_ptr       = (m_bindings_vk.size() > 0) ? &m_bindings_vk.at(0) : nullptr;
    *out_fence_to_set_ptr    = m_fence_ptr;
}

/** Please see header for specification */
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_buffer_memory_update_properties(SparseMemoryBindInfoID in_bind_info_id,
                                                                                      uint32_t               in_n_update,
                                                                                      Anvil::Buffer**        out_opt_buffer_ptr_ptr,
                                                                                      VkDeviceSize*          out_opt_buffer_memory_start_offset_ptr,
                                                                                      Anvil::MemoryBlock**   out_opt_memory_block_ptr_ptr,
                                                                                      VkDeviceSize*          out_opt_memory_block_start_offset_ptr,
                                                                                      bool*                  out_opt_memory_block_owned_by_buffer_ptr,
                                                                                      VkDeviceSize*          out_opt_size_ptr) const
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

    if (out_opt_buffer_ptr_ptr != nullptr)
    {
        *out_opt_buffer_ptr_ptr = buffer_binding_map_iterator->first;
    }

    if (out_opt_buffer_memory_start_offset_ptr != nullptr)
    {
        *out_opt_buffer_memory_start_offset_ptr = buffer_bind.start_offset;
    }

    if (out_opt_memory_block_owned_by_buffer_ptr != nullptr)
    {
        *out_opt_memory_block_owned_by_buffer_ptr = buffer_bind.memory_block_owned_by_target;
    }

    if (out_opt_memory_block_ptr_ptr != nullptr)
    {
        *out_opt_memory_block_ptr_ptr = buffer_bind.memory_block_ptr;
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
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_image_memory_update_properties(SparseMemoryBindInfoID   in_bind_info_id,
                                                                                     uint32_t                 in_n_update,
                                                                                     Anvil::Image**           out_opt_image_ptr_ptr,
                                                                                     VkImageSubresource*      out_opt_subresource_ptr,
                                                                                     VkOffset3D*              out_opt_offset_ptr,
                                                                                     VkExtent3D*              out_opt_extent_ptr,
                                                                                     VkSparseMemoryBindFlags* out_opt_flags_ptr,
                                                                                     Anvil::MemoryBlock**     out_opt_memory_block_ptr_ptr,
                                                                                     VkDeviceSize*            out_opt_memory_block_start_offset_ptr,
                                                                                     bool*                    out_opt_memory_block_owned_by_image_ptr) const
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

    if (out_opt_memory_block_owned_by_image_ptr != nullptr)
    {
        *out_opt_memory_block_owned_by_image_ptr = image_bind.memory_block_owned_by_image;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::Utils::SparseMemoryBindingUpdateInfo::get_image_opaque_memory_update_properties(SparseMemoryBindInfoID   in_bind_info_id,
                                                                                            uint32_t                 in_n_update,
                                                                                            Anvil::Image**           out_opt_image_ptr_ptr,
                                                                                            VkDeviceSize*            out_opt_resource_offset_ptr,
                                                                                            VkDeviceSize*            out_opt_size_ptr,
                                                                                            VkSparseMemoryBindFlags* out_opt_flags_ptr,
                                                                                            Anvil::MemoryBlock**     out_opt_memory_block_ptr_ptr,
                                                                                            VkDeviceSize*            out_opt_memory_block_start_offset_ptr,
                                                                                            bool*                    out_opt_memory_block_owned_by_image_ptr) const
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

    if (out_opt_memory_block_owned_by_image_ptr != nullptr)
    {
        *out_opt_memory_block_owned_by_image_ptr = image_opaque_bind.memory_block_owned_by_target;
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

Anvil::MTSafety Anvil::Utils::convert_boolean_to_mt_safety_enum(bool in_mt_safe)
{
    return (in_mt_safe) ? MT_SAFETY_ENABLED
                        : MT_SAFETY_DISABLED;
}

bool Anvil::Utils::convert_mt_safety_enum_to_boolean(Anvil::MTSafety          in_mt_safety,
                                                     const Anvil::BaseDevice* in_device_ptr)
{
    bool result = false;

    switch (in_mt_safety)
    {
        case MT_SAFETY_DISABLED: result = false; break;
        case MT_SAFETY_ENABLED:  result = true;  break;

        case MT_SAFETY_INHERIT_FROM_PARENT_DEVICE:
        {
            anvil_assert(in_device_ptr != nullptr);

            result = in_device_ptr->is_mt_safe();
            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/** Please see header for specification */
void Anvil::Utils::convert_queue_family_bits_to_family_indices(const Anvil::BaseDevice* in_device_ptr,
                                                               Anvil::QueueFamilyBits   in_queue_families,
                                                               uint32_t*                out_opt_queue_family_indices_ptr,
                                                               uint32_t*                out_opt_n_queue_family_indices_ptr)
{
    uint32_t n_result_queue_family_indices(0);

    static const struct
    {
        Anvil::QueueFamily     queue_family;
        Anvil::QueueFamilyType queue_family_type;
    } queue_family_data[] =
    {
        {Anvil::QUEUE_FAMILY_COMPUTE_BIT,           Anvil::QueueFamilyType::COMPUTE},
        {Anvil::QUEUE_FAMILY_DMA_BIT,               Anvil::QueueFamilyType::TRANSFER},
        {Anvil::QUEUE_FAMILY_GRAPHICS_BIT,          Anvil::QueueFamilyType::UNIVERSAL},
    };

    for (const auto& current_queue_fam_data : queue_family_data)
    {
        if ((in_queue_families & current_queue_fam_data.queue_family) != 0)
        {
            uint32_t        n_queue_family_indices   = 0;
            const uint32_t* queue_family_indices_ptr = nullptr;

            in_device_ptr->get_queue_family_indices_for_queue_family_type(current_queue_fam_data.queue_family_type,
                                                                         &n_queue_family_indices,
                                                                         &queue_family_indices_ptr);

            if (out_opt_queue_family_indices_ptr != nullptr)
            {
                for (uint32_t n_queue_family_index = 0;
                              n_queue_family_index < n_queue_family_indices;
                            ++n_queue_family_index, ++n_result_queue_family_indices)
                {
                    out_opt_queue_family_indices_ptr[n_result_queue_family_indices] = queue_family_indices_ptr[n_queue_family_index];
                }
            }
            else
            {
                n_result_queue_family_indices += n_queue_family_indices;
            }
        }
    }

    if (out_opt_n_queue_family_indices_ptr != nullptr)
    {
        *out_opt_n_queue_family_indices_ptr = n_result_queue_family_indices;
    }
}

/** Please see header for specification */
VkAccessFlags Anvil::Utils::get_access_mask_from_image_layout(VkImageLayout          in_layout,
                                                              Anvil::QueueFamilyType in_queue_family_type)
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

    switch (in_queue_family_type)
    {
        case Anvil::QueueFamilyType::COMPUTE:
        {
            result &= (VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                       VK_ACCESS_MEMORY_READ_BIT           |
                       VK_ACCESS_MEMORY_WRITE_BIT          |
                       VK_ACCESS_SHADER_READ_BIT           |
                       VK_ACCESS_SHADER_WRITE_BIT          |
                       VK_ACCESS_TRANSFER_READ_BIT         |
                       VK_ACCESS_TRANSFER_WRITE_BIT        |
                       VK_ACCESS_UNIFORM_READ_BIT);

            break;
        }

        case Anvil::QueueFamilyType::TRANSFER:
        {
            result &= (VK_ACCESS_MEMORY_READ_BIT    |
                       VK_ACCESS_MEMORY_WRITE_BIT   |
                       VK_ACCESS_TRANSFER_READ_BIT  |
                       VK_ACCESS_TRANSFER_WRITE_BIT);

            break;
        }

        case Anvil::QueueFamilyType::UNIVERSAL:
        {
            result &= (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT          |
                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT         |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT  |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                       VK_ACCESS_INDIRECT_COMMAND_READ_BIT          |
                       VK_ACCESS_INDEX_READ_BIT                     |
                       VK_ACCESS_MEMORY_READ_BIT                    |
                       VK_ACCESS_MEMORY_WRITE_BIT                   |
                       VK_ACCESS_SHADER_READ_BIT                    |
                       VK_ACCESS_SHADER_WRITE_BIT                   |
                       VK_ACCESS_TRANSFER_READ_BIT                  |
                       VK_ACCESS_TRANSFER_WRITE_BIT                 |
                       VK_ACCESS_UNIFORM_READ_BIT                   |
                       VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

            break;
        }

        case Anvil::QueueFamilyType::UNDEFINED:
        {
            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
Anvil::QueueFamilyBits Anvil::Utils::get_queue_family_bits_from_queue_family_type(Anvil::QueueFamilyType in_queue_family_type)
{
    Anvil::QueueFamilyBits result = 0;

    switch (in_queue_family_type)
    {
        case Anvil::QueueFamilyType::COMPUTE:   result = Anvil::QUEUE_FAMILY_COMPUTE_BIT;                                    break;
        case Anvil::QueueFamilyType::TRANSFER:  result = Anvil::QUEUE_FAMILY_DMA_BIT;                                        break;
        case Anvil::QueueFamilyType::UNIVERSAL: result = Anvil::QUEUE_FAMILY_COMPUTE_BIT | Anvil::QUEUE_FAMILY_GRAPHICS_BIT; break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(Anvil::QueueFamilyType in_queue_family_type)
{
    static const char* result_strings[] =
    {
        "Compute",
        "Transfer",
        "Universal",
    };
    static const uint32_t n_result_strings = sizeof(result_strings) / sizeof(result_strings[0]);

    static_assert(n_result_strings == static_cast<uint32_t>(Anvil::QueueFamilyType::COUNT), "");

    return result_strings[static_cast<uint32_t>(in_queue_family_type)];
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
const char* Anvil::Utils::get_raw_string(Anvil::ShaderStage in_shader_stage)
{
    const char* result = "?";

    switch (in_shader_stage)
    {
        case SHADER_STAGE_COMPUTE:                 result = "SHADER_STAGE_COMPUTE";                 break;
        case SHADER_STAGE_FRAGMENT:                result = "SHADER_STAGE_FRAGMENT";                break;
        case SHADER_STAGE_GEOMETRY:                result = "SHADER_STAGE_GEOMETRY";                break;
        case SHADER_STAGE_TESSELLATION_CONTROL:    result = "SHADER_STAGE_TESSELLATION_CONTROL";    break;
        case SHADER_STAGE_TESSELLATION_EVALUATION: result = "SHADER_STAGE_TESSELLATION_EVALUATION"; break;
        case SHADER_STAGE_VERTEX:                  result = "SHADER_STAGE_VERTEX";                  break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
const char* Anvil::Utils::get_raw_string(VkShaderStageFlagBits in_shader_stage)
{
    const char* result = "?";

    switch (in_shader_stage)
    {
        case VK_SHADER_STAGE_ALL_GRAPHICS:                result = "VK_SHADER_STAGE_ALL_GRAPHICS";                break;
        case VK_SHADER_STAGE_COMPUTE_BIT:                 result = "VK_SHADER_STAGE_COMPUTE_BIT";                 break;
        case VK_SHADER_STAGE_FRAGMENT_BIT:                result = "VK_SHADER_STAGE_FRAGMENT_BIT";                break;
        case VK_SHADER_STAGE_GEOMETRY_BIT:                result = "VK_SHADER_STAGE_GEOMETRY_BIT";                break;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    result = "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT";    break;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: result = "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT"; break;
        case VK_SHADER_STAGE_VERTEX_BIT:                  result = "VK_SHADER_STAGE_VERTEX_BIT";                  break;

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
VkShaderStageFlagBits Anvil::Utils::get_shader_stage_flag_bits_from_shader_stage(Anvil::ShaderStage in_shader_stage)
{
    VkShaderStageFlagBits result = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

    switch (in_shader_stage)
    {
        case Anvil::ShaderStage::SHADER_STAGE_COMPUTE:                 result = VK_SHADER_STAGE_COMPUTE_BIT;                 break;
        case Anvil::ShaderStage::SHADER_STAGE_FRAGMENT:                result = VK_SHADER_STAGE_FRAGMENT_BIT;                break;
        case Anvil::ShaderStage::SHADER_STAGE_GEOMETRY:                result = VK_SHADER_STAGE_GEOMETRY_BIT;                break;
        case Anvil::ShaderStage::SHADER_STAGE_TESSELLATION_CONTROL:    result = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;    break;
        case Anvil::ShaderStage::SHADER_STAGE_TESSELLATION_EVALUATION: result = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; break;
        case Anvil::ShaderStage::SHADER_STAGE_VERTEX:                  result = VK_SHADER_STAGE_VERTEX_BIT;                  break;

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
