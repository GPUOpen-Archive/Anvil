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

#include "misc/glsl_to_spirv.h"
#include "misc/io.h"
#include "misc/object_tracker.h"
#include "wrappers/device.h"
#include "wrappers/shader_module.h"
#include <algorithm>
#include <sstream>

#ifndef _WIN32
    #include <limits.h>
    #include <unistd.h>
    #include <sys/wait.h>
#endif

#ifdef ANVIL_LINK_WITH_GLSLANG
    #ifdef max
        #undef max
    #endif
    #ifdef min
        #undef min
    #endif
    #undef snprintf

    #if defined(_MSC_VER)
        #pragma warning(push)
        #pragma warning(disable: 4100)
        #pragma warning(disable: 4365)
        #pragma warning(disable: 4625)
    #endif

    #include "glslang/SPIRV/GlslangToSpv.h"

    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif
#endif

#ifndef ANVIL_LINK_WITH_GLSLANG
    #define SPIRV_FILE_NAME_LEN 100
#else
    /* Helper class used as a process-wide RAII (de-)-initializer for glslangvalidator. **/
    class GLSLangGlobalInitializer
    {
    public:
        /* Constructor. */
        GLSLangGlobalInitializer()
        {
            glslang::InitializeProcess();
        }

        /* Destructor */
        ~GLSLangGlobalInitializer()
        {
            glslang::FinalizeProcess();
        }
    };

    /* Constructor. */
    Anvil::GLSLangLimits::GLSLangLimits(const Anvil::BaseDevice* in_device_ptr)
    {
        const auto&             limits                         = in_device_ptr->get_physical_device_properties().core_vk1_0_properties_ptr->limits;
        Anvil::SampleCountFlags max_sampled_image_sample_count;
        int32_t                 max_sampled_image_samples      = 0;
        auto                    max_storage_image_sample_count = limits.storage_image_sample_counts;
        int32_t                 max_storage_image_samples      = 0;

        max_sampled_image_sample_count = std::max<Anvil::SampleCountFlags>(limits.sampled_image_color_sample_counts, limits.sampled_image_depth_sample_counts);
        max_sampled_image_sample_count = std::max<Anvil::SampleCountFlags>(max_sampled_image_sample_count,           limits.sampled_image_integer_sample_counts);
        max_sampled_image_sample_count = std::max<Anvil::SampleCountFlags>(max_sampled_image_sample_count,           limits.sampled_image_stencil_sample_counts);

        const struct SampleCountToSamplesData
        {
            Anvil::SampleCountFlags sample_count;
            int32_t*                out_result_ptr;
        } conversion_items[] =
        {
            {max_sampled_image_sample_count, &max_sampled_image_samples},
            {max_storage_image_sample_count, &max_storage_image_samples}
        };
        const uint32_t n_conversion_items = sizeof(conversion_items) / sizeof(conversion_items[0]);

        for (uint32_t n_conversion_item = 0;
                      n_conversion_item < n_conversion_items;
                    ++n_conversion_item)
        {
            const SampleCountToSamplesData& current_item = conversion_items[n_conversion_item];
            int32_t                         result       = 1;

            if ((current_item.sample_count & Anvil::SampleCountFlagBits::_16_BIT) != 0)
            {
                result = 16;
            }
            else
            if ((current_item.sample_count & Anvil::SampleCountFlagBits::_8_BIT) != 0)
            {
                result = 8;
            }
            else
            if ((current_item.sample_count & Anvil::SampleCountFlagBits::_4_BIT) != 0)
            {
                result = 4;
            }
            else
            if ((current_item.sample_count & Anvil::SampleCountFlagBits::_2_BIT) != 0)
            {
                result = 2;
            }

            *current_item.out_result_ptr = result;
        }

        #define CLAMP_TO_INT_MAX(x) ((x <= INT_MAX) ? x : INT_MAX)

        m_resources_ptr.reset(
            new TBuiltInResource()
        );

        m_resources_ptr->maxLights                                   = 32; /* irrelevant to Vulkan */
        m_resources_ptr->maxClipPlanes                               = 6;  /* irrelevant to Vulkan */
        m_resources_ptr->maxTextureUnits                             = 32; /* irrelevant to Vulkan */
        m_resources_ptr->maxTextureCoords                            = 32; /* irrelevant to Vulkan */
        m_resources_ptr->maxVertexAttribs                            = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_vertex_input_attributes) );
        m_resources_ptr->maxVertexUniformComponents                  = 4096; /* irrelevant to Vulkan  */
        m_resources_ptr->maxVaryingFloats                            = 64;   /* irrelevant to Vulkan? */
        m_resources_ptr->maxVertexTextureImageUnits                  = 32;   /* irrelevant to Vulkan? */
        m_resources_ptr->maxCombinedTextureImageUnits                = 80;   /* irrelevant to Vulkan? */
        m_resources_ptr->maxTextureImageUnits                        = 32;   /* irrelevant to Vulkan? */
        m_resources_ptr->maxFragmentUniformComponents                = 4096; /* irrelevant to Vulkan? */
        m_resources_ptr->maxDrawBuffers                              = 32;   /* irrelevant to Vulkan  */
        m_resources_ptr->maxVertexUniformVectors                     = 128;  /* irrelevant to Vulkan? */
        m_resources_ptr->maxVaryingVectors                           = 8;    /* irrelevant to Vulkan? */
        m_resources_ptr->maxFragmentUniformVectors                   = 16;   /* irrelevant to Vulkan? */
        m_resources_ptr->maxVertexOutputVectors                      = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_vertex_output_components  / 4) );
        m_resources_ptr->maxFragmentInputVectors                     = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_fragment_input_components / 4) );
        m_resources_ptr->minProgramTexelOffset                       = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.min_texel_offset) );
        m_resources_ptr->maxProgramTexelOffset                       = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_texel_offset) );
        m_resources_ptr->maxClipDistances                            = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_clip_distances) );
        m_resources_ptr->maxComputeWorkGroupCountX                   = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_compute_work_group_count[0]) );
        m_resources_ptr->maxComputeWorkGroupCountY                   = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_compute_work_group_count[1]) );
        m_resources_ptr->maxComputeWorkGroupCountZ                   = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_compute_work_group_count[2]) );
        m_resources_ptr->maxComputeWorkGroupSizeX                    = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_compute_work_group_size[0]) );
        m_resources_ptr->maxComputeWorkGroupSizeY                    = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_compute_work_group_size[1]) );
        m_resources_ptr->maxComputeWorkGroupSizeZ                    = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_compute_work_group_size[2]) );
        m_resources_ptr->maxComputeUniformComponents                 = 1024; /* irrelevant to Vulkan? */
        m_resources_ptr->maxComputeTextureImageUnits                 = 16;   /* irrelevant to Vulkan? */
        m_resources_ptr->maxComputeImageUniforms                     = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_per_stage_descriptor_storage_images) );
        m_resources_ptr->maxComputeAtomicCounters                    = 8;    /* irrelevant to Vulkan */
        m_resources_ptr->maxComputeAtomicCounterBuffers              = 1;    /* irrelevant to Vulkan */
        m_resources_ptr->maxVaryingComponents                        = 60;   /* irrelevant to Vulkan */
        m_resources_ptr->maxVertexOutputComponents                   = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_vertex_output_components) );
        m_resources_ptr->maxGeometryInputComponents                  = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_geometry_input_components) );
        m_resources_ptr->maxGeometryOutputComponents                 = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_geometry_output_components) );
        m_resources_ptr->maxFragmentInputComponents                  = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_fragment_input_components) );
        m_resources_ptr->maxImageUnits                               = 8; /* irrelevant to Vulkan */
        m_resources_ptr->maxCombinedImageUnitsAndFragmentOutputs     = 8; /* irrelevant to Vulkan? */
        m_resources_ptr->maxCombinedShaderOutputResources            = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_fragment_combined_output_resources) );
        m_resources_ptr->maxImageSamples                             = max_storage_image_samples;
        m_resources_ptr->maxVertexImageUniforms                      = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_per_stage_descriptor_storage_images) );
        m_resources_ptr->maxTessControlImageUniforms                 = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_per_stage_descriptor_storage_images) );
        m_resources_ptr->maxTessEvaluationImageUniforms              = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_per_stage_descriptor_storage_images) );
        m_resources_ptr->maxGeometryImageUniforms                    = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_per_stage_descriptor_storage_images) );
        m_resources_ptr->maxFragmentImageUniforms                    = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_per_stage_descriptor_storage_images) );
        m_resources_ptr->maxCombinedImageUniforms                    = static_cast<int32_t>(CLAMP_TO_INT_MAX(5 /* vs, tc, te, gs, fs */ * limits.max_per_stage_descriptor_storage_images) );
        m_resources_ptr->maxGeometryTextureImageUnits                = 16; /* irrelevant to Vulkan? */
        m_resources_ptr->maxGeometryOutputVertices                   = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_geometry_output_vertices) );
        m_resources_ptr->maxGeometryTotalOutputComponents            = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_geometry_total_output_components) );
        m_resources_ptr->maxGeometryUniformComponents                = 1024; /* irrelevant to Vulkan? */
        m_resources_ptr->maxGeometryVaryingComponents                = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_geometry_input_components) );
        m_resources_ptr->maxTessControlInputComponents               = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_tessellation_control_per_vertex_input_components) );
        m_resources_ptr->maxTessControlOutputComponents              = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_tessellation_control_per_vertex_output_components) );
        m_resources_ptr->maxTessControlTextureImageUnits             = 16;   /* irrelevant to Vulkan? */
        m_resources_ptr->maxTessControlUniformComponents             = 1024; /* irrelevant to Vulkan? */
        m_resources_ptr->maxTessControlTotalOutputComponents         = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_tessellation_control_total_output_components) );
        m_resources_ptr->maxTessEvaluationInputComponents            = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_tessellation_evaluation_input_components) );
        m_resources_ptr->maxTessEvaluationOutputComponents           = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_tessellation_evaluation_output_components) );
        m_resources_ptr->maxTessEvaluationTextureImageUnits          = 16; /* irrelevant to Vulkan? */
        m_resources_ptr->maxTessEvaluationUniformComponents          = 1024; /* irrelevant to Vulkan? */
        m_resources_ptr->maxTessPatchComponents                      = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_tessellation_control_per_patch_output_components) );
        m_resources_ptr->maxPatchVertices                            = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_tessellation_patch_size) );
        m_resources_ptr->maxTessGenLevel                             = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_tessellation_generation_level) );
        m_resources_ptr->maxViewports                                = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_viewports) );
        m_resources_ptr->maxVertexAtomicCounters                     = 0; /* not supported in Vulkan */
        m_resources_ptr->maxTessControlAtomicCounters                = 0; /* not supported in Vulkan */
        m_resources_ptr->maxTessEvaluationAtomicCounters             = 0; /* not supported in Vulkan */
        m_resources_ptr->maxGeometryAtomicCounters                   = 0; /* not supported in Vulkan */
        m_resources_ptr->maxFragmentAtomicCounters                   = 0; /* not supported in Vulkan */
        m_resources_ptr->maxCombinedAtomicCounters                   = 0; /* not supported in Vulkan */
        m_resources_ptr->maxAtomicCounterBindings                    = 0; /* not supported in Vulkan */
        m_resources_ptr->maxVertexAtomicCounterBuffers               = 0; /* not supported in Vulkan */
        m_resources_ptr->maxTessControlAtomicCounterBuffers          = 0; /* not supported in Vulkan */
        m_resources_ptr->maxTessEvaluationAtomicCounterBuffers       = 0; /* not supported in Vulkan */
        m_resources_ptr->maxGeometryAtomicCounterBuffers             = 0; /* not supported in Vulkan */
        m_resources_ptr->maxFragmentAtomicCounterBuffers             = 0; /* not supported in Vulkan */
        m_resources_ptr->maxCombinedAtomicCounterBuffers             = 0; /* not supported in Vulkan */
        m_resources_ptr->maxAtomicCounterBufferSize                  = 0; /* not supported in Vulkan */
        m_resources_ptr->maxCullDistances                            = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_cull_distances) );
        m_resources_ptr->maxCombinedClipAndCullDistances             = static_cast<int32_t>(CLAMP_TO_INT_MAX(limits.max_combined_clip_and_cull_distances) );
        m_resources_ptr->maxSamples                                  = (max_sampled_image_samples > max_storage_image_samples) ? CLAMP_TO_INT_MAX(max_sampled_image_samples)
                                                                                                                               : CLAMP_TO_INT_MAX(max_storage_image_samples);
        m_resources_ptr->limits.nonInductiveForLoops                 = 1;
        m_resources_ptr->limits.whileLoops                           = 1;
        m_resources_ptr->limits.doWhileLoops                         = 1;
        m_resources_ptr->limits.generalUniformIndexing               = 1;
        m_resources_ptr->limits.generalAttributeMatrixVectorIndexing = 1;
        m_resources_ptr->limits.generalVaryingIndexing               = 1;
        m_resources_ptr->limits.generalSamplerIndexing               = 1;
        m_resources_ptr->limits.generalVariableIndexing              = 1;
        m_resources_ptr->limits.generalConstantMatrixVectorIndexing  = 1;

        if (in_device_ptr->get_extension_info()->ext_transform_feedback() )
        {
            const auto xfb_props_ptr = in_device_ptr->get_physical_device_properties().ext_transform_feedback_properties_ptr;

            m_resources_ptr->maxTransformFeedbackBuffers                 = xfb_props_ptr->n_max_transform_feedback_buffers;
            m_resources_ptr->maxTransformFeedbackInterleavedComponents   = xfb_props_ptr->max_transform_feedback_buffer_data_stride * 4;
        }
        else
        {
            m_resources_ptr->maxTransformFeedbackBuffers               = 0; /* not supported in core Vulkan */
            m_resources_ptr->maxTransformFeedbackInterleavedComponents = 0; /* not supported in core Vulkan */
        }
    }

    static const GLSLangGlobalInitializer glslang_helper;
#endif


/* Please see header for specification */
Anvil::GLSLShaderToSPIRVGenerator::GLSLShaderToSPIRVGenerator(const Anvil::BaseDevice* in_device_ptr,
                                                              const Mode&              in_mode,
                                                              std::string              in_data,
                                                              ShaderStage              in_shader_stage,
                                                              SpvVersion               in_spirv_version)
    :CallbacksSupportProvider(GLSL_SHADER_TO_SPIRV_GENERATOR_CALLBACK_ID_COUNT),
     m_data                  (in_data),
     m_glsl_source_code_dirty(true),
     m_mode                  (in_mode),
     m_shader_stage          (in_shader_stage),
     m_spirv_version         (in_spirv_version)
{
    #ifdef ANVIL_LINK_WITH_GLSLANG
    {
        if (in_device_ptr != nullptr)
        {
            m_limits_ptr.reset(
                new GLSLangLimits(in_device_ptr)
            );
        }
    }
    #endif
}

/* Please see header for specification */
Anvil::GLSLShaderToSPIRVGenerator::~GLSLShaderToSPIRVGenerator()
{
    auto object_tracker_ptr = Anvil::ObjectTracker::get();

    object_tracker_ptr->unregister_object(Anvil::ObjectType::ANVIL_GLSL_SHADER_TO_SPIRV_GENERATOR,
                                          this);

    m_spirv_blob.clear();
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::add_empty_definition(std::string in_definition_name)
{
    return add_definition_value_pair(in_definition_name,
                                     "");
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::add_extension_behavior(std::string       in_extension_name,
                                                               ExtensionBehavior in_behavior)
{
    bool result = false;

    if (m_extension_behaviors.find(in_extension_name) != m_extension_behaviors.end() )
    {
        anvil_assert_fail();

        goto end;
    }

    m_extension_behaviors[in_extension_name] = in_behavior;

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::add_definition_value_pair(std::string in_definition_name,
                                                                  std::string in_value)
{
    bool result = false;

    if (m_definition_values.find(in_definition_name) != m_definition_values.end() )
    {
        anvil_assert_fail();

        goto end;
    }

    m_definition_values[in_definition_name] = in_value;

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::add_placeholder_value_pair(const std::string& in_placeholder_name,
                                                                   const std::string& in_value)
{
    bool result = false;

    for (const auto& placeholder_value_pair : m_placeholder_values)
    {
        if (placeholder_value_pair.first == in_placeholder_name)
        {
            anvil_assert_fail();

            goto end;
        }
    }

    m_placeholder_values.push_back(std::make_pair(in_placeholder_name, in_value));

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::add_pragma(std::string in_pragma_name,
                                                   std::string in_opt_value)
{
    bool result = false;

    if (m_pragmas.find(in_pragma_name) != m_pragmas.end() )
    {
        anvil_assert_fail();

        goto end;
    }

    m_pragmas[in_pragma_name] = in_opt_value;

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::bake_glsl_source_code() const
{
    std::string    final_glsl_source_string;
    const uint32_t n_definition_values        = static_cast<uint32_t>(m_definition_values.size() );
    const uint32_t n_extension_behaviors      = static_cast<uint32_t>(m_extension_behaviors.size() );
    const uint32_t n_placeholder_values       = static_cast<uint32_t>(m_placeholder_values.size());
    const uint32_t n_pragmas                  = static_cast<uint32_t>(m_pragmas.size() );
    bool           result                     = false;

    anvil_assert(m_glsl_source_code_dirty);

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
            /* Unrecognized mode specified for a GLSLShaderToSPIRVGenerator instance. */
            anvil_assert_fail();

            goto end;
        }
    }

    if (n_pragmas             > 0 ||
        n_placeholder_values  > 0 ||
        n_extension_behaviors > 0 ||
        n_definition_values   > 0)
    {
        size_t glsl_source_string_second_line_index;

        /* Inject extension behavior definitions, starting from the second line. According to the spec, first line in
         * a GLSL shader must define the ESSL/GLSL version, and glslangvalidator seems to be pretty
         * strict about this. */
        glsl_source_string_second_line_index = final_glsl_source_string.find_first_of('\n') + 1;

        for (auto map_iterator  = m_extension_behaviors.begin();
                  map_iterator != m_extension_behaviors.end();
                ++map_iterator)
        {
            const ExtensionBehavior& current_extension_behavior      = map_iterator->second;
            std::string              current_extension_behavior_glsl = get_extension_behavior_glsl_code(current_extension_behavior);
            std::string              current_extension_name          = map_iterator->first;
            std::string              new_line                        = std::string("#extension ")      +
                                                                       current_extension_name          +
                                                                       std::string(" : ")              +
                                                                       current_extension_behavior_glsl +
                                                                       "\n";

            final_glsl_source_string.insert(glsl_source_string_second_line_index,
                                            new_line);

            glsl_source_string_second_line_index += new_line.length();
        }

        /* Follow with #defines which associate values with definition names */
        for (auto map_iterator  = m_definition_values.begin();
                  map_iterator != m_definition_values.end();
                ++map_iterator)
        {
            std::string current_key   = map_iterator->first;
            std::string current_value = map_iterator->second;
            std::string new_line      = std::string("#define ") + current_key + std::string(" ") + current_value + "\n";

            final_glsl_source_string.insert(glsl_source_string_second_line_index,
                                            new_line);
        }

        /* Next define pragmas */
        for (auto& current_pragma : m_pragmas)
        {
            std::string pragma_name  = current_pragma.first;
            std::string pragma_value = current_pragma.second;
            std::string new_line     = std::string("#pragma ") + pragma_name + std::string(" ") + pragma_value + "\n";

            final_glsl_source_string.insert(glsl_source_string_second_line_index,
                                            new_line);
        }

        /* Finish with replacing placeholders with values */
        for(auto vec_iterator  = m_placeholder_values.begin();
                 vec_iterator != m_placeholder_values.end();
               ++vec_iterator)
        {
            const std::string& current_key   = vec_iterator->first;
            const std::string& current_value = vec_iterator->second;
            size_t glsl_source_string_pos    = final_glsl_source_string.find(current_key, 0);

            while (glsl_source_string_pos != std::string::npos)
            {
                final_glsl_source_string.replace(glsl_source_string_pos, current_key.size(), current_value);

                glsl_source_string_pos = final_glsl_source_string.find(current_key, glsl_source_string_pos);
            }
        }
    }

    /* Cache the GLSL source code used for the conversion */
    m_glsl_source_code = final_glsl_source_string;

    /* All done */
    m_glsl_source_code_dirty = false;
    result = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::GLSLShaderToSPIRVGenerator::bake_spirv_blob() const
{
    bool           glsl_filename_is_temporary = false;
    std::string    glsl_filename_with_path;
    bool           result                     = false;

    ANVIL_REDUNDANT_VARIABLE(glsl_filename_is_temporary);

    if (m_glsl_source_code_dirty)
    {
        bake_glsl_source_code();

        anvil_assert(!m_glsl_source_code_dirty);
    }

    if (m_mode == MODE_LOAD_SOURCE_FROM_FILE)
    {
        glsl_filename_is_temporary = false;
        glsl_filename_with_path    = m_data;
    }

    /* Form a temporary file name we will use to write the modified GLSL shader to. */
    #ifndef ANVIL_LINK_WITH_GLSLANG
    {
        switch (m_shader_stage)
        {
            case ShaderStage::COMPUTE:                 glsl_filename_with_path = "temp.comp"; break;
            case ShaderStage::FRAGMENT:                glsl_filename_with_path = "temp.frag"; break;
            case ShaderStage::GEOMETRY:                glsl_filename_with_path = "temp.geom"; break;
            case ShaderStage::TESSELLATION_CONTROL:    glsl_filename_with_path = "temp.tesc"; break;
            case ShaderStage::TESSELLATION_EVALUATION: glsl_filename_with_path = "temp.tese"; break;
            case ShaderStage::VERTEX:                  glsl_filename_with_path = "temp.vert"; break;

            default:
            {
                anvil_assert_fail();

                goto end;
            }
        }

        /* Write down the file to a temporary location */
        Anvil::IO::write_text_file(glsl_filename_with_path,
                                   m_glsl_source_code);

        glsl_filename_is_temporary = true;
    }
    #endif


    #ifdef ANVIL_LINK_WITH_GLSLANG
    {
        /* Shader modules are cached throughout Instance's lifetime in Anvil. It might just happen that
         * the shader we're about to convert to SPIR-V representation has already been converted in the past.
         *
         * Given that the conversion process can be time-consuming, let's try to see if any of the living
         * shader module instances already use exactly the same source code.
         */
        uint32_t n_current_shader_module = 0;
        auto     object_tracker_ptr      = Anvil::ObjectTracker::get();

        do
        {
            auto                       shader_module_raw_ptr = object_tracker_ptr->get_object_at_index     (Anvil::ObjectType::SHADER_MODULE,
                                                                                                            n_current_shader_module);
            const Anvil::ShaderModule* shader_module_ptr     = reinterpret_cast<const Anvil::ShaderModule*>(shader_module_raw_ptr);

            if (shader_module_raw_ptr == nullptr)
            {
                /* Out of shader module instances. */
                break;
            }

            if (shader_module_ptr->get_glsl_source_code() == m_glsl_source_code)
            {
                const auto reference_spirv_blob               = shader_module_ptr->get_spirv_blob();
                const auto reference_spirv_blob_size_in_bytes = reference_spirv_blob.size() * sizeof(reference_spirv_blob.at(0) );

                anvil_assert(reference_spirv_blob_size_in_bytes != 0);

                m_spirv_blob.resize(reference_spirv_blob_size_in_bytes);

                memcpy(&m_spirv_blob.at        (0),
                       &reference_spirv_blob.at(0),
                       reference_spirv_blob_size_in_bytes);

                result = true;
                break;
            }

            /* Move to the next shader module instance */
            ++n_current_shader_module;
        }
        while (n_current_shader_module != 0); /* work around "conditional expression is constant" warnings issued by some compilers */

        if (m_spirv_blob.size() == 0)
        {
            /* Need to bake a brand new SPIR-V blob */
            result = bake_spirv_blob_by_calling_glslang(m_glsl_source_code.c_str() );
        }
    }

    #else
    {
        /* We need to point glslangvalidator at a location where it can stash the SPIR-V blob. */
        result = bake_spirv_blob_by_spawning_glslang_process(glsl_filename_with_path,
                                                             "temp.spv");
    }

end:
    #endif



    return result;
}

#ifdef ANVIL_LINK_WITH_GLSLANG
    /** Takes the GLSL source code, specified under @param body, converts it to SPIR-V and stores
     *  the blob data under m_spirv_blob.
     *
     *  @param body GLSL source code to use as input. Must not be nullptr.
     *
     *  @return true if successful, false otherwise.
     **/
    bool Anvil::GLSLShaderToSPIRVGenerator::bake_spirv_blob_by_calling_glslang(const char* in_body) const
    {
        const EShLanguage         glslang_shader_stage = get_glslang_shader_stage();
        glslang::TIntermediate*   intermediate_ptr     = nullptr;
        glslang::TProgram*        new_program_ptr      = new glslang::TProgram();
        glslang::TShader*         new_shader_ptr       = new glslang::TShader(glslang_shader_stage);
        bool                      result               = false;
        std::vector<unsigned int> spirv_blob;

        anvil_assert(new_program_ptr != nullptr &&
                     new_shader_ptr  != nullptr);

        /* If this assertion check explodes, you're trying to build a SPIR-V blob with a generator, which has
         * been initialized with a null device instance. This is illegal.
         */
        OnGLSLToSPIRVConversionAboutToBeStartedCallbackArgument conversion_about_to_be_started_callback_arg(this);
        OnGLSLToSPIRVConversionFinishedCallbackArgument         conversion_finished_callback_arg           (this);

        anvil_assert(m_limits_ptr != nullptr);

        callback(GLSL_SHADER_TO_SPIRV_GENERATOR_CALLBACK_ID_CONVERSION_ABOUT_TO_START,
                &conversion_about_to_be_started_callback_arg);

        if (new_program_ptr != nullptr &&
            new_shader_ptr  != nullptr)
        {
            bool                              link_result = false;
            glslang::EShTargetLanguageVersion spirv_version;

            /* Try to compile the shader */
            new_shader_ptr->setStrings(&in_body,
                                       1);

            switch (m_spirv_version)
            {
                case SpvVersion::_1_0:
                {
                    spirv_version = glslang::EShTargetSpv_1_0;

                    break;
                }

                case SpvVersion::_1_1:
                {
                    spirv_version = glslang::EShTargetSpv_1_1;

                    break;
                }

                case SpvVersion::_1_2:
                {
                    spirv_version = glslang::EShTargetSpv_1_2;

                    break;
                }

                case SpvVersion::_1_3:
                {
                    spirv_version = glslang::EShTargetSpv_1_3;

                    break;
                }

                case SpvVersion::_1_4:
                {
                    spirv_version = glslang::EShTargetSpv_1_4;

                    break;
                }

                default:
                {
                    spirv_version = glslang::EShTargetSpv_1_0;

                    break;
                }
            }

            new_shader_ptr->setEnvTarget(glslang::EShTargetSpv,
                                         spirv_version);

            result = new_shader_ptr->parse(m_limits_ptr->get_resource_ptr(),
                                           110,   /* defaultVersion    */
                                           false, /* forwardCompatible */
                                           (EShMessages) (EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules) );

            m_debug_info_log  = new_shader_ptr->getInfoDebugLog();
            m_shader_info_log = new_shader_ptr->getInfoLog();

            if (!result)
            {
                /* Shader compilation failed.. */
                fprintf(stderr,
                        "Shader compilation failed. Error log is:\n>>>\n%s\n<<<",
                        m_shader_info_log.c_str() );

                goto end;
            }

            /* Link the program */
            new_program_ptr->addShader(new_shader_ptr);

            link_result              = new_program_ptr->link           (EShMsgDefault);
            m_program_debug_info_log = new_program_ptr->getInfoDebugLog();
            m_program_info_log       = new_program_ptr->getInfoLog     ();

            if (!link_result)
            {
                /* Linking operation failed */
                fprintf(stderr,
                        "Program linking failed. Error log is:\n>>>\n%s\n<<<",
                        m_program_info_log.c_str() );

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

            m_spirv_blob.resize(static_cast<uint32_t>(spirv_blob.size() ) * sizeof(unsigned int) );

            memcpy(&m_spirv_blob.at(0),
                   &spirv_blob[0],
                   m_spirv_blob.size() );

        }

        callback(GLSL_SHADER_TO_SPIRV_GENERATOR_CALLBACK_ID_CONVERSION_FINISHED,
                &conversion_finished_callback_arg);

        /* All done */
        result = true;
    end:
        delete new_program_ptr;
        new_program_ptr = nullptr;

        delete new_shader_ptr;
        new_shader_ptr = nullptr;

        return result;
    }

    /** Retrieves EShLanguage corresponding to m_shader_stage assigned to the GLSLShaderToSPIRVGenerator instance. **/
    EShLanguage Anvil::GLSLShaderToSPIRVGenerator::get_glslang_shader_stage() const
    {
        EShLanguage result = EShLangCount;

        switch (m_shader_stage)
        {
            case Anvil::ShaderStage::COMPUTE:                 result = EShLangCompute;        break;
            case Anvil::ShaderStage::FRAGMENT:                result = EShLangFragment;       break;
            case Anvil::ShaderStage::GEOMETRY:                result = EShLangGeometry;       break;
            case Anvil::ShaderStage::TESSELLATION_CONTROL:    result = EShLangTessControl;    break;
            case Anvil::ShaderStage::TESSELLATION_EVALUATION: result = EShLangTessEvaluation; break;
            case Anvil::ShaderStage::VERTEX:                  result = EShLangVertex;         break;

            default:
            {
                anvil_assert_fail();
            }
        }

        return result;
    }
#else
    /** Reads contents of a file under location @param glsl_filename_with_path and treats the retrieved contents as GLSL source code,
     *  which is then used for GLSL->SPIRV conversion process. The result blob is stored at @param spirv_filename_with_path. The function
     *  then reads the blob contents and stores it under m_spirv_blob.
     *
     *  @param glsl_filename_with_path  As per description above. Must not be nullptr.
     *  @param spirv_filename_with_path As per description above. Must not be nullptr.
     *
     *  @return true if successful, false otherwise.
     **/
    bool Anvil::GLSLShaderToSPIRVGenerator::bake_spirv_blob_by_spawning_glslang_process(const std::string& in_glsl_filename_with_path,
                                                                                        const std::string& in_spirv_filename_with_path) const
    {
        auto        callback_arg            = OnGLSLToSPIRVConversionAboutToBeStartedCallbackArgument(this);
        std::string glslangvalidator_params;
        bool        result                  = false;
        size_t      spirv_file_size         = 0;
        char*       spirv_blob_ptr          = nullptr;

        callback(GLSL_SHADER_TO_SPIRV_GENERATOR_CALLBACK_ID_CONVERSION_ABOUT_TO_START,
                &callback_arg);

        #ifdef _WIN32
        {
            /* Launch glslangvalidator and wait until it finishes doing the job */
            PROCESS_INFORMATION process_info;
            STARTUPINFO         startup_info;

            glslangvalidator_params = "dummy -V -o \"" + in_spirv_filename_with_path + "\" \"" + in_glsl_filename_with_path + "\"";

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
                anvil_assert_fail();

                goto end;
            }

            /* Wait till glslangvalidator is done. */
            if (WaitForSingleObject(process_info.hProcess,
                                    INFINITE) != WAIT_OBJECT_0)
            {
                anvil_assert_fail();

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
                char  glsl_file_name [SPIRV_FILE_NAME_LEN];

                strcpy(spirv_file_name, in_spirv_filename_with_path.c_str());
                strcpy(glsl_file_name,  in_glsl_filename_with_path.c_str());

                argv[3] = spirv_file_name;
                argv[4] = glsl_file_name;
                argv[5] = (char*)0;

                int32_t flag = execv("./glslangValidator", (char* const*)argv);
                if (flag == -1)
                {
                    anvil_assert_fail();
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
                        anvil_assert_fail();
                        goto end;
                    }
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            }
        }
        #endif

        /* Now, read the SPIR-V file contents */


        Anvil::IO::read_file(in_spirv_filename_with_path.c_str(),
                             false, /* is_text_file */
                            &spirv_blob_ptr,
                            &spirv_file_size);

        if (spirv_blob_ptr == nullptr)
        {
            anvil_assert(spirv_blob_ptr != nullptr);

            goto end;
        }

        if (spirv_file_size <= 0)
        {
            anvil_assert(spirv_file_size > 0);

            goto end;
        }

        /* No need to keep the file any more. */
        Anvil::IO::delete_file(in_spirv_filename_with_path);

        m_spirv_blob.resize(spirv_file_size);

        memcpy(&m_spirv_blob.at(0),
               spirv_blob_ptr,
               spirv_file_size);

        delete [] spirv_blob_ptr;
        spirv_blob_ptr = nullptr;

        result = true;

    end:
        return result;
    }
#endif

/* Please see header for specification */
Anvil::GLSLShaderToSPIRVGeneratorUniquePtr Anvil::GLSLShaderToSPIRVGenerator::create(const Anvil::BaseDevice* in_opt_device_ptr,
                                                                                     const Mode&              in_mode,
                                                                                     std::string              in_data,
                                                                                     ShaderStage              in_shader_stage,
                                                                                     SpvVersion               in_spirv_version)
{
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr result_ptr(nullptr,
                                                          std::default_delete<Anvil::GLSLShaderToSPIRVGenerator>() );

    result_ptr.reset(
        new Anvil::GLSLShaderToSPIRVGenerator(in_opt_device_ptr,
                                              in_mode,
                                              in_data,
                                              in_shader_stage,
                                              in_spirv_version)
    );

    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::ANVIL_GLSL_SHADER_TO_SPIRV_GENERATOR,
                                                 result_ptr.get() );

    return result_ptr;
}

/* Please see header for specification */
std::string Anvil::GLSLShaderToSPIRVGenerator::get_extension_behavior_glsl_code(const ExtensionBehavior& in_value) const
{
    std::string result;

    switch (in_value)
    {
        case EXTENSION_BEHAVIOR_DISABLE: result = "disable"; break;
        case EXTENSION_BEHAVIOR_ENABLE:  result = "enable";  break;
        case EXTENSION_BEHAVIOR_REQUIRE: result = "require"; break;
        case EXTENSION_BEHAVIOR_WARN:    result = "warn";    break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}
