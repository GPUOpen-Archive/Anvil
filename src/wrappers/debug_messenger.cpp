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

#include "misc/debug_messenger_create_info.h"
#include "wrappers/debug_messenger.h"
#include "wrappers/instance.h"

Anvil::DebugMessenger::DebugMessenger(const DebugAPI&                          in_debug_api,
                                      Anvil::DebugMessengerCreateInfoUniquePtr in_create_info_ptr)
    :MTSafetySupportProvider       (in_create_info_ptr->get_instance_ptr()->is_mt_safe() ),
     m_debug_api                   (in_debug_api),
     m_debug_callback              (VK_NULL_HANDLE),
     m_messenger                   (VK_NULL_HANDLE)
{
    auto instance_ptr = in_create_info_ptr->get_instance_ptr();

    m_create_info_ptr = std::move(in_create_info_ptr);

    switch (m_debug_api)
    {
        case DebugAPI::EXT_DEBUG_REPORT:
        {
            VkDebugReportCallbackCreateInfoEXT create_info;
            const auto&                        entrypoints  = instance_ptr->get_extension_ext_debug_report_entrypoints();
            VkResult                           result_vk;

            create_info.flags       = get_debug_report_flags_for_debug_message_severity_flags(m_create_info_ptr->get_debug_message_severity() );
            create_info.pfnCallback = callback_handler_ext_debug_report;
            create_info.pNext       = nullptr;
            create_info.pUserData   = this;
            create_info.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

            result_vk = entrypoints.vkCreateDebugReportCallbackEXT(instance_ptr->get_instance_vk(),
                                                                  &create_info,
                                                                   nullptr, /* pAllocator */
                                                                  &m_debug_callback);

            anvil_assert_vk_call_succeeded(result_vk);
            ANVIL_REDUNDANT_VARIABLE      (result_vk);

            break;
        }

        case DebugAPI::EXT_DEBUG_UTILS:
        {
            VkDebugUtilsMessengerCreateInfoEXT create_info;
            const auto&                        entrypoints  = m_create_info_ptr->get_instance_ptr()->get_extension_ext_debug_utils_entrypoints();
            VkResult                           result_vk;

            create_info.flags           = 0;
            create_info.messageSeverity = m_create_info_ptr->get_debug_message_severity().get_vk();
            create_info.messageType     = m_create_info_ptr->get_debug_message_types().get_vk   ();
            create_info.pfnUserCallback = callback_handler_ext_debug_utils;
            create_info.pNext           = nullptr;
            create_info.pUserData       = this;
            create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

            result_vk = entrypoints.vkCreateDebugUtilsMessengerEXT(instance_ptr->get_instance_vk(),
                                                                  &create_info,
                                                                   nullptr, /* pAllocator */
                                                                  &m_messenger);

            anvil_assert_vk_call_succeeded(result_vk);
            ANVIL_REDUNDANT_VARIABLE      (result_vk);

            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }
}

Anvil::DebugMessenger::~DebugMessenger()
{
    auto instance_ptr = m_create_info_ptr->get_instance_ptr();

    switch (m_debug_api)
    {
        case DebugAPI::EXT_DEBUG_REPORT:
        {
            const auto& entrypoints = instance_ptr->get_extension_ext_debug_report_entrypoints();

            if (m_debug_callback != VK_NULL_HANDLE)
            {
                entrypoints.vkDestroyDebugReportCallbackEXT(instance_ptr->get_instance_vk(),
                                                            m_debug_callback,
                                                            nullptr); /* pAllocator */
            }

            break;
        }

        case DebugAPI::EXT_DEBUG_UTILS:
        {
            const auto& entrypoints = instance_ptr->get_extension_ext_debug_utils_entrypoints();

            if (m_messenger != VK_NULL_HANDLE)
            {
                entrypoints.vkDestroyDebugUtilsMessengerEXT(instance_ptr->get_instance_vk(),
                                                            m_messenger,
                                                            nullptr); /* pAllocator */
            }

            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }
}

VkBool32 VKAPI_PTR Anvil::DebugMessenger::callback_handler_ext_debug_report(VkDebugReportFlagsEXT      in_flags,
                                                                            VkDebugReportObjectTypeEXT in_object_type,
                                                                            uint64_t                   in_object,
                                                                            size_t                     in_location,
                                                                            int32_t                    in_message_code,
                                                                            const char*                in_layer_prefix_ptr,
                                                                            const char*                in_message_ptr,
                                                                            void*                      in_user_data_ptr)
{
    auto                   objects         = std::vector<Anvil::DebugObjectNameInfo>(1);
    Anvil::DebugMessenger* this_ptr        = reinterpret_cast<Anvil::DebugMessenger*>(in_user_data_ptr);
    auto                   create_info_ptr = this_ptr->get_create_info_ptr();

    ANVIL_REDUNDANT_ARGUMENT_CONST(in_layer_prefix_ptr);
    ANVIL_REDUNDANT_ARGUMENT_CONST(in_location);
    ANVIL_REDUNDANT_ARGUMENT_CONST(in_location);

    objects.at(0) = Anvil::DebugObjectNameInfo(in_object,
                                               "UNKNOWN", /* in_object_name_ptr */
                                               Anvil::Utils::get_object_type_for_vk_debug_report_object_type(in_object_type) );

    create_info_ptr->get_callback_function()(static_cast<Anvil::DebugMessageSeverityFlagBits>(get_debug_message_severity_flags_for_debug_report_flags(in_flags).get_vk() ),
                                             create_info_ptr->get_debug_message_types(),
                                             nullptr, /* in_opt_message_id_name_ptr */
                                             in_message_code,
                                             in_message_ptr,
                                             std::vector<Anvil::DebugLabel>(),
                                             std::vector<Anvil::DebugLabel>(),
                                             objects);

    return VK_FALSE;
}

VkBool32 VKAPI_PTR Anvil::DebugMessenger::callback_handler_ext_debug_utils(VkDebugUtilsMessageSeverityFlagBitsEXT      in_message_severity,
                                                                           VkDebugUtilsMessageTypeFlagsEXT             in_message_types,
                                                                           const VkDebugUtilsMessengerCallbackDataEXT* in_callback_data_ptr,
                                                                           void*                                       in_user_data_ptr)
{
    auto                   cmd_buffer_labels = std::vector<Anvil::DebugLabel>          (in_callback_data_ptr->cmdBufLabelCount);
    auto                   objects           = std::vector<Anvil::DebugObjectNameInfo> (in_callback_data_ptr->objectCount);
    auto                   queue_labels      = std::vector<Anvil::DebugLabel>          (in_callback_data_ptr->queueLabelCount);
    Anvil::DebugMessenger* this_ptr          = reinterpret_cast<Anvil::DebugMessenger*>(in_user_data_ptr);

    for (uint32_t n_cmd_buffer_label = 0;
                  n_cmd_buffer_label < in_callback_data_ptr->cmdBufLabelCount;
                ++n_cmd_buffer_label)
    {
        const auto debug_label = Anvil::DebugLabel(in_callback_data_ptr->pCmdBufLabels[n_cmd_buffer_label]);

        cmd_buffer_labels.at(n_cmd_buffer_label) = debug_label;
    }

    for (uint32_t n_object = 0;
                  n_object < in_callback_data_ptr->objectCount;
                ++n_object)
    {
        const auto object = Anvil::DebugObjectNameInfo(in_callback_data_ptr->pObjects[n_object]);

        objects.at(n_object) = object;
    }

    for (uint32_t n_queue_label = 0;
                  n_queue_label < in_callback_data_ptr->queueLabelCount;
                ++n_queue_label)
    {
        const auto debug_label = Anvil::DebugLabel(in_callback_data_ptr->pQueueLabels[n_queue_label]);

        queue_labels.at(n_queue_label) = debug_label;
    }

    this_ptr->get_create_info_ptr()->get_callback_function()(static_cast<Anvil::DebugMessageSeverityFlagBits>(in_message_severity),
                                                             static_cast<Anvil::DebugMessageTypeFlagBits>    (in_message_types),
                                                             in_callback_data_ptr->pMessageIdName,
                                                             in_callback_data_ptr->messageIdNumber,
                                                             in_callback_data_ptr->pMessage,
                                                             queue_labels,
                                                             cmd_buffer_labels,
                                                             objects);

    return VK_FALSE;
}

Anvil::DebugMessengerUniquePtr Anvil::DebugMessenger::create(Anvil::DebugMessengerCreateInfoUniquePtr in_create_info_ptr)
{
    Anvil::DebugMessengerUniquePtr result_ptr  (nullptr,
                                                std::default_delete<Anvil::DebugMessenger>() );
    auto                           instance_ptr(in_create_info_ptr->get_instance_ptr() );

    if (instance_ptr->get_enabled_extensions_info()->ext_debug_utils() )
    {
        result_ptr.reset(
            new Anvil::DebugMessenger(DebugAPI::EXT_DEBUG_UTILS,
                                      std::move(in_create_info_ptr) )
        );
    }
    else
    if (instance_ptr->get_enabled_extensions_info()->ext_debug_report() )
    {
        result_ptr.reset(
            new Anvil::DebugMessenger(DebugAPI::EXT_DEBUG_REPORT,
                                      std::move(in_create_info_ptr) )
        );
    }
    else
    {
        anvil_assert_fail();
    }

    anvil_assert(result_ptr != nullptr);
    return result_ptr;
}

Anvil::DebugMessageSeverityFlags Anvil::DebugMessenger::get_debug_message_severity_flags_for_debug_report_flags(const VkDebugReportFlagsEXT& in_flags)
{
    Anvil::DebugMessageSeverityFlags result = Anvil::DebugMessageSeverityFlagBits::NONE;

    if ((in_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) != 0)
    {
        result |= Anvil::DebugMessageSeverityFlagBits::INFO_BIT;
    }

    if ((in_flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0)
    {
        result |= Anvil::DebugMessageSeverityFlagBits::WARNING_BIT;
    }

    if ((in_flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) != 0)
    {
        result |= Anvil::DebugMessageSeverityFlagBits::WARNING_BIT;
    }

    if ((in_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0)
    {
        result |= Anvil::DebugMessageSeverityFlagBits::ERROR_BIT;
    }

    if ((in_flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) != 0)
    {
        result |= Anvil::DebugMessageSeverityFlagBits::VERBOSE_BIT;
    }

    return result;
}

VkDebugReportFlagsEXT Anvil::DebugMessenger::get_debug_report_flags_for_debug_message_severity_flags(const Anvil::DebugMessageSeverityFlags& in_flags)
{
    VkDebugReportFlagsEXT result = 0;

    if ((in_flags & Anvil::DebugMessageSeverityFlagBits::ERROR_BIT) != 0)
    {
        result |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
    }

    if ((in_flags & Anvil::DebugMessageSeverityFlagBits::INFO_BIT) != 0)
    {
        result |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    }

    if ((in_flags & Anvil::DebugMessageSeverityFlagBits::VERBOSE_BIT) != 0)
    {
        result |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    }

    if ((in_flags & Anvil::DebugMessageSeverityFlagBits::WARNING_BIT) != 0)
    {
        result |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
    }

    return result;
}


void Anvil::DebugMessenger::submit_message(const Anvil::DebugMessageSeverityFlagBits&     in_message_severity,
                                           const Anvil::DebugMessageTypeFlags&            in_message_type_flags,
                                           const char*                                    in_message_id_name_ptr,
                                           int32_t                                        in_message_id,
                                           const char*                                    in_message_ptr,
                                           const std::vector<Anvil::DebugLabel>&          in_queue_labels,
                                           const std::vector<Anvil::DebugLabel>&          in_cmd_buffer_labels,
                                           const std::vector<Anvil::DebugObjectNameInfo>& in_objects)
{
    auto instance_ptr = m_create_info_ptr->get_instance_ptr();

    switch (m_debug_api)
    {
        case DebugAPI::EXT_DEBUG_REPORT:
        {
            const auto& entrypoints = instance_ptr->get_extension_ext_debug_report_entrypoints();

            entrypoints.vkDebugReportMessageEXT(instance_ptr->get_instance_vk(),
                                                get_debug_report_flags_for_debug_message_severity_flags(in_message_severity),
                                                (in_objects.size() > 0) ? Anvil::Utils::get_vk_debug_report_object_type_ext_from_object_type(in_objects.at(0).object_type)
                                                                        : VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
                                                (in_objects.size() > 0) ? reinterpret_cast<uint64_t>(&in_objects.at(0).object_handle)
                                                                        : VK_NULL_HANDLE,
                                                0, /* location */
                                                in_message_id,
                                                "", /* pLayerPrefix */
                                                in_message_ptr);

            break;
        }

        case DebugAPI::EXT_DEBUG_UTILS:
        {
            VkDebugUtilsMessengerCallbackDataEXT callback_data;
            auto                                 cmd_buffer_labels_vk = std::vector<VkDebugUtilsLabelEXT>                      (in_cmd_buffer_labels.size() );
            const auto&                          entrypoints          = instance_ptr->get_extension_ext_debug_utils_entrypoints();
            auto                                 objects_vk           = std::vector<VkDebugUtilsObjectNameInfoEXT>             (in_objects.size          () );
            auto                                 queue_labels_vk      = std::vector<VkDebugUtilsLabelEXT>                      (in_queue_labels.size     () );

            for (uint32_t n_cmd_buffer_label = 0;
                          n_cmd_buffer_label < static_cast<uint32_t>(in_cmd_buffer_labels.size() );
                        ++n_cmd_buffer_label)
            {
                auto&       cmd_buffer_label_vk  = cmd_buffer_labels_vk.at(n_cmd_buffer_label);
                const auto& src_cmd_buffer_label = in_cmd_buffer_labels.at(n_cmd_buffer_label);

                cmd_buffer_label_vk.color[0]   = src_cmd_buffer_label.color[0];
                cmd_buffer_label_vk.color[1]   = src_cmd_buffer_label.color[1];
                cmd_buffer_label_vk.color[2]   = src_cmd_buffer_label.color[2];
                cmd_buffer_label_vk.color[3]   = src_cmd_buffer_label.color[3];
                cmd_buffer_label_vk.pLabelName = src_cmd_buffer_label.name_ptr;
                cmd_buffer_label_vk.pNext      = nullptr;
                cmd_buffer_label_vk.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            }

            for (uint32_t n_object = 0;
                          n_object < static_cast<uint32_t>(in_objects.size() );
                        ++n_object)
            {
                auto&       object_vk  = objects_vk.at(n_object);
                const auto& src_object = in_objects.at(n_object);

                object_vk.objectHandle = src_object.object_handle;
                object_vk.objectType   = Anvil::Utils::get_vk_object_type_for_object_type(src_object.object_type);
                object_vk.pNext        = nullptr;
                object_vk.pObjectName  = src_object.object_name_ptr;
                object_vk.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            }

            for (uint32_t n_queue_label = 0;
                          n_queue_label < static_cast<uint32_t>(in_queue_labels.size() );
                        ++n_queue_label)
            {
                auto&       queue_label_vk  = queue_labels_vk.at(n_queue_label);
                const auto& src_queue_label = in_queue_labels.at(n_queue_label);

                queue_label_vk.color[0]   = src_queue_label.color[0];
                queue_label_vk.color[1]   = src_queue_label.color[1];
                queue_label_vk.color[2]   = src_queue_label.color[2];
                queue_label_vk.color[3]   = src_queue_label.color[3];
                queue_label_vk.pLabelName = src_queue_label.name_ptr;
                queue_label_vk.pNext      = nullptr;
                queue_label_vk.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            }

            callback_data.cmdBufLabelCount = static_cast<uint32_t>(in_cmd_buffer_labels.size() );
            callback_data.flags            = 0;
            callback_data.messageIdNumber  = in_message_id;
            callback_data.objectCount      = static_cast<uint32_t>(in_objects.size() );
            callback_data.pCmdBufLabels    = (cmd_buffer_labels_vk.size() > 0) ? &cmd_buffer_labels_vk.at(0) : nullptr;
            callback_data.pMessage         = in_message_ptr;
            callback_data.pMessageIdName   = in_message_id_name_ptr;
            callback_data.pNext            = nullptr;
            callback_data.pObjects         = (objects_vk.size     () > 0) ? &objects_vk.at     (0) : nullptr;
            callback_data.pQueueLabels     = (queue_labels_vk.size() > 0) ? &queue_labels_vk.at(0) : nullptr;
            callback_data.queueLabelCount  = static_cast<uint32_t>(queue_labels_vk.size() );
            callback_data.sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT;

            entrypoints.vkSubmitDebugUtilsMessageEXT(instance_ptr->get_instance_vk(),
                                                     static_cast<VkDebugUtilsMessageSeverityFlagBitsEXT>(in_message_severity),
                                                     in_message_type_flags.get_vk(),
                                                    &callback_data);

            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }
}
