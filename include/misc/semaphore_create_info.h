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
#ifndef MISC_SEMAPHORE_CREATE_INFO_H
#define MISC_SEMAPHORE_CREATE_INFO_H

#include "misc/types.h"

namespace Anvil
{
    class SemaphoreCreateInfo
    {
    public:
        /* Public functions */

        /* TODO.
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - Exportable external semaphore handle type: none
         * - MT safety:                                 Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         */
        static Anvil::SemaphoreCreateInfoUniquePtr create(const Anvil::BaseDevice* in_device_ptr);

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        const Anvil::ExternalSemaphoreHandleTypeFlags& get_exportable_external_semaphore_handle_types() const
        {
            return m_exportable_external_semaphore_handle_types;
        }

        #if defined(_WIN32)
            /* Returns true if set_exportable_nt_handle_info() has been called prior to this call.
             * Otherwise returns false.
             *
             * If the func returns true, *out_result_ptr is set to the queried data.
             */
            bool get_exportable_nt_handle_info(const ExternalNTHandleInfo** out_result_ptr_ptr) const
            {
                bool result = false;

                if (m_exportable_nt_handle_info_specified)
                {
                    *out_result_ptr_ptr = &m_exportable_nt_handle_info;
                    result              = true;
                }

                return result;
            }
        #endif

        const MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }

        void set_device(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        /* TODO
         *
         * Requires VK_KHR_external_semaphore.
         */
        void set_exportable_external_semaphore_handle_types(const Anvil::ExternalSemaphoreHandleTypeFlags& in_external_handle_types)
        {
            m_exportable_external_semaphore_handle_types = in_external_handle_types;
        }

        #if defined(_WIN32)
            /* Lets the app specify additional details for exportable NT handles.
             *
             * If @param in_name is zero-sized, <name> member of the VkExportSemaphoreWin32HandleInfoKHR struct, as chained to the VkSemaphoreCreateInfo struct chain,
             * will be set to nullptr.
             *
             * Requires VK_KHR_external_semaphore_win32
             */
            void set_exportable_nt_handle_info(const SECURITY_ATTRIBUTES* in_opt_attributes_ptr,
                                               const DWORD&               in_access,
                                               const std::wstring&        in_name)
            {
                anvil_assert(!m_exportable_nt_handle_info_specified);

                m_exportable_nt_handle_info.access    = in_access;
                m_exportable_nt_handle_info.name      = in_name;
                m_exportable_nt_handle_info_specified = true;

                if (in_opt_attributes_ptr != nullptr)
                {
                    m_exportable_nt_handle_info_security_attributes           = *in_opt_attributes_ptr;
                    m_exportable_nt_handle_info_security_attributes_specified = true;

                    m_exportable_nt_handle_info.attributes_ptr = &m_exportable_nt_handle_info_security_attributes;
                }
                else
                {
                    m_exportable_nt_handle_info.attributes_ptr                = nullptr;
                    m_exportable_nt_handle_info_security_attributes_specified = false;
                }

            }
        #endif

        void set_mt_safety(const MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

    private:
        /* Private functions */
        SemaphoreCreateInfo(const Anvil::BaseDevice* in_device_ptr,
                            MTSafety                 in_mt_safety);

        /* Private variables */
        const Anvil::BaseDevice*                m_device_ptr;
        Anvil::ExternalSemaphoreHandleTypeFlags m_exportable_external_semaphore_handle_types;
        Anvil::MTSafety                         m_mt_safety;

        #ifdef _WIN32
            ExternalNTHandleInfo m_exportable_nt_handle_info;
            SECURITY_ATTRIBUTES  m_exportable_nt_handle_info_security_attributes;
            bool                 m_exportable_nt_handle_info_security_attributes_specified;
            bool                 m_exportable_nt_handle_info_specified;
        #endif

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(SemaphoreCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(SemaphoreCreateInfo);
    };

}; /* namespace Anvil */

#endif /* MISC_SEMAPHORE_CREATE_INFO_H */