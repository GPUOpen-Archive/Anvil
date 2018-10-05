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
    /* Note: Matches VkSampleLocationEXT in terms of the layout and size.
     */
    typedef struct SampleLocation
    {
        float x;
        float y;

        SampleLocation()
            :x(0.0f),
             y(0.0f)
        {
            /* Stub */
        }

        SampleLocation(const float& in_x,
                       const float& in_y)
            :x(in_x),
             y(in_y)
        {
            /* Stub */
        }
    } SampleLocation;

    static_assert(sizeof(SampleLocation)      == sizeof(VkSampleLocationEXT),      "Struct sizes must match");
    static_assert(offsetof(SampleLocation, x) == offsetof(VkSampleLocationEXT, x), "Member offsets much match");
    static_assert(offsetof(SampleLocation, y) == offsetof(VkSampleLocationEXT, y), "Member offsets much match");

    typedef struct SampleLocationsInfo
    {
        Anvil::SampleCountFlagBits         sample_locations_per_pixel;
        VkExtent2D                         sample_location_grid_size;
        std::vector<Anvil::SampleLocation> sample_locations;

        SampleLocationsInfo()
        {
            sample_locations_per_pixel       = Anvil::SampleCountFlagBits::NONE;
            sample_location_grid_size.height = 0;
            sample_location_grid_size.width  = 0;
        }

        VkSampleLocationsInfoEXT get_vk() const
        {
            VkSampleLocationsInfoEXT result;

            result.pNext                   = nullptr;
            result.pSampleLocations        = (sample_locations.size() != 0) ? reinterpret_cast<const VkSampleLocationEXT*>(&sample_locations.at(0) )
                                                                            : nullptr;
            result.sampleLocationGridSize  = sample_location_grid_size;
            result.sampleLocationsCount    = static_cast<uint32_t>             (sample_locations.size() );
            result.sampleLocationsPerPixel = static_cast<VkSampleCountFlagBits>(sample_locations_per_pixel);
            result.sType                   = VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;

            return result;
        }

    } SampleLocationsInfo;

    typedef struct AttachmentSampleLocations
    {
        uint32_t                   n_attachment;
        Anvil::SampleLocationsInfo sample_locations_info;

        AttachmentSampleLocations()
        {
            n_attachment = UINT32_MAX;
        }

        VkAttachmentSampleLocationsEXT get_vk() const
        {
            VkAttachmentSampleLocationsEXT result;

            result.attachmentIndex     = n_attachment;
            result.sampleLocationsInfo = sample_locations_info.get_vk();

            return result;
        }
    } AttachmentSampleLocations;

    typedef struct SubpassSampleLocations
    {
        uint32_t                   n_subpass;
        Anvil::SampleLocationsInfo sample_locations_info;

        SubpassSampleLocations()
        {
            n_subpass = UINT32_MAX;
        }

        VkSubpassSampleLocationsEXT get_vk() const
        {
            VkSubpassSampleLocationsEXT result;

            result.subpassIndex        = n_subpass;
            result.sampleLocationsInfo = sample_locations_info.get_vk();

            return result;
        }
    } SubpassSampleLocations;

    /* NOTE: Maps 1:1 to VkImageSubresource */
    typedef struct
    {
        uint32_t                         min_image_count;
        uint32_t                         max_image_count;
        VkExtent2D                       current_extent;
        VkExtent2D                       min_image_extent;
        VkExtent2D                       max_image_extent;
        uint32_t                         max_image_array_layers;
        Anvil::SurfaceTransformFlags     supported_transforms;
        Anvil::SurfaceTransformFlagBits  current_transform;
        Anvil::CompositeAlphaFlags       supported_composite_alpha;
        Anvil::ImageUsageFlags           supported_usage_flags;
    } SurfaceCapabilities;

    static_assert(sizeof(SurfaceCapabilities)                              == sizeof(VkSurfaceCapabilitiesKHR),                            "Struct sizes much match");
    static_assert(offsetof(SurfaceCapabilities, min_image_count)           == offsetof(VkSurfaceCapabilitiesKHR, minImageCount),           "Member offsets must match");
    static_assert(offsetof(SurfaceCapabilities, max_image_count)           == offsetof(VkSurfaceCapabilitiesKHR, maxImageCount),           "Member offsets must match");
    static_assert(offsetof(SurfaceCapabilities, current_extent)            == offsetof(VkSurfaceCapabilitiesKHR, currentExtent),           "Member offsets must match");
    static_assert(offsetof(SurfaceCapabilities, min_image_extent)          == offsetof(VkSurfaceCapabilitiesKHR, minImageExtent),          "Member offsets must match");
    static_assert(offsetof(SurfaceCapabilities, max_image_extent)          == offsetof(VkSurfaceCapabilitiesKHR, maxImageExtent),          "Member offsets must match");
    static_assert(offsetof(SurfaceCapabilities, max_image_array_layers)    == offsetof(VkSurfaceCapabilitiesKHR, maxImageArrayLayers),     "Member offsets must match");
    static_assert(offsetof(SurfaceCapabilities, supported_transforms)      == offsetof(VkSurfaceCapabilitiesKHR, supportedTransforms),     "Member offsets must match");
    static_assert(offsetof(SurfaceCapabilities, current_transform)         == offsetof(VkSurfaceCapabilitiesKHR, currentTransform),        "Member offsets must match");
    static_assert(offsetof(SurfaceCapabilities, supported_composite_alpha) == offsetof(VkSurfaceCapabilitiesKHR, supportedCompositeAlpha), "Member offsets must match");
    static_assert(offsetof(SurfaceCapabilities, supported_usage_flags)     == offsetof(VkSurfaceCapabilitiesKHR, supportedUsageFlags),     "Member offsets must match");

    /* NOTE: Maps 1:1 to VkImageSubresource */
    typedef struct
    {
        Anvil::ImageAspectFlags aspect_mask;
        uint32_t                mip_level;
        uint32_t                array_layer;

        VkImageSubresource get_vk() const
        {
            VkImageSubresource result;

            result.arrayLayer = array_layer;
            result.aspectMask = aspect_mask.get_vk();
            result.mipLevel   = mip_level;

            return result;
        }
    } ImageSubresource;

    static_assert(sizeof(ImageSubresource)                == sizeof(VkImageSubresource),               "Struct sizes much match");
    static_assert(offsetof(ImageSubresource, aspect_mask) == offsetof(VkImageSubresource, aspectMask), "Member offsets must match");
    static_assert(offsetof(ImageSubresource, mip_level)   == offsetof(VkImageSubresource, mipLevel),   "Member offsets must match");
    static_assert(offsetof(ImageSubresource, array_layer) == offsetof(VkImageSubresource, arrayLayer), "Member offsets must match");

    /* NOTE: Maps 1:1 to VkImageSubresourceRange */
    typedef struct ImageSubresourceRange
    {
        Anvil::ImageAspectFlags aspect_mask;
        uint32_t                base_mip_level;
        uint32_t                level_count;
        uint32_t                base_array_layer;
        uint32_t                layer_count;

        VkImageSubresourceRange get_vk() const
        {
            VkImageSubresourceRange result;

            result.aspectMask     = aspect_mask.get_vk();
            result.baseMipLevel   = base_mip_level;
            result.levelCount     = level_count;
            result.baseArrayLayer = base_array_layer;
            result.layerCount     = layer_count;

            return result;
        }

        bool operator==(const ImageSubresourceRange&) const;
    } ImageSubresourceRange;

    static_assert(sizeof(ImageSubresourceRange)                     == sizeof(VkImageSubresourceRange),                   "Struct sizes much match");
    static_assert(offsetof(ImageSubresourceRange, aspect_mask)      == offsetof(VkImageSubresourceRange, aspectMask),     "Member offsets must match");
    static_assert(offsetof(ImageSubresourceRange, base_mip_level)   == offsetof(VkImageSubresourceRange, baseMipLevel),   "Member offsets must match");
    static_assert(offsetof(ImageSubresourceRange, level_count)      == offsetof(VkImageSubresourceRange, levelCount),     "Member offsets must match");
    static_assert(offsetof(ImageSubresourceRange, base_array_layer) == offsetof(VkImageSubresourceRange, baseArrayLayer), "Member offsets must match");
    static_assert(offsetof(ImageSubresourceRange, layer_count)      == offsetof(VkImageSubresourceRange, layerCount),     "Member offsets must match");

    /* NOTE: Maps 1:1 to VkImageSubresourceLayers */
    typedef struct
    {
        Anvil::ImageAspectFlags aspect_mask;
        uint32_t                mip_level;
        uint32_t                base_array_layer;
        uint32_t                layer_count;
    } ImageSubresourceLayers;

    static_assert(sizeof(ImageSubresourceLayers)                     == sizeof(VkImageSubresourceLayers),                   "Struct sizes much match");
    static_assert(offsetof(ImageSubresourceLayers, aspect_mask)      == offsetof(VkImageSubresourceLayers, aspectMask),     "Member offsets must match");
    static_assert(offsetof(ImageSubresourceLayers, mip_level)        == offsetof(VkImageSubresourceLayers, mipLevel),       "Member offsets must match");
    static_assert(offsetof(ImageSubresourceLayers, base_array_layer) == offsetof(VkImageSubresourceLayers, baseArrayLayer), "Member offsets must match");
    static_assert(offsetof(ImageSubresourceLayers, layer_count)      == offsetof(VkImageSubresourceLayers, layerCount),     "Member offsets must match");

    /* NOTE: Maps 1:1 to VkSubresourceLayout */
    typedef struct
    {
        VkDeviceSize offset;
        VkDeviceSize size;
        VkDeviceSize row_pitch;
        VkDeviceSize array_pitch;
        VkDeviceSize depth_pitch;
    } SubresourceLayout;

    static_assert(sizeof(SubresourceLayout)                == sizeof(VkSubresourceLayout),               "Struct sizes much match");
    static_assert(offsetof(SubresourceLayout, offset)      == offsetof(VkSubresourceLayout, offset),     "Member offsets must match");
    static_assert(offsetof(SubresourceLayout, size)        == offsetof(VkSubresourceLayout, size),       "Member offsets must match");
    static_assert(offsetof(SubresourceLayout, row_pitch)   == offsetof(VkSubresourceLayout, rowPitch),   "Member offsets must match");
    static_assert(offsetof(SubresourceLayout, array_pitch) == offsetof(VkSubresourceLayout, arrayPitch), "Member offsets must match");
    static_assert(offsetof(SubresourceLayout, depth_pitch) == offsetof(VkSubresourceLayout, depthPitch), "Member offsets must match");

    typedef struct
    {
        Anvil::ImageSubresourceLayers    src_subresource;
        VkOffset3D                       src_offsets[2];
        Anvil::ImageSubresourceLayers    dst_subresource;
        VkOffset3D                       dst_offsets[2];
    } ImageBlit;

    static_assert(sizeof(ImageBlit)                    == sizeof(VkImageBlit),                   "Struct sizes much match");
    static_assert(offsetof(ImageBlit, src_subresource) == offsetof(VkImageBlit, srcSubresource), "Member offsets must match");
    static_assert(offsetof(ImageBlit, src_offsets)     == offsetof(VkImageBlit, srcOffsets),     "Member offsets must match");
    static_assert(offsetof(ImageBlit, dst_subresource) == offsetof(VkImageBlit, dstSubresource), "Member offsets must match");
    static_assert(offsetof(ImageBlit, dst_offsets)     == offsetof(VkImageBlit, dstOffsets),     "Member offsets must match");

    /* NOTE: Maps 1:1 to VkBufferCopy */
    typedef struct
    {
        VkDeviceSize src_offset;
        VkDeviceSize dst_offset;
        VkDeviceSize size;
    } BufferCopy;

    static_assert(sizeof(BufferCopy)               == sizeof(VkBufferCopy),              "Struct sizes much match");
    static_assert(offsetof(BufferCopy, src_offset) == offsetof(VkBufferCopy, srcOffset), "Member offsets must match");
    static_assert(offsetof(BufferCopy, dst_offset) == offsetof(VkBufferCopy, dstOffset), "Member offsets must match");
    static_assert(offsetof(BufferCopy, size)       == offsetof(VkBufferCopy, size),      "Member offsets must match");

    /* NOTE: Maps 1:1 to VkBufferImageCopy */
    typedef struct
    {
        VkDeviceSize                  buffer_offset;
        uint32_t                      buffer_row_length;
        uint32_t                      buffer_image_height;
        Anvil::ImageSubresourceLayers image_subresource;
        VkOffset3D                    image_offset;
        VkExtent3D                    image_extent;
    } BufferImageCopy;

    static_assert(sizeof(BufferImageCopy)                        == sizeof(VkBufferImageCopy),                      "Struct sizes much match");
    static_assert(offsetof(BufferImageCopy, buffer_offset)       == offsetof(VkBufferImageCopy, bufferOffset),      "Member offsets must match");
    static_assert(offsetof(BufferImageCopy, buffer_row_length)   == offsetof(VkBufferImageCopy, bufferRowLength),   "Member offsets must match");
    static_assert(offsetof(BufferImageCopy, buffer_image_height) == offsetof(VkBufferImageCopy, bufferImageHeight), "Member offsets must match");
    static_assert(offsetof(BufferImageCopy, image_subresource)   == offsetof(VkBufferImageCopy, imageSubresource),  "Member offsets must match");
    static_assert(offsetof(BufferImageCopy, image_offset)        == offsetof(VkBufferImageCopy, imageOffset),       "Member offsets must match");
    static_assert(offsetof(BufferImageCopy, image_extent)        == offsetof(VkBufferImageCopy, imageExtent),       "Member offsets must match");

    /* NOTE: Maps 1:1 to VkClearAttachment */
    typedef struct
    {
        Anvil::ImageAspectFlags aspect_mask;
        uint32_t                color_attachment;
        VkClearValue            clear_value;
    } ClearAttachment;

    static_assert(sizeof(ClearAttachment)                     == sizeof(VkClearAttachment),                   "Struct sizes much match");
    static_assert(offsetof(ClearAttachment, aspect_mask)      == offsetof(VkClearAttachment, aspectMask),     "Member offsets must match");
    static_assert(offsetof(ClearAttachment, color_attachment) == offsetof(VkClearAttachment, colorAttachment), "Member offsets must match");
    static_assert(offsetof(ClearAttachment, clear_value)      == offsetof(VkClearAttachment, clearValue),      "Member offsets must match");

    /* NOTE: Maps 1:1 to VkImageCopy */
    typedef struct
    {
        Anvil::ImageSubresourceLayers src_subresource;
        VkOffset3D                    src_offset;
        Anvil::ImageSubresourceLayers dst_subresource;
        VkOffset3D                    dst_offset;
        VkExtent3D                    extent;
    } ImageCopy;

    typedef struct ExternalFenceProperties
    {
        Anvil::ExternalFenceHandleTypeFlags compatible_external_handle_types;
        Anvil::ExternalFenceHandleTypeFlags export_from_imported_external_handle_types;

        bool is_exportable;
        bool is_importable;

        /* Dummy constructor */
        ExternalFenceProperties();

        ExternalFenceProperties(const VkExternalFencePropertiesKHR& in_external_memory_props);
    } ExternalFenceProperties;

    typedef struct ExternalMemoryProperties
    {
        Anvil::ExternalMemoryHandleTypeFlags compatible_external_handle_types;
        Anvil::ExternalMemoryHandleTypeFlags export_from_imported_external_handle_types;

        bool is_exportable;
        bool is_importable;

        /* Dummy constructor */
        ExternalMemoryProperties();

        ExternalMemoryProperties(const VkExternalMemoryPropertiesKHR& in_external_memory_props);
    } ExternalMemoryProperties;

    typedef struct ExternalSemaphoreProperties
    {
        Anvil::ExternalSemaphoreHandleTypeFlags compatible_external_handle_types;
        Anvil::ExternalSemaphoreHandleTypeFlags export_from_imported_external_handle_types;

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
        Anvil::AccessFlags dst_access_mask;
        Anvil::AccessFlags src_access_mask;

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
        explicit BufferBarrier(Anvil::AccessFlags in_source_access_mask,
                               Anvil::AccessFlags in_destination_access_mask,
                               uint32_t           in_src_queue_family_index,
                               uint32_t           in_dst_queue_family_index,
                               Anvil::Buffer*     in_buffer_ptr,
                               VkDeviceSize       in_offset,
                               VkDeviceSize       in_size);

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

        bool operator==(const BufferBarrier&) const;
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
        const Anvil::BufferCreateFlags             create_flags;
        const Anvil::ExternalMemoryHandleTypeFlags external_memory_handle_type;
        const Anvil::BufferUsageFlags              usage_flags;

        explicit BufferPropertiesQuery(const Anvil::BufferCreateFlags              in_create_flags,
                                       const Anvil::ExternalMemoryHandleTypeFlags& in_external_memory_handle_type,
                                       const Anvil::BufferUsageFlags&              in_usage_flags)
            :create_flags               (in_create_flags),
             external_memory_handle_type(in_external_memory_handle_type),
             usage_flags                (in_usage_flags)
        {
            /* Stub */
        }

        BufferPropertiesQuery           (const BufferPropertiesQuery& in_query) = default;
        BufferPropertiesQuery& operator=(const BufferPropertiesQuery& in_query) = delete;
    } BufferPropertiesQuery;

    /* Used by Buffer::set_nonsparse_memory_multi(). Requires VK_KHR_device_group support. */
    typedef struct BufferMemoryBindingUpdate
    {
        Anvil::Buffer*      buffer_ptr;
        bool                memory_block_owned_by_buffer;
        Anvil::MemoryBlock* memory_block_ptr;

        /* May either be empty (for sGPU and mGPU devices) or:
         *
         * 1) hold up as many physical devices as there are assigned to the device
         *    group (mGPU devices)
         * 2) hold the physical device, from which the logical device has been created
         *    (sGPU device)
         */
        std::vector<const Anvil::PhysicalDevice*> physical_devices;

        BufferMemoryBindingUpdate();
    } BufferMemoryBindingUpdate;

    /** TODO */
    typedef struct CommandBufferMGPUSubmission
    {
        /* Command buffer to execute. May be nullptr. */
        Anvil::CommandBufferBase* cmd_buffer_ptr;

        /* Bit mask determining which devices in the device group will execute the command buffer. */
        uint32_t device_mask;

        /* Default dummy constructor */
        CommandBufferMGPUSubmission()
        {
            cmd_buffer_ptr = nullptr;
            device_mask    = 0;
        }
    } CommandBufferMGPUSubmission;

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

    typedef struct EXTExternalMemoryHostProperties
    {
        VkDeviceSize min_imported_host_pointer_alignment;

        EXTExternalMemoryHostProperties();
        EXTExternalMemoryHostProperties(const VkPhysicalDeviceExternalMemoryHostPropertiesEXT& in_props);

        bool operator==(const EXTExternalMemoryHostProperties& in_props) const;
    } EXTExternalMemoryHostProperties;

    typedef struct EXTSampleLocationsProperties
    {
        VkExtent2D              max_sample_location_grid_size;
        float                   sample_location_coordinate_range[2];
        Anvil::SampleCountFlags sample_location_sample_counts;
        uint32_t                sample_location_sub_pixel_bits;
        bool                    variable_sample_locations;

        EXTSampleLocationsProperties();
        EXTSampleLocationsProperties(const VkPhysicalDeviceSampleLocationsPropertiesEXT& in_props);

        bool operator==(const EXTSampleLocationsProperties& in_props) const;
    } EXTSampleLocationsProperties;

    typedef struct EXTVertexAttributeDivisorProperties
    {
        uint32_t max_vertex_attribute_divisor;

        EXTVertexAttributeDivisorProperties();
        EXTVertexAttributeDivisorProperties(const VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT& in_props);

        bool operator==(const EXTVertexAttributeDivisorProperties& in_props) const;
    } EXTVertexAttributeDivisorProperties;

    /* Used by Image::set_memory_multi(). Requires VK_KHR_device_group support. */
    typedef struct ImagePhysicalDeviceMemoryBindingUpdate
    {
        Anvil::Image*                             image_ptr;
        bool                                      memory_block_owned_by_image;
        Anvil::MemoryBlock*                       memory_block_ptr;
        std::vector<const Anvil::PhysicalDevice*> physical_devices;

        ImagePhysicalDeviceMemoryBindingUpdate();
    } ImagePhysicalDeviceMemoryBindingUpdate;

    /* Used by Image::set_memory_multi(). Requires VK_KHR_device_group support. */
    typedef struct ImageSFRMemoryBindingUpdate
    {
        Anvil::Image*         image_ptr;
        bool                  memory_block_owned_by_image;
        Anvil::MemoryBlock*   memory_block_ptr;
        std::vector<VkRect2D> SFRs;

        ImageSFRMemoryBindingUpdate();
    } ImageSFRMemoryBindingUpdate;

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
        Anvil::DescriptorType descriptor_type;
        uint32_t              n_descriptors;
        uint32_t              n_destination_array_element;
        uint32_t              n_destination_binding;
        size_t                offset;
        size_t                stride;

        DescriptorUpdateTemplateEntry();
        DescriptorUpdateTemplateEntry(const Anvil::DescriptorType& in_descriptor_type,
                                      const uint32_t&              in_n_destination_array_element,
                                      const uint32_t&              in_n_destination_binding,
                                      const uint32_t&              in_n_descriptors,
                                      const size_t&                in_offset,
                                      const size_t&                in_stride);

        VkDescriptorUpdateTemplateEntryKHR get_vk_descriptor_update_template_entry_khr() const;

        bool operator==(const DescriptorUpdateTemplateEntry& in_entry) const;
        bool operator< (const DescriptorUpdateTemplateEntry& in_entry) const;
    } DescriptorUpdateTemplateEntry;

    typedef struct ExtensionAMDBufferMarkerEntrypoints
    {
        PFN_vkCmdWriteBufferMarkerAMD vkCmdWriteBufferMarkerAMD;

        ExtensionAMDBufferMarkerEntrypoints();
    } ExtensionAMDBufferMarkerEntrypoints;

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

    typedef struct ExtensionEXTExternalMemoryHostEntrypoints
    {
        PFN_vkGetMemoryHostPointerPropertiesEXT vkGetMemoryHostPointerPropertiesEXT;

        ExtensionEXTExternalMemoryHostEntrypoints();
    } ExtensionEXTExternalMemoryHostEntrypoints;

    typedef struct ExtensionEXTHdrMetadataEntrypoints
    {
        PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT;

        ExtensionEXTHdrMetadataEntrypoints();
    } ExtensionEXTHdrMetadataEntrypoints;

    typedef struct ExtensionEXTSampleLocationsEntrypoints
    {
        PFN_vkCmdSetSampleLocationsEXT                  vkCmdSetSampleLocationsEXT;
        PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT vkGetPhysicalDeviceMultisamplePropertiesEXT;

        ExtensionEXTSampleLocationsEntrypoints();
    } ExtensionEXTSampleLocationsEntrypoints;

    typedef struct ExtensionKHRDeviceGroupEntrypoints
    {
        PFN_vkAcquireNextImage2KHR                  vkAcquireNextImage2KHR;
        PFN_vkCmdDispatchBaseKHR                    vkCmdDispatchBaseKHR;
        PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR   vkGetDeviceGroupPeerMemoryFeaturesKHR;
        PFN_vkGetDeviceGroupPresentCapabilitiesKHR  vkGetDeviceGroupPresentCapabilitiesKHR;
        PFN_vkGetDeviceGroupSurfacePresentModesKHR  vkGetDeviceGroupSurfacePresentModesKHR;
        PFN_vkGetPhysicalDevicePresentRectanglesKHR vkGetPhysicalDevicePresentRectanglesKHR;
        PFN_vkCmdSetDeviceMaskKHR                   vkCmdSetDeviceMaskKHR;

        ExtensionKHRDeviceGroupEntrypoints();
    } ExtensionKHRDeviceGroupEntrypoints;

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

    typedef struct EXTSamplerFilterMinmaxProperties
    {
        bool filter_minmax_single_component_formats;
        bool filter_minmax_image_component_mapping;

        EXTSamplerFilterMinmaxProperties();
        EXTSamplerFilterMinmaxProperties(const VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT& in_props);

        bool operator==(const EXTSamplerFilterMinmaxProperties&) const;

    } EXTSamplerFilterMinmaxProperties;

    typedef struct FenceProperties
    {
        ExternalFenceProperties external_fence_properties;

        /* Dummy constructor */
        FenceProperties();

        FenceProperties(const ExternalFenceProperties& in_external_fence_properties);
    } FenceProperties;

    typedef struct FencePropertiesQuery
    {
        const Anvil::ExternalFenceHandleTypeFlagBits external_fence_handle_type;

        explicit FencePropertiesQuery(const Anvil::ExternalFenceHandleTypeFlagBits& in_external_fence_handle_type)
            :external_fence_handle_type(in_external_fence_handle_type)
        {
            /* Stub */
        }

        FencePropertiesQuery           (const FencePropertiesQuery&) = default;
        FencePropertiesQuery& operator=(const FencePropertiesQuery&) = delete;
    } FencePropertiesQuery;

    typedef struct FormatProperties
    {
        FormatFeatureFlags buffer_capabilities;
        FormatFeatureFlags linear_tiling_capabilities;
        FormatFeatureFlags optimal_tiling_capabilities;

        FormatProperties();
        FormatProperties(const VkFormatProperties& in_format_props);
    } FormatProperties;

    typedef struct XYColorEXT
    {
        float x;
        float y;

        XYColorEXT()
            :x(0.0f),
             y(0.0f)
        {
            /* Stub */
        }

        VkXYColorEXT get_vk() const
        {
            VkXYColorEXT result;

            result.x = x;
            result.y = y;

            return result;
        }
    } XYColorEXT;

    typedef struct HdrMetadataEXT
    {
        Anvil::XYColorEXT  display_primary_red;
        Anvil::XYColorEXT  display_primary_green;
        Anvil::XYColorEXT  display_primary_blue;
        float              max_content_light_level;
        float              max_frame_average_light_level;
        float              max_luminance;
        float              min_luminance;
        Anvil::XYColorEXT  white_point;

        HdrMetadataEXT()
        {
            max_content_light_level       = 0.0f;
            max_frame_average_light_level = 0.0f;
            max_luminance                 = 0.0f;
            min_luminance                 = 0.0f;
        }

        VkHdrMetadataEXT get_vk() const
        {
            VkHdrMetadataEXT result;

            result.displayPrimaryBlue        = display_primary_blue.get_vk ();
            result.displayPrimaryGreen       = display_primary_green.get_vk();
            result.displayPrimaryRed         = display_primary_red.get_vk  ();
            result.maxContentLightLevel      = max_content_light_level;
            result.maxFrameAverageLightLevel = max_frame_average_light_level;
            result.maxLuminance              = max_luminance;
            result.minLuminance              = min_luminance;
            result.pNext                     = nullptr;
            result.sType                     = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
            result.whitePoint                = white_point.get_vk();

            return result;
        }
    } HdrMetadataEXT;

    typedef struct ImageFormatProperties
    {
        ExternalMemoryProperties external_handle_properties;

        VkExtent3D              max_extent;
        VkDeviceSize            max_resource_size;
        uint32_t                n_max_array_layers;
        uint32_t                n_max_mip_levels;
        Anvil::SampleCountFlags sample_counts;

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
        explicit ImageFormatPropertiesQuery(const Anvil::Format&           in_format,
                                            const Anvil::ImageType&        in_image_type,
                                            const Anvil::ImageTiling&      in_tiling,
                                            const Anvil::ImageUsageFlags&  in_usage_flags,
                                            const Anvil::ImageCreateFlags& in_create_flags)
            :create_flags               (in_create_flags),
             external_memory_handle_type(Anvil::ExternalMemoryHandleTypeFlagBits::NONE),
             format                     (in_format),
             image_type                 (in_image_type),
             tiling                     (in_tiling),
             usage_flags                (in_usage_flags)
        {
            /* Stub */
        }

        void set_external_memory_handle_type(const Anvil::ExternalMemoryHandleTypeFlagBits& in_external_memory_handle_type)
        {
            external_memory_handle_type = in_external_memory_handle_type;
        }

        const Anvil::ImageCreateFlags           create_flags;
        Anvil::ExternalMemoryHandleTypeFlagBits external_memory_handle_type;
        const Anvil::Format                     format;
        const Anvil::ImageType                  image_type;
        const Anvil::ImageTiling                tiling;
        const Anvil::ImageUsageFlags            usage_flags;

        ImageFormatPropertiesQuery           (const ImageFormatPropertiesQuery& in_query) = default;
        ImageFormatPropertiesQuery& operator=(const ImageFormatPropertiesQuery& in_query) = delete;
    } ImageFormatPropertiesQuery;

    /* NOTE: Maps 1:1 to VkImageResolve */
    typedef struct
    {
        Anvil::ImageSubresourceLayers src_subresource;
        VkOffset3D                    src_offset;
        Anvil::ImageSubresourceLayers dst_subresource;
        VkOffset3D                    dst_offset;
        VkExtent3D                    extent;
    } ImageResolve;

    static_assert(sizeof(ImageResolve)                    == sizeof(VkImageResolve),                   "Struct sizes must match");
    static_assert(offsetof(ImageResolve, src_subresource) == offsetof(VkImageResolve, srcSubresource), "Member offsets must match");
    static_assert(offsetof(ImageResolve, src_offset)      == offsetof(VkImageResolve, srcOffset),      "Member offsets must match");
    static_assert(offsetof(ImageResolve, dst_subresource) == offsetof(VkImageResolve, dstSubresource), "Member offsets must match");
    static_assert(offsetof(ImageResolve, dst_offset)      == offsetof(VkImageResolve, dstOffset),      "Member offsets must match");
    static_assert(offsetof(ImageResolve, extent)          == offsetof(VkImageResolve, extent),         "Member offsets must match");

    /** Describes an image memory barrier. */
    typedef struct ImageBarrier
    {
        Anvil::AccessFlags dst_access_mask;
        Anvil::AccessFlags src_access_mask;

        uint32_t                     dst_queue_family_index;
        VkImage                      image;
        VkImageMemoryBarrier         image_barrier_vk;
        Anvil::Image*                image_ptr;
        Anvil::ImageLayout           new_layout;
        Anvil::ImageLayout           old_layout;
        uint32_t                     src_queue_family_index;
        Anvil::ImageSubresourceRange subresource_range;

        /** Constructor.
         *
         *  Note that @param in_image_ptr is retained by this function.
         *
         *  @param in_source_access_mask      Source access mask to use for the barrier.
         *  @param in_destination_access_mask Destination access mask to use for the barrier.
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
        ImageBarrier(Anvil::AccessFlags           in_source_access_mask,
                     Anvil::AccessFlags           in_destination_access_mask,
                     Anvil::ImageLayout           in_old_layout,
                     Anvil::ImageLayout           in_new_layout,
                     uint32_t                     in_src_queue_family_index,
                     uint32_t                     in_dst_queue_family_index,
                     Anvil::Image*                in_image_ptr,
                     Anvil::ImageSubresourceRange in_image_subresource_range);

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

       bool operator==(const ImageBarrier& in_barrier) const;

    private:
        ImageBarrier& operator=(const ImageBarrier&);
    } ImageBarrier;

    typedef struct ExternalMemoryHandleImportInfo
    {
        ExternalHandleType handle;   /* Used for non-host pointer import ops */
        void*              host_ptr; /* Used for host pointer import ops     */

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

            host_ptr = nullptr;
        }
    } ExternalMemoryHandleImportInfo;

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

    typedef struct KHR8BitStorageFeatures
    {
        bool storage_buffer_8_bit_access;
        bool storage_push_constant_8;
        bool uniform_and_storage_buffer_8_bit_access;

        KHR8BitStorageFeatures();
        KHR8BitStorageFeatures(const VkPhysicalDevice8BitStorageFeaturesKHR& in_features);

        VkPhysicalDevice8BitStorageFeaturesKHR get_vk_physical_device_8_bit_storage_features() const;

        bool operator==(const KHR8BitStorageFeatures& in_features) const;
    } KHR8BitStorageFeatures;

    typedef struct KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties
    {
        uint8_t  device_luid[VK_LUID_SIZE];
        bool     device_luid_valid;

        uint8_t  device_uuid[VK_UUID_SIZE];
        uint8_t  driver_uuid[VK_UUID_SIZE];

        uint32_t device_node_mask;

        KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties();
        KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties(const VkPhysicalDeviceIDPropertiesKHR& in_properties);

        bool operator==(const KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties& in_props) const;

    } KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties;

    typedef struct KHRMaintenance2PhysicalDevicePointClippingProperties
    {
        PointClippingBehavior point_clipping_behavior;

        KHRMaintenance2PhysicalDevicePointClippingProperties();
        KHRMaintenance2PhysicalDevicePointClippingProperties(const VkPhysicalDevicePointClippingPropertiesKHR& in_props);

        bool operator==(const KHRMaintenance2PhysicalDevicePointClippingProperties&) const;

    } KHRMaintenance2PhysicalDevicePointClippingProperties;

    typedef struct KHRMaintenance3Properties
    {
        VkDeviceSize max_memory_allocation_size;
        uint32_t     max_per_set_descriptors;

        KHRMaintenance3Properties();
        KHRMaintenance3Properties(const VkPhysicalDeviceMaintenance3PropertiesKHR& in_props);

        bool operator==(const KHRMaintenance3Properties&) const;

    } KHRMaintenance3Properties;

    typedef struct KHRMultiviewFeatures
    {
        bool multiview;
        bool multiview_geometry_shader;
        bool multiview_tessellation_shader;

        KHRMultiviewFeatures();
        KHRMultiviewFeatures(const VkPhysicalDeviceMultiviewFeatures& in_features);

        VkPhysicalDeviceMultiviewFeatures get_vk_physical_device_multiview_features() const;

        bool operator==(const KHRMultiviewFeatures& in_features) const;
    } KHRMultiviewFeatures;

    typedef struct KHRMultiviewProperties
    {
        uint32_t max_multiview_view_count;
        uint32_t max_multiview_instance_index;

        KHRMultiviewProperties();
        KHRMultiviewProperties(const VkPhysicalDeviceMultiviewPropertiesKHR& in_props);

        bool operator==(const KHRMultiviewProperties&) const;

    } KHRMultiviewProperties;

    typedef struct KHRVariablePointerFeatures
    {
        bool variable_pointers;
        bool variable_pointers_storage_buffer;

        KHRVariablePointerFeatures();
        KHRVariablePointerFeatures(const VkPhysicalDeviceVariablePointerFeatures& in_features);

        VkPhysicalDeviceVariablePointerFeaturesKHR get_vk_physical_device_variable_pointer_features() const;

        bool operator==(const KHRVariablePointerFeatures& in_features) const;
    } KHRVariablePointerFeatures;

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
        Anvil::AccessFlags destination_access_mask;
        Anvil::AccessFlags source_access_mask;

        VkMemoryBarrier memory_barrier_vk;

        /** Constructor.
         *
         *  @param in_source_access_mask      Source access mask of the Vulkan memory barrier.
         *  @param in_destination_access_mask Destination access mask of the Vulkan memory barrier.
         *
         **/
        explicit MemoryBarrier(Anvil::AccessFlags in_destination_access_mask,
                               Anvil::AccessFlags in_source_access_mask);

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
        Anvil::MemoryHeapFlags flags;

        uint32_t     index;
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

        Anvil::MemoryPropertyFlags flags;

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

    typedef struct MultisamplePropertiesEXT
    {
        VkExtent2D max_sample_location_grid_size;

        MultisamplePropertiesEXT()
        {
            max_sample_location_grid_size.height = 0;
            max_sample_location_grid_size.width  = 0;
        }

        MultisamplePropertiesEXT(const VkMultisamplePropertiesEXT& in_multisample_props_vk)
        {
            max_sample_location_grid_size = in_multisample_props_vk.maxSampleLocationGridSize;
        }
    } MultisamplePropertiesEXT;

    /** Defines data for a single image mip-map.
     *
     *  Use one of the static create_() functions to set up structure fields according to the target
     *  image type.
     **/
    typedef struct MipmapRawData
    {
        /* Image aspect the mip-map data is specified for. */
        Anvil::ImageAspectFlagBits aspect;

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
        static MipmapRawData create_1D_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                             uint32_t                                     in_n_mipmap,
                                                             std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_1D_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                             uint32_t                                     in_n_mipmap,
                                                             const unsigned char*                         in_linear_tightly_packed_data_vector_ptr,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_1D_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
        static MipmapRawData create_1D_array_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_layers,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_row_size,
                                                                   uint32_t                                     in_data_size);
        static MipmapRawData create_1D_array_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_layers,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_row_size,
                                                                   uint32_t                                     in_data_size);
        static MipmapRawData create_1D_array_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
        static MipmapRawData create_2D_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                             uint32_t                                     in_n_mipmap,
                                                             std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_data_size,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_2D_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                             uint32_t                                     in_n_mipmap,
                                                             const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_data_size,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_2D_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
        static MipmapRawData create_2D_array_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_layers,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_data_size,
                                                                   uint32_t                                     in_row_size);
        static MipmapRawData create_2D_array_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_layers,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_data_size,
                                                                   uint32_t                                     in_row_size);
        static MipmapRawData create_2D_array_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
        static MipmapRawData create_3D_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                             uint32_t                                     in_n_layer,
                                                             uint32_t                                     in_n_layer_slices,
                                                             uint32_t                                     in_n_mipmap,
                                                             std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_slice_data_size,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_3D_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                             uint32_t                                     in_n_layer,
                                                             uint32_t                                     in_n_layer_slices,
                                                             uint32_t                                     in_n_mipmap,
                                                             const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                             uint32_t                                     in_slice_data_size,
                                                             uint32_t                                     in_row_size);
        static MipmapRawData create_3D_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
        static MipmapRawData create_cube_map_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_data_size,
                                                                   uint32_t                                     in_row_size);
        static MipmapRawData create_cube_map_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                                   uint32_t                                     in_n_layer,
                                                                   uint32_t                                     in_n_mipmap,
                                                                   const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                                   uint32_t                                     in_data_size,
                                                                   uint32_t                                     in_row_size);
        static MipmapRawData create_cube_map_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
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
        static MipmapRawData create_cube_map_array_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                                         uint32_t                                     in_n_layer,
                                                                         uint32_t                                     in_n_layers,
                                                                         uint32_t                                     in_n_mipmap,
                                                                         std::shared_ptr<unsigned char>               in_linear_tightly_packed_data_ptr,
                                                                         uint32_t                                     in_data_size,
                                                                         uint32_t                                     in_row_size);
        static MipmapRawData create_cube_map_array_from_uchar_ptr       (Anvil::ImageAspectFlagBits                   in_aspect,
                                                                         uint32_t                                     in_n_layer,
                                                                         uint32_t                                     in_n_layers,
                                                                         uint32_t                                     in_n_mipmap,
                                                                         const unsigned char*                         in_linear_tightly_packed_data_ptr,
                                                                         uint32_t                                     in_data_size,
                                                                         uint32_t                                     in_row_size);
        static MipmapRawData create_cube_map_array_from_uchar_vector_ptr(Anvil::ImageAspectFlagBits                   in_aspect,
                                                                         uint32_t                                     in_n_layer,
                                                                         uint32_t                                     in_n_layers,
                                                                         uint32_t                                     in_n_mipmap,
                                                                         std::shared_ptr<std::vector<unsigned char> > in_linear_tightly_packed_data_ptr,
                                                                         uint32_t                                     in_data_size,
                                                                         uint32_t                                     in_row_size);

    private:
        static MipmapRawData create_1D      (Anvil::ImageAspectFlagBits in_aspect,
                                             uint32_t                   in_n_mipmap,
                                             uint32_t                   in_row_size);
        static MipmapRawData create_1D_array(Anvil::ImageAspectFlagBits in_aspect,
                                             uint32_t                   in_n_layer,
                                             uint32_t                   in_n_layers,
                                             uint32_t                   in_n_mipmap,
                                             uint32_t                   in_row_size,
                                             uint32_t                   in_data_size);
        static MipmapRawData create_2D      (Anvil::ImageAspectFlagBits in_aspect,
                                             uint32_t                   in_n_mipmap,
                                             uint32_t                   in_data_size,
                                             uint32_t                   in_row_size);
        static MipmapRawData create_2D_array(Anvil::ImageAspectFlagBits in_aspect,
                                             uint32_t                   in_n_layer,
                                             uint32_t                   in_n_layers,
                                             uint32_t                   in_n_mipmap,
                                             uint32_t                   in_data_size,
                                             uint32_t                   in_row_size);
        static MipmapRawData create_3D      (Anvil::ImageAspectFlagBits in_aspect,
                                             uint32_t                   in_n_layer,
                                             uint32_t                   in_n_slices,
                                             uint32_t                   in_n_mipmap,
                                             uint32_t                   in_data_size,
                                             uint32_t                   in_row_size);
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
        const KHR8BitStorageFeatures*         khr_8bit_storage_features_ptr;
        const KHRMultiviewFeatures*           khr_multiview_features_ptr;
        const KHRVariablePointerFeatures*     khr_variable_pointer_features_ptr;

        PhysicalDeviceFeatures();
        PhysicalDeviceFeatures(const PhysicalDeviceFeaturesCoreVK10* in_core_vk1_0_features_ptr,
                               const EXTDescriptorIndexingFeatures*  in_ext_descriptor_indexing_features_ptr,
                               const KHR16BitStorageFeatures*        in_khr_16_bit_storage_features_ptr,
                               const KHR8BitStorageFeatures*         in_khr_8_bit_storage_features_ptr,
                               const KHRMultiviewFeatures*           in_khr_multiview_features_ptr,
                               const KHRVariablePointerFeatures*     in_khr_variable_pointer_features_ptr);

        bool operator==(const PhysicalDeviceFeatures& in_physical_device_features) const;
    } PhysicalDeviceFeatures;

    typedef struct PhysicalDeviceLimits
    {
        VkDeviceSize            buffer_image_granularity;
        uint32_t                discrete_queue_priorities;
        Anvil::SampleCountFlags framebuffer_color_sample_counts;
        Anvil::SampleCountFlags framebuffer_depth_sample_counts;
        Anvil::SampleCountFlags framebuffer_no_attachments_sample_counts;
        Anvil::SampleCountFlags framebuffer_stencil_sample_counts;
        float                   line_width_granularity;
        float                   line_width_range[2];
        uint32_t                max_bound_descriptor_sets;
        uint32_t                max_clip_distances;
        uint32_t                max_color_attachments;
        uint32_t                max_combined_clip_and_cull_distances;
        uint32_t                max_compute_shared_memory_size;
        uint32_t                max_compute_work_group_count[3];
        uint32_t                max_compute_work_group_invocations;
        uint32_t                max_compute_work_group_size[3];
        uint32_t                max_cull_distances;
        uint32_t                max_descriptor_set_input_attachments;
        uint32_t                max_descriptor_set_sampled_images;
        uint32_t                max_descriptor_set_samplers;
        uint32_t                max_descriptor_set_storage_buffers;
        uint32_t                max_descriptor_set_storage_buffers_dynamic;
        uint32_t                max_descriptor_set_storage_images;
        uint32_t                max_descriptor_set_uniform_buffers;
        uint32_t                max_descriptor_set_uniform_buffers_dynamic;
        uint32_t                max_draw_indexed_index_value;
        uint32_t                max_draw_indirect_count;
        uint32_t                max_fragment_combined_output_resources;
        uint32_t                max_fragment_dual_src_attachments;
        uint32_t                max_fragment_input_components;
        uint32_t                max_fragment_output_attachments;
        uint32_t                max_framebuffer_height;
        uint32_t                max_framebuffer_layers;
        uint32_t                max_framebuffer_width;
        uint32_t                max_geometry_input_components;
        uint32_t                max_geometry_output_components;
        uint32_t                max_geometry_output_vertices;
        uint32_t                max_geometry_shader_invocations;
        uint32_t                max_geometry_total_output_components;
        uint32_t                max_image_array_layers;
        uint32_t                max_image_dimension_1D;
        uint32_t                max_image_dimension_2D;
        uint32_t                max_image_dimension_3D;
        uint32_t                max_image_dimension_cube;
        float                   max_interpolation_offset;
        uint32_t                max_memory_allocation_count;
        uint32_t                max_per_stage_descriptor_input_attachments;
        uint32_t                max_per_stage_descriptor_sampled_images;
        uint32_t                max_per_stage_descriptor_samplers;
        uint32_t                max_per_stage_descriptor_storage_buffers;
        uint32_t                max_per_stage_descriptor_storage_images;
        uint32_t                max_per_stage_descriptor_uniform_buffers;
        uint32_t                max_per_stage_resources;
        uint32_t                max_push_constants_size;
        uint32_t                max_sample_mask_words;
        uint32_t                max_sampler_allocation_count;
        float                   max_sampler_anisotropy;
        float                   max_sampler_lod_bias;
        uint32_t                max_storage_buffer_range;
        uint32_t                max_viewport_dimensions[2];
        uint32_t                max_viewports;
        uint32_t                max_tessellation_control_per_patch_output_components;
        uint32_t                max_tessellation_control_per_vertex_input_components;
        uint32_t                max_tessellation_control_per_vertex_output_components;
        uint32_t                max_tessellation_control_total_output_components;
        uint32_t                max_tessellation_evaluation_input_components;
        uint32_t                max_tessellation_evaluation_output_components;
        uint32_t                max_tessellation_generation_level;
        uint32_t                max_tessellation_patch_size;
        uint32_t                max_texel_buffer_elements;
        uint32_t                max_texel_gather_offset;
        uint32_t                max_texel_offset;
        uint32_t                max_uniform_buffer_range;
        uint32_t                max_vertex_input_attributes;
        uint32_t                max_vertex_input_attribute_offset;
        uint32_t                max_vertex_input_bindings;
        uint32_t                max_vertex_input_binding_stride;
        uint32_t                max_vertex_output_components;
        float                   min_interpolation_offset;
        size_t                  min_memory_map_alignment;
        VkDeviceSize            min_storage_buffer_offset_alignment;
        VkDeviceSize            min_texel_buffer_offset_alignment;
        int32_t                 min_texel_gather_offset;
        int32_t                 min_texel_offset;
        VkDeviceSize            min_uniform_buffer_offset_alignment;
        uint32_t                mipmap_precision_bits;
        VkDeviceSize            non_coherent_atom_size;
        VkDeviceSize            optimal_buffer_copy_offset_alignment;
        VkDeviceSize            optimal_buffer_copy_row_pitch_alignment;
        float                   point_size_granularity;
        float                   point_size_range[2];
        Anvil::SampleCountFlags sampled_image_color_sample_counts;
        Anvil::SampleCountFlags sampled_image_depth_sample_counts;
        Anvil::SampleCountFlags sampled_image_integer_sample_counts;
        Anvil::SampleCountFlags sampled_image_stencil_sample_counts;
        VkDeviceSize            sparse_address_space_size;
        bool                    standard_sample_locations;
        Anvil::SampleCountFlags storage_image_sample_counts;
        bool                    strict_lines;
        uint32_t                sub_pixel_interpolation_offset_bits;
        uint32_t                sub_pixel_precision_bits;
        uint32_t                sub_texel_precision_bits;
        bool                    timestamp_compute_and_graphics;
        float                   timestamp_period;
        float                   viewport_bounds_range[2];
        uint32_t                viewport_sub_pixel_bits;

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
        const EXTExternalMemoryHostProperties*                         ext_external_memory_host_properties_ptr;
        const EXTSampleLocationsProperties*                            ext_sample_locations_properties_ptr;
        const EXTSamplerFilterMinmaxProperties*                        ext_sampler_filter_minmax_properties_ptr;
        const EXTVertexAttributeDivisorProperties*                     ext_vertex_attribute_divisor_properties_ptr;
        const KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties* khr_external_memory_capabilities_physical_device_id_properties_ptr;
        const KHRMaintenance2PhysicalDevicePointClippingProperties*    khr_maintenance2_point_clipping_properties_ptr;
        const KHRMaintenance3Properties*                               khr_maintenance3_properties_ptr;
        const KHRMultiviewProperties*                                  khr_multiview_properties_ptr;

        PhysicalDeviceProperties();
        PhysicalDeviceProperties(const AMDShaderCoreProperties*                                 in_amd_shader_core_properties_ptr,
                                 const PhysicalDevicePropertiesCoreVK10*                        in_core_vk1_0_properties_ptr,
                                 const EXTDescriptorIndexingProperties*                         in_ext_descriptor_indexing_properties_ptr,
                                 const EXTExternalMemoryHostProperties*                         in_ext_external_memory_host_properties_ptr,
                                 const EXTSampleLocationsProperties*                            in_ext_sample_locations_properties_ptr,
                                 const EXTSamplerFilterMinmaxProperties*                        in_ext_sampler_filter_minmax_properties_ptr,
                                 const EXTVertexAttributeDivisorProperties*                     in_ext_vertex_attribute_divisor_properties_ptr,
                                 const KHRExternalMemoryCapabilitiesPhysicalDeviceIDProperties* in_khr_external_memory_caps_physical_device_id_props_ptr,
                                 const KHRMaintenance3Properties*                               in_khr_maintenance3_properties_ptr,
                                 const KHRMaintenance2PhysicalDevicePointClippingProperties*    in_khr_maintenance2_point_clipping_properties_ptr,
                                 const KHRMultiviewProperties*                                  in_khr_multiview_properties_ptr);

        bool operator==(const PhysicalDeviceProperties& in_props) const;
    } PhysicalDeviceProperties;

    /* A single push constant range descriptor */
    typedef struct PushConstantRange
    {
        uint32_t                offset;
        uint32_t                size;
        Anvil::ShaderStageFlags stages;

        /** Constructor
         *
         *  @param in_offset Start offset for the range.
         *  @param in_size   Size of the range.
         *  @param in_stages Valid pipeline stages for the range.
         */
        PushConstantRange(uint32_t                in_offset,
                          uint32_t                in_size,
                          Anvil::ShaderStageFlags in_stages);

        /** Comparison operator. Used internally. */
        bool operator==(const PushConstantRange& in) const;
    } PushConstantRange;

    typedef std::vector<PushConstantRange> PushConstantRanges;

    /** Holds information about a single Vulkan Queue Family. */
    typedef struct QueueFamilyInfo
    {
        Anvil::QueueFlags flags;

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

    /** TODO */
    typedef struct SemaphoreMGPUSubmission
    {
        uint32_t            device_index; //< device index in the device group
        Anvil::Semaphore*   semaphore_ptr;

        SemaphoreMGPUSubmission()
        {
            device_index    = UINT32_MAX;
            semaphore_ptr   = nullptr;
        }
    } SemaphoreMGPUSubmission;

    typedef struct SemaphoreProperties
    {
        ExternalSemaphoreProperties external_semaphore_properties;

        /* Dummy constructor */
        SemaphoreProperties();

        SemaphoreProperties(const ExternalSemaphoreProperties& in_external_semaphore_properties);
    } SemaphoreProperties;

    typedef struct SemaphorePropertiesQuery
    {
        const Anvil::ExternalSemaphoreHandleTypeFlagBits external_semaphore_handle_type;

        explicit SemaphorePropertiesQuery(const Anvil::ExternalSemaphoreHandleTypeFlagBits& in_external_semaphore_handle_type)
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

    typedef struct SparseImageFormatProperties
    {
        Anvil::ImageAspectFlags       aspect_mask;
        VkExtent3D                    image_granularity;
        Anvil::SparseImageFormatFlags flags;

        SparseImageFormatProperties()
        {
            aspect_mask              = Anvil::ImageAspectFlagBits::NONE;
            image_granularity.depth  = 0;
            image_granularity.height = 0;
            image_granularity.width  = 0;
            flags                    = Anvil::SparseImageFormatFlagBits::NONE;
        }

        SparseImageFormatProperties(const VkSparseImageFormatProperties& in_props)
        {
            aspect_mask       = static_cast<Anvil::ImageAspectFlagBits>(in_props.aspectMask);
            image_granularity = in_props.imageGranularity;
            flags             = static_cast<Anvil::SparseImageFormatFlagBits>(in_props.flags);
        }
    } SparseImageFormatProperties;

    static_assert(sizeof(SparseImageFormatProperties)                      == sizeof(VkSparseImageFormatProperties),                     "Struct sizes must match");
    static_assert(offsetof(SparseImageFormatProperties, aspect_mask)       == offsetof(VkSparseImageFormatProperties, aspectMask),       "Member offsets must match");
    static_assert(offsetof(SparseImageFormatProperties, image_granularity) == offsetof(VkSparseImageFormatProperties, imageGranularity), "Member offsets must match");
    static_assert(offsetof(SparseImageFormatProperties, flags)             == offsetof(VkSparseImageFormatProperties, flags),            "Member offsets must match");

    typedef struct SparseImageMemoryRequirements
    {
        Anvil::SparseImageFormatProperties format_properties;
        uint32_t                           image_mip_tail_first_lod;
        VkDeviceSize                       image_mip_tail_size;
        VkDeviceSize                       image_mip_tail_offset;
        VkDeviceSize                       image_mip_tail_stride;

        SparseImageMemoryRequirements()
        {
            image_mip_tail_first_lod = 0;
            image_mip_tail_size      = 0;
            image_mip_tail_offset    = 0;
            image_mip_tail_stride    = 0;
        }

        SparseImageMemoryRequirements(const VkSparseImageMemoryRequirements& in_reqs)
        {
            format_properties        = in_reqs.formatProperties;
            image_mip_tail_first_lod = in_reqs.imageMipTailFirstLod;
            image_mip_tail_size      = in_reqs.imageMipTailSize;
            image_mip_tail_offset    = in_reqs.imageMipTailOffset;
            image_mip_tail_stride    = in_reqs.imageMipTailStride;
        }
    } SparseImageMemoryRequirements;

    static_assert(sizeof(SparseImageMemoryRequirements)                             == sizeof(VkSparseImageMemoryRequirements),                         "Struct sizes must match");
    static_assert(offsetof(SparseImageMemoryRequirements, format_properties)        == offsetof(VkSparseImageMemoryRequirements, formatProperties),     "Member offsets must match");
    static_assert(offsetof(SparseImageMemoryRequirements, image_mip_tail_first_lod) == offsetof(VkSparseImageMemoryRequirements, imageMipTailFirstLod), "Member offsets must match");
    static_assert(offsetof(SparseImageMemoryRequirements, image_mip_tail_size)      == offsetof(VkSparseImageMemoryRequirements, imageMipTailSize),     "Member offsets must match");
    static_assert(offsetof(SparseImageMemoryRequirements, image_mip_tail_offset)    == offsetof(VkSparseImageMemoryRequirements, imageMipTailOffset),   "Member offsets must match");
    static_assert(offsetof(SparseImageMemoryRequirements, image_mip_tail_stride)    == offsetof(VkSparseImageMemoryRequirements, imageMipTailStride),   "Member offsets must match");

    /* Describes sparse properties for an image format */
    typedef struct SparseImageAspectProperties
    {
        Anvil::ImageAspectFlags aspect_mask;

        SparseImageFormatFlags flags;

        VkExtent3D   granularity;
        uint32_t     mip_tail_first_lod;
        VkDeviceSize mip_tail_offset;
        VkDeviceSize mip_tail_size;
        VkDeviceSize mip_tail_stride;

        SparseImageAspectProperties();
        SparseImageAspectProperties(const Anvil::SparseImageMemoryRequirements& in_req);
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
        MGPU,
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
         *  The prototype which does not take *MGPUSubmission* may be used for both single- and multi-GPU device instances.
         *  The other one must only be called for multi-GPU device instances.
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
                                 const Anvil::PipelineStageFlags* in_opt_dst_stage_masks_to_wait_on_ptrs,
                                 bool                             in_should_block,
                                 Anvil::Fence*                    in_opt_fence_ptr = nullptr);
        static SubmitInfo create(uint32_t                         in_n_cmd_buffers,
                                 Anvil::CommandBufferBase* const* in_opt_cmd_buffer_ptrs_ptr,
                                 uint32_t                         in_n_semaphores_to_signal,
                                 Anvil::Semaphore* const*         in_opt_semaphore_to_signal_ptrs_ptr,
                                 uint32_t                         in_n_semaphores_to_wait_on,
                                 Anvil::Semaphore* const*         in_opt_semaphore_to_wait_on_ptrs_ptr,
                                 const Anvil::PipelineStageFlags* in_opt_dst_stage_masks_to_wait_on_ptrs,
                                 bool                             in_should_block,
                                 Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        static SubmitInfo create_execute(Anvil::CommandBufferBase*        in_cmd_buffer_ptr,
                                         bool                             in_should_block,
                                         Anvil::Fence*                    in_opt_fence_ptr = nullptr);
        static SubmitInfo create_execute(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                         uint32_t                         in_n_cmd_buffers,
                                         bool                             in_should_block,
                                         Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        static SubmitInfo create_execute(const CommandBufferMGPUSubmission* in_cmd_buffer_submissions_ptr,
                                         uint32_t                           in_n_command_buffer_submissions,
                                         bool                               in_should_block,
                                         Anvil::Fence*                      in_opt_fence_ptr = nullptr);

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

        static SubmitInfo create_execute_signal(const CommandBufferMGPUSubmission* in_cmd_buffer_submissions_ptr,
                                                uint32_t                           in_n_command_buffer_submissions,
                                                uint32_t                           in_n_signal_semaphore_submissions,
                                                const SemaphoreMGPUSubmission*     in_signal_semaphore_submissions_ptr,
                                                bool                               in_should_block,
                                                Anvil::Fence*                      in_opt_fence_ptr = nullptr);

        /** TODO
         *
         *  NOTE: This function always blocks.
         */
        static SubmitInfo create_signal(uint32_t                 in_n_semaphores_to_signal,
                                        Anvil::Semaphore* const* in_semaphore_to_signal_ptrs_ptr,
                                        Anvil::Fence*            in_opt_fence_ptr = nullptr);

        static SubmitInfo create_signal_wait(uint32_t                         in_n_semaphores_to_signal,
                                             Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                             uint32_t                         in_n_semaphores_to_wait_on,
                                             Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                             const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                             bool                             in_should_block,
                                             Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        /** TODO
         *
         *  NOTE: This function always blocks.
         */
        static SubmitInfo create_wait(uint32_t                         in_n_semaphores_to_wait_on,
                                      Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                      const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                      Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        static SubmitInfo create_wait_execute(Anvil::CommandBufferBase*        in_cmd_buffer_ptr,
                                              uint32_t                         in_n_semaphores_to_wait_on,
                                              Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                              const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                              bool                             in_should_block,
                                              Anvil::Fence*                    in_opt_fence_ptr = nullptr);
        static SubmitInfo create_wait_execute(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                              uint32_t                         in_n_cmd_buffers,
                                              uint32_t                         in_n_semaphores_to_wait_on,
                                              Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                              const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                              bool                             in_should_block,
                                              Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        static SubmitInfo create_wait_execute(const CommandBufferMGPUSubmission* in_cmd_buffer_submissions_ptr,
                                              uint32_t                           in_n_command_buffer_submissions,
                                              uint32_t                           in_n_wait_semaphore_submissions,
                                              const SemaphoreMGPUSubmission*     in_wait_semaphore_submissions_ptr,
                                              const Anvil::PipelineStageFlags*   in_dst_stage_masks_to_wait_on_ptrs,
                                              bool                               in_should_block,
                                              Anvil::Fence*                      in_opt_fence_ptr = nullptr);

        static SubmitInfo create_wait_execute_signal(Anvil::CommandBufferBase*        in_cmd_buffer_ptr,
                                                     uint32_t                         in_n_semaphores_to_signal,
                                                     Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                                     uint32_t                         in_n_semaphores_to_wait_on,
                                                     Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                                     const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                                     bool                             in_should_block,
                                                     Anvil::Fence*                    in_opt_fence_ptr = nullptr);
        static SubmitInfo create_wait_execute_signal(Anvil::CommandBufferBase* const* in_cmd_buffer_ptrs_ptr,
                                                     uint32_t                         in_n_cmd_buffers,
                                                     uint32_t                         in_n_semaphores_to_signal,
                                                     Anvil::Semaphore* const*         in_semaphore_to_signal_ptrs_ptr,
                                                     uint32_t                         in_n_semaphores_to_wait_on,
                                                     Anvil::Semaphore* const*         in_semaphore_to_wait_on_ptrs_ptr,
                                                     const Anvil::PipelineStageFlags* in_dst_stage_masks_to_wait_on_ptrs,
                                                     bool                             in_should_block,
                                                     Anvil::Fence*                    in_opt_fence_ptr = nullptr);

        static SubmitInfo create_wait_execute_signal(const CommandBufferMGPUSubmission* in_cmd_buffer_submissions_ptr,
                                                     uint32_t                           in_n_command_buffer_submissions,
                                                     uint32_t                           in_n_signal_semaphore_submissions,
                                                     const SemaphoreMGPUSubmission*     in_signal_semaphore_submissions_ptr,
                                                     uint32_t                           in_n_wait_semaphore_submissions,
                                                     const SemaphoreMGPUSubmission*     in_wait_semaphore_submissions_ptr,
                                                     const Anvil::PipelineStageFlags*   in_dst_stage_masks_to_wait_on_ptrs,
                                                     bool                               in_should_block,
                                                     Anvil::Fence*                      in_opt_fence_ptr = nullptr);

        const CommandBufferMGPUSubmission* get_command_buffers_mgpu() const
        {
            return command_buffers_mgpu_ptr;
        }

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
            return (dst_stage_wait_masks.size() > 0) ? &dst_stage_wait_masks.at(0) : nullptr;
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

        const Anvil::SemaphoreMGPUSubmission* get_signal_semaphores_mgpu() const
        {
            return signal_semaphores_mgpu_ptr;
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

        const Anvil::SemaphoreMGPUSubmission* get_wait_semaphores_mgpu() const
        {
            return wait_semaphores_mgpu_ptr;
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
        SubmitInfo(uint32_t                         in_n_command_buffers,
                   Anvil::CommandBufferBase*        in_opt_single_cmd_buffer_ptr, /* to support n=1 helper functions */
                   Anvil::CommandBufferBase* const* in_opt_cmd_buffer_ptrs_ptr,
                   uint32_t                         in_n_semaphores_to_signal,
                   Anvil::Semaphore* const*         in_opt_semaphore_to_signal_ptrs_ptr,
                   uint32_t                         in_n_semaphores_to_wait_on,
                   Anvil::Semaphore* const*         in_opt_semaphore_to_wait_on_ptrs_ptr,
                   const Anvil::PipelineStageFlags* in_opt_dst_stage_masks_to_wait_on_ptrs,
                   bool                             in_should_block,
                   Anvil::Fence*                    in_opt_fence_ptr);

        SubmitInfo(uint32_t                           in_n_command_buffer_submissions,
                   const CommandBufferMGPUSubmission* in_opt_command_buffer_submissions_ptr,
                   uint32_t                           in_n_signal_semaphore_submissions,
                   const SemaphoreMGPUSubmission*     in_opt_signal_semaphore_submissions_ptr,
                   uint32_t                           in_n_wait_semaphore_submissions,
                   const SemaphoreMGPUSubmission*     in_opt_wait_semaphore_submissions_ptr,
                   const Anvil::PipelineStageFlags*   in_opt_dst_stage_masks_to_wait_on_ptr,
                   bool                               in_should_block,
                   Anvil::Fence*                      in_opt_fence_ptr);

        Anvil::CommandBufferBase* helper_cmd_buffer_raw_ptr;

        const CommandBufferMGPUSubmission* command_buffers_mgpu_ptr;
        Anvil::CommandBufferBase* const*   command_buffers_sgpu_ptr;
        uint32_t                           n_command_buffers;

        const SemaphoreMGPUSubmission* signal_semaphores_mgpu_ptr;
        Anvil::Semaphore* const*       signal_semaphores_sgpu_ptr;
        uint32_t                       n_signal_semaphores;

        std::vector<VkPipelineStageFlags> dst_stage_wait_masks;
        const SemaphoreMGPUSubmission*    wait_semaphores_mgpu_ptr;
        Anvil::Semaphore* const*          wait_semaphores_sgpu_ptr;
        uint32_t                          n_wait_semaphores;

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

    /* NOTE: Maps 1:1 to VkSurfaceFormatKHR */
    typedef struct SurfaceFormatKHR
    {
        Anvil::Format        format;
        Anvil::ColorSpaceKHR color_space;

        SurfaceFormatKHR()
        {
            color_space = Anvil::ColorSpaceKHR::UNKNOWN;
            format      = Anvil::Format::UNKNOWN;
        }
    } SurfaceFormatKHR;

    static_assert(sizeof  (SurfaceFormatKHR)              == sizeof  (VkSurfaceFormatKHR),             "Struct sizes must match");
    static_assert(offsetof(SurfaceFormatKHR, format)      == offsetof(VkSurfaceFormatKHR, format),     "Member offsets must match");
    static_assert(offsetof(SurfaceFormatKHR, color_space) == offsetof(VkSurfaceFormatKHR, colorSpace), "Member offsets must match");

    /* Represents a Vulkan structure header */
    typedef struct
    {
        VkStructureType type;
        const void*     next_ptr;
    } VkStructHeader;

}; /* namespace Anvil */
#endif
