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
#ifndef MISC_TYPES_H
#define MISC_TYPES_H

#include <cstdio>
#include <forward_list>
#include <mutex>
#include <string>

#include "config.h"


/* Disable some of the warnings we cannot work around because they are caused
 * by external dependencies (ie. Vulkan header)
 */
#ifdef _MSC_VER
    #pragma warning(disable : 4063)
#else
    #pragma GCC diagnostic ignored "-Wswitch"
    #pragma GCC diagnostic ignored "-Wreorder"
    #pragma GCC diagnostic ignored "-Wunused-value"
#endif

/* Determine endianness */
#if REG_DWORD == REG_DWORD_LITTLE_ENDIAN || __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define ANVIL_LITTLE_ENDIAN
#endif

/* The following #define is required to include Vulkan entry-point prototype declarations. */
#ifdef _WIN32
    #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        #define VK_USE_PLATFORM_WIN32_KHR
    #endif
#else
    #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        #define VK_USE_PLATFORM_XCB_KHR
    #endif
#endif


#ifdef _WIN32
    /* NOTE: Version clamp required for IsDebuggerPresent() */
    #define ANVIL_MIN_WIN32_WINNT_REQUIRED 0x0400

    #if !defined(_WIN32_WINNT)
        #define _WIN32_WINNT ANVIL_MIN_WIN32_WINNT_REQUIRED
    #else
        #if _WIN32_WINNT < ANVIL_MIN_WIN32_WINNT_REQUIRED
            #error Please update the _WIN32_WINNT macro in order for Anvil to compile successfully.
        #endif
    #endif

    #if _MSC_VER <= 1800
        #ifndef snprintf
            #define snprintf _snprintf
        #endif
    #endif

    #include <windows.h>

    #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        typedef HWND WindowHandle;
    #else
        typedef void* WindowHandle;
    #endif
#else
    #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        #include "xcb_loader.h"

        typedef xcb_window_t WindowHandle;
    #else
        typedef void* WindowHandle;
    #endif

    #include <string.h>

    #ifndef nullptr
        #define nullptr NULL
    #endif
#endif

#include "vulkan/vulkan.h"
#include "vulkan/vk_platform.h"

#include <map>
#include <memory>
#include <vector>

/* Sanity checks */
#if !defined(VK_AMD_rasterization_order)
    #error Vulkan SDK header used in the compilation process is too old. Please ensure deps\anvil\include\vulkan.h is used.
#endif

/* Wrappers for some of the Vulkan enums we use across Anvil */
#ifdef ANVIL_LITTLE_ENDIAN
    #define VkAccessFlagsVariable(name) \
        union \
        { \
            VkAccessFlags name; \
        \
            struct \
            { \
                uint8_t  VK_ACCESS_INDIRECT_COMMAND_READ_BIT : 1; \
                uint8_t  VK_ACCESS_INDEX_READ_BIT : 1; \
                uint8_t  VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT : 1; \
                uint8_t  VK_ACCESS_UNIFORM_READ_BIT : 1; \
                uint8_t  VK_ACCESS_INPUT_ATTACHMENT_READ_BIT : 1; \
                uint8_t  VK_ACCESS_SHADER_READ_BIT : 1; \
                uint8_t  VK_ACCESS_SHADER_WRITE_BIT : 1; \
                uint8_t  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT : 1; \
                uint8_t  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 1; \
                uint8_t  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT : 1; \
                uint8_t  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 1; \
                uint8_t  VK_ACCESS_TRANSFER_READ_BIT : 1; \
                uint8_t  VK_ACCESS_TRANSFER_WRITE_BIT : 1; \
                uint8_t  VK_ACCESS_HOST_READ_BIT : 1; \
                uint8_t  VK_ACCESS_HOST_WRITE_BIT : 1; \
                uint8_t  VK_ACCESS_MEMORY_READ_BIT : 1; \
                uint8_t  VK_ACCESS_MEMORY_WRITE_BIT : 1; \
                uint32_t OTHER: 15; \
            } name##_flags; \
        };

    #define VkBufferCreateFlagsVariable(name) \
        union \
        { \
            VkBufferCreateFlags name; \
        \
            struct \
            { \
                uint8_t  VK_BUFFER_CREATE_SPARSE_BINDING_BIT : 1; \
                uint8_t  VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT : 1; \
                uint8_t  VK_BUFFER_CREATE_SPARSE_ALIASED_BIT : 1; \
                uint32_t OTHER: 29; \
            } name##_flags; \
        };

    #define VkBufferUsageFlagsVariable(name) \
        union \
        { \
            VkBufferUsageFlags name; \
        \
            struct \
            { \
                uint8_t  VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 1; \
                uint8_t  VK_BUFFER_USAGE_TRANSFER_DST_BIT : 1; \
                uint8_t  VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT : 1; \
                uint8_t  VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : 1; \
                uint8_t  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 1; \
                uint8_t  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 1; \
                uint8_t  VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 1; \
                uint8_t  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 1; \
                uint8_t  VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 1; \
                uint32_t OTHER: 23; \
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
                uint32_t OTHER: 28; \
            } name##_flags; \
        };

    #define VkCompositeAlphaFlagsKHRVariable(name) \
        union \
        { \
            VkCompositeAlphaFlagsKHR name; \
        \
            struct \
            { \
                uint8_t  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : 1; \
                uint8_t  VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR : 1; \
                uint8_t  VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR : 1; \
                uint8_t  VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR : 1; \
                uint32_t OTHER: 28; \
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
                uint8_t  VK_CULL_MODE_BACK_BIT : 1; \
                uint32_t OTHER: 30; \
            } name##_flags; \
        };

    #define VkDependencyFlagsVariable(name) \
        union \
        { \
            VkDependencyFlags name; \
        \
            struct \
            { \
                uint8_t VK_DEPENDENCY_BY_REGION_BIT : 1; \
                uint32_t OTHER: 31; \
            } name##_flags; \
        };

    #define VkFormatFeatureFlagsVariable(name) \
        union \
        { \
            VkFormatFeatureFlags name; \
        \
            struct \
            { \
                uint8_t  VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_BLIT_SRC_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_BLIT_DST_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT : 1; \
                uint8_t  VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG : 1; \
                uint8_t  VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR : 1; \
                uint8_t  VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR : 1; \
                uint32_t OTHER: 16; \
            } name##_flags; \
        };

    #define VkImageAspectFlagsVariable(name) \
        union \
        { \
            VkImageAspectFlags name; \
        \
            struct \
            { \
                uint8_t  VK_IMAGE_ASPECT_COLOR_BIT : 1; \
                uint8_t  VK_IMAGE_ASPECT_DEPTH_BIT : 1; \
                uint8_t  VK_IMAGE_ASPECT_STENCIL_BIT : 1; \
                uint8_t  VK_IMAGE_ASPECT_METADATA_BIT : 1; \
                uint32_t OTHER: 28; \
            } name##_flags; \
        };

    #define VkImageUsageFlagsVariable(name) \
        union \
        { \
            VkImageUsageFlags name; \
        \
            struct \
            { \
                uint8_t  VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 1; \
                uint8_t  VK_IMAGE_USAGE_TRANSFER_DST_BIT : 1; \
                uint8_t  VK_IMAGE_USAGE_SAMPLED_BIT : 1; \
                uint8_t  VK_IMAGE_USAGE_STORAGE_BIT : 1; \
                uint8_t  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 1; \
                uint8_t  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 1; \
                uint8_t  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT : 1; \
                uint8_t  VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT : 1; \
                uint32_t OTHER: 24; \
            } name##_flags; \
        };

    #define VkMemoryHeapFlagsVariable(name) \
        union \
        { \
            VkMemoryHeapFlags name; \
        \
            struct \
            { \
                uint8_t  VK_MEMORY_HEAP_DEVICE_LOCAL_BIT : 1; \
                uint32_t OTHER: 31; \
            } name##_flags; \
        };

    #define VkMemoryPropertyFlagsVariable(name) \
        union \
        { \
            VkMemoryPropertyFlags name; \
        \
            struct \
            { \
                uint8_t  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 1; \
                uint8_t  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 1; \
                uint8_t  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 1; \
                uint8_t  VK_MEMORY_PROPERTY_HOST_CACHED_BIT : 1; \
                uint8_t  VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT : 1; \
                uint32_t OTHER: 27; \
            } name##_flags; \
        };

    #define VkPipelineStageFlagsVariable(name) \
        union \
        { \
            VkPipelineStageFlags name; \
        \
            struct \
            { \
                uint8_t  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_VERTEX_INPUT_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_TRANSFER_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_HOST_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT : 1; \
                uint8_t  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT : 1; \
                uint32_t OTHER: 15; \
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
                uint32_t OTHER: 31; \
            } name##_flags; \
        };

    #define VkQueryPipelineStatisticFlagsVariable(name) \
        union \
        { \
            VkQueryPipelineStatisticFlags name; \
        \
            struct \
            { \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT : 1; \
                uint8_t  VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT : 1; \
                uint32_t OTHER: 21; \
            } name##_flags; \
        };

    #define VkQueryResultFlagsVariable(name) \
        union \
        { \
            VkQueryResultFlags name; \
        \
            struct \
            { \
                uint8_t  VK_QUERY_RESULT_64_BIT : 1; \
                uint8_t  VK_QUERY_RESULT_WAIT_BIT : 1; \
                uint8_t  VK_QUERY_RESULT_WITH_AVAILABILITY_BIT : 1; \
                uint8_t  VK_QUERY_RESULT_PARTIAL_BIT : 1; \
                uint32_t OTHER: 28; \
            } name##_flags; \
        };

    #define VkQueueFlagsVariable(name) \
        union \
        { \
            VkQueueFlags name; \
        \
            struct \
            { \
                uint8_t  VK_QUEUE_GRAPHICS_BIT : 1; \
                uint8_t  VK_QUEUE_COMPUTE_BIT : 1; \
                uint8_t  VK_QUEUE_TRANSFER_BIT : 1; \
                uint8_t  VK_QUEUE_SPARSE_BINDING_BIT : 1; \
                uint32_t OTHER: 28; \
            } name##_flags; \
        };

    #define VkSampleCountFlagsVariable(name) \
        union \
        { \
            VkSampleCountFlags name; \
        \
            struct \
            { \
                uint8_t  VK_SAMPLE_COUNT_1_BIT : 1; \
                uint8_t  VK_SAMPLE_COUNT_2_BIT : 1; \
                uint8_t  VK_SAMPLE_COUNT_4_BIT : 1; \
                uint8_t  VK_SAMPLE_COUNT_8_BIT : 1; \
                uint8_t  VK_SAMPLE_COUNT_16_BIT : 1; \
                uint8_t  VK_SAMPLE_COUNT_32_BIT : 1; \
                uint8_t  VK_SAMPLE_COUNT_64_BIT : 1; \
                uint32_t OTHER: 25; \
            } name##_flags; \
        };

    #define VkShaderStageFlagsVariable(name) \
        union \
        { \
            VkShaderStageFlags name; \
        \
            struct \
            { \
                uint8_t  VK_SHADER_STAGE_VERTEX_BIT : 1; \
                uint8_t  VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 1; \
                uint8_t  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 1; \
                uint8_t  VK_SHADER_STAGE_GEOMETRY_BIT : 1; \
                uint8_t  VK_SHADER_STAGE_FRAGMENT_BIT : 1; \
                uint8_t  VK_SHADER_STAGE_COMPUTE_BIT : 1; \
                uint32_t OTHER: 26; \
            } name##_flags; \
        };

    #define VkSparseImageFormatFlagsVariable(name) \
        union \
        { \
            VkSparseImageFormatFlags name; \
        \
            struct \
            { \
                uint8_t  VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT : 1; \
                uint8_t  VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT: 1; \
                uint8_t  VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT: 1; \
                uint32_t OTHER: 29; \
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
                uint32_t OTHER: 31; \
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
                uint8_t  VK_STENCIL_FACE_BACK_BIT : 1; \
                uint32_t OTHER: 30; \
            } name##_flags; \
        };

    #define VkSurfaceTransformFlagsKHRVariable(name) \
        union \
        { \
            VkSurfaceTransformFlagsKHR name; \
        \
            struct \
            { \
                uint8_t  VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR : 1; \
                uint8_t  VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR : 1; \
                uint32_t OTHER: 23; \
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

/* Defines various enums used by Vulkan API wrapper classes. */
namespace Anvil
{
    /* Forward declarations */
    class  BaseDevice;
    class  Buffer;
    class  BufferView;
    class  CommandBufferBase;
    class  CommandPool;
    class  ComputePipelineManager;
    class  DAGRenderer;
    class  DescriptorPool;
    class  DescriptorSet;
    class  DescriptorSetGroup;
    class  DescriptorSetLayout;
    class  Event;
    class  Fence;
    class  Framebuffer;
    class  GraphicsPipelineManager;
    class  Image;
    class  ImageView;
    class  Instance;
    class  MemoryAllocator;
    class  MemoryBlock;
    struct MemoryHeap;
    struct MemoryProperties;
    struct MemoryType;
    class  PhysicalDevice;
    class  PipelineCache;
    class  PipelineLayout;
    class  PipelineLayoutManager;
    class  PrimaryCommandBuffer;
    class  QueryPool;
    class  Queue;
    class  RenderingSurface;
    class  RenderPass;
    class  Sampler;
    class  SecondaryCommandBuffer;
    class  Semaphore;
    class  SGPUDevice;
    class  ShaderModule;
    class  ShaderModuleCache;
    class  Swapchain;
    class  Window;

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

    /** Describes a buffer memory barrier. */
    typedef struct BufferBarrier
    {
        VkAccessFlagsVariable(dst_access_mask);
        VkAccessFlagsVariable(src_access_mask);

        VkBuffer                       buffer;
        VkBufferMemoryBarrier          buffer_barrier_vk;
        std::shared_ptr<Anvil::Buffer> buffer_ptr;
        uint32_t                       dst_queue_family_index;
        VkDeviceSize                   offset;
        VkDeviceSize                   size;
        uint32_t                       src_queue_family_index;

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
        explicit BufferBarrier(VkAccessFlags                  in_source_access_mask,
                               VkAccessFlags                  in_destination_access_mask,
                               uint32_t                       in_src_queue_family_index,
                               uint32_t                       in_dst_queue_family_index,
                               std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                               VkDeviceSize                   in_offset,
                               VkDeviceSize                   in_size);

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
        EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE,
        EXTENSION_AVAILABILITY_IGNORE,
        EXTENSION_AVAILABILITY_REQUIRE,
    } ExtensionAvailability;

    typedef std::pair<std::string, ExtensionAvailability> DeviceExtensionConfigurationItem;

    /** A struct which tells which extensions must (or should, if supported by the physical device) be enabled
     *  at device creation time.
     */
    typedef struct DeviceExtensionConfiguration
    {
        ExtensionAvailability amd_draw_indirect_count;
        ExtensionAvailability amd_gcn_shader;
        ExtensionAvailability amd_gpu_shader_half_float;
        ExtensionAvailability amd_gpu_shader_int16;
        ExtensionAvailability amd_negative_viewport_height;
        ExtensionAvailability amd_rasterization_order;
        ExtensionAvailability amd_shader_ballot;
        ExtensionAvailability amd_shader_explicit_vertex_parameter;
        ExtensionAvailability amd_shader_trinary_minmax;
        ExtensionAvailability amd_texture_gather_bias_lod;
        ExtensionAvailability ext_debug_marker;
        ExtensionAvailability ext_shader_subgroup_ballot;
        ExtensionAvailability ext_shader_subgroup_vote;
        ExtensionAvailability khr_16bit_storage;
        ExtensionAvailability khr_maintenance1;
        ExtensionAvailability khr_storage_buffer_storage_class;
        ExtensionAvailability khr_surface;
        ExtensionAvailability khr_swapchain;

        std::vector<DeviceExtensionConfigurationItem> other_extensions;


        DeviceExtensionConfiguration();

        bool is_supported_by_physical_device(std::weak_ptr<const Anvil::PhysicalDevice> in_physical_device_ptr,
                                             std::vector<std::string>*                  out_opt_unsupported_extensions_ptr = nullptr) const;
        bool operator==                     (const DeviceExtensionConfiguration&        in)                                           const;
    } DeviceExtensionConfiguration;

    /** Tells the type of an Anvil::BaseDevice instance */
    typedef enum
    {
        /* BaseDevice is implemented by SGPUDevice class */
        DEVICE_TYPE_SINGLE_GPU,

    } DeviceType;

    /** Holds properties of a single Vulkan Extension */
    typedef struct Extension
    {
        std::string name;
        uint32_t    version;

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param in_extension_props Vulkan structure to use for initialization.
         **/
        explicit Extension(const VkExtensionProperties& in_extension_props)
        {
            name    = in_extension_props.extensionName;
            version = in_extension_props.specVersion;
        }

        /** Returns true if @param in_extension_name matches the extension described by the instance. */
        bool operator==(const std::string& in_extension_name) const
        {
            return name == in_extension_name;
        }
    } Extension;

    typedef std::vector<Extension> Extensions;

    typedef struct ExtensionAMDDrawIndirectCountEntrypoints
    {
        PFN_vkCmdDrawIndexedIndirectCountAMD vkCmdDrawIndexedIndirectCountAMD;
        PFN_vkCmdDrawIndirectCountAMD        vkCmdDrawIndirectCountAMD;

        ExtensionAMDDrawIndirectCountEntrypoints()
        {
            vkCmdDrawIndexedIndirectCountAMD = nullptr;
            vkCmdDrawIndirectCountAMD        = nullptr;
        }
    } ExtensionAMDDrawIndirectCountEntrypoints;

    typedef struct ExtensionEXTDebugMarkerEntrypoints
    {
        PFN_vkCmdDebugMarkerBeginEXT      vkCmdDebugMarkerBeginEXT;
        PFN_vkCmdDebugMarkerEndEXT        vkCmdDebugMarkerEndEXT;
        PFN_vkCmdDebugMarkerInsertEXT     vkCmdDebugMarkerInsertEXT;
        PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;
        PFN_vkDebugMarkerSetObjectTagEXT  vkDebugMarkerSetObjectTagEXT;

        ExtensionEXTDebugMarkerEntrypoints()
        {
            vkCmdDebugMarkerBeginEXT      = nullptr;
            vkCmdDebugMarkerEndEXT        = nullptr;
            vkCmdDebugMarkerInsertEXT     = nullptr;
            vkDebugMarkerSetObjectNameEXT = nullptr;
            vkDebugMarkerSetObjectTagEXT  = nullptr;
        }
    } ExtensionEXTDebugMarkerEntrypoints;

    typedef struct ExtensionEXTDebugReportEntrypoints
    {
        PFN_vkCreateDebugReportCallbackEXT  vkCreateDebugReportCallbackEXT;
        PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;

        ExtensionEXTDebugReportEntrypoints()
        {
            vkCreateDebugReportCallbackEXT  = nullptr;
            vkDestroyDebugReportCallbackEXT = nullptr;
        }
    } ExtensionEXTDebugReportEntrypoints;

    typedef struct ExtensionKHRGetPhysicalDeviceProperties2
    {
        PFN_vkGetPhysicalDeviceFeatures2KHR                    vkGetPhysicalDeviceFeatures2KHR;
        PFN_vkGetPhysicalDeviceFormatProperties2KHR            vkGetPhysicalDeviceFormatProperties2KHR;
        PFN_vkGetPhysicalDeviceImageFormatProperties2KHR       vkGetPhysicalDeviceImageFormatProperties2KHR;
        PFN_vkGetPhysicalDeviceMemoryProperties2KHR            vkGetPhysicalDeviceMemoryProperties2KHR;
        PFN_vkGetPhysicalDeviceProperties2KHR                  vkGetPhysicalDeviceProperties2KHR;
        PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR       vkGetPhysicalDeviceQueueFamilyProperties2KHR;
        PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR vkGetPhysicalDeviceSparseImageFormatProperties2KHR;

        ExtensionKHRGetPhysicalDeviceProperties2()
        {
            vkGetPhysicalDeviceFeatures2KHR                    = nullptr;
            vkGetPhysicalDeviceFormatProperties2KHR            = nullptr;
            vkGetPhysicalDeviceImageFormatProperties2KHR       = nullptr;
            vkGetPhysicalDeviceMemoryProperties2KHR            = nullptr;
            vkGetPhysicalDeviceProperties2KHR                  = nullptr;
            vkGetPhysicalDeviceQueueFamilyProperties2KHR       = nullptr;
            vkGetPhysicalDeviceSparseImageFormatProperties2KHR = nullptr;
        }
    } ExtensionKHRGetPhysicalDeviceProperties2;

    typedef struct ExtensionKHRMaintenance1Entrypoints
    {
        PFN_vkTrimCommandPoolKHR vkTrimCommandPoolKHR;

        ExtensionKHRMaintenance1Entrypoints()
        {
            vkTrimCommandPoolKHR = nullptr;
        }
    } ExtensionKHRMaintenance1Entrypoints;

    typedef struct ExtensionKHRSurfaceEntrypoints
    {
        PFN_vkDestroySurfaceKHR                       vkDestroySurfaceKHR;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      vkGetPhysicalDeviceSurfaceFormatsKHR;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR      vkGetPhysicalDeviceSurfaceSupportKHR;

        ExtensionKHRSurfaceEntrypoints()
        {
            vkDestroySurfaceKHR                       = nullptr;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
            vkGetPhysicalDeviceSurfaceFormatsKHR      = nullptr;
            vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
            vkGetPhysicalDeviceSurfaceSupportKHR      = nullptr;
        }
    } ExtensionKHRSurfaceEntrypoints;

    typedef struct ExtensionKHRSwapchainEntrypoints
    {
        PFN_vkAcquireNextImageKHR   vkAcquireNextImageKHR;
        PFN_vkCreateSwapchainKHR    vkCreateSwapchainKHR;
        PFN_vkDestroySwapchainKHR   vkDestroySwapchainKHR;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
        PFN_vkQueuePresentKHR       vkQueuePresentKHR;

        ExtensionKHRSwapchainEntrypoints()
        {
            vkAcquireNextImageKHR   = nullptr;
            vkCreateSwapchainKHR    = nullptr;
            vkDestroySwapchainKHR   = nullptr;
            vkGetSwapchainImagesKHR = nullptr;
            vkQueuePresentKHR       = nullptr;
        }
    } ExtensionKHRSwapchainEntrypoints;

    #ifdef _WIN32
        #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
            typedef struct ExtensionKHRWin32SurfaceEntrypoints
            {
                PFN_vkCreateWin32SurfaceKHR                        vkCreateWin32SurfaceKHR;
                PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;

                ExtensionKHRWin32SurfaceEntrypoints()
                {
                    vkCreateWin32SurfaceKHR                        = nullptr;
                    vkGetPhysicalDeviceWin32PresentationSupportKHR = nullptr;
                }
            } ExtensionKHRWin32SurfaceEntrypoints;
        #endif
    #else
        #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
            typedef struct ExtensionKHRXcbSurfaceEntrypoints
            {
                PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;

                ExtensionKHRXcbSurfaceEntrypoints()
                {
                    vkCreateXcbSurfaceKHR = nullptr;
                }
            } ExtensionKHRXcbSurfaceEntrypoints;
        #endif
    #endif

    /** Holds driver-specific format capabilities */
    typedef struct FormatProperties
    {
        VkFormatFeatureFlagsVariable(buffer_capabilities);
        VkFormatFeatureFlagsVariable(linear_tiling_capabilities);
        VkFormatFeatureFlagsVariable(optimal_tiling_capabilities);

        /* Tells whether the format can be used with functions introduced in VK_AMD_texture_gather_bias_lod */
        bool supports_amd_texture_gather_bias_lod;

        /** Dummy constructor */
        FormatProperties()
        {
            memset(this,
                   0,
                   sizeof(*this) );
        }

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param in_format_props Vulkan structure to use for initialization.
         **/
        FormatProperties(const VkFormatProperties& in_format_props)
        {
            buffer_capabilities                  = in_format_props.bufferFeatures;
            linear_tiling_capabilities           = in_format_props.linearTilingFeatures;
            optimal_tiling_capabilities          = in_format_props.optimalTilingFeatures;
            supports_amd_texture_gather_bias_lod = false;
        }
    } FormatProperties;

    extern bool operator==(const FormatProperties& in1,
                           const FormatProperties& in2);

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

    /** ID of an Anvil framebuffer's attachment */
    typedef uint32_t FramebufferAttachmentID;

    /** Describes an image memory barrier. */
    typedef struct ImageBarrier
    {
        VkAccessFlagsVariable(dst_access_mask);
        VkAccessFlagsVariable(src_access_mask);

        bool                          by_region;
        uint32_t                      dst_queue_family_index;
        VkImage                       image;
        VkImageMemoryBarrier          image_barrier_vk;
        std::shared_ptr<Anvil::Image> image_ptr;
        VkImageLayout                 new_layout;
        VkImageLayout                 old_layout;
        uint32_t                      src_queue_family_index;
        VkImageSubresourceRange       subresource_range;

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
        ImageBarrier(VkAccessFlags                 in_source_access_mask,
                     VkAccessFlags                 in_destination_access_mask,
                     bool                          in_by_region_barrier,
                     VkImageLayout                 in_old_layout,
                     VkImageLayout                 in_new_layout,
                     uint32_t                      in_src_queue_family_index,
                     uint32_t                      in_dst_queue_family_index,
                     std::shared_ptr<Anvil::Image> in_image_ptr,
                     VkImageSubresourceRange       in_image_subresource_range);

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

    enum ImageCreateFlagBits
    {
        IMAGE_CREATE_FLAG_MUTABLE_FORMAT_BIT      = 1 << 0,
        IMAGE_CREATE_FLAG_CUBE_COMPATIBLE_BIT     = 1 << 1,

        /* NOTE: Requires VK_KHR_maintenance1 */
        IMAGE_CREATE_FLAG_2D_ARRAY_COMPATIBLE_BIT = 1 << 2,
    };
    typedef uint32_t ImageCreateFlags;

    /** Holds properties of a single Vulkan Layer. */
    typedef struct Layer
    {
        std::string            description;
        std::vector<Extension> extensions;
        uint32_t               implementation_version;
        std::string            name;
        uint32_t               spec_version;

        /** Dummy constructor.
         *
         *  @param in_layer_name Name to use for the layer.
         **/
        Layer(const std::string& in_layer_name)
        {
            implementation_version = 0;
            name                   = in_layer_name;
            spec_version           = 0;
        }

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param in_layer_props Vulkan structure to use for initialization.
         **/
        Layer(const VkLayerProperties& in_layer_props)
        {
            description            = in_layer_props.description;
            implementation_version = in_layer_props.implementationVersion;
            name                   = in_layer_props.layerName;
            spec_version           = in_layer_props.specVersion;
        }

        /** Returns true if @param in_layer_name matches the layer name described by the instance. */
        bool operator==(const std::string& in_layer_name) const
        {
            return name == in_layer_name;
        }
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
                               VkAccessFlags in_source_access_mask)
        {
            destination_access_mask = static_cast<VkAccessFlagBits>(in_destination_access_mask);
            source_access_mask      = static_cast<VkAccessFlagBits>(in_source_access_mask);

            memory_barrier_vk.dstAccessMask = destination_access_mask;
            memory_barrier_vk.pNext         = nullptr;
            memory_barrier_vk.srcAccessMask = source_access_mask;
            memory_barrier_vk.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        }

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
        MemoryHeap()
        {
            flags = 0;
            size  = 0;
        }
    } MemoryHeap;

    extern bool operator==(const MemoryHeap& in1,
                           const MemoryHeap& in2);

    typedef std::vector<MemoryHeap> MemoryHeaps;

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

    extern bool operator==(const MemoryType& in1,
                           const MemoryType& in2);

    typedef std::vector<MemoryType> MemoryTypes;

    /** Holds information about available memory heaps & types for a specific physical device. */
    typedef struct MemoryProperties
    {
        MemoryHeap* heaps;
        uint32_t    n_heaps;
        MemoryTypes types;

        MemoryProperties()
        {
            heaps   = nullptr;
            n_heaps = 0;
        }

        /** Destructor */
        ~MemoryProperties()
        {
            if (heaps != nullptr)
            {
                delete [] heaps;

                heaps = nullptr;
            }
        }

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

        /* For DOT serialization, we also need a handful of fake object types. */
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
                          VkShaderStageFlags in_stages)
        {
            offset = in_offset;
            size   = in_size;
            stages = static_cast<VkShaderStageFlagBits>(in_stages);
        }

        /** Comparison operator. Used internally. */
        bool operator==(const PushConstantRange& in) const
        {
            return (offset == in.offset &&
                    size   == in.size   &&
                    stages == in.stages);
        }
    } PushConstantRange;

    typedef uint32_t            BindingElementIndex;
    typedef uint32_t            BindingIndex;
    typedef uint32_t            NumberOfBindingElements;
    typedef BindingElementIndex StartBindingElementIndex;

    typedef std::pair<StartBindingElementIndex, NumberOfBindingElements> BindingElementArrayRange;
    typedef std::vector<PushConstantRange>                               PushConstantRanges;

    /** A bitmask defining one or more queue family usage.*/
    typedef enum
    {
        QUEUE_FAMILY_COMPUTE_BIT           = 1 << 0,
        QUEUE_FAMILY_DMA_BIT               = 1 << 1,
        QUEUE_FAMILY_GRAPHICS_BIT          = 1 << 2,
    } QueueFamily;
    typedef int QueueFamilyBits;

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
        explicit QueueFamilyInfo(const VkQueueFamilyProperties& in_props)
        {
            flags                          = in_props.queueFlags;
            min_image_transfer_granularity = in_props.minImageTransferGranularity;
            n_queues                       = in_props.queueCount;
            n_timestamp_bits               = in_props.timestampValidBits;
        }
    } QueueFamilyInfo;

    extern bool operator==(const QueueFamilyInfo& in1,
                           const QueueFamilyInfo& in2);

    typedef std::vector<QueueFamilyInfo> QueueFamilyInfoItems;

    /** Enumerates all available queue family types */
    typedef enum
    {
        QUEUE_FAMILY_TYPE_COMPUTE,
        QUEUE_FAMILY_TYPE_TRANSFER,
        QUEUE_FAMILY_TYPE_UNIVERSAL, /* compute + graphics */

        /* Always last */
        QUEUE_FAMILY_TYPE_COUNT,
        QUEUE_FAMILY_TYPE_FIRST     = QUEUE_FAMILY_TYPE_COMPUTE,
        QUEUE_FAMILY_TYPE_UNDEFINED = QUEUE_FAMILY_TYPE_COUNT
    } QueueFamilyType;

    /** Base pipeline ID. Internal type, used to represent compute / graphics pipeline IDs */
    typedef uint32_t PipelineID;

    /** Pipeline layout ID */
    typedef uint32_t PipelineLayoutID;

    /** Compute Pipeline ID */
    typedef PipelineID ComputePipelineID;

    /** Graphics Pipeline ID */
    typedef PipelineID GraphicsPipelineID;

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

    /* Index of a query within parent query pool instance */
    typedef uint32_t QueryIndex;

    /* Unique ID of a render-pass attachment within scope of a RenderPass instance. */
    typedef uint32_t RenderPassAttachmentID;

    typedef enum
    {
        RENDERING_SURFACE_TYPE_GENERAL,
    } RenderingSurfaceType;

    /* A pair of 32-bit FP values which describes a sample location */
    typedef std::pair<float, float> SampleLocation;

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

    /** Holds all information related to a specific shader module stage entry-point. */
    typedef struct ShaderModuleStageEntryPoint
    {
        std::string                          name;
        std::shared_ptr<Anvil::ShaderModule> shader_module_ptr;
        Anvil::ShaderStage                   stage;

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
        ShaderModuleStageEntryPoint(const std::string&            in_name,
                                    std::shared_ptr<ShaderModule> in_shader_module_ptr,
                                    ShaderStage                   in_stage);

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

        SparseImageAspectProperties()
        {
            memset(this,
                   0,
                   sizeof(*this) );
        }

        SparseImageAspectProperties(const VkSparseImageMemoryRequirements& in_req)
        {
            aspect_mask        = in_req.formatProperties.aspectMask;
            flags              = in_req.formatProperties.flags;
            granularity        = in_req.formatProperties.imageGranularity;
            mip_tail_first_lod = in_req.imageMipTailFirstLod;
            mip_tail_offset    = in_req.imageMipTailOffset;
            mip_tail_size      = in_req.imageMipTailSize;
            mip_tail_stride    = in_req.imageMipTailStride;
        }
    } SparseImageAspectProperties;

    /* Unique ID of a sparse memory bind update */
    typedef uint32_t SparseMemoryBindInfoID;

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

    /* Holds 16-bit storage features */
    typedef struct StorageFeatures16Bit
    {
        bool is_input_output_storage_supported;
        bool is_push_constant_16_bit_storage_supported;
        bool is_storage_buffer_16_bit_access_supported;
        bool is_uniform_and_storage_buffer_16_bit_access_supported;

        StorageFeatures16Bit()
        {
            is_input_output_storage_supported                     = false;
            is_push_constant_16_bit_storage_supported             = false;
            is_storage_buffer_16_bit_access_supported             = false;
            is_uniform_and_storage_buffer_16_bit_access_supported = false;
        }

        StorageFeatures16Bit(bool in_is_input_output_storage_supported,
                             bool in_is_push_constant_16_bit_storage_supported,
                             bool in_is_storage_buffer_16_bit_access_supported,
                             bool in_is_uniform_and_storage_buffer_16_bit_access_supported)
        {
            is_input_output_storage_supported                     = in_is_input_output_storage_supported;
            is_push_constant_16_bit_storage_supported             = in_is_push_constant_16_bit_storage_supported;
            is_storage_buffer_16_bit_access_supported             = in_is_storage_buffer_16_bit_access_supported;
            is_uniform_and_storage_buffer_16_bit_access_supported = in_is_uniform_and_storage_buffer_16_bit_access_supported;
        }
    } StorageFeatures16Bit;

    /* Unique ID of a render-pass' sub-pass attachment within scope of a RenderPass instance. */
    typedef uint32_t SubPassAttachmentID;

    /* Unique ID of a sub-pass within scope of a RenderPass instance. */
    typedef uint32_t SubPassID;

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

    namespace Utils
    {
        /** Converts a Anvil::QueueFamilyBits bitfield value to an array of queue family indices.
         *
         *  @param in_queue_families                  Input value to convert from.
         *  @param out_opt_queue_family_indices_ptr   If not NULL, deref will be updated with *@param out_opt_n_queue_family_indices_ptr
         *                                            values, corresponding to queue family indices, as specified under @param in_queue_families.
         *  @param out_opt_n_queue_family_indices_ptr If not NULL, deref will be set to the number of items that would be or were written
         *                                            under @param out_opt_queue_family_indices_ptr.
         *
         **/
        void convert_queue_family_bits_to_family_indices(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                         Anvil::QueueFamilyBits           in_queue_families,
                                                         uint32_t*                        out_opt_queue_family_indices_ptr,
                                                         uint32_t*                        out_opt_n_queue_family_indices_ptr);

        /** Returns an access mask which has all the access bits, relevant to the user-specified image layout,
         *  enabled. */
        VkAccessFlags get_access_mask_from_image_layout(VkImageLayout in_layout);

        /** Converts a pair of VkMemoryPropertyFlags and VkMemoryHeapFlags bitfields to a corresponding Anvil::MemoryFeatureFlags
         *  enum.
         *
         *  @param in_opt_mem_type_flags Vulkan memory property flags. May be 0.
         *  @param in_opt_mem_heap_flags Vulkan memory heap flags. May be 0.
         *
         *  @return Result bitfield.
         */
        Anvil::MemoryFeatureFlags get_memory_feature_flags_from_vk_property_flags(VkMemoryPropertyFlags in_opt_mem_type_flags,
                                                                                  VkMemoryHeapFlags     in_opt_mem_heap_flags);

        /** Converts the specified VkAttachmentLoadOp value to a raw string
         *
         *  @param in_load_op Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkAttachmentLoadOp in_load_op);

        /** Converts the specified VkAttachmentStoreOp value to a raw string
         *
         *  @param in_store_op Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkAttachmentStoreOp in_store_op);

        /** Converts the specified VkBlendFactor value to a raw string
         *
         *  @param in_blend_factor Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkBlendFactor in_blend_factor);

        /** Converts the specified VkBlendOp value to a raw string
         *
         *  @param in_blend_op Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkBlendOp in_blend_op);

        /** Converts the specified VkCompareOp value to a raw string
         *
         *  @param in_compare_op Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkCompareOp in_compare_op);

        /** Converts the specified VkCullModeFlagBits value to a raw string
         *
         *  @param in_cull_mode Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkCullModeFlagBits in_cull_mode);

        /** Converts the specified VkDescriptorType value to a raw string
         *
         *  @param in_descriptor_type Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkDescriptorType in_descriptor_type);

        /** Converts the specified VkFrontFace value to a raw string
         *
         *  @param in_front_face Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkFrontFace in_front_face);

        /** Converts the specified VkImageAspectFlagBits value to a raw string
         *
         *  @param in_image_aspect_flag Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkImageAspectFlagBits in_image_aspect_flag);

        /** Converts the specified VkImageLayout value to a raw string
         *
         *  @param in_image_layout Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkImageLayout in_image_layout);

        /** Converts the specified VkImageTiling value to a raw string
         *
         *  @param in_image_tiling Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkImageTiling in_image_tiling);

        /** Converts the specified VkImageType value to a raw string
         *
         *  @param in_image_type Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkImageType in_image_type);

        /** Converts the specified VkImageViewType value to a raw string
         *
         *  @param in_image_view_type Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkImageViewType in_image_view_type);

        /** Converts the specified VkLogicOp value to a raw string
         *
         *  @param in_logic_op Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkLogicOp in_logic_op);

        /** Converts the specified VkPolygonMode value to a raw string
         *
         *  @param in_polygon_mode Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkPolygonMode in_polygon_mode);

        /** Converts the specified VkPrimitiveTopology value to a raw string
         *
         *  @param in_topology Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkPrimitiveTopology in_topology);

        /** Converts the specified VkSampleCountFlagBits value to a raw string
         *
         *  @param in_sample_count Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkSampleCountFlagBits in_sample_count);

        /** Converts the specified VkSharingMode value to a raw string
         *
         *  @param in_sharing_mode Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkSharingMode in_sharing_mode);

        /** Converts the specified VkStencilOp value to a raw string
         *
         *  @param in_stencil_op Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkStencilOp in_stencil_op);

        /** Converts an Anvil::MemoryFeatureFlags value to a pair of corresponding Vulkan enum values.
         *
         *  @param in_mem_feature_flags   Anvil memory feature flags to use for conversion.
         *  @param out_mem_type_flags_ptr Deref will be set to a corresponding VkMemoryPropertyFlags value. Must not
         *                                be nullptr.
         *  @param out_mem_heap_flags_ptr Deref will be set to a corresponding VkMemoryHeapFlags value. Must not
         *                                be nullptr.
         **/
        void get_vk_property_flags_from_memory_feature_flags(Anvil::MemoryFeatureFlags in_mem_feature_flags,
                                                             VkMemoryPropertyFlags*    out_mem_type_flags_ptr,
                                                             VkMemoryHeapFlags*        out_mem_heap_flags_ptr);

        /** Tells whether @param in_value is a power-of-two. */
        template <typename type>
        type is_pow2(const type in_value)
        {
            return ((in_value & (in_value - 1)) == 0);
        }

        /** Rounds down @param in_value to a multiple of @param in_base */
        template <typename type>
        type round_down(const type in_value, const type in_base)
        {
            if ((in_value % in_base) == 0)
            {
                return in_value;
            }
            else
            {
                return in_value - (in_value % in_base);
            }
        }

        /** Rounds up @param in_value to a multiple of @param in_base */
        template <typename type>
        type round_up(const type in_value, const type in_base)
        {
            if ((in_value % in_base) == 0)
            {
                return in_value;
            }
            else
            {
                return in_value + (in_base - in_value % in_base);
            }
        }

        /** Container for sparse memory binding updates */
        class SparseMemoryBindingUpdateInfo
        {
        public:
            /* Public functions */

            /** Constructor.
             *
             *  Marks the container as dirty by default.
             */
            SparseMemoryBindingUpdateInfo();

            /** Adds a new bind info to the container. The application can then append buffer memory updates
             *  to the bind info by calling append_buffer_memory_update().
             *
             *  @param in_n_signal_semaphores       Number of semaphores to signal after the bind info is processed. Can be 0.
             *  @param in_opt_signal_semaphores_ptr An array of semaphores (sized @param in_n_signal_semaphores) to signal.
             *                                      Should be null if @param in_n_signal_semaphores is 0.
             *  @param in_n_wait_semaphores         Number of semaphores to wait on before the bind info should start being
             *                                      processed. Can be 0.
             *  @param in_opt_wait_semaphores_ptr   An array of semaphores (sized @param in_n_wait_semaphores) to wait on,
             *                                      before processing the bind info. Should be null if @param in_n_wait_semaphores
             *                                      is 0.
             *
             *  @return ID of the new bind info.
             **/
            SparseMemoryBindInfoID add_bind_info(uint32_t                            in_n_signal_semaphores,
                                                 std::shared_ptr<Anvil::Semaphore>*  in_opt_signal_semaphores_ptr,
                                                 uint32_t                            in_n_wait_semaphores,
                                                 std::shared_ptr<Anvil::Semaphore>*  in_opt_wait_semaphores_ptr);

            /** Appends a new buffer memory block update to the bind info.
             *
             *  @param in_bind_info_id                   ID of the bind info to append the update to.
             *  @param in_buffer_ptr                     Buffer instance to update. Must not be NULL.
             *  @param in_buffer_memory_start_offset     Start offset of the target memory region.
             *  @param in_opt_memory_block_ptr           Memory block to use for the binding. May be NULL.
             *  @param in_opt_memory_block_start_offset  Start offset of the source memory region. Ignored
             *                                           if @param in_memory_block_ptr is NULL.
             *  @param in_size                           Size of the memory block to update.
             **/
            void append_buffer_memory_update(SparseMemoryBindInfoID              in_bind_info_id,
                                             std::shared_ptr<Anvil::Buffer>      in_buffer_ptr,
                                             VkDeviceSize                        in_buffer_memory_start_offset,
                                             std::shared_ptr<Anvil::MemoryBlock> in_opt_memory_block_ptr,
                                             VkDeviceSize                        in_opt_memory_block_start_offset,
                                             VkDeviceSize                        in_size);

            /** Appends a new non-opaque image memory update to the bind info.
             *
             *  @param in_bind_info_id                  ID of the bind info to append the update to.
             *  @param in_image_ptr                     Image instance to update. Must not be NULL.
             *  @param in_subresource                   Subresource which should be used for the update operation.
             *  @param in_offset                        Image region offset for the update operation.
             *  @param in_extent                        Extent of the update operation.
             *  @param in_flags                         VkSparseMemoryBindFlags value to use for the update.
             *  @param in_opt_memory_block_ptr          Memory block to use for the update operation. May be NULL.
             *  @param in_opt_memory_block_start_offset Start offset of the source memory region. ignored if
             *                                          @param in_opt_memory_block_ptr is NULL.
             **/
            void append_image_memory_update(SparseMemoryBindInfoID              in_bind_info_id,
                                            std::shared_ptr<Anvil::Image>       in_image_ptr,
                                            const VkImageSubresource&           in_subresource,
                                            const VkOffset3D&                   in_offset,
                                            const VkExtent3D&                   in_extent,
                                            VkSparseMemoryBindFlags             in_flags,
                                            std::shared_ptr<Anvil::MemoryBlock> in_opt_memory_block_ptr,
                                            VkDeviceSize                        in_opt_memory_block_start_offset);

            /** Appends a new opaque image memory update to the bind info.
             *
             *  @param in_bind_info_id                  ID of the bind info to append the update to.
             *  @param in_image_ptr                     Image instance to update. Must not be NULL.
             *  @param in_resource_offset               Raw memory image start offset to use for the update.
             *  @param in_size                          Number of bytes to update.
             *  @param in_flags                         VkSparseMemoryBindFlags value to use for the update.
             *  @param in_opt_memory_block_ptr          Memory block to use for the update operation. May be NULL.
             *  @param in_opt_memory_block_start_offset Start offset of the source memory region. Ignored if
             *                                          @param in_opt_memory_block_ptr is NULL.
             **/
            void append_opaque_image_memory_update(SparseMemoryBindInfoID              in_bind_info_id,
                                                   std::shared_ptr<Anvil::Image>       in_image_ptr,
                                                   VkDeviceSize                        in_resource_offset,
                                                   VkDeviceSize                        in_size,
                                                   VkSparseMemoryBindFlags             in_flags,
                                                   std::shared_ptr<Anvil::MemoryBlock> in_opt_memory_block_ptr,
                                                   VkDeviceSize                        in_opt_memory_block_start_offset);

            /** Retrieves bind info properties.
             *
             *  @param in_bind_info_id                           ID of the bind info to retrieve properties of.
             *  @param out_opt_n_buffer_memory_updates_ptr       Deref will be set to the number of buffer memory updates, assigned
             *                                                   to the specified bind info item. May be NULL.
             *  @param out_opt_n_image_memory_updates_ptr        Deref will be set to the number of non-opaque image memory updates, assigned
             *                                                   to the specified bind info item. May be NULL.
             *  @param out_opt_n_image_opaque_memory_updates_ptr Deref will be set to the number of image opaque memory updates, assigned
             *                                                   to the specified bind info item. May be NULL.
             *  @param out_opt_fence_to_set_ptr                  Deref will be set to the fence, which is going to be set once all
             *                                                   updates assigned to the bind info item are executed. May be NULL.
             *  @param out_opt_n_signal_semaphores_ptr           Deref will be set to the number of semaphores, which should be
             *                                                   signalled after bindings are applied. May be NULL.
             *  @param out_opt_signal_semaphores_ptr_ptr         Deref will be set to an array of signal semaphores. May be NULL.
             *  @param out_opt_n_wait_semaphores_ptr             Deref will be set to the number of semaphores, which should be
             *                                                   waited on before bindings are applied. May be NULL.
             *  @param out_opt_wait_semaphores_ptr_ptr           Deref will be set to an array of wait semaphores. May be NULL.
             *
             *  @return true if successful, false otherwise.
             **/
            bool get_bind_info_properties(SparseMemoryBindInfoID                     in_bind_info_id,
                                          uint32_t* const                            out_opt_n_buffer_memory_updates_ptr,
                                          uint32_t* const                            out_opt_n_image_memory_updates_ptr,
                                          uint32_t* const                            out_opt_n_image_opaque_memory_updates_ptr,
                                          uint32_t* const                            out_opt_n_signal_semaphores_ptr,
                                          const std::shared_ptr<Anvil::Semaphore>**  out_opt_signal_semaphores_ptr_ptr,
                                          uint32_t* const                            out_opt_n_wait_semaphores_ptr,
                                          const std::shared_ptr<Anvil::Semaphore>**  out_opt_wait_semaphores_ptr_ptr) const;

            /** Retrieves Vulkan descriptors which should be used for the vkQueueBindSparse() call.
             *
             *  This call will trigger baking, if the container is marked as dirty.
             *
             *  @param out_bind_info_count_ptr Deref will be set to the value which should be passed in the
             *                                 <bindInfoCount> argument of the call. Must not be NULL.
             *  @param out_bind_info_ptr       Deref will be set to a pointer to an array, which should be
             *                                 passed in the <pBindInfo> argument of the call. Must not be NULL.
             *  @param out_fence_to_set_ptr    Deref will be set to the fence, which should be set by the implementation
             *                                 after all bindings are in place. Note that the fence itself is optional
             *                                 and may be null.
             **/
            void get_bind_sparse_call_args(uint32_t*                      out_bind_info_count_ptr,
                                           const VkBindSparseInfo**       out_bind_info_ptr,
                                           std::shared_ptr<Anvil::Fence>* out_fence_to_set_ptr);

            /** Retrieves details of buffer memory binding updates, cached for user-specified bind info.
             *
             *  @param in_bind_info_id                        ID of the bind info, which owns the update, whose properties are
             *                                                being queried.
             *  @param in_n_update                            Index of the buffer memory update to retrieve properties of.
             *  @param out_opt_buffer_ptr                     If not NULL, deref will be set to the buffer, whose sparse memory
             *                                                binding should be updated. Otherwise ignored.
             *  @param out_opt_buffer_memory_start_offset_ptr If not NULL, deref will be set to the start offset of the buffer,
             *                                                at which the memory block should be bound. Otherwise ignored.
             *  @param out_opt_memory_block_ptr               If not NULL, deref will be set to the memory block, which should
             *                                                be used for the binding. Otherwise ignored.
             *  @param out_opt_memory_block_start_offset_ptr  If not NULL, deref will be set to the start offset of the memory block,
             *                                                from which the memory region, which should be used for the binding,
             *                                                starts. Otherwise ignored.
             *  @param out_opt_size_ptr                       If not NULL, deref will be set to the size of the memory region,
             *                                                which should be used for the binding. Otherwise ignored.
             *
             *  @return true if successful, false otherwise.
             **/
            bool get_buffer_memory_update_properties(SparseMemoryBindInfoID               in_bind_info_id,
                                                     uint32_t                             in_n_update,
                                                     std::shared_ptr<Anvil::Buffer>*      out_opt_buffer_ptr,
                                                     VkDeviceSize*                        out_opt_buffer_memory_start_offset_ptr,
                                                     std::shared_ptr<Anvil::MemoryBlock>* out_opt_memory_block_ptr,
                                                     VkDeviceSize*                        out_opt_memory_block_start_offset_ptr,
                                                     VkDeviceSize*                        out_opt_size_ptr) const;

            /** Retrieves the fence, if one was earlier assigned to the instance */
            std::shared_ptr<Anvil::Fence> get_fence() const
            {
                return m_fence_ptr;
            }

            /** Retrieves properties of a non-opaque image memory update with a given ID.
             *
             *  @param in_bind_info_id                       ID of the bind info, which owns the update, and whose properties are
             *                                               being queried.
             *  @param in_n_update                           Index of the image memory update to retrieve properties of.
             *  @param out_opt_image_ptr_ptr                 If not NULL, deref will be set to the image which should be updated.
             *                                               Otherwise ignored.
             *  @param out_opt_subresouce_ptr                If not NULL, deref will be set to the subresource to be used for the
             *                                               update. Otherwise ignored.
             *  @param out_opt_offset_ptr                    If not NULL, deref will be set to image start offset, at which
             *                                               the memory block should be bound. Otherwise ignored.
             *  @param out_opt_extent_ptr                    If not NULL, deref will be set to the extent of the update. Otherwise
             *                                               ignored.
             *  @param out_opt_flags_ptr                     If not NULL, deref will be set to VkSparseMemoryBindFlags value which
             *                                               is going to be used for the update. Otherwise ignored.
             *  @param out_opt_memory_block_ptr_ptr          If not NULL, deref will be set to pointer to the memory block, which
             *                                               is going to be used for the bind operation. Otherwise ignored.
             *  @param out_opt_memory_block_start_offset_ptr If not NULL, deref will be set to the start offset of the memory block,
             *                                               which should be used for the binding operation. Otherwise ignored.
             *
             *  @return true if successful, false otherwise.
             **/
            bool get_image_memory_update_properties(SparseMemoryBindInfoID               in_bind_info_id,
                                                    uint32_t                             in_n_update,
                                                    std::shared_ptr<Anvil::Image>*       out_opt_image_ptr_ptr,
                                                    VkImageSubresource*                  out_opt_subresource_ptr,
                                                    VkOffset3D*                          out_opt_offset_ptr,
                                                    VkExtent3D*                          out_opt_extent_ptr,
                                                    VkSparseMemoryBindFlags*             out_opt_flags_ptr,
                                                    std::shared_ptr<Anvil::MemoryBlock>* out_opt_memory_block_ptr_ptr,
                                                    VkDeviceSize*                        out_opt_memory_block_start_offset_ptr) const;

            /** Retrieves properties of an opaque image memory updated with a given ID.
             *
             *  @param in_bind_info_id                       ID of the bind info, which owns the update, and whose properties are being
             *                                               queried.
             *  @param in_n_update                           Index of the opaque image memory update to retrieve properties of.
             *  @param out_opt_image_ptr_ptr                 If not NULL, deref will be set to the image which should be updated. Otherwise
             *                                               ignored.
             *  @param out_opt_resource_offset_ptr           If not NULL, deref will be set to the raw image memory offset, which should
             *                                               be used for the update. Otherwise ignored.
             *  @param out_opt_size_ptr                      If not NULL, deref will be set to the size of the image memory which should
             *                                               be used for the update. Otherwise ignored.
             *  @param out_opt_flags_ptr                     If not NULL, deref will be set to the VkSParseMemoryBindFlags value which is
             *                                               going to be used for the update. Otherwise igfnored.
             *  @param out_opt_memory_block_ptr_ptr          If not NULL, deref will be set to pointer to the memory block, which is going
             *                                               to be used for the bind operation. Otherwise ignored.
             *  @param out_opt_memory_block_start_offset_ptr If not NULL, deref will be set to the start offset of the memory block, which
             *                                               should be used for the binding operation. Otherwise ignored.
             *
             *  @return true if successful, false otherwise.
             */
            bool get_image_opaque_memory_update_properties(SparseMemoryBindInfoID               in_bind_info_id,
                                                           uint32_t                             in_n_update,
                                                           std::shared_ptr<Anvil::Image>*       out_opt_image_ptr_ptr,
                                                           VkDeviceSize*                        out_opt_resource_offset_ptr,
                                                           VkDeviceSize*                        out_opt_size_ptr,
                                                           VkSparseMemoryBindFlags*             out_opt_flags_ptr,
                                                           std::shared_ptr<Anvil::MemoryBlock>* out_opt_memory_block_ptr_ptr,
                                                           VkDeviceSize*                        out_opt_memory_block_start_offset_ptr) const;

            /** Tells how many bind info items have been assigned to the descriptor */
            uint32_t get_n_bind_info_items() const
            {
                return static_cast<uint32_t>(m_bindings.size() );
            }

            /* Changes the fence (null by default), which should be set by the Vulkan implementation after it finishes
             * updating the bindings.
            **/
            void set_fence(std::shared_ptr<Anvil::Fence> in_fence_ptr)
            {
                m_fence_ptr = in_fence_ptr;
            }

        private:
            /* Private type definitions */
            typedef struct
            {
                VkDeviceSize                        start_offset;
                std::shared_ptr<Anvil::MemoryBlock> memory_block_ptr;
                VkDeviceSize                        memory_block_start_offset;
                VkDeviceSize                        size;

                VkSparseMemoryBindFlagsVariable(flags);
            } GeneralBindInfo;

            typedef struct
            {
                VkExtent3D                          extent;
                VkOffset3D                          offset;
                std::shared_ptr<Anvil::MemoryBlock> memory_block_ptr;
                VkDeviceSize                        memory_block_start_offset;
                VkImageSubresource                  subresource;

                VkSparseMemoryBindFlagsVariable(flags);
            } ImageBindInfo;

            typedef std::map<std::shared_ptr<Anvil::Buffer>, std::pair<std::vector<GeneralBindInfo>, std::vector<VkSparseMemoryBind>      >> BufferBindUpdateMap;
            typedef std::map<std::shared_ptr<Anvil::Image>,  std::pair<std::vector<ImageBindInfo>,   std::vector<VkSparseImageMemoryBind> >> ImageBindUpdateMap;
            typedef std::map<std::shared_ptr<Anvil::Image>,  std::pair<std::vector<GeneralBindInfo>, std::vector<VkSparseMemoryBind>      >> ImageOpaqueBindUpdateMap;

            typedef struct BindingInfo
            {
                BufferBindUpdateMap      buffer_updates;
                ImageOpaqueBindUpdateMap image_opaque_updates;
                ImageBindUpdateMap       image_updates;

                std::vector<std::shared_ptr<Anvil::Semaphore> > signal_semaphores;
                std::vector<VkSemaphore>                        signal_semaphores_vk;
                std::vector<std::shared_ptr<Anvil::Semaphore> > wait_semaphores;
                std::vector<VkSemaphore>                        wait_semaphores_vk;

                BindingInfo()
                {
                    /* Stub */
                }
            } BindingInfo;

            std::vector<BindingInfo>      m_bindings;
            bool                          m_dirty;
            std::shared_ptr<Anvil::Fence> m_fence_ptr;

            std::vector<VkBindSparseInfo>                  m_bindings_vk;
            std::vector<VkSparseBufferMemoryBindInfo>      m_buffer_bindings_vk;
            std::vector<VkSparseImageMemoryBindInfo>       m_image_bindings_vk;
            std::vector<VkSparseImageOpaqueMemoryBindInfo> m_image_opaque_bindings_vk;

            /* Private functions */
            SparseMemoryBindingUpdateInfo          (const SparseMemoryBindingUpdateInfo&);
            SparseMemoryBindingUpdateInfo operator=(const SparseMemoryBindingUpdateInfo&);

            void bake();
        };
    };

    /* Represents a Vulkan structure header */
    typedef struct
    {
        VkStructureType type;
        const void*     next_ptr;
    } VkStructHeader;

    /* Describes recognized subpass attachment types */
    enum Result
    {
        RESULT_SUCCESS,
        RESULT_ERROR,
        RESULT_NOT_SUPPORTED
    };
}; /* Anvil namespace */

#endif /* MISC_TYPES_H */
