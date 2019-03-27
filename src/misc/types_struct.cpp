//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/descriptor_set_create_info.h"
#include "misc/formats.h"
#include "misc/image_create_info.h"
#include "misc/types.h"
#include "wrappers/buffer.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/image.h"
#include <cfloat>
#include <cmath>

#define BOOL_TO_VK_BOOL32(x) ((x)             ? VK_TRUE : VK_FALSE);
#define VK_BOOL32_TO_BOOL(x) ((x == VK_FALSE) ? false   : true)

#ifdef max
    #undef max
#endif

Anvil::AMDShaderCoreProperties::AMDShaderCoreProperties()
{
    memset(this,
           0,
           sizeof(*this));
}

Anvil::AMDShaderCoreProperties::AMDShaderCoreProperties(const VkPhysicalDeviceShaderCorePropertiesAMD& in_props)
{
    compute_units_per_shader_array = in_props.computeUnitsPerShaderArray;
    max_sgpr_allocation            = in_props.maxSgprAllocation;
    max_vgpr_allocation            = in_props.maxVgprAllocation;
    min_sgpr_allocation            = in_props.minSgprAllocation;
    min_vgpr_allocation            = in_props.minVgprAllocation;
    shader_arrays_per_engine_count = in_props.shaderArraysPerEngineCount;
    shader_engine_count            = in_props.shaderEngineCount;
    sgpr_allocation_granularity    = in_props.sgprAllocationGranularity;
    sgprs_per_simd                 = in_props.sgprsPerSimd;
    simd_per_compute_unit          = in_props.simdPerComputeUnit;
    vgpr_allocation_granularity    = in_props.vgprAllocationGranularity;
    vgprs_per_simd                 = in_props.vgprsPerSimd;
    wavefronts_per_simd            = in_props.wavefrontsPerSimd;
    wavefront_size                 = in_props.wavefrontSize;
}

bool Anvil::AMDShaderCoreProperties::operator==(const Anvil::AMDShaderCoreProperties& in_props) const
{
    return (compute_units_per_shader_array == in_props.compute_units_per_shader_array &&
            max_sgpr_allocation            == in_props.max_sgpr_allocation            &&
            max_vgpr_allocation            == in_props.max_vgpr_allocation            &&
            min_sgpr_allocation            == in_props.min_sgpr_allocation            &&
            min_vgpr_allocation            == in_props.min_vgpr_allocation            &&
            sgprs_per_simd                 == in_props.sgprs_per_simd                 &&
            sgpr_allocation_granularity    == in_props.sgpr_allocation_granularity    &&
            shader_arrays_per_engine_count == in_props.shader_arrays_per_engine_count &&
            shader_engine_count            == in_props.shader_engine_count            &&
            simd_per_compute_unit          == in_props.simd_per_compute_unit          &&
            wavefronts_per_simd            == in_props.wavefronts_per_simd            &&
            wavefront_size                 == in_props.wavefront_size                 &&
            vgpr_allocation_granularity    == in_props.vgpr_allocation_granularity    &&
            vgprs_per_simd                 == in_props.vgprs_per_simd);
}

/** Please see header for specification */
Anvil::BufferBarrier::BufferBarrier(const BufferBarrier& in)
{
    buffer                 = in.buffer;
    buffer_ptr             = in.buffer_ptr;
    dst_access_mask        = in.dst_access_mask;
    dst_queue_family_index = in.dst_queue_family_index;
    offset                 = in.offset;
    size                   = in.size;
    src_access_mask        = in.src_access_mask;
    src_queue_family_index = in.src_queue_family_index;
}

/** Please see header for specification */
Anvil::BufferBarrier::BufferBarrier(Anvil::AccessFlags in_source_access_mask,
                                    Anvil::AccessFlags in_destination_access_mask,
                                    uint32_t           in_src_queue_family_index,
                                    uint32_t           in_dst_queue_family_index,
                                    Anvil::Buffer*     in_buffer_ptr,
                                    VkDeviceSize       in_offset,
                                    VkDeviceSize       in_size)
{
    buffer                 = in_buffer_ptr->get_buffer();
    buffer_ptr             = in_buffer_ptr;
    dst_access_mask        = in_destination_access_mask;
    dst_queue_family_index = in_dst_queue_family_index;
    offset                 = in_offset;
    size                   = in_size;
    src_access_mask        = in_source_access_mask;
    src_queue_family_index = in_src_queue_family_index;

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

bool Anvil::BufferBarrier::operator==(const Anvil::BufferBarrier& in_barrier) const
{
    return (dst_access_mask        == in_barrier.dst_access_mask        &&
            src_access_mask        == in_barrier.src_access_mask        &&
            buffer_ptr             == in_barrier.buffer_ptr             &&
            dst_queue_family_index == in_barrier.dst_queue_family_index &&
            offset                 == in_barrier.offset                 &&
            size                   == in_barrier.size                   &&
            src_queue_family_index == in_barrier.src_queue_family_index);
}

VkBufferMemoryBarrier Anvil::BufferBarrier::get_barrier_vk() const
{
    VkBufferMemoryBarrier result;

    result.buffer              = buffer_ptr->get_buffer();
    result.dstAccessMask       = dst_access_mask.get_vk();
    result.dstQueueFamilyIndex = dst_queue_family_index;
    result.offset              = offset;
    result.pNext               = nullptr;
    result.size                = size;
    result.srcAccessMask       = src_access_mask.get_vk(),
    result.srcQueueFamilyIndex = src_queue_family_index;
    result.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

    return result;
}

Anvil::BufferProperties::BufferProperties()
{
    /* Stub */
}

Anvil::BufferProperties::BufferProperties(const ExternalMemoryProperties& in_external_handle_properties)
    :external_handle_properties (in_external_handle_properties)
{
    /* Stub */
}


Anvil::BufferMemoryBindingUpdate::BufferMemoryBindingUpdate()
{
    buffer_ptr                   = nullptr;
    memory_block_owned_by_buffer = false;
    memory_block_ptr             = nullptr;
}

Anvil::DebugObjectNameInfo::DebugObjectNameInfo(const VkDebugUtilsObjectNameInfoEXT& in_name_info_vk)
{
    object_handle   = in_name_info_vk.objectHandle;
    object_name_ptr = in_name_info_vk.pObjectName;
    object_type     = Anvil::Utils::get_object_type_for_vk_object_type(in_name_info_vk.objectType);
}

Anvil::DescriptorSetAllocation::DescriptorSetAllocation(const Anvil::DescriptorSetLayout* in_ds_layout_ptr)
{
    anvil_assert( in_ds_layout_ptr                                                                  != nullptr);
    anvil_assert(!in_ds_layout_ptr->get_create_info()->contains_variable_descriptor_count_binding() );

    ds_layout_ptr                  = in_ds_layout_ptr;
    n_variable_descriptor_bindings = UINT32_MAX;
}

/* Constructor.
 *
 * Use if you need to allocate a descriptor set using a descriptor set layout which
 * CONTAINS a variable count descriptor binding.
 */
Anvil::DescriptorSetAllocation::DescriptorSetAllocation(const Anvil::DescriptorSetLayout* in_ds_layout_ptr,
                                                        const uint32_t&                   in_n_variable_descriptor_bindings)
{
    anvil_assert(in_ds_layout_ptr != nullptr);

    uint32_t   binding_array_size = 0;
    uint32_t   binding_index      = UINT32_MAX;
    const auto ds_create_info_ptr = in_ds_layout_ptr->get_create_info();

    ds_create_info_ptr->contains_variable_descriptor_count_binding(&binding_index);
    anvil_assert(binding_index != UINT32_MAX);

    ds_create_info_ptr->get_binding_properties_by_binding_index(binding_index,
                                                                nullptr, /* out_opt_descriptor_type_ptr */
                                                               &binding_array_size);
    anvil_assert(in_n_variable_descriptor_bindings <= binding_array_size);

    ds_layout_ptr                  = in_ds_layout_ptr;
    n_variable_descriptor_bindings = in_n_variable_descriptor_bindings;
}

Anvil::DescriptorUpdateTemplateEntry::DescriptorUpdateTemplateEntry()
    :descriptor_type            (Anvil::DescriptorType::UNKNOWN),
     n_descriptors              (UINT32_MAX),
     n_destination_array_element(UINT32_MAX),
     n_destination_binding      (UINT32_MAX),
     offset                     (SIZE_MAX),
     stride                     (SIZE_MAX)
{
    /* Stub */
}

Anvil::DescriptorUpdateTemplateEntry::DescriptorUpdateTemplateEntry(const Anvil::DescriptorType& in_descriptor_type,
                                                                    const uint32_t&              in_n_destination_array_element,
                                                                    const uint32_t&              in_n_destination_binding,
                                                                    const uint32_t&              in_n_descriptors,
                                                                    const size_t&                in_offset,
                                                                    const size_t&                in_stride)
    :descriptor_type            (in_descriptor_type),
     n_descriptors              (in_n_descriptors),
     n_destination_array_element(in_n_destination_array_element),
     n_destination_binding      (in_n_destination_binding),
     offset                     (in_offset),
     stride                     (in_stride)
{
    if (in_descriptor_type == Anvil::DescriptorType::INLINE_UNIFORM_BLOCK)
    {
        n_destination_array_element *= 4;
        n_descriptors               *= 4;
    }
}

bool Anvil::DescriptorUpdateTemplateEntry::operator==(const Anvil::DescriptorUpdateTemplateEntry& in_entry) const
{
    return (in_entry.descriptor_type             == descriptor_type)             &&
           (in_entry.n_descriptors               == n_descriptors)               &&
           (in_entry.n_destination_array_element == n_destination_array_element) &&
           (in_entry.n_destination_binding       == n_destination_binding)       &&
           (in_entry.offset                      == offset)                      &&
           (in_entry.stride                      == stride);
}

bool Anvil::DescriptorUpdateTemplateEntry::operator<(const Anvil::DescriptorUpdateTemplateEntry& in_entry) const
{
    if (in_entry.descriptor_type < descriptor_type)
    {
        return true;
    }
    else
    if (in_entry.descriptor_type > descriptor_type)
    {
        return false;
    }

    if (in_entry.n_descriptors < n_descriptors)
    {
        return true;
    }
    else
    if (in_entry.n_descriptors > n_descriptors)
    {
        return false;
    }

    if (in_entry.n_destination_array_element < n_destination_array_element)
    {
        return true;
    }
    else
    if (in_entry.n_destination_array_element > n_destination_array_element)
    {
        return false;
    }

    if (in_entry.n_destination_binding < n_destination_binding)
    {
        return true;
    }
    else
    if (in_entry.n_destination_binding > n_destination_binding)
    {
        return false;
    }

    if (in_entry.offset < offset)
    {
        return true;
    }
    else
    if (in_entry.offset > offset)
    {
        return false;
    }

    if (in_entry.stride < stride)
    {
        return true;
    }

    return false;
}

/** Please see header for specification */
VkDescriptorUpdateTemplateEntryKHR Anvil::DescriptorUpdateTemplateEntry::get_vk_descriptor_update_template_entry_khr() const
{
    VkDescriptorUpdateTemplateEntryKHR result;

    result.descriptorCount = n_descriptors;
    result.descriptorType  = static_cast<VkDescriptorType>(descriptor_type);
    result.dstArrayElement = n_destination_array_element;
    result.dstBinding      = n_destination_binding;
    result.offset          = offset;
    result.stride          = stride;

    return result;
}

Anvil::ExtensionAMDBufferMarkerEntrypoints::ExtensionAMDBufferMarkerEntrypoints()
{
    vkCmdWriteBufferMarkerAMD = nullptr;
}

Anvil::ExtensionAMDDrawIndirectCountEntrypoints::ExtensionAMDDrawIndirectCountEntrypoints()
{
    vkCmdDrawIndexedIndirectCountAMD = nullptr;
    vkCmdDrawIndirectCountAMD        = nullptr;
}

Anvil::ExtensionAMDShaderInfoEntrypoints::ExtensionAMDShaderInfoEntrypoints()
{
    vkGetShaderInfoAMD = nullptr;
}

Anvil::ExtensionEXTDebugMarkerEntrypoints::ExtensionEXTDebugMarkerEntrypoints()
{
    vkCmdDebugMarkerBeginEXT      = nullptr;
    vkCmdDebugMarkerEndEXT        = nullptr;
    vkCmdDebugMarkerInsertEXT     = nullptr;
    vkDebugMarkerSetObjectNameEXT = nullptr;
    vkDebugMarkerSetObjectTagEXT  = nullptr;
}

Anvil::ExtensionEXTDebugReportEntrypoints::ExtensionEXTDebugReportEntrypoints()
{
    vkCreateDebugReportCallbackEXT  = nullptr;
    vkDebugReportMessageEXT         = nullptr;
    vkDestroyDebugReportCallbackEXT = nullptr;
}

Anvil::ExtensionEXTDebugUtilsEntrypoints::ExtensionEXTDebugUtilsEntrypoints()
{
    vkCmdBeginDebugUtilsLabelEXT    = nullptr;
    vkCmdEndDebugUtilsLabelEXT      = nullptr;
    vkCmdInsertDebugUtilsLabelEXT   = nullptr;
    vkCreateDebugUtilsMessengerEXT  = nullptr;
    vkDestroyDebugUtilsMessengerEXT = nullptr;
    vkSetDebugUtilsObjectNameEXT    = nullptr;
    vkSetDebugUtilsObjectTagEXT     = nullptr;
    vkQueueBeginDebugUtilsLabelEXT  = nullptr;
    vkQueueEndDebugUtilsLabelEXT    = nullptr;
    vkQueueInsertDebugUtilsLabelEXT = nullptr;
    vkSubmitDebugUtilsMessageEXT    = nullptr;
}

Anvil::ExtensionEXTExternalMemoryHostEntrypoints::ExtensionEXTExternalMemoryHostEntrypoints()
{
    vkGetMemoryHostPointerPropertiesEXT = nullptr;
}

Anvil::ExtensionEXTHdrMetadataEntrypoints::ExtensionEXTHdrMetadataEntrypoints()
{
    vkSetHdrMetadataEXT = nullptr;
}

Anvil::ExtensionEXTSampleLocationsEntrypoints::ExtensionEXTSampleLocationsEntrypoints()
{
    vkCmdSetSampleLocationsEXT                  = nullptr;
    vkGetPhysicalDeviceMultisamplePropertiesEXT = nullptr;
}

Anvil::ExtensionEXTTransformFeedbackEntrypoints::ExtensionEXTTransformFeedbackEntrypoints()
{
    vkCmdBeginQueryIndexedEXT            = nullptr;
    vkCmdBeginTransformFeedbackEXT       = nullptr;
    vkCmdBindTransformFeedbackBuffersEXT = nullptr;
    vkCmdDrawIndirectByteCountEXT        = nullptr;
    vkCmdEndQueryIndexedEXT              = nullptr;
    vkCmdEndTransformFeedbackEXT         = nullptr;
}

Anvil::ExtensionKHRCreateRenderpass2Entrypoints::ExtensionKHRCreateRenderpass2Entrypoints()
{
    vkCmdBeginRenderPass2KHR = nullptr;
    vkCmdEndRenderPass2KHR   = nullptr;
    vkCmdNextSubpass2KHR     = nullptr;
    vkCreateRenderPass2KHR   = nullptr;
}

Anvil::ExtensionKHRDeviceGroupEntrypoints::ExtensionKHRDeviceGroupEntrypoints()
{
    vkAcquireNextImage2KHR                  = nullptr;
    vkCmdDispatchBaseKHR                    = nullptr;
    vkGetDeviceGroupPeerMemoryFeaturesKHR   = nullptr;
    vkGetDeviceGroupPresentCapabilitiesKHR  = nullptr;
    vkGetDeviceGroupSurfacePresentModesKHR  = nullptr;
    vkGetPhysicalDevicePresentRectanglesKHR = nullptr;
    vkCmdSetDeviceMaskKHR                   = nullptr;
}

Anvil::ExtensionKHRBindMemory2Entrypoints::ExtensionKHRBindMemory2Entrypoints()
{
    vkBindBufferMemory2KHR = nullptr;
    vkBindImageMemory2KHR  = nullptr;
}

Anvil::ExtensionKHRDescriptorUpdateTemplateEntrypoints::ExtensionKHRDescriptorUpdateTemplateEntrypoints()
{
    vkCreateDescriptorUpdateTemplateKHR  = nullptr;
    vkDestroyDescriptorUpdateTemplateKHR = nullptr;
    vkUpdateDescriptorSetWithTemplateKHR = nullptr;
}

Anvil::ExtensionKHRDeviceGroupCreationEntrypoints::ExtensionKHRDeviceGroupCreationEntrypoints()
{
    vkEnumeratePhysicalDeviceGroupsKHR = nullptr;
}

Anvil::ExtensionKHRDrawIndirectCountEntrypoints::ExtensionKHRDrawIndirectCountEntrypoints()
{
    vkCmdDrawIndexedIndirectCountKHR = nullptr;
    vkCmdDrawIndirectCountKHR        = nullptr;
}

Anvil::ExtensionKHRExternalFenceCapabilitiesEntrypoints::ExtensionKHRExternalFenceCapabilitiesEntrypoints()
{
    vkGetPhysicalDeviceExternalFencePropertiesKHR = nullptr;
}

Anvil::ExtensionKHRExternalMemoryCapabilitiesEntrypoints::ExtensionKHRExternalMemoryCapabilitiesEntrypoints()
{
    vkGetPhysicalDeviceExternalBufferPropertiesKHR = nullptr;
}

Anvil::ExtensionKHRExternalSemaphoreCapabilitiesEntrypoints::ExtensionKHRExternalSemaphoreCapabilitiesEntrypoints()
{
    vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = nullptr;
}

#ifdef _WIN32
    Anvil::ExtensionKHRExternalFenceWin32Entrypoints::ExtensionKHRExternalFenceWin32Entrypoints()
    {
        vkGetFenceWin32HandleKHR    = nullptr;
        vkImportFenceWin32HandleKHR = nullptr;
    }

    Anvil::ExtensionKHRExternalMemoryWin32Entrypoints::ExtensionKHRExternalMemoryWin32Entrypoints()
    {
        vkGetMemoryWin32HandleKHR           = nullptr;
        vkGetMemoryWin32HandlePropertiesKHR = nullptr;
    }

    Anvil::ExtensionKHRExternalSemaphoreWin32Entrypoints::ExtensionKHRExternalSemaphoreWin32Entrypoints()
    {
        vkGetSemaphoreWin32HandleKHR    = nullptr;
        vkImportSemaphoreWin32HandleKHR = nullptr;
    }
#else
    Anvil::ExtensionKHRExternalFenceFdEntrypoints::ExtensionKHRExternalFenceFdEntrypoints()
    {
        vkGetFenceFdKHR    = nullptr;
        vkImportFenceFdKHR = nullptr;
    }

    Anvil::ExtensionKHRExternalMemoryFdEntrypoints::ExtensionKHRExternalMemoryFdEntrypoints()
    {
        vkGetMemoryFdKHR           = nullptr;
        vkGetMemoryFdPropertiesKHR = nullptr;
    }

    Anvil::ExtensionKHRExternalSemaphoreFdEntrypoints::ExtensionKHRExternalSemaphoreFdEntrypoints()
    {
        vkGetSemaphoreFdKHR    = nullptr;
        vkImportSemaphoreFdKHR = nullptr;
    }
#endif

Anvil::ExtensionKHRMaintenance1Entrypoints::ExtensionKHRMaintenance1Entrypoints()
{
    vkTrimCommandPoolKHR = nullptr;
}

Anvil::ExtensionKHRMaintenance3Entrypoints::ExtensionKHRMaintenance3Entrypoints()
{
    vkGetDescriptorSetLayoutSupportKHR = nullptr;
}

Anvil::ExtensionKHRSamplerYCbCrConversionEntrypoints::ExtensionKHRSamplerYCbCrConversionEntrypoints()
{
    vkCreateSamplerYcbcrConversionKHR  = nullptr;
    vkDestroySamplerYcbcrConversionKHR = nullptr;
}

Anvil::ExtensionKHRSurfaceEntrypoints::ExtensionKHRSurfaceEntrypoints()
{
    vkDestroySurfaceKHR                       = nullptr;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
    vkGetPhysicalDeviceSurfaceFormatsKHR      = nullptr;
    vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
    vkGetPhysicalDeviceSurfaceSupportKHR      = nullptr;
}

Anvil::ExtensionKHRSwapchainEntrypoints::ExtensionKHRSwapchainEntrypoints()
{
    vkAcquireNextImageKHR   = nullptr;
    vkCreateSwapchainKHR    = nullptr;
    vkDestroySwapchainKHR   = nullptr;
    vkGetSwapchainImagesKHR = nullptr;
    vkQueuePresentKHR       = nullptr;
}

#ifdef _WIN32
    #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        Anvil::ExtensionKHRWin32SurfaceEntrypoints::ExtensionKHRWin32SurfaceEntrypoints()
        {
            vkCreateWin32SurfaceKHR                        = nullptr;
            vkGetPhysicalDeviceWin32PresentationSupportKHR = nullptr;
        }
    #endif
#else
    #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        Anvil::ExtensionKHRXcbSurfaceEntrypoints::ExtensionKHRXcbSurfaceEntrypoints()
        {
            vkCreateXcbSurfaceKHR = nullptr;
        }
    #endif
#endif

Anvil::ExtensionKHRGetMemoryRequirements2Entrypoints::ExtensionKHRGetMemoryRequirements2Entrypoints()
{
    vkGetBufferMemoryRequirements2KHR      = nullptr;
    vkGetImageMemoryRequirements2KHR       = nullptr;
    vkGetImageSparseMemoryRequirements2KHR = nullptr;
}

Anvil::ExtensionKHRGetPhysicalDeviceProperties2::ExtensionKHRGetPhysicalDeviceProperties2()
{
    vkGetPhysicalDeviceFeatures2KHR                    = nullptr;
    vkGetPhysicalDeviceFormatProperties2KHR            = nullptr;
    vkGetPhysicalDeviceImageFormatProperties2KHR       = nullptr;
    vkGetPhysicalDeviceMemoryProperties2KHR            = nullptr;
    vkGetPhysicalDeviceProperties2KHR                  = nullptr;
    vkGetPhysicalDeviceQueueFamilyProperties2KHR       = nullptr;
    vkGetPhysicalDeviceSparseImageFormatProperties2KHR = nullptr;
}

Anvil::EXTConservativeRasterizationProperties::EXTConservativeRasterizationProperties()
    :conservative_point_and_line_rasterization       (false),
     conservative_rasterization_post_depth_coverage  (false),
     degenerate_lines_rasterized                     (false),
     degenerate_triangles_rasterized                 (false),
     extra_primitive_overestimation_size_granularity (0.0),
     fully_covered_fragment_shader_input_variable    (false),
     max_extra_primitive_overestimation_size         (0.0),
     primitive_overestimation_size                   (0.0),
     primitive_underestimation                       (false)
{
    /* Stub */
}

Anvil::EXTConservativeRasterizationProperties::EXTConservativeRasterizationProperties(const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& in_properties)
    :conservative_point_and_line_rasterization      (VK_BOOL32_TO_BOOL(in_properties.conservativePointAndLineRasterization)),
    conservative_rasterization_post_depth_coverage  (VK_BOOL32_TO_BOOL(in_properties.conservativeRasterizationPostDepthCoverage)),
    degenerate_lines_rasterized                     (VK_BOOL32_TO_BOOL(in_properties.degenerateLinesRasterized)),
    degenerate_triangles_rasterized                 (VK_BOOL32_TO_BOOL(in_properties.degenerateTrianglesRasterized)),
    extra_primitive_overestimation_size_granularity (in_properties.extraPrimitiveOverestimationSizeGranularity),
    fully_covered_fragment_shader_input_variable    (VK_BOOL32_TO_BOOL(in_properties.fullyCoveredFragmentShaderInputVariable)),
    max_extra_primitive_overestimation_size         (in_properties.maxExtraPrimitiveOverestimationSize),
    primitive_overestimation_size                   (in_properties.primitiveOverestimationSize),
    primitive_underestimation                       (VK_BOOL32_TO_BOOL(in_properties.primitiveUnderestimation) )
{
    /* Stub */
}

bool Anvil::EXTConservativeRasterizationProperties::operator==(const Anvil::EXTConservativeRasterizationProperties& in_properties) const
{
    return (conservative_point_and_line_rasterization       == in_properties.conservative_point_and_line_rasterization       &&
            conservative_rasterization_post_depth_coverage  == in_properties.conservative_rasterization_post_depth_coverage  &&
            degenerate_lines_rasterized                     == in_properties.degenerate_lines_rasterized                     &&
            degenerate_triangles_rasterized                 == in_properties.degenerate_triangles_rasterized                 &&
            extra_primitive_overestimation_size_granularity == in_properties.extra_primitive_overestimation_size_granularity &&
            fully_covered_fragment_shader_input_variable    == in_properties.fully_covered_fragment_shader_input_variable    &&
            max_extra_primitive_overestimation_size         == in_properties.max_extra_primitive_overestimation_size         &&
            primitive_overestimation_size                   == in_properties.primitive_overestimation_size                   &&
            primitive_underestimation                       == in_properties.primitive_underestimation);
}

VkPhysicalDeviceConservativeRasterizationPropertiesEXT Anvil::EXTConservativeRasterizationProperties::get_vk_physical_device_conservative_rasterization_properties() const
{
    VkPhysicalDeviceConservativeRasterizationPropertiesEXT result;

    result.conservativePointAndLineRasterization       = BOOL_TO_VK_BOOL32(conservative_point_and_line_rasterization);
    result.conservativeRasterizationPostDepthCoverage  = BOOL_TO_VK_BOOL32(conservative_rasterization_post_depth_coverage);
    result.degenerateLinesRasterized                   = BOOL_TO_VK_BOOL32(degenerate_lines_rasterized);
    result.degenerateTrianglesRasterized               = BOOL_TO_VK_BOOL32(degenerate_triangles_rasterized);
    result.extraPrimitiveOverestimationSizeGranularity = extra_primitive_overestimation_size_granularity;
    result.fullyCoveredFragmentShaderInputVariable     = BOOL_TO_VK_BOOL32(fully_covered_fragment_shader_input_variable);
    result.maxExtraPrimitiveOverestimationSize         = max_extra_primitive_overestimation_size;
    result.pNext                                       = nullptr;
    result.primitiveOverestimationSize                 = primitive_overestimation_size;
    result.primitiveUnderestimation                    = BOOL_TO_VK_BOOL32(primitive_underestimation);
    result.sType                                       = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT);

    return result;
}

Anvil::EXTDepthClipEnableFeatures::EXTDepthClipEnableFeatures()
    :depth_clip_enable(false)
{
    /* Stub */
}

Anvil::EXTDepthClipEnableFeatures::EXTDepthClipEnableFeatures(const VkPhysicalDeviceDepthClipEnableFeaturesEXT& in_features)
    :depth_clip_enable(VK_BOOL32_TO_BOOL(in_features.depthClipEnable) )
{
    /* Stub */
}

VkPhysicalDeviceDepthClipEnableFeaturesEXT Anvil::EXTDepthClipEnableFeatures::get_vk_physical_device_depth_clip_enable_features() const
{
    VkPhysicalDeviceDepthClipEnableFeaturesEXT result;

    result.depthClipEnable = BOOL_TO_VK_BOOL32(depth_clip_enable);
    result.pNext           = nullptr;
    result.sType           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;

    return result;
}

bool Anvil::EXTDepthClipEnableFeatures::operator==(const EXTDepthClipEnableFeatures& in_features) const
{
    return (depth_clip_enable == in_features.depth_clip_enable);
}

Anvil::EXTDescriptorIndexingFeatures::EXTDescriptorIndexingFeatures()
    :descriptor_binding_partially_bound                       (false),
     descriptor_binding_sampled_image_update_after_bind       (false),
     descriptor_binding_storage_buffer_update_after_bind      (false),
     descriptor_binding_storage_image_update_after_bind       (false),
     descriptor_binding_storage_texel_buffer_update_after_bind(false),
     descriptor_binding_uniform_buffer_update_after_bind      (false),
     descriptor_binding_uniform_texel_buffer_update_after_bind(false),
     descriptor_binding_update_unused_while_pending           (false),
     descriptor_binding_variable_descriptor_count             (false),
     runtime_descriptor_array                                 (false),
     shader_input_attachment_array_dynamic_indexing           (false),
     shader_input_attachment_array_non_uniform_indexing       (false),
     shader_sampled_image_array_non_uniform_indexing          (false),
     shader_storage_buffer_array_non_uniform_indexing         (false),
     shader_storage_image_array_non_uniform_indexing          (false),
     shader_storage_texel_buffer_array_dynamic_indexing       (false),
     shader_storage_texel_buffer_array_non_uniform_indexing   (false),
     shader_uniform_buffer_array_non_uniform_indexing         (false),
     shader_uniform_texel_buffer_array_dynamic_indexing       (false),
     shader_uniform_texel_buffer_array_non_uniform_indexing   (false)
{
    /* Stub */
}

Anvil::EXTDescriptorIndexingFeatures::EXTDescriptorIndexingFeatures(const VkPhysicalDeviceDescriptorIndexingFeaturesEXT& in_features)
    :descriptor_binding_partially_bound                       (VK_BOOL32_TO_BOOL(in_features.descriptorBindingPartiallyBound) ),
     descriptor_binding_sampled_image_update_after_bind       (VK_BOOL32_TO_BOOL(in_features.descriptorBindingSampledImageUpdateAfterBind) ),
     descriptor_binding_storage_buffer_update_after_bind      (VK_BOOL32_TO_BOOL(in_features.descriptorBindingStorageBufferUpdateAfterBind) ),
     descriptor_binding_storage_image_update_after_bind       (VK_BOOL32_TO_BOOL(in_features.descriptorBindingStorageImageUpdateAfterBind) ),
     descriptor_binding_storage_texel_buffer_update_after_bind(VK_BOOL32_TO_BOOL(in_features.descriptorBindingStorageTexelBufferUpdateAfterBind) ),
     descriptor_binding_uniform_buffer_update_after_bind      (VK_BOOL32_TO_BOOL(in_features.descriptorBindingUniformBufferUpdateAfterBind) ),
     descriptor_binding_uniform_texel_buffer_update_after_bind(VK_BOOL32_TO_BOOL(in_features.descriptorBindingUniformTexelBufferUpdateAfterBind) ),
     descriptor_binding_update_unused_while_pending           (VK_BOOL32_TO_BOOL(in_features.descriptorBindingUpdateUnusedWhilePending) ),
     descriptor_binding_variable_descriptor_count             (VK_BOOL32_TO_BOOL(in_features.descriptorBindingVariableDescriptorCount) ),
     runtime_descriptor_array                                 (VK_BOOL32_TO_BOOL(in_features.runtimeDescriptorArray) ),
     shader_input_attachment_array_dynamic_indexing           (VK_BOOL32_TO_BOOL(in_features.shaderInputAttachmentArrayDynamicIndexing) ),
     shader_input_attachment_array_non_uniform_indexing       (VK_BOOL32_TO_BOOL(in_features.shaderInputAttachmentArrayNonUniformIndexing) ),
     shader_sampled_image_array_non_uniform_indexing          (VK_BOOL32_TO_BOOL(in_features.shaderSampledImageArrayNonUniformIndexing) ),
     shader_storage_buffer_array_non_uniform_indexing         (VK_BOOL32_TO_BOOL(in_features.shaderStorageBufferArrayNonUniformIndexing) ),
     shader_storage_image_array_non_uniform_indexing          (VK_BOOL32_TO_BOOL(in_features.shaderStorageImageArrayNonUniformIndexing) ),
     shader_storage_texel_buffer_array_dynamic_indexing       (VK_BOOL32_TO_BOOL(in_features.shaderStorageTexelBufferArrayDynamicIndexing) ),
     shader_storage_texel_buffer_array_non_uniform_indexing   (VK_BOOL32_TO_BOOL(in_features.shaderStorageTexelBufferArrayNonUniformIndexing) ),
     shader_uniform_buffer_array_non_uniform_indexing         (VK_BOOL32_TO_BOOL(in_features.shaderUniformBufferArrayNonUniformIndexing) ),
     shader_uniform_texel_buffer_array_dynamic_indexing       (VK_BOOL32_TO_BOOL(in_features.shaderUniformTexelBufferArrayDynamicIndexing) ),
     shader_uniform_texel_buffer_array_non_uniform_indexing   (VK_BOOL32_TO_BOOL(in_features.shaderUniformTexelBufferArrayNonUniformIndexing) )
{
    /* Stub */
}

bool Anvil::EXTDescriptorIndexingFeatures::operator==(const EXTDescriptorIndexingFeatures& in_features) const
{
    return (descriptor_binding_partially_bound                        == in_features.descriptor_binding_partially_bound                        &&
            descriptor_binding_sampled_image_update_after_bind        == in_features.descriptor_binding_sampled_image_update_after_bind        &&
            descriptor_binding_storage_buffer_update_after_bind       == in_features.descriptor_binding_storage_buffer_update_after_bind       &&
            descriptor_binding_storage_image_update_after_bind        == in_features.descriptor_binding_storage_image_update_after_bind        &&
            descriptor_binding_storage_texel_buffer_update_after_bind == in_features.descriptor_binding_storage_texel_buffer_update_after_bind &&
            descriptor_binding_uniform_buffer_update_after_bind       == in_features.descriptor_binding_uniform_buffer_update_after_bind       &&
            descriptor_binding_uniform_texel_buffer_update_after_bind == in_features.descriptor_binding_uniform_texel_buffer_update_after_bind &&
            descriptor_binding_update_unused_while_pending            == in_features.descriptor_binding_update_unused_while_pending            &&
            descriptor_binding_variable_descriptor_count              == in_features.descriptor_binding_variable_descriptor_count              &&
            runtime_descriptor_array                                  == in_features.runtime_descriptor_array                                  &&
            shader_input_attachment_array_dynamic_indexing            == in_features.shader_input_attachment_array_dynamic_indexing            &&
            shader_input_attachment_array_non_uniform_indexing        == in_features.shader_input_attachment_array_non_uniform_indexing        &&
            shader_sampled_image_array_non_uniform_indexing           == in_features.shader_sampled_image_array_non_uniform_indexing           &&
            shader_storage_buffer_array_non_uniform_indexing          == in_features.shader_storage_buffer_array_non_uniform_indexing          &&
            shader_storage_image_array_non_uniform_indexing           == in_features.shader_storage_image_array_non_uniform_indexing           &&
            shader_storage_texel_buffer_array_dynamic_indexing        == in_features.shader_storage_texel_buffer_array_dynamic_indexing        &&
            shader_storage_texel_buffer_array_non_uniform_indexing    == in_features.shader_storage_texel_buffer_array_non_uniform_indexing    &&
            shader_uniform_buffer_array_non_uniform_indexing          == in_features.shader_uniform_buffer_array_non_uniform_indexing          &&
            shader_uniform_texel_buffer_array_dynamic_indexing        == in_features.shader_uniform_texel_buffer_array_dynamic_indexing        &&
            shader_uniform_texel_buffer_array_non_uniform_indexing    == in_features.shader_uniform_texel_buffer_array_non_uniform_indexing);
}

VkPhysicalDeviceDescriptorIndexingFeaturesEXT Anvil::EXTDescriptorIndexingFeatures::get_vk_physical_device_descriptor_indexing_features() const
{
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT result;

    result.descriptorBindingPartiallyBound                    = BOOL_TO_VK_BOOL32(descriptor_binding_partially_bound);
    result.descriptorBindingSampledImageUpdateAfterBind       = BOOL_TO_VK_BOOL32(descriptor_binding_sampled_image_update_after_bind);
    result.descriptorBindingStorageBufferUpdateAfterBind      = BOOL_TO_VK_BOOL32(descriptor_binding_storage_buffer_update_after_bind);
    result.descriptorBindingStorageImageUpdateAfterBind       = BOOL_TO_VK_BOOL32(descriptor_binding_storage_image_update_after_bind);
    result.descriptorBindingStorageTexelBufferUpdateAfterBind = BOOL_TO_VK_BOOL32(descriptor_binding_storage_texel_buffer_update_after_bind);
    result.descriptorBindingUniformBufferUpdateAfterBind      = BOOL_TO_VK_BOOL32(descriptor_binding_uniform_buffer_update_after_bind);
    result.descriptorBindingUniformTexelBufferUpdateAfterBind = BOOL_TO_VK_BOOL32(descriptor_binding_uniform_texel_buffer_update_after_bind);
    result.descriptorBindingUpdateUnusedWhilePending          = BOOL_TO_VK_BOOL32(descriptor_binding_update_unused_while_pending);
    result.descriptorBindingVariableDescriptorCount           = BOOL_TO_VK_BOOL32(descriptor_binding_variable_descriptor_count);
    result.pNext                                              = nullptr;
    result.runtimeDescriptorArray                             = BOOL_TO_VK_BOOL32(runtime_descriptor_array);
    result.shaderInputAttachmentArrayDynamicIndexing          = BOOL_TO_VK_BOOL32(shader_input_attachment_array_dynamic_indexing);
    result.shaderInputAttachmentArrayNonUniformIndexing       = BOOL_TO_VK_BOOL32(shader_input_attachment_array_non_uniform_indexing);
    result.shaderSampledImageArrayNonUniformIndexing          = BOOL_TO_VK_BOOL32(shader_sampled_image_array_non_uniform_indexing);
    result.shaderStorageBufferArrayNonUniformIndexing         = BOOL_TO_VK_BOOL32(shader_storage_buffer_array_non_uniform_indexing);
    result.shaderStorageImageArrayNonUniformIndexing          = BOOL_TO_VK_BOOL32(shader_storage_image_array_non_uniform_indexing);
    result.shaderStorageTexelBufferArrayDynamicIndexing       = BOOL_TO_VK_BOOL32(shader_storage_texel_buffer_array_dynamic_indexing);
    result.shaderStorageTexelBufferArrayNonUniformIndexing    = BOOL_TO_VK_BOOL32(shader_storage_texel_buffer_array_non_uniform_indexing);
    result.shaderUniformBufferArrayNonUniformIndexing         = BOOL_TO_VK_BOOL32(shader_uniform_buffer_array_non_uniform_indexing);
    result.shaderUniformTexelBufferArrayDynamicIndexing       = BOOL_TO_VK_BOOL32(shader_uniform_texel_buffer_array_dynamic_indexing);
    result.shaderUniformTexelBufferArrayNonUniformIndexing    = BOOL_TO_VK_BOOL32(shader_uniform_texel_buffer_array_non_uniform_indexing);
    result.sType                                              = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT);

    return result;
}

Anvil::EXTDescriptorIndexingProperties::EXTDescriptorIndexingProperties()
    :max_descriptor_set_update_after_bind_input_attachments      (UINT32_MAX),
     max_descriptor_set_update_after_bind_sampled_images         (UINT32_MAX),
     max_descriptor_set_update_after_bind_samplers               (UINT32_MAX),
     max_descriptor_set_update_after_bind_storage_buffers        (UINT32_MAX),
     max_descriptor_set_update_after_bind_storage_buffers_dynamic(UINT32_MAX),
     max_descriptor_set_update_after_bind_storage_images         (UINT32_MAX),
     max_descriptor_set_update_after_bind_uniform_buffers        (UINT32_MAX),
     max_descriptor_set_update_after_bind_uniform_buffers_dynamic(UINT32_MAX),
     max_per_stage_descriptor_update_after_bind_input_attachments(UINT32_MAX),
     max_per_stage_descriptor_update_after_bind_sampled_images   (UINT32_MAX),
     max_per_stage_descriptor_update_after_bind_samplers         (UINT32_MAX),
     max_per_stage_descriptor_update_after_bind_storage_buffers  (UINT32_MAX),
     max_per_stage_descriptor_update_after_bind_storage_images   (UINT32_MAX),
     max_per_stage_descriptor_update_after_bind_uniform_buffers  (UINT32_MAX),
     max_per_stage_update_after_bind_resources                   (UINT32_MAX),
     max_update_after_bind_descriptors_in_all_pools              (UINT32_MAX),
     shader_input_attachment_array_non_uniform_indexing_native   (false),
     shader_sampled_image_array_non_uniform_indexing_native      (false),
     shader_storage_buffer_array_non_uniform_indexing_native     (false),
     shader_storage_image_array_non_uniform_indexing_native      (false),
     shader_uniform_buffer_array_non_uniform_indexing_native     (false)
{
    /* Stub */
}

Anvil::EXTDescriptorIndexingProperties::EXTDescriptorIndexingProperties(const VkPhysicalDeviceDescriptorIndexingPropertiesEXT& in_props)
    :max_descriptor_set_update_after_bind_input_attachments      (in_props.maxDescriptorSetUpdateAfterBindInputAttachments),
     max_descriptor_set_update_after_bind_sampled_images         (in_props.maxDescriptorSetUpdateAfterBindSampledImages),
     max_descriptor_set_update_after_bind_samplers               (in_props.maxDescriptorSetUpdateAfterBindSamplers),
     max_descriptor_set_update_after_bind_storage_buffers        (in_props.maxDescriptorSetUpdateAfterBindStorageBuffers),
     max_descriptor_set_update_after_bind_storage_buffers_dynamic(in_props.maxDescriptorSetUpdateAfterBindStorageBuffersDynamic),
     max_descriptor_set_update_after_bind_storage_images         (in_props.maxDescriptorSetUpdateAfterBindStorageImages),
     max_descriptor_set_update_after_bind_uniform_buffers        (in_props.maxDescriptorSetUpdateAfterBindUniformBuffers),
     max_descriptor_set_update_after_bind_uniform_buffers_dynamic(in_props.maxDescriptorSetUpdateAfterBindUniformBuffersDynamic),
     max_per_stage_descriptor_update_after_bind_input_attachments(in_props.maxPerStageDescriptorUpdateAfterBindInputAttachments),
     max_per_stage_descriptor_update_after_bind_sampled_images   (in_props.maxPerStageDescriptorUpdateAfterBindSampledImages),
     max_per_stage_descriptor_update_after_bind_samplers         (in_props.maxPerStageDescriptorUpdateAfterBindSamplers),
     max_per_stage_descriptor_update_after_bind_storage_buffers  (in_props.maxPerStageDescriptorUpdateAfterBindStorageBuffers),
     max_per_stage_descriptor_update_after_bind_storage_images   (in_props.maxPerStageDescriptorUpdateAfterBindStorageImages),
     max_per_stage_descriptor_update_after_bind_uniform_buffers  (in_props.maxPerStageDescriptorUpdateAfterBindUniformBuffers),
     max_per_stage_update_after_bind_resources                   (in_props.maxPerStageUpdateAfterBindResources),
     max_update_after_bind_descriptors_in_all_pools              (in_props.maxUpdateAfterBindDescriptorsInAllPools),
     shader_input_attachment_array_non_uniform_indexing_native   (VK_BOOL32_TO_BOOL(in_props.shaderInputAttachmentArrayNonUniformIndexingNative) ),
     shader_sampled_image_array_non_uniform_indexing_native      (VK_BOOL32_TO_BOOL(in_props.shaderSampledImageArrayNonUniformIndexingNative) ),
     shader_storage_buffer_array_non_uniform_indexing_native     (VK_BOOL32_TO_BOOL(in_props.shaderStorageBufferArrayNonUniformIndexingNative) ),
     shader_storage_image_array_non_uniform_indexing_native      (VK_BOOL32_TO_BOOL(in_props.shaderStorageImageArrayNonUniformIndexingNative) ),
     shader_uniform_buffer_array_non_uniform_indexing_native     (VK_BOOL32_TO_BOOL(in_props.shaderUniformBufferArrayNonUniformIndexingNative) )
{
    /* Stub */
}

bool Anvil::EXTDescriptorIndexingProperties::operator==(const EXTDescriptorIndexingProperties& in_props) const
{
    return (max_descriptor_set_update_after_bind_input_attachments       == in_props.max_descriptor_set_update_after_bind_input_attachments       &&
            max_descriptor_set_update_after_bind_sampled_images          == in_props.max_descriptor_set_update_after_bind_sampled_images          &&
            max_descriptor_set_update_after_bind_samplers                == in_props.max_descriptor_set_update_after_bind_samplers                &&
            max_descriptor_set_update_after_bind_storage_buffers         == in_props.max_descriptor_set_update_after_bind_storage_buffers         &&
            max_descriptor_set_update_after_bind_storage_buffers_dynamic == in_props.max_descriptor_set_update_after_bind_storage_buffers_dynamic &&
            max_descriptor_set_update_after_bind_storage_images          == in_props.max_descriptor_set_update_after_bind_storage_images          &&
            max_descriptor_set_update_after_bind_uniform_buffers         == in_props.max_descriptor_set_update_after_bind_uniform_buffers         &&
            max_descriptor_set_update_after_bind_uniform_buffers_dynamic == in_props.max_descriptor_set_update_after_bind_uniform_buffers_dynamic &&
            max_per_stage_descriptor_update_after_bind_input_attachments == in_props.max_per_stage_descriptor_update_after_bind_input_attachments &&
            max_per_stage_descriptor_update_after_bind_sampled_images    == in_props.max_per_stage_descriptor_update_after_bind_sampled_images    &&
            max_per_stage_descriptor_update_after_bind_samplers          == in_props.max_per_stage_descriptor_update_after_bind_samplers          &&
            max_per_stage_descriptor_update_after_bind_storage_buffers   == in_props.max_per_stage_descriptor_update_after_bind_storage_buffers   &&
            max_per_stage_descriptor_update_after_bind_storage_images    == in_props.max_per_stage_descriptor_update_after_bind_storage_images    &&
            max_per_stage_descriptor_update_after_bind_uniform_buffers   == in_props.max_per_stage_descriptor_update_after_bind_uniform_buffers   &&
            max_per_stage_update_after_bind_resources                    == in_props.max_per_stage_update_after_bind_resources                    &&
            max_update_after_bind_descriptors_in_all_pools               == in_props.max_update_after_bind_descriptors_in_all_pools               &&
            shader_input_attachment_array_non_uniform_indexing_native    == in_props.shader_input_attachment_array_non_uniform_indexing_native    &&
            shader_sampled_image_array_non_uniform_indexing_native       == in_props.shader_sampled_image_array_non_uniform_indexing_native       &&
            shader_storage_buffer_array_non_uniform_indexing_native      == in_props.shader_storage_buffer_array_non_uniform_indexing_native      &&
            shader_storage_image_array_non_uniform_indexing_native       == in_props.shader_storage_image_array_non_uniform_indexing_native       &&
            shader_uniform_buffer_array_non_uniform_indexing_native      == in_props.shader_uniform_buffer_array_non_uniform_indexing_native);
}

Anvil::EXTExternalMemoryHostProperties::EXTExternalMemoryHostProperties()
{
    min_imported_host_pointer_alignment = 0;
}

Anvil::EXTExternalMemoryHostProperties::EXTExternalMemoryHostProperties(const VkPhysicalDeviceExternalMemoryHostPropertiesEXT& in_props)
{
    min_imported_host_pointer_alignment = in_props.minImportedHostPointerAlignment;
}

bool Anvil::EXTExternalMemoryHostProperties::operator==(const Anvil::EXTExternalMemoryHostProperties& in_props) const
{
    return (min_imported_host_pointer_alignment == in_props.min_imported_host_pointer_alignment);
}

Anvil::EXTInlineUniformBlockFeatures::EXTInlineUniformBlockFeatures()
    :descriptor_binding_inline_uniform_block_update_after_bind(false),
     inline_uniform_block                                     (false)
{
    /* Stub */
}

Anvil::EXTInlineUniformBlockFeatures::EXTInlineUniformBlockFeatures(const VkPhysicalDeviceInlineUniformBlockFeaturesEXT& in_features)
    :descriptor_binding_inline_uniform_block_update_after_bind(VK_BOOL32_TO_BOOL(in_features.descriptorBindingInlineUniformBlockUpdateAfterBind) ),
     inline_uniform_block                                     (VK_BOOL32_TO_BOOL(in_features.inlineUniformBlock) )
{
    /* Stub */
}

VkPhysicalDeviceInlineUniformBlockFeaturesEXT Anvil::EXTInlineUniformBlockFeatures::get_vk_physical_device_inline_uniform_block_features() const
{
    VkPhysicalDeviceInlineUniformBlockFeaturesEXT result;

    result.descriptorBindingInlineUniformBlockUpdateAfterBind = BOOL_TO_VK_BOOL32(descriptor_binding_inline_uniform_block_update_after_bind);
    result.inlineUniformBlock                                 = BOOL_TO_VK_BOOL32(inline_uniform_block);
    result.pNext                                              = nullptr;
    result.sType                                              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT;

    return result;
}

bool Anvil::EXTInlineUniformBlockFeatures::operator==(const EXTInlineUniformBlockFeatures& in_features) const
{
    return (in_features.descriptor_binding_inline_uniform_block_update_after_bind == descriptor_binding_inline_uniform_block_update_after_bind) &&
           (in_features.inline_uniform_block                                      == inline_uniform_block);
}

Anvil::EXTInlineUniformBlockProperties::EXTInlineUniformBlockProperties()
    :max_descriptor_set_inline_uniform_blocks                        (0),
     max_descriptor_set_update_after_bind_inline_uniform_blocks      (0),
     max_inline_uniform_block_size                                   (0),
     max_per_stage_descriptor_inline_uniform_blocks                  (0),
     max_per_stage_descriptor_update_after_bind_inline_uniform_blocks(0)
{
    /* Stub */
}

Anvil::EXTInlineUniformBlockProperties::EXTInlineUniformBlockProperties(const VkPhysicalDeviceInlineUniformBlockPropertiesEXT& in_props)
    :max_descriptor_set_inline_uniform_blocks                        (in_props.maxDescriptorSetInlineUniformBlocks),
     max_descriptor_set_update_after_bind_inline_uniform_blocks      (in_props.maxDescriptorSetUpdateAfterBindInlineUniformBlocks),
     max_inline_uniform_block_size                                   (in_props.maxInlineUniformBlockSize),
     max_per_stage_descriptor_inline_uniform_blocks                  (in_props.maxPerStageDescriptorInlineUniformBlocks),
     max_per_stage_descriptor_update_after_bind_inline_uniform_blocks(in_props.maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks)
{
    /* Stub */
}

bool Anvil::EXTInlineUniformBlockProperties::operator==(const EXTInlineUniformBlockProperties& in_props) const
{
    return (max_descriptor_set_inline_uniform_blocks                         == in_props.max_descriptor_set_inline_uniform_blocks)                          &&
           (max_descriptor_set_update_after_bind_inline_uniform_blocks       == in_props.max_descriptor_set_update_after_bind_inline_uniform_blocks)        &&
           (max_inline_uniform_block_size                                    == in_props.max_inline_uniform_block_size)                                     &&
           (max_per_stage_descriptor_inline_uniform_blocks                   == in_props.max_per_stage_descriptor_inline_uniform_blocks)                    &&
           (max_per_stage_descriptor_update_after_bind_inline_uniform_blocks == in_props.max_per_stage_descriptor_update_after_bind_inline_uniform_blocks);
}

Anvil::EXTPCIBusInfoProperties::EXTPCIBusInfoProperties()
    :pci_bus     (0),
     pci_device  (0),
     pci_domain  (0),
     pci_function(0)
{
    /* Stub */
}

Anvil::EXTPCIBusInfoProperties::EXTPCIBusInfoProperties(const VkPhysicalDevicePCIBusInfoPropertiesEXT& in_props)
    :pci_bus     (in_props.pciBus),
     pci_device  (in_props.pciDevice),
     pci_domain  (in_props.pciDomain),
     pci_function(in_props.pciFunction)
{
    /* Stub */
}

bool Anvil::EXTPCIBusInfoProperties::operator==(const Anvil::EXTPCIBusInfoProperties& in_props) const
{
    return (in_props.pci_bus      == pci_bus      &&
            in_props.pci_device   == pci_device   &&
            in_props.pci_domain   == pci_domain   &&
            in_props.pci_function == pci_function);
}

Anvil::EXTSampleLocationsProperties::EXTSampleLocationsProperties()
{
    max_sample_location_grid_size.height = 0;
    max_sample_location_grid_size.width  = 0;
    sample_location_coordinate_range[0]  = 0.0f;
    sample_location_coordinate_range[1]  = 0.0f;
    sample_location_sample_counts        = Anvil::SampleCountFlagBits::NONE;
    sample_location_sub_pixel_bits       = 0;
    variable_sample_locations            = false;
}

Anvil::EXTSampleLocationsProperties::EXTSampleLocationsProperties(const VkPhysicalDeviceSampleLocationsPropertiesEXT& in_props)
{
    max_sample_location_grid_size.height = in_props.maxSampleLocationGridSize.height;
    max_sample_location_grid_size.width  = in_props.maxSampleLocationGridSize.width;
    sample_location_coordinate_range[0]  = in_props.sampleLocationCoordinateRange[0];
    sample_location_coordinate_range[1]  = in_props.sampleLocationCoordinateRange[1];
    sample_location_sample_counts        = static_cast<Anvil::SampleCountFlagBits>(in_props.sampleLocationSampleCounts);
    sample_location_sub_pixel_bits       = in_props.sampleLocationSubPixelBits;
    variable_sample_locations            = (in_props.variableSampleLocations == VK_TRUE);
}

bool Anvil::EXTSampleLocationsProperties::operator==(const Anvil::EXTSampleLocationsProperties& in_props) const
{
    return (max_sample_location_grid_size.height == in_props.max_sample_location_grid_size.height &&
            max_sample_location_grid_size.width  == in_props.max_sample_location_grid_size.width  &&
            sample_location_coordinate_range[0]  == in_props.sample_location_coordinate_range[0]  &&
            sample_location_coordinate_range[1]  == in_props.sample_location_coordinate_range[1]  &&
            sample_location_sample_counts        == in_props.sample_location_sample_counts        &&
            sample_location_sub_pixel_bits       == in_props.sample_location_sub_pixel_bits       &&
            variable_sample_locations            == in_props.variable_sample_locations);
}

Anvil::EXTSamplerFilterMinmaxProperties::EXTSamplerFilterMinmaxProperties()
{
    filter_minmax_image_component_mapping  = false;
    filter_minmax_single_component_formats = false;
}

Anvil::EXTSamplerFilterMinmaxProperties::EXTSamplerFilterMinmaxProperties(const VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT& in_props)
{
    filter_minmax_image_component_mapping  = (in_props.filterMinmaxImageComponentMapping  == VK_TRUE);
    filter_minmax_single_component_formats = (in_props.filterMinmaxSingleComponentFormats == VK_TRUE);
}

bool Anvil::EXTSamplerFilterMinmaxProperties::operator==(const Anvil::EXTSamplerFilterMinmaxProperties& in_props) const
{
    return (filter_minmax_image_component_mapping  == in_props.filter_minmax_image_component_mapping &&
            filter_minmax_single_component_formats == in_props.filter_minmax_single_component_formats);
}

Anvil::EXTScalarBlockLayoutFeatures::EXTScalarBlockLayoutFeatures()
    :scalar_block_layout(false)
{
    /* Stub */
}

Anvil::EXTScalarBlockLayoutFeatures::EXTScalarBlockLayoutFeatures(const VkPhysicalDeviceScalarBlockLayoutFeaturesEXT& in_features)
{
    scalar_block_layout = VK_BOOL32_TO_BOOL(in_features.scalarBlockLayout);
}

VkPhysicalDeviceScalarBlockLayoutFeaturesEXT Anvil::EXTScalarBlockLayoutFeatures::get_vk_physical_device_scalar_block_layout_features_ext() const
{
    VkPhysicalDeviceScalarBlockLayoutFeaturesEXT result;

    result.pNext             = nullptr;
    result.scalarBlockLayout = BOOL_TO_VK_BOOL32(scalar_block_layout);
    result.sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT;

    return result;
}

bool Anvil::EXTScalarBlockLayoutFeatures::operator==(const EXTScalarBlockLayoutFeatures& in_features) const
{
    return (in_features.scalar_block_layout == scalar_block_layout);
}

Anvil::EXTTransformFeedbackFeatures::EXTTransformFeedbackFeatures()
    :geometry_streams  (false),
     transform_feedback(false)
{
    /* Stub */
}

Anvil::EXTTransformFeedbackFeatures::EXTTransformFeedbackFeatures(const VkPhysicalDeviceTransformFeedbackFeaturesEXT& in_features)
    :geometry_streams  (in_features.geometryStreams   == VK_TRUE),
     transform_feedback(in_features.transformFeedback == VK_TRUE)
{
    /* Stub */
}

VkPhysicalDeviceTransformFeedbackFeaturesEXT Anvil::EXTTransformFeedbackFeatures::get_vk_physical_device_transform_feedback_features() const
{
    VkPhysicalDeviceTransformFeedbackFeaturesEXT result;

    result.geometryStreams   = (geometry_streams) ? VK_TRUE : VK_FALSE;
    result.pNext             = nullptr;
    result.sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT;
    result.transformFeedback = (transform_feedback) ? VK_TRUE : VK_FALSE;

    return result;
}

bool Anvil::EXTTransformFeedbackFeatures::operator==(const EXTTransformFeedbackFeatures& in_features) const
{
    return (in_features.geometry_streams   == geometry_streams    &&
            in_features.transform_feedback == transform_feedback);
}

Anvil::EXTTransformFeedbackProperties::EXTTransformFeedbackProperties()
    :max_transform_feedback_buffer_data_size                (0),
     max_transform_feedback_buffer_data_stride              (0),
     max_transform_feedback_buffer_size                     (0),
     max_transform_feedback_stream_data_size                (0),
     n_max_transform_feedback_buffers                       (0),
     n_max_transform_feedback_streams                       (0),
     supports_transform_feedback_draw                       (false),
     supports_transform_feedback_queries                    (false),
     supports_transform_feedback_rasterization_stream_select(false),
     supports_transform_feedback_streams_lines_triangles    (false)
{
    /* Stub */
}

Anvil::EXTTransformFeedbackProperties::EXTTransformFeedbackProperties(const VkPhysicalDeviceTransformFeedbackPropertiesEXT& in_props)
    :max_transform_feedback_buffer_data_size                (in_props.maxTransformFeedbackBufferDataSize),
     max_transform_feedback_buffer_data_stride              (in_props.maxTransformFeedbackBufferDataStride),
     max_transform_feedback_buffer_size                     (in_props.maxTransformFeedbackBufferSize),
     max_transform_feedback_stream_data_size                (in_props.maxTransformFeedbackStreamDataSize),
     n_max_transform_feedback_buffers                       (in_props.maxTransformFeedbackBuffers),
     n_max_transform_feedback_streams                       (in_props.maxTransformFeedbackStreams),
     supports_transform_feedback_draw                       (in_props.transformFeedbackDraw                      == VK_TRUE),
     supports_transform_feedback_queries                    (in_props.transformFeedbackQueries                   == VK_TRUE),
     supports_transform_feedback_rasterization_stream_select(in_props.transformFeedbackRasterizationStreamSelect == VK_TRUE),
     supports_transform_feedback_streams_lines_triangles    (in_props.transformFeedbackStreamsLinesTriangles     == VK_TRUE)
{
    /* Stub */
}

bool Anvil::EXTTransformFeedbackProperties::operator==(const Anvil::EXTTransformFeedbackProperties& in_props) const
{
    return (max_transform_feedback_buffer_data_size                 == in_props.max_transform_feedback_buffer_data_size                 &&
            max_transform_feedback_buffer_data_stride               == in_props.max_transform_feedback_buffer_data_stride               &&
            max_transform_feedback_buffer_size                      == in_props.max_transform_feedback_buffer_size                      &&
            max_transform_feedback_stream_data_size                 == in_props.max_transform_feedback_stream_data_size                 &&
            n_max_transform_feedback_buffers                        == in_props.n_max_transform_feedback_buffers                        &&
            n_max_transform_feedback_streams                        == in_props.n_max_transform_feedback_streams                        &&
            supports_transform_feedback_draw                        == in_props.supports_transform_feedback_draw                        &&
            supports_transform_feedback_queries                     == in_props.supports_transform_feedback_queries                     &&
            supports_transform_feedback_rasterization_stream_select == in_props.supports_transform_feedback_rasterization_stream_select &&
            supports_transform_feedback_streams_lines_triangles     == in_props.supports_transform_feedback_streams_lines_triangles);
}

Anvil::EXTVertexAttributeDivisorProperties::EXTVertexAttributeDivisorProperties()
    :max_vertex_attribute_divisor(0)
{
    /* Stub */
}

Anvil::EXTVertexAttributeDivisorProperties::EXTVertexAttributeDivisorProperties(const VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT& in_props)
    :max_vertex_attribute_divisor(in_props.maxVertexAttribDivisor)
{
    /* Stub */
}

bool Anvil::EXTVertexAttributeDivisorProperties::operator==(const Anvil::EXTVertexAttributeDivisorProperties& in_props) const
{
    return (max_vertex_attribute_divisor == in_props.max_vertex_attribute_divisor);
}

Anvil::ExternalFenceProperties::ExternalFenceProperties()
{
    is_exportable = false;
    is_importable = false;
}

Anvil::ExternalFenceProperties::ExternalFenceProperties(const VkExternalFencePropertiesKHR& in_external_fence_props)
{
    compatible_external_handle_types           = static_cast<Anvil::ExternalFenceHandleTypeFlagBits>(in_external_fence_props.compatibleHandleTypes);
    export_from_imported_external_handle_types = static_cast<Anvil::ExternalFenceHandleTypeFlagBits>(in_external_fence_props.exportFromImportedHandleTypes);
    is_exportable                              = (in_external_fence_props.externalFenceFeatures & VK_EXTERNAL_FENCE_FEATURE_EXPORTABLE_BIT_KHR) != 0;
    is_importable                              = (in_external_fence_props.externalFenceFeatures & VK_EXTERNAL_FENCE_FEATURE_IMPORTABLE_BIT_KHR) != 0;
}

Anvil::ExternalMemoryProperties::ExternalMemoryProperties()
{
    is_exportable = false;
    is_importable = false;
}

Anvil::ExternalMemoryProperties::ExternalMemoryProperties(const VkExternalMemoryPropertiesKHR& in_external_memory_props)
{
    compatible_external_handle_types           = static_cast<Anvil::ExternalMemoryHandleTypeFlagBits>(in_external_memory_props.compatibleHandleTypes);
    export_from_imported_external_handle_types = static_cast<Anvil::ExternalMemoryHandleTypeFlagBits>(in_external_memory_props.exportFromImportedHandleTypes);
    is_exportable                              = (in_external_memory_props.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHR) != 0;
    is_importable                              = (in_external_memory_props.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_KHR) != 0;
}

Anvil::ExternalSemaphoreProperties::ExternalSemaphoreProperties()
{
    is_exportable = false;
    is_importable = false;
}

Anvil::ExternalSemaphoreProperties::ExternalSemaphoreProperties(const VkExternalSemaphorePropertiesKHR& in_external_semaphore_props)
{
    compatible_external_handle_types           = static_cast<Anvil::ExternalSemaphoreHandleTypeFlagBits>(in_external_semaphore_props.compatibleHandleTypes);
    export_from_imported_external_handle_types = static_cast<Anvil::ExternalSemaphoreHandleTypeFlagBits>(in_external_semaphore_props.exportFromImportedHandleTypes);
    is_exportable                              = (in_external_semaphore_props.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT_KHR) != 0;
    is_importable                              = (in_external_semaphore_props.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT_KHR) != 0;
}

Anvil::FenceProperties::FenceProperties()
{
    /* Stub */
}

Anvil::FenceProperties::FenceProperties(const ExternalFenceProperties& in_external_fence_properties)
    :external_fence_properties(in_external_fence_properties)
{
    /* Stub */
}

Anvil::FormatProperties::FormatProperties()
{
    memset(this,
           0,
           sizeof(*this) );
}

Anvil::FormatProperties::FormatProperties(const VkFormatProperties& in_format_props)
{
    buffer_capabilities         = static_cast<Anvil::FormatFeatureFlagBits>(in_format_props.bufferFeatures);
    linear_tiling_capabilities  = static_cast<Anvil::FormatFeatureFlagBits>(in_format_props.linearTilingFeatures);
    optimal_tiling_capabilities = static_cast<Anvil::FormatFeatureFlagBits>(in_format_props.optimalTilingFeatures);
}

Anvil::ImageFormatProperties::ImageFormatProperties()
    :max_resource_size                        (0),
     n_combined_image_sampler_descriptors_used(0),
     n_max_array_layers                       (0),
     n_max_mip_levels                         (0),
     sample_counts                            (Anvil::SampleCountFlagBits::NONE),
     supports_amd_texture_gather_bias_lod     (false),
     valid_stencil_aspect_image_usage_flags   (Anvil::ImageUsageFlagBits::NONE)
{
    max_extent.depth  = 0;
    max_extent.height = 0;
    max_extent.width  = 0;
}

Anvil::ImageFormatProperties::ImageFormatProperties(const VkImageFormatProperties&  in_image_format_props,
                                                    const bool&                     in_supports_amd_texture_gather_bias_lod,
                                                    const ExternalMemoryProperties& in_external_handle_properties,
                                                    const Anvil::ImageUsageFlags&   in_valid_stencil_aspect_image_usage_flags,
                                                    const uint32_t&                 in_n_combined_image_sampler_descriptors_used)
{
    external_handle_properties                = in_external_handle_properties;
    max_extent                                = in_image_format_props.maxExtent;
    max_resource_size                         = in_image_format_props.maxResourceSize;
    n_combined_image_sampler_descriptors_used = in_n_combined_image_sampler_descriptors_used;
    n_max_array_layers                        = in_image_format_props.maxArrayLayers;
    n_max_mip_levels                          = in_image_format_props.maxMipLevels;
    sample_counts                             = static_cast<Anvil::SampleCountFlagBits>(in_image_format_props.sampleCounts);
    supports_amd_texture_gather_bias_lod      = in_supports_amd_texture_gather_bias_lod;
    valid_stencil_aspect_image_usage_flags    = in_valid_stencil_aspect_image_usage_flags;
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
Anvil::ImageBarrier::ImageBarrier(Anvil::AccessFlags           in_source_access_mask,
                                  Anvil::AccessFlags           in_destination_access_mask,
                                  Anvil::ImageLayout           in_old_layout,
                                  Anvil::ImageLayout           in_new_layout,
                                  uint32_t                     in_src_queue_family_index,
                                  uint32_t                     in_dst_queue_family_index,
                                  Anvil::Image*                in_image_ptr,
                                  Anvil::ImageSubresourceRange in_image_subresource_range)
{
    dst_access_mask        = in_destination_access_mask;
    dst_queue_family_index = in_dst_queue_family_index;
    image                  = (in_image_ptr != VK_NULL_HANDLE) ? in_image_ptr->get_image()
                                                              : VK_NULL_HANDLE;
    image_ptr              = in_image_ptr;
    new_layout             = in_new_layout;
    old_layout             = in_old_layout;
    src_access_mask        = in_source_access_mask;
    src_queue_family_index = in_src_queue_family_index;
    subresource_range      = in_image_subresource_range;

    /* NOTE: Barriers referring to DS images must always specify both aspects. */
    {
        const auto image_format = in_image_ptr->get_create_info_ptr()->get_format();

        if (Anvil::Formats::has_depth_aspect  (image_format) &&
            Anvil::Formats::has_stencil_aspect(image_format) )
        {
            if (subresource_range.aspect_mask != (Anvil::ImageAspectFlagBits::DEPTH_BIT | Anvil::ImageAspectFlagBits::STENCIL_BIT) )
            {
                subresource_range.aspect_mask = (Anvil::ImageAspectFlagBits::DEPTH_BIT | Anvil::ImageAspectFlagBits::STENCIL_BIT);
            }
        }
    }

    image_barrier_vk.dstAccessMask       = in_destination_access_mask.get_vk();
    image_barrier_vk.dstQueueFamilyIndex = in_dst_queue_family_index;
    image_barrier_vk.image               = (in_image_ptr != nullptr) ? in_image_ptr->get_image()
                                                                     : VK_NULL_HANDLE;
    image_barrier_vk.newLayout           = static_cast<VkImageLayout>(in_new_layout);
    image_barrier_vk.oldLayout           = static_cast<VkImageLayout>(in_old_layout);
    image_barrier_vk.pNext               = nullptr;
    image_barrier_vk.srcAccessMask       = in_source_access_mask.get_vk();
    image_barrier_vk.srcQueueFamilyIndex = in_src_queue_family_index;
    image_barrier_vk.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier_vk.subresourceRange    = subresource_range.get_vk();

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

bool Anvil::ImageBarrier::operator==(const ImageBarrier& in_barrier) const
{
    bool result = true;

    result &= (dst_access_mask == in_barrier.dst_access_mask);
    result &= (src_access_mask == in_barrier.src_access_mask);

    result &= (dst_queue_family_index == in_barrier.dst_queue_family_index);
    result &= (image                  == in_barrier.image);
    result &= (image_ptr              == in_barrier.image_ptr);
    result &= (new_layout             == in_barrier.new_layout);
    result &= (old_layout             == in_barrier.old_layout);
    result &= (src_queue_family_index == in_barrier.src_queue_family_index);

    result &= (subresource_range == in_barrier.subresource_range);

    return result;
}

bool Anvil::ImageSubresourceRange::operator==(const Anvil::ImageSubresourceRange& in_subresource_range) const
{
    return (aspect_mask      == in_subresource_range.aspect_mask      &&
            base_mip_level   == in_subresource_range.base_mip_level   &&
            level_count      == in_subresource_range.level_count      &&
            base_array_layer == in_subresource_range.base_array_layer &&
            layer_count      == in_subresource_range.layer_count);
}

Anvil::KHRSamplerYCbCrConversionFeatures::KHRSamplerYCbCrConversionFeatures()
    :sampler_ycbcr_conversion(false)
{
    /* Stub */
}

Anvil::KHRSamplerYCbCrConversionFeatures::KHRSamplerYCbCrConversionFeatures(const VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR& in_features)
    :sampler_ycbcr_conversion(VK_BOOL32_TO_BOOL(in_features.samplerYcbcrConversion) )
{
    /* Stub */
}

VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR Anvil::KHRSamplerYCbCrConversionFeatures::get_vk_physical_device_sampler_ycbcr_conversion_features() const
{
    VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR result;

    result.pNext                  = nullptr;
    result.samplerYcbcrConversion = BOOL_TO_VK_BOOL32(sampler_ycbcr_conversion);
    result.sType                  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES_KHR;

    return result;
}

bool Anvil::KHRSamplerYCbCrConversionFeatures::operator==(const Anvil::KHRSamplerYCbCrConversionFeatures& in_features) const
{
    return (in_features.sampler_ycbcr_conversion == sampler_ycbcr_conversion);
}

Anvil::MemoryBudget::MemoryBudget()
{
    heap_budget.fill(0);
    heap_usage.fill (0);
}

Anvil::MemoryBudget::MemoryBudget(const VkPhysicalDeviceMemoryBudgetPropertiesEXT& in_properties)
{
    anvil_assert(heap_budget.size() == sizeof(in_properties.heapBudget) / sizeof(in_properties.heapBudget[0]));
    anvil_assert(heap_usage.size()  == sizeof(in_properties.heapUsage)  / sizeof(in_properties.heapUsage[0]));

    memcpy(heap_budget.data(),
           in_properties.heapBudget,
           sizeof(in_properties.heapBudget));

    memcpy(heap_usage.data(),
           in_properties.heapUsage,
           sizeof(in_properties.heapUsage));
}

Anvil::EXTMemoryPriorityFeatures::EXTMemoryPriorityFeatures()
{
    is_memory_priority_supported = false;
}

Anvil::EXTMemoryPriorityFeatures::EXTMemoryPriorityFeatures(const VkPhysicalDeviceMemoryPriorityFeaturesEXT& in_features)
{
    is_memory_priority_supported = VK_BOOL32_TO_BOOL(in_features.memoryPriority);
}

VkPhysicalDeviceMemoryPriorityFeaturesEXT Anvil::EXTMemoryPriorityFeatures::get_vk_physical_device_memory_priority_features() const
{
    VkPhysicalDeviceMemoryPriorityFeaturesEXT result;

    result.sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;
    result.pNext          = nullptr;
    result.memoryPriority = BOOL_TO_VK_BOOL32(is_memory_priority_supported);

    return result;
}

bool Anvil::EXTMemoryPriorityFeatures::operator==(const EXTMemoryPriorityFeatures& in_memory_priority_features) const
{
    return (is_memory_priority_supported == in_memory_priority_features.is_memory_priority_supported);
}

Anvil::KHR16BitStorageFeatures::KHR16BitStorageFeatures()
{
    is_input_output_storage_supported                     = false;
    is_push_constant_16_bit_storage_supported             = false;
    is_storage_buffer_16_bit_access_supported             = false;
    is_uniform_and_storage_buffer_16_bit_access_supported = false;
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

VkPhysicalDevice16BitStorageFeaturesKHR Anvil::KHR16BitStorageFeatures::get_vk_physical_device_16_bit_storage_features() const
{
    VkPhysicalDevice16BitStorageFeaturesKHR result;

    result.pNext                              = nullptr;
    result.storageBuffer16BitAccess           = BOOL_TO_VK_BOOL32(is_storage_buffer_16_bit_access_supported);
    result.storageInputOutput16               = BOOL_TO_VK_BOOL32(is_input_output_storage_supported);
    result.storagePushConstant16              = BOOL_TO_VK_BOOL32(is_push_constant_16_bit_storage_supported);
    result.sType                              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR;
    result.uniformAndStorageBuffer16BitAccess = BOOL_TO_VK_BOOL32(is_uniform_and_storage_buffer_16_bit_access_supported);

    return result;

}

Anvil::KHR8BitStorageFeatures::KHR8BitStorageFeatures()
{
    storage_buffer_8_bit_access             = false;
    storage_push_constant_8                 = false;
    uniform_and_storage_buffer_8_bit_access = false;
}

Anvil::KHR8BitStorageFeatures::KHR8BitStorageFeatures(const VkPhysicalDevice8BitStorageFeaturesKHR& in_features)
{
    storage_buffer_8_bit_access             = (in_features.storageBuffer8BitAccess           == VK_TRUE);
    storage_push_constant_8                 = (in_features.storagePushConstant8              == VK_TRUE);
    uniform_and_storage_buffer_8_bit_access = (in_features.uniformAndStorageBuffer8BitAccess == VK_TRUE);
}

VkPhysicalDevice8BitStorageFeaturesKHR Anvil::KHR8BitStorageFeatures::get_vk_physical_device_8_bit_storage_features() const
{
    VkPhysicalDevice8BitStorageFeaturesKHR result;

    result.pNext                             = nullptr;
    result.storageBuffer8BitAccess           = (storage_buffer_8_bit_access) ? VK_TRUE : VK_FALSE;
    result.storagePushConstant8              = (storage_push_constant_8)     ? VK_TRUE : VK_FALSE;
    result.sType                             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR;
    result.uniformAndStorageBuffer8BitAccess = (uniform_and_storage_buffer_8_bit_access) ? VK_TRUE : VK_FALSE;

    return result;
}

bool Anvil::KHR8BitStorageFeatures::operator==(const Anvil::KHR8BitStorageFeatures& in_features) const
{
    return (storage_buffer_8_bit_access             == in_features.storage_buffer_8_bit_access             &&
            storage_push_constant_8                 == in_features.storage_push_constant_8                 &&
            uniform_and_storage_buffer_8_bit_access == in_features.uniform_and_storage_buffer_8_bit_access);
}

Anvil::KHRDepthStencilResolveProperties::KHRDepthStencilResolveProperties()
{
    independent_resolve             = false;
    independent_resolve_none        = false;
    supported_depth_resolve_modes   = Anvil::ResolveModeFlagBits::NONE;
    supported_stencil_resolve_modes = Anvil::ResolveModeFlagBits::NONE;
}

Anvil::KHRDepthStencilResolveProperties::KHRDepthStencilResolveProperties(const VkPhysicalDeviceDepthStencilResolvePropertiesKHR& in_properties)
{
    independent_resolve             = (in_properties.independentResolve     == VK_TRUE);
    independent_resolve_none        = (in_properties.independentResolveNone == VK_TRUE);
    supported_depth_resolve_modes   = static_cast<Anvil::ResolveModeFlagBits>(in_properties.supportedDepthResolveModes);
    supported_stencil_resolve_modes = static_cast<Anvil::ResolveModeFlagBits>(in_properties.supportedStencilResolveModes);
}

bool Anvil::KHRDepthStencilResolveProperties::operator==(const Anvil::KHRDepthStencilResolveProperties& in_props) const
{
    return (in_props.independent_resolve             == independent_resolve)             &&
           (in_props.independent_resolve_none        == independent_resolve_none)        &&
           (in_props.supported_depth_resolve_modes   == supported_depth_resolve_modes)   &&
           (in_props.supported_stencil_resolve_modes == supported_stencil_resolve_modes);
}


Anvil::KHRDriverPropertiesProperties::KHRDriverPropertiesProperties()
    :driver_id(Anvil::DriverIdKHR::UNKNOWN)
{
    memset(driver_info,
           0,
           sizeof(driver_info) );
    memset(driver_name,
           0,
           sizeof(driver_name) );
}

Anvil::KHRDriverPropertiesProperties::KHRDriverPropertiesProperties(const VkPhysicalDeviceDriverPropertiesKHR& in_properties)
{
    static_assert(sizeof(driver_info) == sizeof(in_properties.driverInfo), "Field size mismatch");
    static_assert(sizeof(driver_name) == sizeof(in_properties.driverName), "Field size mismatch");

    conformance_version = Anvil::ConformanceVersionKHR   (in_properties.conformanceVersion);
    driver_id           = static_cast<Anvil::DriverIdKHR>(in_properties.driverID);

    memcpy(driver_info,
           in_properties.driverInfo,
           sizeof(driver_info) );
    memcpy(driver_name,
           in_properties.driverName,
           sizeof(driver_name) );
}

bool Anvil::KHRDriverPropertiesProperties::operator==(const KHRDriverPropertiesProperties& in_props) const
{
    bool result = false;

    if (!(in_props.conformance_version == conformance_version) )
    {
        goto end;
    }

    if (in_props.driver_id != driver_id)
    {
        goto end;
    }

    if (memcmp(in_props.driver_info,
               driver_info,
               sizeof(driver_info) ) != 0)
    {
        goto end;
    }

    if (memcmp(in_props.driver_name,
               driver_name,
               sizeof(driver_name) ) != 0)
    {
        goto end;
    }

    result = true;
end:
    return result;
}


Anvil::KHRFloat16Int8Features::KHRFloat16Int8Features()
{
    shader_float16 = false;
    shader_int8    = false;
}

Anvil::KHRFloat16Int8Features::KHRFloat16Int8Features(const VkPhysicalDeviceFloat16Int8FeaturesKHR& in_features)
{
    shader_float16  = (in_features.shaderFloat16   == VK_TRUE);
    shader_int8     = (in_features.shaderInt8      == VK_TRUE);
}

VkPhysicalDeviceFloat16Int8FeaturesKHR Anvil::KHRFloat16Int8Features::get_vk_physical_device_float16_int8_features() const
{
    VkPhysicalDeviceFloat16Int8FeaturesKHR result;

    result.pNext            = nullptr;
    result.shaderFloat16    = (shader_float16) ? VK_TRUE : VK_FALSE;
    result.shaderInt8       = (shader_int8)    ? VK_TRUE : VK_FALSE;
    result.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;

    return result;
}

bool Anvil::KHRFloat16Int8Features::operator==(const Anvil::KHRFloat16Int8Features& in_features) const
{
    return (shader_float16  == in_features.shader_float16 &&
            shader_int8     == in_features.shader_int8);
}

Anvil::KHRMaintenance2PhysicalDevicePointClippingProperties::KHRMaintenance2PhysicalDevicePointClippingProperties()
    :point_clipping_behavior(PointClippingBehavior::UNKNOWN)
{
    /* Stub */
}

Anvil::KHRMaintenance2PhysicalDevicePointClippingProperties::KHRMaintenance2PhysicalDevicePointClippingProperties(const VkPhysicalDevicePointClippingPropertiesKHR& in_props)
    :point_clipping_behavior(static_cast<PointClippingBehavior>(in_props.pointClippingBehavior) )
{
    /* Stub */
}

bool Anvil::KHRMaintenance2PhysicalDevicePointClippingProperties::operator==(const Anvil::KHRMaintenance2PhysicalDevicePointClippingProperties& in_props) const
{
    return (in_props.point_clipping_behavior == point_clipping_behavior);
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

Anvil::KHRMultiviewFeatures::KHRMultiviewFeatures()
{
    multiview                     = false;
    multiview_geometry_shader     = false;
    multiview_tessellation_shader = false;
}

Anvil::KHRMultiviewFeatures::KHRMultiviewFeatures(const VkPhysicalDeviceMultiviewFeatures& in_features)
{
    multiview                     = (in_features.multiview                   == VK_TRUE);
    multiview_geometry_shader     = (in_features.multiviewGeometryShader     == VK_TRUE);
    multiview_tessellation_shader = (in_features.multiviewTessellationShader == VK_TRUE);
}

VkPhysicalDeviceMultiviewFeatures Anvil::KHRMultiviewFeatures::get_vk_physical_device_multiview_features() const
{
    VkPhysicalDeviceMultiviewFeatures result;

    result.multiview                   = (multiview)                     ? VK_TRUE : VK_FALSE;
    result.multiviewGeometryShader     = (multiview_geometry_shader)     ? VK_TRUE : VK_FALSE;
    result.multiviewTessellationShader = (multiview_tessellation_shader) ? VK_TRUE : VK_FALSE;
    result.pNext                       = nullptr;
    result.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;

    return result;
}

bool Anvil::KHRMultiviewFeatures::operator==(const Anvil::KHRMultiviewFeatures& in_features) const
{
    return (in_features.multiview                     == multiview                     &&
            in_features.multiview_geometry_shader     == multiview_geometry_shader     &&
            in_features.multiview_tessellation_shader == multiview_tessellation_shader);
}

Anvil::KHRMultiviewProperties::KHRMultiviewProperties()
{
    max_multiview_instance_index = 0;
    max_multiview_view_count     = 0;
}

Anvil::KHRMultiviewProperties::KHRMultiviewProperties(const VkPhysicalDeviceMultiviewPropertiesKHR& in_props)
{
    max_multiview_instance_index = in_props.maxMultiviewInstanceIndex;
    max_multiview_view_count     = in_props.maxMultiviewViewCount;
}

bool Anvil::KHRMultiviewProperties::operator==(const KHRMultiviewProperties& in_props) const
{
    return (max_multiview_instance_index == in_props.max_multiview_instance_index &&
            max_multiview_view_count     == in_props.max_multiview_view_count);
}

Anvil::KHRShaderAtomicInt64Features::KHRShaderAtomicInt64Features()
    :shader_buffer_int64_atomics(false),
     shader_shared_int64_atomics(false)
{
    /* Stub */
}

Anvil::KHRShaderAtomicInt64Features::KHRShaderAtomicInt64Features(const VkPhysicalDeviceShaderAtomicInt64FeaturesKHR& in_features)
{
    shader_buffer_int64_atomics = VK_BOOL32_TO_BOOL(in_features.shaderBufferInt64Atomics);
    shader_shared_int64_atomics = VK_BOOL32_TO_BOOL(in_features.shaderSharedInt64Atomics);
}

VkPhysicalDeviceShaderAtomicInt64FeaturesKHR Anvil::KHRShaderAtomicInt64Features::get_vk_physical_device_shader_atomic_int64_features() const
{
    VkPhysicalDeviceShaderAtomicInt64FeaturesKHR result;

    result.pNext                    = nullptr;
    result.shaderBufferInt64Atomics = BOOL_TO_VK_BOOL32(shader_buffer_int64_atomics);
    result.shaderSharedInt64Atomics = BOOL_TO_VK_BOOL32(shader_shared_int64_atomics);
    result.sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES_KHR;

    return result;
}

bool Anvil::KHRShaderAtomicInt64Features::operator==(const KHRShaderAtomicInt64Features& in_features) const
{
    return (in_features.shader_buffer_int64_atomics == shader_buffer_int64_atomics) &&
           (in_features.shader_shared_int64_atomics == shader_shared_int64_atomics);
}

Anvil::KHRShaderFloatControlsProperties::KHRShaderFloatControlsProperties()
    :separate_denorm_settings                   (false),
     separate_rounding_mode_settings            (false),
     shader_denorm_flush_to_zero_float16        (false),
     shader_denorm_flush_to_zero_float32        (false),
     shader_denorm_flush_to_zero_float64        (false),
     shader_denorm_preserve_float16             (false),
     shader_denorm_preserve_float32             (false),
     shader_denorm_preserve_float64             (false),
     shader_rounding_mode_RTE_float16           (false),
     shader_rounding_mode_RTE_float32           (false),
     shader_rounding_mode_RTE_float64           (false),
     shader_rounding_mode_RTZ_float16           (false),
     shader_rounding_mode_RTZ_float32           (false),
     shader_rounding_mode_RTZ_float64           (false),
     shader_signed_zero_inf_nan_preserve_float16(false),
     shader_signed_zero_inf_nan_preserve_float32(false),
     shader_signed_zero_inf_nan_preserve_float64(false)
{
    /* Stub */
}

Anvil::KHRShaderFloatControlsProperties::KHRShaderFloatControlsProperties(const VkPhysicalDeviceFloatControlsPropertiesKHR& in_properties)
    :separate_denorm_settings                   (VK_BOOL32_TO_BOOL(in_properties.separateDenormSettings) ),
     separate_rounding_mode_settings            (VK_BOOL32_TO_BOOL(in_properties.separateRoundingModeSettings) ),
     shader_denorm_flush_to_zero_float16        (VK_BOOL32_TO_BOOL(in_properties.shaderDenormFlushToZeroFloat16) ),
     shader_denorm_flush_to_zero_float32        (VK_BOOL32_TO_BOOL(in_properties.shaderDenormFlushToZeroFloat32) ),
     shader_denorm_flush_to_zero_float64        (VK_BOOL32_TO_BOOL(in_properties.shaderDenormFlushToZeroFloat64) ),
     shader_denorm_preserve_float16             (VK_BOOL32_TO_BOOL(in_properties.shaderDenormPreserveFloat16) ),
     shader_denorm_preserve_float32             (VK_BOOL32_TO_BOOL(in_properties.shaderDenormPreserveFloat32) ),
     shader_denorm_preserve_float64             (VK_BOOL32_TO_BOOL(in_properties.shaderDenormPreserveFloat64) ),
     shader_rounding_mode_RTE_float16           (VK_BOOL32_TO_BOOL(in_properties.shaderRoundingModeRTEFloat16) ),
     shader_rounding_mode_RTE_float32           (VK_BOOL32_TO_BOOL(in_properties.shaderRoundingModeRTEFloat32) ),
     shader_rounding_mode_RTE_float64           (VK_BOOL32_TO_BOOL(in_properties.shaderRoundingModeRTEFloat64) ),
     shader_rounding_mode_RTZ_float16           (VK_BOOL32_TO_BOOL(in_properties.shaderRoundingModeRTZFloat16) ),
     shader_rounding_mode_RTZ_float32           (VK_BOOL32_TO_BOOL(in_properties.shaderRoundingModeRTZFloat32) ),
     shader_rounding_mode_RTZ_float64           (VK_BOOL32_TO_BOOL(in_properties.shaderRoundingModeRTZFloat64) ),
     shader_signed_zero_inf_nan_preserve_float16(VK_BOOL32_TO_BOOL(in_properties.shaderSignedZeroInfNanPreserveFloat16) ),
     shader_signed_zero_inf_nan_preserve_float32(VK_BOOL32_TO_BOOL(in_properties.shaderSignedZeroInfNanPreserveFloat32) ),
     shader_signed_zero_inf_nan_preserve_float64(VK_BOOL32_TO_BOOL(in_properties.shaderSignedZeroInfNanPreserveFloat64) )
{
    /* Stub */
}

VkPhysicalDeviceFloatControlsPropertiesKHR Anvil::KHRShaderFloatControlsProperties::get_vk_physical_device_float_controls_properties() const
{
    VkPhysicalDeviceFloatControlsPropertiesKHR result;

    result.pNext                                 = nullptr;
    result.sType                                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES_KHR;
    result.separateDenormSettings                = separate_denorm_settings;
    result.separateRoundingModeSettings          = separate_rounding_mode_settings;
    result.shaderDenormFlushToZeroFloat16        = shader_denorm_flush_to_zero_float16;
    result.shaderDenormFlushToZeroFloat32        = shader_denorm_flush_to_zero_float32;
    result.shaderDenormFlushToZeroFloat64        = shader_denorm_flush_to_zero_float64;
    result.shaderDenormPreserveFloat16           = shader_denorm_preserve_float16;
    result.shaderDenormPreserveFloat32           = shader_denorm_preserve_float32;
    result.shaderDenormPreserveFloat64           = shader_denorm_preserve_float64;
    result.shaderRoundingModeRTEFloat16          = shader_rounding_mode_RTE_float16;
    result.shaderRoundingModeRTEFloat32          = shader_rounding_mode_RTE_float32;
    result.shaderRoundingModeRTEFloat64          = shader_rounding_mode_RTE_float64;
    result.shaderRoundingModeRTZFloat16          = shader_rounding_mode_RTZ_float16;
    result.shaderRoundingModeRTZFloat32          = shader_rounding_mode_RTZ_float32;
    result.shaderRoundingModeRTZFloat64          = shader_rounding_mode_RTZ_float64;
    result.shaderSignedZeroInfNanPreserveFloat16 = shader_signed_zero_inf_nan_preserve_float16;
    result.shaderSignedZeroInfNanPreserveFloat32 = shader_signed_zero_inf_nan_preserve_float32;
    result.shaderSignedZeroInfNanPreserveFloat64 = shader_signed_zero_inf_nan_preserve_float64;

    return result;
}

bool Anvil::KHRShaderFloatControlsProperties::operator==(const KHRShaderFloatControlsProperties& in_properties) const
{
    return
        (separate_denorm_settings                    == in_properties.separate_denorm_settings)                    &&
        (separate_rounding_mode_settings             == in_properties.separate_rounding_mode_settings)             &&
        (shader_denorm_flush_to_zero_float16         == in_properties.shader_denorm_flush_to_zero_float16)         &&
        (shader_denorm_flush_to_zero_float32         == in_properties.shader_denorm_flush_to_zero_float32)         &&
        (shader_denorm_flush_to_zero_float64         == in_properties.shader_denorm_flush_to_zero_float64)         &&
        (shader_denorm_preserve_float16              == in_properties.shader_denorm_preserve_float16)              &&
        (shader_denorm_preserve_float32              == in_properties.shader_denorm_preserve_float32)              &&
        (shader_denorm_preserve_float64              == in_properties.shader_denorm_preserve_float64)              &&
        (shader_rounding_mode_RTE_float16            == in_properties.shader_rounding_mode_RTE_float16)            &&
        (shader_rounding_mode_RTE_float32            == in_properties.shader_rounding_mode_RTE_float32)            &&
        (shader_rounding_mode_RTE_float64            == in_properties.shader_rounding_mode_RTE_float64)            &&
        (shader_rounding_mode_RTZ_float16            == in_properties.shader_rounding_mode_RTZ_float16)            &&
        (shader_rounding_mode_RTZ_float32            == in_properties.shader_rounding_mode_RTZ_float32)            &&
        (shader_rounding_mode_RTZ_float64            == in_properties.shader_rounding_mode_RTZ_float64)            &&
        (shader_signed_zero_inf_nan_preserve_float16 == in_properties.shader_signed_zero_inf_nan_preserve_float16) &&
        (shader_signed_zero_inf_nan_preserve_float32 == in_properties.shader_signed_zero_inf_nan_preserve_float32) &&
        (shader_signed_zero_inf_nan_preserve_float64 == in_properties.shader_signed_zero_inf_nan_preserve_float64);
}

Anvil::KHRVariablePointerFeatures::KHRVariablePointerFeatures()
{
    variable_pointers                = false;
    variable_pointers_storage_buffer = false;
}

Anvil::KHRVariablePointerFeatures::KHRVariablePointerFeatures(const VkPhysicalDeviceVariablePointerFeatures& in_features)
{
    variable_pointers                = (in_features.variablePointers              == VK_TRUE);
    variable_pointers_storage_buffer = (in_features.variablePointersStorageBuffer == VK_TRUE);
}

VkPhysicalDeviceVariablePointerFeaturesKHR Anvil::KHRVariablePointerFeatures::get_vk_physical_device_variable_pointer_features() const
{
    VkPhysicalDeviceVariablePointerFeaturesKHR result;

    result.pNext                         = nullptr;
    result.sType                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES_KHR;
    result.variablePointers              = (variable_pointers)                ? VK_TRUE : VK_FALSE;
    result.variablePointersStorageBuffer = (variable_pointers_storage_buffer) ? VK_TRUE : VK_FALSE;

    return result;
}

bool Anvil::KHRVariablePointerFeatures::operator==(const Anvil::KHRVariablePointerFeatures& in_props) const
{
    return (variable_pointers                == in_props.variable_pointers                  &&
            variable_pointers_storage_buffer == in_props.variable_pointers_storage_buffer);
}

Anvil::KHRVulkanMemoryModelFeatures::KHRVulkanMemoryModelFeatures()
{
    vulkan_memory_model                                = false;
    vulkan_memory_model_availability_visibility_chains = false;
    vulkan_memory_model_device_scope                   = false;
}

Anvil::KHRVulkanMemoryModelFeatures::KHRVulkanMemoryModelFeatures(const VkPhysicalDeviceVulkanMemoryModelFeaturesKHR& in_features)
{
    vulkan_memory_model                                = VK_BOOL32_TO_BOOL(in_features.vulkanMemoryModel);
    vulkan_memory_model_availability_visibility_chains = VK_BOOL32_TO_BOOL(in_features.vulkanMemoryModelAvailabilityVisibilityChains);
    vulkan_memory_model_device_scope                   = VK_BOOL32_TO_BOOL(in_features.vulkanMemoryModelDeviceScope);
}

VkPhysicalDeviceVulkanMemoryModelFeaturesKHR Anvil::KHRVulkanMemoryModelFeatures::get_vk_physical_device_vulkan_memory_model_features() const
{
    VkPhysicalDeviceVulkanMemoryModelFeaturesKHR result;

    result.pNext                                         = nullptr;
    result.sType                                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES_KHR;
    result.vulkanMemoryModel                             = BOOL_TO_VK_BOOL32(vulkan_memory_model);
    result.vulkanMemoryModelAvailabilityVisibilityChains = BOOL_TO_VK_BOOL32(vulkan_memory_model_availability_visibility_chains);
    result.vulkanMemoryModelDeviceScope                  = BOOL_TO_VK_BOOL32(vulkan_memory_model_device_scope);

    return result;
}

bool Anvil::KHRVulkanMemoryModelFeatures::operator==(const KHRVulkanMemoryModelFeatures& in_features) const
{
    return (in_features.vulkan_memory_model                                == vulkan_memory_model)                                &&
           (in_features.vulkan_memory_model_availability_visibility_chains == vulkan_memory_model_availability_visibility_chains) &&
           (in_features.vulkan_memory_model_device_scope                   == vulkan_memory_model_device_scope);
}

Anvil::Layer::Layer(const std::string& in_layer_name)
{
    implementation_version = 0;
    name                   = in_layer_name;
    spec_version           = 0;
}

Anvil::Layer::Layer(const VkLayerProperties& in_layer_props)
{
    description            = in_layer_props.description;
    implementation_version = in_layer_props.implementationVersion;
    name                   = in_layer_props.layerName;
    spec_version           = in_layer_props.specVersion;
}

bool Anvil::Layer::operator==(const std::string& in_layer_name) const
{
    return name == in_layer_name;
}

Anvil::MemoryBarrier::MemoryBarrier(Anvil::AccessFlags in_destination_access_mask,
                                    Anvil::AccessFlags in_source_access_mask)
{
    destination_access_mask = in_destination_access_mask;
    source_access_mask      = in_source_access_mask;

    memory_barrier_vk.dstAccessMask = destination_access_mask.get_vk();
    memory_barrier_vk.pNext         = nullptr;
    memory_barrier_vk.srcAccessMask = source_access_mask.get_vk();
    memory_barrier_vk.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
}

Anvil::MemoryHeap::MemoryHeap()
{
    index = UINT32_MAX;
    size  = 0;
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

Anvil::MemoryProperties::MemoryProperties()
{
    heaps   = nullptr;
    n_heaps = 0;
}

/** Destructor */
Anvil::MemoryProperties::~MemoryProperties()
{
    delete [] heaps;

    heaps = nullptr;
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
        heaps[n_heap].flags = static_cast<Anvil::MemoryHeapFlagBits>(in_mem_properties.memoryHeaps[n_heap].flags);
        heaps[n_heap].index = n_heap;
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
    flags    = static_cast<Anvil::MemoryPropertyFlagBits>(in_type.propertyFlags);
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D(Anvil::ImageAspectFlagBits in_aspect,
                                                     uint32_t                   in_n_mipmap,
                                                     uint32_t                   in_row_size)
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array(Anvil::ImageAspectFlagBits in_aspect,
                                                           uint32_t                   in_n_layer,
                                                           uint32_t                   in_n_layers,
                                                           uint32_t                   in_n_mipmap,
                                                           uint32_t                   in_row_size,
                                                           uint32_t                   in_data_size)
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D(Anvil::ImageAspectFlagBits in_aspect,
                                                     uint32_t                   in_n_mipmap,
                                                     uint32_t                   in_data_size,
                                                     uint32_t                   in_row_size)
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array(Anvil::ImageAspectFlagBits in_aspect,
                                                           uint32_t                   in_n_layer,
                                                           uint32_t                   in_n_layers,
                                                           uint32_t                   in_n_mipmap,
                                                           uint32_t                   in_data_size,
                                                           uint32_t                   in_row_size)
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D(Anvil::ImageAspectFlagBits in_aspect,
                                                     uint32_t                   in_n_layer,
                                                     uint32_t                   in_n_slices,
                                                     uint32_t                   in_n_mipmap,
                                                     uint32_t                   in_data_size,
                                                     uint32_t                   in_row_size)
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_from_uchar_ptr(Anvil::ImageAspectFlagBits     in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_from_uchar_ptr(Anvil::ImageAspectFlagBits in_aspect,
                                                                    uint32_t                   in_n_mipmap,
                                                                    const unsigned char*       in_linear_tightly_packed_data_ptr,
                                                                    uint32_t                   in_row_size)
{
    MipmapRawData result = create_1D(in_aspect,
                                     in_n_mipmap,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array_from_uchar_ptr(Anvil::ImageAspectFlagBits     in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array_from_uchar_ptr(Anvil::ImageAspectFlagBits in_aspect,
                                                                          uint32_t                   in_n_layer,
                                                                          uint32_t                   in_n_layers,
                                                                          uint32_t                   in_n_mipmap,
                                                                          const unsigned char*       in_linear_tightly_packed_data_ptr,
                                                                          uint32_t                   in_row_size,
                                                                          uint32_t                   in_data_size)
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_1D_array_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_from_uchar_ptr(Anvil::ImageAspectFlagBits     in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_from_uchar_ptr(Anvil::ImageAspectFlagBits in_aspect,
                                                                    uint32_t                   in_n_mipmap,
                                                                    const unsigned char*       in_linear_tightly_packed_data_ptr,
                                                                    uint32_t                   in_data_size,
                                                                    uint32_t                   in_row_size)
{
    MipmapRawData result = create_2D(in_aspect,
                                     in_n_mipmap,
                                     in_data_size,
                                     in_row_size);

    result.linear_tightly_packed_data_uchar_raw_ptr = in_linear_tightly_packed_data_ptr;

    return result;
}

/* Please see header for specification */
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array_from_uchar_ptr(Anvil::ImageAspectFlagBits     in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array_from_uchar_ptr(Anvil::ImageAspectFlagBits in_aspect,
                                                                          uint32_t                   in_n_layer,
                                                                          uint32_t                   in_n_layers,
                                                                          uint32_t                   in_n_mipmap,
                                                                          const unsigned char*       in_linear_tightly_packed_data_ptr,
                                                                          uint32_t                   in_data_size,
                                                                          uint32_t                   in_row_size)
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_2D_array_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D_from_uchar_ptr(Anvil::ImageAspectFlagBits     in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D_from_uchar_ptr(Anvil::ImageAspectFlagBits in_aspect,
                                                                    uint32_t                   in_n_layer,
                                                                    uint32_t                   in_n_layer_slices,
                                                                    uint32_t                   in_n_mipmap,
                                                                    const unsigned char*       in_linear_tightly_packed_data_ptr,
                                                                    uint32_t                   in_slice_data_size,
                                                                    uint32_t                   in_row_size)
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_3D_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_from_uchar_ptr(Anvil::ImageAspectFlagBits     in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_from_uchar_ptr(Anvil::ImageAspectFlagBits in_aspect,
                                                                          uint32_t                   in_n_layer,
                                                                          uint32_t                   in_n_mipmap,
                                                                          const unsigned char*       in_linear_tightly_packed_data_ptr,
                                                                          uint32_t                   in_data_size,
                                                                          uint32_t                   in_row_size)
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_array_from_uchar_ptr(Anvil::ImageAspectFlagBits     in_aspect,
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_array_from_uchar_ptr(Anvil::ImageAspectFlagBits in_aspect,
                                                                                uint32_t                   in_n_layer,
                                                                                uint32_t                   in_n_layers,
                                                                                uint32_t                   in_n_mipmap,
                                                                                const unsigned char*       in_linear_tightly_packed_data_ptr,
                                                                                uint32_t                   in_data_size,
                                                                                uint32_t                   in_row_size)
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
Anvil::MipmapRawData Anvil::MipmapRawData::create_cube_map_array_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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

Anvil::PhysicalDeviceProperties::PhysicalDeviceProperties()
{
    amd_shader_core_properties_ptr                                     = nullptr;
    core_vk1_0_properties_ptr                                          = nullptr;
    core_vk1_1_properties_ptr                                          = nullptr;
    ext_conservative_rasterization_properties_ptr                      = nullptr;
    ext_descriptor_indexing_properties_ptr                             = nullptr;
    ext_external_memory_host_properties_ptr                            = nullptr;
    ext_inline_uniform_block_properties_ptr                            = nullptr;
    ext_pci_bus_info_properties_ptr                                    = nullptr;
    ext_sample_locations_properties_ptr                                = nullptr;
    ext_sampler_filter_minmax_properties_ptr                           = nullptr;
    ext_transform_feedback_properties_ptr                              = nullptr;
    ext_vertex_attribute_divisor_properties_ptr                        = nullptr;
    khr_depth_stencil_resolve_properties_ptr                           = nullptr;
    khr_driver_properties_properties_ptr                               = nullptr;
    khr_external_memory_capabilities_physical_device_id_properties_ptr = nullptr;
    khr_maintenance2_point_clipping_properties_ptr                     = nullptr;
    khr_maintenance3_properties_ptr                                    = nullptr;
    khr_multiview_properties_ptr                                       = nullptr;
    khr_shader_float_controls_properties_ptr                           = nullptr;
}

Anvil::PhysicalDeviceProperties::PhysicalDeviceProperties(const AMDShaderCoreProperties*                                        in_amd_shader_core_properties_ptr,
                                                          const PhysicalDevicePropertiesCoreVK10*                               in_core_vk1_0_properties_ptr,
                                                          const PhysicalDevicePropertiesCoreVK11*                               in_core_vk1_1_properties_ptr,
                                                          const EXTConservativeRasterizationProperties*                         in_ext_conservative_rasterization_properties_ptr,
                                                          const EXTDescriptorIndexingProperties*                                in_ext_descriptor_indexing_properties_ptr,
                                                          const EXTExternalMemoryHostProperties*                                in_ext_external_memory_host_properties_ptr,
                                                          const EXTInlineUniformBlockProperties*                                in_ext_inline_uniform_block_properties_ptr,
                                                          const EXTPCIBusInfoProperties*                                        in_ext_pci_bus_info_properties_ptr,
                                                          const EXTSampleLocationsProperties*                                   in_ext_sample_locations_properties_ptr,
                                                          const EXTSamplerFilterMinmaxProperties*                               in_ext_sampler_filter_minmax_properties_ptr,
                                                          const EXTTransformFeedbackProperties*                                 in_ext_transform_feedback_properties_ptr,
                                                          const EXTVertexAttributeDivisorProperties*                            in_ext_vertex_attribute_divisor_properties_ptr,
                                                          const Anvil::KHRDepthStencilResolveProperties*                        in_khr_depth_stencil_resolve_props_ptr,
                                                          const KHRDriverPropertiesProperties*                                  in_khr_driver_properties_props_ptr,
                                                          const Anvil::KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties* in_khr_external_memory_caps_physical_device_id_props_ptr,
                                                          const KHRMaintenance3Properties*                                      in_khr_maintenance3_properties_ptr,
                                                          const Anvil::KHRMaintenance2PhysicalDevicePointClippingProperties*    in_khr_maintenance2_point_clipping_properties_ptr,
                                                          const Anvil::KHRMultiviewProperties*                                  in_khr_multiview_properties_ptr,
                                                          const KHRShaderFloatControlsProperties*                               in_khr_shader_float_controls_properties_ptr)
    :amd_shader_core_properties_ptr                                    (in_amd_shader_core_properties_ptr),
     core_vk1_0_properties_ptr                                         (in_core_vk1_0_properties_ptr),
     core_vk1_1_properties_ptr                                         (in_core_vk1_1_properties_ptr),
     ext_conservative_rasterization_properties_ptr                     (in_ext_conservative_rasterization_properties_ptr),
     ext_descriptor_indexing_properties_ptr                            (in_ext_descriptor_indexing_properties_ptr),
     ext_external_memory_host_properties_ptr                           (in_ext_external_memory_host_properties_ptr),
     ext_inline_uniform_block_properties_ptr                           (in_ext_inline_uniform_block_properties_ptr),
     ext_pci_bus_info_properties_ptr                                   (in_ext_pci_bus_info_properties_ptr),
     ext_sample_locations_properties_ptr                               (in_ext_sample_locations_properties_ptr),
     ext_sampler_filter_minmax_properties_ptr                          (in_ext_sampler_filter_minmax_properties_ptr),
     ext_transform_feedback_properties_ptr                             (in_ext_transform_feedback_properties_ptr),
     ext_vertex_attribute_divisor_properties_ptr                       (in_ext_vertex_attribute_divisor_properties_ptr),
     khr_depth_stencil_resolve_properties_ptr                          (in_khr_depth_stencil_resolve_props_ptr),
     khr_driver_properties_properties_ptr                              (in_khr_driver_properties_props_ptr),
     khr_external_memory_capabilities_physical_device_id_properties_ptr(in_khr_external_memory_caps_physical_device_id_props_ptr),
     khr_maintenance2_point_clipping_properties_ptr                    (in_khr_maintenance2_point_clipping_properties_ptr),
     khr_maintenance3_properties_ptr                                   (in_khr_maintenance3_properties_ptr),
     khr_multiview_properties_ptr                                      (in_khr_multiview_properties_ptr),
     khr_shader_float_controls_properties_ptr                          (in_khr_shader_float_controls_properties_ptr)
{
    /* Stub */
}

bool Anvil::PhysicalDeviceProperties::operator==(const PhysicalDeviceProperties& in_props) const
{
    bool       amd_shader_core_properties_match                   = false;
    const bool core_vk1_0_features_match                          = (*core_vk1_0_properties_ptr == *in_props.core_vk1_0_properties_ptr);
    bool       core_vk1_1_features_match                          = false;
    bool       ext_conservative_rasterization_properties_match    = false;
    bool       ext_descriptor_indexing_properties_match           = false;
    bool       ext_external_memory_host_properties_match          = false;
    bool       ext_inline_uniform_block_properties_match          = false;
    bool       ext_pci_bus_info_properties_match                  = false;
    bool       ext_sample_locations_properties_match              = false;
    bool       ext_sampler_filter_minmax_properties_match         = false;
    bool       ext_vertex_attribute_divisor_properties_match      = false;
    bool       khr_depth_stencil_resolve_properties_match         = false;
    bool       khr_driver_properties_properties_match             = false;
    bool       khr_external_memory_capabilities_properties_match  = false;
    bool       khr_maintenance2_properties_match                  = false;
    bool       khr_maintenance3_properties_match                  = false;
    bool       khr_multiview_properties_match                     = false;
    bool       khr_shader_float_controls_properties_match         = false;

    if (amd_shader_core_properties_ptr          != nullptr &&
        in_props.amd_shader_core_properties_ptr != nullptr)
    {
        amd_shader_core_properties_match = (*amd_shader_core_properties_ptr == *in_props.amd_shader_core_properties_ptr);
    }
    else
    {
        amd_shader_core_properties_match = (amd_shader_core_properties_ptr          == nullptr &&
                                            in_props.amd_shader_core_properties_ptr == nullptr);
    }

    if (core_vk1_1_properties_ptr          != nullptr &&
        in_props.core_vk1_1_properties_ptr != nullptr)
    {
        core_vk1_1_features_match = (*core_vk1_1_properties_ptr == *in_props.core_vk1_1_properties_ptr);
    }
    else
    {
        core_vk1_1_features_match = (core_vk1_1_properties_ptr          == nullptr &&
                                     in_props.core_vk1_1_properties_ptr == nullptr);
    }

    if (ext_conservative_rasterization_properties_ptr          != nullptr &&
        in_props.ext_conservative_rasterization_properties_ptr != nullptr)
    {
        ext_conservative_rasterization_properties_match = (*ext_conservative_rasterization_properties_ptr == *in_props.ext_conservative_rasterization_properties_ptr);
    }
    else
    {
        ext_conservative_rasterization_properties_match = (ext_conservative_rasterization_properties_ptr          == nullptr &&
                                                           in_props.ext_conservative_rasterization_properties_ptr == nullptr);
    }

    if (ext_descriptor_indexing_properties_ptr          != nullptr &&
        in_props.ext_descriptor_indexing_properties_ptr != nullptr)
    {
        ext_descriptor_indexing_properties_match = (*ext_descriptor_indexing_properties_ptr == *in_props.ext_descriptor_indexing_properties_ptr);
    }
    else
    {
        ext_descriptor_indexing_properties_match = (ext_descriptor_indexing_properties_ptr          == nullptr &&
                                                    in_props.ext_descriptor_indexing_properties_ptr == nullptr);
    }

    if (ext_external_memory_host_properties_ptr          != nullptr &&
        in_props.ext_external_memory_host_properties_ptr != nullptr)
    {
        ext_external_memory_host_properties_match = (*ext_external_memory_host_properties_ptr == *in_props.ext_external_memory_host_properties_ptr);
    }
    else
    {
        ext_external_memory_host_properties_match = (ext_external_memory_host_properties_ptr          == nullptr &&
                                                     in_props.ext_external_memory_host_properties_ptr == nullptr);
    }

    if (ext_inline_uniform_block_properties_ptr != nullptr &&
        in_props.ext_inline_uniform_block_properties_ptr != nullptr)
    {
        ext_inline_uniform_block_properties_match = (*ext_inline_uniform_block_properties_ptr == *in_props.ext_inline_uniform_block_properties_ptr);
    }
    else
    {
        ext_inline_uniform_block_properties_match = (ext_inline_uniform_block_properties_ptr          == nullptr &&
                                                     in_props.ext_inline_uniform_block_properties_ptr == nullptr);
    }

    if (ext_pci_bus_info_properties_ptr          != nullptr &&
        in_props.ext_pci_bus_info_properties_ptr != nullptr)
    {
        ext_pci_bus_info_properties_match = (*ext_pci_bus_info_properties_ptr == *in_props.ext_pci_bus_info_properties_ptr);
    }
    else
    {
        ext_pci_bus_info_properties_match = (ext_pci_bus_info_properties_ptr          == nullptr &&
                                             in_props.ext_pci_bus_info_properties_ptr == nullptr);
    }

    if (ext_sample_locations_properties_ptr          != nullptr &&
        in_props.ext_sample_locations_properties_ptr != nullptr)
    {
        ext_sample_locations_properties_match = (*ext_sample_locations_properties_ptr == *in_props.ext_sample_locations_properties_ptr);
    }
    else
    {
        ext_sample_locations_properties_match = (ext_sample_locations_properties_ptr          == nullptr &&
                                                 in_props.ext_sample_locations_properties_ptr == nullptr);
    }

    if (ext_sampler_filter_minmax_properties_ptr         != nullptr &&
        in_props.ext_sampler_filter_minmax_properties_ptr != nullptr)
    {
        ext_sampler_filter_minmax_properties_match = (*ext_sampler_filter_minmax_properties_ptr == *in_props.ext_sampler_filter_minmax_properties_ptr);
    }
    else
    {
        ext_sampler_filter_minmax_properties_match = (ext_sampler_filter_minmax_properties_ptr          == nullptr &&
                                                      in_props.ext_sampler_filter_minmax_properties_ptr == nullptr);
    }

    if (ext_vertex_attribute_divisor_properties_ptr          != nullptr &&
        in_props.ext_vertex_attribute_divisor_properties_ptr != nullptr)
    {
        ext_vertex_attribute_divisor_properties_match = (*ext_vertex_attribute_divisor_properties_ptr == *in_props.ext_vertex_attribute_divisor_properties_ptr);
    }
    else
    {
        ext_vertex_attribute_divisor_properties_match = (ext_vertex_attribute_divisor_properties_ptr          == nullptr &&
                                                         in_props.ext_vertex_attribute_divisor_properties_ptr == nullptr);
    }

    if (khr_depth_stencil_resolve_properties_ptr          != nullptr &&
        in_props.khr_depth_stencil_resolve_properties_ptr != nullptr)
    {
        khr_depth_stencil_resolve_properties_match = (*khr_depth_stencil_resolve_properties_ptr == *in_props.khr_depth_stencil_resolve_properties_ptr);
    }
    else
    {
        khr_depth_stencil_resolve_properties_match = (khr_depth_stencil_resolve_properties_ptr          == nullptr &&
                                                      in_props.khr_depth_stencil_resolve_properties_ptr == nullptr);
    }

    if (khr_driver_properties_properties_ptr          != nullptr &&
        in_props.khr_driver_properties_properties_ptr != nullptr)
    {
        khr_driver_properties_properties_match = (*khr_driver_properties_properties_ptr == *in_props.khr_driver_properties_properties_ptr);
    }
    else
    {
        khr_driver_properties_properties_match = (khr_driver_properties_properties_ptr          == nullptr &&
                                                  in_props.khr_driver_properties_properties_ptr == nullptr);
    }

    if (khr_external_memory_capabilities_physical_device_id_properties_ptr          != nullptr &&
        in_props.khr_external_memory_capabilities_physical_device_id_properties_ptr!= nullptr)
    {
        khr_external_memory_capabilities_properties_match = (*khr_external_memory_capabilities_physical_device_id_properties_ptr == *in_props.khr_external_memory_capabilities_physical_device_id_properties_ptr);
    }
    else
    {
        khr_external_memory_capabilities_properties_match = (khr_external_memory_capabilities_physical_device_id_properties_ptr          == nullptr &&
                                                             in_props.khr_external_memory_capabilities_physical_device_id_properties_ptr == nullptr);
    }

    if (khr_maintenance2_point_clipping_properties_ptr          != nullptr &&
        in_props.khr_maintenance2_point_clipping_properties_ptr != nullptr)
    {
        khr_maintenance2_properties_match = (*khr_maintenance2_point_clipping_properties_ptr == *in_props.khr_maintenance2_point_clipping_properties_ptr);
    }
    else
    {
        khr_maintenance2_properties_match = (khr_maintenance2_point_clipping_properties_ptr          == nullptr &&
                                             in_props.khr_maintenance2_point_clipping_properties_ptr == nullptr);
    }

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

    if (khr_multiview_properties_ptr          != nullptr &&
        in_props.khr_multiview_properties_ptr != nullptr)
    {
        khr_multiview_properties_match = (*khr_multiview_properties_ptr == *in_props.khr_multiview_properties_ptr);
    }
    else
    {
        khr_multiview_properties_match = (khr_multiview_properties_ptr          == nullptr &&
                                          in_props.khr_multiview_properties_ptr == nullptr);
    }

    if (khr_shader_float_controls_properties_ptr          != nullptr &&
        in_props.khr_shader_float_controls_properties_ptr != nullptr)
    {
        khr_shader_float_controls_properties_match  = (*khr_shader_float_controls_properties_ptr == *in_props.khr_shader_float_controls_properties_ptr);
    }
    else
    {
        khr_shader_float_controls_properties_match = (khr_shader_float_controls_properties_ptr          == nullptr &&
                                                      in_props.khr_shader_float_controls_properties_ptr == nullptr);
    }

    return amd_shader_core_properties_match                   &&
           core_vk1_0_features_match                          &&
           core_vk1_1_features_match                          &&
           ext_conservative_rasterization_properties_match    &&
           ext_descriptor_indexing_properties_match           &&
           ext_external_memory_host_properties_match          &&
           ext_inline_uniform_block_properties_match          &&
           ext_pci_bus_info_properties_match                  &&
           ext_sample_locations_properties_match              &&
           ext_sampler_filter_minmax_properties_match         &&
           ext_vertex_attribute_divisor_properties_match      &&
           khr_depth_stencil_resolve_properties_match         &&
           khr_driver_properties_properties_match             &&
           khr_external_memory_capabilities_properties_match  &&
           khr_maintenance2_properties_match                  &&
           khr_maintenance3_properties_match                  &&
           khr_multiview_properties_match                     &&
           khr_shader_float_controls_properties_match;
}

Anvil::PhysicalDevicePropertiesCoreVK10::PhysicalDevicePropertiesCoreVK10()
    :api_version   (UINT32_MAX),
     device_id     (UINT32_MAX),
     device_type   (VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM),
     driver_version(UINT32_MAX),
     vendor_id     (UINT32_MAX)
{
    memset(device_name,
           0xFF,
           sizeof(device_name) );
    memset(pipeline_cache_uuid,
           0xFF,
           sizeof(pipeline_cache_uuid) );
}

Anvil::PhysicalDevicePropertiesCoreVK11::PhysicalDevicePropertiesCoreVK11()
{
    /* Stub */
}

Anvil::PhysicalDeviceFeatures::PhysicalDeviceFeatures()
{
    core_vk1_0_features_ptr                   = nullptr;
    core_vk1_1_features_ptr                   = nullptr;
    ext_depth_clip_enable_features_ptr        = nullptr;
    ext_descriptor_indexing_features_ptr      = nullptr;
    ext_inline_uniform_block_features_ptr     = nullptr;
    ext_scalar_block_layout_features_ptr      = nullptr;
    ext_transform_feedback_features_ptr       = nullptr;
    ext_memory_priority_features_ptr          = nullptr;
    khr_16bit_storage_features_ptr            = nullptr;
    khr_8bit_storage_features_ptr             = nullptr;
    khr_float16_int8_features_ptr             = nullptr;
    khr_multiview_features_ptr                = nullptr;
    khr_sampler_ycbcr_conversion_features_ptr = nullptr;
    khr_shader_atomic_int64_features_ptr      = nullptr;
    khr_variable_pointer_features_ptr         = nullptr;
    khr_vulkan_memory_model_features_ptr      = nullptr;
}

Anvil::PhysicalDeviceFeatures::PhysicalDeviceFeatures(const PhysicalDeviceFeaturesCoreVK10*    in_core_vk1_0_features_ptr,
                                                      const PhysicalDeviceFeaturesCoreVK11*    in_core_vk1_1_features_ptr,
                                                      const EXTDepthClipEnableFeatures*        in_ext_depth_clip_enable_features_ptr,
                                                      const EXTDescriptorIndexingFeatures*     in_ext_descriptor_indexing_features_ptr,
                                                      const EXTInlineUniformBlockFeatures*     in_ext_inline_uniform_block_features_ptr,
                                                      const EXTScalarBlockLayoutFeatures*      in_ext_scalar_block_layout_features_ptr,
                                                      const EXTTransformFeedbackFeatures*      in_ext_transform_feedback_features_ptr,
                                                      const EXTMemoryPriorityFeatures*         in_ext_memory_priority_features_ptr,
                                                      const KHR16BitStorageFeatures*           in_khr_16_bit_storage_features_ptr,
                                                      const KHR8BitStorageFeatures*            in_khr_8_bit_storage_features_ptr,
                                                      const KHRFloat16Int8Features*            in_khr_float16_int8_features_ptr,
                                                      const KHRMultiviewFeatures*              in_khr_multiview_features_ptr,
                                                      const KHRSamplerYCbCrConversionFeatures* in_khr_sampler_ycbcr_conversion_features_ptr,
                                                      const KHRShaderAtomicInt64Features*      in_khr_shader_atomic_int64_features_ptr,
                                                      const KHRVariablePointerFeatures*        in_khr_variable_pointer_features_ptr,
                                                      const KHRVulkanMemoryModelFeatures*      in_khr_vulkan_memory_model_features_ptr)
{
    core_vk1_0_features_ptr                   = in_core_vk1_0_features_ptr;
    core_vk1_1_features_ptr                   = in_core_vk1_1_features_ptr;
    ext_depth_clip_enable_features_ptr        = in_ext_depth_clip_enable_features_ptr;
    ext_descriptor_indexing_features_ptr      = in_ext_descriptor_indexing_features_ptr;
    ext_inline_uniform_block_features_ptr     = in_ext_inline_uniform_block_features_ptr;
    ext_scalar_block_layout_features_ptr      = in_ext_scalar_block_layout_features_ptr;
    ext_transform_feedback_features_ptr       = in_ext_transform_feedback_features_ptr;
    ext_memory_priority_features_ptr          = in_ext_memory_priority_features_ptr;
    khr_16bit_storage_features_ptr            = in_khr_16_bit_storage_features_ptr;
    khr_8bit_storage_features_ptr             = in_khr_8_bit_storage_features_ptr;
    khr_float16_int8_features_ptr             = in_khr_float16_int8_features_ptr;
    khr_multiview_features_ptr                = in_khr_multiview_features_ptr;
    khr_sampler_ycbcr_conversion_features_ptr = in_khr_sampler_ycbcr_conversion_features_ptr;
    khr_shader_atomic_int64_features_ptr      = in_khr_shader_atomic_int64_features_ptr;
    khr_variable_pointer_features_ptr         = in_khr_variable_pointer_features_ptr;
    khr_vulkan_memory_model_features_ptr      = in_khr_vulkan_memory_model_features_ptr;
}

bool Anvil::PhysicalDeviceFeatures::operator==(const PhysicalDeviceFeatures& in_physical_device_features) const
{
    const bool core_vk1_0_features_match                   = (*core_vk1_0_features_ptr == *in_physical_device_features.core_vk1_0_features_ptr);
    const bool core_vk1_1_features_match                   = ( core_vk1_1_features_ptr == nullptr                                              && in_physical_device_features.core_vk1_1_features_ptr == nullptr) ||
                                                             (*core_vk1_1_features_ptr == *in_physical_device_features.core_vk1_1_features_ptr);
    bool       ext_depth_clip_enable_features_match        = false;
    bool       ext_descriptor_indexing_features_match      = false;
    bool       ext_inline_uniform_block_features_match     = false;
    bool       ext_scalar_block_layout_features_match      = false;
    bool       ext_transform_feedback_features_match       = false;
    bool       ext_memory_priority_features_match          = false;
    bool       khr_16bit_storage_features_match            = false;
    bool       khr_8bit_storage_features_match             = false;
    bool       khr_float16_int8_features_match             = false;
    bool       khr_multiview_features_match                = false;
    bool       khr_sampler_ycbcr_conversion_features_match = false;
    bool       khr_shader_atomic_int64_features_match      = false;
    bool       khr_variable_pointer_features_match         = false;
    bool       khr_vulkan_memory_features_match            = false;

    if (ext_depth_clip_enable_features_ptr                             != nullptr &&
        in_physical_device_features.ext_depth_clip_enable_features_ptr != nullptr)
    {
        ext_depth_clip_enable_features_match = (*ext_depth_clip_enable_features_ptr == *in_physical_device_features.ext_depth_clip_enable_features_ptr);
    }
    else
    {
        ext_depth_clip_enable_features_match = (ext_depth_clip_enable_features_ptr                             == nullptr &&
                                                in_physical_device_features.ext_depth_clip_enable_features_ptr == nullptr);
    }

    if (ext_descriptor_indexing_features_ptr                             != nullptr &&
        in_physical_device_features.ext_descriptor_indexing_features_ptr != nullptr)
    {
        ext_descriptor_indexing_features_match = (*ext_descriptor_indexing_features_ptr == *in_physical_device_features.ext_descriptor_indexing_features_ptr);
    }
    else
    {
        ext_descriptor_indexing_features_match = (ext_descriptor_indexing_features_ptr                             == nullptr &&
                                                  in_physical_device_features.ext_descriptor_indexing_features_ptr == nullptr);
    }

    if (ext_inline_uniform_block_features_ptr                             != nullptr &&
        in_physical_device_features.ext_inline_uniform_block_features_ptr != nullptr)
    {
        ext_inline_uniform_block_features_match = (*ext_inline_uniform_block_features_ptr == *in_physical_device_features.ext_inline_uniform_block_features_ptr);
    }
    else
    {
        ext_inline_uniform_block_features_match = (ext_inline_uniform_block_features_ptr                             == nullptr &&
                                                   in_physical_device_features.ext_inline_uniform_block_features_ptr == nullptr);
    }

    if (ext_scalar_block_layout_features_ptr                            != nullptr &&
        in_physical_device_features.ext_scalar_block_layout_features_ptr != nullptr)
    {
        ext_scalar_block_layout_features_match = (*ext_scalar_block_layout_features_ptr == *in_physical_device_features.ext_scalar_block_layout_features_ptr);
    }
    else
    {
        ext_scalar_block_layout_features_match = (ext_scalar_block_layout_features_ptr                             == nullptr &&
                                                  in_physical_device_features.ext_scalar_block_layout_features_ptr == nullptr);
    }

    if (ext_transform_feedback_features_ptr                             != nullptr &&
        in_physical_device_features.ext_transform_feedback_features_ptr != nullptr)
    {
        ext_transform_feedback_features_match = (*ext_transform_feedback_features_ptr == *in_physical_device_features.ext_transform_feedback_features_ptr);
    }
    else
    {
        ext_transform_feedback_features_match = (ext_transform_feedback_features_ptr                             == nullptr &&
                                                 in_physical_device_features.ext_transform_feedback_features_ptr == nullptr);
    }

    if (ext_memory_priority_features_ptr                             != nullptr &&
        in_physical_device_features.ext_memory_priority_features_ptr != nullptr)
    {
        ext_memory_priority_features_match = (*ext_memory_priority_features_ptr == *in_physical_device_features.ext_memory_priority_features_ptr);
    }

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

    if (khr_8bit_storage_features_ptr                             != nullptr &&
        in_physical_device_features.khr_8bit_storage_features_ptr != nullptr)
    {
        khr_8bit_storage_features_match = (*khr_8bit_storage_features_ptr == *in_physical_device_features.khr_8bit_storage_features_ptr);
    }
    else
    {
        khr_8bit_storage_features_match = (khr_8bit_storage_features_ptr                             == nullptr &&
                                           in_physical_device_features.khr_8bit_storage_features_ptr == nullptr);
    }

    if (khr_float16_int8_features_ptr                             != nullptr &&
        in_physical_device_features.khr_float16_int8_features_ptr != nullptr)
    {
        khr_float16_int8_features_match = (*khr_float16_int8_features_ptr == *in_physical_device_features.khr_float16_int8_features_ptr);
    }
    else
    {
        khr_float16_int8_features_match = (khr_float16_int8_features_ptr                             == nullptr &&
                                           in_physical_device_features.khr_float16_int8_features_ptr == nullptr);
    }

    if (khr_multiview_features_ptr                             != nullptr &&
        in_physical_device_features.khr_multiview_features_ptr != nullptr)
    {
        khr_multiview_features_match = (*khr_multiview_features_ptr == *in_physical_device_features.khr_multiview_features_ptr);
    }
    else
    {
        khr_multiview_features_match = (khr_multiview_features_ptr                             == nullptr &&
                                        in_physical_device_features.khr_multiview_features_ptr == nullptr);
    }

    if (khr_sampler_ycbcr_conversion_features_ptr                             != nullptr &&
        in_physical_device_features.khr_sampler_ycbcr_conversion_features_ptr != nullptr)
    {
        khr_sampler_ycbcr_conversion_features_match = (*khr_sampler_ycbcr_conversion_features_ptr == *in_physical_device_features.khr_sampler_ycbcr_conversion_features_ptr);
    }
    else
    {
        khr_sampler_ycbcr_conversion_features_match = (khr_sampler_ycbcr_conversion_features_ptr                             == nullptr &&
                                                       in_physical_device_features.khr_sampler_ycbcr_conversion_features_ptr == nullptr);
    }

    if (khr_shader_atomic_int64_features_ptr                             != nullptr &&
        in_physical_device_features.khr_shader_atomic_int64_features_ptr != nullptr)
    {
        khr_shader_atomic_int64_features_match = (*khr_shader_atomic_int64_features_ptr == *in_physical_device_features.khr_shader_atomic_int64_features_ptr);
    }
    else
    {
        khr_shader_atomic_int64_features_match = (khr_shader_atomic_int64_features_ptr                             == nullptr &&
                                                  in_physical_device_features.khr_shader_atomic_int64_features_ptr == nullptr);
    }

    if (khr_variable_pointer_features_ptr                             != nullptr &&
        in_physical_device_features.khr_variable_pointer_features_ptr != nullptr)
    {
        khr_variable_pointer_features_match = (*khr_variable_pointer_features_ptr == *in_physical_device_features.khr_variable_pointer_features_ptr);
    }
    else
    {
        khr_variable_pointer_features_match = (khr_variable_pointer_features_ptr                             == nullptr &&
                                               in_physical_device_features.khr_variable_pointer_features_ptr == nullptr);
    }

    if (khr_vulkan_memory_model_features_ptr                             != nullptr &&
        in_physical_device_features.khr_vulkan_memory_model_features_ptr != nullptr)
    {
        khr_vulkan_memory_features_match = (*khr_vulkan_memory_model_features_ptr == *in_physical_device_features.khr_vulkan_memory_model_features_ptr);
    }
    else
    {
        khr_vulkan_memory_features_match = (khr_vulkan_memory_model_features_ptr                             == nullptr &&
                                            in_physical_device_features.khr_vulkan_memory_model_features_ptr == nullptr);
    }

    return core_vk1_0_features_match                   &&
           core_vk1_1_features_match                   &&
           ext_depth_clip_enable_features_match        &&
           ext_descriptor_indexing_features_match      &&
           ext_inline_uniform_block_features_match     &&
           ext_scalar_block_layout_features_match      &&
           ext_transform_feedback_features_match       &&
           ext_memory_priority_features_match          &&
           khr_16bit_storage_features_match            &&
           khr_8bit_storage_features_match             &&
           khr_float16_int8_features_match             &&
           khr_multiview_features_match                &&
           khr_sampler_ycbcr_conversion_features_match &&
           khr_shader_atomic_int64_features_match      &&
           khr_variable_pointer_features_match         &&
           khr_vulkan_memory_features_match;
}

Anvil::PhysicalDeviceGroup::PhysicalDeviceGroup()
{
    supports_subset_allocations = false;
}

Anvil::PhysicalDeviceLimits::PhysicalDeviceLimits()
    :buffer_image_granularity                             (std::numeric_limits<VkDeviceSize>::max() ),
     discrete_queue_priorities                            (UINT32_MAX),
     framebuffer_color_sample_counts                      (Anvil::SampleCountFlagBits::NONE),
     framebuffer_depth_sample_counts                      (Anvil::SampleCountFlagBits::NONE),
     framebuffer_no_attachments_sample_counts             (Anvil::SampleCountFlagBits::NONE),
     framebuffer_stencil_sample_counts                    (Anvil::SampleCountFlagBits::NONE),
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
     sampled_image_color_sample_counts                    (Anvil::SampleCountFlagBits::NONE),
     sampled_image_depth_sample_counts                    (Anvil::SampleCountFlagBits::NONE),
     sampled_image_integer_sample_counts                  (Anvil::SampleCountFlagBits::NONE),
     sampled_image_stencil_sample_counts                  (Anvil::SampleCountFlagBits::NONE),
     sparse_address_space_size                            (std::numeric_limits<VkDeviceSize>::max() ),
     standard_sample_locations                            (false),
     storage_image_sample_counts                          (Anvil::SampleCountFlagBits::NONE),
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
     framebuffer_color_sample_counts                      (static_cast<Anvil::SampleCountFlagBits>(in_device_limits.framebufferColorSampleCounts) ),
     framebuffer_depth_sample_counts                      (static_cast<Anvil::SampleCountFlagBits>(in_device_limits.framebufferDepthSampleCounts) ),
     framebuffer_no_attachments_sample_counts             (static_cast<Anvil::SampleCountFlagBits>(in_device_limits.framebufferNoAttachmentsSampleCounts) ),
     framebuffer_stencil_sample_counts                    (static_cast<Anvil::SampleCountFlagBits>(in_device_limits.framebufferStencilSampleCounts) ),
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
     sampled_image_color_sample_counts                    (static_cast<Anvil::SampleCountFlagBits>(in_device_limits.sampledImageColorSampleCounts) ),
     sampled_image_depth_sample_counts                    (static_cast<Anvil::SampleCountFlagBits>(in_device_limits.sampledImageDepthSampleCounts) ),
     sampled_image_integer_sample_counts                  (static_cast<Anvil::SampleCountFlagBits>(in_device_limits.sampledImageIntegerSampleCounts) ),
     sampled_image_stencil_sample_counts                  (static_cast<Anvil::SampleCountFlagBits>(in_device_limits.sampledImageStencilSampleCounts) ),
     sparse_address_space_size                            (in_device_limits.sparseAddressSpaceSize),
     standard_sample_locations                            (VK_BOOL32_TO_BOOL(in_device_limits.standardSampleLocations) ),
     storage_image_sample_counts                          (static_cast<Anvil::SampleCountFlagBits>(in_device_limits.storageImageSampleCounts) ),
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

Anvil::KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties::KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties()
{
    device_luid_valid = false;
    device_node_mask  = 0;

    memset(device_luid,
           0,
           VK_LUID_SIZE);
    memset(device_uuid,
           0,
           VK_UUID_SIZE);
    memset(driver_uuid,
           0,
           VK_UUID_SIZE);
}

Anvil::KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties::KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties(const VkPhysicalDeviceIDPropertiesKHR& in_properties)
{
    device_luid_valid = VK_BOOL32_TO_BOOL(in_properties.deviceLUIDValid);
    device_node_mask  = in_properties.deviceNodeMask;

    memcpy(device_luid,
           in_properties.deviceLUID,
           VK_LUID_SIZE);
    memcpy(device_uuid,
           in_properties.deviceUUID,
           VK_UUID_SIZE);
    memcpy(driver_uuid,
           in_properties.driverUUID,
           VK_UUID_SIZE);
}

bool Anvil::KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties::operator==(const Anvil::KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties& in_props) const
{
    bool result = false;

    if (device_luid_valid != in_props.device_luid_valid ||
        device_node_mask  != in_props.device_node_mask)
    {
        goto end;
    }

    if (device_luid_valid)
    {
        if (memcmp(device_luid,
                   in_props.device_luid,
                   sizeof(device_luid)) != 0)
        {
            goto end;
        }
    }

    if (memcmp(device_uuid,
               in_props.device_uuid,
               sizeof(device_uuid)) != 0)
    {
        goto end;
    }

    if (memcmp(driver_uuid,
               in_props.driver_uuid,
               sizeof(driver_uuid)) != 0)
    {
        goto end;
    }

    result = true;
end:
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

Anvil::PhysicalDevicePropertiesCoreVK11::PhysicalDevicePropertiesCoreVK11(const VkPhysicalDeviceProtectedMemoryProperties& in_protected_memory_properties,
                                                                          const VkPhysicalDeviceSubgroupProperties&        in_subgroup_properties)
    :protected_memory_properties(in_protected_memory_properties),
     subgroup_properties        (in_subgroup_properties)
{
    /* Stub */
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

bool Anvil::PhysicalDevicePropertiesCoreVK11::operator==(const PhysicalDevicePropertiesCoreVK11& in_props) const
{
    bool result = false;

    if (protected_memory_properties == in_props.protected_memory_properties &&
        subgroup_properties         == in_props.subgroup_properties)
    {
        result = true;
    }

    return result;
}

Anvil::PhysicalDeviceProtectedMemoryFeatures::PhysicalDeviceProtectedMemoryFeatures()
    :protected_memory(false)
{
    /* Stub */
}

Anvil::PhysicalDeviceProtectedMemoryFeatures::PhysicalDeviceProtectedMemoryFeatures(const VkPhysicalDeviceProtectedMemoryFeatures& in_features)
{
    protected_memory = VK_BOOL32_TO_BOOL(in_features.protectedMemory);
}

VkPhysicalDeviceProtectedMemoryFeatures Anvil::PhysicalDeviceProtectedMemoryFeatures::get_vk_physical_device_protected_memory_features() const
{
    VkPhysicalDeviceProtectedMemoryFeatures result;

    result.pNext           = nullptr;
    result.protectedMemory = BOOL_TO_VK_BOOL32(protected_memory);
    result.sType           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES;

    return result;
}

bool Anvil::PhysicalDeviceProtectedMemoryFeatures::operator==(const PhysicalDeviceProtectedMemoryFeatures& in_features) const
{
    return (protected_memory == in_features.protected_memory);
}

Anvil::PhysicalDeviceProtectedMemoryProperties::PhysicalDeviceProtectedMemoryProperties()
    :protected_no_fault(false)
{
    /* Stub */
}

Anvil::PhysicalDeviceProtectedMemoryProperties::PhysicalDeviceProtectedMemoryProperties(const VkPhysicalDeviceProtectedMemoryProperties& in_props)
    :protected_no_fault(VK_BOOL32_TO_BOOL(in_props.protectedNoFault) )
{
    /* Stub */
}

bool Anvil::PhysicalDeviceProtectedMemoryProperties::operator==(const PhysicalDeviceProtectedMemoryProperties& in_props) const
{
    return (protected_no_fault == in_props.protected_no_fault);
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

Anvil::PhysicalDeviceSubgroupProperties::PhysicalDeviceSubgroupProperties()
{
    quad_operations_in_all_stages = false;
    subgroup_size                 = 0;
    supported_operations          = Anvil::SubgroupFeatureFlagBits::NONE;
    supported_stages              = Anvil::ShaderStageFlagBits::NONE;
}

Anvil::PhysicalDeviceSubgroupProperties::PhysicalDeviceSubgroupProperties(const VkPhysicalDeviceSubgroupProperties & in_props)
{
    quad_operations_in_all_stages = (in_props.quadOperationsInAllStages == VK_TRUE);
    subgroup_size                 = in_props.subgroupSize;
    supported_operations          = static_cast<Anvil::SubgroupFeatureFlagBits>(in_props.supportedOperations);
    supported_stages              = static_cast<Anvil::ShaderStageFlagBits>(in_props.supportedStages);
}

bool Anvil::PhysicalDeviceSubgroupProperties::operator==(const PhysicalDeviceSubgroupProperties& in_props) const
{
    return (quad_operations_in_all_stages == in_props.quad_operations_in_all_stages &&
            subgroup_size                 == in_props.subgroup_size                 &&
            supported_operations          == in_props.supported_operations          &&
            supported_stages              == in_props.supported_stages);
}

Anvil::PushConstantRange::PushConstantRange(uint32_t                in_offset,
                                            uint32_t                in_size,
                                            Anvil::ShaderStageFlags in_stages)
{
    offset = in_offset;
    size   = in_size;
    stages = in_stages;
}

bool Anvil::PushConstantRange::operator==(const PushConstantRange& in) const
{
    return (offset == in.offset &&
            size   == in.size   &&
            stages == in.stages);
}

Anvil::QueueFamilyInfo::QueueFamilyInfo(const VkQueueFamilyProperties& in_props)
{
    flags                          = static_cast<Anvil::QueueFlagBits>(in_props.queueFlags);
    min_image_transfer_granularity = in_props.minImageTransferGranularity;
    n_queues                       = in_props.queueCount;
    n_timestamp_bits               = in_props.timestampValidBits;
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
Anvil::SemaphoreProperties::SemaphoreProperties()
{
    /* Stub */
}

/** Please see header for specification */
Anvil::SemaphoreProperties::SemaphoreProperties(const ExternalSemaphoreProperties& in_external_semaphore_properties)
    :external_semaphore_properties(in_external_semaphore_properties)
{
    /* Stub */
}

/** Please see header for specification */
Anvil::ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint()
{
    shader_module_ptr = nullptr;
    stage             = ShaderStage::UNKNOWN;
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

Anvil::SparseImageAspectProperties::SparseImageAspectProperties()
{
    memset(this,
           0,
           sizeof(*this) );
}

Anvil::SparseImageAspectProperties::SparseImageAspectProperties(const Anvil::SparseImageMemoryRequirements& in_req)
{
    aspect_mask        = in_req.format_properties.aspect_mask;
    flags              = in_req.format_properties.flags;
    granularity        = in_req.format_properties.image_granularity;
    mip_tail_first_lod = in_req.image_mip_tail_first_lod;
    mip_tail_offset    = in_req.image_mip_tail_offset;
    mip_tail_size      = in_req.image_mip_tail_size;
    mip_tail_stride    = in_req.image_mip_tail_stride;
}

Anvil::SparseMemoryBindingUpdateInfo::SparseMemoryBindingUpdateInfo()
{
    m_dirty     = true;
    m_fence_ptr = nullptr;
}

Anvil::SpecializationConstant::SpecializationConstant()
{
    constant_id  = UINT32_MAX;
    n_bytes      = UINT32_MAX;
    start_offset = UINT32_MAX;
}

Anvil::SpecializationConstant::SpecializationConstant(uint32_t in_constant_id,
                                                      uint32_t in_n_bytes,
                                                      uint32_t in_start_offset)
{
    constant_id  = in_constant_id;
    n_bytes      = in_n_bytes;
    start_offset = in_start_offset;
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
     logic_op                                    (false),
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
     logic_op                                    (VK_BOOL32_TO_BOOL(in_physical_device_features.logicOp) ),
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
    result.logicOp                                 = BOOL_TO_VK_BOOL32(logic_op);
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
           (logic_op                                     == in_data.logic_op)                                     &&
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

bool Anvil::PhysicalDeviceFeaturesCoreVK11::operator==(const PhysicalDeviceFeaturesCoreVK11& in_data) const
{
    return (protected_memory_features == in_data.protected_memory_features);
}

Anvil::PhysicalDeviceFeaturesCoreVK11::PhysicalDeviceFeaturesCoreVK11()
{
    /* Stub */
}

Anvil::PhysicalDeviceFeaturesCoreVK11::PhysicalDeviceFeaturesCoreVK11(const PhysicalDeviceProtectedMemoryFeatures& in_protected_memory_features)
    :protected_memory_features(in_protected_memory_features)
{
    /* Stub */
}

Anvil::SubmitInfo::SubmitInfo(uint32_t                         in_n_command_buffers,
                              Anvil::CommandBufferBase*        in_opt_single_cmd_buffer_ptr,
                              Anvil::CommandBufferBase* const* in_opt_cmd_buffer_ptrs_ptr,
                              uint32_t                         in_n_semaphores_to_signal,
                              Anvil::Semaphore* const*         in_opt_semaphore_to_signal_ptrs_ptr,
                              uint32_t                         in_n_semaphores_to_wait_on,
                              Anvil::Semaphore* const*         in_opt_semaphore_to_wait_on_ptrs_ptr,
                              const Anvil::PipelineStageFlags* in_opt_dst_stage_masks_to_wait_on_ptrs,
                              bool                             in_should_block,
                              Anvil::Fence*                    in_opt_fence_ptr)
    :command_buffers_mgpu_ptr                      (nullptr),
     command_buffers_sgpu_ptr                      (in_opt_cmd_buffer_ptrs_ptr),
#if defined(_WIN32)
     d3d12_fence_signal_semaphore_values_ptr       (nullptr),
     d3d12_fence_wait_semaphore_values_ptr         (nullptr),
#endif
     dst_stage_wait_masks                          (in_n_semaphores_to_wait_on),
     fence_ptr                                     (in_opt_fence_ptr),
#if defined(_WIN32)
     keyed_mutex_n_acquire_keys                     (0),
     keyed_mutex_acquire_d3d11_memory_block_ptrs_ptr(nullptr),
     keyed_mutex_acquire_mutex_key_value_ptrs       (nullptr),
     keyed_mutex_acquire_timeout_ptrs               (nullptr),
     keyed_mutex_n_release_keys                     (0),
     keyed_mutex_release_d3d11_memory_block_ptrs_ptr(nullptr),
     keyed_mutex_release_mutex_key_value_ptrs       (nullptr),
#endif
     is_protected                                  (false),
     n_command_buffers                             (in_n_command_buffers),
     n_signal_semaphores                           (in_n_semaphores_to_signal),
     n_wait_semaphores                             (in_n_semaphores_to_wait_on),
     signal_semaphores_mgpu_ptr                    (nullptr),
     signal_semaphores_sgpu_ptr                    (in_opt_semaphore_to_signal_ptrs_ptr),
     should_block                                  (in_should_block),
     timeout                                       (UINT64_MAX),
     type                                          (SubmissionType::SGPU),
     wait_semaphores_mgpu_ptr                      (nullptr),
     wait_semaphores_sgpu_ptr                      (in_opt_semaphore_to_wait_on_ptrs_ptr)
{
    for (uint32_t n_wait_mask = 0;
                  n_wait_mask < in_n_semaphores_to_wait_on;
                ++n_wait_mask)
    {
        dst_stage_wait_masks.at(n_wait_mask) = in_opt_dst_stage_masks_to_wait_on_ptrs[n_wait_mask].get_vk();
    }

    if (in_opt_single_cmd_buffer_ptr)
    {
        anvil_assert(in_n_command_buffers == 1);

        helper_cmd_buffer_raw_ptr = in_opt_single_cmd_buffer_ptr;
        command_buffers_sgpu_ptr  = &helper_cmd_buffer_raw_ptr;
    }
    else
    {
        anvil_assert((in_n_command_buffers == 0)                                          ||
                     (in_n_command_buffers != 0 && in_opt_cmd_buffer_ptrs_ptr != nullptr) );
    }

    anvil_assert((in_n_semaphores_to_signal == 0)                                                   ||
                 (in_n_semaphores_to_signal != 0 && in_opt_semaphore_to_signal_ptrs_ptr != nullptr) );

    anvil_assert((in_n_semaphores_to_wait_on == 0)                                                                                                         ||
                 (in_n_semaphores_to_wait_on != 0 && in_opt_semaphore_to_wait_on_ptrs_ptr != nullptr && in_opt_dst_stage_masks_to_wait_on_ptrs != nullptr) );
}

Anvil::SubmitInfo::SubmitInfo(uint32_t                           in_n_command_buffer_submissions,
                              const CommandBufferMGPUSubmission* in_opt_command_buffer_submissions_ptr,
                              uint32_t                           in_n_signal_semaphore_submissions,
                              const SemaphoreMGPUSubmission*     in_opt_signal_semaphore_submissions_ptr,
                              uint32_t                           in_n_wait_semaphore_submissions,
                              const SemaphoreMGPUSubmission*     in_opt_wait_semaphore_submissions_ptr,
                              const Anvil::PipelineStageFlags*   in_opt_dst_stage_masks_to_wait_on_ptr,
                              bool                               in_should_block,
                              Anvil::Fence*                      in_opt_fence_ptr)
    :command_buffers_mgpu_ptr                      (in_opt_command_buffer_submissions_ptr),
     command_buffers_sgpu_ptr                      (nullptr),
#if defined(_WIN32)
     d3d12_fence_signal_semaphore_values_ptr       (nullptr),
     d3d12_fence_wait_semaphore_values_ptr         (nullptr),
#endif
     dst_stage_wait_masks                          (in_n_wait_semaphore_submissions),
     fence_ptr                                     (in_opt_fence_ptr),
     is_protected                                  (false),
#if defined(_WIN32)
     keyed_mutex_n_acquire_keys                     (0),
     keyed_mutex_acquire_d3d11_memory_block_ptrs_ptr(nullptr),
     keyed_mutex_acquire_mutex_key_value_ptrs       (nullptr),
     keyed_mutex_acquire_timeout_ptrs               (nullptr),
     keyed_mutex_n_release_keys                     (0),
     keyed_mutex_release_d3d11_memory_block_ptrs_ptr(nullptr),
     keyed_mutex_release_mutex_key_value_ptrs       (nullptr),
#endif
     n_command_buffers                             (in_n_command_buffer_submissions),
     n_signal_semaphores                           (in_n_signal_semaphore_submissions),
     n_wait_semaphores                             (in_n_wait_semaphore_submissions),
     signal_semaphores_mgpu_ptr                    (in_opt_signal_semaphore_submissions_ptr),
     signal_semaphores_sgpu_ptr                    (nullptr),
     should_block                                  (in_should_block),
     timeout                                       (UINT64_MAX),
     type                                          (SubmissionType::MGPU),
     wait_semaphores_mgpu_ptr                      (in_opt_wait_semaphore_submissions_ptr),
     wait_semaphores_sgpu_ptr                      (nullptr)
{
    for (uint32_t n_wait_mask = 0;
                  n_wait_mask < in_n_wait_semaphore_submissions;
                ++n_wait_mask)
    {
        dst_stage_wait_masks.at(n_wait_mask) = in_opt_dst_stage_masks_to_wait_on_ptr[n_wait_mask].get_vk();
    }

    anvil_assert((in_n_command_buffer_submissions == 0)                                                     ||
                 (in_n_command_buffer_submissions != 0 && in_opt_command_buffer_submissions_ptr != nullptr) );

    anvil_assert((in_n_signal_semaphore_submissions == 0)                                                       ||
                 (in_n_signal_semaphore_submissions != 0 && in_opt_signal_semaphore_submissions_ptr != nullptr) );

    anvil_assert((in_n_wait_semaphore_submissions == 0)                                                                                                         ||
                 (in_n_wait_semaphore_submissions != 0 && in_opt_wait_semaphore_submissions_ptr != nullptr && in_opt_dst_stage_masks_to_wait_on_ptr != nullptr) );
}

Anvil::SubmitInfo Anvil::SubmitInfo::create(Anvil::CommandBufferBase*        in_opt_cmd_buffer_ptr,
                                            uint32_t                         in_n_semaphores_to_signal,
                                            Anvil::Semaphore* const*         in_opt_semaphore_to_signal_ptrs_ptr,
                                            uint32_t                         in_n_semaphores_to_wait_on,
                                            Anvil::Semaphore* const*         in_opt_semaphore_to_wait_on_ptrs_ptr,
                                            const Anvil::PipelineStageFlags* in_opt_dst_stage_masks_to_wait_on_ptrs,
                                            bool                             in_should_block,
                                            Anvil::Fence*                    in_opt_fence_ptr)
{
    return Anvil::SubmitInfo((in_opt_cmd_buffer_ptr != nullptr ? 1 : 0),
                             in_opt_cmd_buffer_ptr,
                             nullptr, /* in_opt_command_buffer_submissions_ptr */
                             in_n_semaphores_to_signal,
                             in_opt_semaphore_to_signal_ptrs_ptr,
                             in_n_semaphores_to_wait_on,
                             in_opt_semaphore_to_wait_on_ptrs_ptr,
                             in_opt_dst_stage_masks_to_wait_on_ptrs,
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create(uint32_t                         in_n_cmd_buffers,
                                            Anvil::CommandBufferBase* const* in_opt_cmd_buffer_ptrs_ptr,
                                            uint32_t                         in_n_semaphores_to_signal,
                                            Anvil::Semaphore* const*         in_opt_semaphore_to_signal_ptrs_ptr,
                                            uint32_t                         in_n_semaphores_to_wait_on,
                                            Anvil::Semaphore* const*         in_opt_semaphore_to_wait_on_ptrs_ptr,
                                            const Anvil::PipelineStageFlags* in_opt_dst_stage_masks_to_wait_on_ptrs,
                                            bool                             in_should_block,
                                            Anvil::Fence*                    in_opt_fence_ptr)
{
    return Anvil::SubmitInfo(in_n_cmd_buffers,
                             nullptr, /* in_opt_command_buffer_single_submission_ptr */
                             in_opt_cmd_buffer_ptrs_ptr,
                             in_n_semaphores_to_signal,
                             in_opt_semaphore_to_signal_ptrs_ptr,
                             in_n_semaphores_to_wait_on,
                             in_opt_semaphore_to_wait_on_ptrs_ptr,
                             in_opt_dst_stage_masks_to_wait_on_ptrs,
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_execute(Anvil::CommandBufferBase* in_cmd_buffer_ptr,
                                                    bool                      in_should_block,
                                                    Anvil::Fence*             in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_ptr != nullptr);

    return Anvil::SubmitInfo(1,       /* in_n_cmd_buffers                       */
                             in_cmd_buffer_ptr,
                             nullptr, /* in_opt_command_buffer_submissions_ptr  */
                             0,       /* in_n_semaphores_to_signal              */
                             nullptr, /* in_opt_semaphore_to_signal_ptrs_ptr    */
                             0,       /* in_n_semaphores_to_wait_on             */
                             nullptr, /* in_opt_semaphore_to_wait_on_ptrs_ptr   */
                             nullptr, /* in_opt_dst_stage_masks_to_wait_on_ptrs */
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_execute(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                                    uint32_t                         in_n_cmd_buffers,
                                                    bool                             in_should_block,
                                                    Anvil::Fence*                    in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_ptrs_ptr != nullptr);
    anvil_assert(in_n_cmd_buffers       >  0);

    return Anvil::SubmitInfo(in_n_cmd_buffers,
                             nullptr, /* in_opt_command_buffer_single_submission_ptr */
                             in_cmd_buffer_ptrs_ptr,
                             0,       /* in_n_semaphores_to_signal              */
                             nullptr, /* in_opt_semaphore_to_signal_ptrs_ptr    */
                             0,       /* in_n_semaphores_to_wait_on             */
                             nullptr, /* in_opt_semaphore_to_wait_on_ptrs_ptr   */
                             nullptr, /* in_opt_dst_stage_masks_to_wait_on_ptrs */
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_execute(const CommandBufferMGPUSubmission* in_cmd_buffer_submissions_ptr,
                                                    uint32_t                           in_n_command_buffer_submissions,
                                                    bool                               in_should_block,
                                                    Anvil::Fence*                      in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_submissions_ptr   != nullptr);
    anvil_assert(in_n_command_buffer_submissions >  0);

    return Anvil::SubmitInfo(in_n_command_buffer_submissions,
                             in_cmd_buffer_submissions_ptr,
                             0,       /* in_n_semaphores_to_signal                   */
                             nullptr, /* in_opt_semaphore_to_signal_ptrs_ptr         */
                             0,       /* in_n_semaphores_to_wait_on                  */
                             nullptr, /* in_opt_semaphore_to_wait_on_ptrs_ptr        */
                             nullptr, /* in_opt_dst_stage_masks_to_wait_on_ptrs      */
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_execute_signal(Anvil::CommandBufferBase* in_cmd_buffer_ptr,
                                                           uint32_t                  in_n_semaphores_to_signal,
                                                           Anvil::Semaphore* const*  in_semaphore_to_signal_ptrs_ptr,
                                                           bool                      in_should_block,
                                                           Anvil::Fence*             in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_ptr               != nullptr);
    anvil_assert(in_semaphore_to_signal_ptrs_ptr != nullptr);

    return Anvil::SubmitInfo(1,       /* in_n_cmd_buffers                       */
                             in_cmd_buffer_ptr,
                             nullptr, /* in_opt_command_buffer_submissions_ptr  */
                             in_n_semaphores_to_signal,
                             in_semaphore_to_signal_ptrs_ptr,
                             0,       /* in_n_semaphores_to_wait_on             */
                             nullptr, /* in_opt_semaphore_to_wait_on_ptrs_ptr   */
                             nullptr, /* in_opt_dst_stage_masks_to_wait_on_ptrs */
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_execute_signal(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                                           uint32_t                         in_n_cmd_buffers,
                                                           uint32_t                         in_n_semaphores_to_signal,
                                                           Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                                           bool                             in_should_block,
                                                           Anvil::Fence*                    in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_ptrs_ptr          != nullptr);
    anvil_assert(in_n_cmd_buffers                >  0);
    anvil_assert(in_semaphore_to_signal_ptrs_ptr != nullptr);

    return Anvil::SubmitInfo(in_n_cmd_buffers,
                             nullptr, /* in_opt_command_buffer_single_submission_ptr */
                             in_cmd_buffer_ptrs_ptr,
                             in_n_semaphores_to_signal,
                             in_semaphore_to_signal_ptrs_ptr,
                             0,       /* in_n_semaphores_to_wait_on             */
                             nullptr, /* in_opt_semaphore_to_wait_on_ptrs_ptr   */
                             nullptr, /* in_opt_dst_stage_masks_to_wait_on_ptrs */
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_execute_signal(const Anvil::CommandBufferMGPUSubmission* in_cmd_buffer_submissions_ptr,
                                                           uint32_t                                  in_n_command_buffer_submissions,
                                                           uint32_t                                  in_n_signal_semaphore_submissions,
                                                           const Anvil::SemaphoreMGPUSubmission*     in_signal_semaphore_submissions_ptr,
                                                           bool                                      in_should_block,
                                                           Anvil::Fence*                             in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_submissions_ptr       != nullptr);
    anvil_assert(in_n_command_buffer_submissions     >  0);
    anvil_assert(in_signal_semaphore_submissions_ptr != nullptr);

    return Anvil::SubmitInfo(in_n_command_buffer_submissions,
                             in_cmd_buffer_submissions_ptr,
                             in_n_signal_semaphore_submissions,
                             in_signal_semaphore_submissions_ptr,
                             0,       /* in_n_semaphores_to_wait_on             */
                             nullptr, /* in_opt_semaphore_to_wait_on_ptrs_ptr   */
                             nullptr, /* in_opt_dst_stage_masks_to_wait_on_ptrs */
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_signal(uint32_t                 in_n_semaphores_to_signal,
                                                   Anvil::Semaphore* const* in_semaphore_to_signal_ptrs_ptr,
                                                   Anvil::Fence*            in_opt_fence_ptr)
{
    anvil_assert(in_n_semaphores_to_signal       >  0);
    anvil_assert(in_semaphore_to_signal_ptrs_ptr != nullptr);

    return Anvil::SubmitInfo(0,       /* in_n_command_buffers */
                             nullptr, /* in_opt_command_buffer_single_submission_ptr */
                             nullptr, /* in_opt_command_buffer_submissions_ptr       */
                             in_n_semaphores_to_signal,
                             in_semaphore_to_signal_ptrs_ptr,
                             0,       /* in_n_semaphores_to_wait_on         */
                             nullptr, /* in_semaphore_to_wait_on_ptrs_ptr   */
                             nullptr, /* in_dst_stage_masks_to_wait_on_ptrs */
                             true,    /* in_should_block                    */
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_signal(uint32_t                              in_n_signal_semaphore_submissions,
                                                   const Anvil::SemaphoreMGPUSubmission* in_signal_semaphore_submissions_ptr,
                                                   Anvil::Fence*                         in_opt_fence_ptr)
{
    anvil_assert(in_n_signal_semaphore_submissions   >  0);
    anvil_assert(in_signal_semaphore_submissions_ptr != nullptr);

   return Anvil::SubmitInfo(0,                                   /* in_n_command_buffer_submissions,        */
                            nullptr,                             /* in_opt_command_buffer_submissions_ptr,  */
                            in_n_signal_semaphore_submissions,
                            in_signal_semaphore_submissions_ptr,
                            0,                                   /* in_n_wait_semaphore_submissions,        */
                            nullptr,                             /* in_opt_wait_semaphore_submissions_ptr,  */
                            nullptr,                             /* in_opt_dst_stage_masks_to_wait_on_ptr,  */
                            true,                                /* in_should_block,                        */
                            in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_signal_wait(uint32_t                         in_n_semaphores_to_signal,
                                                        Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                                        uint32_t                         in_n_semaphores_to_wait_on,
                                                        Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                                        const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                                        bool                             in_should_block,
                                                        Anvil::Fence*                    in_opt_fence_ptr)
{
    anvil_assert(in_n_semaphores_to_signal        >  0);
    anvil_assert(in_semaphore_to_signal_ptrs_ptr  != nullptr);
    anvil_assert(in_n_semaphores_to_wait_on       >  0);
    anvil_assert(in_semaphore_to_wait_on_ptrs_ptr != nullptr);

    return Anvil::SubmitInfo(0,       /* in_n_command_buffers                        */
                             nullptr, /* in_opt_command_buffer_single_submission_ptr */
                             nullptr, /* in_opt_command_buffer_submissions_ptr       */
                             in_n_semaphores_to_signal,
                             in_semaphore_to_signal_ptrs_ptr,
                             in_n_semaphores_to_wait_on,
                             in_semaphore_to_wait_on_ptrs_ptr,
                             in_dst_stage_masks_to_wait_on_ptrs,
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_signal_wait(uint32_t                              in_n_signal_semaphore_submissions,
                                                        const Anvil::SemaphoreMGPUSubmission* in_signal_semaphore_submissions_ptr,
                                                        uint32_t                              in_n_wait_semaphore_submissions,
                                                        const Anvil::SemaphoreMGPUSubmission* in_wait_semaphore_submissions_ptr,
                                                        const Anvil::PipelineStageFlags*      in_dst_stage_masks_to_wait_on_ptrs,
                                                        bool                                  in_should_block,
                                                        Anvil::Fence*                         in_opt_fence_ptr)
{
    anvil_assert(in_n_signal_semaphore_submissions   >  0);
    anvil_assert(in_signal_semaphore_submissions_ptr != nullptr);
    anvil_assert(in_n_wait_semaphore_submissions     >  0);
    anvil_assert(in_wait_semaphore_submissions_ptr   != nullptr);

    return Anvil::SubmitInfo(0,       /* in_n_command_buffers                  */
                             nullptr, /* in_opt_command_buffer_submissions_ptr */
                             in_n_signal_semaphore_submissions,
                             in_signal_semaphore_submissions_ptr,
                             in_n_wait_semaphore_submissions,
                             in_wait_semaphore_submissions_ptr,
                             in_dst_stage_masks_to_wait_on_ptrs,
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_wait(uint32_t                         in_n_semaphores_to_wait_on,
                                                 Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                                 const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                                 Anvil::Fence*                    in_opt_fence_ptr)
{
    anvil_assert(in_dst_stage_masks_to_wait_on_ptrs != nullptr);
    anvil_assert(in_n_semaphores_to_wait_on         >  0);
    anvil_assert(in_semaphore_to_wait_on_ptrs_ptr   != nullptr);

    return Anvil::SubmitInfo(0,       /* in_n_command_buffers                        */
                             nullptr, /* in_opt_command_buffer_single_submission_ptr */
                             nullptr, /* in_opt_command_buffer_submissions_ptr       */
                             0,       /* in_n_semaphores_to_signal                   */
                             nullptr, /* in_semaphore_to_signal_ptrs_ptr             */
                             in_n_semaphores_to_wait_on,
                             in_semaphore_to_wait_on_ptrs_ptr,
                             in_dst_stage_masks_to_wait_on_ptrs,
                             true,    /* in_should_block */
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_wait(uint32_t                               in_n_wait_semaphore_submissions,
                                                 const Anvil::SemaphoreMGPUSubmission*  in_wait_semaphore_submissions_ptr,
                                                 const Anvil::PipelineStageFlags*       in_dst_stage_masks_to_wait_on_ptrs,
                                                 Anvil::Fence*                          in_opt_fence_ptr)
{
    anvil_assert(in_dst_stage_masks_to_wait_on_ptrs != nullptr);
    anvil_assert(in_n_wait_semaphore_submissions    >  0);
    anvil_assert(in_wait_semaphore_submissions_ptr  != nullptr);

    return Anvil::SubmitInfo(0,                                   /* in_n_command_buffer_submissions,        */
                             nullptr,                             /* in_opt_command_buffer_submissions_ptr,  */
                             0,                                   /* in_n_signal_semaphore_submissions,      */
                             nullptr,                             /* in_opt_signal_semaphore_submissions_ptr,*/
                             in_n_wait_semaphore_submissions,
                             in_wait_semaphore_submissions_ptr,
                             in_dst_stage_masks_to_wait_on_ptrs,
                             true,                                /* in_should_block,                        */
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_wait_execute(Anvil::CommandBufferBase*        in_cmd_buffer_ptr,
                                                         uint32_t                         in_n_semaphores_to_wait_on,
                                                         Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                                         const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                                         bool                             in_should_block,
                                                         Anvil::Fence*                    in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_ptr                  != nullptr);
    anvil_assert(in_dst_stage_masks_to_wait_on_ptrs != nullptr);
    anvil_assert(in_semaphore_to_wait_on_ptrs_ptr   != nullptr);

    return Anvil::SubmitInfo(1,       /* in_n_command_buffers                  */
                             in_cmd_buffer_ptr,
                             nullptr, /* in_opt_command_buffer_submissions_ptr */
                             0,       /* in_n_semaphores_to_signal             */
                             nullptr, /* in_opt_semaphore_to_signal_ptrs_ptr   */
                             in_n_semaphores_to_wait_on,
                             in_semaphore_to_wait_on_ptrs_ptr,
                             in_dst_stage_masks_to_wait_on_ptrs,
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_wait_execute(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                                         uint32_t                         in_n_cmd_buffers,
                                                         uint32_t                         in_n_semaphores_to_wait_on,
                                                         Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                                         const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                                         bool                             in_should_block,
                                                         Anvil::Fence*                    in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_ptrs_ptr             != nullptr);
    anvil_assert(in_n_cmd_buffers                   >  0);
    anvil_assert(in_dst_stage_masks_to_wait_on_ptrs != nullptr);
    anvil_assert(in_semaphore_to_wait_on_ptrs_ptr   != nullptr);

    return Anvil::SubmitInfo(in_n_cmd_buffers,
                             nullptr, /* in_opt_command_buffer_single_submission_ptr */
                             in_cmd_buffer_ptrs_ptr,
                             0,       /* in_n_semaphores_to_signal                   */
                             nullptr, /* in_opt_semaphore_to_signal_ptrs_ptr         */
                             in_n_semaphores_to_wait_on,
                             in_semaphore_to_wait_on_ptrs_ptr,
                             in_dst_stage_masks_to_wait_on_ptrs,
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_wait_execute(const Anvil::CommandBufferMGPUSubmission* in_cmd_buffer_submissions_ptr,
                                                         uint32_t                                  in_n_command_buffer_submissions,
                                                         uint32_t                                  in_n_wait_semaphore_submissions,
                                                         const Anvil::SemaphoreMGPUSubmission*     in_wait_semaphore_submissions_ptr,
                                                         const Anvil::PipelineStageFlags*          in_dst_stage_masks_to_wait_on_ptrs,
                                                         bool                                      in_should_block,
                                                         Anvil::Fence*                             in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_submissions_ptr     != nullptr);
    anvil_assert(in_n_command_buffer_submissions   >  0);
    anvil_assert(in_wait_semaphore_submissions_ptr != nullptr);

    return Anvil::SubmitInfo(in_n_command_buffer_submissions,
                             in_cmd_buffer_submissions_ptr,
                             0,       /* in_n_semaphores_to_signal           */
                             nullptr, /* in_opt_semaphore_to_signal_ptrs_ptr */
                             in_n_wait_semaphore_submissions,
                             in_wait_semaphore_submissions_ptr,
                             in_dst_stage_masks_to_wait_on_ptrs,
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_wait_execute_signal(Anvil::CommandBufferBase*        in_cmd_buffer_ptr,
                                                                uint32_t                         in_n_semaphores_to_signal,
                                                                Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                                                uint32_t                         in_n_semaphores_to_wait_on,
                                                                Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                                                const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                                                bool                             in_should_block,
                                                                Anvil::Fence*                    in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_ptr                != nullptr);
    anvil_assert(in_semaphore_to_signal_ptrs_ptr  != nullptr);
    anvil_assert(in_semaphore_to_wait_on_ptrs_ptr != nullptr);

    return Anvil::SubmitInfo(1,        /* in_n_command_buffers                 */
                             in_cmd_buffer_ptr,
                             nullptr, /* in_opt_command_buffer_submissions_ptr */
                             in_n_semaphores_to_signal,
                             in_semaphore_to_signal_ptrs_ptr,
                             in_n_semaphores_to_wait_on,
                             in_semaphore_to_wait_on_ptrs_ptr,
                             in_dst_stage_masks_to_wait_on_ptrs,
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_wait_execute_signal(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                                                uint32_t                         in_n_cmd_buffers,
                                                                uint32_t                         in_n_semaphores_to_signal,
                                                                Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                                                uint32_t                         in_n_semaphores_to_wait_on,
                                                                Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                                                const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                                                bool                             in_should_block,
                                                                Anvil::Fence*                    in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_ptrs_ptr           != nullptr);
    anvil_assert(in_n_cmd_buffers                 >  0);
    anvil_assert(in_semaphore_to_signal_ptrs_ptr  != nullptr);
    anvil_assert(in_semaphore_to_wait_on_ptrs_ptr != nullptr);

    return Anvil::SubmitInfo(in_n_cmd_buffers,
                             nullptr, /* in_opt_command_buffer_single_submission_ptr */
                             in_cmd_buffer_ptrs_ptr,
                             in_n_semaphores_to_signal,
                             in_semaphore_to_signal_ptrs_ptr,
                             in_n_semaphores_to_wait_on,
                             in_semaphore_to_wait_on_ptrs_ptr,
                             in_dst_stage_masks_to_wait_on_ptrs,
                             in_should_block,
                             in_opt_fence_ptr);
}

Anvil::SubmitInfo Anvil::SubmitInfo::create_wait_execute_signal(const Anvil::CommandBufferMGPUSubmission* in_cmd_buffer_submissions_ptr,
                                                                uint32_t                                  in_n_command_buffer_submissions,
                                                                uint32_t                                  in_n_signal_semaphore_submissions,
                                                                const Anvil::SemaphoreMGPUSubmission*     in_signal_semaphore_submissions_ptr,
                                                                uint32_t                                  in_n_wait_semaphore_submissions,
                                                                const Anvil::SemaphoreMGPUSubmission*     in_wait_semaphore_submissions_ptr,
                                                                const Anvil::PipelineStageFlags*          in_dst_stage_masks_to_wait_on_ptrs,
                                                                bool                                      in_should_block,
                                                                Anvil::Fence*                             in_opt_fence_ptr)
{
    anvil_assert(in_cmd_buffer_submissions_ptr       != nullptr);
    anvil_assert(in_n_command_buffer_submissions     >  0);
    anvil_assert(in_dst_stage_masks_to_wait_on_ptrs  != nullptr);
    anvil_assert(in_signal_semaphore_submissions_ptr != nullptr);
    anvil_assert(in_wait_semaphore_submissions_ptr   != nullptr);

    return Anvil::SubmitInfo(in_n_command_buffer_submissions,
                             in_cmd_buffer_submissions_ptr,
                             in_n_signal_semaphore_submissions,
                             in_signal_semaphore_submissions_ptr,
                             in_n_wait_semaphore_submissions,
                             in_wait_semaphore_submissions_ptr,
                             in_dst_stage_masks_to_wait_on_ptrs,
                             in_should_block,
                             in_opt_fence_ptr);
}

