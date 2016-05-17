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

/** Implements a wrapper for a single Vulkan Shader Module. Implemented in order to:
 *
 *  - encapsulate all state related to a single shader module.
 *  - let ObjectTracker detect leaking shader module wrapper instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_SHADER_MODULE_H
#define WRAPPERS_SHADER_MODULE_H

#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
    /* Forward declarations */
    class GLSLShaderToSPIRVGenerator;

    class ShaderModule : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Creates a new shader module instance from the specified GLSLShader instance.
         *
         *  Since a single GLSL shader can only describe a single compute/rendering stage
         *  via the main() entry-point, the created shader module will only expose one
         *  entry-point for one shader stage.
         *
         *  @param device_ptr      Device to use to instantiate the shader module. Must not be nullptr.
         *  @param spirv_generator SPIR-V generator, initialized with a GLSL shader body.
         **/
        explicit ShaderModule(Anvil::Device*              device_ptr,
                              GLSLShaderToSPIRVGenerator& spirv_generator);

        /** Creates a new shader module instance from a raw SPIR-V blob.
         *
         *  @param device_ptr         Device to use to instantiate the shader module instance. Must
         *                            not be nullptr.
         *  @param spirv_blob         Buffer holding raw SPIR-V blob contents. Must hold at least
         *                            @param n_spirv_blob_bytes bytes. Must not be nullptr.
         *  @param n_spirv_blob_bytes Number of bytes available for reading under @param spirv_blob.
         *  @param cs_entrypoint_name Compute shader stage entry-point, if one is defined in the blob.
         *                            Otherwise, should be set to nullptr.
         *  @param fs_entrypoint_name Fragment shader stage entry-point, if one is defined in the blob.
         *                            Otherwise, should be set to nullptr.
         *  @param gs_entrypoint_name Geometry shader stage entry-point, if one is defined in the blob.
         *                            Otherwise, should be set to nullptr.
         *  @param tc_entrypoint_name Tessellation control shader stage entry-point, if one is defined
         *                            in the blob. Otherwise, should be set to nullptr.
         *  @param te_entrypoint_name Tessellation evaluation shader stage entry-point, if one is defined
         *                            in the blob. Otherwise, should be set to nullptr.
         *                            Otherwise, should be set to nullptr.
         *  @param vs_entrypoint_name Vertex shader stage entry-point, if one is defined in the blob.
         *                            Otherwise, should be set to nullptr.
         **/
        explicit ShaderModule(Anvil::Device* device_ptr,
                              const char*    spirv_blob,
                              uint32_t       n_spirv_blob_bytes,
                              const char*    cs_entrypoint_name,
                              const char*    fs_entrypoint_name,
                              const char*    gs_entrypoint_name,
                              const char*    tc_entrypoint_name,
                              const char*    te_entrypoint_name,
                              const char*    vs_entrypoint_name);

        /** Returns name of the compute shader stage entry-point, as defined at construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const char* get_cs_entrypoint_name() const
        {
            return m_cs_entrypoint_name;
        }

        /** Returns name of the fragment shader stage entry-point, as defined at construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const char* get_fs_entrypoint_name() const
        {
            return m_fs_entrypoint_name;
        }

        /** Returns name of the geometry shader stage entry-point, as defined at construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const char* get_gs_entrypoint_name() const
        {
            return m_gs_entrypoint_name;
        }

        /** Returns raw Vulkan shader module handle. */
        VkShaderModule get_module() const
        {
            return m_module;
        }

        /** Returns name of the tessellation control shader stage entry-point, as defined at
         *  construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const char* get_tc_entrypoint_name() const
        {
            return m_tc_entrypoint_name;
        }

        /** Returns name of the tessellation evaluation shader stage entry-point, as defined at
         *  construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const char* get_te_entrypoint_name() const
        {
            return m_te_entrypoint_name;
        }

        /** Returns name of the vertex shader stage entry-point, as defined at construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const char* get_vs_entrypoint_name() const
        {
            return m_vs_entrypoint_name;
        }

    private:
        /* Private functions */
        ShaderModule           (const ShaderModule&);
        ShaderModule& operator=(const ShaderModule&);

        /** Destructor. Releases internally maintained Vulkan shader module instance. */
        virtual ~ShaderModule();

        /** Creates a Vulkan shader module instance, using the specified buffer holding SPIR-V blob data.
         *
         *  @param spirv_blob         Buffer holding raw SPIR-V blob contents. Must hold at least
         *                            @param n_spirv_blob_bytes bytes. Must not be nullptr.
         *  @param n_spirv_blob_bytes Number of bytes available for reading under @param spirv_blob.
         *
         *  @return true if successful, false otherwise.
         **/
        bool init_from_spirv_blob(const char* spirv_blob,
                                  uint32_t    n_spirv_blob_bytes);

        /* Private variables */
        const char* m_cs_entrypoint_name;
        const char* m_fs_entrypoint_name;
        const char* m_gs_entrypoint_name;
        const char* m_tc_entrypoint_name;
        const char* m_te_entrypoint_name;
        const char* m_vs_entrypoint_name;

        Anvil::Device* m_device_ptr;
        VkShaderModule  m_module;
    };

    /** Delete functor. Useful if you need to wrap the shader module instance in an auto pointer */
    struct ShaderModuleDeleter
    {
        void operator()(ShaderModule* shader_module_ptr) const
        {
            shader_module_ptr->release();
        }
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SHADER_MODULE_H */