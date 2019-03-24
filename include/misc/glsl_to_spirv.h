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

/*  Implements an utility which takes GLSL source code (either defined in a file, or explicitly by the user) and converts it
 *  to a SPIR-V blob. The blob can then be used to initialize a Anvil::ShaderModule instance.
 *
 *  Optionally, users can inject arbitrary number of #defines (with or without the accompanying value).
 **/
#ifndef MISC_GLSL_TO_SPIRV_H
#define MISC_GLSL_TO_SPIRV_H

#include "config.h"
#include "misc/callbacks.h"
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
            explicit GLSLangLimits(const Anvil::BaseDevice* in_device_ptr);

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

    typedef enum
    {
        /* Call-back issued right before the conversion starts.
         *
         * @param callback_arg OnGLSLToSPIRVConversionAboutToBeStartedCallbackArgument instance.
         */
        GLSL_SHADER_TO_SPIRV_GENERATOR_CALLBACK_ID_CONVERSION_ABOUT_TO_START,

        /* Call-back issued right after the conversion ends.
         *
         * @param callback_arg OnGLSLToSPIRVConversionFinishedCallbackArgument instance.
         */
        GLSL_SHADER_TO_SPIRV_GENERATOR_CALLBACK_ID_CONVERSION_FINISHED,

        GLSL_SHADER_TO_SPIRV_GENERATOR_CALLBACK_ID_COUNT
    } GLSLShaderToSPIRVGeneratorCallbackID;

    /** Loads a GLSL shader from the file specified at creation time, customizes it with a user-specified set of #defines,
     *  and then converts the source code to a SPIR-V blob.
     **/
    class GLSLShaderToSPIRVGenerator : public CallbacksSupportProvider
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
         *  @param in_opt_device_ptr Logical device, whose limit values should be passed to glslang. May be null
         *                           if the object is only intended to be used for forming GLSL source code.
         *  @param in_mode           Defines type of contents specified under @param in_data.
         *  @param in_data           If @param in_mode is MODE_LOAD_SOURCE_FROM_FILE, @param in_data holds the name
         *                           of the file (possibly including path) where the GLSL source code is stored.
         *                           If @param in_mode is MODE_USE_SPECIFIED_SOURCE, @param in_data holds GLSL source code
         *                           which should be used. This mode is NOT supported if ANVIL_LINK_WITH_GLSLANG
         *                           macro is undefined.
         *  @param in_shader_stage   Shader stage described by the file.
         *  @param in_spirv_version  Target SPIR-V version.
         **/
         static Anvil::GLSLShaderToSPIRVGeneratorUniquePtr create(const Anvil::BaseDevice* in_opt_device_ptr,
                                                                  const Mode&              in_mode,
                                                                  std::string              in_data,
                                                                  ShaderStage              in_shader_stage,
                                                                  SpvVersion               in_spirv_version = SpvVersion::_1_0);

         /** Destructor. Releases all created Vulkan objects, as well as the SPIR-V blob data. */
         ~GLSLShaderToSPIRVGenerator();

         /** Adds a "#define [definition_name] [value]" line after the first newline found in the
          *  source code.
          *
          *  The definition will be inserted AFTER extensions, if any have been requested by using
          *  the add_extension_behavior() mechanism.
          *
          *  @param in_definition_name As specified above.
          *  @param in_value           As specified above.
          *
          *  @return true if the function succeeded, false otherwise.
          **/
         bool add_definition_value_pair(std::string in_definition_name,
                                        std::string in_value);

         /** Adds a "#define [definition_name] [value]" line after the first newline found in the
          *  source code.
          *
          *  The definitions will be inserted AFTER extensions, if any have been requested by using
          *  the add_extension_behavior() mechanism.
          *
          *  @param in_definition_name As specified above.
          *  @param in_value           As specified above.
          *
          *  @return true if the function succeeded, false otherwise.
          **/
         template <typename T>
         bool add_definition_value_pair(std::string in_definition_name,
                                        T           in_value)
         {
            std::stringstream value_sstream;

            value_sstream << in_value;

            return add_definition_value_pair(in_definition_name,
                                             value_sstream.str() );
         }

         /** Adds a "#define [definition_name]" line after the first newline found in the source code.
          *
          *  The definition will be inserted AFTER extensions, if any have been requested by using
          *  the add_extension_behavior() mechanism.
          *
          *  @param in_definition_name As specified above.
          *
          *  @return true if the function succeeded, false otherwise.
          **/
         bool add_empty_definition(std::string in_definition_name);

         /* Registers a new extension behavior specification.
          *
          * At baking time, a new line, specifying the extension behavior, will be added
          * at the beginning of the shader.
          *
          * @param in_extension_name Name of the GLSL extension
          * @param in_behavior       Behavior to use for the extension. See documentation of
          *                          the enum for more details.
          *
          * @return true if successful, false otherwise.
          **/
         bool add_extension_behavior(std::string       in_extension_name,
                                     ExtensionBehavior in_behavior);

         /** Replaces all instances of [placeholder_name] with [value] in the shader source.
          *
          *  @param in_placeholder_name As specified above.
          *  @param in_value            As specified above.
          *
          *  @return true if the function succeeded, false otherwise.
          **/
         bool add_placeholder_value_pair(const std::string& in_placeholder_name,
                                         const std::string& in_value);

         /** Replaces all instances of [placeholder_name] with [value] in the shader source.
          *
          *  @param in_placeholder_name As specified above.
          *  @param in_value            As specified above.
          *
          *  @return true if the function succeeded, false otherwise.
          **/
         template <typename T>
         bool add_placeholder_value_pair(const std::string& in_placeholder_name,
                                         const T&           in_value)
         {
            std::stringstream value_sstream;

            value_sstream << in_value;

            return add_placeholder_value_pair(in_placeholder_name,
                                              value_sstream.str() );
         }

         /** Adds a new pragma which is going to be injected into the GLSL code.
          *
          *  @param in_pragma_name Value to follow the #pragma keyword.
          *  @param in_opt_value   Value to be assigned to the pragma. May be zero-sized.
          *
          *  @return true if successful, false otherwise.
          **/
         bool add_pragma(std::string in_pragma_name,
                         std::string in_opt_value = "");

         /* Loads the GLSL source code, injects the requested #defines and writes the result code
          * to a temporary file. Then, the func invokes glslangvalidator to build a SPIR-V blob
          * of the updated GLSL shader, deletes the temp file, loads up the blob and purges it.
          *
          * @return true if successful, false otherwise.
          **/
         bool bake_spirv_blob() const;

         /* Converts a ExtensionBehavior enum value to a corresponding GLSL definition */
         std::string get_extension_behavior_glsl_code(const ExtensionBehavior& in_value) const;

         #ifdef ANVIL_LINK_WITH_GLSLANG
            const std::string& get_debug_info_log() const
            {
                return m_debug_info_log;
            }

            const std::string& get_program_debug_info_log() const
            {
                return m_program_debug_info_log;
            }

            /** Returns info log which contains detailed information regarding the program linking process.
              *
              *  Call if get_spirv_blob() returns nullptr to find out more about shader issues which
              *  prevented the process from finishing successfully.
              *
              *  @return See description above.
              */
             const std::string& get_program_info_log() const
             {
                 return m_program_info_log;
             }

             /** Returns info log which contains detailed information regarding the shader compilation process.
              *
              *  Call if get_spirv_blob() returns nullptr to find out more about shader issues which
              *  prevented the process from finishing successfully.
              *
              *  @return See description above.
              */
             const std::string& get_shader_info_log() const
             {
                 return m_shader_info_log;
             }
         #endif

         /* Retrieves GLSL source code that has been used for GLSL->SPIR-V conversion.
          *
          * This function should only be called after bake_spirv_blob() has been invoked.
          * Otherwise, an assertion failure will occur, as the returned string will have a size of 0.
          *
          */
         const std::string& get_glsl_source_code() const
         {
             if (m_glsl_source_code_dirty)
             {
                 bake_glsl_source_code();

                 anvil_assert(!m_glsl_source_code_dirty);
             }

             return m_glsl_source_code;
         }

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
         const char* get_spirv_blob() const
         {
             if (m_spirv_blob.size() == 0)
             {
                 bake_spirv_blob();
             }

             return (m_spirv_blob.size() != 0) ? &m_spirv_blob.at(0) : nullptr;
         }

         /** Returns the number of bytes the SPIR-V blob, accessible via get_spirv_blob(), takes. */
         uint32_t get_spirv_blob_size() const
         {
             if (m_spirv_blob.size() == 0)
             {
                 bake_spirv_blob();
             }

             return static_cast<uint32_t>(m_spirv_blob.size() );
         }

    private:
        /* Private type declarations */
        typedef std::map<std::string, ExtensionBehavior>         ExtensionNameToExtensionBehaviorMap;
        typedef std::map<std::string, std::string>               DefinitionNameToValueMap;
        typedef std::vector<std::pair<std::string, std::string>> PlaceholderNameAndValueVector;

        /* Private functions */
        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(GLSLShaderToSPIRVGenerator);
        ANVIL_DISABLE_COPY_CONSTRUCTOR   (GLSLShaderToSPIRVGenerator);

        /* Constructor. Please see create() documentation for specification */
        explicit GLSLShaderToSPIRVGenerator(const Anvil::BaseDevice* in_device_ptr,
                                            const Mode&              in_mode,
                                            std::string              in_data,
                                            ShaderStage              in_shader_stage,
                                            SpvVersion               in_spirv_version);

        bool bake_glsl_source_code() const;

        #ifdef ANVIL_LINK_WITH_GLSLANG
            bool        bake_spirv_blob_by_calling_glslang(const char* in_body) const;
            EShLanguage get_glslang_shader_stage          () const;
        #else
            bool bake_spirv_blob_by_spawning_glslang_process(const std::string& in_glsl_filename_with_path,
                                                             const std::string& in_spirv_filename_with_path) const;
        #endif

        /* Private members */
        #ifdef ANVIL_LINK_WITH_GLSLANG
            mutable std::string            m_debug_info_log;
            std::unique_ptr<GLSLangLimits> m_limits_ptr;
            mutable std::string            m_program_debug_info_log;
            mutable std::string            m_program_info_log;
            mutable std::string            m_shader_info_log;
        #endif

        std::string m_data;
        Mode        m_mode;

        mutable std::string m_glsl_source_code;
        mutable bool        m_glsl_source_code_dirty;

        ShaderStage               m_shader_stage;
        SpvVersion                m_spirv_version;
        mutable std::vector<char> m_spirv_blob;

        DefinitionNameToValueMap            m_definition_values;
        ExtensionNameToExtensionBehaviorMap m_extension_behaviors;
        PlaceholderNameAndValueVector       m_placeholder_values;
        DefinitionNameToValueMap            m_pragmas;
    };
}; /* namespace Anvil */

#endif /* MISC_GLSL_TO_SPIRV_H */
