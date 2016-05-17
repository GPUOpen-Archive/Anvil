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

#include "config.h"
#include "misc/glsl_to_spirv.h"
#include "misc/io.h"
#include <sstream>

#ifndef _WIN32
    #include <unistd.h>
    #include <sys/wait.h>
#endif

#ifndef ANVIL_LINK_WITH_GLSLANG
    #define SPIRV_FILE_NAME_LEN 100
#else
    #include "glslang/SPIRV/GlslangToSpv.h"

    /* Helper class used as a process-wide RAII (de-)-initializer for glslangvalidator.
     * Also holds limits by the tool.
     **/
    class GLSLangInitializer
    {
    public:
        /* Constructor. */
        GLSLangInitializer()
        {
            glslang::InitializeProcess();

            m_resources.maxLights                                   = 32;
            m_resources.maxClipPlanes                               = 6;
            m_resources.maxTextureUnits                             = 32;
            m_resources.maxTextureCoords                            = 32;
            m_resources.maxVertexAttribs                            = 64;
            m_resources.maxVertexUniformComponents                  = 4096;
            m_resources.maxVaryingFloats                            = 64;
            m_resources.maxVertexTextureImageUnits                  = 32;
            m_resources.maxCombinedTextureImageUnits                = 80;
            m_resources.maxTextureImageUnits                        = 32;
            m_resources.maxFragmentUniformComponents                = 4096;
            m_resources.maxDrawBuffers                              = 32;
            m_resources.maxVertexUniformVectors                     = 128;
            m_resources.maxVaryingVectors                           = 8;
            m_resources.maxFragmentUniformVectors                   = 16;
            m_resources.maxVertexOutputVectors                      = 16;
            m_resources.maxFragmentInputVectors                     = 15;
            m_resources.minProgramTexelOffset                       = -8;
            m_resources.maxProgramTexelOffset                       = 7;
            m_resources.maxClipDistances                            = 8;
            m_resources.maxComputeWorkGroupCountX                   = 65535;
            m_resources.maxComputeWorkGroupCountY                   = 65535;
            m_resources.maxComputeWorkGroupCountZ                   = 65535;
            m_resources.maxComputeWorkGroupSizeX                    = 1024;
            m_resources.maxComputeWorkGroupSizeY                    = 1024;
            m_resources.maxComputeWorkGroupSizeZ                    = 64;
            m_resources.maxComputeUniformComponents                 = 1024;
            m_resources.maxComputeTextureImageUnits                 = 16;
            m_resources.maxComputeImageUniforms                     = 8;
            m_resources.maxComputeAtomicCounters                    = 8;
            m_resources.maxComputeAtomicCounterBuffers              = 1;
            m_resources.maxVaryingComponents                        = 60;
            m_resources.maxVertexOutputComponents                   = 64;
            m_resources.maxGeometryInputComponents                  = 64;
            m_resources.maxGeometryOutputComponents                 = 128;
            m_resources.maxFragmentInputComponents                  = 128;
            m_resources.maxImageUnits                               = 8;
            m_resources.maxCombinedImageUnitsAndFragmentOutputs     = 8;
            m_resources.maxCombinedShaderOutputResources            = 8;
            m_resources.maxImageSamples                             = 0;
            m_resources.maxVertexImageUniforms                      = 0;
            m_resources.maxTessControlImageUniforms                 = 0;
            m_resources.maxTessEvaluationImageUniforms              = 0;
            m_resources.maxGeometryImageUniforms                    = 0;
            m_resources.maxFragmentImageUniforms                    = 8;
            m_resources.maxCombinedImageUniforms                    = 8;
            m_resources.maxGeometryTextureImageUnits                = 16;
            m_resources.maxGeometryOutputVertices                   = 256;
            m_resources.maxGeometryTotalOutputComponents            = 1024;
            m_resources.maxGeometryUniformComponents                = 1024;
            m_resources.maxGeometryVaryingComponents                = 64;
            m_resources.maxTessControlInputComponents               = 128;
            m_resources.maxTessControlOutputComponents              = 128;
            m_resources.maxTessControlTextureImageUnits             = 16;
            m_resources.maxTessControlUniformComponents             = 1024;
            m_resources.maxTessControlTotalOutputComponents         = 4096;
            m_resources.maxTessEvaluationInputComponents            = 128;
            m_resources.maxTessEvaluationOutputComponents           = 128;
            m_resources.maxTessEvaluationTextureImageUnits          = 16;
            m_resources.maxTessEvaluationUniformComponents          = 1024;
            m_resources.maxTessPatchComponents                      = 120;
            m_resources.maxPatchVertices                            = 32;
            m_resources.maxTessGenLevel                             = 64;
            m_resources.maxViewports                                = 16;
            m_resources.maxVertexAtomicCounters                     = 0;
            m_resources.maxTessControlAtomicCounters                = 0;
            m_resources.maxTessEvaluationAtomicCounters             = 0;
            m_resources.maxGeometryAtomicCounters                   = 0;
            m_resources.maxFragmentAtomicCounters                   = 8;
            m_resources.maxCombinedAtomicCounters                   = 8;
            m_resources.maxAtomicCounterBindings                    = 1;
            m_resources.maxVertexAtomicCounterBuffers               = 0;
            m_resources.maxTessControlAtomicCounterBuffers          = 0;
            m_resources.maxTessEvaluationAtomicCounterBuffers       = 0;
            m_resources.maxGeometryAtomicCounterBuffers             = 0;
            m_resources.maxFragmentAtomicCounterBuffers             = 1;
            m_resources.maxCombinedAtomicCounterBuffers             = 1;
            m_resources.maxAtomicCounterBufferSize                  = 16384;
            m_resources.maxTransformFeedbackBuffers                 = 4;
            m_resources.maxTransformFeedbackInterleavedComponents   = 64;
            m_resources.maxCullDistances                            = 8;
            m_resources.maxCombinedClipAndCullDistances             = 8;
            m_resources.maxSamples                                  = 4;
            m_resources.limits.nonInductiveForLoops                 = 1;
            m_resources.limits.whileLoops                           = 1;
            m_resources.limits.doWhileLoops                         = 1;
            m_resources.limits.generalUniformIndexing               = 1;
            m_resources.limits.generalAttributeMatrixVectorIndexing = 1;
            m_resources.limits.generalVaryingIndexing               = 1;
            m_resources.limits.generalSamplerIndexing               = 1;
            m_resources.limits.generalVariableIndexing              = 1;
            m_resources.limits.generalConstantMatrixVectorIndexing  = 1;
        }

        /* Destructor */
        ~GLSLangInitializer()
        {
            glslang::FinalizeProcess();
        }

        /** Retrieves a pointer to an initialized TBuiltInResource instance */
        const TBuiltInResource* get_resource_ptr() const
        {
            return &m_resources;
        }

    private:
        /* Private fields */
        TBuiltInResource m_resources;
    };

    GLSLangInitializer glslang_helper;
#endif


/* Please see header for specification */
Anvil::GLSLShaderToSPIRVGenerator::GLSLShaderToSPIRVGenerator(const Mode& mode,
                                                              std::string data,
                                                              ShaderStage shader_stage)
    :m_data           (data),
     m_mode           (mode),
     m_shader_stage   (shader_stage),
     m_spirv_blob     (nullptr),
     m_spirv_blob_size(0)
{
    /* Stub */
}

/* Please see header for specification */
Anvil::GLSLShaderToSPIRVGenerator::~GLSLShaderToSPIRVGenerator()
{
    if (m_spirv_blob != nullptr)
    {
        delete [] m_spirv_blob;

        m_spirv_blob = nullptr;
    }
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::add_empty_definition(std::string definition_name)
{
    return add_definition_value_pair(definition_name,
                                     "");
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::add_definition_value_pair(std::string definition_name,
                                                                  std::string value)
{
    bool result = false;

    if (m_definitions.find(definition_name) != m_definitions.end() )
    {
        anvil_assert(false);

        goto end;
    }

    m_definitions[definition_name] = value;

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::add_definition_value_pair(std::string definition_name,
                                                                  int         value)
{
    std::stringstream value_sstream;

    value_sstream << value;

    return add_definition_value_pair(definition_name,
                                     value_sstream.str() );
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::bake_spirv_blob()
{
    std::string    final_glsl_source_string;
    bool           glsl_filename_is_temporary = false;
    std::string    glsl_filename_with_path;
    const uint32_t n_definitions              = (uint32_t) m_definitions.size();
    bool           result                     = false;

    {
        switch (m_mode)
        {
            case MODE_LOAD_SOURCE_FROM_FILE:
            {
                char* glsl_source = nullptr;

                Anvil::IO::read_file(m_data,
                                     true, /* is_text_file */
                                    &glsl_source,
                                     nullptr);  /* out_opt_size_ptr */

                if (glsl_source == nullptr)
                {
                    anvil_assert(glsl_source != nullptr);

                    goto end;
                }

                final_glsl_source_string = std::string(glsl_source);

                delete [] glsl_source;
                break;
            }

            case MODE_USE_SPECIFIED_SOURCE:
            {
                final_glsl_source_string = m_data;

                break;
            }

            default:
            {
                anvil_assert(false && "Unrecognized mode specified for a GLSLShaderToSPIRVGenerator instance.");
            }
        }
    }

    if (n_definitions > 0)
    {
        size_t glsl_source_string_second_line_index;

        /* Inject the #defines, starting from the second line. According to the spec, first line in
         * a GLSL shader must define the ESSL/GLSL version, and glslangvalidator seems to be pretty
         * strict about this. */
        glsl_source_string_second_line_index = final_glsl_source_string.find_first_of('\n') + 1;

        for (auto map_iterator  = m_definitions.begin();
                  map_iterator != m_definitions.end();
                ++map_iterator)
        {
            std::string current_key   = map_iterator->first;
            std::string current_value = map_iterator->second;
            std::string new_line      = std::string("#define ") + current_key + std::string(" ") + current_value + "\n";

            final_glsl_source_string.insert(glsl_source_string_second_line_index,
                                            new_line);
        }

        /* Form a temporary file name we will use to write the modified GLSL shader to. */
        #ifndef ANVIL_LINK_WITH_GLSLANG
        {
            switch (m_shader_stage)
            {
                case SHADER_STAGE_COMPUTE:                 glsl_filename_with_path = "temp.comp"; break;
                case SHADER_STAGE_FRAGMENT:                glsl_filename_with_path = "temp.frag"; break;
                case SHADER_STAGE_GEOMETRY:                glsl_filename_with_path = "temp.geom"; break;
                case SHADER_STAGE_TESSELLATION_CONTROL:    glsl_filename_with_path = "temp.tesc"; break;
                case SHADER_STAGE_TESSELLATION_EVALUATION: glsl_filename_with_path = "temp.tese"; break;
                case SHADER_STAGE_VERTEX:                  glsl_filename_with_path = "temp.vert"; break;

                default:
                {
                    anvil_assert(false);

                    goto end;
                }
            }

            /* Write down the file to a temporary location */
            Anvil::IO::write_file(glsl_filename_with_path,
                                  final_glsl_source_string);
        }
        #endif

        glsl_filename_is_temporary = true;
    }
    else
    {
        if (m_mode == MODE_LOAD_SOURCE_FROM_FILE)
        {
            glsl_filename_is_temporary = false;
            glsl_filename_with_path    = m_data;
        }
        else
        {
            #ifdef ANVIL_LINK_WITH_GLSLANG
            {
                /* That's fine, we don't need glsl_filename_with_path for this build */
            }
            #else
            {
                anvil_assert(false && "Unsupported mode specified for a GLSLShaderToSPIRVGenerator instance.");
            }
            #endif
        }
    }

    #ifdef ANVIL_LINK_WITH_GLSLANG
    {
        result = bake_spirv_blob_by_calling_glslang(final_glsl_source_string.c_str() );
    }
    #else
    {
        /* We need to point glslangvalidator at a location where it can stash the SPIR-V blob. */
        result = bake_spirv_blob_by_spawning_glslang_process(glsl_filename_with_path,
                                                             "temp.spv");
    }
    #endif

end:
    return result;
}

#ifdef ANVIL_LINK_WITH_GLSLANG
    /** Takes the GLSL source code, specified under @param body, converts it to SPIR-V and stores
     *  the blob data under m_spirv_blob & m_spirv_blob_size
     *
     *  @param body GLSL source code to use as input. Must not be nullptr.
     *
     *  @return true if successful, false otherwise.
     **/
    bool Anvil::GLSLShaderToSPIRVGenerator::bake_spirv_blob_by_calling_glslang(const char* body)
    {
        const EShLanguage         glslang_shader_stage = get_glslang_shader_stage();
        glslang::TIntermediate*   intermediate_ptr     = nullptr;
        glslang::TProgram*        new_program_ptr      = new glslang::TProgram();
        glslang::TShader*         new_shader_ptr       = new glslang::TShader(glslang_shader_stage);
        bool                      result               = false;
        std::vector<unsigned int> spirv_blob;

        anvil_assert(new_program_ptr != nullptr &&
                     new_shader_ptr != nullptr);

        if (new_program_ptr != nullptr &&
            new_shader_ptr  != nullptr)
        {
            /* Try to compile the shader */
            new_shader_ptr->setStrings(&body,
                                       1);

            result = new_shader_ptr->parse(glslang_helper.get_resource_ptr(),
                                           110,   /* defaultVersion    */
                                           false, /* forwardCompatible */
                                           (EShMessages) (EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules) );

            if (!result)
            {
                /* Shader compilation failed.. */
                fprintf(stderr,
                        "Shader compilation failed.");

                goto end;
            }

            /* Link the program */
            new_program_ptr->addShader(new_shader_ptr);

            if (!new_program_ptr->link(EShMsgDefault) )
            {
                /* Linking operation failed */
                fprintf(stderr,
                        "Program linking failed.");

                goto end;
            }

            /* Convert the intermediate representation to SPIR-V blob. */
            intermediate_ptr = new_program_ptr->getIntermediate(glslang_shader_stage);

            if (intermediate_ptr == nullptr)
            {
                anvil_assert(intermediate_ptr != nullptr);

                goto end;
            }

            glslang::GlslangToSpv(*intermediate_ptr,
                                  spirv_blob);

            if (spirv_blob.size() == 0)
            {
                anvil_assert(spirv_blob.size() != 0);

                goto end;
            }

            m_spirv_blob_size = spirv_blob.size() * sizeof(unsigned int);
            m_spirv_blob      = new char[m_spirv_blob_size];

            if (m_spirv_blob == nullptr)
            {
                anvil_assert(m_spirv_blob != nullptr);

                goto end;
            }

            memcpy(m_spirv_blob,
                  &spirv_blob[0],
                   m_spirv_blob_size);
        }

        /* All done */
        result = true;
    end:
        if (new_program_ptr != nullptr)
        {
            delete new_program_ptr;

            new_program_ptr = nullptr;
        }

        if (new_shader_ptr != nullptr)
        {
            delete new_shader_ptr;

            new_shader_ptr = nullptr;
        }

        return result;
    }

    /** Retrieves EShLanguage corresponding to m_shader_stage assigned to the GLSLShaderToSPIRVGenerator instance. **/
    EShLanguage Anvil::GLSLShaderToSPIRVGenerator::get_glslang_shader_stage() const
    {
        EShLanguage result = EShLangCount;

        switch (m_shader_stage)
        {
            case Anvil::SHADER_STAGE_COMPUTE:                 result = EShLangCompute;        break;
            case Anvil::SHADER_STAGE_FRAGMENT:                result = EShLangFragment;       break;
            case Anvil::SHADER_STAGE_GEOMETRY:                result = EShLangGeometry;       break;
            case Anvil::SHADER_STAGE_TESSELLATION_CONTROL:    result = EShLangTessControl;    break;
            case Anvil::SHADER_STAGE_TESSELLATION_EVALUATION: result = EShLangTessEvaluation; break;
            case Anvil::SHADER_STAGE_VERTEX:                  result = EShLangVertex;         break;

            default:
            {
                anvil_assert(false);
            }
        }

        return result;
    }
#else
    /** Reads contents of a file under location @param glsl_filename_with_path and treats the retrieved contents as GLSL source code,
     *  which is then used for GLSL->SPIRV conversion process. The result blob is stored at @param spirv_filename_with_path. The function
     *  then reads the blob contents and stores it under m_spirv_blob & m_spirv_blob_size
     *
     *  @param glsl_filename_with_path  As per description above. Must not be nullptr.
     *  @param spirv_filename_with_path As per description above. Must not be nullptr.
     *
     *  @return true if successful, false otherwise.
     **/
    bool Anvil::GLSLShaderToSPIRVGenerator::bake_spirv_blob_by_spawning_glslang_process(const std::string& glsl_filename_with_path,
                                                                                        const std::string& spirv_filename_with_path)
    {
        std::string glslangvalidator_params;
        bool        result                  = false;
        size_t      spirv_file_size         = 0;

        #ifdef _WIN32
        {
            /* Launch glslangvalidator and wait until it finishes doing the job */
            PROCESS_INFORMATION process_info;
            STARTUPINFO         startup_info;

            glslangvalidator_params = "dummy -V -o \"" + spirv_filename_with_path + "\" \"" + glsl_filename_with_path + "\"";

            memset(&process_info,
                   0,
                   sizeof(process_info) );
            memset(&startup_info,
                   0,
                   sizeof(startup_info) );

            startup_info.cb = sizeof(startup_info);

            if (!CreateProcess(".\\glslangValidator.exe",
                               (LPSTR) glslangvalidator_params.c_str(),
                               nullptr, /* lpProcessAttributes */
                               nullptr, /* lpThreadAttributes */
                               FALSE, /* bInheritHandles */
                               CREATE_NO_WINDOW, 
                               nullptr, /* lpEnvironment */
                               nullptr, /* lpCurrentDirectory */
                               &startup_info,
                               &process_info) )
            {
                anvil_assert(false);

                goto end;
            }

            /* Wait till glslangvalidator is done. */
            if (WaitForSingleObject(process_info.hProcess,
                                    INFINITE) != WAIT_OBJECT_0)
            {
                anvil_assert(false);

                goto end;
            }
        }
        #else
        {
            int32_t status;
            pid_t   child_pid = 0;

            child_pid = fork();

            if (child_pid == 0)
            {
                char* argv[6] = {(char*)"-S", (char*)"-V", (char*)"-o"};

                char  spirv_file_name[SPIRV_FILE_NAME_LEN];
                char  glsl_file_name[SPIRV_FILE_NAME_LEN];

                strcpy(spirv_file_name, spirv_filename_with_path.c_str());
                strcpy(glsl_file_name, glsl_filename_with_path.c_str());

                argv[3] = spirv_file_name;
                argv[4] = glsl_file_name;
                argv[5] = (char*)0;

                int32_t flag = execv("./glslangValidator", (char* const*)argv);
                if (flag == -1)
                {
                    anvil_assert(false);
                    goto end;
                }
            }
            else
            {
                do
                {
                    pid_t wpid = waitpid(child_pid, &status, WUNTRACED | WCONTINUED);
                    if (wpid == -1)
                    {
                        anvil_assert(false);
                        goto end;
                    }
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            }
        }
        #endif

        /* Now, read the SPIR-V file contents */
        Anvil::IO::read_file(spirv_filename_with_path.c_str(),
                             false, /* is_text_file */
                            &m_spirv_blob,
                            &spirv_file_size);

        if (m_spirv_blob == nullptr)
        {
            anvil_assert(m_spirv_blob != nullptr);

            goto end;
        }

        if (spirv_file_size <= 0)
        {
            anvil_assert(spirv_file_size > 0);

            goto end;
        }

        /* No need to keep the file any more. */
        Anvil::IO::delete_file(spirv_filename_with_path);

        m_spirv_blob_size = (uint32_t) spirv_file_size;
        result            = true;

    end:
        return result;
    }
#endif