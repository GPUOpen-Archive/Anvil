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
#include "misc/instance_create_info.h"
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


/* Low-level #defines follow */
#define APP_NAME                "MultiViewport example application"
#define N_SUBDIVISION_TRIANGLES (128)
#define N_VIEWPORTS             (4)
#define WINDOW_WIDTH            (1280)
#define WINDOW_HEIGHT           (720)

/* When offscreen rendering is enabled, N_FRAMES_TO_RENDER tells how many frames should be
 * rendered before leaving */
#define N_FRAMES_TO_RENDER (1)

static const uint32_t g_color1_attribute_binding  = 4;
static const uint32_t g_color2_attribute_binding  = 3;
static const uint32_t g_color3_attribute_binding  = 2;
static const uint32_t g_color4_attribute_binding  = 1;
static const uint32_t g_vertex_attribute_binding  = 0;
static const uint32_t g_n_attribute_bindings      = 5;

static const uint32_t g_color1_attribute_location = 0;
static const uint32_t g_color2_attribute_location = 1;
static const uint32_t g_color3_attribute_location = 2;
static const uint32_t g_color4_attribute_location = 3;
static const uint32_t g_vertex_attribute_location = 4;

static const char* g_glsl_render_frag =
    "#version 430\n"
    "\n"
    "layout(location = 0) in  vec3 fs_color;\n"
    "layout(location = 0) out vec4 result;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    result = vec4(fs_color, 1.0);\n"
    "}\n";

static const char* g_glsl_render_geom =
    "#version 430\n"
    "\n"
    "layout(invocations = 1, triangles) in;\n"
    "\n"
    "layout(triangle_strip, max_vertices = 12) out;\n"
    "\n"
    "layout(location = 0) in  vec3 in_color1[];\n"
    "layout(location = 1) in  vec3 in_color2[];\n"
    "layout(location = 2) in  vec3 in_color3[];\n"
    "layout(location = 3) in  vec3 in_color4[];\n"
    "layout(location = 0) out vec3 fs_color;\n"
    "\n"
    "\n"
    "void main()\n"
    "{\n"
    "    for (int viewport_index = 0;\n"
    "             viewport_index < 4;\n"
    "           ++viewport_index)\n"
    "    {\n"
    "        gl_ViewportIndex = viewport_index;\n"
    "\n"
    "        for (int n_result_vertex = 0;\n"
    "                 n_result_vertex < 3;\n"
    "               ++n_result_vertex)\n"
    "        {\n"
    "            switch (viewport_index)\n"
    "            {\n"
    "                case 0:  fs_color = in_color1[n_result_vertex]; break;\n"
    "                case 1:  fs_color = in_color2[n_result_vertex]; break;\n"
    "                case 2:  fs_color = in_color3[n_result_vertex]; break;\n"
    "                case 3:  fs_color = in_color4[n_result_vertex]; break;\n"
    "\n"
    "                default: fs_color = vec3(1.0, 0.0, 0.0); break;\n"
    "            }\n"
    "\n"
    "            gl_Position  = gl_in[n_result_vertex].gl_Position;\n"
    "\n"
    "            EmitVertex();\n"
    "        }\n"
    "\n"
    "        EndPrimitive();\n"
    "    }\n"
    "}\n";
static const char* g_glsl_render_vert =
    "#version 450\n"
    "\n"
    "layout(location = 0) in vec3 in_color1;\n"
    "layout(location = 1) in vec3 in_color2;\n"
    "layout(location = 2) in vec3 in_color3;\n"
    "layout(location = 3) in vec3 in_color4;\n"
    "layout(location = 4) in vec2 in_vertex;\n"
    "\n"
    "layout(location = 0) out  vec3 out_color1;\n"
    "layout(location = 1) out  vec3 out_color2;\n"
    "layout(location = 2) out  vec3 out_color3;\n"
    "layout(location = 3) out  vec3 out_color4;\n"
    "\n"
    "void main()\n"
    "{\n"
    "     out_color1 = in_color1.xyz;\n"
    "     out_color2 = in_color2.xyz;\n"
    "     out_color3 = in_color3.xyz;\n"
    "     out_color4 = in_color4.xyz;\n"
    "\n"
    "    gl_Position = vec4(in_vertex, 0.0, 1.0);\n"
    "}\n";


App::App()
    :m_n_last_semaphore_used(0),
     m_n_swapchain_images   (N_SWAPCHAIN_IMAGES),
     m_physical_device_ptr  (nullptr),
     m_present_queue_ptr    (nullptr)
{
}

App::~App()
{
    deinit();
}

void App::deinit()
{
    Anvil::Vulkan::vkDeviceWaitIdle(m_device_ptr->get_device_vk() );

    m_frame_signal_semaphores.clear();
    m_frame_wait_semaphores.clear();

    m_rendering_surface_ptr.reset();
    m_swapchain_ptr.reset();

    for (uint32_t n_command_buffer = 0;
                  n_command_buffer < sizeof(m_command_buffers) / sizeof(m_command_buffers[0]);
                ++n_command_buffer)
    {
        m_command_buffers[n_command_buffer].reset();
    }

    for (uint32_t n_fbo = 0;
                  n_fbo < sizeof(m_fbos) / sizeof(m_fbos[0]);
                ++n_fbo)
    {
        m_fbos[n_fbo].reset();
    }

    m_fs_ptr.reset();
    m_gs_ptr.reset();
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
    auto                            present_queue_ptr               = m_device_ptr->get_universal_queue(0);
    const Anvil::PipelineStageFlags wait_stage_mask                 = Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT;

    /* Determine the signal + wait semaphores to use for drawing this frame */
    m_n_last_semaphore_used = (m_n_last_semaphore_used + 1) % m_n_swapchain_images;

    curr_frame_signal_semaphore_ptr = m_frame_signal_semaphores[m_n_last_semaphore_used].get();
    curr_frame_wait_semaphore_ptr   = m_frame_wait_semaphores  [m_n_last_semaphore_used].get();

    /* Determine the semaphore which the swapchain image */
    {
        const auto acquire_result = m_swapchain_ptr->acquire_image(curr_frame_wait_semaphore_ptr,
                                                                  &n_swapchain_image);

        ANVIL_REDUNDANT_VARIABLE_CONST(acquire_result);
        anvil_assert                  (acquire_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }

    /* Submit work chunk and present */
    m_device_ptr->get_universal_queue(0)->submit(
        Anvil::SubmitInfo::create_wait_execute_signal(m_command_buffers[n_swapchain_image].get(),
                                                      1, /* n_semaphores_to_signal */
                                                     &curr_frame_signal_semaphore_ptr,
                                                      1, /* n_semaphores_to_wait_on */
                                                     &curr_frame_wait_semaphore_ptr,
                                                     &wait_stage_mask,
                                                      false)    /* should_block  */
    );

    {
        Anvil::SwapchainOperationErrorCode present_result = Anvil::SwapchainOperationErrorCode::DEVICE_LOST;

        present_queue_ptr->present(m_swapchain_ptr.get(),
                                   n_swapchain_image,
                                   1, /* n_wait_semaphores */
                                  &curr_frame_signal_semaphore_ptr,
                                  &present_result);

        ANVIL_REDUNDANT_VARIABLE(present_result);
        anvil_assert            (present_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }

    #ifdef ENABLE_OFFSCREEN_RENDERING
    {
        if (n_frames_rendered < N_FRAMES_TO_RENDER)
        {
            ++n_frames_rendered;
        }
        else
        {
            m_window_ptr->close();
        }
    }
    #endif
}

Anvil::Format App::get_mesh_color_data_format() const
{
    return Anvil::Format::R32G32B32_SFLOAT;
}

uint32_t App::get_mesh_color_data_n_components() const
{
    return 3;
}

uint32_t App::get_mesh_color_data_start_offset(uint32_t n_stream,
                                               uint32_t n_vertex) const
{
    const uint32_t color_data_per_vertex_size = get_mesh_color_data_n_components() * sizeof(float);
    const uint32_t color_data_stream_size     = color_data_per_vertex_size * get_mesh_n_vertices();

    uint32_t result = (n_stream != 0) ? color_data_stream_size * n_stream 
                                      : 0;

    result += n_vertex * color_data_per_vertex_size;

    return result;
}

std::unique_ptr<uint8_t[]> App::get_mesh_data() const
{
    const float                pi                    = 3.14159265f;
    std::unique_ptr<uint8_t[]> result_data_ptr;
    uint32_t                   result_mesh_data_size =  0;
    uint32_t                   result_n_vertices     =  0;

    /* Generate the mesh data. We need a total of five data streams:
     *
     * 1. Vertex data.                             (VK_FORMAT_R32G32_SFLOAT format)
     * 2. Color data, separately for each viewport (VK_FORMAT_R32G32B32_SFLOAT format)
     *
     * First compute a start offset for each stream, and then proceed with data generation.
     */
    static_assert(N_VIEWPORTS == 4, "get_mesh_data() impl assumes implicitly N_VIEWPORTS is 4");

    result_n_vertices     = get_mesh_n_vertices();
    result_mesh_data_size = get_mesh_data_size ();


    result_data_ptr.reset(
        new uint8_t[result_mesh_data_size]
    );

    for (unsigned int n_vertex = 0;
                      n_vertex < result_n_vertices;
                    ++n_vertex)
    {
        float* color1_data_ptr = reinterpret_cast<float*>(result_data_ptr.get() + get_mesh_color_data_start_offset (0, /* n_stream */
                                                                                                                    n_vertex) );
        float* color2_data_ptr = reinterpret_cast<float*>(result_data_ptr.get() + get_mesh_color_data_start_offset (1, /* n_stream */
                                                                                                                    n_vertex) );
        float* color3_data_ptr = reinterpret_cast<float*>(result_data_ptr.get() + get_mesh_color_data_start_offset (2, /* n_stream */
                                                                                                                    n_vertex) );
        float* color4_data_ptr = reinterpret_cast<float*>(result_data_ptr.get() + get_mesh_color_data_start_offset (3, /* n_stream */
                                                                                                                    n_vertex) );
        float* vertex_data_ptr = reinterpret_cast<float*>(result_data_ptr.get() + get_mesh_vertex_data_start_offset(n_vertex) );
        float  x;
        float  y;

        float* color_data_stream_ptrs[] =
        {
            color1_data_ptr,
            color2_data_ptr,
            color3_data_ptr,
            color4_data_ptr
        };

        if (n_vertex == 0)
        {
            x = 0.0f;
            y = 0.0f;
        }
        else
        {
            x = sin(float(n_vertex) / float(N_SUBDIVISION_TRIANGLES - 1) * 2.0f * pi);
            y = cos(float(n_vertex) / float(N_SUBDIVISION_TRIANGLES - 1) * 2.0f * pi);
        }

        *vertex_data_ptr = x; ++vertex_data_ptr;
        *vertex_data_ptr = y;

        for (unsigned int n_color_stream = 0;
                          n_color_stream < N_VIEWPORTS;
                        ++n_color_stream)
        {
            float* color_data_ptr = color_data_stream_ptrs[n_color_stream];

            *color_data_ptr = sin(float(x * 25.0f + n_color_stream) )                                           * 0.5f + 0.5f; ++color_data_ptr;
            *color_data_ptr = cos(float(y * 71.0f + n_color_stream) )                                           * 0.5f + 0.5f; ++color_data_ptr;
            *color_data_ptr = sin(float(x * 25.0f + n_color_stream) ) * cos(float(y * 71.0f + n_color_stream) ) * 0.5f + 0.5f;
        }
    }

    return result_data_ptr;
}

uint32_t App::get_mesh_data_size() const
{
    return get_mesh_vertex_data_start_offset() + get_mesh_vertex_data_n_components() * sizeof(float) * get_mesh_n_vertices();
}

uint32_t App::get_mesh_n_vertices() const
{
    return (1 /* central vertex */ + N_SUBDIVISION_TRIANGLES);
}

Anvil::Format App::get_mesh_vertex_data_format() const
{
    return Anvil::Format::R32G32_SFLOAT;
}

uint32_t App::get_mesh_vertex_data_n_components() const
{
    return 2;
}

uint32_t App::get_mesh_vertex_data_start_offset(uint32_t n_vertex) const
{
    const uint32_t vertex_data_per_vertex_size = sizeof(float) * get_mesh_vertex_data_n_components();
    const uint32_t vertex_data_start_offset    = get_mesh_color_data_start_offset(N_VIEWPORTS,
                                                                                  0);

    return vertex_data_start_offset + n_vertex * vertex_data_per_vertex_size;
}

void App::get_scissor_viewport_info(VkRect2D*   out_scissors,
                                    VkViewport* out_viewports)
{
    unsigned int min_size = (WINDOW_HEIGHT > WINDOW_WIDTH) ? WINDOW_WIDTH : WINDOW_HEIGHT;
    unsigned int x_delta  = (WINDOW_WIDTH  - min_size) / 2;
    unsigned int y_delta  = (WINDOW_HEIGHT - min_size) / 2;

    if (out_scissors != nullptr)
    {
        /* Top-left region */
        out_scissors[0].extent.height = WINDOW_HEIGHT / 2 - y_delta;
        out_scissors[0].extent.width  = WINDOW_WIDTH / 2 - x_delta;
        out_scissors[0].offset.x      = 0;
        out_scissors[0].offset.y      = 0;

        /* Top-right region */
        out_scissors[1]          = out_scissors[0];
        out_scissors[1].offset.x = WINDOW_WIDTH / 2 + x_delta;

        /* Bottom-left region */
        out_scissors[2]          = out_scissors[0];
        out_scissors[2].offset.y = WINDOW_HEIGHT / 2 + y_delta;

        /* Bottom-right region */
        out_scissors[3]          = out_scissors[2];
        out_scissors[3].offset.x = WINDOW_WIDTH / 2 + x_delta;
    }

    if (out_viewports != nullptr)
    {
        /* Top-left region */
        out_viewports[0].height   = float(WINDOW_HEIGHT / 2 - y_delta);
        out_viewports[0].maxDepth = 1.0f;
        out_viewports[0].minDepth = 0.0f;
        out_viewports[0].width    = float(WINDOW_WIDTH / 2 - x_delta);
        out_viewports[0].x        = 0.0f;
        out_viewports[0].y        = 0.0f;

        /* Top-right region */
        out_viewports[1]   = out_viewports[0];
        out_viewports[1].x = float(WINDOW_WIDTH / 2 + x_delta);

        /* Bottom-left region */
        out_viewports[2]   = out_viewports[0];
        out_viewports[2].y = float(WINDOW_HEIGHT / 2 + y_delta);

        /* Bottom-right region */
        out_viewports[3]   = out_viewports[2];
        out_viewports[3].x = float(WINDOW_WIDTH / 2 + x_delta);
    }
}

void App::init()
{
    init_vulkan   ();
    init_window   ();
    init_swapchain();

    init_buffers     ();
    init_framebuffers();
    init_semaphores  ();
    init_shaders     ();

    init_gfx_pipelines  ();
    init_command_buffers();
}

void App::init_buffers()
{
    auto mesh_data_ptr = get_mesh_data();

    /* Initialize the buffer object */
    auto create_info_ptr = Anvil::BufferCreateInfo::create_alloc(m_device_ptr.get(),
                                                                 get_mesh_data_size(),
                                                                 Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                 Anvil::SharingMode::EXCLUSIVE,
                                                                 Anvil::BufferCreateFlagBits::NONE,
                                                                 Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT,
                                                                 Anvil::MemoryFeatureFlagBits::NONE); /* in_memory_features */

    create_info_ptr->set_client_data(mesh_data_ptr.get() );

    m_mesh_data_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
}

void App::init_command_buffers()
{
    auto                         gfx_pipeline_manager_ptr(m_device_ptr->get_graphics_pipeline_manager() );
    Anvil::ImageSubresourceRange subresource_range;
    auto                         universal_queue_ptr     (m_device_ptr->get_universal_queue(0) );

    subresource_range.aspect_mask      = Anvil::ImageAspectFlagBits::COLOR_BIT;
    subresource_range.base_array_layer = 0;
    subresource_range.base_mip_level   = 0;
    subresource_range.layer_count      = 1;
    subresource_range.level_count      = 1;

    /* Set up rendering command buffers. We need one per swap-chain image. */
    uint32_t        n_universal_queue_family_indices = 0;
    const uint32_t* universal_queue_family_indices   = nullptr;

    m_device_ptr->get_queue_family_indices_for_queue_family_type(Anvil::QueueFamilyType::UNIVERSAL,
                                                                &n_universal_queue_family_indices,
                                                                &universal_queue_family_indices);

    anvil_assert(n_universal_queue_family_indices > 0);

    for (unsigned int n_current_swapchain_image = 0;
                      n_current_swapchain_image < N_SWAPCHAIN_IMAGES;
                    ++n_current_swapchain_image)
    {
        auto cmd_buffer_ptr = m_device_ptr->get_command_pool_for_queue_family_index(universal_queue_family_indices[0])->alloc_primary_level_command_buffer();

        /* Start recording commands */
        cmd_buffer_ptr->start_recording(false, /* one_time_submit          */
                                        true); /* simultaneous_use_allowed */
        {
            /* Switch the swap-chain image layout to renderable */
            {
                Anvil::ImageBarrier image_barrier(Anvil::AccessFlagBits::NONE,                       /* source_access_mask */
                                                  Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT,
                                                  Anvil::ImageLayout::UNDEFINED,
                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                  universal_queue_ptr->get_queue_family_index(),
                                                  universal_queue_ptr->get_queue_family_index(),
                                                  m_swapchain_ptr->get_image(n_current_swapchain_image),
                                                  subresource_range);

                cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::TOP_OF_PIPE_BIT,
                                                        Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
                                                        Anvil::DependencyFlagBits::NONE,
                                                        0,              /* in_memory_barrier_count        */
                                                        nullptr,        /* in_memory_barrier_ptrs         */
                                                        0,              /* in_buffer_memory_barrier_count */
                                                        nullptr,        /* in_buffer_memory_barrier_ptrs  */
                                                        1,              /* in_image_memory_barrier_count  */
                                                       &image_barrier);
            }

            /* Issue the draw call */
            VkClearValue attachment_clear_value;
            VkRect2D     render_area;
            VkViewport   viewports[4];

            get_scissor_viewport_info(nullptr, /* out_scissors */
                                      viewports);

            attachment_clear_value.color.float32[0] = 0.5f;
            attachment_clear_value.color.float32[1] = 0.25f;
            attachment_clear_value.color.float32[2] = 0.125f;
            attachment_clear_value.color.float32[3] = 1.0f;

            render_area.extent.height = WINDOW_HEIGHT;
            render_area.extent.width  = WINDOW_WIDTH;
            render_area.offset.x      = 0;
            render_area.offset.y      = 0;

            cmd_buffer_ptr->record_begin_render_pass(1, /* in_n_clear_values */
                                                    &attachment_clear_value,
                                                     m_fbos[n_current_swapchain_image].get(),
                                                     render_area,
                                                     m_renderpass_ptr.get(),
                                                     Anvil::SubpassContents::INLINE);
            {
                Anvil::Buffer* mesh_data_buffer_per_binding[g_n_attribute_bindings] =
                {
                    m_mesh_data_buffer_ptr.get(),
                    m_mesh_data_buffer_ptr.get(),
                    m_mesh_data_buffer_ptr.get(),
                    m_mesh_data_buffer_ptr.get(),
                    m_mesh_data_buffer_ptr.get(),
                };
                VkDeviceSize mesh_data_buffer_data_offset_per_binding[g_n_attribute_bindings] = {0};

                mesh_data_buffer_data_offset_per_binding[g_color1_attribute_binding] = get_mesh_color_data_start_offset (0 /* n_stream */);
                mesh_data_buffer_data_offset_per_binding[g_color2_attribute_binding] = get_mesh_color_data_start_offset (1 /* n_stream */);
                mesh_data_buffer_data_offset_per_binding[g_color3_attribute_binding] = get_mesh_color_data_start_offset (2 /* n_stream */);
                mesh_data_buffer_data_offset_per_binding[g_color4_attribute_binding] = get_mesh_color_data_start_offset (3 /* n_stream */);
                mesh_data_buffer_data_offset_per_binding[g_vertex_attribute_binding] = get_mesh_vertex_data_start_offset();

                cmd_buffer_ptr->record_bind_pipeline      (Anvil::PipelineBindPoint::GRAPHICS,
                                                           m_pipeline_id);
                cmd_buffer_ptr->record_bind_vertex_buffers(0, /* startBinding */
                                                           g_n_attribute_bindings,
                                                           mesh_data_buffer_per_binding,
                                                           mesh_data_buffer_data_offset_per_binding);
                cmd_buffer_ptr->record_set_viewport       (0, /* in_first_viewport */
                                                           sizeof(viewports) / sizeof(viewports[0]),
                                                           viewports);

                cmd_buffer_ptr->record_draw(get_mesh_n_vertices(),
                                            1,  /* instanceCount */
                                            0,  /* firstVertex */
                                            0); /* firstInstance */
            }
            cmd_buffer_ptr->record_end_render_pass();

            /* Change the swap-chain image's layout to presentable */
            {
                Anvil::ImageBarrier image_barrier(Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT,        /* source_access_mask      */
                                                  Anvil::AccessFlagBits::MEMORY_READ_BIT,                   /* destination_access_mask */
                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,/* old_image_layout        */
#ifdef ENABLE_OFFSCREEN_RENDERING
                                                  Anvil::ImageLayout::GENERAL,                 /* new_image_layout        */
#else
                                                  Anvil::ImageLayout::PRESENT_SRC_KHR,         /* new_image_layout        */
#endif
                                                  universal_queue_ptr->get_queue_family_index(),
                                                  universal_queue_ptr->get_queue_family_index(),
                                                  m_swapchain_ptr->get_image(n_current_swapchain_image),
                                                  subresource_range);

                cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
                                                        Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                        Anvil::DependencyFlagBits::NONE,
                                                        0,              /* in_memory_barrier_count        */
                                                        nullptr,        /* in_memory_barrier_ptrs         */
                                                        0,              /* in_buffer_memory_barrier_count */
                                                        nullptr,        /* in_buffer_memory_barrier_ptrs  */
                                                        1,              /* in_image_memory_barrier_count  */
                                                       &image_barrier);
            }
        }

        /* Close the recording process */
        cmd_buffer_ptr->stop_recording();

        m_command_buffers[n_current_swapchain_image] = std::move(cmd_buffer_ptr);
    }
}

void App::init_framebuffers()
{
    bool result;

    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < N_SWAPCHAIN_IMAGES;
                ++n_swapchain_image)
    {
        {
            auto fb_create_info_ptr = Anvil::FramebufferCreateInfo::create(m_device_ptr.get(),
                                                                           WINDOW_WIDTH,
                                                                           WINDOW_HEIGHT,
                                                                           1 /* n_layers */);

            result = fb_create_info_ptr->add_attachment(m_swapchain_ptr->get_image_view(n_swapchain_image),
                                                        nullptr /* out_opt_attachment_id_ptrs */);
            anvil_assert(result);

            m_fbos[n_swapchain_image] = Anvil::Framebuffer::create(std::move(fb_create_info_ptr) );
        }

        m_fbos[n_swapchain_image]->set_name_formatted("Framebuffer used to render to swapchain image [%d]",
                                                      n_swapchain_image);
    }
}

void App::init_gfx_pipelines()
{
    auto                gfx_pipeline_manager_ptr(m_device_ptr->get_graphics_pipeline_manager() );
    const Anvil::Format mesh_color_data_format  (get_mesh_color_data_format                 ());
    const Anvil::Format mesh_vertex_data_format (get_mesh_vertex_data_format                ());
    const uint32_t      n_mesh_color_components (get_mesh_color_data_n_components           ());
    const uint32_t      n_mesh_vertex_components(get_mesh_vertex_data_n_components          ());

    /* Create a render pass for the pipeline */
    Anvil::GraphicsPipelineCreateInfoUniquePtr gfx_pipeline_create_info_ptr;
    Anvil::RenderPassAttachmentID              render_pass_color_attachment_id;
    Anvil::SubPassID                           render_pass_subpass_id;
    VkRect2D                                   scissors[4];

    get_scissor_viewport_info(scissors,
                              nullptr); /* viewports */

    {
        Anvil::RenderPassCreateInfoUniquePtr render_pass_create_info_ptr(new Anvil::RenderPassCreateInfo(m_device_ptr.get() ) );

        render_pass_create_info_ptr->add_color_attachment        (m_swapchain_ptr->get_create_info_ptr()->get_format(),
                                                                  Anvil::SampleCountFlagBits::_1_BIT,
                                                                  Anvil::AttachmentLoadOp::CLEAR,
                                                                  Anvil::AttachmentStoreOp::STORE,
                                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                  false, /* may_alias */
                                                                 &render_pass_color_attachment_id);
        render_pass_create_info_ptr->add_subpass                 (&render_pass_subpass_id);
        render_pass_create_info_ptr->add_subpass_color_attachment(render_pass_subpass_id,
                                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                  render_pass_color_attachment_id,
                                                                  0,        /* in_location                      */
                                                                  nullptr); /* in_opt_attachment_resolve_id_ptr */

        m_renderpass_ptr = Anvil::RenderPass::create(std::move(render_pass_create_info_ptr),
                                                     m_swapchain_ptr.get() );
    }

    m_renderpass_ptr->set_name("Main renderpass");

    /* Configure the graphics pipeline */
    gfx_pipeline_create_info_ptr = Anvil::GraphicsPipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                             m_renderpass_ptr.get(),
                                                                             render_pass_subpass_id,
                                                                            *m_fs_ptr,
                                                                            *m_gs_ptr,
                                                                             Anvil::ShaderModuleStageEntryPoint(),
                                                                             Anvil::ShaderModuleStageEntryPoint(),
                                                                            *m_vs_ptr);

    gfx_pipeline_create_info_ptr->set_n_dynamic_viewports     (sizeof(scissors) / sizeof(scissors[0]) );
    gfx_pipeline_create_info_ptr->set_primitive_topology      (Anvil::PrimitiveTopology::TRIANGLE_FAN);
    gfx_pipeline_create_info_ptr->set_rasterization_properties(Anvil::PolygonMode::FILL,
                                                               Anvil::CullModeFlagBits::NONE,
                                                               Anvil::FrontFace::COUNTER_CLOCKWISE,
                                                               1.0f /* line_width */);
    gfx_pipeline_create_info_ptr->toggle_dynamic_state        (true, /* should_enable */
                                                               Anvil::DynamicState::VIEWPORT);
    gfx_pipeline_create_info_ptr->toggle_primitive_restart    (true /* should_enable */);

    gfx_pipeline_create_info_ptr->add_vertex_attribute(g_vertex_attribute_location,
                                                       mesh_vertex_data_format,
                                                       0,                                        /* offset_in_bytes */
                                                       sizeof(float) * n_mesh_vertex_components, /* stride_in_bytes */
                                                       Anvil::VertexInputRate::VERTEX,
                                                       g_vertex_attribute_binding);
    gfx_pipeline_create_info_ptr->add_vertex_attribute(g_color1_attribute_location,
                                                       mesh_color_data_format,
                                                       0,                                       /* offset_in_bytes */
                                                       sizeof(float) * n_mesh_color_components, /* stride_in_bytes */
                                                       Anvil::VertexInputRate::VERTEX,
                                                       g_color1_attribute_binding);
    gfx_pipeline_create_info_ptr->add_vertex_attribute(g_color2_attribute_location,
                                                       mesh_color_data_format,
                                                       0,                                       /* offset_in_bytes */
                                                       sizeof(float) * n_mesh_color_components, /* stride_in_bytes */
                                                       Anvil::VertexInputRate::VERTEX,
                                                       g_color2_attribute_binding);
    gfx_pipeline_create_info_ptr->add_vertex_attribute(g_color3_attribute_location,
                                                       mesh_color_data_format,
                                                       0,                                       /* offset_in_bytes */
                                                       sizeof(float) * n_mesh_color_components, /* stride_in_bytes */
                                                       Anvil::VertexInputRate::VERTEX,
                                                       g_color3_attribute_binding);
    gfx_pipeline_create_info_ptr->add_vertex_attribute(g_color4_attribute_location,
                                                       mesh_color_data_format,
                                                       0,                                       /* offset_in_bytes */
                                                       sizeof(float) * n_mesh_color_components, /* stride_in_bytes */
                                                       Anvil::VertexInputRate::VERTEX,
                                                       g_color4_attribute_binding);

    for (uint32_t n_scissor_box = 0;
                  n_scissor_box < sizeof(scissors) / sizeof(scissors[0]);
                ++n_scissor_box)
    {
        gfx_pipeline_create_info_ptr->set_scissor_box_properties(n_scissor_box,
                                                                 scissors[n_scissor_box].offset.x,
                                                                 scissors[n_scissor_box].offset.y,
                                                                 scissors[n_scissor_box].extent.width,
                                                                 scissors[n_scissor_box].extent.height);
    }


    gfx_pipeline_manager_ptr->add_pipeline(std::move(gfx_pipeline_create_info_ptr),
                                          &m_pipeline_id);
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
    Anvil::ShaderModuleUniquePtr               fs_module_ptr;
    Anvil::ShaderModuleUniquePtr               gs_module_ptr;
    Anvil::ShaderModuleUniquePtr               vs_module_ptr;
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr geometry_shader_ptr;
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr vertex_shader_ptr;

    fragment_shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    g_glsl_render_frag,
                                                                    Anvil::ShaderStage::FRAGMENT);
    vertex_shader_ptr   = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    g_glsl_render_vert,
                                                                    Anvil::ShaderStage::VERTEX);
    geometry_shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    g_glsl_render_geom,
                                                                    Anvil::ShaderStage::GEOMETRY);

    fs_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get       (),
                                                                     fragment_shader_ptr.get() );
    gs_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get       (),
                                                                     geometry_shader_ptr.get() );
    vs_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get       (),
                                                                     vertex_shader_ptr.get  () );

    fs_module_ptr->set_name("Fragment shader");
    gs_module_ptr->set_name("Geometry shader");
    vs_module_ptr->set_name("Vertex shader");

    m_fs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            std::move(fs_module_ptr),
            Anvil::ShaderStage::FRAGMENT)
    );
    m_gs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            std::move(gs_module_ptr),
            Anvil::ShaderStage::GEOMETRY)
    );
    m_vs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            std::move(vs_module_ptr),
            Anvil::ShaderStage::VERTEX)
    );
}

void App::init_swapchain()
{
    {
        auto create_info_ptr = Anvil::RenderingSurfaceCreateInfo::create(m_instance_ptr.get(),
                                                                         m_device_ptr.get  (),
                                                                         m_window_ptr.get  () );

        m_rendering_surface_ptr = Anvil::RenderingSurface::create(std::move(create_info_ptr) );
    }

    m_rendering_surface_ptr->set_name("Main rendering surface");


    m_swapchain_ptr = dynamic_cast<Anvil::SGPUDevice*>(m_device_ptr.get() )->create_swapchain(m_rendering_surface_ptr.get(),
                                                                                              m_window_ptr.get           (),
                                                                                              Anvil::Format::B8G8R8A8_UNORM,
                                                                                              Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,
                                                                                              Anvil::PresentModeKHR::FIFO_KHR,
                                                                                              Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
                                                                                              m_n_swapchain_images);

    m_swapchain_ptr->set_name("Main swapchain");

    /* Cache the queue we are going to use for presentation */
    const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

    if (!m_rendering_surface_ptr->get_queue_families_with_present_support(dynamic_cast<Anvil::SGPUDevice*>(m_device_ptr.get() )->get_physical_device(),
                                                                         &present_queue_fams_ptr) )
    {
        anvil_assert_fail();
    }

    m_present_queue_ptr = m_device_ptr->get_queue_for_queue_family_index(present_queue_fams_ptr->at(0),
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
        auto create_info_ptr = Anvil::InstanceCreateInfo::create(APP_NAME,  /* app_name    */
                                                                 APP_NAME,  /* engine_name */
#ifdef ENABLE_VALIDATION
                                                                 std::bind(&App::on_validation_callback,
                                                                           this,
                                                                           std::placeholders::_1,
                                                                           std::placeholders::_2),
#else
                                                                 Anvil::DebugCallbackFunction(),
#endif
                                                                 false);   /* in_mt_safe */

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

