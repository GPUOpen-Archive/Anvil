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

/* Uncomment the #define below to enable off-screen rendering */
// #define ENABLE_OFFSCREEN_RENDERING

/* Uncomment the #define below to enable validation */
// #define ENABLE_VALIDATION


#include <string>
#include <cmath>
#include "config.h"
#include "misc/buffer_create_info.h"
#include "misc/framebuffer_create_info.h"
#include "misc/glsl_to_spirv.h"
#include "misc/graphics_pipeline_create_info.h"
#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/instance_create_info.h"
#include "misc/io.h"
#include "misc/memory_allocator.h"
#include "misc/object_tracker.h"
#include "misc/render_pass_create_info.h"
#include "misc/rendering_surface_create_info.h"
#include "misc/semaphore_create_info.h"
#include "misc/swapchain_create_info.h"
#include "misc/time.h"
#include "misc/window_factory.h"
#include "wrappers/buffer.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/device.h"
#include "wrappers/event.h"
#include "wrappers/graphics_pipeline_manager.h"
#include "wrappers/framebuffer.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"
#include "wrappers/query_pool.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/render_pass.h"
#include "wrappers/semaphore.h"
#include "wrappers/shader_module.h"
#include "wrappers/swapchain.h"
#include "app.h"


/* Sanity checks */
#if defined(_WIN32)
    #if !defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT) && !defined(ENABLE_OFFSCREEN_RENDERING)
        #error Anvil has not been built with Win32/64 window system support. The application can only be built in offscreen rendering mode.
    #endif
#else
    #if !defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT) && !defined(ENABLE_OFFSCREEN_RENDERING)
        #error Anvil has not been built with XCB window system support. The application can only be built in offscreen rendering mode.
    #endif
#endif


/* Low-level #defines follow.. */

/* When offscreen rendering is enabled, N_FRAMES_TO_RENDER tells how many frames should be
 * rendered before leaving */
#define N_FRAMES_TO_RENDER (8)

#define APP_NAME      "Push constants example app"
#define N_TRIANGLES   (16)
#define WINDOW_WIDTH  (1280)
#define WINDOW_HEIGHT (720)


static const char* g_glsl_frag =
    "#version 430\n"
    "\n"
    "layout (location = 0)      in  vec3 color;\n"
    "layout (location = 1) flat in  int  instance_id;\n"
    "layout (location = 0)      out vec4 result;\n"
    "\n"
    "layout (push_constant) uniform PCLuminance\n"
    "{\n"
    "    vec4 value0;\n"
    "    vec4 value1;\n"
    "    vec4 value2;\n"
    "    vec4 value3;\n"
    "} pcLuminance;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    int  index = instance_id / 4;\n"
    "    vec4 luminance;\n"
    "\n"
    "    result = vec4(color.xyz, 1.0);\n"
    "\n"
    "    if (index == 0)\n"
    "        luminance = pcLuminance.value0;\n"
    "    else if (index == 1)\n"
    "        luminance = pcLuminance.value1;\n"
    "    else if (index == 2)\n"
    "        luminance = pcLuminance.value2;\n"
    "    else if (index == 3)\n"
    "        luminance = pcLuminance.value3;\n"
    "\n"
    "    result.w = luminance[instance_id % 4];\n"
    "}\n";

static const char* g_glsl_vert =
    "#version 430\n"
    "\n"
    "layout (location = 0) in vec4 vertexData;\n"
    "layout (location = 1) in vec3 colorData;\n"
    "\n"
    "layout (location = 0)      out vec3 result_color;\n"
    "layout (location = 1) flat out int  result_instance_id;\n"
    "\n"
    "layout (std140, binding = 0) uniform dataUB\n"
    "{\n"
    "    ivec4 frame_index;\n"
    "    vec4  position_rotation[N_TRIANGLES]; /* XY position, XY rotation */\n"
    "    vec4  size             [N_TRIANGLES / 4];\n"
    "};\n"
    "\n"
    "layout (push_constant) uniform PCLuminance\n"
    "{\n"
    "    vec4 value0;\n"
    "    vec4 value1;\n"
    "    vec4 value2;\n"
    "    vec4 value3;\n"
    "} pcLuminance;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    int  index = gl_InstanceIndex / 4;\n"
    "    vec4 luminance;\n"
    "\n"
    "    if (index == 0)\n"
    "        luminance = pcLuminance.value0;\n"
    "    else if (index == 1)\n"
    "        luminance = pcLuminance.value1;\n"
    "    else if (index == 2)\n"
    "        luminance = pcLuminance.value2;\n"
    "    else if (index == 3)\n"
    "        luminance = pcLuminance.value3;\n"
    "\n"
    "    result_color        = colorData + vec3(0.0, 0.0, 1.0 - luminance[gl_InstanceIndex % 4]);\n"
    "    result_instance_id  = gl_InstanceIndex;\n"
    "\n"
    "\n"
    "    vec4 result_position = vec4(vertexData.xy, 0.0, 1.0);\n"
    "    vec2 cos_factor      = cos(position_rotation[gl_InstanceIndex].zw);\n"
    "    vec2 sin_factor      = sin(position_rotation[gl_InstanceIndex].zw);\n"
    "\n"
    "    result_position.xy   = vec2(dot(vertexData.xy, vec2(cos_factor.x, -sin_factor.y) ),\n"
    "                                dot(vertexData.xy, vec2(sin_factor.x,  cos_factor.y) ));\n"
    "\n"
    "    switch (gl_InstanceIndex % 4)\n"
    "    {\n"
    "        case 0: result_position.xy *= vec2(size[index].x); break;\n"
    "        case 1: result_position.xy *= vec2(size[index].y); break;\n"
    "        case 2: result_position.xy *= vec2(size[index].z); break;\n"
    "        case 3: result_position.xy *= vec2(size[index].w); break;\n"
    "    }\n"
    "\n"
    "    result_position.xy += position_rotation[gl_InstanceIndex].xy;\n"
    "    gl_Position         = result_position;\n"
    "}\n";

const float g_mesh_data[] =
{
   -1.0f,  1.0f,  0.0f, 1.0f,   /* position */
    0.75f, 0.25f, 0.1f,         /* color    */

   -1.0f, -1.0f,  0.0f, 1.0f,   /* position */
    0.25f, 0.75f, 0.2f,         /* color    */

    1.0f, -1.0f,  0.0f, 1.0f,   /* position */
    0.1f,  0.3f,  0.5f,         /* color    */
};

static const uint32_t g_mesh_data_color_start_offset    = sizeof(float) * 4;
static const uint32_t g_mesh_data_color_stride          = sizeof(float) * 7;
static const uint32_t g_mesh_data_n_vertices            = 3;
static const uint32_t g_mesh_data_position_start_offset = 0;
static const uint32_t g_mesh_data_position_stride       = sizeof(float) * 7;


App::App()
    :m_n_last_semaphore_used           (0),
     m_n_swapchain_images              (N_SWAPCHAIN_IMAGES),
     m_ub_data_size_per_swapchain_image(0)
{
    // ..
}

App::~App()
{
    deinit();
}

void App::deinit()
{
    auto gfx_pipeline_manager_ptr = m_device_ptr->get_graphics_pipeline_manager();

    Anvil::Vulkan::vkDeviceWaitIdle(m_device_ptr->get_device_vk() );

    if (m_pipeline_id != UINT32_MAX)
    {
        gfx_pipeline_manager_ptr->delete_pipeline(m_pipeline_id);

        m_pipeline_id = UINT32_MAX;
    }

    m_frame_signal_semaphores.clear();
    m_frame_wait_semaphores.clear();

    m_rendering_surface_ptr.reset();
    m_swapchain_ptr.reset();

    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < N_SWAPCHAIN_IMAGES;
                ++n_swapchain_image)
    {
        m_command_buffers[n_swapchain_image].reset();
        m_fbos[n_swapchain_image].reset();
    }

    m_data_buffer_ptr.reset();
    m_dsg_ptr.reset();
    m_fs_ptr.reset();
    m_mesh_data_buffer_ptr.reset();
    m_renderpass_ptr.reset();
    m_vs_ptr.reset();

    m_device_ptr.reset();
    m_instance_ptr.reset();

    m_window_ptr.reset();
}

void App::draw_frame()
{
    Anvil::Semaphore*               curr_frame_signal_semaphore_ptr = nullptr;
    Anvil::Semaphore*               curr_frame_wait_semaphore_ptr   = nullptr;
    static uint32_t                 n_frames_rendered               = 0;
    uint32_t                        n_swapchain_image;
    Anvil::Queue*                   present_queue_ptr               = m_device_ptr->get_universal_queue(0);
    Anvil::Semaphore*               present_wait_semaphore_ptr      = nullptr;
    const Anvil::PipelineStageFlags wait_stage_mask                 = Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT;

    /* Determine the signal + wait semaphores to use for drawing this frame */
    m_n_last_semaphore_used = (m_n_last_semaphore_used + 1) % m_n_swapchain_images;

    curr_frame_signal_semaphore_ptr = m_frame_signal_semaphores[m_n_last_semaphore_used].get();
    curr_frame_wait_semaphore_ptr   = m_frame_wait_semaphores  [m_n_last_semaphore_used].get();

    present_wait_semaphore_ptr = curr_frame_signal_semaphore_ptr;

    /* Determine the semaphore which the swapchain image */
    {
        const auto acquire_result = m_swapchain_ptr->acquire_image(curr_frame_wait_semaphore_ptr,
                                                                  &n_swapchain_image,
                                                                   true); /* in_should_block */

        ANVIL_REDUNDANT_VARIABLE_CONST(acquire_result);
        anvil_assert                  (acquire_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }

    /* Submit work chunk and present */
    update_data_ub_contents(n_swapchain_image);

    present_queue_ptr->submit(
        Anvil::SubmitInfo::create(m_command_buffers[n_swapchain_image].get(),
                                  1, /* n_semaphores_to_signal */
                                 &curr_frame_signal_semaphore_ptr,
                                  1, /* n_semaphores_to_wait_on */
                                 &curr_frame_wait_semaphore_ptr,
                                 &wait_stage_mask,
                                  false /* should_block */)
    );

    {
        Anvil::SwapchainOperationErrorCode present_result = Anvil::SwapchainOperationErrorCode::DEVICE_LOST;

        present_queue_ptr->present(m_swapchain_ptr.get(),
                                   n_swapchain_image,
                                   1, /* n_wait_semaphores */
                                  &present_wait_semaphore_ptr,
                                  &present_result);

        ANVIL_REDUNDANT_VARIABLE(present_result);
        anvil_assert            (present_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }

    ++n_frames_rendered;

    #if defined(ENABLE_OFFSCREEN_RENDERING)
    {
        if (n_frames_rendered >= N_FRAMES_TO_RENDER)
        {
            m_window_ptr->close();
        }
    }
    #endif
}

void App::get_luminance_data(std::unique_ptr<float[]>* out_result_ptr,
                             uint32_t*                 out_result_size_ptr) const
{
    std::unique_ptr<float[]> luminance_data_ptr;
    float*                   luminance_data_raw_ptr;
    uint32_t                 luminance_data_size;

    static_assert(N_TRIANGLES == 16,
                  "Shader and the app logic assumes N_TRIANGLES will always be 16");

    luminance_data_size = sizeof(float) * N_TRIANGLES;

    luminance_data_ptr.reset(new float[luminance_data_size / sizeof(float)] );

    luminance_data_raw_ptr = luminance_data_ptr.get();

    for (uint32_t n_tri = 0;
                  n_tri < N_TRIANGLES;
                ++n_tri)
    {
        luminance_data_raw_ptr[n_tri] = float(n_tri) / float(N_TRIANGLES - 1);
    }

    *out_result_ptr      = std::move(luminance_data_ptr);
    *out_result_size_ptr = luminance_data_size;
}

const unsigned char* App::get_mesh_data() const
{
    return reinterpret_cast<const unsigned char*>(g_mesh_data);
}

Anvil::Format App::get_mesh_data_color_format() const
{
    return Anvil::Format::R32G32B32_SFLOAT;
}

uint32_t App::get_mesh_data_color_start_offset() const
{
    return g_mesh_data_color_start_offset;
}

uint32_t App::get_mesh_data_color_stride() const
{
    return g_mesh_data_color_stride;
}

Anvil::Format App::get_mesh_data_position_format() const
{
    return Anvil::Format::R32G32B32A32_SFLOAT;
}

uint32_t App::get_mesh_data_position_start_offset() const
{
    return g_mesh_data_position_start_offset;
}

uint32_t App::get_mesh_data_position_stride() const
{
    return g_mesh_data_position_stride;
}

uint32_t App::get_mesh_data_size() const
{
    return sizeof(g_mesh_data);
}

uint32_t App::get_mesh_n_vertices() const
{
    return g_mesh_data_n_vertices;
}

void App::init()
{
    init_vulkan   ();
    init_window   ();
    init_swapchain();

    init_buffers     ();
    init_dsgs        ();
    init_framebuffers();
    init_semaphores  ();
    init_shaders     ();

    init_gfx_pipelines  ();
    init_command_buffers();
}

void App::init_buffers()
{
    const unsigned char* mesh_data                        = get_mesh_data();
    const uint32_t       mesh_data_size                   = get_mesh_data_size();
    const VkDeviceSize   ub_data_size_per_swapchain_image = sizeof(int)                 * 4 + /* frame index + padding             */
                                                            sizeof(float) * N_TRIANGLES * 4 + /* position (vec2) + rotation (vec2) */
                                                            sizeof(float) * N_TRIANGLES +     /* luminance                         */
                                                            sizeof(float) * N_TRIANGLES;      /* size                              */
    const auto           ub_data_alignment_requirement    = m_device_ptr->get_physical_device_properties().core_vk1_0_properties_ptr->limits.min_uniform_buffer_offset_alignment;
    const auto           ub_data_size_total               = N_SWAPCHAIN_IMAGES * (Anvil::Utils::round_up(ub_data_size_per_swapchain_image,
                                                                                                         ub_data_alignment_requirement) );

    m_ub_data_size_per_swapchain_image = ub_data_size_total / N_SWAPCHAIN_IMAGES;

    /* Use a memory allocator to re-use memory blocks wherever possible */
    auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(m_device_ptr.get() );

    /* Set up a buffer to hold uniform data */
    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        ub_data_size_total,
                                                                        Anvil::QueueFamilyFlagBits::COMPUTE_BIT | Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT);

        m_data_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    m_data_buffer_ptr->set_name("Data buffer");

    allocator_ptr->add_buffer(m_data_buffer_ptr.get(),
                              Anvil::MemoryFeatureFlagBits::NONE); /* in_required_memory_features */

    /* Set up a buffer to hold mesh data */
    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        get_mesh_data_size(),
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT);

        m_mesh_data_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    m_mesh_data_buffer_ptr->set_name("Mesh vertex data buffer");

    allocator_ptr->add_buffer(m_mesh_data_buffer_ptr.get(),
                              Anvil::MemoryFeatureFlagBits::NONE); /* in_required_memory_features */

    /* Allocate memory blocks and copy data where applicable */
    m_mesh_data_buffer_ptr->write(0, /* start_offset */
                                  mesh_data_size,
                                  mesh_data);
}

void App::init_command_buffers()
{
    auto                          gfx_pipeline_manager_ptr(m_device_ptr->get_graphics_pipeline_manager() );
    Anvil::ImageSubresourceRange  image_subresource_range;
    std::unique_ptr<float[]>      luminance_data_ptr;
    uint32_t                      luminance_data_size;
    Anvil::Queue*                 universal_queue_ptr     (m_device_ptr->get_universal_queue(0) );

    image_subresource_range.aspect_mask      = Anvil::ImageAspectFlagBits::COLOR_BIT;
    image_subresource_range.base_array_layer = 0;
    image_subresource_range.base_mip_level   = 0;
    image_subresource_range.layer_count      = 1;
    image_subresource_range.level_count      = 1;

    get_luminance_data(&luminance_data_ptr,
                       &luminance_data_size);

    for (uint32_t n_command_buffer = 0;
                  n_command_buffer < N_SWAPCHAIN_IMAGES;
                ++n_command_buffer)
    {
        Anvil::PrimaryCommandBufferUniquePtr cmd_buffer_ptr;

        cmd_buffer_ptr = m_device_ptr->get_command_pool_for_queue_family_index(m_device_ptr->get_universal_queue(0)->get_queue_family_index() )->alloc_primary_level_command_buffer();

        /* Start recording commands */
        cmd_buffer_ptr->start_recording(false, /* one_time_submit          */
                                        true); /* simultaneous_use_allowed */

        /* Switch the swap-chain image to the color_attachment_optimal image layout */
        {
            Anvil::ImageBarrier image_barrier(Anvil::AccessFlagBits::NONE,                       /* source_access_mask       */
                                              Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT, /* destination_access_mask  */
                                              Anvil::ImageLayout::UNDEFINED,                  /* old_image_layout */
                                              Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,   /* new_image_layout */
                                              universal_queue_ptr->get_queue_family_index(),
                                              universal_queue_ptr->get_queue_family_index(),
                                              m_swapchain_ptr->get_image(n_command_buffer),
                                              image_subresource_range);

            cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::TOP_OF_PIPE_BIT,            /* src_stage_mask                 */
                                                    Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,/* dst_stage_mask                 */
                                                    Anvil::DependencyFlagBits::NONE,
                                                    0,                                                        /* in_memory_barrier_count        */
                                                    nullptr,                                                  /* in_memory_barrier_ptrs         */
                                                    0,                                                        /* in_buffer_memory_barrier_count */
                                                    nullptr,                                                  /* in_buffer_memory_barrier_ptrs  */
                                                    1,                                                        /* in_image_memory_barrier_count  */
                                                   &image_barrier);
        }

        /* Make sure CPU-written data is flushed before we start rendering */
        Anvil::BufferBarrier buffer_barrier(Anvil::AccessFlagBits::HOST_WRITE_BIT,                 /* in_source_access_mask      */
                                            Anvil::AccessFlagBits::UNIFORM_READ_BIT,               /* in_destination_access_mask */
                                            universal_queue_ptr->get_queue_family_index(),         /* in_src_queue_family_index  */
                                            universal_queue_ptr->get_queue_family_index(),         /* in_dst_queue_family_index  */
                                            m_data_buffer_ptr.get(),
                                            m_ub_data_size_per_swapchain_image * n_command_buffer, /* in_offset                  */
                                            m_ub_data_size_per_swapchain_image);

        cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::HOST_BIT,
                                                Anvil::PipelineStageFlagBits::VERTEX_SHADER_BIT,
                                                Anvil::DependencyFlagBits::NONE,
                                                0,               /* in_memory_barrier_count        */
                                                nullptr,         /* in_memory_barriers_ptr         */
                                                1,               /* in_buffer_memory_barrier_count */
                                               &buffer_barrier,
                                                0,               /* in_image_memory_barrier_count  */
                                                nullptr);        /* in_image_memory_barriers_ptr   */

        /* 2. Render the geometry. */ 
        VkClearValue       attachment_clear_value;
        VkRect2D           render_area;
        VkShaderStageFlags shaderStageFlags = 0;

        attachment_clear_value.color.float32[0] = 1.0f;
        attachment_clear_value.color.float32[1] = 0.5f;
        attachment_clear_value.color.float32[2] = 0.2f;
        attachment_clear_value.color.float32[3] = 1.0f;

        render_area.extent.height = WINDOW_HEIGHT;
        render_area.extent.width  = WINDOW_WIDTH;
        render_area.offset.x      = 0;
        render_area.offset.y      = 0;

        cmd_buffer_ptr->record_begin_render_pass(1, /* in_n_clear_values */
                                                &attachment_clear_value,
                                                 m_fbos[n_command_buffer].get(),
                                                 render_area,
                                                 m_renderpass_ptr.get(),
                                                 Anvil::SubpassContents::INLINE);
        {
            const uint32_t     data_ub_offset           = static_cast<uint32_t>(m_ub_data_size_per_swapchain_image * n_command_buffer);
            auto               ds_ptr                   = m_dsg_ptr->get_descriptor_set                         (0 /* n_set */);
            const VkDeviceSize mesh_data_buffer_offset  = 0;
            auto               mesh_data_buffer_raw_ptr = m_mesh_data_buffer_ptr.get();
            auto               pipeline_layout_ptr      = gfx_pipeline_manager_ptr->get_pipeline_layout(m_pipeline_id);

            cmd_buffer_ptr->record_bind_pipeline(Anvil::PipelineBindPoint::GRAPHICS,
                                                 m_pipeline_id);

            cmd_buffer_ptr->record_push_constants(pipeline_layout_ptr,
                                                  Anvil::ShaderStageFlagBits::FRAGMENT_BIT | Anvil::ShaderStageFlagBits::VERTEX_BIT,
                                                  0, /* in_offset */
                                                  luminance_data_size,
                                                  luminance_data_ptr.get() );

            cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                        pipeline_layout_ptr,
                                                        0, /* firstSet */
                                                        1, /* setCount */
                                                       &ds_ptr,
                                                        1,               /* dynamicOffsetCount */
                                                       &data_ub_offset); /* pDynamicOffsets    */

            cmd_buffer_ptr->record_bind_vertex_buffers(0, /* startBinding */ 
                                                       1, /* bindingCount */
                                                      &mesh_data_buffer_raw_ptr,
                                                      &mesh_data_buffer_offset);

            cmd_buffer_ptr->record_draw(3,            /* in_vertex_count   */
                                        N_TRIANGLES,  /* in_instance_count */
                                        0,            /* in_first_vertex   */
                                        0);           /* in_first_instance */
        }
        cmd_buffer_ptr->record_end_render_pass();

        /* Close the recording process */
        cmd_buffer_ptr->stop_recording();

        m_command_buffers[n_command_buffer] = std::move(cmd_buffer_ptr);
    }
}

void App::init_dsgs()
{
    {
        auto dsg_create_info_ptrs = std::vector<Anvil::DescriptorSetCreateInfoUniquePtr>(1);

        dsg_create_info_ptrs[0] = Anvil::DescriptorSetCreateInfo::create();

        dsg_create_info_ptrs[0]->add_binding(0, /* n_binding */
                                             Anvil::DescriptorType::UNIFORM_BUFFER_DYNAMIC,
                                             1, /* n_elements */
                                             Anvil::ShaderStageFlagBits::VERTEX_BIT);

        m_dsg_ptr = Anvil::DescriptorSetGroup::create(m_device_ptr.get(),
                                                      dsg_create_info_ptrs,
                                                      false); /* releaseable_sets */
    }

    m_dsg_ptr->set_binding_item(0, /* n_set     */
                                0, /* n_binding */
                                Anvil::DescriptorSet::DynamicUniformBufferBindingElement(m_data_buffer_ptr.get(),
                                                                                         0, /* in_start_offset */
                                                                                         m_ub_data_size_per_swapchain_image) );
}

void App::init_events()
{
    /* Stub */
}

void App::init_framebuffers()
{
    bool result;

    /* We need to instantiate 1 framebuffer object per each used swap-chain image */
    for (uint32_t n_fbo = 0;
                  n_fbo < N_SWAPCHAIN_IMAGES;
                ++n_fbo)
    {
        Anvil::ImageView* attachment_image_view_ptr = nullptr;

        attachment_image_view_ptr = m_swapchain_ptr->get_image_view(n_fbo);

        /* Create the internal framebuffer object */
        {
            auto create_info_ptr = Anvil::FramebufferCreateInfo::create(m_device_ptr.get(),
                                                                        WINDOW_WIDTH,
                                                                        WINDOW_HEIGHT,
                                                                        1 /* n_layers */);

            result = create_info_ptr->add_attachment(attachment_image_view_ptr,
                                                     nullptr /* out_opt_attachment_id_ptr */);
            anvil_assert(result);

            m_fbos[n_fbo] = Anvil::Framebuffer::create(std::move(create_info_ptr) );
        }

        m_fbos[n_fbo]->set_name_formatted("Framebuffer for swapchain image [%d]",
                                          n_fbo);
    }
}

void App::init_gfx_pipelines()
{
    Anvil::GraphicsPipelineCreateInfoUniquePtr gfx_pipeline_create_info_ptr;
    auto                                       gfx_pipeline_manager_ptr(m_device_ptr->get_graphics_pipeline_manager() );

    /* Create a renderpass for the pipeline */
    Anvil::RenderPassAttachmentID render_pass_color_attachment_id;
    Anvil::SubPassID              render_pass_subpass_id;

    {
        Anvil::RenderPassCreateInfoUniquePtr render_pass_create_info_ptr(new Anvil::RenderPassCreateInfo(m_device_ptr.get() ) );

        render_pass_create_info_ptr->add_color_attachment(m_swapchain_ptr->get_create_info_ptr()->get_format(),
                                                          Anvil::SampleCountFlagBits::_1_BIT,
                                                          Anvil::AttachmentLoadOp::CLEAR,
                                                          Anvil::AttachmentStoreOp::STORE,
                                                          Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
#ifdef ENABLE_OFFSCREEN_RENDERING
                                                          Anvil::ImageLayout::GENERAL,
#else
                                                          Anvil::ImageLayout::PRESENT_SRC_KHR,
#endif
                                                          false, /* may_alias */
                                                         &render_pass_color_attachment_id);


        render_pass_create_info_ptr->add_subpass                 (&render_pass_subpass_id);
        render_pass_create_info_ptr->add_subpass_color_attachment(render_pass_subpass_id,
                                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                  render_pass_color_attachment_id,
                                                                  0,        /* location                      */
                                                                  nullptr); /* opt_attachment_resolve_id_ptr */


        m_renderpass_ptr = Anvil::RenderPass::create(std::move(render_pass_create_info_ptr),
                                                     m_swapchain_ptr.get() );
    }

    m_renderpass_ptr->set_name("Main renderpass");

    gfx_pipeline_create_info_ptr = Anvil::GraphicsPipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                             m_renderpass_ptr.get(),
                                                                             render_pass_subpass_id,
                                                                             *m_fs_ptr,
                                                                             Anvil::ShaderModuleStageEntryPoint(), /* in_geometry_shader        */
                                                                             Anvil::ShaderModuleStageEntryPoint(), /* in_tess_control_shader    */
                                                                             Anvil::ShaderModuleStageEntryPoint(), /* in_tess_evaluation_shader */
                                                                             *m_vs_ptr);



    gfx_pipeline_create_info_ptr->set_descriptor_set_create_info       (m_dsg_ptr->get_descriptor_set_create_info() );
    gfx_pipeline_create_info_ptr->attach_push_constant_range           (0, /* in_offset */
                                                                        sizeof(float) * 4 /* vec4 */ * 4 /* vec4 values */,
                                                                        Anvil::ShaderStageFlagBits::FRAGMENT_BIT | Anvil::ShaderStageFlagBits::VERTEX_BIT);
    gfx_pipeline_create_info_ptr->set_rasterization_properties         (Anvil::PolygonMode::FILL,
                                                                        Anvil::CullModeFlagBits::NONE,
                                                                        Anvil::FrontFace::COUNTER_CLOCKWISE,
                                                                        1.0f); /* in_line_width       */
    gfx_pipeline_create_info_ptr->set_color_blend_attachment_properties(0,     /* in_attachment_id    */
                                                                        true,  /* in_blending_enabled */
                                                                        Anvil::BlendOp::ADD,
                                                                        Anvil::BlendOp::ADD,
                                                                        Anvil::BlendFactor::SRC_ALPHA,
                                                                        Anvil::BlendFactor::ONE_MINUS_SRC_ALPHA,
                                                                        Anvil::BlendFactor::SRC_ALPHA,
                                                                        Anvil::BlendFactor::ONE_MINUS_SRC_ALPHA,
                                                                        Anvil::ColorComponentFlagBits::A_BIT | Anvil::ColorComponentFlagBits::B_BIT | Anvil::ColorComponentFlagBits::G_BIT | Anvil::ColorComponentFlagBits::R_BIT);

    gfx_pipeline_create_info_ptr->add_vertex_attribute(0, /* in_location */
                                                       get_mesh_data_position_format(),
                                                       get_mesh_data_position_start_offset(),
                                                       get_mesh_data_position_stride(),
                                                       Anvil::VertexInputRate::VERTEX);
    gfx_pipeline_create_info_ptr->add_vertex_attribute(1, /* location */
                                                       get_mesh_data_color_format      (),
                                                       get_mesh_data_color_start_offset(),
                                                       get_mesh_data_color_stride      (),
                                                       Anvil::VertexInputRate::VERTEX);


    gfx_pipeline_manager_ptr->add_pipeline(std::move(gfx_pipeline_create_info_ptr),
                                          &m_pipeline_id);
}

void App::init_images()
{
    /* Stub */
}

void App::init_semaphores()
{
    for (uint32_t n_semaphore = 0;
                  n_semaphore < m_n_swapchain_images;
                ++n_semaphore)
    {
        Anvil::SemaphoreUniquePtr new_signal_semaphore_ptr;
        Anvil::SemaphoreUniquePtr new_wait_semaphore_ptr;

        {
            auto create_info_ptr = Anvil::SemaphoreCreateInfo::create(m_device_ptr.get() );

            new_signal_semaphore_ptr = Anvil::Semaphore::create(std::move(create_info_ptr) );
        }

        {
            auto create_info_ptr = Anvil::SemaphoreCreateInfo::create(m_device_ptr.get() );

            new_wait_semaphore_ptr = Anvil::Semaphore::create(std::move(create_info_ptr) );
        }

        new_signal_semaphore_ptr->set_name_formatted("Signal semaphore [%d]",
                                                     n_semaphore);
        new_wait_semaphore_ptr->set_name_formatted  ("Wait semaphore [%d]",
                                                     n_semaphore);

        m_frame_signal_semaphores.push_back(std::move(new_signal_semaphore_ptr) );
        m_frame_wait_semaphores.push_back  (std::move(new_wait_semaphore_ptr) );
    }
}

void App::init_shaders()
{
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr fragment_shader_ptr;
    Anvil::ShaderModuleUniquePtr               fragment_shader_module_ptr;
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr vertex_shader_ptr;
    Anvil::ShaderModuleUniquePtr               vertex_shader_module_ptr;

    fragment_shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    g_glsl_frag,
                                                                    Anvil::ShaderStage::FRAGMENT);
    vertex_shader_ptr   = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    g_glsl_vert,
                                                                    Anvil::ShaderStage::VERTEX);

    fragment_shader_ptr->add_definition_value_pair("N_TRIANGLES",
                                                   N_TRIANGLES);
    vertex_shader_ptr->add_definition_value_pair  ("N_TRIANGLES",
                                                   N_TRIANGLES);


    fragment_shader_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get       (),
                                                                                  fragment_shader_ptr.get() );
    vertex_shader_module_ptr   = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get       (),
                                                                                  vertex_shader_ptr.get  () );

    fragment_shader_module_ptr->set_name("Fragment shader module");
    vertex_shader_module_ptr->set_name  ("Vertex shader module");

    m_fs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint("main",
                                               std::move(fragment_shader_module_ptr),
                                               Anvil::ShaderStage::FRAGMENT)
    );
    m_vs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint("main",
                                               std::move(vertex_shader_module_ptr),
                                               Anvil::ShaderStage::VERTEX)
    );
}

void App::init_swapchain()
{
    Anvil::SGPUDevice* device_ptr(reinterpret_cast<Anvil::SGPUDevice*>(m_device_ptr.get() ));

    {
        auto create_info_ptr = Anvil::RenderingSurfaceCreateInfo::create(m_instance_ptr.get(),
                                                                         m_device_ptr.get  (),
                                                                         m_window_ptr.get  () );

        m_rendering_surface_ptr = Anvil::RenderingSurface::create(std::move(create_info_ptr) );
    }

    m_rendering_surface_ptr->set_name("Main rendering surface");


    m_swapchain_ptr = device_ptr->create_swapchain(m_rendering_surface_ptr.get(),
                                                   m_window_ptr.get           (),
                                                   Anvil::Format::B8G8R8A8_UNORM,
                                                   Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,
                                                   Anvil::PresentModeKHR::FIFO_KHR,
                                                   Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
                                                   m_n_swapchain_images);

    m_swapchain_ptr->set_name("Main swapchain");

    /* Cache the queue we are going to use for presentation */
    const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

    if (!m_rendering_surface_ptr->get_queue_families_with_present_support(device_ptr->get_physical_device(),
                                                                         &present_queue_fams_ptr) )
    {
        anvil_assert_fail();
    }

    m_present_queue_ptr = device_ptr->get_queue_for_queue_family_index(present_queue_fams_ptr->at(0),
                                                                       0); /* in_n_queue */
}

void App::init_window()
{
    #ifdef ENABLE_OFFSCREEN_RENDERING
        const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS;
    #else
        #ifdef _WIN32
            const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_SYSTEM;
        #else
            const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_XCB;
        #endif
    #endif

    /* Create a window */
    m_window_ptr = Anvil::WindowFactory::create_window(platform,
                                                       APP_NAME,
                                                       1280,
                                                       720,
                                                       true, /* in_closable */
                                                       std::bind(&App::draw_frame,
                                                                 this)
    );
}

void App::init_vulkan()
{
    /* Create a Vulkan instance */
    {
        auto create_info_ptr = Anvil::InstanceCreateInfo::create(APP_NAME,  /* in_app_name    */
                                                                 APP_NAME,  /* in_engine_name */
#ifdef ENABLE_VALIDATION
                                                                 std::bind(&App::on_validation_callback,
                                                                           this,
                                                                           std::placeholders::_1,
                                                                           std::placeholders::_2),
#else
                                                                 Anvil::DebugCallbackFunction(),
#endif
                                                                 false); /* in_mt_safe */

        m_instance_ptr = Anvil::Instance::create(std::move(create_info_ptr) );
    }

    m_physical_device_ptr = m_instance_ptr->get_physical_device(0);

    /* Create a Vulkan device */
    {
        auto create_info_ptr = Anvil::DeviceCreateInfo::create_sgpu(m_physical_device_ptr,
                                                                    true,                       /* in_enable_shader_module_cache */
                                                                    Anvil::DeviceExtensionConfiguration(),
                                                                    std::vector<std::string>(), /* in_layers */
                                                                    Anvil::CommandPoolCreateFlagBits::NONE,
                                                                    false);                     /* in_mt_safe */

        m_device_ptr = Anvil::SGPUDevice::create(std::move(create_info_ptr) );
    }
}

void App::on_validation_callback(Anvil::DebugMessageSeverityFlags in_severity,
                                 const char*                      in_message_ptr)
{
    if ((in_severity & Anvil::DebugMessageSeverityFlagBits::ERROR_BIT) != 0)
    {
        fprintf(stderr,
                "[!] %s\n",
                in_message_ptr);
    }
}

void App::run()
{
    m_window_ptr->run();
}

/** Updates the buffer memory, which holds position, rotation and size data for all triangles. */
void App::update_data_ub_contents(uint32_t in_n_swapchain_image)
{
    struct
    {
        int   frame_index      [4];      /* frame index + padding (ivec3) */
        float position_rotation[16 * 4]; /* pos (vec2) + rot (vec2)       */
        float size             [16];
    } data;

    char*           mapped_data_ptr   = nullptr;
    static uint32_t n_frames_rendered = 0;
    const float     scale_factor      = 1.35f;

    data.frame_index[0] = static_cast<int>(m_time.get_time_in_msec() / 2); /* slow down a little */

    for (unsigned int n_triangle = 0;
                      n_triangle < N_TRIANGLES;
                    ++n_triangle)
    {
        float x = cos(3.14152965f * 2.0f * float(n_triangle) / float(N_TRIANGLES) ) * 0.5f * scale_factor;
        float y = sin(3.14152965f * 2.0f * float(n_triangle) / float(N_TRIANGLES) ) * 0.5f * scale_factor;

        data.position_rotation[n_triangle * 4 + 0] = x;
        data.position_rotation[n_triangle * 4 + 1] = y;
        data.position_rotation[n_triangle * 4 + 2] = float(data.frame_index[0]) / 360.0f + 3.14152965f * 2.0f * float(n_triangle) / float(N_TRIANGLES);
        data.position_rotation[n_triangle * 4 + 3] = float(data.frame_index[0]) / 360.0f + 3.14152965f * 2.0f * float(n_triangle) / float(N_TRIANGLES);
        data.size             [n_triangle]         = 0.2f;
    }

    m_data_buffer_ptr->write(in_n_swapchain_image * m_ub_data_size_per_swapchain_image, /* start_offset */
                             sizeof(data),
                            &data,
                             m_device_ptr->get_universal_queue(0) );

    ++n_frames_rendered;
}


int main(int argc, char *argv[])
{
    std::unique_ptr<App> app_ptr(new App() );

    app_ptr->init();
    app_ptr->run();

    #ifdef _DEBUG
    {
        app_ptr.reset();

        Anvil::ObjectTracker::get()->check_for_leaks();
    }
    #endif

    return 0;
}

