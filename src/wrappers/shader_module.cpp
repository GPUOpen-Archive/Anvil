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

#include "misc/debug.h"
#include "misc/glsl_to_spirv.h"
#include "misc/object_tracker.h"
#include "misc/shader_module_cache.h"
#include "wrappers/device.h"
#include "wrappers/instance.h"
#include "wrappers/shader_module.h"

#ifdef ANVIL_LINK_WITH_GLSLANG
    #include "glslang/SPIRV/disassemble.h"
#endif


/** Please see header for specification */
Anvil::ShaderModule::ShaderModule(std::weak_ptr<Anvil::BaseDevice>            in_device_ptr,
                                  std::shared_ptr<GLSLShaderToSPIRVGenerator> in_spirv_generator_ptr)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT),
     m_device_ptr              (in_device_ptr),
     m_device_raw_ptr          (in_device_ptr.lock().get() )
{
    bool              result                 = false;
    const char*       shader_spirv_blob      = in_spirv_generator_ptr->get_spirv_blob();
    const uint32_t    shader_spirv_blob_size = in_spirv_generator_ptr->get_spirv_blob_size();
    const ShaderStage shader_stage           = in_spirv_generator_ptr->get_shader_stage();

    ANVIL_REDUNDANT_VARIABLE(result);

    anvil_assert(shader_spirv_blob      != nullptr);
    anvil_assert(shader_spirv_blob_size >  0);

    m_cs_entrypoint_name = (shader_stage == SHADER_STAGE_COMPUTE)                 ? "main" : "";
    m_fs_entrypoint_name = (shader_stage == SHADER_STAGE_FRAGMENT)                ? "main" : "";
    m_glsl_source_code   = in_spirv_generator_ptr->get_glsl_source_code();
    m_gs_entrypoint_name = (shader_stage == SHADER_STAGE_GEOMETRY)                ? "main" : "";
    m_tc_entrypoint_name = (shader_stage == SHADER_STAGE_TESSELLATION_CONTROL)    ? "main" : "";
    m_te_entrypoint_name = (shader_stage == SHADER_STAGE_TESSELLATION_EVALUATION) ? "main" : "";
    m_vs_entrypoint_name = (shader_stage == SHADER_STAGE_VERTEX)                  ? "main" : "";

    result = init_from_spirv_blob(shader_spirv_blob,
                                  shader_spirv_blob_size);

    anvil_assert(result);
}

/** Please see header for specification */
Anvil::ShaderModule::ShaderModule(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                  const char*                      in_spirv_blob,
                                  uint32_t                         in_n_spirv_blob_bytes,
                                  const std::string&               in_cs_entrypoint_name,
                                  const std::string&               in_fs_entrypoint_name,
                                  const std::string&               in_gs_entrypoint_name,
                                  const std::string&               in_tc_entrypoint_name,
                                  const std::string&               in_te_entrypoint_name,
                                  const std::string&               in_vs_entrypoint_name)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT),
     m_cs_entrypoint_name      (in_cs_entrypoint_name),
     m_device_ptr              (in_device_ptr),
     m_device_raw_ptr          (in_device_ptr.lock().get() ),
     m_fs_entrypoint_name      (in_fs_entrypoint_name),
     m_gs_entrypoint_name      (in_gs_entrypoint_name),
     m_tc_entrypoint_name      (in_tc_entrypoint_name),
     m_te_entrypoint_name      (in_te_entrypoint_name),
     m_vs_entrypoint_name      (in_vs_entrypoint_name)
{
    bool result = init_from_spirv_blob(in_spirv_blob,
                                       in_n_spirv_blob_bytes);

    ANVIL_REDUNDANT_VARIABLE(result);
    anvil_assert            (result);
}

/** Please see header for specification */
Anvil::ShaderModule::~ShaderModule()
{
    auto object_tracker_ptr = Anvil::ObjectTracker::get();

    object_tracker_ptr->unregister_object(Anvil::OBJECT_TYPE_SHADER_MODULE,
                                          this);

    /* Release the Vulkan handle */
    destroy();

    /* Unregister from any callbacks we have subscribed for */
    object_tracker_ptr->unregister_from_callbacks(Anvil::OBJECT_TRACKER_CALLBACK_ID_ON_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                                                  on_object_about_to_be_released,
                                                  this);
}

/** Please see header for specification */
std::shared_ptr<Anvil::ShaderModule> Anvil::ShaderModule::create_from_spirv_generator(std::weak_ptr<Anvil::BaseDevice>            in_device_ptr,
                                                                                      std::shared_ptr<GLSLShaderToSPIRVGenerator> in_spirv_generator_ptr)
{
    std::shared_ptr<Anvil::ShaderModule> result_ptr;
    auto                                 shader_module_cache_ptr = in_device_ptr.lock()->get_parent_instance().lock()->get_shader_module_cache();
    const auto                           shader_stage            = in_spirv_generator_ptr->get_shader_stage();

    /* First check if a shader module with specified parameters has not already been created. If so,
     * we can safely re-use it. */
    result_ptr = shader_module_cache_ptr->get_cached_shader_module(in_device_ptr,
                                                                   in_spirv_generator_ptr->get_spirv_blob(),
                                                                   in_spirv_generator_ptr->get_spirv_blob_size(),
                                                                   (shader_stage == SHADER_STAGE_COMPUTE)                 ? "main" : "",
                                                                   (shader_stage == SHADER_STAGE_FRAGMENT)                ? "main" : "",
                                                                   (shader_stage == SHADER_STAGE_GEOMETRY)                ? "main" : "",
                                                                   (shader_stage == SHADER_STAGE_TESSELLATION_CONTROL)    ? "main" : "",
                                                                   (shader_stage == SHADER_STAGE_TESSELLATION_EVALUATION) ? "main" : "",
                                                                   (shader_stage == SHADER_STAGE_VERTEX)                  ? "main" : "");

    if (result_ptr == nullptr)
    {
        /* Nope? Need to create a new instance then. */
        result_ptr.reset(
            new Anvil::ShaderModule(in_device_ptr,
                                    in_spirv_generator_ptr)
        );

        Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_SHADER_MODULE,
                                                     result_ptr.get() );
    }

    return result_ptr;
}

/** Please see header for specification */
std::shared_ptr<Anvil::ShaderModule> Anvil::ShaderModule::create_from_spirv_blob(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                                 const char*                      in_spirv_blob,
                                                                                 uint32_t                         in_n_spirv_blob_bytes,
                                                                                 const char*                      in_cs_entrypoint_name,
                                                                                 const char*                      in_fs_entrypoint_name,
                                                                                 const char*                      in_gs_entrypoint_name,
                                                                                 const char*                      in_tc_entrypoint_name,
                                                                                 const char*                      in_te_entrypoint_name,
                                                                                 const char*                      in_vs_entrypoint_name)
{
    std::shared_ptr<Anvil::ShaderModule> result_ptr;
    auto                                 shader_module_cache_ptr = in_device_ptr.lock()->get_parent_instance().lock()->get_shader_module_cache();

    /* First check if a shader module with specified parameters has not already been created. If so,
     * we can safely re-use it. */
    result_ptr = shader_module_cache_ptr->get_cached_shader_module(in_device_ptr,
                                                                   in_spirv_blob,
                                                                   in_n_spirv_blob_bytes,
                                                                   in_cs_entrypoint_name,
                                                                   in_fs_entrypoint_name,
                                                                   in_gs_entrypoint_name,
                                                                   in_tc_entrypoint_name,
                                                                   in_te_entrypoint_name,
                                                                   in_vs_entrypoint_name);

    if (result_ptr == nullptr)
    {
        /* Nope? Need to create a new instance then. */
        result_ptr.reset(
            new Anvil::ShaderModule(in_device_ptr,
                                    in_spirv_blob,
                                    in_n_spirv_blob_bytes,
                                    in_cs_entrypoint_name,
                                    in_fs_entrypoint_name,
                                    in_gs_entrypoint_name,
                                    in_tc_entrypoint_name,
                                    in_te_entrypoint_name,
                                    in_vs_entrypoint_name)
        );

        Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_SHADER_MODULE,
                                                     result_ptr.get() );
    }

    return result_ptr;
}

/** Please see header for specification */
void Anvil::ShaderModule::destroy()
{
    if (m_module != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_device_raw_ptr->get_device_vk(),
                              m_module,
                              nullptr /* pAllocator */);

        m_module = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
const std::string& Anvil::ShaderModule::get_disassembly()
{
    #ifdef ANVIL_LINK_WITH_GLSLANG
    {
        if (m_disassembly.size() == 0)
        {
            /* Cache a disassembly of the SPIR-V blob. */
            std::stringstream disassembly_sstream;

            spv::Disassemble(disassembly_sstream,
                             m_spirv_blob);

            m_disassembly = disassembly_sstream.str();
        }
    }
    #endif

    return m_disassembly;
}

/** Please see header for specification */
bool Anvil::ShaderModule::init_from_spirv_blob(const char* in_spirv_blob,
                                               uint32_t    in_n_spirv_blob_bytes)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);
    VkResult                           result_vk;
    VkShaderModuleCreateInfo           shader_module_create_info;

    /* Set up the "shader module" create info descriptor */
    anvil_assert((in_n_spirv_blob_bytes % sizeof(uint32_t) ) == 0);

    shader_module_create_info.codeSize = in_n_spirv_blob_bytes;
    shader_module_create_info.flags    = 0;
    shader_module_create_info.pCode    = reinterpret_cast<const uint32_t*>(in_spirv_blob);
    shader_module_create_info.pNext    = nullptr;
    shader_module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    result_vk = vkCreateShaderModule(device_locked_ptr->get_device_vk(),
                                    &shader_module_create_info,
                                     nullptr, /* pAllocator */
                                    &m_module);

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk) )
    {
        set_vk_handle(m_module);

        m_spirv_blob.resize(in_n_spirv_blob_bytes / sizeof(uint32_t) );

        memcpy(&m_spirv_blob.at(0),
               in_spirv_blob,
               in_n_spirv_blob_bytes);
    }

    /* Sign for device destruction notification, in which case we need to destroy the shader module. */
    Anvil::ObjectTracker::get()->register_for_callbacks(Anvil::OBJECT_TRACKER_CALLBACK_ID_ON_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                                                        on_object_about_to_be_released,
                                                        this);

    return is_vk_call_successful(result_vk);
}

/** TODO */
void Anvil::ShaderModule::on_object_about_to_be_released(void* in_callback_arg,
                                                         void* in_shader_module_raw_ptr)
{
    const ObjectTrackerOnObjectAboutToBeUnregisteredCallbackArg* callback_arg_ptr  = reinterpret_cast<const ObjectTrackerOnObjectAboutToBeUnregisteredCallbackArg*>(in_callback_arg);
    Anvil::ShaderModule*                                         shader_module_ptr = reinterpret_cast<Anvil::ShaderModule*>                                        (in_shader_module_raw_ptr);

    if (callback_arg_ptr->object_type == OBJECT_TYPE_DEVICE)
    {
        if (shader_module_ptr->m_device_raw_ptr == callback_arg_ptr->object_raw_ptr)
        {
            /* Make sure to release the shader module handle before we let the object actually proceed with destruction! */
            shader_module_ptr->destroy();
        }
    }
}