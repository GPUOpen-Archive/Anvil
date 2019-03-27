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
#ifndef TYPES_UTILS_H
#define TYPES_UTILS_H

namespace Anvil
{
    namespace Utils
    {
        MTSafety convert_boolean_to_mt_safety_enum(bool in_mt_safe);

        bool convert_mt_safety_enum_to_boolean(MTSafety                 in_mt_safety,
                                               const Anvil::BaseDevice* in_device_ptr);

        /* NOTE: in_api_version must NOT be Anvil::APIVersion::UNKNOWN */
        void get_version_chunks_for_api_version(const Anvil::APIVersion& in_api_version,
                                                uint32_t*                out_major_version_ptr,
                                                uint32_t*                out_minor_version_ptr);

        Anvil::QueueFamilyFlags get_queue_family_flags_from_queue_family_type(Anvil::QueueFamilyType in_queue_family_type);

        /** Converts a queue family bitfield value to an array of queue family indices.
         *
         *  @param in_queue_families                  Input value to convert from.
         *  @param out_opt_queue_family_indices_ptr   If not NULL, deref will be updated with *@param out_opt_n_queue_family_indices_ptr
         *                                            values, corresponding to queue family indices, as specified under @param in_queue_families.
         *  @param out_opt_n_queue_family_indices_ptr If not NULL, deref will be set to the number of items that would be or were written
         *                                            under @param out_opt_queue_family_indices_ptr.
         *
         **/
        void convert_queue_family_bits_to_family_indices(const Anvil::BaseDevice* in_device_ptr,
                                                         Anvil::QueueFamilyFlags  in_queue_families,
                                                         uint32_t*                out_opt_queue_family_indices_ptr,
                                                         uint32_t*                out_opt_n_queue_family_indices_ptr);

        /** Count the number of set bits in an unsigned value (popcount).
         *  Algorithm taken from Software Optimization Guide for AMD64 Processors, p. 179.
         **/
        static inline uint32_t count_set_bits(uint32_t x)
        {
            x = x - ((x >> 1) & 0x55555555);
            x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
            x = (((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101) >> ((sizeof(uint32_t) - 1) << 3);

            return x;
        }

        /** Returns an access mask which has all the access bits, relevant to the user-specified image layout,
         *  enabled.
         *
         *  The access mask can be further restricted to the specified queue family type.
         */
        Anvil::AccessFlags get_access_mask_from_image_layout(Anvil::ImageLayout     in_layout,
                                                             Anvil::QueueFamilyType in_queue_family_type = Anvil::QueueFamilyType::UNDEFINED);


        /** Converts a pair of Anvil::MemoryPropertyFlags and Anvil::MemoryHeapFlags bitfields to a corresponding Anvil::MemoryFeatureFlags
         *  enum.
         *
         *  @param in_opt_mem_type_flags Vulkan memory property flags. May be 0.
         *  @param in_opt_mem_heap_flags Vulkan memory heap flags. May be 0.
         *
         *  @return Result bitfield.
         */
        Anvil::MemoryFeatureFlags get_memory_feature_flags_from_vk_property_flags(Anvil::MemoryPropertyFlags in_opt_mem_type_flags,
                                                                                  Anvil::MemoryHeapFlags     in_opt_mem_heap_flags);

        /** Converts the specified queue family type to a raw string
         *
         *  @param in_queue_family_type Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(Anvil::QueueFamilyType in_queue_family_type);

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

        /** Converts the specified Anvil::ShaderStage value to a raw string
         *
         *  @param in_shader_stage Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(Anvil::ShaderStage in_shader_stage);

        /** Converts the specified VkShaderStageFlagBits value to a raw string
         *
         *  @param in_shader_stage Input value.
         *
         *  @return Non-NULL value if successful, NULL otherwise.
         */
        const char* get_raw_string(VkShaderStageFlagBits in_shader_stage);

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

        /* Returns Vulkan equivalent of @param in_shader_stage */
        Anvil::ShaderStageFlagBits get_shader_stage_flag_bits_from_shader_stage(Anvil::ShaderStage in_shader_stage);

        /* TODO
         *
         * get_vk_object_type_for_*() return VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT if no enum exists for @param in_object_type. Since EXT_debug_utils
         * has been deprecated, this is not an unlikely scenario.
         */
        Anvil::ObjectType          get_object_type_for_vk_object_type                  (const VkObjectType&               in_object_type);
        Anvil::ObjectType          get_object_type_for_vk_debug_report_object_type     (const VkDebugReportObjectTypeEXT& in_object_type);
        VkDebugReportObjectTypeEXT get_vk_debug_report_object_type_ext_from_object_type(const Anvil::ObjectType&          in_object_type);
        VkObjectType               get_vk_object_type_for_object_type                  (const Anvil::ObjectType&          in_object_type);

        /** Converts an Anvil::MemoryFeatureFlags value to a pair of corresponding Vulkan enum values.
         *
         *  @param in_mem_feature_flags   Anvil memory feature flags to use for conversion.
         *  @param out_mem_type_flags_ptr Deref will be set to a corresponding Anvil::MemoryPropertyFlags value. Must not
         *                                be nullptr.
         *  @param out_mem_heap_flags_ptr Deref will be set to a corresponding Anvil::MemoryHeapFlags value. Must not
         *                                be nullptr.
         **/
        void get_vk_property_flags_from_memory_feature_flags(Anvil::MemoryFeatureFlags   in_mem_feature_flags,
                                                             Anvil::MemoryPropertyFlags* out_mem_type_flags_ptr,
                                                             Anvil::MemoryHeapFlags*     out_mem_heap_flags_ptr);

        #ifdef _WIN32
            bool is_nt_handle(const Anvil::ExternalFenceHandleTypeFlagBits&     in_type);
            bool is_nt_handle(const Anvil::ExternalMemoryHandleTypeFlagBits&    in_type);
            bool is_nt_handle(const Anvil::ExternalSemaphoreHandleTypeFlagBits& in_type);
        #endif

        /** Tells whether @param in_value is a power-of-two. */
        template <typename type>
        bool is_pow2(const type in_value)
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
    };
}; /* Anvil namespace */

#endif /* TYPES_UTILS_H */