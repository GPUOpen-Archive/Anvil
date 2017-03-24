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

/*  Implements an utility which takes GLSL source code (either defined in a file, or explicitly by the user) and converts it
 *  to a SPIR-V blob. The blob can then be used to initialize a Anvil::ShaderModule instance.
 *
 *  Optionally, users can inject arbitrary number of #defines (with or without the accompanying value).
 **/
#ifndef MISC_GLSL_TO_SPIRV_H
#define MISC_GLSL_TO_SPIRV_H

#include "config.h"
#include "misc/debug.h"
#include "misc/types.h"
#include <map>
#include <memory>
#include <sstream>
#include <vector>

#ifdef ANVIL_LINK_WITH_GLSLANG
    #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4619)
        #pragma warning(disable: 4464)
    #endif

    #include "../../deps/glslang/glslang/Public/ShaderLang.h"

    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif
#endif

namespace Anvil
{
    #ifdef ANVIL_LINK_WITH_GLSLANG
        /** Holds glslang limit values, extracted from a physical device instance. */
        class GLSLangLimits
        {
        public:
            /* Constructor. */
            explicit GLSLangLimits(std::weak_ptr<Anvil::BaseDevice> device_ptr);

            /* Destructor */
            ~GLSLangLimits()
            {
                /* Stub */
            }

            /** Retrieves a pointer to an initialized TBuiltInResource instance */
            const struct TBuiltInResource* get_resource_ptr() const
            {
                return m_resources_ptr.get();
            }

        private:
            /* Private functions */
            ANVIL_DISABLE_ASSIGNMENT_OPERATOR(GLSLangLimits);
            ANVIL_DISABLE_COPY_CONSTRUCTOR(GLSLangLimits);

            /* Private fields */
            std::unique_ptr<struct TBuiltInResource> m_resources_ptr;
        };
    #endif

    /** Loads a GLSL shader from the file specified at creation time, customizes it with a user-specified set of #defines,
     *  and then converts the source code to a SPIR-V blob.
     **/
    class GLSLShaderToSPIRVGenerator
    {
    public:
        /* Public type definitions */
        typedef enum
        {
            EXTENSION_BEHAVIOR_DISABLE,
            EXTENSION_BEHAVIOR_ENABLE,
            EXTENSION_BEHAVIOR_REQUIRE,
            EXTENSION_BEHAVIOR_WARN,
        } ExtensionBehavior;

        typedef enum
        {
            MODE_LOAD_SOURCE_FROM_FILE,
            MODE_USE_SPECIFIED_SOURCE
        } Mode;

        /* Public functions */

        /** Creates a new GLSLShaderToSPIRVGenerator instance.
         *
         *  @param evice_ptr    Logical device, whose limit values should be passed to glslang. Must not
         *                      be NULL.
         *  @param mode         Defines type of contents specified under @param data.
         *  @param data         If @param mode is MODE_LOAD_SOURCE_FROM_FILE, @param data holds the name
         *                      of the file (possibly including path) where the GLSL source code is stored.
         *                      If @param mode is MODE_USE_SPECIFIED_SOURCE, @param data holds GLSL source code
         *                      which should be used. This mode is NOT supported if ANVIL_LINK_WITH_GLSLANG
         *                      macro is undefined.
         *  @param shader_stage Shader stage described by the file.
         **/
         static std::shared_ptr<GLSLShaderToSPIRVGenerator> create(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                                   const Mode&                      mode,
                                                                   std::string                      data,
                                                                   ShaderStage                      shader_stage);

         /** Destructor. Releases all created Vulkan objects, as well as the SPIR-V blob data. */
         ~GLSLShaderToSPIRVGenerator();

         /** Adds a "#define [definition_name] [value]" line after the first newline found in the
          *  source code.
          *
          *  The definition will be inserted AFTER extensions, if any have been requested by using
          *  the add_extension_behavior() mechanism.
          *
          *  @param definition_name As specified above.
          *  @param value           As specified above.
          *
          *  @return true if the function succeeded, false otherwise.
          **/
         bool add_definition_value_pair(std::string definition_name,
                                        std::string value);

         /** Adds a "#define [definition_name] [value]" line after the first newline found in the
          *  source code.
          *
          *  The definitions will be inserted AFTER extensions, if any have been requested by using
          *  the add_extension_behavior() mechanism.
          *
          *  @param definition_name As specified above.
          *  @param value           As specified above.
          *
          *  @return true if the function succeeded, false otherwise.
          **/
         template <typename T>
         bool add_definition_value_pair(std::string definition_name,
                                        T           value)
         {
            std::stringstream value_sstream;

            value_sstream << value;

            return add_definition_value_pair(definition_name,
                                             value_sstream.str() );
         }

         /** Adds a "#define [definition_name]" line after the first newline found in the source code.
          *
          *  The definition will be inserted AFTER extensions, if any have been requested by using
          *  the add_extension_behavior() mechanism.
          *
          *  @param definition_name As specified above.
          *
          *  @return true if the function succeeded, false otherwise.
          **/
         bool add_empty_definition(std::string definition_name);

         /* Registers a new extension behavior specification.
          *
          * At baking time, a new line, specifying the extension behavior, will be added
          * at the beginning of the shader.
          *
          * @param extension_name Name of the GLSL extension
          * @param behavior       Behavior to use for the extension. See documentation of
          *                       the enum for more details.
          *
          * @return true if successful, false otherwise.
          **/
         bool add_extension_behavior(std::string       extension_name,
                                     ExtensionBehavior behavior);

         /* Loads the GLSL source code, injects the requested #defines and writes the result code
          * to a temporary file. Then, the func invokes glslangvalidator to build a SPIR-V blob
          * of the updated GLSL shader, deletes the temp file, loads up the blob and purges it.
          *
          * @return true if successful, false otherwise.
          **/
         bool bake_spirv_blob();

         /* Converts a ExtensionBehavior enum value to a corresponding GLSL definition */
         std::string get_extension_behavior_glsl_code(const ExtensionBehavior& value) const;

         /** Tells what shader stage the encapsulated GLSL shader descirbes. */
         ShaderStage get_shader_stage() const
         {
             return m_shader_stage;
         }

         /** Bakes a SPIR-V blob by injecting earlier specified #define name+value pairs into the
          *  GLSL source code and passing such shader code to glslangvalidator.
          *
          *  The baking is only performed once. If the shader has already been baked, the blob
          *  will not be re-created
          *
          *  @return SPIR-V blob or nullptr if the function failed.
          **/
         const char* get_spirv_blob()
         {
             if (m_spirv_blob == nullptr)
             {
                 bool result = bake_spirv_blob();

                 ANVIL_REDUNDANT_VARIABLE(result);

                 anvil_assert(result);
                 anvil_assert(m_spirv_blob != nullptr);
             }

             return m_spirv_blob;
         }

         /** Returns the number of bytes the SPIR-V blob, accessible via get_spirv_blob(), takes. */
         const uint32_t get_spirv_blob_size()
         {
             if (m_spirv_blob == nullptr)
             {
                 bool result = bake_spirv_blob();

                 ANVIL_REDUNDANT_VARIABLE(result);

                 anvil_assert(result);
             }

             anvil_assert(m_spirv_blob_size > 0);

             return m_spirv_blob_size;
         }

    private:
        /* Private type declarations */
        typedef std::map<std::string, ExtensionBehavior> ExtensionNameToExtensionBehaviorMap;
        typedef std::map<std::string, std::string>       DefinitionNameToValueMap;

        /* Private functions */
        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(GLSLShaderToSPIRVGenerator);
        ANVIL_DISABLE_COPY_CONSTRUCTOR   (GLSLShaderToSPIRVGenerator);

        /* Constructor. Please see create() documentation for specification */
        explicit GLSLShaderToSPIRVGenerator(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                            const Mode&                      mode,
                                            std::string                      data,
                                            ShaderStage                      shader_stage);

        #ifdef ANVIL_LINK_WITH_GLSLANG
            bool        bake_spirv_blob_by_calling_glslang(const char* body);
            EShLanguage get_glslang_shader_stage          () const;
        #else
            bool bake_spirv_blob_by_spawning_glslang_process(const std::string& glsl_filename_with_path,
                                                             const std::string& spirv_filename_with_path);
        #endif

        /* Private members */
        #ifdef ANVIL_LINK_WITH_GLSLANG
            GLSLangLimits m_limits;
        #endif

        std::string m_data;
        Mode        m_mode;

        ShaderStage m_shader_stage;
        char*       m_spirv_blob;
        uint32_t    m_spirv_blob_size;

        DefinitionNameToValueMap            m_definition_values;
        ExtensionNameToExtensionBehaviorMap m_extension_behaviors;
    };
}; /* namespace Anvil */

#endif /* MISC_GLSL_TO_SPIRV_H */