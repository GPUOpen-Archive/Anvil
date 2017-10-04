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

#include "config.h"
#include <string>
#include "misc/glsl_to_spirv.h"
#include "misc/io.h"
#include "misc/object_tracker.h"
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
#define APP_NAME      "Occlusion query example application"
#define WINDOW_WIDTH  (1280)
#define WINDOW_HEIGHT (720)

/* When offscreen rendering is enabled, N_FRAMES_TO_RENDER tells how many frames should be
 * rendered before leaving */
#define N_FRAMES_TO_RENDER (60)


static const char* g_glsl_render_quad_frag =
    "#version 430\n"
    "\n"
    "layout(location = 0) in vec4  status_color;\n"
    "layout(location = 0) out vec4 result;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    result = status_color;\n"
    "}\n";

static const char* g_glsl_render_quad_vert =
    "#version 430\n"
    "\n"
    "\n"
    "layout(binding = 0, set = 0) uniform ub\n"
    "{\n"
    "    uint query;\n"
    "};\n"
    "\n"
    "layout(location = 0) out vec4 status_color;\n"
    "\n"
    "\n"
    "void main()\n"
    "{\n"
    "    const float scale = 0.2;\n"
    "\n"
    "    status_color = mix(vec4(1.0, 0.0, 0.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0), vec4(float(query) / 25000) );\n"
    "\n"
    "    switch (gl_VertexIndex)\n"
    "    {\n"
    "        case 0: gl_Position = vec4(-1.0 * scale, -1.0 * scale + 0.5, 0.0, 1.0); break;\n"
    "        case 1: gl_Position = vec4(-1.0 * scale,  1.0 * scale + 0.5, 0.0, 1.0); break;\n"
    "        case 2: gl_Position = vec4( 1.0 * scale, -1.0 * scale + 0.5, 0.0, 1.0); break;\n"
    "        case 3: gl_Position = vec4( 1.0 * scale,  1.0 * scale + 0.5, 0.0, 1.0); break;\n"
    "    }\n"
    "}\n";


static const char* g_glsl_render_tri_frag =
    "#version 430\n"
    "\n"
    "layout(location = 0) out vec4 color;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    color = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";


static const char* g_glsl_render_tri_vert =
    "#version 430\n"
    "\n"
    "layout(binding = 0, set = 0) uniform ub\n"
    "{\n"
    "    float time;\n"
    "};\n"
    "\n"
    "void main()\n"
    "{\n"
    "    const float scale = 0.5;\n"
    "\n"
    "    const vec2 ref_data[] =\n"
    "    {\n"
    "        vec2(0.5, 0.0),\n"
    "        vec2(0.0, 1.0),\n"
    "        vec2(1.0, 1.0)\n"
    "    };\n"
    "    vec2 ref_vertex;\n"
    "\n"
    "    switch (gl_VertexIndex)\n"
    "    {\n"
    "        case 0: ref_vertex = ref_data[0]; break;\n"
    "        case 1: ref_vertex = ref_data[1]; break;\n"
    "        case 2: ref_vertex = ref_data[2]; break;\n"
    "    }\n"
    "\n"
    "    switch (gl_InstanceIndex)\n"
    "    {\n"
    "        case 0: gl_Position = vec4(ref_vertex * vec2(scale) - vec2(0.25, 0.0) + vec2(0.5 * sin(2.0 * 3.14152965 * fract(time * 0.25)), -0.25), 0.0, 1.0); break;\n"
    "        case 1: gl_Position = vec4(ref_vertex * vec2(scale) - vec2(0.25, 0.0) - vec2(0.5 * sin(2.0 * 3.14152965 * fract(time * 0.25)),  0.25), 0.0, 1.0); break;\n"
    "    }\n"
    "}\n";


App::App()
    :m_n_last_semaphore_used(0),
     m_n_swapchain_images   (N_SWAPCHAIN_IMAGES)
{
    // ..
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

    m_1stpass_dsg_ptr.reset();
    m_2ndpass_quad_dsg_ptr.reset();
    m_2ndpass_tri_dsg_ptr.reset();
    m_depth_image_ptr.reset();
    m_depth_image_view_ptr.reset();
    m_quad_fs_ptr.reset();
    m_quad_vs_ptr.reset();
    m_query_bo_ptr.reset();
    m_query_data_copied_event.reset();
    m_query_pool_ptr.reset();
    m_renderpass_quad_ptr.reset();
    m_renderpass_tris_ptr.reset();
    m_time_bo_ptr.reset();
    m_tri_fs_ptr.reset();
    m_tri_vs_ptr.reset();

    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < N_SWAPCHAIN_IMAGES;
                ++n_swapchain_image)
    {
        m_render_tri1_and_generate_ot_data_cmd_buffers[n_swapchain_image].reset();
        m_render_tri2_and_quad_cmd_buffers            [n_swapchain_image].reset();

        m_fbos[n_swapchain_image].reset();
    }

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
    std::shared_ptr<Anvil::Semaphore>  present_wait_semaphore_ptr;
    const VkPipelineStageFlags         wait_stage_mask                 = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    /* Determine the signal + wait semaphores to use for drawing this frame */
    app_ptr->m_n_last_semaphore_used = (app_ptr->m_n_last_semaphore_used + 1) % app_ptr->m_n_swapchain_images;

    curr_frame_signal_semaphore_ptr = app_ptr->m_frame_signal_semaphores[app_ptr->m_n_last_semaphore_used];
    curr_frame_wait_semaphore_ptr   = app_ptr->m_frame_wait_semaphores  [app_ptr->m_n_last_semaphore_used];

    present_wait_semaphore_ptr = curr_frame_signal_semaphore_ptr;

    /* Determine the semaphore which the swapchain image */
    n_swapchain_image = app_ptr->m_swapchain_ptr->acquire_image(curr_frame_wait_semaphore_ptr,
                                                                true); /* in_should_block */

    /* Update time data */
    const float time = float(app_ptr->m_time.get_time_in_msec() ) / 1000.0f;

    app_ptr->m_time_bo_ptr->write(n_swapchain_image * app_ptr->m_time_n_bytes_per_swapchain_image, /* start_offset */
                                  sizeof(time),
                                 &time);

    /* Submit work chunks and present */
    std::shared_ptr<Anvil::CommandBufferBase> cmd_buffers[] =
    {
        app_ptr->m_render_tri1_and_generate_ot_data_cmd_buffers[n_swapchain_image],
        app_ptr->m_render_tri2_and_quad_cmd_buffers            [n_swapchain_image]
    };
    const uint32_t n_cmd_buffers = sizeof(cmd_buffers) / sizeof(cmd_buffers[0]);

    present_queue_ptr->submit_command_buffers(n_cmd_buffers,
                                              cmd_buffers,
                                              1, /* n_semaphores_to_signal */
                                             &curr_frame_signal_semaphore_ptr,
                                              1, /* n_semaphores_to_wait_on */
                                             &curr_frame_wait_semaphore_ptr,
                                             &wait_stage_mask,
                                              false); /* should_block */

    present_queue_ptr->present(app_ptr->m_swapchain_ptr,
                               n_swapchain_image,
                               1, /* n_wait_semaphores */
                              &present_wait_semaphore_ptr);

    ++n_frames_rendered;

    #if defined(ENABLE_OFFSCREEN_RENDERING)
    {
        if (n_frames_rendered >= N_FRAMES_TO_RENDER)
        {
            app_ptr->m_window_ptr->close();
        }
    }
    #endif
}

void App::init()
{
    init_vulkan   ();
    init_window   ();
    init_swapchain();

    init_buffers   ();
    init_dsgs      ();
    init_events    ();
    init_images    ();
    init_query_pool();
    init_semaphores();
    init_shaders   ();

    init_framebuffers   ();
    init_renderpasses   ();
    init_command_buffers();
}

void App::init_buffers()
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_physical_device_ptr);
    const VkDeviceSize                     uniform_alignment_req     (physical_device_locked_ptr->get_device_properties().limits.minUniformBufferOffsetAlignment);

    m_n_bytes_per_query                = static_cast<uint32_t>(Anvil::Utils::round_up(sizeof(uint32_t),
                                                                                      uniform_alignment_req) );
    m_time_n_bytes_per_swapchain_image = static_cast<uint32_t>(Anvil::Utils::round_up(sizeof(float),
                                                                                      uniform_alignment_req) );

    m_query_bo_ptr = Anvil::Buffer::create_nonsparse(
        m_device_ptr,
        m_n_bytes_per_query * N_SWAPCHAIN_IMAGES,
        Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        0,        /* in_memory_features */
        nullptr); /* opt_client_data    */

    m_time_bo_ptr = Anvil::Buffer::create_nonsparse(
        m_device_ptr,
        m_time_n_bytes_per_swapchain_image * N_SWAPCHAIN_IMAGES,
        Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        Anvil::MEMORY_FEATURE_FLAG_MAPPABLE,
        nullptr); /* opt_client_data */
}

void App::init_command_buffers()
{
    VkClearValue                                    clear_values[2];
    std::shared_ptr<Anvil::SGPUDevice>              device_locked_ptr            (m_device_ptr);
    std::shared_ptr<Anvil::GraphicsPipelineManager> gfx_pipeline_manager_ptr     (device_locked_ptr->get_graphics_pipeline_manager() );
    const bool                                      is_ext_debug_marker_supported(device_locked_ptr->is_ext_debug_marker_extension_enabled() );
    VkRect2D                                        render_area;
    std::shared_ptr<Anvil::DescriptorSet>           tri_1stpass_ds0_ptr          (m_1stpass_dsg_ptr->get_descriptor_set     (0) );
    std::shared_ptr<Anvil::DescriptorSet>           quad_2ndpass_ds0_ptr         (m_2ndpass_quad_dsg_ptr->get_descriptor_set(0) );
    std::shared_ptr<Anvil::DescriptorSet>           tri_2ndpass_ds0_ptr          (m_2ndpass_tri_dsg_ptr->get_descriptor_set (0) );

    clear_values[0].color.float32[0]   = 1.0f;
    clear_values[0].color.float32[1]   = 1.0f;
    clear_values[0].color.float32[2]   = 0.0f;
    clear_values[0].color.float32[3]   = 1.0f;
    clear_values[1].depthStencil.depth = 1.0f;

    render_area.extent.height = WINDOW_HEIGHT;
    render_area.extent.width  = WINDOW_WIDTH;
    render_area.offset.x      = 0;
    render_area.offset.y      = 0;

    for (uint32_t n_command_buffer = 0;
                  n_command_buffer < N_SWAPCHAIN_IMAGES;
                ++n_command_buffer)
    {
        const uint32_t query_result_offset = m_n_bytes_per_query                * n_command_buffer;
        const uint32_t time_dynamic_offset = m_time_n_bytes_per_swapchain_image * n_command_buffer;

        m_render_tri1_and_generate_ot_data_cmd_buffers[n_command_buffer] = device_locked_ptr->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)->alloc_primary_level_command_buffer();
        m_render_tri2_and_quad_cmd_buffers            [n_command_buffer] = device_locked_ptr->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)->alloc_primary_level_command_buffer();


        std::shared_ptr<Anvil::PrimaryCommandBuffer> render_tri1_and_generate_ot_data_cmd_buffer_ptr = m_render_tri1_and_generate_ot_data_cmd_buffers[n_command_buffer];
        std::shared_ptr<Anvil::PrimaryCommandBuffer> render_tri2_and_quad_cmd_buffer_ptr             = m_render_tri2_and_quad_cmd_buffers            [n_command_buffer];

        render_tri1_and_generate_ot_data_cmd_buffer_ptr->start_recording(false,  /* one_time_submit          */
                                                                         true);  /* simultaneous_use_allowed */
        {
            render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_reset_event(m_query_data_copied_event,
                                                                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

            render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_begin_render_pass(sizeof(clear_values) / sizeof(clear_values[0]),
                                                                                      clear_values,
                                                                                      m_fbos[n_command_buffer],
                                                                                      render_area,
                                                                                      m_renderpass_tris_ptr,
                                                                                      VK_SUBPASS_CONTENTS_INLINE);
            {
                if (is_ext_debug_marker_supported)
                {
                    render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_debug_marker_begin_EXT("Render left triangle",
                                                                                                   nullptr); /* in_opt_color */
                }

                /* Draw the left triangle. */
                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_bind_pipeline       (VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                             m_1stpass_depth_test_always_pipeline_id);
                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                             gfx_pipeline_manager_ptr->get_graphics_pipeline_layout(m_1stpass_depth_test_always_pipeline_id),
                                                                                             0, /* in_first_set */
                                                                                             1, /* in_set_count */
                                                                                            &tri_1stpass_ds0_ptr,
                                                                                             1, /* in_dynamic_offset_count */
                                                                                            &time_dynamic_offset);

                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_draw(3,  /* in_vertex_count   */
                                                                             1,  /* in_instance_count */
                                                                             0,  /* in_first_vertex   */
                                                                             0); /* in_first_instance */

                if (is_ext_debug_marker_supported)
                {
                    render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_debug_marker_end_EXT();
                }

                /* Proceed to the next subpass, where we render the right triangle with depth test configured to EQUAL. 
                 * At the same time, we count how many fragments passed the test. We're going to use that data to
                 * determine the shade of the quad underneath two triangles. */
                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_next_subpass(VK_SUBPASS_CONTENTS_INLINE);

                if (is_ext_debug_marker_supported)
                {
                    render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_debug_marker_begin_EXT("Render an invisible right triangle with occlusion queries enabled",
                                                                                                   nullptr); /* in_opt_color */
                }

                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                      m_1stpass_depth_test_equal_pipeline_id);

                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_begin_query(m_query_pool_ptr,
                                                                                    n_command_buffer,
                                                                                    0); /* in_flags */
                {
                    render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_draw(3,  /* in_vertex_count   */
                                                                                 1,  /* in_instance_count */
                                                                                 0,  /* in_first_vertex   */
                                                                                 1); /* in_first_instance */
                }                                
                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_end_query(m_query_pool_ptr,
                                                                                  n_command_buffer);

                if (is_ext_debug_marker_supported)
                {
                    render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_debug_marker_end_EXT();
                }
            }
            render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_end_render_pass();

            render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_copy_query_pool_results(m_query_pool_ptr,
                                                                                            n_command_buffer,
                                                                                            1, /* in_query_count */
                                                                                            m_query_bo_ptr,
                                                                                            m_n_bytes_per_query * n_command_buffer, /* in_dst_offset */
                                                                                            0,                                      /* in_dst_stride */
                                                                                            VK_QUERY_RESULT_WAIT_BIT);
            render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_set_event              (m_query_data_copied_event,
                                                                                            VK_PIPELINE_STAGE_TRANSFER_BIT);
        }
        render_tri1_and_generate_ot_data_cmd_buffer_ptr->stop_recording();

        render_tri2_and_quad_cmd_buffer_ptr->start_recording(false, /* one_time_submit          */
                                                             true); /* simultaneous_use_allowed */
        {
            /* Wait until query data arrives */
            Anvil::BufferBarrier query_data_barrier(VK_ACCESS_TRANSFER_WRITE_BIT,
                                                    VK_ACCESS_UNIFORM_READ_BIT,
                                                    device_locked_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL),
                                                    device_locked_ptr->get_queue_family_index(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL),
                                                    m_query_bo_ptr,
                                                    query_result_offset,  /* in_offset */
                                                    m_n_bytes_per_query); /* in_size   */

            render_tri2_and_quad_cmd_buffer_ptr->record_wait_events(1, /* in_event_count */
                                                                   &m_query_data_copied_event,
                                                                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                                    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                                                    0,       /* in_memory_barrier_count        */
                                                                    nullptr, /* in_memory_barriers_ptr         */
                                                                    1,       /* in_buffer_memory_barrier_count */
                                                                   &query_data_barrier,
                                                                    0,        /* in_image_memory_barrier_count */
                                                                    nullptr); /* in_image_memory_barriers_ptr  */

            if (is_ext_debug_marker_supported)
            {
                render_tri2_and_quad_cmd_buffer_ptr->record_debug_marker_begin_EXT("Rasterize the right triangle using occlusion query data",
                                                                                   nullptr); /* in_opt_color_ptr */
            }

            /* Rasterize the right triangle */
            render_tri2_and_quad_cmd_buffer_ptr->record_begin_render_pass(0,       /* in_n_clear_values   */
                                                                          nullptr, /* in_clear_value_ptrs */
                                                                          m_fbos[n_command_buffer],
                                                                          render_area,
                                                                          m_renderpass_quad_ptr,
                                                                          VK_SUBPASS_CONTENTS_INLINE);
            {
                render_tri2_and_quad_cmd_buffer_ptr->record_bind_pipeline       (VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                 m_2ndpass_depth_test_off_tri_pipeline_id);
                render_tri2_and_quad_cmd_buffer_ptr->record_bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                 gfx_pipeline_manager_ptr->get_graphics_pipeline_layout(m_2ndpass_depth_test_off_tri_pipeline_id),
                                                                                 0, /* in_first_set */
                                                                                 1, /* in_set_count */
                                                                                &tri_2ndpass_ds0_ptr,
                                                                                 1, /* in_dynamic_offset_count */
                                                                                &time_dynamic_offset);

                render_tri2_and_quad_cmd_buffer_ptr->record_draw(3,  /* in_vertex_count   */
                                                                 1,  /* in_instance_count */
                                                                 0,  /* in_first_vertex   */
                                                                 1); /* in_first_instance */

                /* Draw the quad */
                render_tri2_and_quad_cmd_buffer_ptr->record_next_subpass(VK_SUBPASS_CONTENTS_INLINE);

                render_tri2_and_quad_cmd_buffer_ptr->record_bind_pipeline       (VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                 m_2ndpass_depth_test_off_quad_pipeline_id);
                render_tri2_and_quad_cmd_buffer_ptr->record_bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                 gfx_pipeline_manager_ptr->get_graphics_pipeline_layout(m_2ndpass_depth_test_off_quad_pipeline_id),
                                                                                 0, /* in_first_set */
                                                                                 1, /* in_set_count */
                                                                                &quad_2ndpass_ds0_ptr,
                                                                                 1, /* in_dynamic_offset_count */
                                                                                &query_result_offset);

                render_tri2_and_quad_cmd_buffer_ptr->record_draw(4,  /* in_vertex_count   */
                                                                 1,  /* in_instance_count */
                                                                 0,  /* in_first_vertex   */
                                                                 0); /* in_first_instance */
            }
            render_tri2_and_quad_cmd_buffer_ptr->record_end_render_pass();

            if (is_ext_debug_marker_supported)
            {
                render_tri2_and_quad_cmd_buffer_ptr->record_debug_marker_end_EXT();
            }
        }
        render_tri2_and_quad_cmd_buffer_ptr->stop_recording();
    }
}

void App::init_dsgs()
{
    m_2ndpass_quad_dsg_ptr = Anvil::DescriptorSetGroup::create(m_device_ptr,
                                                               false, /* releaseable_sets */
                                                               1);    /* n_sets           */
    m_2ndpass_tri_dsg_ptr  = Anvil::DescriptorSetGroup::create(m_device_ptr,
                                                               false, /* releaseable_sets */
                                                               1);    /* n_sets           */
    m_1stpass_dsg_ptr      = Anvil::DescriptorSetGroup::create(m_device_ptr,
                                                               false, /* releaseable_sets */
                                                               1);    /* n_sets           */

    m_1stpass_dsg_ptr->add_binding     (0, /* n_set   */
                                        0, /* binding */
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                        1, /* n_elements */
                                        VK_SHADER_STAGE_VERTEX_BIT);
    m_1stpass_dsg_ptr->set_binding_item(0, /* n_set         */
                                        0, /* binding_index */
                                        Anvil::DescriptorSet::DynamicUniformBufferBindingElement(m_time_bo_ptr,
                                                                                                 0, /* in_start_offset */
                                                                                                 m_time_n_bytes_per_swapchain_image) );

    m_2ndpass_tri_dsg_ptr->add_binding     (0, /* n_set   */
                                            0, /* binding */
                                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                            1, /* n_elements */
                                            VK_SHADER_STAGE_VERTEX_BIT);
    m_2ndpass_tri_dsg_ptr->set_binding_item(0, /* n_set         */
                                            0, /* binding_index */
                                            Anvil::DescriptorSet::DynamicUniformBufferBindingElement(m_time_bo_ptr,
                                                                                                     0, /* in_start_offset */
                                                                                                     m_time_n_bytes_per_swapchain_image) );

    m_2ndpass_quad_dsg_ptr->add_binding     (0, /* n_set   */
                                             0, /* binding */
                                             VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                             1, /* n_elements */
                                             VK_SHADER_STAGE_VERTEX_BIT);
    m_2ndpass_quad_dsg_ptr->set_binding_item(0, /* n_set         */
                                             0, /* binding_index */
                                             Anvil::DescriptorSet::DynamicUniformBufferBindingElement(m_query_bo_ptr,
                                                                                                      0, /* in_start_offset */
                                                                                                      m_n_bytes_per_query));
}

void App::init_events()
{
    m_query_data_copied_event = Anvil::Event::create(m_device_ptr);

    m_query_data_copied_event->set_name("Query data copied event");
}

void App::init_framebuffers()
{
    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < N_SWAPCHAIN_IMAGES;
                ++n_swapchain_image)
    {
        m_fbos[n_swapchain_image] = Anvil::Framebuffer::create(m_device_ptr,
                                                               WINDOW_WIDTH,
                                                               WINDOW_HEIGHT,
                                                               1); /* n_layers */

        m_fbos[n_swapchain_image]->set_name_formatted("Framebuffer for swapchain image [%d]",
                                                      n_swapchain_image);

        m_fbos[n_swapchain_image]->add_attachment(m_swapchain_ptr->get_image_view(n_swapchain_image),
                                                  nullptr); /* out_opt_attachment_id_ptr */
        m_fbos[n_swapchain_image]->add_attachment(m_depth_image_view_ptr,
                                                  nullptr); /* out_opt_attachment_id_ptr */
    }
}

void App::init_images()
{
    m_depth_image_ptr = Anvil::Image::create_nonsparse(
        m_device_ptr,
        VK_IMAGE_TYPE_2D,
        VK_FORMAT_D16_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        1, /* base_mipmap_depth */
        1, /* n_layers          */
        VK_SAMPLE_COUNT_1_BIT,
        Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        false, /* use_full_mipmap_chain */
        0,     /* in_memory_features    */
        false, /* is_mutable            */
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        nullptr);

    m_depth_image_view_ptr = Anvil::ImageView::create_2D(
        m_device_ptr,
        m_depth_image_ptr,
        0, /* n_base_layer        */
        0, /* n_base_mipmap_level */
        1, /* n_mipmaps           */
        VK_IMAGE_ASPECT_DEPTH_BIT,
        m_depth_image_ptr->get_image_format(),
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY);
}

void App::init_query_pool()
{
    m_query_pool_ptr = Anvil::QueryPool::create_non_ps_query_pool(m_device_ptr,
                                                                  VK_QUERY_TYPE_OCCLUSION,
                                                                  N_SWAPCHAIN_IMAGES);
}

void App::init_renderpasses()
{
    std::shared_ptr<Anvil::SGPUDevice>              device_locked_ptr       (m_device_ptr);
    std::shared_ptr<Anvil::GraphicsPipelineManager> gfx_pipeline_manager_ptr(device_locked_ptr->get_graphics_pipeline_manager() );


    for (uint32_t n_renderpass = 0;
                  n_renderpass < 2; /* quad, tri */
                ++n_renderpass)
    {
        Anvil::RenderPassAttachmentID      color_attachment_id;
        Anvil::RenderPassAttachmentID      ds_attachment_id;
        const bool                         is_2nd_renderpass = (n_renderpass == 1);
        std::shared_ptr<Anvil::RenderPass> renderpass_ptr;
        
        renderpass_ptr = Anvil::RenderPass::create(m_device_ptr,
                                                   m_swapchain_ptr);

        renderpass_ptr->set_name( (n_renderpass == 0) ? "Quad renderpass"
                                                      : "Triangle renderpass");

        renderpass_ptr->add_color_attachment(m_swapchain_ptr->get_image_format(),
                                             VK_SAMPLE_COUNT_1_BIT,
                                             (!is_2nd_renderpass) ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                                                  : VK_ATTACHMENT_LOAD_OP_LOAD,
                                             VK_ATTACHMENT_STORE_OP_STORE,
                                             VK_IMAGE_LAYOUT_UNDEFINED,
#ifdef ENABLE_OFFSCREEN_RENDERING
                                             VK_IMAGE_LAYOUT_GENERAL,
#else
                                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
#endif
                                             false, /* may_alias */
                                            &color_attachment_id);

        renderpass_ptr->add_depth_stencil_attachment(m_depth_image_ptr->get_image_format(),
                                                     VK_SAMPLE_COUNT_1_BIT,
                                                     (!is_2nd_renderpass) ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                                                          : VK_ATTACHMENT_LOAD_OP_LOAD,
                                                     VK_ATTACHMENT_STORE_OP_STORE,
                                                     VK_ATTACHMENT_LOAD_OP_DONT_CARE,  /* stencil_load_op  */
                                                     VK_ATTACHMENT_STORE_OP_DONT_CARE, /* stencil_store_op */
                                                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                     false, /* may_alias */
                                                     &ds_attachment_id);

        if (is_2nd_renderpass)
        {
            renderpass_ptr->add_subpass(*m_tri_fs_ptr,
                                        Anvil::ShaderModuleStageEntryPoint(), /* geometry_shader_entrypoint        */
                                        Anvil::ShaderModuleStageEntryPoint(), /* tess_control_shader_entrypoint    */
                                        Anvil::ShaderModuleStageEntryPoint(), /* tess_evaluation_shader_entrypoint */
                                        *m_tri_vs_ptr,
                                       &m_renderpass_2ndpass_depth_test_off_tri_subpass_id);
            renderpass_ptr->add_subpass(*m_quad_fs_ptr,
                                        Anvil::ShaderModuleStageEntryPoint(), /* geometry_shader_entrypoint        */
                                        Anvil::ShaderModuleStageEntryPoint(), /* tess_control_shader_entrypoint    */
                                        Anvil::ShaderModuleStageEntryPoint(), /* tess_evaluation_shader_entrypoint */
                                        *m_quad_vs_ptr,
                                       &m_renderpass_2ndpass_depth_test_off_quad_subpass_id);


            renderpass_ptr->add_subpass_color_attachment(m_renderpass_2ndpass_depth_test_off_tri_subpass_id,
                                                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                         color_attachment_id,
                                                         0, /* location */
                                                         nullptr); /* opt_attachment_resolve_ptr */
            renderpass_ptr->add_subpass_color_attachment(m_renderpass_2ndpass_depth_test_off_quad_subpass_id,
                                                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                         color_attachment_id,
                                                         0, /* location */
                                                         nullptr); /* opt_attachment_resolve_ptr */


            renderpass_ptr->get_subpass_graphics_pipeline_id(m_renderpass_2ndpass_depth_test_off_tri_subpass_id,
                                                            &m_2ndpass_depth_test_off_tri_pipeline_id);
            renderpass_ptr->get_subpass_graphics_pipeline_id(m_renderpass_2ndpass_depth_test_off_quad_subpass_id,
                                                            &m_2ndpass_depth_test_off_quad_pipeline_id);


            gfx_pipeline_manager_ptr->set_pipeline_dsg(m_2ndpass_depth_test_off_tri_pipeline_id,
                                                       m_2ndpass_tri_dsg_ptr);

            gfx_pipeline_manager_ptr->set_pipeline_dsg             (m_2ndpass_depth_test_off_quad_pipeline_id,
                                                                    m_2ndpass_quad_dsg_ptr);
            gfx_pipeline_manager_ptr->set_input_assembly_properties(m_2ndpass_depth_test_off_quad_pipeline_id,
                                                                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

            /* Vulkan requires applications to synchronize subpass execution via subpass dependencies.
             * Even though technically we would be fine with both subpasses being executed in parallel,
             * as the rasterized geometry never overlaps, we have to define a dependency .. */
            renderpass_ptr->add_subpass_to_subpass_dependency(m_renderpass_2ndpass_depth_test_off_tri_subpass_id,
                                                              m_renderpass_2ndpass_depth_test_off_quad_subpass_id,
                                                              VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,  /* source_stage_mask      */
                                                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,    /* destination_stage_mask */
                                                              0,                                    /* source_access_mask      */
                                                              0,                                    /* destination_access_mask */
                                                              VK_TRUE);                             /* by_region               */

            m_renderpass_quad_ptr = renderpass_ptr;
        }
        else
        {
            renderpass_ptr->add_subpass(*m_tri_fs_ptr,
                                        Anvil::ShaderModuleStageEntryPoint(), /* geometry_shader_entrypoint        */
                                        Anvil::ShaderModuleStageEntryPoint(), /* tess_control_shader_entrypoint    */
                                        Anvil::ShaderModuleStageEntryPoint(), /* tess_evaluation_shader_entrypoint */
                                        *m_tri_vs_ptr,
                                       &m_renderpass_1stpass_depth_test_always_subpass_id);
            renderpass_ptr->add_subpass(*m_tri_fs_ptr,
                                        Anvil::ShaderModuleStageEntryPoint(), /* geometry_shader_entrypoint        */
                                        Anvil::ShaderModuleStageEntryPoint(), /* tess_control_shader_entrypoint    */
                                        Anvil::ShaderModuleStageEntryPoint(), /* tess_evaluation_shader_entrypoint */
                                        *m_tri_vs_ptr,
                                       &m_renderpass_1stpass_depth_test_equal_ot_subpass_id);


            renderpass_ptr->add_subpass_color_attachment(m_renderpass_1stpass_depth_test_always_subpass_id,
                                                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                         color_attachment_id,
                                                         0, /* location */
                                                         nullptr); /* opt_attachment_resolve_ptr */
            renderpass_ptr->add_subpass_color_attachment(m_renderpass_1stpass_depth_test_equal_ot_subpass_id,
                                                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                         color_attachment_id,
                                                         0, /* location */
                                                         nullptr); /* opt_attachment_resolve_ptr */

            renderpass_ptr->add_subpass_depth_stencil_attachment(m_renderpass_1stpass_depth_test_always_subpass_id,
                                                                 ds_attachment_id,
                                                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            renderpass_ptr->add_subpass_depth_stencil_attachment(m_renderpass_1stpass_depth_test_equal_ot_subpass_id,
                                                                 ds_attachment_id,
                                                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


            renderpass_ptr->get_subpass_graphics_pipeline_id(m_renderpass_1stpass_depth_test_always_subpass_id,
                                                            &m_1stpass_depth_test_always_pipeline_id);
            renderpass_ptr->get_subpass_graphics_pipeline_id(m_renderpass_1stpass_depth_test_equal_ot_subpass_id,
                                                            &m_1stpass_depth_test_equal_pipeline_id);

            gfx_pipeline_manager_ptr->set_pipeline_dsg   (m_1stpass_depth_test_always_pipeline_id,
                                                          m_1stpass_dsg_ptr);
            gfx_pipeline_manager_ptr->toggle_depth_test  (m_1stpass_depth_test_always_pipeline_id,
                                                          true, /* should_enable */
                                                          VK_COMPARE_OP_ALWAYS);
            gfx_pipeline_manager_ptr->toggle_depth_writes(m_1stpass_depth_test_always_pipeline_id,
                                                          true); /* should_enable */

            gfx_pipeline_manager_ptr->set_pipeline_dsg   (m_1stpass_depth_test_equal_pipeline_id,
                                                          m_1stpass_dsg_ptr);
            gfx_pipeline_manager_ptr->toggle_depth_test  (m_1stpass_depth_test_equal_pipeline_id,
                                                          true, /* should_enable */
                                                          VK_COMPARE_OP_EQUAL);
            gfx_pipeline_manager_ptr->toggle_depth_writes(m_1stpass_depth_test_equal_pipeline_id,
                                                          true); /* should_enable */

            /* depth_test_equal_ot subpass must not start before depth_test_always subpass finishes rasterizing */
            renderpass_ptr->add_subpass_to_subpass_dependency(m_renderpass_1stpass_depth_test_always_subpass_id,
                                                              m_renderpass_1stpass_depth_test_equal_ot_subpass_id,
                                                              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,  /* source_stage_mask      */
                                                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, /* destination_stage_mask */
                                                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                                                              VK_TRUE); /* by_region */

            m_renderpass_tris_ptr = renderpass_ptr;
        }
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

        new_signal_semaphore_ptr->set_name_formatted("Frame signal semaphore [%d]",
                                                     n_semaphore);
        new_wait_semaphore_ptr->set_name_formatted   ("Frame wait semaphore [%d]",
                                                     n_semaphore);

        m_frame_signal_semaphores.push_back(new_signal_semaphore_ptr);
        m_frame_wait_semaphores.push_back  (new_wait_semaphore_ptr);
    }
}

void App::init_shaders()
{
    std::shared_ptr<Anvil::GLSLShaderToSPIRVGenerator> fs_quad_glsl_ptr;
    std::shared_ptr<Anvil::ShaderModule>               fs_quad_module_ptr;
    std::shared_ptr<Anvil::GLSLShaderToSPIRVGenerator> fs_tri_glsl_ptr;
    std::shared_ptr<Anvil::ShaderModule>               fs_tri_module_ptr;
    std::shared_ptr<Anvil::GLSLShaderToSPIRVGenerator> vs_quad_glsl_ptr;
    std::shared_ptr<Anvil::ShaderModule>               vs_quad_module_ptr;
    std::shared_ptr<Anvil::GLSLShaderToSPIRVGenerator> vs_tri_glsl_ptr;
    std::shared_ptr<Anvil::ShaderModule>               vs_tri_module_ptr;

    fs_quad_glsl_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr,
                                                                 Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                 g_glsl_render_quad_frag,
                                                                 Anvil::SHADER_STAGE_FRAGMENT);
    fs_tri_glsl_ptr  = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr,
                                                                 Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                 g_glsl_render_tri_frag,
                                                                 Anvil::SHADER_STAGE_FRAGMENT);
    vs_quad_glsl_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr,
                                                                 Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                 g_glsl_render_quad_vert,
                                                                 Anvil::SHADER_STAGE_VERTEX);
    vs_tri_glsl_ptr  = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr,
                                                                 Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                 g_glsl_render_tri_vert,
                                                                 Anvil::SHADER_STAGE_VERTEX);

    vs_quad_glsl_ptr->add_definition_value_pair("N_SWAPCHAIN_IMAGES",
                                                N_SWAPCHAIN_IMAGES);


    fs_quad_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr,
                                                                          fs_quad_glsl_ptr);
    fs_tri_module_ptr  = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr,
                                                                          fs_tri_glsl_ptr);
    vs_quad_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr,
                                                                          vs_quad_glsl_ptr);
    vs_tri_module_ptr  = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr,
                                                                          vs_tri_glsl_ptr);

    fs_quad_module_ptr->set_name("Quad fragment shader module");
    fs_tri_module_ptr->set_name ("Triangle fragment shader module");
    vs_quad_module_ptr->set_name("Quad vertex shader module");
    vs_tri_module_ptr->set_name ("Triangle vertex shader module");
                       

    m_quad_fs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            fs_quad_module_ptr,
            Anvil::SHADER_STAGE_FRAGMENT)
    );
    m_quad_vs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            vs_quad_module_ptr,
            Anvil::SHADER_STAGE_VERTEX)
    );
    m_tri_fs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            fs_tri_module_ptr,
            Anvil::SHADER_STAGE_FRAGMENT)
    );
    m_tri_vs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            vs_tri_module_ptr,
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
                                             std::vector<std::string>(), /* layers */
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

