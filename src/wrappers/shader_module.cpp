//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
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

#include "misc/debug.h"
#include "misc/glsl_to_spirv.h"
#include "misc/object_tracker.h"
#include "wrappers/device.h"
#include "wrappers/shader_module.h"

/** Please see header for specification */
Anvil::ShaderModule::ShaderModule(Anvil::Device*             device_ptr,
                                  GLSLShaderToSPIRVGenerator& spirv_generator)
    :m_cs_entrypoint_name(nullptr),
     m_device_ptr        (device_ptr),
     m_fs_entrypoint_name(nullptr),
     m_gs_entrypoint_name(nullptr),
     m_tc_entrypoint_name(nullptr),
     m_te_entrypoint_name(nullptr),
     m_vs_entrypoint_name(nullptr)
{
    bool              result;
    const char*       shader_spirv_blob      = spirv_generator.get_spirv_blob();
    const uint32_t    shader_spirv_blob_size = spirv_generator.get_spirv_blob_size();
    const ShaderStage shader_stage           = spirv_generator.get_shader_stage();

    anvil_assert(shader_spirv_blob      != nullptr);
    anvil_assert(shader_spirv_blob_size >  0);

    m_cs_entrypoint_name = (shader_stage == SHADER_STAGE_COMPUTE)                 ? "main" : nullptr;
    m_fs_entrypoint_name = (shader_stage == SHADER_STAGE_FRAGMENT)                ? "main" : nullptr;
    m_gs_entrypoint_name = (shader_stage == SHADER_STAGE_GEOMETRY)                ? "main" : nullptr;
    m_tc_entrypoint_name = (shader_stage == SHADER_STAGE_TESSELLATION_CONTROL)    ? "main" : nullptr;
    m_te_entrypoint_name = (shader_stage == SHADER_STAGE_TESSELLATION_EVALUATION) ? "main" : nullptr;
    m_vs_entrypoint_name = (shader_stage == SHADER_STAGE_VERTEX)                  ? "main" : nullptr;

    result = init_from_spirv_blob(shader_spirv_blob,
                                  shader_spirv_blob_size);

    anvil_assert(result);

    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_SHADER_MODULE,
                                                  this);
}

/** Please see header for specification */
Anvil::ShaderModule::ShaderModule(Anvil::Device* device_ptr,
                                  const char*     spirv_blob,
                                  uint32_t        n_spirv_blob_bytes,
                                  const char*     cs_entrypoint_name,
                                  const char*     fs_entrypoint_name,
                                  const char*     gs_entrypoint_name,
                                  const char*     tc_entrypoint_name,
                                  const char*     te_entrypoint_name,
                                  const char*     vs_entrypoint_name)
    :m_cs_entrypoint_name(cs_entrypoint_name),
     m_device_ptr        (device_ptr),
     m_fs_entrypoint_name(fs_entrypoint_name),
     m_gs_entrypoint_name(gs_entrypoint_name),
     m_tc_entrypoint_name(tc_entrypoint_name),
     m_te_entrypoint_name(te_entrypoint_name),
     m_vs_entrypoint_name(vs_entrypoint_name)
{
    bool result = init_from_spirv_blob(spirv_blob,
                                       n_spirv_blob_bytes);

    anvil_assert(result);

    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_SHADER_MODULE,
                                                  this);
}

/** Please see header for specification */
Anvil::ShaderModule::~ShaderModule()
{
    if (m_module != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_device_ptr->get_device_vk(),
                              m_module,
                              nullptr /* pAllocator */);

        m_module = VK_NULL_HANDLE;
    }

    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_SHADER_MODULE,
                                                    this);
}

/** Please see header for specification */
bool Anvil::ShaderModule::init_from_spirv_blob(const char* spirv_blob,
                                               uint32_t    n_spirv_blob_bytes)
{
    VkResult                 result_vk;
    VkShaderModuleCreateInfo shader_module_create_info;

    /* Set up the "shader module" create info descriptor */
    shader_module_create_info.codeSize = n_spirv_blob_bytes;
    shader_module_create_info.flags    = 0;
    shader_module_create_info.pCode    = reinterpret_cast<const uint32_t*>(spirv_blob);
    shader_module_create_info.pNext    = nullptr;
    shader_module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    result_vk = vkCreateShaderModule(m_device_ptr->get_device_vk(),
                                    &shader_module_create_info,
                                     nullptr, /* pAllocator */
                                    &m_module);

    anvil_assert_vk_call_succeeded(result_vk);

    return is_vk_call_successful(result_vk);
}
