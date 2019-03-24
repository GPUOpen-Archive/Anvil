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
#ifndef MISC_INSTANCE_CREATE_INFO_H
#define MISC_INSTANCE_CREATE_INFO_H

#include "misc/types.h"


namespace Anvil
{
    typedef std::function<void (Anvil::DebugMessageSeverityFlags in_severity,
                                const char*                      in_message)> DebugCallbackFunction;

    class InstanceCreateInfo
    {
    public:
        /* Public functions */

        /** Instantiates a new create info which must be specified when creating Anvil Instance object.
         *
         *  By default, Anvil will query the highest version of Vulkan supported by the implementation and use it when creating the Instance.
         *  You can override this behavior by calling set_api_version().
         *
         *  The following are also assumed by default:
         *
         *  + App version:    0
         *  + Engine version: 0

         *  @param in_app_name                                 Name of the application, to be passed in VkCreateInstanceInfo
         *                                                     structure.
         *  @param in_engine_name                              Name of the engine, to be passed in VkCreateInstanceInfo
         *                                                     structure.
         *  @param in_opt_validation_callback_function         If not nullptr, the specified function will be called whenever
         *                                                     a call-back from any of the validation layers is received.
         *                                                     Ignored otherwise.
         *  @param in_mt_safe                                  True if all instance-based operations where external host synchronization
         *                                                     is required should be automatically synchronized by Anvil.
         *  @param in_opt_disallowed_instance_level_extensions Optional vector holding instance-level extension names that must NOT be
         *                                                     requested at creation time. 
         **/
        static Anvil::InstanceCreateInfoUniquePtr create(const std::string&              in_app_name,
                                                         const std::string&              in_engine_name,
                                                         Anvil::DebugCallbackFunction    in_opt_validation_callback_proc,
                                                         bool                            in_mt_safe,
                                                         const std::vector<std::string>& in_opt_disallowed_instance_level_extensions = std::vector<std::string>() );

        const Anvil::APIVersion& get_api_version() const
        {
            return m_api_version;
        }

        const std::string& get_app_name() const
        {
            return m_app_name;
        }

        const uint32_t& get_app_version() const
        {
            return m_app_version;
        }

        const std::vector<std::string>& get_disallowed_instance_level_extensions() const
        {
            return m_disallowed_instance_level_extensions;
        }

        const std::string& get_engine_name() const
        {
            return m_engine_name;
        }

        const uint32_t& get_engine_version() const
        {
            return m_engine_version;
        }

        /* Returns memory type index which should be used by Anvil when allocating memory. This value can be specified with set_n_memory_type_to_use_for_all_allocs().
         *
         * If UINT32_MAX is returned, Anvil will be free to use any memory type determined to be valid for particular memory allocations. This is the default behavior.
         **/
        uint32_t get_n_memory_type_to_use_for_all_allocs() const
        {
            return m_n_memory_type_to_use_for_all_alocs;
        }

        const Anvil::DebugCallbackFunction& get_validation_callback() const
        {
            return m_validation_callback;
        }

        const bool& is_mt_safe() const
        {
            return m_is_mt_safe;
        }

        void set_api_version(const Anvil::APIVersion& in_api_version)
        {
            m_api_version = in_api_version;
        }

        void set_app_name(const std::string& in_app_name)
        {
            m_app_name = in_app_name;
        }

        void set_app_version(const uint32_t& in_version)
        {
            m_app_version = in_version;
        }

        void set_engine_version(const uint32_t& in_version)
        {
            m_engine_version = in_version;
        }

        void set_disallowed_instance_level_extensions(const std::vector<std::string>& in_extensions)
        {
            m_disallowed_instance_level_extensions = in_extensions;
        }

        void set_engine_name(const std::string& in_engine_name)
        {
            m_engine_name = in_engine_name;
        }

        /* When called, any memory allocations performed by objects owned by Anvil Instance object created using this create info structure
         * will always use the specified memory type index.
         *
         * This function should not be called unless you have a good understanding of the implications.
         */
        void set_n_memory_type_to_use_for_all_allocs(const uint32_t& in_n_memory_type_to_use_for_all_allocs)
        {
            m_n_memory_type_to_use_for_all_alocs = in_n_memory_type_to_use_for_all_allocs;
        }

        void set_validation_callback(const Anvil::DebugCallbackFunction& in_validation_callback)
        {
            m_validation_callback = in_validation_callback;
        }

        void set_is_mt_safe(const bool& in_is_mt_safe)
        {
            m_is_mt_safe = in_is_mt_safe;
        }

    private:

        /* Private functions */
        InstanceCreateInfo(const std::string&              in_app_name,
                           const std::string&              in_engine_name,
                           Anvil::DebugCallbackFunction    in_opt_validation_callback_proc,
                           bool                            in_mt_safe,
                           const std::vector<std::string>& in_opt_disallowed_instance_level_extensions);

        /* Private variables */

        Anvil::APIVersion            m_api_version;
        std::string                  m_app_name;
        uint32_t                     m_app_version;
        std::vector<std::string>     m_disallowed_instance_level_extensions;
        std::string                  m_engine_name;
        uint32_t                     m_engine_version;
        bool                         m_is_mt_safe;
        uint32_t                     m_n_memory_type_to_use_for_all_alocs;
        Anvil::DebugCallbackFunction m_validation_callback;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(InstanceCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(InstanceCreateInfo);
    };

}; /* namespace Anvil */

#endif /* MISC_INSTANCE_CREATE_INFO_H */
