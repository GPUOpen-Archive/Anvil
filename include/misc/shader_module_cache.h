//
// Copyright (c) 2017-2018 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef WRAPPERS_SHADER_MODULE_CACHE_H
#define WRAPPERS_SHADER_MODULE_CACHE_H

#include "misc/mt_safety.h"
#include "misc/types.h"
#include "wrappers/shader_module.h"


namespace Anvil
{
    /** TODO
     *
     *  This object should ONLY be instantiated by Anvil::Instance.
     *
     *  Shader module cache is thread-safe.
     */
    class ShaderModuleCache : public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** TODO */
        ~ShaderModuleCache();

        /** TODO */
        static Anvil::ShaderModuleCacheUniquePtr create();

        /** TODO */
        Anvil::ShaderModuleUniquePtr get_cached_shader_module(const Anvil::BaseDevice* in_device_ptr,
                                                              const char*              in_spirv_blob,
                                                              uint32_t                 in_n_spirv_blob_bytes,
                                                              const std::string&       in_cs_entrypoint_name,
                                                              const std::string&       in_fs_entrypoint_name,
                                                              const std::string&       in_gs_entrypoint_name,
                                                              const std::string&       in_tc_entrypoint_name,
                                                              const std::string&       in_te_entrypoint_name,
                                                              const std::string&       in_vs_entrypoint_name);

    private:
        /* Private type definitions */
        typedef struct HashMapItem
        {
            const Anvil::BaseDevice*     device_ptr;
            std::vector<uint32_t>        spirv_blob;
            Anvil::ShaderModuleUniquePtr shader_module_owned_ptr;

            std::string cs_entrypoint_name;
            std::string fs_entrypoint_name;
            std::string gs_entrypoint_name;
            std::string tc_entrypoint_name;
            std::string te_entrypoint_name;
            std::string vs_entrypoint_name;

            explicit HashMapItem(const Anvil::BaseDevice*     in_device_ptr,
                                 const std::vector<uint32_t>& in_spirv_blob,
                                 const std::string&           in_cs_entrypoint_name,
                                 const std::string&           in_fs_entrypoint_name,
                                 const std::string&           in_gs_entrypoint_name,
                                 const std::string&           in_tc_entrypoint_name,
                                 const std::string&           in_te_entrypoint_name,
                                 const std::string&           in_vs_entrypoint_name,
                                 Anvil::ShaderModule*         in_shader_module_ptr)
            {
                device_ptr              = in_device_ptr;
                spirv_blob              = in_spirv_blob;
                shader_module_owned_ptr = Anvil::ShaderModuleUniquePtr(in_shader_module_ptr,
                                                                       std::default_delete<ShaderModule>() );

                cs_entrypoint_name = in_cs_entrypoint_name;
                fs_entrypoint_name = in_fs_entrypoint_name;
                gs_entrypoint_name = in_gs_entrypoint_name;
                tc_entrypoint_name = in_tc_entrypoint_name;
                te_entrypoint_name = in_te_entrypoint_name;
                vs_entrypoint_name = in_vs_entrypoint_name;
            }

            bool matches(const Anvil::BaseDevice* in_device_ptr,
                         const char*              in_spirv_blob,
                         uint32_t                 in_n_spirv_blob_bytes,
                         const std::string&       in_cs_entrypoint_name,
                         const std::string&       in_fs_entrypoint_name,
                         const std::string&       in_gs_entrypoint_name,
                         const std::string&       in_tc_entrypoint_name,
                         const std::string&       in_te_entrypoint_name,
                         const std::string&       in_vs_entrypoint_name) const
            {
                bool result = (spirv_blob.size() * sizeof(spirv_blob.at(0)) == in_n_spirv_blob_bytes);

                if (result)
                {
                    result = (cs_entrypoint_name == in_cs_entrypoint_name &&
                              fs_entrypoint_name == in_fs_entrypoint_name &&
                              gs_entrypoint_name == in_gs_entrypoint_name &&
                              tc_entrypoint_name == in_tc_entrypoint_name &&
                              te_entrypoint_name == in_te_entrypoint_name &&
                              vs_entrypoint_name == in_vs_entrypoint_name);
                }

                if (result)
                {
                    result = (device_ptr == in_device_ptr);
                }

                if (result)
                {
                    result = (memcmp(&spirv_blob.at(0),
                                     in_spirv_blob,
                                     spirv_blob.size() * sizeof(spirv_blob.at(0) )) == 0);
                }

                return result;
            }
        } HashMapItem;

        typedef std::forward_list<std::unique_ptr<HashMapItem> > HashMapItems;

        /* Private functions */

        ShaderModuleCache();

        void cache               (Anvil::ShaderModule* in_shader_module_ptr);
        void update_subscriptions(bool                 in_should_init);

        size_t get_hash(const char*        in_spirv_blob,
                        uint32_t           in_n_spirv_blob_bytes,
                        const std::string& in_cs_entrypoint_name,
                        const std::string& in_fs_entrypoint_name,
                        const std::string& in_gs_entrypoint_name,
                        const std::string& in_tc_entrypoint_name,
                        const std::string& in_te_entrypoint_name,
                        const std::string& in_vs_entrypoint_name) const;

        void on_shader_module_object_about_to_be_released(CallbackArgument* in_callback_arg_ptr);
        void on_shader_module_object_registered          (CallbackArgument* in_callback_arg_ptr);

        /* Private variables */
        std::map<size_t, HashMapItems> m_item_ptrs;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(ShaderModuleCache);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(ShaderModuleCache);
    };
};

#endif