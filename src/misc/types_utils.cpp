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
#include "wrappers/buffer.h"
#include "wrappers/device.h"

/** Please see header for specification */
void Anvil::Utils::get_version_chunks_for_api_version(const Anvil::APIVersion& in_api_version,
                                                      uint32_t*                out_major_version_ptr,
                                                      uint32_t*                out_minor_version_ptr)
{
    switch (in_api_version)
    {
        case Anvil::APIVersion::_1_0:
        {
            *out_major_version_ptr = 1;
            *out_minor_version_ptr = 0;

            break;
        }

        case Anvil::APIVersion::_1_1:
        {
            *out_major_version_ptr = 1;
            *out_minor_version_ptr = 1;

            break;
        }

        default:
        {
            /* in_api_version must NOT be Anvil::APIVersion::UNKNOWN! */
            anvil_assert_fail();
        }
    }
}

/** Please see header for specification */
Anvil::MemoryFeatureFlags Anvil::Utils::get_memory_feature_flags_from_vk_property_flags(Anvil::MemoryPropertyFlags in_mem_type_flags,
                                                                                        Anvil::MemoryHeapFlags     in_mem_heap_flags)
{
    Anvil::MemoryFeatureFlags result;

    if ((in_mem_type_flags & Anvil::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT) != 0)
    {
        result |= Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT;
    }

    if ((in_mem_type_flags & Anvil::MemoryPropertyFlagBits::HOST_VISIBLE_BIT) != 0)
    {
        result |= Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT;
    }

    if ((in_mem_type_flags & Anvil::MemoryPropertyFlagBits::HOST_COHERENT_BIT) != 0)
    {
        result |= Anvil::MemoryFeatureFlagBits::HOST_COHERENT_BIT;
    }

    if ((in_mem_type_flags & Anvil::MemoryPropertyFlagBits::HOST_CACHED_BIT) != 0)
    {
        result |= Anvil::MemoryFeatureFlagBits::HOST_CACHED_BIT;
    }

    if ((in_mem_type_flags & Anvil::MemoryPropertyFlagBits::LAZILY_ALLOCATED_BIT) != 0)
    {
        result |= Anvil::MemoryFeatureFlagBits::LAZILY_ALLOCATED_BIT;
    }

    if ((in_mem_heap_flags & Anvil::MemoryHeapFlagBits::MULTI_INSTANCE_BIT_KHR) != 0)
    {
        result |= Anvil::MemoryFeatureFlagBits::MULTI_INSTANCE_BIT;
    }

    if ((in_mem_type_flags & Anvil::MemoryPropertyFlagBits::PROTECTED_BIT) != 0)
    {
        result |= Anvil::MemoryFeatureFlagBits::PROTECTED_BIT;
    }

    return result;
}

Anvil::MTSafety Anvil::Utils::convert_boolean_to_mt_safety_enum(bool in_mt_safe)
{
    return (in_mt_safe) ? Anvil::MTSafety::ENABLED
                        : Anvil::MTSafety::DISABLED;
}

bool Anvil::Utils::convert_mt_safety_enum_to_boolean(Anvil::MTSafety          in_mt_safety,
                                                     const Anvil::BaseDevice* in_device_ptr)
{
    bool result = false;

    switch (in_mt_safety)
    {
        case Anvil::MTSafety::DISABLED: result = false; break;
        case Anvil::MTSafety::ENABLED:  result = true;  break;

        case Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE:
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
                                                               Anvil::QueueFamilyFlags  in_queue_families,
                                                               uint32_t*                out_opt_queue_family_indices_ptr,
                                                               uint32_t*                out_opt_n_queue_family_indices_ptr)
{
    uint32_t n_result_queue_family_indices(0);

    static const struct
    {
        Anvil::QueueFamilyFlagBits queue_family;
        Anvil::QueueFamilyType     queue_family_type;
    } queue_family_data[] =
    {
        {Anvil::QueueFamilyFlagBits::COMPUTE_BIT,     Anvil::QueueFamilyType::COMPUTE},
        {Anvil::QueueFamilyFlagBits::DMA_BIT,         Anvil::QueueFamilyType::TRANSFER},
        {Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,    Anvil::QueueFamilyType::UNIVERSAL},
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
Anvil::AccessFlags Anvil::Utils::get_access_mask_from_image_layout(Anvil::ImageLayout     in_layout,
                                                                   Anvil::QueueFamilyType in_queue_family_type)
{
    Anvil::AccessFlags result;

    switch (in_layout)
    {
        case Anvil::ImageLayout::UNDEFINED:
        {
            break;
        }

        case Anvil::ImageLayout::GENERAL:
        {
            result = Anvil::AccessFlagBits::INDIRECT_COMMAND_READ_BIT          |
                     Anvil::AccessFlagBits::INDEX_READ_BIT                     |
                     Anvil::AccessFlagBits::VERTEX_ATTRIBUTE_READ_BIT          |
                     Anvil::AccessFlagBits::UNIFORM_READ_BIT                   |
                     Anvil::AccessFlagBits::INPUT_ATTACHMENT_READ_BIT          |
                     Anvil::AccessFlagBits::SHADER_READ_BIT                    |
                     Anvil::AccessFlagBits::SHADER_WRITE_BIT                   |
                     Anvil::AccessFlagBits::COLOR_ATTACHMENT_READ_BIT          |
                     Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT         |
                     Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_READ_BIT  |
                     Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                     Anvil::AccessFlagBits::TRANSFER_READ_BIT                  |
                     Anvil::AccessFlagBits::TRANSFER_WRITE_BIT                 |
                     Anvil::AccessFlagBits::HOST_READ_BIT                      |
                     Anvil::AccessFlagBits::HOST_WRITE_BIT                     |
                     Anvil::AccessFlagBits::MEMORY_READ_BIT                    |
                     Anvil::AccessFlagBits::MEMORY_WRITE_BIT;

            break;
        }

        case Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
        {
            result = Anvil::AccessFlagBits::COLOR_ATTACHMENT_READ_BIT |
                     Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT;

            break;
        }

        case Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        {
            result = Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_READ_BIT  |
                     Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            break;
        }

        case Anvil::ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        {
            result = Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_READ_BIT;

            break;
        }

        case Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL:
        {
            result = Anvil::AccessFlagBits::SHADER_READ_BIT;

            break;
        }

        case Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL:
        {
            result = Anvil::AccessFlagBits::TRANSFER_READ_BIT;

            break;
        }

        case Anvil::ImageLayout::TRANSFER_DST_OPTIMAL:
        {
            result = Anvil::AccessFlagBits::TRANSFER_WRITE_BIT;

            break;
        }

        case Anvil::ImageLayout::PREINITIALIZED:
        {
            result = Anvil::AccessFlagBits::SHADER_READ_BIT;

            break;
        }

        case Anvil::ImageLayout::PRESENT_SRC_KHR:
        {
            result = Anvil::AccessFlagBits::MEMORY_READ_BIT;

            break;
        }

        default:
        {
            /* Invalid Anvil::ImageLayout argument value */
            anvil_assert_fail();
        }
    }

    switch (in_queue_family_type)
    {
        case Anvil::QueueFamilyType::COMPUTE:
        {
            result &= (Anvil::AccessFlagBits::INDIRECT_COMMAND_READ_BIT |
                       Anvil::AccessFlagBits::MEMORY_READ_BIT           |
                       Anvil::AccessFlagBits::MEMORY_WRITE_BIT          |
                       Anvil::AccessFlagBits::SHADER_READ_BIT           |
                       Anvil::AccessFlagBits::SHADER_WRITE_BIT          |
                       Anvil::AccessFlagBits::TRANSFER_READ_BIT         |
                       Anvil::AccessFlagBits::TRANSFER_WRITE_BIT        |
                       Anvil::AccessFlagBits::UNIFORM_READ_BIT);

            break;
        }

        case Anvil::QueueFamilyType::TRANSFER:
        {
            result &= (Anvil::AccessFlagBits::MEMORY_READ_BIT    |
                       Anvil::AccessFlagBits::MEMORY_WRITE_BIT   |
                       Anvil::AccessFlagBits::TRANSFER_READ_BIT  |
                       Anvil::AccessFlagBits::TRANSFER_WRITE_BIT);

            break;
        }

        case Anvil::QueueFamilyType::UNIVERSAL:
        {
            result &= (Anvil::AccessFlagBits::COLOR_ATTACHMENT_READ_BIT          |
                       Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT         |
                       Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_READ_BIT  |
                       Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                       Anvil::AccessFlagBits::INDIRECT_COMMAND_READ_BIT          |
                       Anvil::AccessFlagBits::INDEX_READ_BIT                     |
                       Anvil::AccessFlagBits::MEMORY_READ_BIT                    |
                       Anvil::AccessFlagBits::MEMORY_WRITE_BIT                   |
                       Anvil::AccessFlagBits::SHADER_READ_BIT                    |
                       Anvil::AccessFlagBits::SHADER_WRITE_BIT                   |
                       Anvil::AccessFlagBits::TRANSFER_READ_BIT                  |
                       Anvil::AccessFlagBits::TRANSFER_WRITE_BIT                 |
                       Anvil::AccessFlagBits::UNIFORM_READ_BIT                   |
                       Anvil::AccessFlagBits::VERTEX_ATTRIBUTE_READ_BIT);

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
Anvil::ObjectType Anvil::Utils::get_object_type_for_vk_debug_report_object_type(const VkDebugReportObjectTypeEXT& in_object_type)
{
    Anvil::ObjectType result = Anvil::ObjectType::UNKNOWN;

    switch (in_object_type)
    {
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:                         result = Anvil::ObjectType::BUFFER;                     break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:                    result = Anvil::ObjectType::BUFFER_VIEW;                break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:                 result = Anvil::ObjectType::COMMAND_BUFFER;             break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:                   result = Anvil::ObjectType::COMMAND_POOL;               break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT:      result = Anvil::ObjectType::DEBUG_REPORT_CALLBACK;      break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:                result = Anvil::ObjectType::DESCRIPTOR_POOL;            break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:                 result = Anvil::ObjectType::DESCRIPTOR_SET;             break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:          result = Anvil::ObjectType::DESCRIPTOR_SET_LAYOUT;      break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT: result = Anvil::ObjectType::DESCRIPTOR_UPDATE_TEMPLATE; break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:                         result = Anvil::ObjectType::DEVICE;                     break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:                  result = Anvil::ObjectType::ANVIL_MEMORY_BLOCK;         break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:                          result = Anvil::ObjectType::EVENT;                      break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:                          result = Anvil::ObjectType::FENCE;                      break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:                    result = Anvil::ObjectType::FRAMEBUFFER;                break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:                          result = Anvil::ObjectType::IMAGE;                      break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:                     result = Anvil::ObjectType::IMAGE_VIEW;                 break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:                       result = Anvil::ObjectType::INSTANCE;                   break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:                result = Anvil::ObjectType::PHYSICAL_DEVICE;            break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:                 result = Anvil::ObjectType::PIPELINE_CACHE;             break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:                result = Anvil::ObjectType::PIPELINE_LAYOUT;            break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:                          result = Anvil::ObjectType::QUEUE;                      break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:                     result = Anvil::ObjectType::QUERY_POOL;                 break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:                    result = Anvil::ObjectType::RENDER_PASS;                break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:                        result = Anvil::ObjectType::SAMPLER;                    break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:                      result = Anvil::ObjectType::SEMAPHORE;                  break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:                  result = Anvil::ObjectType::SHADER_MODULE;              break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:                    result = Anvil::ObjectType::RENDERING_SURFACE;          break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:                  result = Anvil::ObjectType::SWAPCHAIN;                  break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
Anvil::ObjectType Anvil::Utils::get_object_type_for_vk_object_type(const VkObjectType& in_object_type)
{
    return static_cast<Anvil::ObjectType>(in_object_type);
}

/* Please see header for specification */
Anvil::QueueFamilyFlags Anvil::Utils::get_queue_family_flags_from_queue_family_type(Anvil::QueueFamilyType in_queue_family_type)
{
    Anvil::QueueFamilyFlags result;

    switch (in_queue_family_type)
    {
        case Anvil::QueueFamilyType::COMPUTE:   result = Anvil::QueueFamilyFlagBits::COMPUTE_BIT;                                            break;
        case Anvil::QueueFamilyType::TRANSFER:  result = Anvil::QueueFamilyFlagBits::DMA_BIT;                                                break;
        case Anvil::QueueFamilyType::UNIVERSAL: result = Anvil::QueueFamilyFlagBits::COMPUTE_BIT | Anvil::QueueFamilyFlagBits::GRAPHICS_BIT; break;

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
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:                       result = "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL";                       break;
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR: result = "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR"; break;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR: result = "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR"; break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:               result = "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL";               break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:                result = "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL";                break;
        case VK_IMAGE_LAYOUT_GENERAL:                                        result = "VK_IMAGE_LAYOUT_GENERAL";                                        break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:                                 result = "VK_IMAGE_LAYOUT_PREINITIALIZED";                                 break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                                result = "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR";                                break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:                       result = "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL";                       break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:                           result = "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL";                           break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:                           result = "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL";                           break;
        case VK_IMAGE_LAYOUT_UNDEFINED:                                      result = "VK_IMAGE_LAYOUT_UNDEFINED";                                      break;

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
        case ShaderStage::COMPUTE:                 result = "SHADER_STAGE_COMPUTE";                 break;
        case ShaderStage::FRAGMENT:                result = "SHADER_STAGE_FRAGMENT";                break;
        case ShaderStage::GEOMETRY:                result = "SHADER_STAGE_GEOMETRY";                break;
        case ShaderStage::TESSELLATION_CONTROL:    result = "SHADER_STAGE_TESSELLATION_CONTROL";    break;
        case ShaderStage::TESSELLATION_EVALUATION: result = "SHADER_STAGE_TESSELLATION_EVALUATION"; break;
        case ShaderStage::VERTEX:                  result = "SHADER_STAGE_VERTEX";                  break;

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
Anvil::ShaderStageFlagBits Anvil::Utils::get_shader_stage_flag_bits_from_shader_stage(Anvil::ShaderStage in_shader_stage)
{
    Anvil::ShaderStageFlagBits result = static_cast<Anvil::ShaderStageFlagBits>(0);

    switch (in_shader_stage)
    {
        case Anvil::ShaderStage::COMPUTE:                 result = Anvil::ShaderStageFlagBits::COMPUTE_BIT;                 break;
        case Anvil::ShaderStage::FRAGMENT:                result = Anvil::ShaderStageFlagBits::FRAGMENT_BIT;                break;
        case Anvil::ShaderStage::GEOMETRY:                result = Anvil::ShaderStageFlagBits::GEOMETRY_BIT;                break;
        case Anvil::ShaderStage::TESSELLATION_CONTROL:    result = Anvil::ShaderStageFlagBits::TESSELLATION_CONTROL_BIT;    break;
        case Anvil::ShaderStage::TESSELLATION_EVALUATION: result = Anvil::ShaderStageFlagBits::TESSELLATION_EVALUATION_BIT; break;
        case Anvil::ShaderStage::VERTEX:                  result = Anvil::ShaderStageFlagBits::VERTEX_BIT;                  break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/* Please see header for specification */
VkDebugReportObjectTypeEXT Anvil::Utils::get_vk_debug_report_object_type_ext_from_object_type(const Anvil::ObjectType& in_object_type)
{
    VkDebugReportObjectTypeEXT result = VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT;

    switch (in_object_type)
    {
        case Anvil::ObjectType::BUFFER:                     result = VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT;                     break;
        case Anvil::ObjectType::BUFFER_VIEW:                result = VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT;                break;
        case Anvil::ObjectType::COMMAND_BUFFER:             result = VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT;             break;
        case Anvil::ObjectType::COMMAND_POOL:               result = VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT;               break;
        case Anvil::ObjectType::DESCRIPTOR_POOL:            result = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT;            break;
        case Anvil::ObjectType::DESCRIPTOR_SET:             result = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT;             break;
        case Anvil::ObjectType::DESCRIPTOR_SET_LAYOUT:      result = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT;      break;
        case Anvil::ObjectType::DESCRIPTOR_UPDATE_TEMPLATE: result = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT; break;
        case Anvil::ObjectType::DEVICE:                     result = VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT;                     break;
        case Anvil::ObjectType::EVENT:                      result = VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT;                      break;
        case Anvil::ObjectType::FENCE:                      result = VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT;                      break;
        case Anvil::ObjectType::FRAMEBUFFER:                result = VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT;                break;
        case Anvil::ObjectType::IMAGE:                      result = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;                      break;
        case Anvil::ObjectType::IMAGE_VIEW:                 result = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT;                 break;
        case Anvil::ObjectType::INSTANCE:                   result = VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT;                   break;
        case Anvil::ObjectType::PHYSICAL_DEVICE:            result = VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT;            break;
        case Anvil::ObjectType::PIPELINE_CACHE:             result = VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT;             break;
        case Anvil::ObjectType::PIPELINE_LAYOUT:            result = VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT;            break;
        case Anvil::ObjectType::QUERY_POOL:                 result = VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT;                 break;
        case Anvil::ObjectType::QUEUE:                      result = VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT;                      break;
        case Anvil::ObjectType::RENDER_PASS:                result = VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT;                break;
        case Anvil::ObjectType::RENDERING_SURFACE:          result = VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT;                break;
        case Anvil::ObjectType::SAMPLER:                    result = VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT;                    break;
        case Anvil::ObjectType::SEMAPHORE:                  result = VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT;                  break;
        case Anvil::ObjectType::SHADER_MODULE:              result = VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT;              break;
        case Anvil::ObjectType::SWAPCHAIN:                  result = VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT;              break;

        default:
        {
            /* Either Anvil-specific object OR one that is not recognized by VK_EXT_debug_utils. The extension has
             * been deprecated, so there's not much we can do at this point, other than return MAX_ENUM_EXT and hope
             * for the best.
             */
        }
    }

    return result;
}

/* Please see header for specification */
VkObjectType Anvil::Utils::get_vk_object_type_for_object_type(const Anvil::ObjectType& in_object_type)
{
    VkObjectType result = VK_OBJECT_TYPE_MAX_ENUM;

    switch (in_object_type)
    {
        case Anvil::ObjectType::BUFFER:                     result = VK_OBJECT_TYPE_BUFFER;                     break;
        case Anvil::ObjectType::BUFFER_VIEW:                result = VK_OBJECT_TYPE_BUFFER_VIEW;                break;
        case Anvil::ObjectType::COMMAND_BUFFER:             result = VK_OBJECT_TYPE_COMMAND_BUFFER;             break;
        case Anvil::ObjectType::COMMAND_POOL:               result = VK_OBJECT_TYPE_COMMAND_POOL;               break;
        case Anvil::ObjectType::DESCRIPTOR_POOL:            result = VK_OBJECT_TYPE_DESCRIPTOR_POOL;            break;
        case Anvil::ObjectType::DESCRIPTOR_SET:             result = VK_OBJECT_TYPE_DESCRIPTOR_SET;             break;
        case Anvil::ObjectType::DESCRIPTOR_SET_LAYOUT:      result = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;      break;
        case Anvil::ObjectType::DESCRIPTOR_UPDATE_TEMPLATE: result = VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE; break;
        case Anvil::ObjectType::DEVICE:                     result = VK_OBJECT_TYPE_DEVICE;                     break;
        case Anvil::ObjectType::EVENT:                      result = VK_OBJECT_TYPE_EVENT;                      break;
        case Anvil::ObjectType::FENCE:                      result = VK_OBJECT_TYPE_FENCE;                      break;
        case Anvil::ObjectType::FRAMEBUFFER:                result = VK_OBJECT_TYPE_FRAMEBUFFER;                break;
        case Anvil::ObjectType::IMAGE:                      result = VK_OBJECT_TYPE_IMAGE;                      break;
        case Anvil::ObjectType::IMAGE_VIEW:                 result = VK_OBJECT_TYPE_IMAGE_VIEW;                 break;
        case Anvil::ObjectType::INSTANCE:                   result = VK_OBJECT_TYPE_INSTANCE;                   break;
        case Anvil::ObjectType::PHYSICAL_DEVICE:            result = VK_OBJECT_TYPE_PHYSICAL_DEVICE;            break;
        case Anvil::ObjectType::PIPELINE_CACHE:             result = VK_OBJECT_TYPE_PIPELINE_CACHE;             break;
        case Anvil::ObjectType::PIPELINE_LAYOUT:            result = VK_OBJECT_TYPE_PIPELINE_LAYOUT;            break;
        case Anvil::ObjectType::QUERY_POOL:                 result = VK_OBJECT_TYPE_QUERY_POOL;                 break;
        case Anvil::ObjectType::QUEUE:                      result = VK_OBJECT_TYPE_QUEUE;                      break;
        case Anvil::ObjectType::RENDER_PASS:                result = VK_OBJECT_TYPE_RENDER_PASS;                break;
        case Anvil::ObjectType::RENDERING_SURFACE:          result = VK_OBJECT_TYPE_SURFACE_KHR;                break;
        case Anvil::ObjectType::SAMPLER:                    result = VK_OBJECT_TYPE_SAMPLER;                    break;
        case Anvil::ObjectType::SEMAPHORE:                  result = VK_OBJECT_TYPE_SEMAPHORE;                  break;
        case Anvil::ObjectType::SHADER_MODULE:              result = VK_OBJECT_TYPE_SHADER_MODULE;              break;
        case Anvil::ObjectType::SWAPCHAIN:                  result = VK_OBJECT_TYPE_SWAPCHAIN_KHR;              break;

        default:
        {
            /* Either Anvil-specific object OR one that is not recognized by VK_EXT_debug_utils. The extension has
             * been deprecated, so there's not much we can do at this point, other than return MAX_ENUM_EXT and hope
             * for the best.
             */
        }
    }

    return result;
}

/* Please see header for specification */
void Anvil::Utils::get_vk_property_flags_from_memory_feature_flags(Anvil::MemoryFeatureFlags   in_mem_feature_flags,
                                                                   Anvil::MemoryPropertyFlags* out_mem_type_flags_ptr,
                                                                   Anvil::MemoryHeapFlags*     out_mem_heap_flags_ptr)
{
    Anvil::MemoryHeapFlags     result_mem_heap_flags;
    Anvil::MemoryPropertyFlags result_mem_type_flags;

    if ((in_mem_feature_flags & Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT) != 0)
    {
        result_mem_type_flags |= Anvil::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT;
    }

    if ((in_mem_feature_flags & Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) != 0)
    {
        result_mem_type_flags |= Anvil::MemoryPropertyFlagBits::HOST_VISIBLE_BIT;
    }

    if ((in_mem_feature_flags & Anvil::MemoryFeatureFlagBits::HOST_COHERENT_BIT) != 0)
    {
        result_mem_type_flags |= Anvil::MemoryPropertyFlagBits::HOST_COHERENT_BIT;
    }

    if ((in_mem_feature_flags & Anvil::MemoryFeatureFlagBits::HOST_CACHED_BIT) != 0)
    {
        result_mem_type_flags |= Anvil::MemoryPropertyFlagBits::HOST_CACHED_BIT;
    }

    if ((in_mem_feature_flags & Anvil::MemoryFeatureFlagBits::LAZILY_ALLOCATED_BIT) != 0)
    {
        result_mem_type_flags |= Anvil::MemoryPropertyFlagBits::LAZILY_ALLOCATED_BIT;
    }

    if ((in_mem_feature_flags & Anvil::MemoryFeatureFlagBits::MULTI_INSTANCE_BIT) != 0)
    {
        result_mem_heap_flags |= Anvil::MemoryHeapFlagBits::MULTI_INSTANCE_BIT_KHR;
    }

    if ((in_mem_feature_flags & Anvil::MemoryFeatureFlagBits::PROTECTED_BIT) != 0)
    {
        result_mem_type_flags |= Anvil::MemoryPropertyFlagBits::PROTECTED_BIT;
    }

    *out_mem_heap_flags_ptr = result_mem_heap_flags;
    *out_mem_type_flags_ptr = result_mem_type_flags;
}

#ifdef _WIN32
    bool Anvil::Utils::is_nt_handle(const Anvil::ExternalFenceHandleTypeFlagBits& in_type)
    {
        return (in_type == Anvil::ExternalFenceHandleTypeFlagBits::OPAQUE_WIN32_BIT);
    }

    bool Anvil::Utils::is_nt_handle(const Anvil::ExternalMemoryHandleTypeFlagBits& in_type)
    {
        return (in_type == Anvil::ExternalMemoryHandleTypeFlagBits::D3D11_TEXTURE_BIT  ||
                in_type == Anvil::ExternalMemoryHandleTypeFlagBits::D3D12_HEAP_BIT     ||
                in_type == Anvil::ExternalMemoryHandleTypeFlagBits::D3D12_RESOURCE_BIT ||
                in_type == Anvil::ExternalMemoryHandleTypeFlagBits::OPAQUE_WIN32_BIT);
    }

    bool Anvil::Utils::is_nt_handle(const Anvil::ExternalSemaphoreHandleTypeFlagBits& in_type)
    {
        return (in_type == Anvil::ExternalSemaphoreHandleTypeFlagBits::D3D12_FENCE_BIT   ||
                in_type == Anvil::ExternalSemaphoreHandleTypeFlagBits::OPAQUE_WIN32_BIT);
    }
#endif