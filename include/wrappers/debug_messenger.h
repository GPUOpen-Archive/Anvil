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

/* Provides support for debug utils messengers, introduced by VK_EXT_debug_utils extension.
 *
 * Also supports a fall-back path leveraging VK_EXT_debug_report if VK_EXT_debug_utils is unavailable.
 *
 */
#ifndef WRAPPERS_DEBUG_MESSENGER_H
#define WRAPPERS_DEBUG_MESSENGER_H

#include "misc/types.h"
#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "wrappers/device.h"

namespace Anvil
{
    class DebugMessenger : public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        virtual ~DebugMessenger();

        /* Creates a debug messenger instance.
         *
         * Functions exposed by this wrapper require VK_EXT_debug_utils to be available. However, portion of the offered
         * functionality can be handled by implementations supporting VK_EXT_debug_report; in cases only the latter is
         * available, it will be used instead. Unavailable pieces of information will not be included in the callbacks
         * in such case.
         *
         * The function will fail if neither of the two extensions is available.
         **/
        static DebugMessengerUniquePtr create(Anvil::DebugMessengerCreateInfoUniquePtr in_create_info_ptr);

        const Anvil::DebugMessengerCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        void submit_message(const Anvil::DebugMessageSeverityFlagBits&     in_message_severity,
                            const Anvil::DebugMessageTypeFlags&            in_message_type_flags,
                            const char*                                    in_message_id_name_ptr,
                            int32_t                                        in_message_id,
                            const char*                                    in_message_ptr,
                            const std::vector<Anvil::DebugLabel>&          in_queue_labels,
                            const std::vector<Anvil::DebugLabel>&          in_cmd_buffer_labels,
                            const std::vector<Anvil::DebugObjectNameInfo>& in_objects);

    private:
        /* Private type definitions */
        enum class DebugAPI
        {
            EXT_DEBUG_REPORT,
            EXT_DEBUG_UTILS
        };

        /* Private functions */
        DebugMessenger(const DebugAPI&                          in_debug_api,
                       Anvil::DebugMessengerCreateInfoUniquePtr in_create_info_ptr);

        static Anvil::DebugMessageSeverityFlags get_debug_message_severity_flags_for_debug_report_flags(const VkDebugReportFlagsEXT&            in_flags);
        static VkDebugReportFlagsEXT            get_debug_report_flags_for_debug_message_severity_flags(const Anvil::DebugMessageSeverityFlags& in_flags);

        static VkBool32 VKAPI_PTR callback_handler_ext_debug_report(VkDebugReportFlagsEXT                       in_flags,
                                                                    VkDebugReportObjectTypeEXT                  in_object_type,
                                                                    uint64_t                                    in_object,
                                                                    size_t                                      in_location,
                                                                    int32_t                                     in_message_code,
                                                                    const char*                                 in_layer_prefix_ptr,
                                                                    const char*                                 in_message_ptr,
                                                                    void*                                       in_user_data_ptr);
        static VkBool32 VKAPI_PTR callback_handler_ext_debug_utils (VkDebugUtilsMessageSeverityFlagBitsEXT      in_message_severity,
                                                                    VkDebugUtilsMessageTypeFlagsEXT             in_message_types,
                                                                    const VkDebugUtilsMessengerCallbackDataEXT* in_callback_data_ptr,
                                                                    void*                                       in_user_data_ptr);

        /* Private variables */
        Anvil::DebugMessengerCreateInfoUniquePtr m_create_info_ptr;
        const DebugAPI                           m_debug_api;


        VkDebugReportCallbackEXT m_debug_callback; //< only used if m_debug_api == EXT_DEBUG_REPORT
        VkDebugUtilsMessengerEXT m_messenger;      //< only used if m_debug_api == EXT_DEBUG_UTILS
    };
}

#endif /* WRAPPERS_DEBUG_MESSENGER_H */
