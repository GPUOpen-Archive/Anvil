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
#ifndef TYPES_ENUMS_H
#define TYPES_ENUMS_H

namespace Anvil
{
    /* Describes recognized subpass attachment types */
    enum AttachmentType
    {
        ATTACHMENT_TYPE_FIRST,

        ATTACHMENT_TYPE_COLOR = ATTACHMENT_TYPE_FIRST,
        ATTACHMENT_TYPE_DEPTH_STENCIL,
        ATTACHMENT_TYPE_INPUT,
        ATTACHMENT_TYPE_PRESERVE,
        ATTACHMENT_TYPE_RESOLVE,

        ATTACHMENT_TYPE_COUNT,
        ATTACHMENT_TYPE_UNKNOWN = ATTACHMENT_TYPE_COUNT
    };

    enum BufferCreateFlagBits
    {
        BUFFER_CREATE_FLAG_SPARSE_BINDING_BIT   = 1 << 0,
        BUFFER_CREATE_FLAG_SPARSE_RESIDENCY_BIT = 1 << 1,
        BUFFER_CREATE_FLAG_SPARSE_ALIASED_BIT   = 1 << 2,
    };
    typedef uint32_t BufferCreateFlags;

    typedef enum class BufferType
    {
        NONSPARSE_ALLOC,
        NONSPARSE_NO_ALLOC,
        NONSPARSE_NO_ALLOC_CHILD,
        SPARSE_NO_ALLOC,
    } BufferType;

    typedef enum
    {
        DYNAMIC_STATE_BLEND_CONSTANTS_BIT      = 1 << 0,
        DYNAMIC_STATE_DEPTH_BIAS_BIT           = 1 << 1,
        DYNAMIC_STATE_DEPTH_BOUNDS_BIT         = 1 << 2,
        DYNAMIC_STATE_LINE_WIDTH_BIT           = 1 << 3,
        DYNAMIC_STATE_SCISSOR_BIT              = 1 << 4,
        DYNAMIC_STATE_STENCIL_COMPARE_MASK_BIT = 1 << 5,
        DYNAMIC_STATE_STENCIL_REFERENCE_BIT    = 1 << 6,
        DYNAMIC_STATE_STENCIL_WRITE_MASK_BIT   = 1 << 7,
        DYNAMIC_STATE_VIEWPORT_BIT             = 1 << 8,
    } DynamicStateBits;
    typedef uint32_t DynamicStateBitfield;

    typedef enum
    {
        EXTERNAL_FENCE_HANDLE_TYPE_NONE = 0,

        #if defined(_WIN32)
            EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT     = 1 << 0,
            EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT = 1 << 1,
        #else
            EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT = 1 << 2,
            EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT   = 1 << 2,
        #endif

    } ExternalFenceHandleTypeBit;
    typedef uint32_t ExternalFenceHandleTypeBits;

    typedef enum
    {
        EXTERNAL_MEMORY_HANDLE_TYPE_NONE = 0,

        #if defined(_WIN32)
            EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT      = 1 << 0,
            EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT  = 1 << 1,
            EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT     = 1 << 2,
            EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT = 1 << 3,
            EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT        = 1 << 4,
            EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT    = 1 << 5,
        #else
            EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT = 1 << 6,
        #endif

    } ExternalMemoryHandleTypeBit;
    typedef uint32_t ExternalMemoryHandleTypeBits;

    typedef enum
    {
        EXTERNAL_SEMAPHORE_HANDLE_TYPE_NONE = 0,

        #if defined(_WIN32)
            EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT     = 1 << 0,
            EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT = 1 << 1,
            EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT      = 1 << 2,
        #else
            EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT = 1 << 3,
            EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT   = 1 << 4,
        #endif

    } ExternalSemaphoreHandleTypeBit;
    typedef uint32_t ExternalSemaphoreHandleTypeBits;

    /** Describes component layout of a format */
    typedef enum
    {
        /* NOTE: If the ordering used below needs to be changed, make sure to also update formats.cpp::layout_to_n_components */
        COMPONENT_LAYOUT_ABGR,
        COMPONENT_LAYOUT_ARGB,
        COMPONENT_LAYOUT_BGR,
        COMPONENT_LAYOUT_BGRA,
        COMPONENT_LAYOUT_D,
        COMPONENT_LAYOUT_DS,
        COMPONENT_LAYOUT_EBGR,
        COMPONENT_LAYOUT_R,
        COMPONENT_LAYOUT_RG,
        COMPONENT_LAYOUT_RGB,
        COMPONENT_LAYOUT_RGBA,
        COMPONENT_LAYOUT_S,
        COMPONENT_LAYOUT_XD,

        COMPONENT_LAYOUT_UNKNOWN
    } ComponentLayout;

    typedef enum
    {
        /* When specified for a binding, the binding can be modified after having been bound to a pipeline
         * in a command buffer, without invalidating that command buffer.
         * The updated binding becomes visible to following submissions as soon as the update function leaves.
         *
         * Requires VK_EXT_descriptor_indexing.
         */
        DESCRIPTOR_BINDING_FLAG_UPDATE_AFTER_BIND_BIT = 1 << 0,

        /* When specified for a binding, the binding can be modified after having been bound to a pipeline
         * in a command buffer, as long as it is NOT used by the command buffer. Doing so no longer invalidyates
         * the command buffer.
         *
         * Requires VK_EXT_descriptor_indexing.
         */
        DESCRIPTOR_BINDING_FLAG_UPDATE_UNUSED_WHILE_PENDING_BIT = 1 << 1,

        /* When specified for a binding, the binding needs not be assigned valid descriptor(s), as long as none of
         * the shader invocations execute an instruction that performs any memory access using the descriptor.
         *
         * Requires VK_EXT_descriptor_indexing.
         */
        DESCRIPTOR_BINDING_FLAG_PARTIALLY_BOUND_BIT = 1 << 2,

        /* When specified for a binding, the binding gets a variable size which is specified each time a descriptor
         * set is allocated using this layout. The in_descriptor_array_size field specified at DescriptorSetCreateInfo::add_binding()
         * call time acts as an upper bound for the number of elements the binding can handle.
         *
         * Can only be specified for the last binding in the DS layout.
         *
         * Requires VK_EXT_descriptor_indexing.
         */
        DESCRIPTOR_BINDING_FLAG_VARIABLE_DESCRIPTOR_COUNT_BIT = 1 << 3,

    } DescriptorBindingFlagBits;
    typedef uint32_t DescriptorBindingFlags;

    typedef enum
    {
        /* When set, descriptor set allocations will return back to the pool at release time.*/
        DESCRIPTOR_POOL_FLAG_CREATE_FREE_DESCRIPTOR_SET_BIT = 1 << 0,

        /* When set, descriptor sets allocated from this pool can be created with the DESCRIPTOR_BINDING_FLAG_UPDATE_AFTER_BIND_BIT flag.
         *
         * Requires VK_EXT_descriptor_indexing.
         **/
        DESCRIPTOR_POOL_FLAG_CREATE_UPDATE_AFTER_BIND_BIT = 1 << 1,

    } DescriptorPoolFlagBits;
    typedef uint32_t DescriptorPoolFlags;

    typedef enum
    {
        /* Updates dirty DS bindings using vkUpdateDescriptorSet() which is available on all Vulkan implementations. */
        DESCRIPTOR_SET_UPDATE_METHOD_CORE,

        /* Updates dirty DS bindings using vkUpdateDescriptorSetWithTemplateKHR(). Templates are cached across update operations,
         * and are release at DescriptorSet release time.
         *
         * This setting is recommended if you are going to be updating the same set of descriptor set bindings more than once.
         *
         * Only available on devices supporting VK_KHR_descriptor_update_template extension.
         */
        DESCRIPTOR_SET_UPDATE_METHOD_TEMPLATE,
    } DescriptorSetUpdateMethod;

    typedef enum
    {
        EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE,
        EXTENSION_AVAILABILITY_IGNORE,
        EXTENSION_AVAILABILITY_REQUIRE,
    } ExtensionAvailability;

    /** Tells the type of an Anvil::BaseDevice instance */
    typedef enum
    {
        /* BaseDevice is implemented by SGPUDevice class */
        DEVICE_TYPE_SINGLE_GPU,
    } DeviceType;

    typedef enum
    {
        FORMAT_TYPE_SFLOAT,
        FORMAT_TYPE_SFLOAT_UINT,
        FORMAT_TYPE_SINT,
        FORMAT_TYPE_SNORM,
        FORMAT_TYPE_SRGB,
        FORMAT_TYPE_SSCALED,
        FORMAT_TYPE_UFLOAT,
        FORMAT_TYPE_UINT,
        FORMAT_TYPE_UNORM,
        FORMAT_TYPE_UNORM_UINT,
        FORMAT_TYPE_USCALED,

        FORMAT_TYPE_UNKNOWN,
    } FormatType;

    enum ImageCreateFlagBits
    {
        IMAGE_CREATE_FLAG_MUTABLE_FORMAT_BIT      = 1 << 0,
        IMAGE_CREATE_FLAG_CUBE_COMPATIBLE_BIT     = 1 << 1,

        /* NOTE: Requires VK_KHR_maintenance1 */
        IMAGE_CREATE_FLAG_2D_ARRAY_COMPATIBLE_BIT = 1 << 2,
    };
    typedef uint32_t ImageCreateFlags;

    typedef enum class ImageType
    {
        NONSPARSE_ALLOC,
        NONSPARSE_NO_ALLOC,
        SPARSE_NO_ALLOC,
        SWAPCHAIN_WRAPPER
    } ImageType;

    typedef enum class MemoryBlockType
    {
        DERIVED,
        DERIVED_WITH_CUSTOM_DELETE_PROC,
        REGULAR

    } MemoryBlockType;

    enum MemoryFeatureFlagBits
    {
        /* NOTE: If more memory feature flags are added here, make sure to also update Anvil::Utils::get_vk_property_flags_from_memory_feature_flags()
         *       and Anvil::Utils::get_memory_feature_flags_from_vk_property_flags()
         */

        MEMORY_FEATURE_FLAG_DEVICE_LOCAL     = 1 << 0,
        MEMORY_FEATURE_FLAG_HOST_CACHED      = 1 << 1,
        MEMORY_FEATURE_FLAG_HOST_COHERENT    = 1 << 2,
        MEMORY_FEATURE_FLAG_LAZILY_ALLOCATED = 1 << 3,
        MEMORY_FEATURE_FLAG_MAPPABLE         = 1 << 4,
    };
    typedef uint32_t MemoryFeatureFlags;

    typedef enum
    {
        MT_SAFETY_INHERIT_FROM_PARENT_DEVICE,
        MT_SAFETY_ENABLED,
        MT_SAFETY_DISABLED
    } MTSafety;

    typedef enum
    {
        /* NOTE: If new entries are added or existing entry order is modified, make sure to
         *       update Anvil::ObjectTracker::get_object_type_name().
         */
        OBJECT_TYPE_FIRST,

        OBJECT_TYPE_BUFFER = OBJECT_TYPE_FIRST,
        OBJECT_TYPE_BUFFER_VIEW,
        OBJECT_TYPE_COMMAND_BUFFER,
        OBJECT_TYPE_COMMAND_POOL,
        OBJECT_TYPE_COMPUTE_PIPELINE_MANAGER,
        OBJECT_TYPE_DESCRIPTOR_POOL,
        OBJECT_TYPE_DESCRIPTOR_SET,
        OBJECT_TYPE_DESCRIPTOR_SET_GROUP,
        OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
        OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_MANAGER,
        OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE,
        OBJECT_TYPE_DEVICE,
        OBJECT_TYPE_EVENT,
        OBJECT_TYPE_FENCE,
        OBJECT_TYPE_FRAMEBUFFER,
        OBJECT_TYPE_GRAPHICS_PIPELINE_MANAGER,
        OBJECT_TYPE_IMAGE,
        OBJECT_TYPE_IMAGE_VIEW,
        OBJECT_TYPE_INSTANCE,
        OBJECT_TYPE_MEMORY_BLOCK,
        OBJECT_TYPE_PHYSICAL_DEVICE,
        OBJECT_TYPE_PIPELINE_CACHE,
        OBJECT_TYPE_PIPELINE_LAYOUT,
        OBJECT_TYPE_PIPELINE_LAYOUT_MANAGER,
        OBJECT_TYPE_QUERY_POOL,
        OBJECT_TYPE_QUEUE,
        OBJECT_TYPE_RENDER_PASS,
        OBJECT_TYPE_RENDERING_SURFACE,
        OBJECT_TYPE_SAMPLER,
        OBJECT_TYPE_SEMAPHORE,
        OBJECT_TYPE_SHADER_MODULE,
        OBJECT_TYPE_SWAPCHAIN,

        OBJECT_TYPE_GLSL_SHADER_TO_SPIRV_GENERATOR,
        OBJECT_TYPE_GRAPHICS_PIPELINE,

        /* Always last */
        OBJECT_TYPE_COUNT,
        OBJECT_TYPE_UNKNOWN = OBJECT_TYPE_COUNT
    } ObjectType;

    /** Defines, to what extent occlusion queries are going to be used.
     *
     *  Only used for second-level command buffer recording policy declaration.
     **/
    typedef enum
    {
        /** Occlusion queries are not going to be used */
        OCCLUSION_QUERY_SUPPORT_SCOPE_NOT_REQUIRED,

        /** Non-precise occlusion queries may be used */
        OCCLUSION_QUERY_SUPPORT_SCOPE_REQUIRED_NONPRECISE,

        /** Pprecise occlusion queries may be used */
        OCCLUSION_QUERY_SUPPORT_SCOPE_REQUIRED_PRECISE,
    } OcclusionQuerySupportScope;

    /** A bitmask defining one or more queue family usage.*/
    typedef enum
    {
        QUEUE_FAMILY_COMPUTE_BIT  = 1 << 0,
        QUEUE_FAMILY_DMA_BIT      = 1 << 1,
        QUEUE_FAMILY_GRAPHICS_BIT = 1 << 2,

        QUEUE_FAMILY_FIRST_BIT = QUEUE_FAMILY_COMPUTE_BIT,
        QUEUE_FAMILY_LAST_BIT  = QUEUE_FAMILY_GRAPHICS_BIT
    } QueueFamily;
    typedef int QueueFamilyBits;

    /** Enumerates all available queue family types */
    enum class QueueFamilyType
    {
        /* Holds queues that support COMPUTE operations but do NOT support GRAPHICS operations. */
        COMPUTE,

        /* Holds queues that support TRANSFER operations and which have not been classified
         * as COMPUTE or UNIVERSAL queue family members. */
        TRANSFER,

        /* Holds queues that support GRAPHICS operations and which have not been classified
         * as COMPUTE queue family members. */
        UNIVERSAL,

        /* Always last */
        COUNT,
        FIRST     = COMPUTE,
        UNDEFINED = COUNT
    };

    typedef enum
    {
        /* Implementation should wait for each query's status to become available before retrieving its results
         *
         * Core VK 1.0 functionality
         */
        QUERY_RESULT_WAIT_BIT = 1 << 0,

        /* Each query result value is going to be followed by a status value. Non-zero values indicate result is
         * available.
         *
         * Core VK 1.0 functionality
         */
        QUERY_RESULT_WITH_AVAILABILITY_BIT = 1 << 1,

        /* Indicates it is OK for the function to return result values for a sub-range of the requested query range.
         *
         * Core VK 1.0 frunctionality
         */
        QUERY_RESULT_PARTIAL_BIT = 1 << 2,

    } QueryResultBit;
    typedef uint32_t QueryResultBits;

    /* Specifies one of the compute / rendering pipeline stages. */
    typedef enum
    {
        SHADER_STAGE_FIRST,

        SHADER_STAGE_COMPUTE = SHADER_STAGE_FIRST,
        SHADER_STAGE_FRAGMENT,
        SHADER_STAGE_GEOMETRY,
        SHADER_STAGE_TESSELLATION_CONTROL,
        SHADER_STAGE_TESSELLATION_EVALUATION,
        SHADER_STAGE_VERTEX,

        SHADER_STAGE_COUNT,
        SHADER_STAGE_UNKNOWN = SHADER_STAGE_COUNT
    } ShaderStage;

    /* Specifies the type of query for post-compile information about pipeline shaders */
    typedef enum
    {
        SHADER_INFO_FIRST,

        SHADER_INFO_TYPE_BINARY = SHADER_INFO_FIRST,
        SHADER_INFO_TYPE_DISASSEMBLY,
        SHADER_INFO_COUNT,
        SHADER_INFO_UNKNOWN = SHADER_INFO_COUNT
    } ShaderInfoType;

    typedef enum
    {
        /* Support sparse binding only */
        SPARSE_RESIDENCY_SCOPE_NONE,

        /* Support sparse residency, do not support sparse aliased residency */
        SPARSE_RESIDENCY_SCOPE_NONALIASED,

        /* Support sparse aliased residency */
        SPARSE_RESIDENCY_SCOPE_ALIASED,

        SPARSE_RESIDENCY_SCOPE_UNDEFINED
    } SparseResidencyScope;

    /** Defines supported timestamp capture modes. */
    typedef enum
    {
        /* No timestamps should be captured */
        TIMESTAMP_CAPTURE_MODE_DISABLED,

        /* Two timestamps should be captured:
         *
         * 1. top-of-pipe timestamp, preceding actual commands.
         * 2. tof-of-pipe timestamp, after all commands are recorded.
         */
        TIMESTAMP_CAPTURE_MODE_ENABLED_COMMAND_SUBMISSION_TIME,

        /* Two timestamps should be captured:
        *
        * 1. top-of-pipe timestamp, preceding actual commands.
        * 2. bottom-of-pipe timestamp, after all commands are recorded.
        */
        TIMESTAMP_CAPTURE_MODE_ENABLED_COMMAND_EXECUTION_TIME
    } TimestampCaptureMode;
}; /* namespace Anvil */

#endif /* TYPES_ENUMS_H */