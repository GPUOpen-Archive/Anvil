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

#ifdef IGNORE
    #undef IGNORE
#endif

#ifdef STRICT
    #undef STRICT
#endif

namespace Anvil
{
    template<typename IndividualBitEnumType, typename VKFlagsType>
    class Bitfield
    {
    public:
        Bitfield()
            :m_value(static_cast<VKFlagsType>(0) )
        {
            /* Stub */
        }

        Bitfield(const IndividualBitEnumType& in_bit)
            :m_value(static_cast<VKFlagsType>(in_bit) )
        {
            /* Stub */
        }

        Bitfield(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bits)
            :m_value(in_bits.m_value)
        {
            /* Stub */
        }

        inline Bitfield<IndividualBitEnumType, VKFlagsType> operator&(const IndividualBitEnumType& in_bit) const
        {
            auto result = Bitfield<IndividualBitEnumType, VKFlagsType>(static_cast<IndividualBitEnumType>(m_value & static_cast<VKFlagsType>(in_bit) ));

            return result;
        }

        inline Bitfield<IndividualBitEnumType, VKFlagsType> operator&(const IndividualBitEnumType& in_bit)
        {
            auto result = Bitfield<IndividualBitEnumType, VKFlagsType>(static_cast<IndividualBitEnumType>(m_value & static_cast<VKFlagsType>(in_bit)));

            return result;
        }

#if 0
        inline Bitfield<IndividualBitEnumType, VKFlagsType> operator&(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bits) const
        {
            auto result = Bitfield<IndividualBitEnumType, VKFlagsType>(static_cast<IndividualBitEnumType>(static_cast<uint32_t>(m_value) & static_cast<uint32_t>(in_bits.m_value) ));

            return result;
        }


        inline Bitfield<IndividualBitEnumType, VKFlagsType> operator&(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bits)
        {
            auto result = Bitfield<IndividualBitEnumType, VKFlagsType>(static_cast<IndividualBitEnumType>(static_cast<uint32_t>(m_value) & static_cast<uint32_t>(in_bits.m_value) ));

            return result;
        }
#endif

        inline Bitfield<IndividualBitEnumType, VKFlagsType>& operator|=(const IndividualBitEnumType& in_bit)
        {
            m_value |= static_cast<VKFlagsType>(in_bit);

            return *this;
        }

        inline Bitfield<IndividualBitEnumType, VKFlagsType>& operator|=(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bits)
        {
            m_value |= static_cast<VKFlagsType>(in_bits.m_value);

            return *this;
        }

        inline Bitfield<IndividualBitEnumType, VKFlagsType>& operator&=(const IndividualBitEnumType& in_bit)
        {
            m_value &= static_cast<VKFlagsType>(in_bit);

            return *this;
        }

        inline Bitfield<IndividualBitEnumType, VKFlagsType>& operator&=(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bits)
        {
            m_value &= static_cast<VKFlagsType>(in_bits.m_value);

            return *this;
        }

        inline Bitfield<IndividualBitEnumType, VKFlagsType>& operator=(const IndividualBitEnumType& in_bit)
        {
            m_value = static_cast<VKFlagsType>(in_bit);

            return *this;
        }

        inline Bitfield<IndividualBitEnumType, VKFlagsType>& operator=(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bits)
        {
            m_value = static_cast<VKFlagsType>(in_bits.m_value);

            return *this;
        }

        inline bool operator!=(const int& in_value) const
        {
            return m_value != static_cast<VKFlagsType>(in_value);
        }

        inline bool operator!=(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bitfield) const
        {
            return (m_value != in_bitfield.m_value);
        }

        inline bool operator!=(const IndividualBitEnumType& in_bit) const
        {
            return (m_value != static_cast<VKFlagsType>(in_bit) );
        }

        inline bool operator==(const int& in_value) const
        {
            return m_value == static_cast<VKFlagsType>(in_value);
        }

        inline bool operator==(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bitfield) const
        {
            return (m_value == in_bitfield.m_value);
        }

        inline bool operator==(const IndividualBitEnumType& in_bit) const
        {
            return (m_value == static_cast<VKFlagsType>(in_bit) );
        }

        inline bool operator<(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bitfield) const
        {
            return (m_value < in_bitfield.m_value);
        }

        inline bool operator<(const IndividualBitEnumType& in_bit) const
        {
            return (m_value < static_cast<VKFlagsType>(in_bit) );
        }

        inline bool operator<=(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bitfield) const
        {
            return (m_value <= in_bitfield.m_value);
        }

        inline bool operator<=(const IndividualBitEnumType& in_bit) const
        {
            return (m_value <= static_cast<VKFlagsType>(in_bit) );
        }

        inline bool operator>=(const Bitfield<IndividualBitEnumType, VKFlagsType>& in_bitfield) const
        {
            return (m_value >= in_bitfield.m_value);
        }

        inline bool operator>=(const IndividualBitEnumType& in_bit) const
        {
            return (m_value >= static_cast<VKFlagsType>(in_bit) );
        }

        inline const VKFlagsType& get_vk() const
        {
            return m_value;
        }

        inline const VKFlagsType* get_vk_ptr() const
        {
            return &m_value;
        }

    private:
        VKFlagsType m_value;
    };

    #define INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(BitfieldType, VKFlagsType, IndividualBitEnumType)                 \
        IndividualBitEnumType operator&(const IndividualBitEnumType& in_val1, const IndividualBitEnumType& in_val2); \
        BitfieldType          operator&(const BitfieldType&          in_val1, const BitfieldType&          in_val2); \
        IndividualBitEnumType operator|(const IndividualBitEnumType& in_val1, const IndividualBitEnumType& in_val2); \
        BitfieldType          operator|(const BitfieldType&          in_val1, const BitfieldType&          in_val2); \
        IndividualBitEnumType operator~(const IndividualBitEnumType& in_val1);                                       \
        BitfieldType          operator~(const BitfieldType&          in_val1);

    #define INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(BitfieldType, VKFlagsType, IndividualBitEnumType)                                                                  \
        IndividualBitEnumType Anvil::operator&(const IndividualBitEnumType& in_val1, const IndividualBitEnumType& in_val2)                                                \
        {                                                                                                                                                                 \
            IndividualBitEnumType result = static_cast<IndividualBitEnumType>(static_cast<uint32_t>(in_val1) & static_cast<uint32_t>(in_val2) );                          \
                                                                                                                                                                          \
            return result;                                                                                                                                                \
        }                                                                                                                                                                 \
        BitfieldType Anvil::operator&(const BitfieldType& in_val1, const BitfieldType& in_val2)                                                                           \
        {                                                                                                                                                                 \
            BitfieldType result = BitfieldType(static_cast<IndividualBitEnumType>(static_cast<uint32_t>(in_val1.get_vk() ) & static_cast<uint32_t>(in_val2.get_vk() )) ); \
                                                                                                                                                                          \
            return result;                                                                                                                                                \
        }                                                                                                                                                                 \
        IndividualBitEnumType Anvil::operator|(const IndividualBitEnumType& in_val1, const IndividualBitEnumType& in_val2)                                                \
        {                                                                                                                                                                 \
            IndividualBitEnumType result = static_cast<IndividualBitEnumType>(static_cast<uint32_t>(in_val1) | static_cast<uint32_t>(in_val2) );                          \
                                                                                                                                                                          \
            return result;                                                                                                                                                \
        }                                                                                                                                                                 \
        BitfieldType Anvil::operator|(const BitfieldType& in_val1, const BitfieldType& in_val2)                                                                           \
        {                                                                                                                                                                 \
            BitfieldType result = BitfieldType(static_cast<IndividualBitEnumType>(static_cast<uint32_t>(in_val1.get_vk() ) | static_cast<uint32_t>(in_val2.get_vk() )) ); \
                                                                                                                                                                          \
            return result;                                                                                                                                                \
        }                                                                                                                                                                 \
        IndividualBitEnumType Anvil::operator~(const IndividualBitEnumType& in_val1)                                                                                      \
        {                                                                                                                                                                 \
            IndividualBitEnumType result = static_cast<IndividualBitEnumType>(~static_cast<uint32_t>(in_val1) );                                                          \
                                                                                                                                                                          \
            return result;                                                                                                                                                \
        }

    /* NOTE: These map 1:1 to VK equivalents */
    enum class AccessFlagBits
    {
        COLOR_ATTACHMENT_READ_BIT          = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
        COLOR_ATTACHMENT_WRITE_BIT         = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        DEPTH_STENCIL_ATTACHMENT_READ_BIT  = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        HOST_READ_BIT                      = VK_ACCESS_HOST_READ_BIT,
        HOST_WRITE_BIT                     = VK_ACCESS_HOST_WRITE_BIT,
        INDEX_READ_BIT                     = VK_ACCESS_INDEX_READ_BIT,
        INDIRECT_COMMAND_READ_BIT          = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
        INPUT_ATTACHMENT_READ_BIT          = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
        MEMORY_READ_BIT                    = VK_ACCESS_MEMORY_READ_BIT,
        MEMORY_WRITE_BIT                   = VK_ACCESS_MEMORY_WRITE_BIT,
        SHADER_READ_BIT                    = VK_ACCESS_SHADER_READ_BIT,
        SHADER_WRITE_BIT                   = VK_ACCESS_SHADER_WRITE_BIT,
        TRANSFER_READ_BIT                  = VK_ACCESS_TRANSFER_READ_BIT,
        TRANSFER_WRITE_BIT                 = VK_ACCESS_TRANSFER_WRITE_BIT,
        UNIFORM_READ_BIT                   = VK_ACCESS_UNIFORM_READ_BIT,
        VERTEX_ATTRIBUTE_READ_BIT          = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,

        /* VK_EXT_transform_feedback */
        TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT  = VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT,
        TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT = VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT,
        TRANSFORM_FEEDBACK_WRITE_BIT_EXT         = VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::AccessFlagBits, VkAccessFlags> AccessFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(AccessFlags, VkAccessFlags, AccessFlagBits)

    enum class APIVersion
    {
        /* Vulkan 1.0 */
        _1_0,

        /* Vulkan 1.1 */
        _1_1,

        UNKNOWN
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class AttachmentLoadOp
    {
        CLEAR     = VK_ATTACHMENT_LOAD_OP_CLEAR,
        DONT_CARE = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        LOAD      = VK_ATTACHMENT_LOAD_OP_LOAD,

        UNKNOWN = VK_ATTACHMENT_LOAD_OP_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class AttachmentStoreOp
    {
        DONT_CARE = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        STORE     = VK_ATTACHMENT_STORE_OP_STORE,

        UNKNOWN = VK_ATTACHMENT_STORE_OP_MAX_ENUM
    };

    /* Describes recognized subpass attachment types */
    enum class AttachmentType
    {
        ATTACHMENT_TYPE_FIRST,

        COLOR = ATTACHMENT_TYPE_FIRST,
        DEPTH_STENCIL,
        INPUT,
        PRESERVE,
        RESOLVE,

        COUNT,
        UNKNOWN = COUNT
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class BlendFactor
    {
        CONSTANT_ALPHA           = VK_BLEND_FACTOR_CONSTANT_ALPHA,
        CONSTANT_COLOR           = VK_BLEND_FACTOR_CONSTANT_COLOR,
        DST_ALPHA                = VK_BLEND_FACTOR_DST_ALPHA,
        DST_COLOR                = VK_BLEND_FACTOR_DST_COLOR,
        ONE                      = VK_BLEND_FACTOR_ONE,
        ONE_MINUS_CONSTANT_ALPHA = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
        ONE_MINUS_CONSTANT_COLOR = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
        ONE_MINUS_DST_ALPHA      = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
        ONE_MINUS_DST_COLOR      = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        ONE_MINUS_SRC_ALPHA      = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        ONE_MINUS_SRC_COLOR      = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        ONE_MINUS_SRC1_COLOR     = VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
        ONE_MINUS_SRC1_ALPHA     = VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
        SRC_ALPHA                = VK_BLEND_FACTOR_SRC_ALPHA,
        SRC_ALPHA_SATURATE       = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
        SRC1_ALPHA               = VK_BLEND_FACTOR_SRC1_ALPHA,
        SRC1_COLOR               = VK_BLEND_FACTOR_SRC1_COLOR,
        SRC_COLOR                = VK_BLEND_FACTOR_SRC_COLOR,
        ZERO                     = VK_BLEND_FACTOR_ZERO,

        UNKNOWN = VK_BLEND_FACTOR_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class BlendOp
    {
        ADD              = VK_BLEND_OP_ADD,
        MAX              = VK_BLEND_OP_MAX,
        MIN              = VK_BLEND_OP_MIN,
        REVERSE_SUBTRACT = VK_BLEND_OP_REVERSE_SUBTRACT,
        SUBTRACT         = VK_BLEND_OP_SUBTRACT,

        UNKNOWN = VK_BLEND_OP_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class BorderColor
    {
        FLOAT_OPAQUE_BLACK      = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        FLOAT_OPAQUE_WHITE      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        FLOAT_TRANSPARENT_BLACK = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        INT_OPAQUE_BLACK        = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        INT_OPAQUE_WHITE        = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
        INT_TRANSPARENT_BLACK   = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,

        UNKNOWN = VK_BORDER_COLOR_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class BufferCreateFlagBits
    {
        SPARSE_ALIASED_BIT   = VK_BUFFER_CREATE_SPARSE_ALIASED_BIT,
        SPARSE_BINDING_BIT   = VK_BUFFER_CREATE_SPARSE_BINDING_BIT,
        SPARSE_RESIDENCY_BIT = VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT,

        /* Core VK 1.1 */
        CREATE_PROTECTED_BIT = VK_BUFFER_CREATE_PROTECTED_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::BufferCreateFlagBits, VkBufferCreateFlags> BufferCreateFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(BufferCreateFlags, VkBufferCreateFlags, BufferCreateFlagBits)

    enum class BufferType
    {
        ALLOC,
        NO_ALLOC,
        NO_ALLOC_CHILD,
    };

    /* NOTE: These map 1:1 to Vulkan equialents */
    enum class BufferUsageFlagBits
    {
        /* Core VK 1.0 */
        INDEX_BUFFER_BIT         = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        INDIRECT_BUFFER_BIT      = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        STORAGE_BUFFER_BIT       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        STORAGE_TEXEL_BUFFER_BIT = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        TRANSFER_DST_BIT         = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        TRANSFER_SRC_BIT         = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        UNIFORM_BUFFER_BIT       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        UNIFORM_TEXEL_BUFFER_BIT = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        VERTEX_BUFFER_BIT        = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,

        /* VK_EXT_transform_feedback */
        TRANSFORM_FEEDBACK_BUFFER_BIT_EXT         = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT,
        TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::BufferUsageFlagBits, VkBufferUsageFlags> BufferUsageFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(BufferUsageFlags, VkBufferUsageFlags, BufferUsageFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class ChromaLocation
    {
        COSITED_EVEN_KHR = VK_CHROMA_LOCATION_COSITED_EVEN_KHR,
        MIDPOINT_KHR     = VK_CHROMA_LOCATION_MIDPOINT_KHR,

        UNKNOWN = VK_CHROMA_LOCATION_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class ColorComponentFlagBits
    {
        A_BIT = VK_COLOR_COMPONENT_A_BIT,
        B_BIT = VK_COLOR_COMPONENT_B_BIT,
        G_BIT = VK_COLOR_COMPONENT_G_BIT,
        R_BIT = VK_COLOR_COMPONENT_R_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::ColorComponentFlagBits, VkColorComponentFlags> ColorComponentFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(ColorComponentFlags, VkColorComponentFlags, ColorComponentFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class ColorSpaceKHR
    {
        /* VK_KHR_surface */
        SRGB_NONLINEAR_KHR = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,

        /* VK_EXT_swapchain_colorspace */
        DISPLAY_P3_NONLINEAR_EXT    = VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,
        EXTENDED_SRGB_LINEAR_EXT    = VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT,
        DCI_P3_LINEAR_EXT           = VK_COLOR_SPACE_DCI_P3_LINEAR_EXT,
        DCI_P3_NONLINEAR_EXT        = VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT,
        BT709_LINEAR_EXT            = VK_COLOR_SPACE_BT709_LINEAR_EXT,
        BT709_NONLINEAR_EXT         = VK_COLOR_SPACE_BT709_NONLINEAR_EXT,
        BT2020_LINEAR_EXT           = VK_COLOR_SPACE_BT2020_LINEAR_EXT,
        HDR10_ST2084_EXT            = VK_COLOR_SPACE_HDR10_ST2084_EXT,
        DOLBYVISION_EXT             = VK_COLOR_SPACE_DOLBYVISION_EXT,
        HDR10_HLG_EXT               = VK_COLOR_SPACE_HDR10_HLG_EXT,
        ADOBERGB_LINEAR_EXT         = VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT,
        ADOBERGB_NONLINEAR_EXT      = VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT,
        PASS_THROUGH_EXT            = VK_COLOR_SPACE_PASS_THROUGH_EXT,
        EXTENDED_SRGB_NONLINEAR_EXT = VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT,

        UNKNOWN = VK_COLOR_SPACE_MAX_ENUM_KHR
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class CommandPoolCreateFlagBits
    {
        /* Core VK 1.0 */
        CREATE_RESET_COMMAND_BUFFER_BIT = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        CREATE_TRANSIENT_BIT            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,

        /* Core VK 1.1 */
        CREATE_PROTECTED_BIT = VK_COMMAND_POOL_CREATE_PROTECTED_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::CommandPoolCreateFlagBits, VkCommandPoolCreateFlags> CommandPoolCreateFlags;

    /* Note: These map 1:1 to VK equivalents. */
    enum class ComponentSwizzle
    {
        A        = VK_COMPONENT_SWIZZLE_A,
        B        = VK_COMPONENT_SWIZZLE_B,
        G        = VK_COMPONENT_SWIZZLE_G,
        IDENTITY = VK_COMPONENT_SWIZZLE_IDENTITY,
        ONE      = VK_COMPONENT_SWIZZLE_ONE,
        R        = VK_COMPONENT_SWIZZLE_R,
        ZERO     = VK_COMPONENT_SWIZZLE_ZERO,
    };

    /* Note: These map 1:1 to VK equivalents. */
    enum class CompositeAlphaFlagBits
    {
        OPAQUE_BIT_KHR          = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        PRE_MULTIPLIED_BIT_KHR  = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        POST_MULTIPLIED_BIT_KHR = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        INHERIT_BIT_KHR         = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,

        NONE = 0,
    };
    typedef Anvil::Bitfield<Anvil::CompositeAlphaFlagBits, VkCompositeAlphaFlagsKHR> CompositeAlphaFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(CompositeAlphaFlags, VkCompositeAlphaFlagsKHR, CompositeAlphaFlagBits)

/* Note: These map 1:1 to VK equivalents. */
    enum class DebugMessageSeverityFlagBits
    {
        ERROR_BIT   = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        INFO_BIT    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
        VERBOSE_BIT = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
        WARNING_BIT = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::DebugMessageSeverityFlagBits, VkDebugUtilsMessageSeverityFlagsEXT> DebugMessageSeverityFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(DebugMessageSeverityFlags, VkDebugUtilsMessageSeverityFlagsEXT, DebugMessageSeverityFlagBits)

    /* Note: These map 1:1 to VK equivalents. */
    enum class DebugMessageTypeFlagBits
    {
        GENERAL_BIT     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
        PERFORMANCE_BIT = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        VALIDATION_BIT  = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::DebugMessageTypeFlagBits, VkDebugUtilsMessageTypeFlagsEXT> DebugMessageTypeFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(DebugMessageTypeFlags, VkDebugUtilsMessageTypeFlagsEXT, DebugMessageTypeFlagBits)

    /* Note: These map 1:1 to VK equivalents. */
    enum class DependencyFlagBits
    {
        /* Core VK 1.0 */
        BY_REGION_BIT = VK_DEPENDENCY_BY_REGION_BIT,

        /* KHR_device_group */
        DEVICE_GROUP_BIT = VK_DEPENDENCY_DEVICE_GROUP_BIT_KHR,

        /* KHR_multiview */
        VIEW_LOCAL_BIT = VK_DEPENDENCY_VIEW_LOCAL_BIT_KHR,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::DependencyFlagBits, VkDependencyFlags> DependencyFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(DependencyFlags, VkDependencyFlags, DependencyFlagBits)

    enum class DeviceGroupPresentModeFlagBits
    {
        LOCAL_BIT_KHR              = VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_BIT_KHR,
        LOCAL_MULTI_DEVICE_BIT_KHR = VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_MULTI_DEVICE_BIT_KHR,
        REMOTE_BIT_KHR             = VK_DEVICE_GROUP_PRESENT_MODE_REMOTE_BIT_KHR,
        SUM_BIT_KHR                = VK_DEVICE_GROUP_PRESENT_MODE_SUM_BIT_KHR,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::DeviceGroupPresentModeFlagBits, VkDeviceGroupPresentModeFlagsKHR> DeviceGroupPresentModeFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(DeviceGroupPresentModeFlags, VkDeviceGroupPresentModeFlagsKHR, DeviceGroupPresentModeFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class DriverIdKHR
    {
        AMD_PROPRIETARY_KHR           = VK_DRIVER_ID_AMD_PROPRIETARY_KHR,
        AMD_OPEN_SOURCE_KHR           = VK_DRIVER_ID_AMD_OPEN_SOURCE_KHR,
        ARM_PROPRIETARY_KHR           = VK_DRIVER_ID_ARM_PROPRIETARY_KHR,
        IMAGINATION_PROPRIETARY_KHR   = VK_DRIVER_ID_IMAGINATION_PROPRIETARY_KHR,
        INTEL_OPEN_SOURCE_MESA_KHR    = VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA_KHR,
        INTEL_PROPRIETARY_WINDOWS_KHR = VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS_KHR,
        MESA_RADV_KHR                 = VK_DRIVER_ID_MESA_RADV_KHR,
        NVIDIA_PROPRIETARY_KHR        = VK_DRIVER_ID_NVIDIA_PROPRIETARY_KHR,
        QUALCOMM_PROPRIETARY_KHR      = VK_DRIVER_ID_QUALCOMM_PROPRIETARY_KHR,

        UNKNOWN
    };

    enum class DynamicState
    {
        /* Core VK 1.0 */
        BLEND_CONSTANTS      = VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        DEPTH_BIAS           = VK_DYNAMIC_STATE_DEPTH_BIAS,
        DEPTH_BOUNDS         = VK_DYNAMIC_STATE_DEPTH_BOUNDS,
        LINE_WIDTH           = VK_DYNAMIC_STATE_LINE_WIDTH,
        SCISSOR              = VK_DYNAMIC_STATE_SCISSOR,
        STENCIL_COMPARE_MASK = VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
        STENCIL_REFERENCE    = VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        STENCIL_WRITE_MASK   = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
        VIEWPORT             = VK_DYNAMIC_STATE_VIEWPORT,

        /* VK_EXT_sample_locations */
        SAMPLE_LOCATIONS_EXT = VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT,
    };

    enum class ExternalFenceHandleTypeFlagBits
    {
        #if defined(_WIN32)
            OPAQUE_WIN32_BIT     = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
            OPAQUE_WIN32_KMT_BIT = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR,
        #else
            OPAQUE_FD_BIT = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR,
            SYNC_FD_BIT   = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT_KHR,
        #endif

        NONE = 0,
    };
    typedef Anvil::Bitfield<Anvil::ExternalFenceHandleTypeFlagBits, VkExternalFenceHandleTypeFlagsKHR> ExternalFenceHandleTypeFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(ExternalFenceHandleTypeFlags, VkExternalFenceHandleTypeFlagsKHR, ExternalFenceHandleTypeFlagBits)

    enum class ExternalMemoryHandleTypeFlagBits
    {
        #if defined(_WIN32)
            /* VK_KHR_external_memory_win32 */
            OPAQUE_WIN32_BIT      = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
            OPAQUE_WIN32_KMT_BIT  = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR,
            D3D11_TEXTURE_BIT     = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT_KHR,
            D3D11_TEXTURE_KMT_BIT = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT_KHR,
            D3D12_HEAP_BIT        = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT_KHR,
            D3D12_RESOURCE_BIT    = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT_KHR,
        #else
            /* VK_KHR_external_memory_fd */
            OPAQUE_FD_BIT = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR,
        #endif

        /* VK_EXT_external_memory_host */
        HOST_ALLOCATION_BIT_EXT            = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT,
        HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT,

        NONE = 0,
    };
    typedef Anvil::Bitfield<Anvil::ExternalMemoryHandleTypeFlagBits, VkExternalMemoryHandleTypeFlagsKHR> ExternalMemoryHandleTypeFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(ExternalMemoryHandleTypeFlags, VkExternalMemoryHandleTypeFlagsKHR, ExternalMemoryHandleTypeFlagBits)

    enum class ExternalSemaphoreHandleTypeFlagBits
    {
        #if defined(_WIN32)
            OPAQUE_WIN32_BIT     = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
            OPAQUE_WIN32_KMT_BIT = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR,
            D3D12_FENCE_BIT      = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT_KHR,
        #else
            OPAQUE_FD_BIT = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR,
            SYNC_FD_BIT   = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT_KHR,
        #endif

        NONE = 0,
    };
    typedef Anvil::Bitfield<Anvil::ExternalSemaphoreHandleTypeFlagBits, VkExternalSemaphoreHandleTypeFlagsKHR> ExternalSemaphoreHandleTypeFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(ExternalSemaphoreHandleTypeFlags, VkExternalSemaphoreHandleTypeFlagsKHR, ExternalSemaphoreHandleTypeFlagBits)

    enum class FormatFeatureFlagBits
    {
        /* Core VK 1.0 */
        BLIT_DST_BIT                    = VK_FORMAT_FEATURE_BLIT_DST_BIT,
        BLIT_SRC_BIT                    = VK_FORMAT_FEATURE_BLIT_SRC_BIT,
        COLOR_ATTACHMENT_BIT            = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT,
        COLOR_ATTACHMENT_BLEND_BIT      = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT,
        DEPTH_STENCIL_ATTACHMENT_BIT    = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        SAMPLED_IMAGE_BIT               = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT,
        SAMPLED_IMAGE_FILTER_LINEAR_BIT = VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT,
        STORAGE_IMAGE_ATOMIC_BIT        = VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT,
        STORAGE_IMAGE_BIT               = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT,
        STORAGE_TEXEL_BUFFER_ATOMIC_BIT = VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT,
        STORAGE_TEXEL_BUFFER_BIT        = VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT,
        UNIFORM_TEXEL_BUFFER_BIT        = VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT,
        VERTEX_BUFFER_BIT               = VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT,

        /* EXT_sampler_filter_minmax */
        SAMPLED_IMAGE_FILTER_MINMAX_BIT_EXT = VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT_EXT,

        /* KHR_maintenance1 */
        TRANSFER_DST_BIT_KHR = VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR,
        TRANSFER_SRC_BIT_KHR = VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR,

        /* KHR_sampler_ycbcr_conversion */
        MIDPOINT_CHROMA_SAMPLES_BIT_KHR                                                 = VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT_KHR,
        COSITED_CHROMA_SAMPLES_BIT_KHR                                                  = VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT_KHR,
        SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT_KHR                            = VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT_KHR,
        SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT_KHR           = VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT_KHR,
        SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT_KHR           = VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT_KHR,
        SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT_KHR = VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT_KHR,
        DISJOINT_BIT_KHR                                                                = VK_FORMAT_FEATURE_DISJOINT_BIT_KHR,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::FormatFeatureFlagBits, VkFormatFeatureFlags> FormatFeatureFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(FormatFeatureFlags, VkFormatFeatureFlags, FormatFeatureFlagBits)

    enum class PeerMemoryFeatureFlagBits
    {
        COPY_DST_BIT    = VK_PEER_MEMORY_FEATURE_COPY_DST_BIT_KHR,
        COPY_SRC_BIT    = VK_PEER_MEMORY_FEATURE_COPY_SRC_BIT_KHR,
        GENERIC_DST_BIT = VK_PEER_MEMORY_FEATURE_GENERIC_DST_BIT_KHR,
        GENERIC_SRC_BIT = VK_PEER_MEMORY_FEATURE_GENERIC_SRC_BIT_KHR,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::PeerMemoryFeatureFlagBits, VkPeerMemoryFeatureFlagBitsKHR> PeerMemoryFeatureFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(PeerMemoryFeatureFlags, VkPeerMemoryFeatureFlags, PeerMemoryFeatureFlagBits)

    /* NOTE: Maps 1:1 to VK equivalents */
    enum class CompareOp
    {
        NEVER            = VK_COMPARE_OP_NEVER,
        LESS             = VK_COMPARE_OP_LESS,
        EQUAL            = VK_COMPARE_OP_EQUAL,
        LESS_OR_EQUAL    = VK_COMPARE_OP_LESS_OR_EQUAL,
        GREATER          = VK_COMPARE_OP_GREATER,
        NOT_EQUAL        = VK_COMPARE_OP_NOT_EQUAL,
        GREATER_OR_EQUAL = VK_COMPARE_OP_GREATER_OR_EQUAL,
        ALWAYS           = VK_COMPARE_OP_ALWAYS,

        UNKNOWN = VK_COMPARE_OP_MAX_ENUM
    };

    /** Describes component layout of a format */
    enum class ComponentLayout
    {
        /* NOTE: If the ordering used below needs to be changed, make sure to also update formats.cpp::layout_to_n_components */
        ABGR,
        ARGB,
        B,
        BGR,
        BGRA,
        BGRG,
        BR,
        BX,
        BXGXRXGX,
        BXRX,
        D,
        DS,
        EBGR,
        G,
        GBGR,
        GX,
        GXBXGXRX,
        R,
        RG,
        RGB,
        RGBA,
        RX,
        RXGX,
        RXGXBXAX,
        S,
        XD,

        UNKNOWN
    };

    enum class ConservativeRasterizationModeEXT
    {
        DISABLED      = VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT,
        OVERESTIMATE  = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT,
        UNDERESTIMATE = VK_CONSERVATIVE_RASTERIZATION_MODE_UNDERESTIMATE_EXT,

        UNKNOWN = VK_CONSERVATIVE_RASTERIZATION_MODE_MAX_ENUM_EXT
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class CullModeFlagBits
    {
        BACK_BIT  = VK_CULL_MODE_BACK_BIT,
        FRONT_BIT = VK_CULL_MODE_FRONT_BIT,
        NONE      = VK_CULL_MODE_NONE,

        FRONT_AND_BACK = VK_CULL_MODE_FRONT_AND_BACK,
    };
    typedef Anvil::Bitfield<Anvil::CullModeFlagBits, VkCullModeFlags> CullModeFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(CullModeFlags, VkCullModeFlags, CullModeFlagBits)

    /* Note: These map 1:1 to VK equivalents */
    enum class DescriptorBindingFlagBits
    {
        /* When specified for a binding, the binding can be modified after having been bound to a pipeline
         * in a command buffer, without invalidating that command buffer.
         * The updated binding becomes visible to following submissions as soon as the update function leaves.
         *
         * Requires VK_EXT_descriptor_indexing.
         */
        UPDATE_AFTER_BIND_BIT = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT,

        /* When specified for a binding, the binding can be modified after having been bound to a pipeline
         * in a command buffer, as long as it is NOT used by the command buffer. Doing so no longer invalidyates
         * the command buffer.
         *
         * Requires VK_EXT_descriptor_indexing.
         */
        UPDATE_UNUSED_WHILE_PENDING_BIT = VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT,

        /* When specified for a binding, the binding needs not be assigned valid descriptor(s), as long as none of
         * the shader invocations execute an instruction that performs any memory access using the descriptor.
         *
         * Requires VK_EXT_descriptor_indexing.
         */
        PARTIALLY_BOUND_BIT = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT,

        /* When specified for a binding, the binding gets a variable size which is specified each time a descriptor
         * set is allocated using this layout. The in_descriptor_array_size field specified at DescriptorSetCreateInfo::add_binding()
         * call time acts as an upper bound for the number of elements the binding can handle.
         *
         * Can only be specified for the last binding in the DS layout.
         *
         * Requires VK_EXT_descriptor_indexing.
         */
        VARIABLE_DESCRIPTOR_COUNT_BIT = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::DescriptorBindingFlagBits, VkDescriptorBindingFlagsEXT> DescriptorBindingFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(DescriptorBindingFlags, VkDescriptorBindingFlagsEXT, DescriptorBindingFlagBits)

    /* NOTE: Maps 1:1 to VK equivalents */
    enum class DescriptorPoolCreateFlagBits
    {
        /* When set, descriptor set allocations will return back to the pool at release time.*/
        FREE_DESCRIPTOR_SET_BIT = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

        /* When set, descriptor sets allocated from this pool can be created with the DESCRIPTOR_BINDING_FLAG_UPDATE_AFTER_BIND_BIT flag.
         *
         * Requires VK_EXT_descriptor_indexing.
         **/
        UPDATE_AFTER_BIND_BIT = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::DescriptorPoolCreateFlagBits, VkDescriptorPoolCreateFlags> DescriptorPoolCreateFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(DescriptorPoolCreateFlags, VkDescriptorPoolCreateFlags, DescriptorPoolCreateFlagBits)

    enum class DescriptorSetUpdateMethod
    {
        /* Updates dirty DS bindings using vkUpdateDescriptorSet() which is available on all Vulkan implementations. */
        CORE,

        /* Updates dirty DS bindings using vkUpdateDescriptorSetWithTemplateKHR(). Templates are cached across update operations,
         * and are release at DescriptorSet release time.
         *
         * This setting is recommended if you are going to be updating the same set of descriptor set bindings more than once.
         *
         * Only available on devices supporting VK_KHR_descriptor_update_template extension.
         */
        TEMPLATE,
    };

    enum class ExtensionAvailability
    {
        ENABLE_IF_AVAILABLE,
        IGNORE,
        REQUIRE,
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class DescriptorType
    {
        /* Core VK 1.0 functionality */

        COMBINED_IMAGE_SAMPLER = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        INPUT_ATTACHMENT       = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
        SAMPLED_IMAGE          = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        SAMPLER                = VK_DESCRIPTOR_TYPE_SAMPLER,
        STORAGE_BUFFER         = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        STORAGE_BUFFER_DYNAMIC = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
        STORAGE_IMAGE          = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        STORAGE_TEXEL_BUFFER   = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        UNIFORM_BUFFER         = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        UNIFORM_BUFFER_DYNAMIC = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        UNIFORM_TEXEL_BUFFER   = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,

        /* Requires VK_EXT_inline_uniform_block */
        INLINE_UNIFORM_BLOCK = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT,


        UNKNOWN = VK_DESCRIPTOR_TYPE_MAX_ENUM
    };

    /** Tells the type of an Anvil::BaseDevice instance */
    enum class DeviceType
    {
        /* BaseDevice is implemented by SGPUDevice class */
        SINGLE_GPU,

        /* BaseDevice is implemented by MGPUDevice class */
        MULTI_GPU,

        UNKNOWN
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class Filter
    {
        /* Core VK 1.0 functionality */
        LINEAR  = VK_FILTER_LINEAR,
        NEAREST = VK_FILTER_NEAREST,

        UNKNOWN = VK_FILTER_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class Format
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

        /* Requires VK_KHR_sampler_ycbcr_conversion */
        G8B8G8R8_422_UNORM                          = VK_FORMAT_G8B8G8R8_422_UNORM_KHR,
        B8G8R8G8_422_UNORM                          = VK_FORMAT_B8G8R8G8_422_UNORM_KHR,
        G8_B8_R8_3PLANE_420_UNORM                   = VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR,
        G8_B8R8_2PLANE_420_UNORM                    = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR,
        G8_B8_R8_3PLANE_422_UNORM                   = VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR,
        G8_B8R8_2PLANE_422_UNORM                    = VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR,
        G8_B8_R8_3PLANE_444_UNORM                   = VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR,
        R10X6_UNORM_PACK16                          = VK_FORMAT_R10X6_UNORM_PACK16_KHR,
        R10X6G10X6_UNORM_2PACK16                    = VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR,
        R10X6G10X6B10X6A10X6_UNORM_4PACK16          = VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR,
        G10X6B10X6G10X6R10X6_422_UNORM_4PACK16      = VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR,
        B10X6G10X6R10X6G10X6_422_UNORM_4PACK16      = VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR,
        G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16  = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR,
        G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16   = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR,
        G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16  = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR,
        G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16   = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR,
        G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16  = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR,
        R12X4_UNORM_PACK16                          = VK_FORMAT_R12X4_UNORM_PACK16_KHR,
        R12X4G12X4_UNORM_2PACK16                    = VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR,
        R12X4G12X4B12X4A12X4_UNORM_4PACK16          = VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR,
        G12X4B12X4G12X4R12X4_422_UNORM_4PACK16      = VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR,
        B12X4G12X4R12X4G12X4_422_UNORM_4PACK16      = VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR,
        G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16  = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR,
        G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16   = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR,
        G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16  = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR,
        G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16   = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR,
        G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16  = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR,
        G16B16G16R16_422_UNORM                      = VK_FORMAT_G16B16G16R16_422_UNORM_KHR,
        B16G16R16G16_422_UNORM                      = VK_FORMAT_B16G16R16G16_422_UNORM_KHR,
        G16_B16_R16_3PLANE_420_UNORM                = VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR,
        G16_B16R16_2PLANE_420_UNORM                 = VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR,
        G16_B16_R16_3PLANE_422_UNORM                = VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR,
        G16_B16R16_2PLANE_422_UNORM                 = VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR,
        G16_B16_R16_3PLANE_444_UNORM                = VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR,

        /* Other .. */
        UNKNOWN = VK_FORMAT_UNDEFINED,
    };


    enum class FormatType
    {
        SFLOAT,
        SFLOAT_UINT,
        SINT,
        SNORM,
        SRGB,
        SSCALED,
        UFLOAT,
        UINT,
        UNORM,
        UNORM_UINT,
        USCALED,

        UNKNOWN,
        COUNT = UNKNOWN
    } ;

    enum class FrontFace
    {
        CLOCKWISE         = VK_FRONT_FACE_CLOCKWISE,
        COUNTER_CLOCKWISE = VK_FRONT_FACE_COUNTER_CLOCKWISE,

        UNKNOWN = VK_FRONT_FACE_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class ImageAspectFlagBits
    {
        /* Core VK 1.0 aspects */
        COLOR_BIT    = VK_IMAGE_ASPECT_COLOR_BIT,
        DEPTH_BIT    = VK_IMAGE_ASPECT_DEPTH_BIT,
        METADATA_BIT = VK_IMAGE_ASPECT_METADATA_BIT,
        STENCIL_BIT  = VK_IMAGE_ASPECT_STENCIL_BIT,

        /* VK_KHR_sampler_ycbcr_conversion aspects */
        PLANE_0_BIT = VK_IMAGE_ASPECT_PLANE_0_BIT_KHR,
        PLANE_1_BIT = VK_IMAGE_ASPECT_PLANE_1_BIT_KHR,
        PLANE_2_BIT = VK_IMAGE_ASPECT_PLANE_2_BIT_KHR,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::ImageAspectFlagBits, VkImageAspectFlags> ImageAspectFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(ImageAspectFlags, VkImageAspectFlags, ImageAspectFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class ImageCreateFlagBits
    {
        /* Core VK 1.0 stuff */
        CUBE_COMPATIBLE_BIT  = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        MUTABLE_FORMAT_BIT   = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
        SPARSE_ALIASED_BIT   = VK_IMAGE_CREATE_SPARSE_ALIASED_BIT,
        SPARSE_BINDING_BIT   = VK_IMAGE_CREATE_SPARSE_BINDING_BIT,
        SPARSE_RESIDENCY_BIT = VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT,

        /* NOTE: Requires VK_EXT_sample_locations */
        SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT = VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT,

        /* NOTE: Requires VK_KHR_bind_memory2 */
        ALIAS_BIT                       = VK_IMAGE_CREATE_ALIAS_BIT_KHR,
        SPLIT_INSTANCE_BIND_REGIONS_BIT = VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR,

        /* NOTE: Requires VK_KHR_maintenance1 */
        _2D_ARRAY_COMPATIBLE_BIT = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR,

        /* NOTE: Requires VK_KHR_maintenance2 */
        BLOCK_TEXEL_VIEW_COMPATIBLE_BIT = VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR,
        EXTENDED_USAGE_BIT              = VK_IMAGE_CREATE_EXTENDED_USAGE_BIT_KHR,

        /* NOTE: Requires VK_KHR_sampler_ycbcr_conversion */
        CREATE_DISJOINT_BIT = VK_IMAGE_CREATE_DISJOINT_BIT_KHR,

        /* Note: Requires core VK 1.1 or newer */
        CREATE_PROTECTED_BIT = VK_IMAGE_CREATE_PROTECTED_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::ImageCreateFlagBits, VkImageCreateFlags> ImageCreateFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(ImageCreateFlags, VkImageCreateFlags, ImageCreateFlagBits)

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

        UNKNOWN = VK_IMAGE_LAYOUT_MAX_ENUM,
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class ImageUsageFlagBits
    {
        /* Core VK 1.0 usages */
        TRANSFER_DST_BIT             = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        TRANSFER_SRC_BIT             = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        SAMPLED_BIT                  = VK_IMAGE_USAGE_SAMPLED_BIT,
        STORAGE_BIT                  = VK_IMAGE_USAGE_STORAGE_BIT,
        COLOR_ATTACHMENT_BIT         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        DEPTH_STENCIL_ATTACHMENT_BIT = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        TRANSIENT_ATTACHMENT_BIT     = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        INPUT_ATTACHMENT_BIT         = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::ImageUsageFlagBits, VkImageUsageFlags> ImageUsageFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(ImageUsageFlags, VkImageUsageFlags, ImageUsageFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class ImageViewType
    {
        _1D         = VK_IMAGE_VIEW_TYPE_1D,
        _1D_ARRAY   = VK_IMAGE_VIEW_TYPE_1D_ARRAY,
        _2D         = VK_IMAGE_VIEW_TYPE_2D,
        _2D_ARRAY   = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        _3D         = VK_IMAGE_VIEW_TYPE_3D,
        _CUBE       = VK_IMAGE_VIEW_TYPE_CUBE,
        _CUBE_ARRAY = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,

        UNKNOWN = VK_IMAGE_VIEW_TYPE_MAX_ENUM
    };

    enum class ImageInternalType
    {
        ALLOC,
        NO_ALLOC,
        PEER_NO_ALLOC,
        SWAPCHAIN_WRAPPER
    };

    /* NOTE: These correspond 1:1 to VK equivalents */
    enum class ImageTiling
    {
        LINEAR  = VK_IMAGE_TILING_LINEAR,
        OPTIMAL = VK_IMAGE_TILING_OPTIMAL,

        UNKNOWN = VK_IMAGE_TILING_MAX_ENUM
    };

    /* NOTE: These correspond 1:1 to VK equivalents */
    enum class ImageType
    {
        _1D = VK_IMAGE_TYPE_1D,
        _2D = VK_IMAGE_TYPE_2D,
        _3D = VK_IMAGE_TYPE_3D,

        UNKNOWN = VK_IMAGE_TYPE_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class IndexType
    {
        UINT16 = VK_INDEX_TYPE_UINT16,
        UINT32 = VK_INDEX_TYPE_UINT32,

        UNKNOWN = VK_INDEX_TYPE_MAX_ENUM
    };

    enum class LogicOp
    {
        AND           = VK_LOGIC_OP_AND,
        AND_INVERTED  = VK_LOGIC_OP_AND_INVERTED,
        AND_REVERSE   = VK_LOGIC_OP_AND_REVERSE,
        CLEAR         = VK_LOGIC_OP_CLEAR,
        COPY          = VK_LOGIC_OP_COPY,
        COPY_INVERTED = VK_LOGIC_OP_COPY_INVERTED,
        EQUIVALENT    = VK_LOGIC_OP_EQUIVALENT,
        INVERT        = VK_LOGIC_OP_INVERT,
        NAND          = VK_LOGIC_OP_NAND,
        NO_OP         = VK_LOGIC_OP_NO_OP,
        NOR           = VK_LOGIC_OP_NOR,
        OR            = VK_LOGIC_OP_OR,
        OR_INVERTED   = VK_LOGIC_OP_OR_INVERTED,
        OR_REVERSE    = VK_LOGIC_OP_OR_REVERSE,
        SET           = VK_LOGIC_OP_SET,
        XOR           = VK_LOGIC_OP_XOR,

        UNKNOWN = VK_LOGIC_OP_MAX_ENUM
    };

    enum class MemoryBlockType
    {
        DERIVED,
        DERIVED_WITH_CUSTOM_DELETE_PROC,
        REGULAR,
        REGULAR_WITH_MEMORY_TYPE
    };

    enum class MemoryFeatureFlagBits
    {
        /* NOTE: If more memory feature flags are added here, make sure to also update Anvil::Utils::get_vk_property_flags_from_memory_feature_flags()
         *       and Anvil::Utils::get_memory_feature_flags_from_vk_property_flags()
         */

        DEVICE_LOCAL_BIT     = 1 << 0,
        HOST_CACHED_BIT      = 1 << 1,
        HOST_COHERENT_BIT    = 1 << 2,
        LAZILY_ALLOCATED_BIT = 1 << 3,
        MAPPABLE_BIT         = 1 << 4,
        MULTI_INSTANCE_BIT   = 1 << 5,

        /* Core VK 1.1 only */
        PROTECTED_BIT        = 1 << 6,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::MemoryFeatureFlagBits, uint32_t> MemoryFeatureFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(MemoryFeatureFlags, uint32_t, MemoryFeatureFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class MemoryHeapFlagBits
    {
        /* Core VK 1.0 */
        DEVICE_LOCAL_BIT = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,

        /* VK_KHR_device_group or core VK 1.1 */
        MULTI_INSTANCE_BIT_KHR = VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::MemoryHeapFlagBits, VkMemoryHeapFlags> MemoryHeapFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(MemoryHeapFlags, VkMemoryHeapFlags, MemoryHeapFlagBits)

    /* NOTE: Maps 1:1 to VK equivalents */
    enum class MemoryOverallocationBehavior
    {
        ALLOWED    = VK_MEMORY_OVERALLOCATION_BEHAVIOR_ALLOWED_AMD,
        DEFAULT    = VK_MEMORY_OVERALLOCATION_BEHAVIOR_DEFAULT_AMD,
        DISALLOWED = VK_MEMORY_OVERALLOCATION_BEHAVIOR_DISALLOWED_AMD,

        UNKNOWN = VK_MEMORY_OVERALLOCATION_BEHAVIOR_MAX_ENUM_AMD
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class MemoryPropertyFlagBits
    {
        /* Core VK 1.0 */
        DEVICE_LOCAL_BIT     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        HOST_CACHED_BIT      = VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
        HOST_COHERENT_BIT    = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        HOST_VISIBLE_BIT     = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        LAZILY_ALLOCATED_BIT = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,

        /* Core VK 1.1 */
        PROTECTED_BIT = VK_MEMORY_PROPERTY_PROTECTED_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::MemoryPropertyFlagBits, VkMemoryPropertyFlags> MemoryPropertyFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(MemoryPropertyFlags, VkMemoryPropertyFlags, MemoryPropertyFlagBits)

    enum class MTSafety
    {
        INHERIT_FROM_PARENT_DEVICE,
        ENABLED,
        DISABLED
    };

    enum class ObjectType
    {
        /* NOTE: If new entries are added or existing entries are removed, make sure to
         *       update Anvil::ObjectTracker::get_object_type_name().
         */
        BUFFER                      = VK_OBJECT_TYPE_BUFFER,
        BUFFER_VIEW                 = VK_OBJECT_TYPE_BUFFER_VIEW,
        COMMAND_BUFFER              = VK_OBJECT_TYPE_COMMAND_BUFFER,
        COMMAND_POOL                = VK_OBJECT_TYPE_COMMAND_POOL,
        DEBUG_REPORT_CALLBACK       = VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT,
        DEBUG_UTILS_MESSENGER       = VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT,
        DESCRIPTOR_POOL             = VK_OBJECT_TYPE_DESCRIPTOR_POOL,
        DESCRIPTOR_SET              = VK_OBJECT_TYPE_DESCRIPTOR_SET,
        DESCRIPTOR_SET_LAYOUT       = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
        DESCRIPTOR_UPDATE_TEMPLATE  = VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE,
        DEVICE                      = VK_OBJECT_TYPE_DEVICE,
        EVENT                       = VK_OBJECT_TYPE_EVENT,
        FENCE                       = VK_OBJECT_TYPE_FENCE,
        FRAMEBUFFER                 = VK_OBJECT_TYPE_FRAMEBUFFER,
        IMAGE                       = VK_OBJECT_TYPE_IMAGE,
        IMAGE_VIEW                  = VK_OBJECT_TYPE_IMAGE_VIEW,
        INSTANCE                    = VK_OBJECT_TYPE_INSTANCE,
        PHYSICAL_DEVICE             = VK_OBJECT_TYPE_PHYSICAL_DEVICE,
        PIPELINE                    = VK_OBJECT_TYPE_PIPELINE,
        PIPELINE_CACHE              = VK_OBJECT_TYPE_PIPELINE_CACHE,
        PIPELINE_LAYOUT             = VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        QUERY_POOL                  = VK_OBJECT_TYPE_QUERY_POOL,
        QUEUE                       = VK_OBJECT_TYPE_QUEUE,
        RENDER_PASS                 = VK_OBJECT_TYPE_RENDER_PASS,
        RENDERING_SURFACE           = VK_OBJECT_TYPE_SURFACE_KHR,
        SAMPLER                     = VK_OBJECT_TYPE_SAMPLER,
        SAMPLER_YCBCR_CONVERSION    = VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR,
        SEMAPHORE                   = VK_OBJECT_TYPE_SEMAPHORE,
        SHADER_MODULE               = VK_OBJECT_TYPE_SHADER_MODULE,
        SWAPCHAIN                   = VK_OBJECT_TYPE_SWAPCHAIN_KHR,

        /* Anvil-specific items */
        ANVIL_COMPUTE_PIPELINE_MANAGER = VK_OBJECT_TYPE_END_RANGE + 1,
        ANVIL_DESCRIPTOR_SET_GROUP,
        ANVIL_DESCRIPTOR_SET_LAYOUT_MANAGER,
        ANVIL_GLSL_SHADER_TO_SPIRV_GENERATOR,
        ANVIL_GRAPHICS_PIPELINE_MANAGER,
        ANVIL_MEMORY_BLOCK,
        ANVIL_PIPELINE_LAYOUT_MANAGER,

        /* Always last */
        UNKNOWN
    };

    /** Defines, to what extent occlusion queries are going to be used.
     *
     *  Only used for second-level command buffer recording policy declaration.
     **/
    enum class OcclusionQuerySupportScope
    {
        /** Occlusion queries are not going to be used */
        NOT_REQUIRED,

        /** Non-precise occlusion queries may be used */
        REQUIRED_NONPRECISE,

        /** Pprecise occlusion queries may be used */
        REQUIRED_PRECISE,
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class PipelineBindPoint
    {
        /* Core VK 1.0 */
        COMPUTE  = VK_PIPELINE_BIND_POINT_COMPUTE,
        GRAPHICS = VK_PIPELINE_BIND_POINT_GRAPHICS,

        UNKNOWN = VK_PIPELINE_BIND_POINT_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class PipelineStageFlagBits
    {
        /* Core VK 1.0 */
        ALL_COMMANDS_BIT                   = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        ALL_GRAPHICS_BIT                   = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        BOTTOM_OF_PIPE_BIT                 = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        COLOR_ATTACHMENT_OUTPUT_BIT        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        COMPUTE_SHADER_BIT                 = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        DRAW_INDIRECT_BIT                  = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        EARLY_FRAGMENT_TESTS_BIT           = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        FRAGMENT_SHADER_BIT                = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        GEOMETRY_SHADER_BIT                = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
        HOST_BIT                           = VK_PIPELINE_STAGE_HOST_BIT,
        LATE_FRAGMENT_TESTS_BIT            = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        TESSELLATION_CONTROL_SHADER_BIT    = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,
        TESSELLATION_EVALUATION_SHADER_BIT = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
        TOP_OF_PIPE_BIT                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        TRANSFER_BIT                       = VK_PIPELINE_STAGE_TRANSFER_BIT,
        VERTEX_INPUT_BIT                   = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VERTEX_SHADER_BIT                  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,

        /* VK_EXT_transform_feedback */
        TRANSFORM_FEEDBACK_BIT_EXT         = VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::PipelineStageFlagBits, VkPipelineStageFlags> PipelineStageFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(PipelineStageFlags, VkPipelineStageFlags, PipelineStageFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class PointClippingBehavior
    {
        ALL_CLIP_PLANES       = VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES_KHR,
        USER_CLIP_PLANES_ONLY = VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY_KHR,

        UNKNOWN = VK_POINT_CLIPPING_BEHAVIOR_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class PipelineCreateFlagBits
    {
        /* Core VK 1.0 */
        ALLOW_DERIVATIVES_BIT    = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT,
        DISABLE_OPTIMIZATION_BIT = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
        DERIVATIVE_BIT           = VK_PIPELINE_CREATE_DERIVATIVE_BIT,

        /* VK_KHR_multiview */
        VIEW_INDEX_FROM_DEVICE_INDEX_BIT = VK_PIPELINE_CREATE_VIEW_INDEX_FROM_DEVICE_INDEX_BIT_KHR,

        /* VK_KHR_device_group */
        DISPATCH_BASE_BIT = VK_PIPELINE_CREATE_DISPATCH_BASE_KHR,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::PipelineCreateFlagBits, VkPipelineCreateFlagBits> PipelineCreateFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(PipelineCreateFlags, VkPipelineCreateFlags, PipelineCreateFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class PolygonMode
    {
        FILL  = VK_POLYGON_MODE_FILL,
        LINE  = VK_POLYGON_MODE_LINE,
        POINT = VK_POLYGON_MODE_POINT,

        UNKNOWN = VK_POLYGON_MODE_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class PresentModeKHR
    {
        /* VK_KHR_surface */
        IMMEDIATE_KHR    = VK_PRESENT_MODE_IMMEDIATE_KHR,
        MAILBOX_KHR      = VK_PRESENT_MODE_MAILBOX_KHR,
        FIFO_KHR         = VK_PRESENT_MODE_FIFO_KHR,
        FIFO_RELAXED_KHR = VK_PRESENT_MODE_FIFO_RELAXED_KHR,

        UNKNOWN = VK_PRESENT_MODE_MAX_ENUM_KHR
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class PrimitiveTopology
    {
        LINE_LIST                     = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        LINE_LIST_WITH_ADJACENCY      = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
        LINE_STRIP                    = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        LINE_STRIP_WITH_ADJACENCY     = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
        PATCH_LIST                    = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
        POINT_LIST                    = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        TRIANGLE_FAN                  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
        TRIANGLE_LIST                 = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        TRIANGLE_LIST_WITH_ADJACENCY  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
        TRIANGLE_STRIP                = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        TRIANGLE_STRIP_WITH_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,

        UNKNOWN = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM
    };

    /** A bitmask defining one or more queue family usage.*/
    enum class QueueFamilyFlagBits
    {
        COMPUTE_BIT  = 1 << 0,
        DMA_BIT      = 1 << 1,
        GRAPHICS_BIT = 1 << 2,

        FIRST_BIT = COMPUTE_BIT,
        LAST_BIT  = GRAPHICS_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::QueueFamilyFlagBits, uint32_t> QueueFamilyFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(QueueFamilyFlags, uint32_t, QueueFamilyFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class QueueGlobalPriority
    {
        HIGH_EXT     = VK_QUEUE_GLOBAL_PRIORITY_HIGH_EXT,
        LOW_EXT      = VK_QUEUE_GLOBAL_PRIORITY_LOW_EXT,
        MEDIUM_EXT   = VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_EXT,
        REALTIME_EXT = VK_QUEUE_GLOBAL_PRIORITY_REALTIME_EXT,

        UNKNOWN = VK_QUEUE_GLOBAL_PRIORITY_MAX_ENUM_EXT
    };

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

    /* NOTE: These map 1:1 to VK equivalents */
    enum class QueueFlagBits
    {
        /* Core VK 1.0 */
        COMPUTE_BIT        = VK_QUEUE_COMPUTE_BIT,
        GRAPHICS_BIT       = VK_QUEUE_GRAPHICS_BIT,
        SPARSE_BINDING_BIT = VK_QUEUE_SPARSE_BINDING_BIT,
        TRANSFER_BIT       = VK_QUEUE_TRANSFER_BIT,

        /* Core VK 1.1 */
        PROTECTED_BIT = VK_QUEUE_PROTECTED_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::QueueFlagBits, VkQueueFlagBits> QueueFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(QueueFlags, VkQueueFlags, QueueFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class QueryControlFlagBits
    {
        PRECISE_BIT = VK_QUERY_CONTROL_PRECISE_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::QueryControlFlagBits, VkQueryControlFlagBits> QueryControlFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(QueryControlFlags, VkQueryControlFlags, QueryControlFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class QueryPipelineStatisticFlagBits
    {
        CLIPPING_INVOCATIONS_BIT                       = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT,
        CLIPPING_PRIMITIVES_BIT                        = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT,
        COMPUTE_SHADER_INVOCATIONS_BIT                 = VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT,
        FRAGMENT_SHADER_INVOCATIONS_BIT                = VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT,
        GEOMETRY_SHADER_INVOCATIONS_BIT                = VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT,
        GEOMETRY_SHADER_PRIMITIVES_BIT                 = VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT,
        INPUT_ASSEMBLY_VERTICES_BIT                    = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT,
        INPUT_ASSEMBLY_PRIMITIVES_BIT                  = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT,
        TESSELLATION_CONTROL_SHADER_PATCHES_BIT        = VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT,
        TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT = VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT,
        VERTEX_SHADER_INVOCATIONS_BIT                  = VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT,

        NONE = 0,
    };
    typedef Anvil::Bitfield<Anvil::QueryPipelineStatisticFlagBits, VkQueryPipelineStatisticFlagBits> QueryPipelineStatisticFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(QueryPipelineStatisticFlags, VkQueryPipelineStatisticFlags, QueryPipelineStatisticFlagBits)

    enum class QueryResultFlagBits
    {
        _64_BIT = VK_QUERY_RESULT_64_BIT,

        /* Implementation should wait for each query's status to become available before retrieving its results
         *
         * Core VK 1.0 functionality
         */
        WAIT_BIT = VK_QUERY_RESULT_WAIT_BIT,

        /* Each query result value is going to be followed by a status value. Non-zero values indicate result is
         * available.
         *
         * Core VK 1.0 functionality
         */
        WITH_AVAILABILITY_BIT = VK_QUERY_RESULT_WITH_AVAILABILITY_BIT,

        /* Indicates it is OK for the function to return result values for a sub-range of the requested query range.
         *
         * Core VK 1.0 frunctionality
         */
        PARTIAL_BIT = VK_QUERY_RESULT_PARTIAL_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::QueryResultFlagBits, VkQueryResultFlagBits> QueryResultFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(QueryResultFlags, VkQueryResultFlags, QueryResultFlagBits)

    enum class RasterizationOrderAMD
    {
        RELAXED = VK_RASTERIZATION_ORDER_RELAXED_AMD,
        STRICT  = VK_RASTERIZATION_ORDER_STRICT_AMD,

        UNKNOWN = VK_RASTERIZATION_ORDER_MAX_ENUM_AMD
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class ResolveModeFlagBits
    {
        /* VK_KHR_depth_stencil_resolve */

        SAMPLE_ZERO_BIT_KHR = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT_KHR,
        AVERAGE_BIT_KHR     = VK_RESOLVE_MODE_AVERAGE_BIT_KHR,
        MIN_BIT_KHR         = VK_RESOLVE_MODE_MIN_BIT_KHR,
        MAX_BIT_KHR         = VK_RESOLVE_MODE_MAX_BIT_KHR,

        NONE = VK_RESOLVE_MODE_NONE_KHR,
    };
    typedef Anvil::Bitfield<Anvil::ResolveModeFlagBits, VkResolveModeFlagBitsKHR> ResolveModeFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(ResolveModeFlags, VkResolveModeFlagsKHR, ResolveModeFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class SampleCountFlagBits
    {
        _1_BIT  = VK_SAMPLE_COUNT_1_BIT,
        _2_BIT  = VK_SAMPLE_COUNT_2_BIT,
        _4_BIT  = VK_SAMPLE_COUNT_4_BIT,
        _8_BIT  = VK_SAMPLE_COUNT_8_BIT,
        _16_BIT = VK_SAMPLE_COUNT_16_BIT,
        _32_BIT = VK_SAMPLE_COUNT_32_BIT,
        _64_BIT = VK_SAMPLE_COUNT_64_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::SampleCountFlagBits, VkSampleCountFlagBits> SampleCountFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(SampleCountFlags, VkSampleCountFlags, SampleCountFlagBits)

    /* NOTE: These map 1:1 to VK equivalents */
    enum class SamplerAddressMode
    {
        /* Core VK 1.0 */
        CLAMP_TO_BORDER      = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        CLAMP_TO_EDGE        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        MIRRORED_REPEAT      = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        REPEAT               = VK_SAMPLER_ADDRESS_MODE_REPEAT,

        /* VK_KHR_sampler_mirror_clamp_to_edge */
        MIRROR_CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,

        UNKNOWN = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class SamplerMipmapMode
    {
        LINEAR  = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        NEAREST = VK_SAMPLER_MIPMAP_MODE_NEAREST,

        UNKNOWN = VK_SAMPLER_MIPMAP_MODE_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class SamplerReductionMode
    {
        /* VK_EXT_sampler_filter_minmax */
        WEIGHTED_AVERAGE_EXT = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT,
        MAX_EXT              = VK_SAMPLER_REDUCTION_MODE_MAX_EXT,
        MIN_EXT              = VK_SAMPLER_REDUCTION_MODE_MIN_EXT,

        UNKNOWN = VK_SAMPLER_REDUCTION_MODE_MAX_ENUM_EXT
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class SamplerYCbCrModelConversion
    {
        RGB_IDENTITY_KHR   = VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR,
        YCBCR_IDENTITY_KHR = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY_KHR,
        YCBCR_709_KHR      = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709_KHR,
        YCBCR_601_KHR      = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601_KHR,
        YCBCR_2020_KHR     = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020_KHR,

        UNKNOWN = VK_SAMPLER_YCBCR_MODEL_CONVERSION_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class SamplerYCbCrRange
    {
        RGB_IDENTITY_KHR   = VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR,
        YCBCR_IDENTITY_KHR = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY_KHR,
        YCBCR_709_KHR      = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709_KHR,
        YCBCR_601_KHR      = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601_KHR,
        YCBCR_2020_KHR     = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020_KHR,

        UNKNOWN = VK_SAMPLER_YCBCR_MODEL_CONVERSION_MAX_ENUM
    };

    /* Specifies one of the compute / rendering pipeline stages. */
    enum class ShaderStage
    {
        FIRST,

        COMPUTE = FIRST,
        FRAGMENT,
        GEOMETRY,
        TESSELLATION_CONTROL,
        TESSELLATION_EVALUATION,
        VERTEX,

        COUNT,
        UNKNOWN = COUNT
    };

    /* NOTE: Maps 1:1 to VK equivalents */
    enum class ShaderStageFlagBits
    {
        COMPUTE_BIT                 = VK_SHADER_STAGE_COMPUTE_BIT,
        FRAGMENT_BIT                = VK_SHADER_STAGE_FRAGMENT_BIT,
        GEOMETRY_BIT                = VK_SHADER_STAGE_GEOMETRY_BIT,
        TESSELLATION_CONTROL_BIT    = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
        TESSELLATION_EVALUATION_BIT = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
        VERTEX_BIT                  = VK_SHADER_STAGE_VERTEX_BIT,

        ALL          = VK_SHADER_STAGE_ALL,
        ALL_GRAPHICS = VK_SHADER_STAGE_ALL_GRAPHICS,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::ShaderStageFlagBits, VkShaderStageFlags> ShaderStageFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(ShaderStageFlags, VkShaderStageFlags, ShaderStageFlagBits)

    /* Specifies the type of query for post-compile information about pipeline shaders */
    enum class ShaderInfoType
    {
        FIRST,

        BINARY      = FIRST,
        DISASSEMBLY,

        COUNT,
        UNKNOWN = COUNT
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class SharingMode
    {
        CONCURRENT = VK_SHARING_MODE_CONCURRENT,
        EXCLUSIVE  = VK_SHARING_MODE_EXCLUSIVE,

        UNKNOWN
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class SparseImageFormatFlagBits
    {
        /* Core VK 1.0 */
        ALIGNED_MIP_SIZE_BIT        = VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT,
        NONNSTANDARD_BLOCK_SIZE_BIT = VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT,
        SINGLE_MIPTAIL_BIT          = VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::SparseImageFormatFlagBits, VkSparseImageFormatFlags> SparseImageFormatFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(SparseImageFormatFlags, VkSparseImageFormatFlags, SparseImageFormatFlagBits)

    enum class SparseMemoryBindFlagBits
    {
        /* Core VK 1.0 */
        BIND_METADATA_BIT = VK_SPARSE_MEMORY_BIND_METADATA_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::SparseMemoryBindFlagBits, VkSparseMemoryBindFlags> SparseMemoryBindFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(SparseMemoryBindFlags, VkSparseMemoryBindFlags, SparseMemoryBindFlagBits)

    /* Specifies SPIR-V language version */
    enum class SpvVersion
    {
        _1_0,
        _1_1,
        _1_2,
        _1_3,
        _1_4,
        UNKNOWN
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class StencilFaceFlagBits
    {
        BACK_BIT  = VK_STENCIL_FACE_BACK_BIT,
        FRONT_BIT = VK_STENCIL_FACE_FRONT_BIT,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::StencilFaceFlagBits, VkStencilFaceFlags> StencilFaceFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(StencilFaceFlags, VkStencilFaceFlags, StencilFaceFlagBits)

    enum class StencilOp
    {
        DECREMENT_AND_CLAMP = VK_STENCIL_OP_DECREMENT_AND_CLAMP,
        DECREMENT_AND_WRAP  = VK_STENCIL_OP_DECREMENT_AND_WRAP,
        INCREMENT_AND_CLAMP = VK_STENCIL_OP_INCREMENT_AND_CLAMP,
        INCREMENT_AND_WRAP  = VK_STENCIL_OP_INCREMENT_AND_WRAP,
        INVERT              = VK_STENCIL_OP_INVERT,
        KEEP                = VK_STENCIL_OP_KEEP,
        REPLACE             = VK_STENCIL_OP_REPLACE,
        ZERO                = VK_STENCIL_OP_ZERO,

        UNKNOWN = VK_STENCIL_OP_MAX_ENUM
    };

    /* NOTE: These map 1:1 to VK equivalents */
    enum class SubgroupFeatureFlagBits
    {
        ARITHMETIC_BIT = VK_SUBGROUP_FEATURE_ARITHMETIC_BIT,
        BALLOT_BIT     = VK_SUBGROUP_FEATURE_BALLOT_BIT,
        BASIC_BIT      = VK_SUBGROUP_FEATURE_BASIC_BIT,
        CLUSTERED_BIT  = VK_SUBGROUP_FEATURE_CLUSTERED_BIT,
        QUAD_BIT       = VK_SUBGROUP_FEATURE_QUAD_BIT,
        RELATIVE_BIT   = VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT,
        SHUFFLE_BIT    = VK_SUBGROUP_FEATURE_SHUFFLE_BIT,
        VOTE_BIT       = VK_SUBGROUP_FEATURE_VOTE_BIT,

        NONE = VK_SUBGROUP_FEATURE_FLAG_BITS_MAX_ENUM
    };

    typedef Anvil::Bitfield<Anvil::SubgroupFeatureFlagBits, VkSubgroupFeatureFlagBits> SubgroupFeatureFlags;

    /* NOTE: These map 1:1 to VK equivalents */
    enum class SubpassContents
    {
        INLINE                    = VK_SUBPASS_CONTENTS_INLINE,
        SECONDARY_COMMAND_BUFFERS = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS,

        UNKNOWN = VK_SUBPASS_CONTENTS_MAX_ENUM
    };

    enum class SurfaceTransformFlagBits
    {
        HORIZONTAL_MIRROR_BIT_KHR            = VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR,
        HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR = VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR,
        HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR = VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR,
        HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR  = VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR,
        IDENTITY_BIT_KHR                     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        INHERIT_BIT_KHR                      = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR,
        ROTATE_180_BIT_KHR                   = VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR,
        ROTATE_270_BIT_KHR                   = VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR,
        ROTATE_90_BIT_KHR                    = VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::SurfaceTransformFlagBits, VkSurfaceTransformFlagsKHR> SurfaceTransformFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(SurfaceTransformFlags, VkSurfaceTransformFlagsKHR, SurfaceTransformFlagBits)

    /* NOTE: Enums map 1:1 to their VK equivalents */
    enum class SwapchainCreateFlagBits
    {
        /* Requires VK_KHR_device_group */
        SPLIT_INSTANCE_BIND_REGIONS_BIT = VK_SWAPCHAIN_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR,

        /* Requires VK_KHR_swapchain_mutable_format */
        CREATE_MUTABLE_FORMAT_BIT = VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR,

        NONE = 0
    };
    typedef Anvil::Bitfield<Anvil::SwapchainCreateFlagBits, VkSwapchainCreateFlagsKHR> SwapchainCreateFlags;

    INJECT_BITFIELD_HELPER_FUNC_PROTOTYPES(SwapchainCreateFlags, VkSwapchainCreateFlagsKHR, SwapchainCreateFlagBits)

    /* Note: These map 1:1 to corresponding VK error & VK swapchain error codes. */
    enum class SwapchainOperationErrorCode
    {
        DEVICE_LOST  = VK_ERROR_DEVICE_LOST,
        OUT_OF_DATE  = VK_ERROR_OUT_OF_DATE_KHR,
        SUBOPTIMAL   = VK_SUBOPTIMAL_KHR,
        SUCCESS      = VK_SUCCESS,
        SURFACE_LOST = VK_ERROR_SURFACE_LOST_KHR
    };

    /* NOTE: Enums map 1:1 to their VK equivalents */
    enum class TessellationDomainOrigin
    {
        LOWER_LEFT = VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT_KHR,
        UPPER_LEFT = VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT_KHR,

        UNKNOWN = VK_TESSELLATION_DOMAIN_ORIGIN_MAX_ENUM
    };

    enum class VertexInputRate
    {
        INSTANCE = VK_VERTEX_INPUT_RATE_INSTANCE,
        VERTEX   = VK_VERTEX_INPUT_RATE_VERTEX,

        UNKNOWN = VK_VERTEX_INPUT_RATE_MAX_ENUM
    };
}; /* namespace Anvil */

#endif /* TYPES_ENUMS_H */
