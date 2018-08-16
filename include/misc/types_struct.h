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
#ifndef TYPES_STRUCT_H
#define TYPES_STRUCT_H

namespace Anvil
{
    typedef struct ExternalFenceProperties
    {
        Anvil::ExternalFenceHandleTypeBits compatible_external_handle_types;
        Anvil::ExternalFenceHandleTypeBits export_from_imported_external_handle_types;

        bool is_exportable;
        bool is_importable;

        /* Dummy constructor */
        ExternalFenceProperties();

        ExternalFenceProperties(const VkExternalFencePropertiesKHR& in_external_memory_props);
    } ExternalFenceProperties;

    typedef struct ExternalMemoryProperties
    {
        Anvil::ExternalMemoryHandleTypeBits compatible_external_handle_types;
        Anvil::ExternalMemoryHandleTypeBits export_from_imported_external_handle_types;

        bool is_exportable;
        bool is_importable;

        /* Dummy constructor */
        ExternalMemoryProperties();

        ExternalMemoryProperties(const VkExternalMemoryPropertiesKHR& in_external_memory_props);
    } ExternalMemoryProperties;

    typedef struct ExternalSemaphoreProperties
    {
        Anvil::ExternalSemaphoreHandleTypeBits compatible_external_handle_types;
        Anvil::ExternalSemaphoreHandleTypeBits export_from_imported_external_handle_types;

        bool is_exportable;
        bool is_importable;

        /* Dummy constructor */
        ExternalSemaphoreProperties();

        ExternalSemaphoreProperties(const VkExternalSemaphorePropertiesKHR& in_external_memory_props);
    } ExternalSemaphoreProperties;

    /* Holds shader core properties pertaining to a physical device */
    typedef struct AMDShaderCoreProperties
    {
        uint32_t shader_engine_count;            ///< Number of shader engines.
        uint32_t shader_arrays_per_engine_count; ///< Number of shader arrays.
        uint32_t compute_units_per_shader_array; ///< Number of CUs per shader array.
        uint32_t simd_per_compute_unit;          ///< Number of SIMDs per compute unit.
        uint32_t wavefronts_per_simd;            ///< Number of wavefront slots in each SIMD.
        uint32_t wavefront_size;                 ///< Wavefront size.
        uint32_t sgprs_per_simd;                 ///< Number of physical SGPRs per SIMD.
        uint32_t min_sgpr_allocation;            ///< Minimum number of SGPRs that can be allocated by a wave.
        uint32_t max_sgpr_allocation;            ///< Number of available SGPRs.
        uint32_t sgpr_allocation_granularity;    ///< SGPRs are allocated in groups of this size.  Meaning, if your shader
                                                 ///  only uses 1 SGPR, you will still end up reserving this number of
                                                 ///  SGPRs.
        uint32_t vgprs_per_simd;                 ///< Number of physical VGPRs per SIMD.
        uint32_t min_vgpr_allocation;            ///< Minimum number of VGPRs that can be allocated by a wave.
        uint32_t max_vgpr_allocation;            ///< Number of available VGPRs.
        uint32_t vgpr_allocation_granularity;    ///< VGPRs are allocated in groups of this size.  Meaning, if your shader
                                                 ///  only uses 1 VGPR, you will still end up reserving this number of
                                                 ///  VGPRs.

        AMDShaderCoreProperties();
        AMDShaderCoreProperties(const VkPhysicalDeviceShaderCorePropertiesAMD& in_props);

        bool operator==(const AMDShaderCoreProperties& in_props) const;
    } AMDShaderCoreProperties;

    /** Describes a buffer memory barrier. */
    typedef struct BufferBarrier
    {
        VkAccessFlagsVariable(dst_access_mask);
        VkAccessFlagsVariable(src_access_mask);

        VkBuffer              buffer;
        VkBufferMemoryBarrier buffer_barrier_vk;
        Anvil::Buffer*        buffer_ptr;
        uint32_t              dst_queue_family_index;
        VkDeviceSize          offset;
        VkDeviceSize          size;
        uint32_t              src_queue_family_index;

        /** Constructor.
         *
         *  Note that @param in_buffer_ptr is retained by this function.
         *
         *  @param in_source_access_mask      Source access mask to use for the barrier.
         *  @param in_destination_access_mask Destination access mask to use for the barrier.
         *  @param in_src_queue_family_index  Source queue family index to use for the barrier.
         *  @param in_dst_queue_family_index  Destination queue family index to use for the barrier.
         *  @param in_buffer_ptr              Pointer to a Buffer instance the instantiated barrier
         *                                    refers to. Must not be nullptr.
         *  @param in_offset                  Start offset of the region described by the barrier.
         *  @param in_size                    Size of the region described by the barrier.
         **/
        explicit BufferBarrier(VkAccessFlags  in_source_access_mask,
                               VkAccessFlags  in_destination_access_mask,
                               uint32_t       in_src_queue_family_index,
                               uint32_t       in_dst_queue_family_index,
                               Anvil::Buffer* in_buffer_ptr,
                               VkDeviceSize   in_offset,
                               VkDeviceSize   in_size);

        /** Destructor.
         *
         *  Releases the encapsulated Buffer instance.
         **/
        virtual ~BufferBarrier();

        /** Copy constructor.
         *
         *  Retains the Buffer instance stored in the input barrier.
         *
         *  @param in Barrier instance to copy data from.
         **/
        BufferBarrier(const BufferBarrier& in);

        /** Returns a Vulkan buffer memory barrier descriptor, whose configuration corresponds to
         *  to the configuration of this descriptor.
         **/
        virtual VkBufferMemoryBarrier get_barrier_vk() const
        {
            return buffer_barrier_vk;
        }

        /** Returns a pointer to the Vulkan descriptor, whose configuration corresponds to
         *  the configuration of this descriptor.
         *
         *  The returned pointer remains valid for the duration of the Barrier descriptor's
         *  life-time.
         **/
        const VkBufferMemoryBarrier* get_barrier_vk_ptr() const
        {
            return &buffer_barrier_vk;
        }

    private:
        BufferBarrier& operator=(const BufferBarrier&);
    } BufferBarrier;

    typedef struct BufferProperties
    {
        ExternalMemoryProperties external_handle_properties;

        BufferProperties();
        BufferProperties(const ExternalMemoryProperties& in_external_handle_properties);

        BufferProperties           (const BufferProperties&) = default;
        BufferProperties& operator=(const BufferProperties&) = default;
    } BufferProperties;

    typedef struct BufferPropertiesQuery
    {
        const Anvil::BufferCreateFlags           create_flags;
        const Anvil::ExternalMemoryHandleTypeBit external_memory_handle_type;
        const VkBufferUsageFlags                 usage_flags;

        explicit BufferPropertiesQuery(const Anvil::BufferCreateFlags            in_create_flags,
                                       const Anvil::ExternalMemoryHandleTypeBit& in_external_memory_handle_type,
                                       const VkBufferUsageFlags&                 in_usage_flags)
            :create_flags               (in_create_flags),
             external_memory_handle_type(in_external_memory_handle_type),
             usage_flags                (in_usage_flags)
        {
            /* Stub */
        }

        BufferPropertiesQuery           (const BufferPropertiesQuery& in_query) = default;
        BufferPropertiesQuery& operator=(const BufferPropertiesQuery& in_query) = delete;
    } BufferPropertiesQuery;

    #if defined(_WIN32)
        typedef struct ExternalNTHandleInfo
        {
            DWORD                      access;
            const SECURITY_ATTRIBUTES* attributes_ptr;
            std::wstring               name;

            ExternalNTHandleInfo()
            {
                access         = 0;
                attributes_ptr = nullptr;
            }
        } ExternalNTHandleInfo;
    #endif

    typedef struct EXTDescriptorIndexingFeatures
    {
        bool descriptor_binding_partially_bound;
        bool descriptor_binding_sampled_image_update_after_bind;
        bool descriptor_binding_storage_buffer_update_after_bind;
        bool descriptor_binding_storage_image_update_after_bind;
        bool descriptor_binding_storage_texel_buffer_update_after_bind;
        bool descriptor_binding_uniform_buffer_update_after_bind;
        bool descriptor_binding_uniform_texel_buffer_update_after_bind;
        bool descriptor_binding_update_unused_while_pending;
        bool descriptor_binding_variable_descriptor_count;
        bool runtime_descriptor_array;
        bool shader_input_attachment_array_dynamic_indexing;
        bool shader_input_attachment_array_non_uniform_indexing;
        bool shader_sampled_image_array_non_uniform_indexing;
        bool shader_storage_buffer_array_non_uniform_indexing;
        bool shader_storage_image_array_non_uniform_indexing;
        bool shader_storage_texel_buffer_array_dynamic_indexing;
        bool shader_storage_texel_buffer_array_non_uniform_indexing;
        bool shader_uniform_buffer_array_non_uniform_indexing;
        bool shader_uniform_texel_buffer_array_dynamic_indexing;
        bool shader_uniform_texel_buffer_array_non_uniform_indexing;

        EXTDescriptorIndexingFeatures();
        EXTDescriptorIndexingFeatures(const VkPhysicalDeviceDescriptorIndexingFeaturesEXT& in_features);

        VkPhysicalDeviceDescriptorIndexingFeaturesEXT get_vk_physical_device_descriptor_indexing_features() const;

        bool operator==(const EXTDescriptorIndexingFeatures& in_features) const;
    } EXTDescriptorIndexingFeatures;

    typedef struct EXTDescriptorIndexingProperties
    {
        uint32_t max_descriptor_set_update_after_bind_input_attachments;
        uint32_t max_descriptor_set_update_after_bind_sampled_images;
        uint32_t max_descriptor_set_update_after_bind_samplers;
        uint32_t max_descriptor_set_update_after_bind_storage_buffers;
        uint32_t max_descriptor_set_update_after_bind_storage_buffers_dynamic;
        uint32_t max_descriptor_set_update_after_bind_storage_images;
        uint32_t max_descriptor_set_update_after_bind_uniform_buffers;
        uint32_t max_descriptor_set_update_after_bind_uniform_buffers_dynamic;
        uint32_t max_per_stage_descriptor_update_after_bind_input_attachments;
        uint32_t max_per_stage_descriptor_update_after_bind_sampled_images;
        uint32_t max_per_stage_descriptor_update_after_bind_samplers;
        uint32_t max_per_stage_descriptor_update_after_bind_storage_buffers;
        uint32_t max_per_stage_descriptor_update_after_bind_storage_images;
        uint32_t max_per_stage_descriptor_update_after_bind_uniform_buffers;
        uint32_t max_per_stage_update_after_bind_resources;
        uint32_t max_update_after_bind_descriptors_in_all_pools;
        bool     shader_input_attachment_array_non_uniform_indexing_native;
        bool     shader_sampled_image_array_non_uniform_indexing_native;
        bool     shader_storage_buffer_array_non_uniform_indexing_native;
        bool     shader_storage_image_array_non_uniform_indexing_native;
        bool     shader_uniform_buffer_array_non_uniform_indexing_native;

        EXTDescriptorIndexingProperties();
        EXTDescriptorIndexingProperties(const VkPhysicalDeviceDescriptorIndexingPropertiesEXT& in_props);

        bool operator==(const EXTDescriptorIndexingProperties& in_props) const;
    } EXTDescriptorIndexingProperties;

    typedef struct EXTVertexAttributeDivisorProperties
    {
        uint32_t max_vertex_attribute_divisor;

        EXTVertexAttributeDivisorProperties();
        EXTVertexAttributeDivisorProperties(const VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT& in_props);
    } EXTVertexAttributeDivisorProperties;

    typedef struct DescriptorSetAllocation
    {
        /* Descriptor set layout to use for the allocation request */
        const Anvil::DescriptorSetLayout* ds_layout_ptr;

        /* Number of descriptors to use for the variable descriptor binding defined in the DS layout.
         *
         * This value is only required if ds_layout_ptr contains a binding created with the
         * DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT flag. Otherwise, it is ignored.
         *
         */
        uint32_t n_variable_descriptor_bindings;

        /* Dummy constructor. Do not use as input for DS allocation functions. */
        DescriptorSetAllocation()
        {
            ds_layout_ptr                  = nullptr;
            n_variable_descriptor_bindings = UINT32_MAX;
        }

        /* Constructor.
         *
         * Use if you need to allocate a descriptor set using a descriptor set layout which
         * does NOT contain a variable count descriptor binding.
         */
        DescriptorSetAllocation(const Anvil::DescriptorSetLayout* in_ds_layout_ptr);

        /* Constructor.
         *
         * Use if you need to allocate a descriptor set using a descriptor set layout which
         * CONTAINS a variable count descriptor binding.
         */
        DescriptorSetAllocation(const Anvil::DescriptorSetLayout* in_ds_layout_ptr,
                                const uint32_t&                   in_n_variable_descriptor_bindings);
    } DescriptorSetAllocation;

    typedef struct DescriptorUpdateTemplateEntry
    {
        VkDescriptorType descriptor_type;
        uint32_t         n_descriptors;
        uint32_t         n_destination_array_element;
        uint32_t         n_destination_binding;
        size_t           offset;
        size_t           stride;

        DescriptorUpdateTemplateEntry();
        DescriptorUpdateTemplateEntry(const VkDescriptorType& in_descriptor_type,
                                      const uint32_t&         in_n_destination_array_element,
                                      const uint32_t&         in_n_destination_binding,
                                      const uint32_t&         in_n_descriptors,
                                      const size_t&           in_offset,
                                      const size_t&           in_stride);

        VkDescriptorUpdateTemplateEntryKHR get_vk_descriptor_update_template_entry_khr() const;

        bool operator==(const DescriptorUpdateTemplateEntry& in_entry) const;
        bool operator< (const DescriptorUpdateTemplateEntry& in_entry) const;
    } DescriptorUpdateTemplateEntry;

    typedef struct ExtensionAMDDrawIndirectCountEntrypoints
    {
        PFN_vkCmdDrawIndexedIndirectCountAMD vkCmdDrawIndexedIndirectCountAMD;
        PFN_vkCmdDrawIndirectCountAMD        vkCmdDrawIndirectCountAMD;

        ExtensionAMDDrawIndirectCountEntrypoints();
    } ExtensionAMDDrawIndirectCountEntrypoints;

    typedef struct ExtensionAMDShaderInfoEntrypoints
    {
        PFN_vkGetShaderInfoAMD vkGetShaderInfoAMD;

        ExtensionAMDShaderInfoEntrypoints();
    } ExtensionAMDShaderInfoEntrypoints;

    typedef struct ExtensionEXTDebugMarkerEntrypoints
    {
        PFN_vkCmdDebugMarkerBeginEXT      vkCmdDebugMarkerBeginEXT;
        PFN_vkCmdDebugMarkerEndEXT        vkCmdDebugMarkerEndEXT;
        PFN_vkCmdDebugMarkerInsertEXT     vkCmdDebugMarkerInsertEXT;
        PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;
        PFN_vkDebugMarkerSetObjectTagEXT  vkDebugMarkerSetObjectTagEXT;

        ExtensionEXTDebugMarkerEntrypoints();
    } ExtensionEXTDebugMarkerEntrypoints;

    typedef struct ExtensionEXTDebugReportEntrypoints
    {
        PFN_vkCreateDebugReportCallbackEXT  vkCreateDebugReportCallbackEXT;
        PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;

        ExtensionEXTDebugReportEntrypoints();
    } ExtensionEXTDebugReportEntrypoints;

    typedef struct ExtensionKHRDrawIndirectCountEntrypoints
    {
        PFN_vkCmdDrawIndexedIndirectCountKHR vkCmdDrawIndexedIndirectCountKHR;
        PFN_vkCmdDrawIndirectCountKHR        vkCmdDrawIndirectCountKHR;

        ExtensionKHRDrawIndirectCountEntrypoints();
    } ExtensionKHRDrawIndirectCountEntrypoints;

    typedef struct ExtensionKHRBindMemory2Entrypoints
    {
        PFN_vkBindBufferMemory2KHR vkBindBufferMemory2KHR;
        PFN_vkBindImageMemory2KHR  vkBindImageMemory2KHR;

        ExtensionKHRBindMemory2Entrypoints();
    } ExtensionKHRBindMemory2Entrypoints;

    typedef struct ExtensionKHRDescriptorUpdateTemplateEntrypoints
    {
        PFN_vkCreateDescriptorUpdateTemplateKHR  vkCreateDescriptorUpdateTemplateKHR;
        PFN_vkDestroyDescriptorUpdateTemplateKHR vkDestroyDescriptorUpdateTemplateKHR;
        PFN_vkUpdateDescriptorSetWithTemplateKHR vkUpdateDescriptorSetWithTemplateKHR;

      ExtensionKHRDescriptorUpdateTemplateEntrypoints();
    } ExtensionKHRDescriptorUpdateTemplateEntrypoints;

    typedef struct ExtensionKHRExternalFenceCapabilitiesEntrypoints
    {
        PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR vkGetPhysicalDeviceExternalFencePropertiesKHR;

        ExtensionKHRExternalFenceCapabilitiesEntrypoints();
    } ExtensionKHRExternalFenceCapabilitiesEntrypoints;

    typedef struct ExtensionKHRExternalMemoryCapabilitiesEntrypoints
    {
        PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR vkGetPhysicalDeviceExternalBufferPropertiesKHR;

        ExtensionKHRExternalMemoryCapabilitiesEntrypoints();
    } ExtensionKHRExternalMemoryCapabilitiesEntrypoints;

    typedef struct ExtensionKHRExternalSemaphoreCapabilitiesEntrypoints
    {
        PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR vkGetPhysicalDeviceExternalSemaphorePropertiesKHR;

        ExtensionKHRExternalSemaphoreCapabilitiesEntrypoints();
    } ExtensionKHRExternalSemaphoreCapabilitiesEntrypoints;

    #if defined(_WIN32)
        typedef struct ExtensionKHRExternalFenceWin32Entrypoints
        {
            PFN_vkGetFenceWin32HandleKHR    vkGetFenceWin32HandleKHR;
            PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR;

            ExtensionKHRExternalFenceWin32Entrypoints();
        } ExtensionKHRExternalFenceWin32Entrypoints;

        typedef struct ExtensionKHRExternalMemoryWin32Entrypoints
        {
            PFN_vkGetMemoryWin32HandleKHR           vkGetMemoryWin32HandleKHR;
            PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR;

            ExtensionKHRExternalMemoryWin32Entrypoints();
        } ExtensionKHRExternalMemoryWin32Entrypoints;

        typedef struct ExtensionKHRExternalSemaphoreWin32Entrypoints
        {
            PFN_vkGetSemaphoreWin32HandleKHR    vkGetSemaphoreWin32HandleKHR;
            PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR;

            ExtensionKHRExternalSemaphoreWin32Entrypoints();
        } ExtensionKHRExternalSemaphoreWin32Entrypoints;
#else
        typedef struct ExtensionKHRExternalFenceFdEntrypoints
        {
            PFN_vkGetFenceFdKHR    vkGetFenceFdKHR;
            PFN_vkImportFenceFdKHR vkImportFenceFdKHR;

            ExtensionKHRExternalFenceFdEntrypoints();
        } ExtensionKHRExternalFenceFdEntrypoints;

        typedef struct ExtensionKHRExternalMemoryFdEntrypoints
        {
            PFN_vkGetMemoryFdKHR           vkGetMemoryFdKHR;
            PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR;

            ExtensionKHRExternalMemoryFdEntrypoints();
        } ExtensionKHRExternalMemoryFdEntrypoints;

        typedef struct ExtensionKHRExternalSemaphoreFdEntrypoints
        {
            PFN_vkGetSemaphoreFdKHR    vkGetSemaphoreFdKHR;
            PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR;

            ExtensionKHRExternalSemaphoreFdEntrypoints();
        } ExtensionKHRExternalSemaphoreFdEntrypoints;
#endif

    typedef struct ExtensionKHRGetMemoryRequirements2Entrypoints
    {
        PFN_vkGetBufferMemoryRequirements2KHR      vkGetBufferMemoryRequirements2KHR;
        PFN_vkGetImageMemoryRequirements2KHR       vkGetImageMemoryRequirements2KHR;
        PFN_vkGetImageSparseMemoryRequirements2KHR vkGetImageSparseMemoryRequirements2KHR;

        ExtensionKHRGetMemoryRequirements2Entrypoints();
    } ExtensionKHRGetMemoryRequirements2Entrypoints;

    typedef struct ExtensionKHRGetPhysicalDeviceProperties2
    {
        PFN_vkGetPhysicalDeviceFeatures2KHR                    vkGetPhysicalDeviceFeatures2KHR;
        PFN_vkGetPhysicalDeviceFormatProperties2KHR            vkGetPhysicalDeviceFormatProperties2KHR;
        PFN_vkGetPhysicalDeviceImageFormatProperties2KHR       vkGetPhysicalDeviceImageFormatProperties2KHR;
        PFN_vkGetPhysicalDeviceMemoryProperties2KHR            vkGetPhysicalDeviceMemoryProperties2KHR;
        PFN_vkGetPhysicalDeviceProperties2KHR                  vkGetPhysicalDeviceProperties2KHR;
        PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR       vkGetPhysicalDeviceQueueFamilyProperties2KHR;
        PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR vkGetPhysicalDeviceSparseImageFormatProperties2KHR;

        ExtensionKHRGetPhysicalDeviceProperties2();
    } ExtensionKHRGetPhysicalDeviceProperties2;

    typedef struct ExtensionKHRMaintenance1Entrypoints
    {
        PFN_vkTrimCommandPoolKHR vkTrimCommandPoolKHR;

        ExtensionKHRMaintenance1Entrypoints();
    } ExtensionKHRMaintenance1Entrypoints;

    typedef struct ExtensionKHRMaintenance3Entrypoints
    {
        PFN_vkGetDescriptorSetLayoutSupportKHR vkGetDescriptorSetLayoutSupportKHR;

        ExtensionKHRMaintenance3Entrypoints();
    } ExtensionKHRMaintenance3Entrypoints;

    typedef struct ExtensionKHRSurfaceEntrypoints
    {
        PFN_vkDestroySurfaceKHR                       vkDestroySurfaceKHR;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      vkGetPhysicalDeviceSurfaceFormatsKHR;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR      vkGetPhysicalDeviceSurfaceSupportKHR;

        ExtensionKHRSurfaceEntrypoints();
    } ExtensionKHRSurfaceEntrypoints;

    typedef struct ExtensionKHRSwapchainEntrypoints
    {
        PFN_vkAcquireNextImageKHR   vkAcquireNextImageKHR;
        PFN_vkCreateSwapchainKHR    vkCreateSwapchainKHR;
        PFN_vkDestroySwapchainKHR   vkDestroySwapchainKHR;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
        PFN_vkQueuePresentKHR       vkQueuePresentKHR;

        ExtensionKHRSwapchainEntrypoints();
    } ExtensionKHRSwapchainEntrypoints;

    #ifdef _WIN32
        #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
            typedef struct ExtensionKHRWin32SurfaceEntrypoints
            {
                PFN_vkCreateWin32SurfaceKHR                        vkCreateWin32SurfaceKHR;
                PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;

                ExtensionKHRWin32SurfaceEntrypoints();
            } ExtensionKHRWin32SurfaceEntrypoints;
        #endif
    #else
        #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
            typedef struct ExtensionKHRXcbSurfaceEntrypoints
            {
                PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;

                ExtensionKHRXcbSurfaceEntrypoints();
            } ExtensionKHRXcbSurfaceEntrypoints;
        #endif
    #endif

    typedef struct ExtensionKHRDeviceGroupCreationEntrypoints
    {
        PFN_vkEnumeratePhysicalDeviceGroupsKHR vkEnumeratePhysicalDeviceGroupsKHR;

        ExtensionKHRDeviceGroupCreationEntrypoints();
    } ExtensionKHRDeviceGroupCreationEntrypoints;

    typedef struct FenceProperties
    {
        ExternalFenceProperties external_fence_properties;

        /* Dummy constructor */
        FenceProperties();

        FenceProperties(const ExternalFenceProperties& in_external_fence_properties);
    } FenceProperties;

    typedef struct FencePropertiesQuery
    {
        const Anvil::ExternalFenceHandleTypeBit external_fence_handle_type;

        explicit FencePropertiesQuery(const Anvil::ExternalFenceHandleTypeBit& in_external_fence_handle_type)
            :external_fence_handle_type(in_external_fence_handle_type)
        {
            /* Stub */
        }

        FencePropertiesQuery           (const FencePropertiesQuery&) = default;
        FencePropertiesQuery& operator=(const FencePropertiesQuery&) = delete;
    } FencePropertiesQuery;

    typedef struct FormatProperties
    {
        VkFormatFeatureFlagsVariable(buffer_capabilities);
        VkFormatFeatureFlagsVariable(linear_tiling_capabilities);
        VkFormatFeatureFlagsVariable(optimal_tiling_capabilities);

        FormatProperties();
        FormatProperties(const VkFormatProperties& in_format_props);
    } FormatProperties;

    typedef struct ImageFormatProperties
    {
        ExternalMemoryProperties external_handle_properties;

        VkExtent3D         max_extent;
        VkDeviceSize       max_resource_size;
        uint32_t           n_max_array_layers;
        uint32_t           n_max_mip_levels;
        VkSampleCountFlags sample_counts;

        /* Tells whether the format can be used with functions introduced in VK_AMD_texture_gather_bias_lod */
        bool supports_amd_texture_gather_bias_lod;


        /** Dummy constructor */
        ImageFormatProperties();

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param in_format_props Vulkan structure to use for initialization.
         **/
        ImageFormatProperties(const VkImageFormatProperties&   in_image_format_props,
                              const bool&                      in_supports_amd_texture_gather_bias_lod,
                              const ExternalMemoryProperties& in_external_handle_properties);
    } ImageFormatProperties;

    typedef struct ImageFormatPropertiesQuery
    {
        /* TODO
         *
         * NOTE: In order to retrieve information regarding device's external handle support for a particular image
         *       configuration, make sure to call ImageFormatPropertiesQuery::set_external_memory_handle_type(), prior
         *       to passing the struct instance as an arg to get_image_format_properties().
         */
        explicit ImageFormatPropertiesQuery(const VkFormat&           in_format,
                                            const VkImageType&        in_image_type,
                                            const VkImageTiling&      in_tiling,
                                            const VkImageUsageFlags&  in_usage_flags,
                                            const VkImageCreateFlags& in_create_flags)
            :create_flags               (in_create_flags),
             external_memory_handle_type(EXTERNAL_MEMORY_HANDLE_TYPE_NONE),
             format                     (in_format),
             image_type                 (in_image_type),
             tiling                     (in_tiling),
             usage_flags                (in_usage_flags)
        {
            /* Stub */
        }

        void set_external_memory_handle_type(const Anvil::ExternalMemoryHandleTypeBit& in_external_memory_handle_type)
        {
            external_memory_handle_type = in_external_memory_handle_type;
        }

        const VkImageCreateFlags           create_flags;
        Anvil::ExternalMemoryHandleTypeBit external_memory_handle_type;
        const VkFormat                     format;
        const VkImageType                  image_type;
        const VkImageTiling                tiling;
        const VkImageUsageFlags            usage_flags;

        ImageFormatPropertiesQuery           (const ImageFormatPropertiesQuery& in_query) = default;
        ImageFormatPropertiesQuery& operator=(const ImageFormatPropertiesQuery& in_query) = delete;
    } ImageFormatPropertiesQuery;

    /** Describes an image memory barrier. */
    typedef struct ImageBarrier
    {
        VkAccessFlagsVariable(dst_access_mask);
        VkAccessFlagsVariable(src_access_mask);

        bool                    by_region;
        uint32_t                dst_queue_family_index;
        VkImage                 image;
        VkImageMemoryBarrier    image_barrier_vk;
        Anvil::Image*           image_ptr;
        VkImageLayout           new_layout;
        VkImageLayout           old_layout;
        uint32_t                src_queue_family_index;
        VkImageSubresourceRange subresource_range;

        /** Constructor.
         *
         *  Note that @param in_image_ptr is retained by this function.
         *
         *  @param in_source_access_mask      Source access mask to use for the barrier.
         *  @param in_destination_access_mask Destination access mask to use for the barrier.
         *  @param in_by_region_barrier       true if this is a by-region barrier.
         *  @param in_old_layout              Old layout of @param in_image_ptr to use for the barrier.
         *  @param in_new_layout              New layout of @param in_image_ptr to use for the barrier.
         *  @param in_src_queue_family_index  Source queue family index to use for the barrier.
         *  @param in_dst_queue_family_index  Destination queue family index to use for the barrier.
         *  @param in_image_ptr               Image instance the barrier refers to. May be nullptr, in which case
         *                                    "image" and "image_ptr" fields will be set to nullptr.
         *                                    The instance will be retained by this function.
         *  @param in_image_subresource_range Subresource range to use for the barrier.
         *
         **/
        ImageBarrier(VkAccessFlags           in_source_access_mask,
                     VkAccessFlags           in_destination_access_mask,
                     bool                    in_by_region_barrier,
                     VkImageLayout           in_old_layout,
                     VkImageLayout           in_new_layout,
                     uint32_t                in_src_queue_family_index,
                     uint32_t                in_dst_queue_family_index,
                     Anvil::Image*           in_image_ptr,
                     VkImageSubresourceRange in_image_subresource_range);

        /** Destructor.
         *
         *  Releases the encapsulated Image instance.
         **/
       virtual ~ImageBarrier();

       /** Copy constructor.
         *
         *  Retains the Image instance stored in the input barrier.
         *
         *  @param in Barrier instance to copy data from.
         **/
       ImageBarrier(const ImageBarrier& in);

       /** Returns a Vulkan memory barrier descriptor, whose configuration corresponds to
         *  to the configuration of this descriptor.
         **/
       virtual VkImageMemoryBarrier get_barrier_vk() const
       {
           return image_barrier_vk;
       }

       /** Returns a pointer to the Vulkan descriptor, whose configuration corresponds to
         *  the configuration of this descriptor.
         *
         *  The returned pointer remains valid for the duration of the Barrier descriptor's
         *  life-time.
         **/
       const VkImageMemoryBarrier* get_barrier_vk_ptr() const
       {
           return &image_barrier_vk;
       }

    private:
        ImageBarrier& operator=(const ImageBarrier&);
    } ImageBarrier;

    typedef struct ExternalMemoryHandleImportInfo
    {
        ExternalHandleType handle;

        #if defined(_WIN32)
            std::wstring name;
        #endif

        ExternalMemoryHandleImportInfo()
        {
            #if defined(_WIN32)
                handle = nullptr;
            #else
                handle = -1;
            #endif
        }
    } ExternalMemoryHandleImportInfo;

    /* Holds 16-bit storage features */
    typedef struct KHR16BitStorageFeatures
    {
        bool is_input_output_storage_supported;
        bool is_push_constant_16_bit_storage_supported;
        bool is_storage_buffer_16_bit_access_supported;
        bool is_uniform_and_storage_buffer_16_bit_access_supported;

        KHR16BitStorageFeatures();
        KHR16BitStorageFeatures(const VkPhysicalDevice16BitStorageFeaturesKHR& in_features);

        VkPhysicalDevice16BitStorageFeaturesKHR get_vk_physical_device_16_bit_storage_features() const;

        bool operator==(const KHR16BitStorageFeatures& in_features) const;
    } KHR16BitStorageFeatures;

    typedef struct KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties
    {
        uint8_t  device_luid[VK_LUID_SIZE];
        bool     device_luid_valid;

        uint8_t  device_uuid[VK_UUID_SIZE];
        uint8_t  driver_uuid[VK_UUID_SIZE];

        uint32_t device_node_mask;

        KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties();
        KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties(const VkPhysicalDeviceIDPropertiesKHR& in_properties);
    } KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties;

    typedef struct KHRMaintenance3Properties
    {
        VkDeviceSize max_memory_allocation_size;
        uint32_t     max_per_set_descriptors;

        KHRMaintenance3Properties();
        KHRMaintenance3Properties(const VkPhysicalDeviceMaintenance3PropertiesKHR& in_props);

        bool operator==(const KHRMaintenance3Properties&) const;

    } KHRMaintenance3Properties;

    /** Holds properties of a single Vulkan Layer. */
    typedef struct Layer
    {
        std::string              description;
        std::vector<std::string> extensions;
        uint32_t                 implementation_version;
        std::string              name;
        uint32_t                 spec_version;

        /** Dummy constructor.
         *
         *  @param in_layer_name Name to use for the layer.
         **/
        Layer(const std::string& in_layer_name);

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param in_layer_props Vulkan structure to use for initialization.
         **/
        Layer(const VkLayerProperties& in_layer_props);

        /** Returns true if @param in_layer_name matches the layer name described by the instance. */
        bool operator==(const std::string& in_layer_name) const;
    } Layer;

    typedef std::vector<Layer> Layers;

    /** Describes a Vulkan memory barrier. */
    typedef struct MemoryBarrier
    {
        VkAccessFlagsVariable(destination_access_mask);
        VkAccessFlagsVariable(source_access_mask);

        VkMemoryBarrier memory_barrier_vk;

        /** Constructor.
         *
         *  @param in_source_access_mask      Source access mask of the Vulkan memory barrier.
         *  @param in_destination_access_mask Destination access mask of the Vulkan memory barrier.
         *
         **/
        explicit MemoryBarrier(VkAccessFlags in_destination_access_mask,
                               VkAccessFlags in_source_access_mask);

        /** Destructor. */
        virtual ~MemoryBarrier()
        {
            /* Stub */
        }

        /** Returns a Vulkan memory barrier descriptor, whose configuration corresponds to
         *  to the configuration of this descriptor.
         **/
        virtual VkMemoryBarrier get_barrier_vk() const
        {
            return memory_barrier_vk;
        }

        /** Returns a pointer to the Vulkan descriptor, whose configuration corresponds to
         *  the configuration of this descriptor.
         *
         *  The returned pointer remains valid for the duration of the Barrier descriptor's
         *  life-time.
         **/
        virtual const VkMemoryBarrier* get_barrier_vk_ptr() const
        {
            return &memory_barrier_vk;
        }
    } MemoryBarrier;

    /** Holds properties of a single Vulkan Memory Heap. */
    typedef struct MemoryHeap
    {
        VkMemoryHeapFlagsVariable(flags);

        VkDeviceSize size;

        /** Stub constructor */
        MemoryHeap();
    } MemoryHeap;

    typedef std::vector<MemoryHeap> MemoryHeaps;

    extern bool operator==(const MemoryHeap& in1,
                           const MemoryHeap& in2);

    /** Holds properties of a single Vulkan Memory Type. */
    typedef struct MemoryType
    {
        MemoryFeatureFlags features;
        MemoryHeap*        heap_ptr;

        VkMemoryPropertyFlagsVariable(flags);

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param in_type             Vulkan structure to use for initialization.
         *  @param in_memory_props_ptr Used to initialize the MemoryHeap pointer member. Must not be nullptr.
         **/
        explicit MemoryType(const VkMemoryType&      in_type,
                            struct MemoryProperties* in_memory_props_ptr);
    } MemoryType;

    typedef std::vector<MemoryType> MemoryTypes;

    extern bool operator==(const MemoryType& in1,
                           const MemoryType& in2);

    /** Holds information about available memory heaps & types for a specific physical device. */
    typedef struct MemoryProperties
    {
        MemoryHeap* heaps;
        uint32_t    n_heaps;
        MemoryTypes types;

        MemoryProperties();

        /** Destructor */
        ~MemoryProperties();

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param in_mem_properties Vulkan structure to use for initialization.
         **/
        void init(const VkPhysicalDeviceMemoryProperties& in_mem_properties);

    private:
        MemoryProperties           (const MemoryProperties&);
        MemoryProperties& operator=(const MemoryProperties&);
    } MemoryProperties;

    extern bool operator==(const MemoryProperties& in1,
                           const MemoryProperties& in2);

    /** Defines data for a single image mip-map.
     *
     *  Use one of the static create_() functions to set up structure fields according to the target
     *  image type.
     **/
    typedef struct MipmapRawData
    {
        /* Image aspect the mip-map data is specified for. */
        VkImageAspectFlagBits aspect;

        /* Start layer index */
        uint32_t n_layer;

        /* Number of layers to update */
        uint32_t n_layers;

        /* Number of 3D texture slices to update. For non-3D texture types, this field
         * should be set to 1. */
        uint32_t n_slices;


        /* Index of the mip-map to update. */
        uint32_t n_mipmap;


        /* Pointer to a buffer holding raw data representation. The data structure is characterized by
         * data_size, row_size and slice_size fields.
         *
         * It is assumed the data under the pointer is tightly packed, and stored in column->row->slice->layer
         * order.
         */
        std::shared_ptr<unsigned char>               linear_tightly_packed_data_uchar_ptr;
        const unsigned char*                         linear_tightly_packed_data_uchar_raw_ptr;
        std::shared_ptr<std::vector<unsigned char> > linear_tightly_packed_data_uchar_vec_ptr;


        /* Total number of bytes available for reading under linear_tightly_packed_data_ptr */
        uint32_t data_size;

        /* Number of bytes each row takes */
        uint32_t row_size;


        /** Creates a MipmapRawData instance which can be used to upload data to 1D Image instances:
         *
         *  @param in_aspect                                Image aspect to modify.
         *  @param in_n_mipmap                              Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr        Pointer to raw mip-map data.
         *  @param in_linear_tightly_packed_data_vector_ptr Vector holding raw mip-map data.
         *  @param in_row_size                              Number of bytes each texture row takes.
         *
         *  NOTE: Mipmap contents is NOT cached at call time. This implies raw pointers are ASSUMED to
         *        be valid at baking time.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_1D_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                             uint32_t                                     in_n_mipmap,
                                                             std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_1D_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                             uint32_t                                     in_n_mipmap,
                                                             const unsigned char*                         in_linear_tightly_packed_data_vector_ptr,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_1D_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                             uint32_t                                     in_n_mipmap,
                                                             std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_row_size);

        /** Creates a MipmapRawData instance which can be used to upload data to 1D Array Image instances:
         *
         *  @param in_aspect                                Image aspect to modify.
         *  @param in_n_layer                               Index of a texture layer the mip-map data should be uploaded to.
         *  @param in_n_layers                              Number of texture layers to be updated.
         *  @param in_n_mipmap                              Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr        Pointer to raw mip-map data.
         *  @param in_linear_tightly_packed_data_vector_ptr Vector holding raw mip-map data.
         *  @param in_row_size                              Number of bytes each texture row takes.
         *  @param in_data_size                             Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_1D_array_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_layers,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_row_size,
                                                                   uint32_t                                     in_data_size);
        static MipmapRawData create_1D_array_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_layers,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_row_size,
                                                                   uint32_t                                     in_data_size);
        static MipmapRawData create_1D_array_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_layers,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_row_size,
                                                                   uint32_t                                     in_data_size);

        /** Creates a MipmapRawData instance which can be used to upload data to 2D Image instances:
         *
         *  @param in_aspect                                Image aspect to modify.
         *  @param in_n_mipmap                              Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr        Pointer to raw mip-map data.
         *  @param in_linear_tightly_packed_data_vector_ptr Vector holding raw mip-map data.
         *  @param in_data_size                             Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *  @param in_row_size                              Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_2D_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                             uint32_t                                     in_n_mipmap,
                                                             std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_data_size,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_2D_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                             uint32_t                                     in_n_mipmap,
                                                             const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_data_size,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_2D_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                             uint32_t                                     in_n_mipmap,
                                                             std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_data_size,
                                                             uint32_t                                     in_row_size);

        /** Creates a MipmapRawData instance which can be used to upload data to 2D Array Image instances:
         *
         *  @param in_aspect                                Image aspect to modify.
         *  @param in_n_layer                               Index of a texture layer the mip-map data should be uploaded to.
         *  @param in_n_layers                              Number of texture layers to be updated.
         *  @param in_n_mipmap                              Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr        Pointer to raw mip-map data.
         *  @param in_linear_tightly_packed_data_vector_ptr Vector holding raw mip-map data.
         *  @param in_data_size                             Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *  @param in_row_size                              Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_2D_array_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_layers,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_data_size,
                                                                   uint32_t                                     in_row_size);
        static MipmapRawData create_2D_array_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_layers,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_data_size,
                                                                   uint32_t                                     in_row_size);
        static MipmapRawData create_2D_array_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_layers,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_data_size,
                                                                   uint32_t                                     in_row_size);

        /** Creates a MipmapRawData instnce which can be used to upload data to 3D Image instances:
         *
         *  @param in_aspect                                Image aspect to modify.
         *  @param in_n_layer                               Index of a texture layer the mip-map data should be uploaded to.
         *  @param in_n_slices                              Number of texture slices to be updated.
         *  @param in_n_mipmap                              Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr        Pointer to raw mip-map data.
         *  @param in_linear_tightly_packed_data_vector_ptr Vector holding raw mip-map data.
         *  @param in_slice_data_size                       Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr for a single slice.
         *  @param in_row_size                              Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_3D_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                             uint32_t                                     in_n_layer,
                                                             uint32_t                                     in_n_layer_slices,
                                                             uint32_t                                     in_n_mipmap,
                                                             std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_slice_data_size,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_3D_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                             uint32_t                                     in_n_layer,
                                                             uint32_t                                     in_n_layer_slices,
                                                             uint32_t                                     in_n_mipmap,
                                                             const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_slice_data_size,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_3D_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                             uint32_t                                     in_n_layer,
                                                             uint32_t                                     in_n_layer_slices,
                                                             uint32_t                                     in_n_mipmap,
                                                             std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_slice_data_size,
                                                             uint32_t                                     in_row_size);

        /** Creates a MipmapRawData instance which can be used to upload data to Cube Map Image instances:
         *
         *  @param in_aspect                                Image aspect to modify.
         *  @param in_n_layer                               Index of a texture layer the mip-map data should be uploaded to.
         *                                                  Valid values and corresponding cube map faces: 0: -X, 1: -Y, 2: -Z, 3: +X, 4: +Y, 5: +Z
         *  @param in_n_mipmap                              Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr        Pointer to raw mip-map data.
         *  @param in_linear_tightly_packed_data_vector_ptr Vector holding raw mip-map data.
         *  @param in_data_size                             Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *  @param in_row_size                              Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_cube_map_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_data_size,
                                                                   uint32_t                                     in_row_size);
        static MipmapRawData create_cube_map_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_data_size,
                                                                   uint32_t                                     in_row_size);
        static MipmapRawData create_cube_map_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_data_size,
                                                                   uint32_t                                     in_row_size);

        /** Creates a MipmapRawData instance which can be used to upload data to Cube Map Array Image instances:
         *
         *  @param in_aspect                                Image aspect to modify.
         *  @param in_n_layer                               Index of a texture layer the mip-map data should be uploaded to.
         *                                                  Cube map faces, as selected for layer at index (n_layer % 6), are:
         *                                                  0: -X, 1: -Y, 2: -Z, 3: +X, 4: +Y, 5: +Z
         *  @param in_n_layers                              Number of texture layers to update.
         *  @param in_n_mipmap                              Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr        Pointer to raw mip-map data.
         *  @param in_linear_tightly_packed_data_vector_ptr Vector holding raw mip-map data.
         *  @param in_data_size                             Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *  @param in_row_size                              Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_cube_map_array_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                                         uint32_t                                     in_n_layer,
                                                                         uint32_t                                     in_n_layers,
                                                                         uint32_t                                     in_n_mipmap,
                                                                         std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                                         uint32_t                                     in_data_size,
                                                                         uint32_t                                     in_row_size);
        static MipmapRawData create_cube_map_array_from_uchar_ptr       (VkImageAspectFlagBits                        in_aspect,
                                                                         uint32_t                                     in_n_layer,
                                                                         uint32_t                                     in_n_layers,
                                                                         uint32_t                                     in_n_mipmap,
                                                                         const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                                         uint32_t                                     in_data_size,
                                                                         uint32_t                                     in_row_size);
        static MipmapRawData create_cube_map_array_from_uchar_vector_ptr(VkImageAspectFlagBits                        in_aspect,
                                                                         uint32_t                                     in_n_layer,
                                                                         uint32_t                                     in_n_layers,
                                                                         uint32_t                                     in_n_mipmap,
                                                                         std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                         uint32_t                                     in_data_size,
                                                                         uint32_t                                     in_row_size);

    private:
        static MipmapRawData create_1D      (VkImageAspectFlagBits in_aspect,
                                             uint32_t              in_n_mipmap,
                                             uint32_t              in_row_size);
        static MipmapRawData create_1D_array(VkImageAspectFlagBits in_aspect,
                                             uint32_t              in_n_layer,
                                             uint32_t              in_n_layers,
                                             uint32_t              in_n_mipmap,
                                             uint32_t              in_row_size,
                                             uint32_t              in_data_size);
        static MipmapRawData create_2D      (VkImageAspectFlagBits in_aspect,
                                             uint32_t              in_n_mipmap,
                                             uint32_t              in_data_size,
                                             uint32_t              in_row_size);
        static MipmapRawData create_2D_array(VkImageAspectFlagBits in_aspect,
                                             uint32_t              in_n_layer,
                                             uint32_t              in_n_layers,
                                             uint32_t              in_n_mipmap,
                                             uint32_t              in_data_size,
                                             uint32_t              in_row_size);
        static MipmapRawData create_3D      (VkImageAspectFlagBits in_aspect,
                                             uint32_t              in_n_layer,
                                             uint32_t              in_n_slices,
                                             uint32_t              in_n_mipmap,
                                             uint32_t              in_data_size,
                                             uint32_t              in_row_size);
    } MipmapRawData;

    /* Dummy delete functor */
    template<class Type>
    struct NullDeleter
    {
        void operator()(Type* in_unused_ptr)
        {
            in_unused_ptr;
        }
    };

    /* Used internally by Buffer and Image to track page occupancy status */
    typedef union
    {
        uint32_t raw;

        struct
        {
            uint8_t page_bit_0  : 1;
            uint8_t page_bit_1  : 1;
            uint8_t page_bit_2  : 1;
            uint8_t page_bit_3  : 1;
            uint8_t page_bit_4  : 1;
            uint8_t page_bit_5  : 1;
            uint8_t page_bit_6  : 1;
            uint8_t page_bit_7  : 1;
            uint8_t page_bit_8  : 1;
            uint8_t page_bit_9  : 1;
            uint8_t page_bit_10 : 1;
            uint8_t page_bit_11 : 1;
            uint8_t page_bit_12 : 1;
            uint8_t page_bit_13 : 1;
            uint8_t page_bit_14 : 1;
            uint8_t page_bit_15 : 1;
            uint8_t page_bit_16 : 1;
            uint8_t page_bit_17 : 1;
            uint8_t page_bit_18 : 1;
            uint8_t page_bit_19 : 1;
            uint8_t page_bit_20 : 1;
            uint8_t page_bit_21 : 1;
            uint8_t page_bit_22 : 1;
            uint8_t page_bit_23 : 1;
            uint8_t page_bit_24 : 1;
            uint8_t page_bit_25 : 1;
            uint8_t page_bit_26 : 1;
            uint8_t page_bit_27 : 1;
            uint8_t page_bit_28 : 1;
            uint8_t page_bit_29 : 1;
            uint8_t page_bit_30 : 1;
            uint8_t page_bit_31 : 1;
        } page_bits;
    } PageOccupancyStatus;

    typedef struct PhysicalDeviceFeaturesCoreVK10
    {
        bool alpha_to_one;
        bool depth_bias_clamp;
        bool depth_bounds;
        bool depth_clamp;
        bool draw_indirect_first_instance;
        bool dual_src_blend;
        bool fill_mode_non_solid;
        bool fragment_stores_and_atomics;
        bool full_draw_index_uint32;
        bool geometry_shader;
        bool image_cube_array;
        bool independent_blend;
        bool inherited_queries;
        bool large_points;
        bool logic_op;
        bool multi_draw_indirect;
        bool multi_viewport;
        bool occlusion_query_precise;
        bool pipeline_statistics_query;
        bool robust_buffer_access;
        bool sampler_anisotropy;
        bool sample_rate_shading;
        bool shader_clip_distance;
        bool shader_cull_distance;
        bool shader_float64;
        bool shader_image_gather_extended;
        bool shader_int16;
        bool shader_int64;
        bool shader_resource_residency;
        bool shader_resource_min_lod;
        bool shader_sampled_image_array_dynamic_indexing;
        bool shader_storage_buffer_array_dynamic_indexing;
        bool shader_storage_image_array_dynamic_indexing;
        bool shader_storage_image_extended_formats;
        bool shader_storage_image_multisample;
        bool shader_storage_image_read_without_format;
        bool shader_storage_image_write_without_format;
        bool shader_tessellation_and_geometry_point_size;
        bool shader_uniform_buffer_array_dynamic_indexing;
        bool sparse_binding;
        bool sparse_residency_2_samples;
        bool sparse_residency_4_samples;
        bool sparse_residency_8_samples;
        bool sparse_residency_16_samples;
        bool sparse_residency_aliased;
        bool sparse_residency_buffer;
        bool sparse_residency_image_2D;
        bool sparse_residency_image_3D;
        bool tessellation_shader;
        bool texture_compression_ASTC_LDR;
        bool texture_compression_BC;
        bool texture_compression_ETC2;
        bool variable_multisample_rate;
        bool vertex_pipeline_stores_and_atomics;
        bool wide_lines;

        VkPhysicalDeviceFeatures get_vk_physical_device_features() const;
        bool                     operator==                     (const PhysicalDeviceFeaturesCoreVK10& in_data) const;

        PhysicalDeviceFeaturesCoreVK10();
        PhysicalDeviceFeaturesCoreVK10(const VkPhysicalDeviceFeatures& in_physical_device_features);

    } PhysicalDeviceFeaturesCoreVK10;

    typedef struct PhysicalDeviceFeatures
    {
        const PhysicalDeviceFeaturesCoreVK10* core_vk1_0_features_ptr;
        const EXTDescriptorIndexingFeatures*  ext_descriptor_indexing_features_ptr;
        const KHR16BitStorageFeatures*        khr_16bit_storage_features_ptr;

        PhysicalDeviceFeatures();
        PhysicalDeviceFeatures(const PhysicalDeviceFeaturesCoreVK10* in_core_vk1_0_features_ptr,
                               const EXTDescriptorIndexingFeatures*  in_ext_descriptor_indexing_features_ptr,
                               const KHR16BitStorageFeatures*        in_khr_16_bit_storage_features_ptr);

        bool operator==(const PhysicalDeviceFeatures& in_physical_device_features) const;
    } PhysicalDeviceFeatures;

    typedef struct PhysicalDeviceLimits
    {
        VkDeviceSize          buffer_image_granularity;
        uint32_t              discrete_queue_priorities;
        VkSampleCountFlags    framebuffer_color_sample_counts;
        VkSampleCountFlags    framebuffer_depth_sample_counts;
        VkSampleCountFlags    framebuffer_no_attachments_sample_counts;
        VkSampleCountFlags    framebuffer_stencil_sample_counts;
        float                 line_width_granularity;
        float                 line_width_range[2];
        uint32_t              max_bound_descriptor_sets;
        uint32_t              max_clip_distances;
        uint32_t              max_color_attachments;
        uint32_t              max_combined_clip_and_cull_distances;
        uint32_t              max_compute_shared_memory_size;
        uint32_t              max_compute_work_group_count[3];
        uint32_t              max_compute_work_group_invocations;
        uint32_t              max_compute_work_group_size[3];
        uint32_t              max_cull_distances;
        uint32_t              max_descriptor_set_input_attachments;
        uint32_t              max_descriptor_set_sampled_images;
        uint32_t              max_descriptor_set_samplers;
        uint32_t              max_descriptor_set_storage_buffers;
        uint32_t              max_descriptor_set_storage_buffers_dynamic;
        uint32_t              max_descriptor_set_storage_images;
        uint32_t              max_descriptor_set_uniform_buffers;
        uint32_t              max_descriptor_set_uniform_buffers_dynamic;
        uint32_t              max_draw_indexed_index_value;
        uint32_t              max_draw_indirect_count;
        uint32_t              max_fragment_combined_output_resources;
        uint32_t              max_fragment_dual_src_attachments;
        uint32_t              max_fragment_input_components;
        uint32_t              max_fragment_output_attachments;
        uint32_t              max_framebuffer_height;
        uint32_t              max_framebuffer_layers;
        uint32_t              max_framebuffer_width;
        uint32_t              max_geometry_input_components;
        uint32_t              max_geometry_output_components;
        uint32_t              max_geometry_output_vertices;
        uint32_t              max_geometry_shader_invocations;
        uint32_t              max_geometry_total_output_components;
        uint32_t              max_image_array_layers;
        uint32_t              max_image_dimension_1D;
        uint32_t              max_image_dimension_2D;
        uint32_t              max_image_dimension_3D;
        uint32_t              max_image_dimension_cube;
        float                 max_interpolation_offset;
        uint32_t              max_memory_allocation_count;
        uint32_t              max_per_stage_descriptor_input_attachments;
        uint32_t              max_per_stage_descriptor_sampled_images;
        uint32_t              max_per_stage_descriptor_samplers;
        uint32_t              max_per_stage_descriptor_storage_buffers;
        uint32_t              max_per_stage_descriptor_storage_images;
        uint32_t              max_per_stage_descriptor_uniform_buffers;
        uint32_t              max_per_stage_resources;
        uint32_t              max_push_constants_size;
        uint32_t              max_sample_mask_words;
        uint32_t              max_sampler_allocation_count;
        float                 max_sampler_anisotropy;
        float                 max_sampler_lod_bias;
        uint32_t              max_storage_buffer_range;
        uint32_t              max_viewport_dimensions[2];
        uint32_t              max_viewports;
        uint32_t              max_tessellation_control_per_patch_output_components;
        uint32_t              max_tessellation_control_per_vertex_input_components;
        uint32_t              max_tessellation_control_per_vertex_output_components;
        uint32_t              max_tessellation_control_total_output_components;
        uint32_t              max_tessellation_evaluation_input_components;
        uint32_t              max_tessellation_evaluation_output_components;
        uint32_t              max_tessellation_generation_level;
        uint32_t              max_tessellation_patch_size;
        uint32_t              max_texel_buffer_elements;
        uint32_t              max_texel_gather_offset;
        uint32_t              max_texel_offset;
        uint32_t              max_uniform_buffer_range;
        uint32_t              max_vertex_input_attributes;
        uint32_t              max_vertex_input_attribute_offset;
        uint32_t              max_vertex_input_bindings;
        uint32_t              max_vertex_input_binding_stride;
        uint32_t              max_vertex_output_components;
        float                 min_interpolation_offset;
        size_t                min_memory_map_alignment;
        VkDeviceSize          min_storage_buffer_offset_alignment;
        VkDeviceSize          min_texel_buffer_offset_alignment;
        int32_t               min_texel_gather_offset;
        int32_t               min_texel_offset;
        VkDeviceSize          min_uniform_buffer_offset_alignment;
        uint32_t              mipmap_precision_bits;
        VkDeviceSize          non_coherent_atom_size;
        VkDeviceSize          optimal_buffer_copy_offset_alignment;
        VkDeviceSize          optimal_buffer_copy_row_pitch_alignment;
        float                 point_size_granularity;
        float                 point_size_range[2];
        VkSampleCountFlags    sampled_image_color_sample_counts;
        VkSampleCountFlags    sampled_image_depth_sample_counts;
        VkSampleCountFlags    sampled_image_integer_sample_counts;
        VkSampleCountFlags    sampled_image_stencil_sample_counts;
        VkDeviceSize          sparse_address_space_size;
        bool                  standard_sample_locations;
        VkSampleCountFlags    storage_image_sample_counts;
        bool                  strict_lines;
        uint32_t              sub_pixel_interpolation_offset_bits;
        uint32_t              sub_pixel_precision_bits;
        uint32_t              sub_texel_precision_bits;
        bool                  timestamp_compute_and_graphics;
        float                 timestamp_period;
        float                 viewport_bounds_range[2];
        uint32_t              viewport_sub_pixel_bits;

        PhysicalDeviceLimits();
        PhysicalDeviceLimits(const VkPhysicalDeviceLimits& in_device_limits);

        bool operator==(const PhysicalDeviceLimits& in_device_limits) const;
    } PhysicalDeviceLimits;

    typedef struct PhysicalDeviceSparseProperties
    {
        bool residency_standard_2D_block_shape;
        bool residency_standard_2D_multisample_block_shape;
        bool residency_standard_3D_block_shape;
        bool residency_aligned_mip_size;
        bool residency_non_resident_strict;

        PhysicalDeviceSparseProperties();
        PhysicalDeviceSparseProperties(const VkPhysicalDeviceSparseProperties& in_sparse_props);

        bool operator==(const PhysicalDeviceSparseProperties& in_props) const;
    } PhysicalDeviceSparseProperties;

    typedef struct PhysicalDevicePropertiesCoreVK10
    {
        uint32_t             api_version;
        uint32_t             device_id;
        char                 device_name        [VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
        VkPhysicalDeviceType device_type;
        uint32_t             driver_version;
        uint8_t              pipeline_cache_uuid[VK_UUID_SIZE];
        uint32_t             vendor_id;

        PhysicalDeviceLimits           limits;
        PhysicalDeviceSparseProperties sparse_properties;

        bool operator==(const PhysicalDevicePropertiesCoreVK10& in_props) const;

        PhysicalDevicePropertiesCoreVK10();
        PhysicalDevicePropertiesCoreVK10(const VkPhysicalDeviceProperties& in_physical_device_properties);
    } PhysicalDevicePropertiesCoreVK10;

    typedef struct PhysicalDeviceProperties
    {
        const AMDShaderCoreProperties*                                 amd_shader_core_properties_ptr;
        const PhysicalDevicePropertiesCoreVK10*                        core_vk1_0_properties_ptr;
        const EXTDescriptorIndexingProperties*                         ext_descriptor_indexing_properties_ptr;
        const EXTVertexAttributeDivisorProperties*                     ext_vertex_attribute_divisor_properties_ptr;
        const KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties* khr_external_memory_capabilities_physical_device_id_properties_ptr;
        const KHRMaintenance3Properties*                               khr_maintenance3_properties_ptr;

        PhysicalDeviceProperties();
        PhysicalDeviceProperties(const AMDShaderCoreProperties*                                 in_amd_shader_core_properties_ptr,
                                 const PhysicalDevicePropertiesCoreVK10*                        in_core_vk1_0_properties_ptr,
                                 const EXTDescriptorIndexingProperties*                         in_ext_descriptor_indexing_properties_ptr,
                                 const EXTVertexAttributeDivisorProperties*                     in_ext_vertex_attribute_divisor_properties_ptr,
                                 const KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties* in_khr_external_memory_caps_physical_device_id_props_ptr,
                                 const KHRMaintenance3Properties*                               in_khr_maintenance3_properties_ptr);

        bool operator==(const PhysicalDeviceProperties& in_props) const;
    } PhysicalDeviceProperties;

    /* A single push constant range descriptor */
    typedef struct PushConstantRange
    {
        uint32_t              offset;
        uint32_t              size;
        VkShaderStageFlagBits stages;

        /** Constructor
         *
         *  @param in_offset Start offset for the range.
         *  @param in_size   Size of the range.
         *  @param in_stages Valid pipeline stages for the range.
         */
        PushConstantRange(uint32_t           in_offset,
                          uint32_t           in_size,
                          VkShaderStageFlags in_stages);

        /** Comparison operator. Used internally. */
        bool operator==(const PushConstantRange& in) const;
    } PushConstantRange;

    typedef std::vector<PushConstantRange> PushConstantRanges;

    /** Holds information about a single Vulkan Queue Family. */
    typedef struct QueueFamilyInfo
    {
        VkQueueFlagsVariable(flags);

        VkExtent3D min_image_transfer_granularity;
        uint32_t   n_queues;
        uint32_t   n_timestamp_bits;

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param in_props Vulkan structure to use for initialization.
         **/
        explicit QueueFamilyInfo(const VkQueueFamilyProperties& in_props);
    } QueueFamilyInfo;

    typedef std::vector<QueueFamilyInfo> QueueFamilyInfoItems;

    extern bool operator==(const QueueFamilyInfo& in1,
                           const QueueFamilyInfo& in2);

    /** Describes a physical device group */
    typedef struct PhysicalDeviceGroup
    {
        std::vector<const Anvil::PhysicalDevice*> physical_device_ptrs;
        bool                                      supports_subset_allocations;

        explicit PhysicalDeviceGroup();
    } PhysicalDeviceGroup;

    typedef struct SemaphoreProperties
    {
        ExternalSemaphoreProperties external_semaphore_properties;

        /* Dummy constructor */
        SemaphoreProperties();

        SemaphoreProperties(const ExternalSemaphoreProperties& in_external_semaphore_properties);
    } SemaphoreProperties;

    typedef struct SemaphorePropertiesQuery
    {
        const Anvil::ExternalSemaphoreHandleTypeBit external_semaphore_handle_type;

        explicit SemaphorePropertiesQuery(const Anvil::ExternalSemaphoreHandleTypeBit& in_external_semaphore_handle_type)
            :external_semaphore_handle_type(in_external_semaphore_handle_type)
        {
            /* Stub */
        }

        SemaphorePropertiesQuery           (const SemaphorePropertiesQuery&) = default;
        SemaphorePropertiesQuery& operator=(const SemaphorePropertiesQuery&) = delete;
    } SemaphorePropertiesQuery;

    /** Holds all information related to a specific shader module stage entry-point. */
    typedef struct ShaderModuleStageEntryPoint
    {
        std::string           name;
        ShaderModuleUniquePtr shader_module_owned_ptr;
        Anvil::ShaderModule*  shader_module_ptr;
        Anvil::ShaderStage    stage;

        /** Dummy constructor */
        ShaderModuleStageEntryPoint();

        /** Copy constructor. */
        ShaderModuleStageEntryPoint(const ShaderModuleStageEntryPoint& in);

        /** Constructor.
         *
         *  @param in_name              Entry-point name. Must not be nullptr.
         *  @param in_shader_module_ptr ShaderModule instance to use.
         *  @param in_stage             Shader stage the entry-point implements.
         */
        ShaderModuleStageEntryPoint(const std::string&    in_name,
                                    ShaderModule*         in_shader_module_ptr,
                                    ShaderStage           in_stage);
        ShaderModuleStageEntryPoint(const std::string&    in_name,
                                    ShaderModuleUniquePtr in_shader_module_ptr,
                                    ShaderStage           in_stage);

        /** Destructor. */
        ~ShaderModuleStageEntryPoint();

        ShaderModuleStageEntryPoint& operator=(const ShaderModuleStageEntryPoint&);
    } ShaderModuleStageEntryPoint;

    /* Describes sparse properties for an image format */
    typedef struct SparseImageAspectProperties
    {
        VkImageAspectFlagsVariable      (aspect_mask);
        VkSparseImageFormatFlagsVariable(flags);

        VkExtent3D   granularity;
        uint32_t     mip_tail_first_lod;
        VkDeviceSize mip_tail_offset;
        VkDeviceSize mip_tail_size;
        VkDeviceSize mip_tail_stride;

        SparseImageAspectProperties();
        SparseImageAspectProperties(const VkSparseImageMemoryRequirements& in_req);
    } SparseImageAspectProperties;

    typedef struct SpecializationConstant
    {
        uint32_t constant_id;
        uint32_t n_bytes;
        uint32_t start_offset;

        /** Dummy constructor. Should only be used by STL containers. */
        SpecializationConstant();

        /** Constructor.
         *
         *  @param in_constant_id  Specialization constant ID.
         *  @param in_n_bytes      Number of bytes consumed by the constant.
         *  @param in_start_offset Start offset, at which the constant data starts.
         */
        SpecializationConstant(uint32_t in_constant_id,
                               uint32_t in_n_bytes,
                               uint32_t in_start_offset);
    } SpecializationConstant;

    typedef std::vector<SpecializationConstant> SpecializationConstants;

    typedef enum class SubmissionType
    {
        SGPU
    } SubmissionType;

    typedef struct SubmitInfo
    {
        /** Creates a submission info which, when submitted, waits on the semaphores specified by the user,
         *  executes user-defined command buffers, and then signals the semaphores passed by arguments.
         *  If a non-nullptr fence is specified, the function will also wait on the call to finish GPU-side
         *  before returning.
         *
         *  It is valid to specify 0 command buffers or signal/wait semaphores, in which case
         *  these steps will be skipped.
         *
         *  If @param in_should_block is true and @param in_opt_fence_ptr is nullptr, the function will create
         *  a new fence, wait on it, and then release it prior to leaving. This may come at a performance cost.
         *
         *  NOTE: By default, the following values are associated with a new SubmitInfo instance:
         *
         *  - D3D12 fence submit info: none
         *
         *  To adjust these settings, please use corresponding set_..() functions, prior to passing the structure over to Queue::submit().
         *
         *
         *  @param in_n_command_buffers                   Number of command buffers under @param in_opt_cmd_buffer_ptrs
         *                                                which should be executed. May be 0.
         *  @param in_opt_cmd_buffer_ptrs_ptr             Ptr to an array of command buffers to execute. Can be nullptr if
         *                                                @param n_command_buffers is 0.
         *  @param in_n_semaphores_to_signal              Number of semaphores to signal after command buffers finish
         *                                                executing. May be 0.
         *  @param in_opt_semaphore_to_signal_ptrs_ptr    Ptr to an array of semaphores to signal after execution. May be nullptr if
         *                                                @param n_semaphores_to_signal is 0.
         *  @param in_n_semaphores_to_wait_on             Number of semaphores to wait on before executing command buffers.
         *                                                May be 0.
         *  @param in_opt_semaphore_to_wait_on_ptrs_ptr   Ptr to an array of semaphores to wait on prior to execution. May be nullptr
         *                                                if @param in_n_semaphores_to_wait_on is 0.
         *  @param in_opt_dst_stage_masks_to_wait_on_ptrs Array of size @param in_n_semaphores_to_wait_on, specifying stages
         *                                                at which the wait ops should be performed. May be nullptr if
         *                                                @param n_semaphores_to_wait_on is 0.
         *  @param in_should_block                        true if the function should wait for the scheduled commands to
         *                                                finish executing, false otherwise.
         *  @param in_opt_fence_ptr                       Fence to use when submitting the comamnd buffers to the queue.
         *                                                The fence will be waited on if @param in_should_block is true.
         *                                                If @param in_should_block is false, the fence will be passed at
         *                                                submit call time, but will not be waited on.
         *
         *  @param in_command_buffer_submissions          TODO
         *  @param in_semaphores_to_signal_ptr            TODO
         *  @param in_semaphores_to_wait_on_ptr           TODO
         **/

        static SubmitInfo create(Anvil::CommandBufferBase*        in_opt_cmd_buffer_ptr,
                                 uint32_t                         in_n_semaphores_to_signal,
                                 Anvil::Semaphore* const*         in_opt_semaphore_to_signal_ptrs_ptr,
                                 uint32_t                         in_n_semaphores_to_wait_on,
                                 Anvil::Semaphore* const*         in_opt_semaphore_to_wait_on_ptrs_ptr,
                                 const VkPipelineStageFlags*      in_opt_dst_stage_masks_to_wait_on_ptrs,
                                 bool                             in_should_block,
                                 Anvil::Fence*                    in_opt_fence_ptr = nullptr);
        static SubmitInfo create(uint32_t                         in_n_cmd_buffers,
                                 Anvil::CommandBufferBase* const* in_opt_cmd_buffer_ptrs_ptr,
                                 uint32_t                         in_n_semaphores_to_signal,
                                 Anvil::Semaphore* const*         in_opt_semaphore_to_signal_ptrs_ptr,
                                 uint32_t                         in_n_semaphores_to_wait_on,
                                 Anvil::Semaphore* const*         in_opt_semaphore_to_wait_on_ptrs_ptr,
                                 const VkPipelineStageFlags*      in_opt_dst_stage_masks_to_wait_on_ptrs,
                                 bool                             in_should_block,
                                 Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        static SubmitInfo create_execute(Anvil::CommandBufferBase*        in_cmd_buffer_ptr,
                                         bool                             in_should_block,
                                         Anvil::Fence*                    in_opt_fence_ptr = nullptr);
        static SubmitInfo create_execute(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                         uint32_t                         in_n_cmd_buffers,
                                         bool                             in_should_block,
                                         Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        static SubmitInfo create_execute_signal(Anvil::CommandBufferBase*        in_cmd_buffer_ptr,
                                                uint32_t                         in_n_semaphores_to_signal,
                                                Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                                bool                             in_should_block,
                                                Anvil::Fence*                    in_opt_fence_ptr = nullptr);
        static SubmitInfo create_execute_signal(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                                uint32_t                         in_n_cmd_buffers,
                                                uint32_t                         in_n_semaphores_to_signal,
                                                Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                                bool                             in_should_block,
                                                Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        /** TODO
         *
         *  NOTE: This function always blocks.
         */
        static SubmitInfo create_signal(uint32_t                 in_n_semaphores_to_signal,
                                        Anvil::Semaphore* const* in_semaphore_to_signal_ptrs_ptr,
                                        Anvil::Fence*            in_opt_fence_ptr = nullptr);

        static SubmitInfo create_signal_wait(uint32_t                    in_n_semaphores_to_signal,
                                             Anvil::Semaphore* const*    in_semaphore_to_signal_ptrs_ptr,
                                             uint32_t                    in_n_semaphores_to_wait_on,
                                             Anvil::Semaphore* const*    in_semaphore_to_wait_on_ptrs_ptr,
                                             const VkPipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                             bool                        in_should_block,
                                             Anvil::Fence*               in_opt_fence_ptr = nullptr);

        /** TODO
         *
         *  NOTE: This function always blocks.
         */
        static SubmitInfo create_wait(uint32_t                         in_n_semaphores_to_wait_on,
                                      Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                      const VkPipelineStageFlags*      in_dst_stage_masks_to_wait_on_ptrs,
                                      Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        static SubmitInfo create_wait_execute(Anvil::CommandBufferBase*        in_cmd_buffer_ptr,
                                              uint32_t                         in_n_semaphores_to_wait_on,
                                              Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                              const VkPipelineStageFlags*      in_dst_stage_masks_to_wait_on_ptrs,
                                              bool                             in_should_block,
                                              Anvil::Fence*                    in_opt_fence_ptr = nullptr);
        static SubmitInfo create_wait_execute(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                              uint32_t                         in_n_cmd_buffers,
                                              uint32_t                         in_n_semaphores_to_wait_on,
                                              Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                              const VkPipelineStageFlags*      in_dst_stage_masks_to_wait_on_ptrs,
                                              bool                             in_should_block,
                                              Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        static SubmitInfo create_wait_execute_signal(Anvil::CommandBufferBase*        in_cmd_buffer_ptr,
                                                     uint32_t                         in_n_semaphores_to_signal,
                                                     Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                                     uint32_t                         in_n_semaphores_to_wait_on,
                                                     Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                                     const VkPipelineStageFlags*      in_dst_stage_masks_to_wait_on_ptrs,
                                                     bool                             in_should_block,
                                                     Anvil::Fence*                    in_opt_fence_ptr = nullptr);
        static SubmitInfo create_wait_execute_signal(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                                     uint32_t                         in_n_cmd_buffers,
                                                     uint32_t                         in_n_semaphores_to_signal,
                                                     Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                                     uint32_t                         in_n_semaphores_to_wait_on,
                                                     Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                                     const VkPipelineStageFlags*      in_dst_stage_masks_to_wait_on_ptrs,
                                                     bool                             in_should_block,
                                                     Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        Anvil::CommandBufferBase* const* get_command_buffers_sgpu() const
        {
            return command_buffers_sgpu_ptr;
        }

        #if defined(_WIN32)
            bool get_d3d12_fence_semaphore_values(const uint64_t** out_d3d12_fence_signal_semaphore_values_ptr_ptr,
                                                  const uint64_t** out_d3d12_fence_wait_semaphore_values_ptr_ptr) const
            {
                bool result = (d3d12_fence_signal_semaphore_values_ptr != nullptr && n_signal_semaphores != 0) ||
                              (d3d12_fence_wait_semaphore_values_ptr   != nullptr && n_wait_semaphores   != 0);

                *out_d3d12_fence_signal_semaphore_values_ptr_ptr = d3d12_fence_signal_semaphore_values_ptr;
                *out_d3d12_fence_wait_semaphore_values_ptr_ptr   = d3d12_fence_wait_semaphore_values_ptr;

                return result;
            }
        #endif

        const VkPipelineStageFlags* get_destination_stage_wait_masks() const
        {
            return dst_stage_wait_masks_ptr;
        }

        Anvil::Fence* get_fence() const
        {
            return fence_ptr;
        }

        const uint32_t& get_n_command_buffers() const
        {
            return n_command_buffers;
        }

        const uint32_t& get_n_signal_semaphores() const
        {
            return n_signal_semaphores;
        }

        const uint32_t& get_n_wait_semaphores() const
        {
            return n_wait_semaphores;
        }

        Anvil::Semaphore* const* get_signal_semaphores_sgpu() const
        {
            return signal_semaphores_sgpu_ptr;
        }

        const bool& get_should_block() const
        {
            return should_block;
        }

        const uint64_t& get_timeout() const
        {
            return timeout;
        }

        const SubmissionType& get_type() const
        {
            return type;
        }

        Anvil::Semaphore* const* get_wait_semaphores_sgpu() const
        {
            return wait_semaphores_sgpu_ptr;
        }

        #if defined(_WIN32)
            /* Calling this function will make Anvil fill & chain a VkD3D12FenceSubmitInfoKHR struct at queue submission time.
             *
             * Requires VK_KHR_external_semaphore_win32 support.
             *
             * NOTE: The structure caches the provided pointers, not the contents available under derefs! Make sure the pointers remain valid
             *       for the time of the Queue::submit() call.
             *
             * @param in_signal_semaphore_values_ptr An array of exactly n_signal_semaphores values. Usage as per VkD3D12FenceSubmitInfoKHR::pSignalSemaphoreValues.
             *                                       Must not be nullptr unless n_signal_semaphores is 0.
             * @param in_wait_semaphore_values_ptr   An array of exactly n_wait_semaphores values. Usage as per VkD3D12FenceSubmitInfoKHR::pWaitSemaphoreValues.
             *                                       Must not be nullptr unless n_wait_semaphores is 0.
             **/
            void set_d3d12_fence_semaphore_values(const uint64_t* in_signal_semaphore_values_ptr,
                                                  const uint32_t& in_n_signal_semaphore_values,
                                                  const uint64_t* in_wait_semaphore_values_ptr,
                                                  const uint32_t& in_n_wait_semaphore_values)
            {
                ANVIL_REDUNDANT_ARGUMENT_CONST(in_n_signal_semaphore_values);
                ANVIL_REDUNDANT_ARGUMENT_CONST(in_n_wait_semaphore_values);

                anvil_assert((n_signal_semaphores != 0  && in_signal_semaphore_values_ptr != nullptr) ||
                             (n_signal_semaphores == 0) );
                anvil_assert((n_wait_semaphores   != 0  && in_wait_semaphore_values_ptr   != nullptr) ||
                             (n_wait_semaphores   == 0) );

                anvil_assert(in_n_signal_semaphore_values == n_signal_semaphores);
                anvil_assert(in_n_wait_semaphore_values   == n_wait_semaphores);

                d3d12_fence_signal_semaphore_values_ptr = in_signal_semaphore_values_ptr;
                d3d12_fence_wait_semaphore_values_ptr   = in_wait_semaphore_values_ptr;
            }
        #endif

        /* Sets a timeout which is used when waiting on a fence that the submission is associated with.
         *
         * If your submission times out, you're likely about to experience a TDR and lose the device.
         */
        void set_timeout(const uint64_t& in_timeout)
        {
            anvil_assert(should_block);

            timeout = in_timeout;
        }
    private:
        SubmitInfo(Anvil::CommandBufferBase*   in_cmd_buffer_ptr,
                   uint32_t                    in_n_semaphores_to_signal,
                   Anvil::Semaphore* const*    in_opt_semaphore_to_signal_ptrs_ptr,
                   uint32_t                    in_n_semaphores_to_wait_on,
                   Anvil::Semaphore* const*    in_opt_semaphore_to_wait_on_ptrs_ptr,
                   const VkPipelineStageFlags* in_opt_dst_stage_masks_to_wait_on_ptrs,
                   bool                        in_should_block,
                   Anvil::Fence*               in_opt_fence_ptr);

        SubmitInfo(uint32_t                         in_n_command_buffers,
                   Anvil::CommandBufferBase* const* in_opt_cmd_buffer_ptrs_ptr,
                   uint32_t                         in_n_semaphores_to_signal,
                   Anvil::Semaphore* const*         in_opt_semaphore_to_signal_ptrs_ptr,
                   uint32_t                         in_n_semaphores_to_wait_on,
                   Anvil::Semaphore* const*         in_opt_semaphore_to_wait_on_ptrs_ptr,
                   const VkPipelineStageFlags*      in_opt_dst_stage_masks_to_wait_on_ptrs,
                   bool                             in_should_block,
                   Anvil::Fence*                    in_opt_fence_ptr);

        Anvil::CommandBufferBase* helper_cmd_buffer_raw_ptr;

        Anvil::CommandBufferBase* const*   command_buffers_sgpu_ptr;
        uint32_t                           n_command_buffers;

        Anvil::Semaphore* const*       signal_semaphores_sgpu_ptr;
        uint32_t                       n_signal_semaphores;

        const VkPipelineStageFlags*    dst_stage_wait_masks_ptr;
        Anvil::Semaphore* const*       wait_semaphores_sgpu_ptr;
        uint32_t                       n_wait_semaphores;

        Anvil::Fence* fence_ptr;

        #if defined(_WIN32)
            const uint64_t* d3d12_fence_signal_semaphore_values_ptr;
            const uint64_t* d3d12_fence_wait_semaphore_values_ptr;
        #endif

        bool                 should_block;
        uint64_t             timeout;
        const SubmissionType type;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(SubmitInfo);
    } SubmitInfo;

    /* Represents a Vulkan structure header */
    typedef struct
    {
        VkStructureType type;
        const void*     next_ptr;
    } VkStructHeader;

}; /* namespace Anvil */
#endif
