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

#ifndef MISC_DEBUG_MESSENGER_CREATE_INFO_H
#define MISC_DEBUG_MESSENGER_CREATE_INFO_H

#include "misc/types.h"

namespace Anvil
{
    class DebugMessengerCreateInfo
    {
    public:
        /* Public functions */

        /** Destructor. */
        ~DebugMessengerCreateInfo()
        {
            /* Stub */
        }

        /* Instantiates a create info structure for debug messengers.
         *
         * @param in_instance_ptr                 Vulkan instance to use when creating the Debug Messenger object. Must not be nullptr.
         * @param in_debug_message_severity_flags Severity of debug messages to use for callbacks.
         * @param in_debug_message_type_flags     Types of debug messages to use for callbacks.
         * @param in_callback_function            Callback function to use. Must not be nullptr.
         *
         * @result Create info structure ijnstance.
         */
        static DebugMessengerCreateInfoUniquePtr create(Anvil::Instance*                        in_instance_ptr,
                                                        const Anvil::DebugMessageSeverityFlags& in_debug_message_severity_flags,
                                                        const Anvil::DebugMessageTypeFlags&     in_debug_message_type_flags,
                                                        Anvil::DebugMessengerCallbackFunction   in_callback_function);

        const Anvil::DebugMessengerCallbackFunction& get_callback_function() const
        {
            return m_callback_function;
        }

        const Anvil::DebugMessageSeverityFlags& get_debug_message_severity() const
        {
            return m_debug_message_severity_flags;
        }

        const Anvil::DebugMessageTypeFlags& get_debug_message_types() const
        {
            return m_debug_message_type_flags;
        }

        Anvil::Instance* get_instance_ptr() const
        {
            return m_instance_ptr;
        }

    private:
        /* Please see create() documentation for more details */

        DebugMessengerCreateInfo(Anvil::Instance*                        in_instance_ptr,
                                 const Anvil::DebugMessageSeverityFlags& in_debug_message_severity_flags,
                                 const Anvil::DebugMessageTypeFlags&     in_debug_message_type_flags,
                                 Anvil::DebugMessengerCallbackFunction   in_callback_function);

        /* Private variables */

        Anvil::DebugMessengerCallbackFunction m_callback_function;
        Anvil::DebugMessageSeverityFlags      m_debug_message_severity_flags;
        Anvil::DebugMessageTypeFlags          m_debug_message_type_flags;
        Anvil::Instance*                      m_instance_ptr;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(DebugMessengerCreateInfo);
    };
}; /* namespace Anvil */

#endif /* MISC_DEBUG_MESSENGER_CREATE_INFO_H */