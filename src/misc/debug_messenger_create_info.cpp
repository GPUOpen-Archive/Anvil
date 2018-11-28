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

Anvil::DebugMessengerCreateInfo::DebugMessengerCreateInfo(Anvil::Instance*                        in_instance_ptr,
                                                          const Anvil::DebugMessageSeverityFlags& in_debug_message_severity_flags,
                                                          const Anvil::DebugMessageTypeFlags&     in_debug_message_type_flags,
                                                          Anvil::DebugMessengerCallbackFunction   in_callback_function)
    :m_callback_function           (in_callback_function),
     m_debug_message_severity_flags(in_debug_message_severity_flags),
     m_debug_message_type_flags    (in_debug_message_type_flags),
     m_instance_ptr                (in_instance_ptr)
{
    /* Stub */
}

Anvil::DebugMessengerCreateInfoUniquePtr Anvil::DebugMessengerCreateInfo::create(Anvil::Instance*                        in_instance_ptr,
                                                                                 const Anvil::DebugMessageSeverityFlags& in_debug_message_severity_flags,
                                                                                 const Anvil::DebugMessageTypeFlags&     in_debug_message_type_flags,
                                                                                 Anvil::DebugMessengerCallbackFunction   in_callback_function)
{
    Anvil::DebugMessengerCreateInfoUniquePtr result_ptr;

    result_ptr.reset(
        new DebugMessengerCreateInfo(in_instance_ptr,
                                     in_debug_message_severity_flags,
                                     in_debug_message_type_flags,
                                     in_callback_function)
    );
    anvil_assert(result_ptr != nullptr);

    return result_ptr;
}