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

    /* NOTE: These map 1:1 to Vulkan equialents */
    enum BufferUsageFlagBits
    {
        /* Core VK 1.0 */
        BUFFER_USAGE_FLAG_INDEX_BUFFER_BIT         = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        BUFFER_USAGE_FLAG_INDIRECT_BUFFER_BIT      = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        BUFFER_USAGE_FLAG_STORAGE_BUFFER_BIT       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        BUFFER_USAGE_FLAG_STORAGE_TEXEL_BUFFER_BIT = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        BUFFER_USAGE_FLAG_TRANSFER_DST_BIT         = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        BUFFER_USAGE_FLAG_TRANSFER_SRC_BIT         = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        BUFFER_USAGE_FLAG_UNIFORM_BUFFER_BIT       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        BUFFER_USAGE_FLAG_UNIFORM_TEXEL_BUFFER_BIT = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        BUFFER_USAGE_FLAG_VERTEX_BUFFER_BIT        = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,

        BUFFER_USAGE_UNKNOWN = 0
    };
    typedef uint32_t BufferUsageFlags;

    /* Note: These map 1:1 to VK equivalents. */
    typedef enum class ComponentSwizzle
    {
        A        = VK_COMPONENT_SWIZZLE_A,
        B        = VK_COMPONENT_SWIZZLE_B,
        G        = VK_COMPONENT_SWIZZLE_G,
        IDENTITY = VK_COMPONENT_SWIZZLE_IDENTITY,
        ONE      = VK_COMPONENT_SWIZZLE_ONE,
        R        = VK_COMPONENT_SWIZZLE_R,
        ZERO     = VK_COMPONENT_SWIZZLE_ZERO,

    } ComponentSwizzle;

    typedef enum
    {
        DEPENDENCY_BY_REGION_BIT    = VK_DEPENDENCY_BY_REGION_BIT,
        DEPENDENCY_DEVICE_GROUP_BIT = VK_DEPENDENCY_DEVICE_GROUP_BIT_KHR,
    } DependencyBits;
    typedef uint32_t DependencyFlags;

    typedef enum
    {
        DYNAMIC_STATE_BLEND_CONSTANTS_BIT                  = 1 << 0,
        DYNAMIC_STATE_DEPTH_BIAS_BIT                       = 1 << 1,
        DYNAMIC_STATE_DEPTH_BOUNDS_BIT                     = 1 << 2,
        DYNAMIC_STATE_LINE_WIDTH_BIT                       = 1 << 3,
        DYNAMIC_STATE_SCISSOR_BIT                          = 1 << 4,
        DYNAMIC_STATE_STENCIL_COMPARE_MASK_BIT             = 1 << 5,
        DYNAMIC_STATE_STENCIL_REFERENCE_BIT                = 1 << 6,
        DYNAMIC_STATE_STENCIL_WRITE_MASK_BIT               = 1 << 7,
        DYNAMIC_STATE_VIEWPORT_BIT                         = 1 << 8,
    } DynamicStateBits;
    typedef uint32_t DynamicStateBitfield;

    typedef enum
    {
        EXTERNAL_FENCE_HANDLE_TYPE_NONE = 0,

        #if defined(_WIN32)
            EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT     = 1 << 0,
            EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT = 1 << 1,
        #else
            EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT = 1 << 0,
            EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT   = 1 << 1,
        #endif

        /* Always last */
        EXTERNAL_FENCE_HANDLE_TYPE_COUNT
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
            EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT = 1 << 0,
        #endif

        /* Always last */
        EXTERNAL_MEMORY_HANDLE_TYPE_COUNT
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
            EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT = 1 << 0,
            EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT   = 1 << 1,
        #endif

        /* Always last */
        EXTERNAL_SEMAPHORE_HANDLE_TYPE_COUNT
    } ExternalSemaphoreHandleTypeBit;
    typedef uint32_t ExternalSemaphoreHandleTypeBits;

    typedef enum
    {
        PEER_MEMORY_FEATURE_COPY_DST_BIT    = 1 << 0,
        PEER_MEMORY_FEATURE_COPY_SRC_BIT    = 1 << 1,
        PEER_MEMORY_FEATURE_GENERIC_DST_BIT = 1 << 2,
        PEER_MEMORY_FEATURE_GENERIC_SRC_BIT = 1 << 3,
    } PeerMemoryFeatureBit;
    typedef uint32_t PeerMemoryFeatureFlags;

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

        /* BaseDevice is implemented by MGPUDevice class */
        DEVICE_TYPE_MULTI_GPU
    } DeviceType;

    /* NOTE: These map 1:1 to VK equivalents */
    typedef enum class Format
    {
        R4G4_UNORM_PACK8                    = VK_FORMAT_R4G4_UNORM_PACK8,
        R4G4B4A4_UNORM_PACK16               = VK_FORMAT_R4G4B4A4_UNORM_PACK16,
        B4G4R4A4_UNORM_PACK16               = VK_FORMAT_B4G4R4A4_UNORM_PACK16,
        R5G6B5_UNORM_PACK16                 = VK_FORMAT_R5G6B5_UNORM_PACK16,
        B5G6R5_UNORM_PACK16                 = VK_FORMAT_B5G6R5_UNORM_PACK16,
        R5G5B5A1_UNORM_PACK16               = VK_FORMAT_R5G5B5A1_UNORM_PACK16,
        B5G5R5A1_UNORM_PACK16               = VK_FORMAT_B5G5R5A1_UNORM_PACK16,
        A1R5G5B5_UNORM_PACK16               = VK_FORMAT_A1R5G5B5_UNORM_PACK16,
        R8_UNORM                            = VK_FORMAT_R8_UNORM,
        R8_SNORM                            = VK_FORMAT_R8_SNORM,
        R8_USCALED                          = VK_FORMAT_R8_USCALED,
        R8_SSCALED                          = VK_FORMAT_R8_SSCALED,
        R8_UINT                             = VK_FORMAT_R8_UINT,
        R8_SINT                             = VK_FORMAT_R8_SINT,
        R8_SRGB                             = VK_FORMAT_R8_SRGB,
        R8G8_UNORM                          = VK_FORMAT_R8G8_UNORM,
        R8G8_SNORM                          = VK_FORMAT_R8G8_SNORM,
        R8G8_USCALED                        = VK_FORMAT_R8G8_USCALED,
        R8G8_SSCALED                        = VK_FORMAT_R8G8_SSCALED,
        R8G8_UINT                           = VK_FORMAT_R8G8_UINT,
        R8G8_SINT                           = VK_FORMAT_R8G8_SINT,
        R8G8_SRGB                           = VK_FORMAT_R8G8_SRGB,
        R8G8B8_UNORM                        = VK_FORMAT_R8G8B8_UNORM,
        R8G8B8_SNORM                        = VK_FORMAT_R8G8B8_SNORM,
        R8G8B8_USCALED                      = VK_FORMAT_R8G8B8_USCALED,
        R8G8B8_SSCALED                      = VK_FORMAT_R8G8B8_SSCALED,
        R8G8B8_UINT                         = VK_FORMAT_R8G8B8_UINT,
        R8G8B8_SINT                         = VK_FORMAT_R8G8B8_SINT,
        R8G8B8_SRGB                         = VK_FORMAT_R8G8B8_SRGB,
        B8G8R8_UNORM                        = VK_FORMAT_B8G8R8_UNORM,
        B8G8R8_SNORM                        = VK_FORMAT_B8G8R8_SNORM,
        B8G8R8_USCALED                      = VK_FORMAT_B8G8R8_USCALED,
        B8G8R8_SSCALED                      = VK_FORMAT_B8G8R8_SSCALED,
        B8G8R8_UINT                         = VK_FORMAT_B8G8R8_UINT,
        B8G8R8_SINT                         = VK_FORMAT_B8G8R8_SINT,
        B8G8R8_SRGB                         = VK_FORMAT_B8G8R8_SRGB,
        R8G8B8A8_UNORM                      = VK_FORMAT_R8G8B8A8_UNORM,
        R8G8B8A8_SNORM                      = VK_FORMAT_R8G8B8A8_SNORM,
        R8G8B8A8_USCALED                    = VK_FORMAT_R8G8B8A8_USCALED,
        R8G8B8A8_SSCALED                    = VK_FORMAT_R8G8B8A8_SSCALED,
        R8G8B8A8_UINT                       = VK_FORMAT_R8G8B8A8_UINT,
        R8G8B8A8_SINT                       = VK_FORMAT_R8G8B8A8_SINT,
        R8G8B8A8_SRGB                       = VK_FORMAT_R8G8B8A8_SRGB,
        B8G8R8A8_UNORM                      = VK_FORMAT_B8G8R8A8_UNORM,
        B8G8R8A8_SNORM                      = VK_FORMAT_B8G8R8A8_SNORM,
        B8G8R8A8_USCALED                    = VK_FORMAT_B8G8R8A8_USCALED,
        B8G8R8A8_SSCALED                    = VK_FORMAT_B8G8R8A8_SSCALED,
        B8G8R8A8_UINT                       = VK_FORMAT_B8G8R8A8_UINT,
        B8G8R8A8_SINT                       = VK_FORMAT_B8G8R8A8_SINT,
        B8G8R8A8_SRGB                       = VK_FORMAT_B8G8R8A8_SRGB,
        A8B8G8R8_UNORM_PACK32               = VK_FORMAT_A8B8G8R8_UNORM_PACK32,
        A8B8G8R8_SNORM_PACK32               = VK_FORMAT_A8B8G8R8_SNORM_PACK32,
        A8B8G8R8_USCALED_PACK32             = VK_FORMAT_A8B8G8R8_USCALED_PACK32,
        A8B8G8R8_SSCALED_PACK32             = VK_FORMAT_A8B8G8R8_SSCALED_PACK32,
        A8B8G8R8_UINT_PACK32                = VK_FORMAT_A8B8G8R8_UINT_PACK32,
        A8B8G8R8_SINT_PACK32                = VK_FORMAT_A8B8G8R8_SINT_PACK32,
        A8B8G8R8_SRGB_PACK32                = VK_FORMAT_A8B8G8R8_SRGB_PACK32,
        A2R10G10B10_UNORM_PACK32            = VK_FORMAT_A2R10G10B10_UNORM_PACK32,
        A2R10G10B10_SNORM_PACK32            = VK_FORMAT_A2R10G10B10_SNORM_PACK32,
        A2R10G10B10_USCALED_PACK32          = VK_FORMAT_A2R10G10B10_USCALED_PACK32,
        A2R10G10B10_SSCALED_PACK32          = VK_FORMAT_A2R10G10B10_SSCALED_PACK32,
        A2R10G10B10_UINT_PACK32             = VK_FORMAT_A2R10G10B10_UINT_PACK32,
        A2R10G10B10_SINT_PACK32             = VK_FORMAT_A2R10G10B10_SINT_PACK32,
        A2B10G10R10_UNORM_PACK32            = VK_FORMAT_A2B10G10R10_UNORM_PACK32,
        A2B10G10R10_SNORM_PACK32            = VK_FORMAT_A2B10G10R10_SNORM_PACK32,
        A2B10G10R10_USCALED_PACK32          = VK_FORMAT_A2B10G10R10_USCALED_PACK32,
        A2B10G10R10_SSCALED_PACK32          = VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
        A2B10G10R10_UINT_PACK32             = VK_FORMAT_A2B10G10R10_UINT_PACK32,
        A2B10G10R10_SINT_PACK32             = VK_FORMAT_A2B10G10R10_SINT_PACK32,
        R16_UNORM                           = VK_FORMAT_R16_UNORM,
        R16_SNORM                           = VK_FORMAT_R16_SNORM,
        R16_USCALED                         = VK_FORMAT_R16_USCALED,
        R16_SSCALED                         = VK_FORMAT_R16_SSCALED,
        R16_UINT                            = VK_FORMAT_R16_UINT,
        R16_SINT                            = VK_FORMAT_R16_SINT,
        R16_SFLOAT                          = VK_FORMAT_R16_SFLOAT,
        R16G16_UNORM                        = VK_FORMAT_R16G16_UNORM,
        R16G16_SNORM                        = VK_FORMAT_R16G16_SNORM,
        R16G16_USCALED                      = VK_FORMAT_R16G16_USCALED,
        R16G16_SSCALED                      = VK_FORMAT_R16G16_SSCALED,
        R16G16_UINT                         = VK_FORMAT_R16G16_UINT,
        R16G16_SINT                         = VK_FORMAT_R16G16_SINT,
        R16G16_SFLOAT                       = VK_FORMAT_R16G16_SFLOAT,
        R16G16B16_UNORM                     = VK_FORMAT_R16G16B16_UNORM,
        R16G16B16_SNORM                     = VK_FORMAT_R16G16B16_SNORM,
        R16G16B16_USCALED                   = VK_FORMAT_R16G16B16_USCALED,
        R16G16B16_SSCALED                   = VK_FORMAT_R16G16B16_SSCALED,
        R16G16B16_UINT                      = VK_FORMAT_R16G16B16_UINT,
        R16G16B16_SINT                      = VK_FORMAT_R16G16B16_SINT,
        R16G16B16_SFLOAT                    = VK_FORMAT_R16G16B16_SFLOAT,
        R16G16B16A16_UNORM                  = VK_FORMAT_R16G16B16A16_UNORM,
        R16G16B16A16_SNORM                  = VK_FORMAT_R16G16B16A16_SNORM,
        R16G16B16A16_USCALED                = VK_FORMAT_R16G16B16A16_USCALED,
        R16G16B16A16_SSCALED                = VK_FORMAT_R16G16B16A16_SSCALED,
        R16G16B16A16_UINT                   = VK_FORMAT_R16G16B16A16_UINT,
        R16G16B16A16_SINT                   = VK_FORMAT_R16G16B16A16_SINT,
        R16G16B16A16_SFLOAT                 = VK_FORMAT_R16G16B16A16_SFLOAT,
        R32_UINT                            = VK_FORMAT_R32_UINT,
        R32_SINT                            = VK_FORMAT_R32_SINT,
        R32_SFLOAT                          = VK_FORMAT_R32_SFLOAT,
        R32G32_UINT                         = VK_FORMAT_R32G32_UINT,
        R32G32_SINT                         = VK_FORMAT_R32G32_SINT,
        R32G32_SFLOAT                       = VK_FORMAT_R32G32_SFLOAT,
        R32G32B32_UINT                      = VK_FORMAT_R32G32B32_UINT,
        R32G32B32_SINT                      = VK_FORMAT_R32G32B32_SINT,
        R32G32B32_SFLOAT                    = VK_FORMAT_R32G32B32_SFLOAT,
        R32G32B32A32_UINT                   = VK_FORMAT_R32G32B32A32_UINT,
        R32G32B32A32_SINT                   = VK_FORMAT_R32G32B32A32_SINT,
        R32G32B32A32_SFLOAT                 = VK_FORMAT_R32G32B32A32_SFLOAT,
        R64_UINT                            = VK_FORMAT_R64_UINT,
        R64_SINT                            = VK_FORMAT_R64_SINT,
        R64_SFLOAT                          = VK_FORMAT_R64_SFLOAT,
        R64G64_UINT                         = VK_FORMAT_R64G64_UINT,
        R64G64_SINT                         = VK_FORMAT_R64G64_SINT,
        R64G64_SFLOAT                       = VK_FORMAT_R64G64_SFLOAT,
        R64G64B64_UINT                      = VK_FORMAT_R64G64B64_UINT,
        R64G64B64_SINT                      = VK_FORMAT_R64G64B64_SINT,
        R64G64B64_SFLOAT                    = VK_FORMAT_R64G64B64_SFLOAT,
        R64G64B64A64_UINT                   = VK_FORMAT_R64G64B64A64_UINT,
        R64G64B64A64_SINT                   = VK_FORMAT_R64G64B64A64_SINT,
        R64G64B64A64_SFLOAT                 = VK_FORMAT_R64G64B64A64_SFLOAT,
        B10G11R11_UFLOAT_PACK32             = VK_FORMAT_B10G11R11_UFLOAT_PACK32,
        E5B9G9R9_UFLOAT_PACK32              = VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
        D16_UNORM                           = VK_FORMAT_D16_UNORM,
        X8_D24_UNORM_PACK32                 = VK_FORMAT_X8_D24_UNORM_PACK32,
        D32_SFLOAT                          = VK_FORMAT_D32_SFLOAT,
        S8_UINT                             = VK_FORMAT_S8_UINT,
        D16_UNORM_S8_UINT                   = VK_FORMAT_D16_UNORM_S8_UINT,
        D24_UNORM_S8_UINT                   = VK_FORMAT_D24_UNORM_S8_UINT,
        D32_SFLOAT_S8_UINT                  = VK_FORMAT_D32_SFLOAT_S8_UINT,
        BC1_RGB_UNORM_BLOCK                 = VK_FORMAT_BC1_RGB_UNORM_BLOCK,
        BC1_RGB_SRGB_BLOCK                  = VK_FORMAT_BC1_RGB_SRGB_BLOCK,
        BC1_RGBA_UNORM_BLOCK                = VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
        BC1_RGBA_SRGB_BLOCK                 = VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
        BC2_UNORM_BLOCK                     = VK_FORMAT_BC2_UNORM_BLOCK,
        BC2_SRGB_BLOCK                      = VK_FORMAT_BC2_SRGB_BLOCK,
        BC3_UNORM_BLOCK                     = VK_FORMAT_BC3_UNORM_BLOCK,
        BC3_SRGB_BLOCK                      = VK_FORMAT_BC3_SRGB_BLOCK,
        BC4_UNORM_BLOCK                     = VK_FORMAT_BC4_UNORM_BLOCK,
        BC4_SNORM_BLOCK                     = VK_FORMAT_BC4_SNORM_BLOCK,
        BC5_UNORM_BLOCK                     = VK_FORMAT_BC5_UNORM_BLOCK,
        BC5_SNORM_BLOCK                     = VK_FORMAT_BC5_SNORM_BLOCK,
        BC6H_UFLOAT_BLOCK                   = VK_FORMAT_BC6H_UFLOAT_BLOCK,
        BC6H_SFLOAT_BLOCK                   = VK_FORMAT_BC6H_SFLOAT_BLOCK,
        BC7_UNORM_BLOCK                     = VK_FORMAT_BC7_UNORM_BLOCK,
        BC7_SRGB_BLOCK                      = VK_FORMAT_BC7_SRGB_BLOCK,
        ETC2_R8G8B8_UNORM_BLOCK             = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
        ETC2_R8G8B8_SRGB_BLOCK              = VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
        ETC2_R8G8B8A1_UNORM_BLOCK           = VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
        ETC2_R8G8B8A1_SRGB_BLOCK            = VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
        ETC2_R8G8B8A8_UNORM_BLOCK           = VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
        ETC2_R8G8B8A8_SRGB_BLOCK            = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
        EAC_R11_UNORM_BLOCK                 = VK_FORMAT_EAC_R11_UNORM_BLOCK,
        EAC_R11_SNORM_BLOCK                 = VK_FORMAT_EAC_R11_SNORM_BLOCK,
        EAC_R11G11_UNORM_BLOCK              = VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
        EAC_R11G11_SNORM_BLOCK              = VK_FORMAT_EAC_R11G11_SNORM_BLOCK,
        ASTC_4x4_UNORM_BLOCK                = VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
        ASTC_4x4_SRGB_BLOCK                 = VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
        ASTC_5x4_UNORM_BLOCK                = VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
        ASTC_5x4_SRGB_BLOCK                 = VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
        ASTC_5x5_UNORM_BLOCK                = VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
        ASTC_5x5_SRGB_BLOCK                 = VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
        ASTC_6x5_UNORM_BLOCK                = VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
        ASTC_6x5_SRGB_BLOCK                 = VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
        ASTC_6x6_UNORM_BLOCK                = VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
        ASTC_6x6_SRGB_BLOCK                 = VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
        ASTC_8x5_UNORM_BLOCK                = VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
        ASTC_8x5_SRGB_BLOCK                 = VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
        ASTC_8x6_UNORM_BLOCK                = VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
        ASTC_8x6_SRGB_BLOCK                 = VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
        ASTC_8x8_UNORM_BLOCK                = VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
        ASTC_8x8_SRGB_BLOCK                 = VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
        ASTC_10x5_UNORM_BLOCK               = VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
        ASTC_10x5_SRGB_BLOCK                = VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
        ASTC_10x6_UNORM_BLOCK               = VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
        ASTC_10x6_SRGB_BLOCK                = VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
        ASTC_10x8_UNORM_BLOCK               = VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
        ASTC_10x8_SRGB_BLOCK                = VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
        ASTC_10x10_UNORM_BLOCK              = VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
        ASTC_10x10_SRGB_BLOCK               = VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
        ASTC_12x10_UNORM_BLOCK              = VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
        ASTC_12x10_SRGB_BLOCK               = VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
        ASTC_12x12_UNORM_BLOCK              = VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
        ASTC_12x12_SRGB_BLOCK               = VK_FORMAT_ASTC_12x12_SRGB_BLOCK,

        /* Other .. */
        UNKNOWN = VK_FORMAT_UNDEFINED,
    } Format;

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

    /* NOTE: These map 1:1 to VK equivalents */
    enum ImageAspectFlagBits
    {
        /* Core VK 1.0 aspects */
        IMAGE_ASPECT_FLAG_COLOR_BIT    = VK_IMAGE_ASPECT_COLOR_BIT,
        IMAGE_ASPECT_FLAG_DEPTH_BIT    = VK_IMAGE_ASPECT_DEPTH_BIT,
        IMAGE_ASPECT_FLAG_METADATA_BIT = VK_IMAGE_ASPECT_METADATA_BIT,
        IMAGE_ASPECT_FLAG_STENCIL_BIT  = VK_IMAGE_ASPECT_STENCIL_BIT,

        IMAGE_ASPECT_UNKNOWN = 0
    };
    typedef uint32_t ImageAspectFlags;

    /* NOTE: These map 1:1 to VK equivalents */
    enum ImageCreateFlagBits
    {
        IMAGE_CREATE_FLAG_MUTABLE_FORMAT_BIT      = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
        IMAGE_CREATE_FLAG_CUBE_COMPATIBLE_BIT     = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,

        /* NOTE: Requires VK_KHR_bind_memory2 */
        IMAGE_CREATE_FLAG_SPLIT_INSTANCE_BIND_REGIONS_BIT = VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR,
        IMAGE_CREATE_FLAG_ALIAS_BIT                       = VK_IMAGE_CREATE_ALIAS_BIT_KHR,

        /* NOTE: Requires VK_KHR_maintenance1 */
        IMAGE_CREATE_FLAG_2D_ARRAY_COMPATIBLE_BIT = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR,

        /* NOTE: Requires VK_KHR_maintenance2 */
        IMAGE_CREATE_FLAG_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT = VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR,
        IMAGE_CREATE_FLAG_EXTENDED_USAGE_BIT              = VK_IMAGE_CREATE_EXTENDED_USAGE_BIT_KHR,
    };

    typedef uint32_t ImageCreateFlags;

    /* NOTE: These map 1:1 to VK equivalents */
    enum class ImageLayout
    {
        /* Core VK 1.0 */
        COLOR_ATTACHMENT_OPTIMAL         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        DEPTH_STENCIL_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        DEPTH_STENCIL_READ_ONLY_OPTIMAL  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        GENERAL                          = VK_IMAGE_LAYOUT_GENERAL,
        PREINITIALIZED                   = VK_IMAGE_LAYOUT_PREINITIALIZED,
        SHADER_READ_ONLY_OPTIMAL         = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        TRANSFER_DST_OPTIMAL             = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        TRANSFER_SRC_OPTIMAL             = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        UNDEFINED                        = VK_IMAGE_LAYOUT_UNDEFINED,

        /* Requires VK_KHR_maintenance2 */
        DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR,
        DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR,

        /* Requires VK_KHR_swapchain */
        PRESENT_SRC_KHR = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

        UNKNOWN,
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum ImageUsageFlagBits
    {
        /* Core VK 1.0 usages */
        IMAGE_USAGE_FLAG_TRANSFER_DST_BIT             = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        IMAGE_USAGE_FLAG_TRANSFER_SRC_BIT             = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        IMAGE_USAGE_FLAG_SAMPLED_BIT                  = VK_IMAGE_USAGE_SAMPLED_BIT,
        IMAGE_USAGE_FLAG_STORAGE_BIT                  = VK_IMAGE_USAGE_STORAGE_BIT,
        IMAGE_USAGE_FLAG_COLOR_ATTACHMENT_BIT         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        IMAGE_USAGE_FLAG_DEPTH_STENCIL_ATTACHMENT_BIT = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        IMAGE_USAGE_FLAG_TRANSIENT_ATTACHMENT_BIT     = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        IMAGE_USAGE_FLAG_INPUT_ATTACHMENT_BIT         = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,

        IMAGE_USAGE_UNKNOWN                           = 0
    };
    typedef uint32_t ImageUsageFlags;

    typedef enum class ImageInternalType
    {
        NONSPARSE_ALLOC,
        NONSPARSE_NO_ALLOC,
        NONSPARSE_PEER_NO_ALLOC,
        SPARSE_NO_ALLOC,
        SWAPCHAIN_WRAPPER
    } ImageInternalType;

    /* NOTE: These correspond 1:1 to VK equivalents */
    typedef enum class ImageTiling
    {
        LINEAR  = VK_IMAGE_TILING_LINEAR,
        OPTIMAL = VK_IMAGE_TILING_OPTIMAL,

        UNKNOWN
    } ImageTiling;

    /* NOTE: These correspond 1:1 to VK equivalents */
    typedef enum class ImageType
    {
        _1D = VK_IMAGE_TYPE_1D,
        _2D = VK_IMAGE_TYPE_2D,
        _3D = VK_IMAGE_TYPE_3D,

        UNKNOWN
    } ImageType;

    enum class IndexType
    {
        UINT16,
        UINT32,

        UNKNOWN
    };

    enum class MemoryBlockType
    {
        DERIVED,
        DERIVED_WITH_CUSTOM_DELETE_PROC,
        REGULAR
    };

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
        MEMORY_FEATURE_FLAG_MULTI_INSTANCE   = 1 << 5,
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

    /* NOTE: These map 1:1 to VK equivalents */
    typedef enum class PointClippingBehavior
    {
        ALL_CLIP_PLANES       = VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES_KHR,
        USER_CLIP_PLANES_ONLY = VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY_KHR,

        UNKNOWN = VK_POINT_CLIPPING_BEHAVIOR_MAX_ENUM
    } PointClippingBehavior;

    /** A bitmask defining one or more queue family usage.*/
    typedef enum
    {
        QUEUE_FAMILY_COMPUTE_BIT      = 1 << 0,
        QUEUE_FAMILY_DMA_BIT          = 1 << 1,
        QUEUE_FAMILY_GRAPHICS_BIT     = 1 << 2,

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

    /* NOTE: These map 1:1 to VK equivalents */
    enum SampleCountFlagBits
    {
        SAMPLE_COUNT_FLAG_1_BIT  = VK_SAMPLE_COUNT_1_BIT,
        SAMPLE_COUNT_FLAG_2_BIT  = VK_SAMPLE_COUNT_2_BIT,
        SAMPLE_COUNT_FLAG_4_BIT  = VK_SAMPLE_COUNT_4_BIT,
        SAMPLE_COUNT_FLAG_8_BIT  = VK_SAMPLE_COUNT_8_BIT,
        SAMPLE_COUNT_FLAG_16_BIT = VK_SAMPLE_COUNT_16_BIT,
        SAMPLE_COUNT_FLAG_32_BIT = VK_SAMPLE_COUNT_32_BIT,
        SAMPLE_COUNT_FLAG_64_BIT = VK_SAMPLE_COUNT_64_BIT,

        SAMPLE_COUNT_UNKNOWN = 0
    };
    typedef uint32_t SampleCountFlags;

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

    /* NOTE: These map 1:1 to VK equivalents */
    typedef enum class SharingMode
    {
        CONCURRENT = VK_SHARING_MODE_CONCURRENT,
        EXCLUSIVE  = VK_SHARING_MODE_EXCLUSIVE,

        UNKNOWN
    } SharingMode;

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

    /* NOTE: Enums map 1:1 to their VK equivalents */
    typedef enum class TessellationDomainOrigin
    {
        LOWER_LEFT = VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT_KHR,
        UPPER_LEFT = VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT_KHR,

        UNKNOWN = VK_TESSELLATION_DOMAIN_ORIGIN_MAX_ENUM
    } TessellationDomainOrigin;

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