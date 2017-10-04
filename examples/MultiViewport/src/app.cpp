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
#include "misc/glsl_to_spirv.h"
#include "misc/object_tracker.h"
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
     m_n_swapchain_images   (N_SWAPCHAIN_IMAGES)
{
}

App::~App()
{
    deinit();
}

void App::deinit()
{
    vkDeviceWaitIdle(m_device_ptr.lock()->get_device_vk() );

    m_frame_signal_semaphores.clear();
    m_frame_wait_semaphores.clear();

    m_present_queue_ptr.reset();
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

    m_device_ptr.lock()->destroy();
    m_device_ptr.reset();

    m_instance_ptr->destroy();
    m_instance_ptr.reset();

    m_window_ptr.reset();
}

void App::draw_frame(void* app_raw_ptr)
{
    App*                               app_ptr                         = static_cast<App*>(app_raw_ptr);
    std::shared_ptr<Anvil::Semaphore>  curr_frame_signal_semaphore_ptr;
    std::shared_ptr<Anvil::Semaphore>  curr_frame_wait_semaphore_ptr;
    std::shared_ptr<Anvil::SGPUDevice> device_locked_ptr               = app_ptr->m_device_ptr.lock();
    static uint32_t                    n_frames_rendered               = 0;
    uint32_t                           n_swapchain_image;
    std::shared_ptr<Anvil::Queue>      present_queue_ptr               = device_locked_ptr->get_universal_queue(0);
    const VkPipelineStageFlags         wait_stage_mask                 = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    /* Determine the signal + wait semaphores to use for drawing this frame */
    app_ptr->m_n_last_semaphore_used = (app_ptr->m_n_last_semaphore_used + 1) % app_ptr->m_n_swapchain_images;

    curr_frame_signal_semaphore_ptr = app_ptr->m_frame_signal_semaphores[app_ptr->m_n_last_semaphore_used];
    curr_frame_wait_semaphore_ptr   = app_ptr->m_frame_wait_semaphores  [app_ptr->m_n_last_semaphore_used];

    /* Determine the semaphore which the swapchain image */
    n_swapchain_image = app_ptr->m_swapchain_ptr->acquire_image(curr_frame_wait_semaphore_ptr);

    /* Submit work chunk and present */
    device_locked_ptr->get_universal_queue(0)->submit_command_buffer_with_signal_wait_semaphores(app_ptr->m_command_buffers[n_swapchain_image],
                                                                                                 1, /* n_semaphores_to_signal */
                                                                                                &curr_frame_signal_semaphore_ptr,
                                                                                                 1, /* n_semaphores_to_wait_on */
                                                                                                &curr_frame_wait_semaphore_ptr,
                                                                                                &wait_stage_mask,
                                                                                                 false,    /* should_block  */
                                                                                                 nullptr); /* opt_fence_ptr */

    present_queue_ptr->present(app_ptr->m_swapchain_ptr,
                               n_swapchain_image,
                               1, /* n_wait_semaphores */
                              &curr_frame_signal_semaphore_ptr);

    #ifdef ENABLE_OFFSCREEN_RENDERING
    {
        if (n_frames_rendered < N_FRAMES_TO_RENDER)
        {
            ++n_frames_rendered;
        }
        else
        {
            app_ptr->m_window_ptr->close();
        }
    }
    #endif
}

VkFormat App::get_mesh_color_data_format() const
{
    return VK_FORMAT_R32G32B32_SFLOAT;
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

std::shared_ptr<unsigned char> App::get_mesh_data() const
{
    const float                    pi                    = 3.14159265f;
    std::shared_ptr<unsigned char> result_data_ptr;
    uint32_t                       result_mesh_data_size =  0;
    uint32_t                       result_n_vertices     =  0;

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


    result_data_ptr.reset(new unsigned char[result_mesh_data_size]);

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

VkFormat App::get_mesh_vertex_data_format() const
{
    return VK_FORMAT_R32G32_SFLOAT;
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
    std::shared_ptr<unsigned char> mesh_data_ptr = get_mesh_data();

    /* Initialize the buffer object */
    m_mesh_data_buffer_ptr = Anvil::Buffer::create_nonsparse(
        m_device_ptr,
        get_mesh_data_size(),
        Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        0, /* in_memory_features */
        mesh_data_ptr.get() );
}

void App::init_command_buffers()
{
    std::shared_ptr<Anvil::SGPUDevice>              device_locked_ptr       (m_device_ptr);
    std::shared_ptr<Anvil::GraphicsPipelineManager> gfx_pipeline_manager_ptr(device_locked_ptr->get_graphics_pipeline_manager() );
    VkImageSubresourceRange                         subresource_range;
    std::shared_ptr<Anvil::Queue>                   universal_queue_ptr     (device_locked_ptr->get_universal_queue(0) );

    subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseArrayLayer = 0;
    subresource_range.baseMipLevel   = 0;
    subresource_range.layerCount     = 1;
    subresource_range.levelCount     = 1;

    /* Set up rendering command buffers. We need one per swap-chain image. */
    for (unsigned int n_current_swapchain_image = 0;
                      n_current_swapchain_image < N_SWAPCHAIN_IMAGES;
                    ++n_current_swapchain_image)
    {
        std::shared_ptr<Anvil::PrimaryCommandBuffer> cmd_buffer_ptr = device_locked_ptr->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)->alloc_primary_level_command_buffer();

        /* Start recording commands */
        cmd_buffer_ptr->start_recording(false, /* one_time_submit          */
                                        true); /* simultaneous_use_allowed */
        {
            /* Switch the swap-chain image layout to renderable */
            {
                Anvil::ImageBarrier image_barrier(0, /* source_access_mask      */
                                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                  false,
                                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                  universal_queue_ptr->get_queue_family_index(),
                                                  universal_queue_ptr->get_queue_family_index(),
                                                  m_swapchain_ptr->get_image(n_current_swapchain_image),
                                                  subresource_range);

                cmd_buffer_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                        VK_FALSE,       /* in_by_region                   */
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
                                                     m_fbos[n_current_swapchain_image],
                                                     render_area,
                                                     m_renderpass_ptr,
                                                     VK_SUBPASS_CONTENTS_INLINE);
            {
                std::shared_ptr<Anvil::Buffer> mesh_data_buffer_per_binding[g_n_attribute_bindings] =
                {
                    m_mesh_data_buffer_ptr,
                    m_mesh_data_buffer_ptr,
                    m_mesh_data_buffer_ptr,
                    m_mesh_data_buffer_ptr,
                    m_mesh_data_buffer_ptr,
                };
                VkDeviceSize mesh_data_buffer_data_offset_per_binding[g_n_attribute_bindings] = {0};

                mesh_data_buffer_data_offset_per_binding[g_color1_attribute_binding] = get_mesh_color_data_start_offset (0 /* n_stream */);
                mesh_data_buffer_data_offset_per_binding[g_color2_attribute_binding] = get_mesh_color_data_start_offset (1 /* n_stream */);
                mesh_data_buffer_data_offset_per_binding[g_color3_attribute_binding] = get_mesh_color_data_start_offset (2 /* n_stream */);
                mesh_data_buffer_data_offset_per_binding[g_color4_attribute_binding] = get_mesh_color_data_start_offset (3 /* n_stream */);
                mesh_data_buffer_data_offset_per_binding[g_vertex_attribute_binding] = get_mesh_vertex_data_start_offset();

                cmd_buffer_ptr->record_bind_pipeline      (VK_PIPELINE_BIND_POINT_GRAPHICS,
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
                Anvil::ImageBarrier image_barrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,        /* source_access_mask      */
                                                  0,                                           /* destination_access_mask */
                                                  false,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,    /* old_image_layout        */
#ifdef ENABLE_OFFSCREEN_RENDERING
                                                  VK_IMAGE_LAYOUT_GENERAL,                     /* new_image_layout        */
#else
                                                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,             /* new_image_layout        */
#endif
                                                  universal_queue_ptr->get_queue_family_index(),
                                                  universal_queue_ptr->get_queue_family_index(),
                                                  m_swapchain_ptr->get_image(n_current_swapchain_image),
                                                  subresource_range);

                cmd_buffer_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                                        VK_FALSE,       /* in_by_region                   */
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

        m_command_buffers[n_current_swapchain_image] = cmd_buffer_ptr;
    }
}

void App::init_framebuffers()
{
    bool result;

    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < N_SWAPCHAIN_IMAGES;
                ++n_swapchain_image)
    {
        m_fbos[n_swapchain_image] = Anvil::Framebuffer::create(
            m_device_ptr,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            1 /* n_layers */);

        m_fbos[n_swapchain_image]->set_name_formatted("Framebuffer used to render to swapchain image [%d]",
                                                      n_swapchain_image);

        result = m_fbos[n_swapchain_image]->add_attachment(m_swapchain_ptr->get_image_view(n_swapchain_image),
                                                           nullptr /* out_opt_attachment_id_ptrs */);
        anvil_assert(result);
    }
}

void App::init_gfx_pipelines()
{
    std::shared_ptr<Anvil::SGPUDevice>              device_locked_ptr       (m_device_ptr);
    std::shared_ptr<Anvil::GraphicsPipelineManager> gfx_pipeline_manager_ptr(device_locked_ptr->get_graphics_pipeline_manager() );
    const VkFormat                                  mesh_color_data_format  (get_mesh_color_data_format       ());
    const VkFormat                                  mesh_vertex_data_format (get_mesh_vertex_data_format      ());
    const uint32_t                                  n_mesh_color_components (get_mesh_color_data_n_components ());
    const uint32_t                                  n_mesh_vertex_components(get_mesh_vertex_data_n_components());
    bool                                            result;

    /* Create a render pass for the pipeline */
    Anvil::RenderPassAttachmentID render_pass_color_attachment_id;
    Anvil::SubPassID              render_pass_subpass_id;
    VkRect2D                      scissors[4];

    get_scissor_viewport_info(scissors,
                              nullptr); /* viewports */

    m_renderpass_ptr = Anvil::RenderPass::create(
        m_device_ptr,
        m_swapchain_ptr);

    m_renderpass_ptr->set_name("Main renderpass");

    result = m_renderpass_ptr->add_color_attachment(m_swapchain_ptr->get_image_format(),
                                                    VK_SAMPLE_COUNT_1_BIT,
                                                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                    VK_ATTACHMENT_STORE_OP_STORE,
                                                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                    false, /* may_alias */
                                                   &render_pass_color_attachment_id);
    anvil_assert(result);

    result = m_renderpass_ptr->add_subpass(*m_fs_ptr,
                                           *m_gs_ptr,
                                            Anvil::ShaderModuleStageEntryPoint(),
                                            Anvil::ShaderModuleStageEntryPoint(),
                                           *m_vs_ptr,
                                          &render_pass_subpass_id);
    anvil_assert(result);

    result = m_renderpass_ptr->get_subpass_graphics_pipeline_id(render_pass_subpass_id,
                                                                         &m_pipeline_id);
    anvil_assert(result);

    result = m_renderpass_ptr->add_subpass_color_attachment(render_pass_subpass_id,
                                                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                                      render_pass_color_attachment_id,
                                                                      0,        /* location                      */
                                                                      nullptr); /* opt_attachment_resolve_id_ptr */
    anvil_assert(result);

    /* Configure the graphics pipeline */
    gfx_pipeline_manager_ptr->set_dynamic_viewport_state_properties(m_pipeline_id,
                                                                    sizeof(scissors) / sizeof(scissors[0]) );
    gfx_pipeline_manager_ptr->set_input_assembly_properties        (m_pipeline_id,
                                                                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
    gfx_pipeline_manager_ptr->set_rasterization_properties         (m_pipeline_id,
                                                                    VK_POLYGON_MODE_FILL,
                                                                    VK_CULL_MODE_NONE,
                                                                    VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                                                    1.0f /* line_width */);
    gfx_pipeline_manager_ptr->toggle_dynamic_states                (m_pipeline_id,
                                                                    true, /* should_enable */
                                                                    Anvil::GraphicsPipelineManager::DYNAMIC_STATE_VIEWPORT_BIT);
    gfx_pipeline_manager_ptr->toggle_primitive_restart             (m_pipeline_id,
                                                                    true /* should_enable */);

    result  = gfx_pipeline_manager_ptr->add_vertex_attribute(m_pipeline_id,
                                                             g_vertex_attribute_location,
                                                             mesh_vertex_data_format,
                                                             0,                                        /* offset_in_bytes */
                                                             sizeof(float) * n_mesh_vertex_components, /* stride_in_bytes */
                                                             VK_VERTEX_INPUT_RATE_VERTEX,
                                                             g_vertex_attribute_binding);
    result &= gfx_pipeline_manager_ptr->add_vertex_attribute(m_pipeline_id,
                                                             g_color1_attribute_location,
                                                             mesh_color_data_format,
                                                             0,                                       /* offset_in_bytes */
                                                             sizeof(float) * n_mesh_color_components, /* stride_in_bytes */
                                                             VK_VERTEX_INPUT_RATE_VERTEX,
                                                             g_color1_attribute_binding);
    result &= gfx_pipeline_manager_ptr->add_vertex_attribute(m_pipeline_id,
                                                             g_color2_attribute_location,
                                                             mesh_color_data_format,
                                                             0,                                       /* offset_in_bytes */
                                                             sizeof(float) * n_mesh_color_components, /* stride_in_bytes */
                                                             VK_VERTEX_INPUT_RATE_VERTEX,
                                                             g_color2_attribute_binding);
    result &= gfx_pipeline_manager_ptr->add_vertex_attribute(m_pipeline_id,
                                                             g_color3_attribute_location,
                                                             mesh_color_data_format,
                                                             0,                                       /* offset_in_bytes */
                                                             sizeof(float) * n_mesh_color_components, /* stride_in_bytes */
                                                             VK_VERTEX_INPUT_RATE_VERTEX,
                                                             g_color3_attribute_binding);
    result &= gfx_pipeline_manager_ptr->add_vertex_attribute(m_pipeline_id,
                                                             g_color4_attribute_location,
                                                             mesh_color_data_format,
                                                             0,                                       /* offset_in_bytes */
                                                             sizeof(float) * n_mesh_color_components, /* stride_in_bytes */
                                                             VK_VERTEX_INPUT_RATE_VERTEX,
                                                             g_color4_attribute_binding);

    anvil_assert(result);

    for (uint32_t n_scissor_box = 0;
                  n_scissor_box < sizeof(scissors) / sizeof(scissors[0]);
                ++n_scissor_box)
    {
        gfx_pipeline_manager_ptr->set_scissor_box_properties(m_pipeline_id,
                                                             n_scissor_box,
                                                             scissors[n_scissor_box].offset.x,
                                                             scissors[n_scissor_box].offset.y,
                                                             scissors[n_scissor_box].extent.width,
                                                             scissors[n_scissor_box].extent.height);
    }
}

void App::init_semaphores()
{
    for (uint32_t n_semaphore = 0;
                  n_semaphore < m_n_swapchain_images;
                ++n_semaphore)
    {
        std::shared_ptr<Anvil::Semaphore> new_signal_semaphore_ptr = Anvil::Semaphore::create(m_device_ptr);
        std::shared_ptr<Anvil::Semaphore> new_wait_semaphore_ptr   = Anvil::Semaphore::create(m_device_ptr);

        new_signal_semaphore_ptr->set_name_formatted("Signal semaphore [%d]",
                                                     n_semaphore);
        new_wait_semaphore_ptr->set_name_formatted  ("Wait semaphore [%d]",
                                                     n_semaphore);

        m_frame_signal_semaphores.push_back(new_signal_semaphore_ptr);
        m_frame_wait_semaphores.push_back  (new_wait_semaphore_ptr);
    }
}

void App::init_shaders()
{
    std::shared_ptr<Anvil::GLSLShaderToSPIRVGenerator> fragment_shader_ptr;
    std::shared_ptr<Anvil::ShaderModule>               fragment_shader_module_ptr;
    std::shared_ptr<Anvil::GLSLShaderToSPIRVGenerator> geometry_shader_ptr;
    std::shared_ptr<Anvil::ShaderModule>               geometry_shader_module_ptr;
    std::shared_ptr<Anvil::GLSLShaderToSPIRVGenerator> vertex_shader_ptr;
    std::shared_ptr<Anvil::ShaderModule>               vertex_shader_module_ptr;

    fragment_shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    g_glsl_render_frag,
                                                                    Anvil::SHADER_STAGE_FRAGMENT);
    vertex_shader_ptr   = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    g_glsl_render_vert,
                                                                    Anvil::SHADER_STAGE_VERTEX);
    geometry_shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    g_glsl_render_geom,
                                                                    Anvil::SHADER_STAGE_GEOMETRY);

    fragment_shader_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr,
                                                                                  fragment_shader_ptr);
    geometry_shader_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr,
                                                                                  geometry_shader_ptr);
    vertex_shader_module_ptr   = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr,
                                                                                  vertex_shader_ptr);

    fragment_shader_module_ptr->set_name("Fragment shader");
    geometry_shader_module_ptr->set_name("Geometry shader");
    vertex_shader_module_ptr->set_name  ("Vertex shader");

    m_fs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            fragment_shader_module_ptr,
            Anvil::SHADER_STAGE_FRAGMENT)
    );
    m_gs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            geometry_shader_module_ptr,
            Anvil::SHADER_STAGE_GEOMETRY)
    );
    m_vs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            vertex_shader_module_ptr,
            Anvil::SHADER_STAGE_VERTEX)
    );
}

void App::init_swapchain()
{
    std::shared_ptr<Anvil::SGPUDevice> device_locked_ptr(m_device_ptr);

    m_rendering_surface_ptr = Anvil::RenderingSurface::create(m_instance_ptr,
                                                              m_device_ptr,
                                                              m_window_ptr);

    m_rendering_surface_ptr->set_name("Main rendering surface");


    m_swapchain_ptr = device_locked_ptr->create_swapchain(m_rendering_surface_ptr,
                                                          m_window_ptr,
                                                          VK_FORMAT_B8G8R8A8_UNORM,
                                                          VK_PRESENT_MODE_FIFO_KHR,
                                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                          m_n_swapchain_images);

    m_swapchain_ptr->set_name("Main swapchain");

    /* Cache the queue we are going to use for presentation */
    const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

    if (!m_rendering_surface_ptr->get_queue_families_with_present_support(device_locked_ptr->get_physical_device(),
                                                                         &present_queue_fams_ptr) )
    {
        anvil_assert_fail();
    }

    m_present_queue_ptr = device_locked_ptr->get_queue(present_queue_fams_ptr->at(0),
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
                                                       draw_frame,
                                                       this);
}

void App::init_vulkan()
{
    /* Create a Vulkan instance */
    m_instance_ptr = Anvil::Instance::create(APP_NAME,  /* app_name */
                                             APP_NAME,  /* engine_name */
#ifdef ENABLE_VALIDATION
                                             on_validation_callback,
#else
                                             nullptr,
#endif
                                             nullptr);   /* validation_proc_user_arg */

    m_physical_device_ptr = m_instance_ptr->get_physical_device(0);

    /* Create a Vulkan device */
    m_device_ptr = Anvil::SGPUDevice::create(m_physical_device_ptr,
                                             Anvil::DeviceExtensionConfiguration(),
                                             std::vector<std::string>(), /* layers                               */
                                             false,                      /* transient_command_buffer_allocs_only */
                                             false);                     /* support_resettable_command_buffers   */
}

VkBool32 App::on_validation_callback(VkDebugReportFlagsEXT      message_flags,
                                     VkDebugReportObjectTypeEXT object_type,
                                     const char*                layer_prefix,
                                     const char*                message,
                                     void*                      user_arg)
{
    if ((message_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0)
    {
        fprintf(stderr,
                "[!] %s\n",
                message);
    }

    return false;
}

void App::run()
{
    m_window_ptr->run();
}


int main()
{
    std::shared_ptr<App> app_ptr(new App() );

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

