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

#include "misc/debug_marker.h"
#include "wrappers/device.h"

/** Please see header for specification */
Anvil::DebugMarkerSupportProviderWorker::DebugMarkerSupportProviderWorker(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                          VkDebugReportObjectTypeEXT       in_vk_object_type)
    :m_device_ptr      (in_device_ptr),
     m_object_tag_name (0),
     m_vk_object_handle(VK_NULL_HANDLE),
     m_vk_object_type  (in_vk_object_type)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

    m_is_ext_debug_marker_available = device_locked_ptr->is_ext_debug_marker_extension_enabled();
    m_object_tag_name               = 0;
}

/** Please see header for specification */
void Anvil::DebugMarkerSupportProviderWorker::set_name_internal(const std::string& in_object_name,
                                                                bool               in_should_force)
{
    /* NOTE: We need to support deferred name assignment here. */
    if (m_object_name != in_object_name ||
        in_should_force)
    {
        m_object_name = in_object_name;

        if (m_is_ext_debug_marker_available        &&
            (m_vk_object_handle != VK_NULL_HANDLE) )
        {
            std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
            const auto&                        entrypoints      (device_locked_ptr->get_extension_ext_debug_marker_entrypoints() );
            VkDebugMarkerObjectNameInfoEXT     name_info;
            VkResult                           result_vk;

            name_info.object      = m_vk_object_handle;
            name_info.objectType  = m_vk_object_type;
            name_info.pNext       = nullptr;
            name_info.pObjectName = in_object_name.c_str();
            name_info.sType       = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;

            result_vk = entrypoints.vkDebugMarkerSetObjectNameEXT(device_locked_ptr->get_device_vk(),
                                                                 &name_info);
            anvil_assert_vk_call_succeeded(result_vk);

            ANVIL_REDUNDANT_VARIABLE(result_vk);
        }
    }
}

/** Please see header for specification */
void Anvil::DebugMarkerSupportProviderWorker::set_tag_internal(const uint64_t in_tag_name,
                                                               size_t         in_tag_size,
                                                               const void*    in_tag_ptr,
                                                               bool           in_should_force)
{
    bool should_update = in_should_force;

    if (!should_update)
    {
        should_update = (m_object_tag_data.size() != in_tag_size);
    }

    if (!should_update)
    {
        should_update = (memcmp(&m_object_tag_data.at(0),
                                in_tag_ptr,
                                in_tag_size) != 0);
    }

    if (!should_update)
    {
        should_update = (m_object_tag_name != in_tag_name);
    }

    if (should_update)
    {
        m_object_tag_name = in_tag_name;
        m_object_tag_data.resize(in_tag_size);

        memcpy(&m_object_tag_data,
               in_tag_ptr,
               in_tag_size);

        if (m_is_ext_debug_marker_available        &&
            (m_vk_object_handle != VK_NULL_HANDLE) )
        {
            std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
            const auto&                        entrypoints      (device_locked_ptr->get_extension_ext_debug_marker_entrypoints() );
            VkResult                           result_vk;
            VkDebugMarkerObjectTagInfoEXT      tag_info;

            tag_info.object     = m_vk_object_handle;
            tag_info.objectType = m_vk_object_type;
            tag_info.pNext      = nullptr;
            tag_info.sType      = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
            tag_info.tagName    = m_object_tag_name;
            tag_info.tagSize    = in_tag_size;

            result_vk = entrypoints.vkDebugMarkerSetObjectTagEXT(device_locked_ptr->get_device_vk(),
                                                                &tag_info);
            anvil_assert_vk_call_succeeded(result_vk);

            ANVIL_REDUNDANT_VARIABLE(result_vk);
        }
    }
}

/** Please see header for specification */
void Anvil::DebugMarkerSupportProviderWorker::set_vk_handle_internal(uint64_t in_vk_object_handle)
{
    if (in_vk_object_handle == VK_NULL_HANDLE)
    {
        anvil_assert(m_vk_object_handle != VK_NULL_HANDLE);

        m_vk_object_handle = VK_NULL_HANDLE;

        m_object_name.clear    ();
        m_object_tag_data.clear();

        m_object_tag_name = 0;
    }
    else
    if (m_vk_object_handle != in_vk_object_handle)
    {
        anvil_assert(m_vk_object_handle == VK_NULL_HANDLE);

        m_vk_object_handle = in_vk_object_handle;

        if (m_object_name.size() != 0)
        {
            set_name_internal(m_object_name.c_str(),
                              true); /* in_should_force */
        }

        if (m_object_tag_data.size() != 0)
        {
            set_tag_internal(m_object_tag_name,
                             m_object_tag_data.size(),
                            &m_object_tag_data.at(0),
                             true); /* should_force */
        }
    }
}
