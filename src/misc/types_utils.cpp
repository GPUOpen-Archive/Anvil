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
#include "misc/types.h"
#include "wrappers/device.h"

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

VkExternalMemoryHandleTypeFlagsKHR Anvil::Utils::convert_external_memory_handle_types_to_vk_external_memory_handle_type_flags(const Anvil::ExternalMemoryHandleTypeFlags& in_flags)
{
    ANVIL_REDUNDANT_ARGUMENT_CONST(in_flags);
    anvil_assert                  (in_flags == 0);

    return 0;
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
        {Anvil::QUEUE_FAMILY_COMPUTE_BIT,  Anvil::QueueFamilyType::COMPUTE},
        {Anvil::QUEUE_FAMILY_DMA_BIT,      Anvil::QueueFamilyType::TRANSFER},
        {Anvil::QUEUE_FAMILY_GRAPHICS_BIT, Anvil::QueueFamilyType::UNIVERSAL},
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
