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
#ifndef TYPES_MACRO_H
#define TYPES_MACRO_H

/* Wrappers for some of the Vulkan enums we use across Anvil */
#ifdef ANVIL_LITTLE_ENDIAN

    #define VkAccessFlagsVariable(name) \
        union \
        { \
            VkAccessFlags name; \
        \
            struct \
            { \
                uint8_t  VK_ACCESS_INDIRECT_COMMAND_READ_BIT          : 1; \
                uint8_t  VK_ACCESS_INDEX_READ_BIT                     : 1; \
                uint8_t  VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT          : 1; \
                uint8_t  VK_ACCESS_UNIFORM_READ_BIT                   : 1; \
                uint8_t  VK_ACCESS_INPUT_ATTACHMENT_READ_BIT          : 1; \
                uint8_t  VK_ACCESS_SHADER_READ_BIT                    : 1; \
                uint8_t  VK_ACCESS_SHADER_WRITE_BIT                   : 1; \
                uint8_t  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT          : 1; \
                uint8_t  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT         : 1; \
                uint8_t  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT  : 1; \
                uint8_t  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 1; \
                uint8_t  VK_ACCESS_TRANSFER_READ_BIT                  : 1; \
                uint8_t  VK_ACCESS_TRANSFER_WRITE_BIT                 : 1; \
                uint8_t  VK_ACCESS_HOST_READ_BIT                      : 1; \
                uint8_t  VK_ACCESS_HOST_WRITE_BIT                     : 1; \
                uint8_t  VK_ACCESS_MEMORY_READ_BIT                    : 1; \
                uint8_t  VK_ACCESS_MEMORY_WRITE_BIT                   : 1; \
            } name##_flags; \
        };

    #define VkBufferCreateFlagsVariable(name) \
        union \
        { \
            VkBufferCreateFlags name; \
        \
            struct \
            { \
                uint8_t  VK_BUFFER_CREATE_SPARSE_BINDING_BIT   : 1; \
                uint8_t  VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT : 1; \
                uint8_t  VK_BUFFER_CREATE_SPARSE_ALIASED_BIT   : 1; \
            } name##_flags; \
        };

    #define VkColorComponentFlagsVariable(name) \
        union \
        { \
            VkColorComponentFlags name; \
        \
            struct \
            { \
                uint8_t  VK_COLOR_COMPONENT_R_BIT : 1; \
                uint8_t  VK_COLOR_COMPONENT_G_BIT : 1; \
                uint8_t  VK_COLOR_COMPONENT_B_BIT : 1; \
                uint8_t  VK_COLOR_COMPONENT_A_BIT : 1; \
            } name##_flags; \
        };

    #define VkCompositeAlphaFlagsKHRVariable(name) \
        union \
        { \
            VkCompositeAlphaFlagsKHR name; \
        \
            struct \
            { \
                uint8_t  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR          : 1; \
                uint8_t  VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR  : 1; \
                uint8_t  VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR : 1; \
                uint8_t  VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR         : 1; \
            } name##_flags; \
        };

    #define VkCullModeFlagsVariable(name) \
        union \
        { \
            VkCullModeFlags name; \
        \
            struct \
            { \
                uint8_t  VK_CULL_MODE_FRONT_BIT : 1; \
                uint8_t  VK_CULL_MODE_BACK_BIT  : 1; \
            } name##_flags; \
        };

    #define VkDependencyFlagsVariable(name) \
        union \
        { \
            VkDependencyFlags name; \
        \
            struct \
            { \
                uint8_t VK_DEPENDENCY_BY_REGION_BIT        : 1; \
                uint8_t VK_DEPENDENCY_VIEW_LOCAL_BIT_KHR   : 1; \
                uint8_t VK_DEPENDENCY_DEVICE_GROUP_BIT_KHR : 1; \
            } name##_flags; \
        };

    #define VkDeviceGroupPresentModeFlagBitsKHRVariable(name) \
        union \
        { \
            VkDeviceGroupPresentModeFlagBitsKHR name; \
        \
            struct \
            { \
                uint8_t VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_BIT_KHR              : 1; \
                uint8_t VK_DEVICE_GROUP_PRESENT_MODE_REMOTE_BIT_KHR             : 1; \
                uint8_t VK_DEVICE_GROUP_PRESENT_MODE_SUM_BIT_KHR                : 1; \
                uint8_t VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_MULTI_DEVICE_BIT_KHR : 1; \
            } name##_flags; \
        };

    #define VkFormatFeatureFlagsVariable(name) \
        union \
        { \
            VkFormatFeatureFlags name; \
        \
            struct \
            { \
                uint8_t  VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT                  : 1; \
                uint8_t  VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT                  : 1; \
                uint8_t  VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT           : 1; \
                uint8_t  VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT           : 1; \
                uint8_t  VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT           : 1; \
                uint8_t  VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT    : 1; \
                uint8_t  VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT                  : 1; \
                uint8_t  VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT               : 1; \
                uint8_t  VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT         : 1; \
                uint8_t  VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT       : 1; \
                uint8_t  VK_FORMAT_FEATURE_BLIT_SRC_BIT                       : 1; \
                uint8_t  VK_FORMAT_FEATURE_BLIT_DST_BIT                       : 1; \
                uint8_t  VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT    : 1; \
                uint8_t  VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG : 1; \
                uint8_t  VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR               : 1; \
                uint8_t  VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR               : 1; \
            } name##_flags; \
        };

    #define VkMemoryHeapFlagsVariable(name) \
        union \
        { \
            VkMemoryHeapFlags name; \
        \
            struct \
            { \
                uint8_t VK_MEMORY_HEAP_DEVICE_LOCAL_BIT       : 1; \
                uint8_t VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR : 1; \
            } name##_flags; \
        };

    #define VkMemoryPropertyFlagsVariable(name) \
        union \
        { \
            VkMemoryPropertyFlags name; \
        \
            struct \
            { \
                uint8_t  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT     : 1; \
                uint8_t  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT     : 1; \
                uint8_t  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT    : 1; \
                uint8_t  VK_MEMORY_PROPERTY_HOST_CACHED_BIT      : 1; \
                uint8_t  VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT : 1; \
            } name##_flags; \
        };

    #define VkPipelineStageFlagsVariable(name) \
        union \
        { \
            VkPipelineStageFlags name; \
        \
            struct \
            { \
                uint8_t  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT                    : 1; \
                uint8_t  VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT                  : 1; \
                uint8_t  VK_PIPELINE_STAGE_VERTEX_INPUT_BIT                   : 1; \
                uint8_t  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT                  : 1; \
                uint8_t  VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT    : 1; \
                uint8_t  VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT                : 1; \
                uint8_t  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT                : 1; \
                uint8_t  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT           : 1; \
                uint8_t  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT            : 1; \
                uint8_t  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT        : 1; \
                uint8_t  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT                 : 1; \
                uint8_t  VK_PIPELINE_STAGE_TRANSFER_BIT                       : 1; \
                uint8_t  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT                 : 1; \
                uint8_t  VK_PIPELINE_STAGE_HOST_BIT                           : 1; \
                uint8_t  VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT                   : 1; \
                uint8_t  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT                   : 1; \
            } name##_flags; \
        };

    #define VkQueryControlFlagsVariable(name) \
        union \
        { \
            VkQueryControlFlags name; \
        \
            struct \
            { \
                uint8_t  VK_QUERY_CONTROL_PRECISE_BIT : 1; \
            } name##_flags; \
        };

    #define VkQueryPipelineStatisticFlagsVariable(name) \
        union \
        { \
            VkQueryPipelineStatisticFlags name; \
        \
            struct \
            { \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT                    : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT                  : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT                  : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT                : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT                 : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT                       : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT                        : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT                : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT        : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT                 : 1; \
            } name##_flags; \
        };

    #define VkQueryResultFlagsVariable(name) \
        union \
        { \
            VkQueryResultFlags name; \
        \
            struct \
            { \
                uint8_t  VK_QUERY_RESULT_64_BIT                : 1; \
                uint8_t  VK_QUERY_RESULT_WAIT_BIT              : 1; \
                uint8_t  VK_QUERY_RESULT_WITH_AVAILABILITY_BIT : 1; \
                uint8_t  VK_QUERY_RESULT_PARTIAL_BIT           : 1; \
            } name##_flags; \
        };

    #define VkQueueFlagsVariable(name) \
        union \
        { \
            VkQueueFlags name; \
        \
            struct \
            { \
                uint8_t  VK_QUEUE_GRAPHICS_BIT       : 1; \
                uint8_t  VK_QUEUE_COMPUTE_BIT        : 1; \
                uint8_t  VK_QUEUE_TRANSFER_BIT       : 1; \
                uint8_t  VK_QUEUE_SPARSE_BINDING_BIT : 1; \
            } name##_flags; \
        };

    #define VkShaderStageFlagsVariable(name) \
        union \
        { \
            VkShaderStageFlags name; \
        \
            struct \
            { \
                uint8_t  VK_SHADER_STAGE_VERTEX_BIT                  : 1; \
                uint8_t  VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT    : 1; \
                uint8_t  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 1; \
                uint8_t  VK_SHADER_STAGE_GEOMETRY_BIT                : 1; \
                uint8_t  VK_SHADER_STAGE_FRAGMENT_BIT                : 1; \
                uint8_t  VK_SHADER_STAGE_COMPUTE_BIT                 : 1; \
            } name##_flags; \
        };

    #define VkSparseImageFormatFlagsVariable(name) \
        union \
        { \
            VkSparseImageFormatFlags name; \
        \
            struct \
            { \
                uint8_t  VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT         : 1; \
                uint8_t  VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT       : 1; \
                uint8_t  VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT : 1; \
            } name##_flags; \
        };

    #define VkSparseMemoryBindFlagsVariable(name) \
        union \
        { \
            VkSparseMemoryBindFlags name; \
        \
            struct \
            { \
                uint8_t  VK_SPARSE_MEMORY_BIND_METADATA_BIT : 1; \
            } name##_flags; \
        };

    #define VkStencilFaceFlagsVariable(name) \
        union \
        { \
            VkStencilFaceFlags name; \
        \
            struct \
            { \
                uint8_t  VK_STENCIL_FACE_FRONT_BIT : 1; \
                uint8_t  VK_STENCIL_FACE_BACK_BIT  : 1; \
            } name##_flags; \
        };

    #define VkSurfaceTransformFlagsKHRVariable(name) \
        union \
        { \
            VkSurfaceTransformFlagsKHR name; \
        \
            struct \
            { \
                uint8_t  VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR                     : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR                    : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR                   : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR                   : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR            : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR  : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR                      : 1; \
            } name##_flags; \
        };
#else
    #error "Big-endian arch's are not supported"
#endif

/* Helper macros */
#define ANVIL_DISABLE_ASSIGNMENT_OPERATOR(x) private: x& operator=(const x&);
#define ANVIL_DISABLE_COPY_CONSTRUCTOR(x)    private: x(const x&);
#define ANVIL_REDUNDANT_ARGUMENT(x)          x = x;
#define ANVIL_REDUNDANT_ARGUMENT_CONST(x)    x;
#define ANVIL_REDUNDANT_VARIABLE(x)          x = x;
#define ANVIL_REDUNDANT_VARIABLE_CONST(x)    x;

#endif /* TYPES_MACRO_H */