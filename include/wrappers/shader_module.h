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

/** Implements a wrapper for a single Vulkan Shader Module. Implemented in order to:
 *
 *  - encapsulate all state related to a single shader module.
 *  - let ObjectTracker detect leaking shader module wrapper instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_SHADER_MODULE_H
#define WRAPPERS_SHADER_MODULE_H

#include "misc/debug_marker.h"
#include "misc/types.h"

namespace Anvil
{
    /* Forward declarations */
    class GLSLShaderToSPIRVGenerator;

    class ShaderModule : public DebugMarkerSupportProvider<ShaderModule>,
                         public std::enable_shared_from_this<ShaderModule>
    {
    public:
        /* Public functions */

        /** Creates a new shader module instance from the specified GLSLShader instance.
         *
         *  Since a single GLSL shader can only describe a single compute/rendering stage
         *  via the main() entry-point, the created shader module will only expose one
         *  entry-point for one shader stage.
         *
         *  @param in_device_ptr          Device to use to instantiate the shader module. Must not be nullptr.
         *  @param in_spirv_generator_ptr SPIR-V generator, initialized with a GLSL shader body.
         **/
        static std::shared_ptr<ShaderModule> create_from_spirv_generator(std::weak_ptr<Anvil::BaseDevice>            in_device_ptr,
                                                                         std::shared_ptr<GLSLShaderToSPIRVGenerator> in_spirv_generator_ptr);

        /** Creates a new shader module instance from a raw SPIR-V blob.
         *
         *  @param in_device_ptr             Device to use to instantiate the shader module instance. Must
         *                                   not be nullptr.
         *  @param in_spirv_blob             Buffer holding raw SPIR-V blob contents. Must hold at least
         *                                   @param n_spirv_blob_bytes bytes. Must not be nullptr.
         *  @param in_n_spirv_blob_bytes     Number of bytes available for reading under @param in_spirv_blob.
         *  @param in_opt_cs_entrypoint_name Compute shader stage entry-point, if one is defined in the blob.
         *                                   Otherwise, should be set to nullptr.
         *  @param in_opt_fs_entrypoint_name Fragment shader stage entry-point, if one is defined in the blob.
         *                                   Otherwise, should be set to nullptr.
         *  @param in_opt_gs_entrypoint_name Geometry shader stage entry-point, if one is defined in the blob.
         *                                   Otherwise, should be set to nullptr.
         *  @param in_opt_tc_entrypoint_name Tessellation control shader stage entry-point, if one is defined
         *                                   in the blob. Otherwise, should be set to nullptr.
         *  @param in_opt_te_entrypoint_name Tessellation evaluation shader stage entry-point, if one is defined
         *                                   in the blob. Otherwise, should be set to nullptr.
         *                                   Otherwise, should be set to nullptr.
         *  @param in_opt_vs_entrypoint_name Vertex shader stage entry-point, if one is defined in the blob.
         *                                   Otherwise, should be set to nullptr.
         **/
        static std::shared_ptr<ShaderModule> create_from_spirv_blob(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                    const char*                      in_spirv_blob,
                                                                    uint32_t                         in_n_spirv_blob_bytes,
                                                                    const char*                      in_opt_cs_entrypoint_name,
                                                                    const char*                      in_opt_fs_entrypoint_name,
                                                                    const char*                      in_opt_gs_entrypoint_name,
                                                                    const char*                      in_opt_tc_entrypoint_name,
                                                                    const char*                      in_opt_te_entrypoint_name,
                                                                    const char*                      in_opt_vs_entrypoint_name);
        static std::shared_ptr<ShaderModule> create_from_spirv_blob(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                    const char*                      in_spirv_blob,
                                                                    uint32_t                         in_n_spirv_blob_bytes,
                                                                    const std::string&               in_opt_cs_entrypoint_name,
                                                                    const std::string&               in_opt_fs_entrypoint_name,
                                                                    const std::string&               in_opt_gs_entrypoint_name,
                                                                    const std::string&               in_opt_tc_entrypoint_name,
                                                                    const std::string&               in_opt_te_entrypoint_name,
                                                                    const std::string&               in_opt_vs_entrypoint_name)
        {
            return create_from_spirv_blob(in_device_ptr,
                                          in_spirv_blob,
                                          in_n_spirv_blob_bytes,
                                          in_opt_cs_entrypoint_name.c_str(),
                                          in_opt_fs_entrypoint_name.c_str(),
                                          in_opt_gs_entrypoint_name.c_str(),
                                          in_opt_tc_entrypoint_name.c_str(),
                                          in_opt_te_entrypoint_name.c_str(),
                                          in_opt_vs_entrypoint_name.c_str() );
        }

        /** Destructor. Releases internally maintained Vulkan shader module instance. */
        virtual ~ShaderModule();

        /** Returns name of the compute shader stage entry-point, as defined at construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const std::string& get_cs_entrypoint_name() const
        {
            return m_cs_entrypoint_name;
        }

        /** Returns a disassembly of the SPIR-V blob.
         *
         *  The actual disassembly is retrieved from glslang and cached for subsequent requests.
         *
         *  This function only returns a non-empty string if ANVIL_LINK_WITH_GLSLANG is enabled.
         */
        const std::string& get_disassembly();

        /** Returns name of the fragment shader stage entry-point, as defined at construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const std::string& get_fs_entrypoint_name() const
        {
            return m_fs_entrypoint_name;
        }

        /** Returns GLSL souirce code used to initialize this shader module instance.
         *
         *  This function will ONLY return non-zero-sized text string if the ShaderModule instance
         *  has been created using create_from_spirv_generator(). Otherwise, an assertion failure
         *  will be triggered.
         *
         * */
        const std::string& get_glsl_source_code() const
        {
            return m_glsl_source_code;
        }

        /** Returns name of the geometry shader stage entry-point, as defined at construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const std::string& get_gs_entrypoint_name() const
        {
            return m_gs_entrypoint_name;
        }

        /** Returns raw Vulkan shader module handle. */
        VkShaderModule get_module() const
        {
            return m_module;
        }

        /** Returns the device, for which this shader module has been created. */
        std::weak_ptr<Anvil::BaseDevice> get_parent_device() const
        {
            return m_device_ptr;
        }

        /** Returns SPIR-V blob which was used to instantiate this shader module */
        const std::vector<uint32_t>& get_spirv_blob() const
        {
            anvil_assert(m_spirv_blob.size() != 0);

            return m_spirv_blob;
        }

        /** Returns name of the tessellation control shader stage entry-point, as defined at
         *  construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const std::string& get_tc_entrypoint_name() const
        {
            return m_tc_entrypoint_name;
        }

        /** Returns name of the tessellation evaluation shader stage entry-point, as defined at
         *  construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const std::string& get_te_entrypoint_name() const
        {
            return m_te_entrypoint_name;
        }

        /** Returns name of the vertex shader stage entry-point, as defined at construction time.
         *
         *  Will return nullptr if no entry-point was defined.
         **/
        const std::string& get_vs_entrypoint_name() const
        {
            return m_vs_entrypoint_name;
        }

    private:
        /* Private functions */

        /* Constructor. Please see create() for specification */
        explicit ShaderModule(std::weak_ptr<Anvil::BaseDevice>            in_device_ptr,
                              std::shared_ptr<GLSLShaderToSPIRVGenerator> in_spirv_generator_ptr);
        explicit ShaderModule(std::weak_ptr<Anvil::BaseDevice>            in_device_ptr,
                              const char*                                 in_spirv_blob,
                              uint32_t                                    in_n_spirv_blob_bytes,
                              const std::string&                          in_opt_cs_entrypoint_name,
                              const std::string&                          in_opt_fs_entrypoint_name,
                              const std::string&                          in_opt_gs_entrypoint_name,
                              const std::string&                          in_opt_tc_entrypoint_name,
                              const std::string&                          in_opt_te_entrypoint_name,
                              const std::string&                          in_opt_vs_entrypoint_name);

        ShaderModule           (const ShaderModule&);
        ShaderModule& operator=(const ShaderModule&);

        /* Destroys the Vulkan Shader module instance */
        void destroy();

        /** Creates a Vulkan shader module instance, using the specified buffer holding SPIR-V blob data.
         *
         *  @param in_spirv_blob         Buffer holding raw SPIR-V blob contents. Must hold at least
         *                               @param n_spirv_blob_bytes bytes. Must not be nullptr.
         *  @param in_n_spirv_blob_bytes Number of bytes available for reading under @param in_spirv_blob.
         *
         *  @return true if successful, false otherwise.
         **/
        bool init_from_spirv_blob(const char* in_spirv_blob,
                                  uint32_t    in_n_spirv_blob_bytes);

        static void on_object_about_to_be_released(void* in_callback_arg,
                                                   void* in_shader_module_raw_ptr);

        /* Private variables */
        std::string m_cs_entrypoint_name;
        std::string m_fs_entrypoint_name;
        std::string m_gs_entrypoint_name;
        std::string m_tc_entrypoint_name;
        std::string m_te_entrypoint_name;
        std::string m_vs_entrypoint_name;

        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        const Anvil::BaseDevice*         m_device_raw_ptr;
        std::string                      m_glsl_source_code;
        VkShaderModule                   m_module;
        std::vector<uint32_t>            m_spirv_blob;

#ifdef ANVIL_LINK_WITH_GLSLANG
        std::string m_disassembly;
#endif
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SHADER_MODULE_H */