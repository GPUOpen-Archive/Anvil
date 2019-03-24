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
#include "misc/buffer_create_info.h"
#include "misc/event_create_info.h"
#include "misc/framebuffer_create_info.h"
#include "misc/glsl_to_spirv.h"
#include "misc/graphics_pipeline_create_info.h"
#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/instance_create_info.h"
#include "misc/io.h"
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
    :m_1stpass_depth_test_always_pipeline_id  (UINT32_MAX),
     m_1stpass_depth_test_equal_pipeline_id   (UINT32_MAX),
     m_2ndpass_depth_test_off_quad_pipeline_id(UINT32_MAX),
     m_2ndpass_depth_test_off_tri_pipeline_id (UINT32_MAX),
     m_n_last_semaphore_used                  (0),
     m_n_swapchain_images                     (N_SWAPCHAIN_IMAGES)
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

    if (m_1stpass_depth_test_always_pipeline_id != UINT32_MAX)
    {
        gfx_pipeline_manager_ptr->delete_pipeline(m_1stpass_depth_test_always_pipeline_id);

        m_1stpass_depth_test_always_pipeline_id = UINT32_MAX;
    }

    if (m_1stpass_depth_test_equal_pipeline_id != UINT32_MAX)
    {
        gfx_pipeline_manager_ptr->delete_pipeline(m_1stpass_depth_test_equal_pipeline_id);

        m_1stpass_depth_test_equal_pipeline_id = UINT32_MAX;
    }

    if (m_2ndpass_depth_test_off_quad_pipeline_id != UINT32_MAX)
    {
        gfx_pipeline_manager_ptr->delete_pipeline(m_2ndpass_depth_test_off_quad_pipeline_id);

        m_2ndpass_depth_test_off_quad_pipeline_id = UINT32_MAX;
    }

    if (m_2ndpass_depth_test_off_tri_pipeline_id != UINT32_MAX)
    {
        gfx_pipeline_manager_ptr->delete_pipeline(m_2ndpass_depth_test_off_tri_pipeline_id);

        m_2ndpass_depth_test_off_tri_pipeline_id = UINT32_MAX;
    }

    m_frame_signal_semaphores.clear();
    m_frame_wait_semaphores.clear();

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
                                                                   true /* in_should_block */);

        ANVIL_REDUNDANT_VARIABLE_CONST(acquire_result);
        anvil_assert                  (acquire_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }

    /* Update time data */
    const float time = float(m_time.get_time_in_msec() ) / 1000.0f;

    m_time_bo_ptr->write(n_swapchain_image * m_time_n_bytes_per_swapchain_image, /* start_offset */
                         sizeof(time),
                        &time);

    /* Submit work chunks and present */
    Anvil::CommandBufferBase* cmd_buffers[] =
    {
        m_render_tri1_and_generate_ot_data_cmd_buffers[n_swapchain_image].get(),
        m_render_tri2_and_quad_cmd_buffers            [n_swapchain_image].get()
    };
    const uint32_t n_cmd_buffers = sizeof(cmd_buffers) / sizeof(cmd_buffers[0]);

    present_queue_ptr->submit(
        Anvil::SubmitInfo::create_wait_execute_signal(cmd_buffers,
                                                      n_cmd_buffers,
                                                      1, /* n_semaphores_to_signal */
                                                     &curr_frame_signal_semaphore_ptr,
                                                      1, /* n_semaphores_to_wait_on */
                                                     &curr_frame_wait_semaphore_ptr,
                                                     &wait_stage_mask,
                                                      false) /* should_block */
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
    const VkDeviceSize uniform_alignment_req(m_physical_device_ptr->get_device_properties().core_vk1_0_properties_ptr->limits.min_uniform_buffer_offset_alignment);

    m_n_bytes_per_query                = static_cast<uint32_t>(Anvil::Utils::round_up(static_cast<VkDeviceSize>(sizeof(uint32_t)),
                                                                                      uniform_alignment_req) );
    m_time_n_bytes_per_swapchain_image = static_cast<uint32_t>(Anvil::Utils::round_up(static_cast<VkDeviceSize>(sizeof(float)),
                                                                                      uniform_alignment_req) );

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_alloc(m_device_ptr.get(),
                                                                     m_n_bytes_per_query * N_SWAPCHAIN_IMAGES,
                                                                     Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                     Anvil::SharingMode::EXCLUSIVE,
                                                                     Anvil::BufferCreateFlagBits::NONE,
                                                                     Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                                     Anvil::MemoryFeatureFlagBits::NONE); /* in_memory_features */

        m_query_bo_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_alloc(m_device_ptr.get(),
                                                                     m_time_n_bytes_per_swapchain_image * N_SWAPCHAIN_IMAGES,
                                                                     Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                     Anvil::SharingMode::EXCLUSIVE,
                                                                     Anvil::BufferCreateFlagBits::NONE,
                                                                     Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
                                                                     Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT);

        m_time_bo_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }
}

void App::init_command_buffers()
{
    VkClearValue   clear_values[2];
    auto           gfx_pipeline_manager_ptr     (m_device_ptr->get_graphics_pipeline_manager() );
    const bool     is_ext_debug_marker_supported(m_device_ptr->get_extension_info()->ext_debug_marker() );
    VkRect2D       render_area;
    auto           tri_1stpass_ds0_ptr          (m_1stpass_dsg_ptr->get_descriptor_set     (0) );
    auto           quad_2ndpass_ds0_ptr         (m_2ndpass_quad_dsg_ptr->get_descriptor_set(0) );
    auto           tri_2ndpass_ds0_ptr          (m_2ndpass_tri_dsg_ptr->get_descriptor_set (0) );

    uint32_t        n_universal_queues_available   = 0;
    const uint32_t* universal_queue_family_indices = nullptr;

    m_device_ptr->get_queue_family_indices_for_queue_family_type(Anvil::QueueFamilyType::UNIVERSAL,
                                                                &n_universal_queues_available,
                                                                &universal_queue_family_indices);

    anvil_assert(n_universal_queues_available > 0);

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

        m_render_tri1_and_generate_ot_data_cmd_buffers[n_command_buffer] = m_device_ptr->get_command_pool_for_queue_family_index(universal_queue_family_indices[0])->alloc_primary_level_command_buffer();
        m_render_tri2_and_quad_cmd_buffers            [n_command_buffer] = m_device_ptr->get_command_pool_for_queue_family_index(universal_queue_family_indices[0])->alloc_primary_level_command_buffer();

        Anvil::PrimaryCommandBuffer* render_tri1_and_generate_ot_data_cmd_buffer_ptr = m_render_tri1_and_generate_ot_data_cmd_buffers[n_command_buffer].get();
        Anvil::PrimaryCommandBuffer* render_tri2_and_quad_cmd_buffer_ptr             = m_render_tri2_and_quad_cmd_buffers            [n_command_buffer].get();

        render_tri1_and_generate_ot_data_cmd_buffer_ptr->start_recording(false,  /* one_time_submit          */
                                                                         true);  /* simultaneous_use_allowed */
        {
            render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_reset_event(m_query_data_copied_event.get(),
                                                                                Anvil::PipelineStageFlagBits::TOP_OF_PIPE_BIT);

            render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_begin_render_pass(sizeof(clear_values) / sizeof(clear_values[0]),
                                                                                      clear_values,
                                                                                      m_fbos[n_command_buffer].get(),
                                                                                      render_area,
                                                                                      m_renderpass_tris_ptr.get(),
                                                                                      Anvil::SubpassContents::INLINE);
            {
                if (is_ext_debug_marker_supported)
                {
                    render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_debug_marker_begin_EXT("Render left triangle",
                                                                                                   nullptr); /* in_opt_color */
                }

                /* Draw the left triangle. */
                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_bind_pipeline       (Anvil::PipelineBindPoint::GRAPHICS,
                                                                                             m_1stpass_depth_test_always_pipeline_id);
                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                                                             gfx_pipeline_manager_ptr->get_pipeline_layout(m_1stpass_depth_test_always_pipeline_id),
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
                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_next_subpass(Anvil::SubpassContents::INLINE);

                if (is_ext_debug_marker_supported)
                {
                    render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_debug_marker_begin_EXT("Render an invisible right triangle with occlusion queries enabled",
                                                                                                   nullptr); /* in_opt_color */
                }

                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_bind_pipeline(Anvil::PipelineBindPoint::GRAPHICS,
                                                                                      m_1stpass_depth_test_equal_pipeline_id);

                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_begin_query(m_query_pool_ptr.get(),
                                                                                    n_command_buffer,
                                                                                    Anvil::QueryControlFlagBits::NONE); /* in_flags */
                {
                    render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_draw(3,  /* in_vertex_count   */
                                                                                 1,  /* in_instance_count */
                                                                                 0,  /* in_first_vertex   */
                                                                                 1); /* in_first_instance */
                }                                
                render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_end_query(m_query_pool_ptr.get(),
                                                                                  n_command_buffer);

                if (is_ext_debug_marker_supported)
                {
                    render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_debug_marker_end_EXT();
                }
            }
            render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_end_render_pass();

            render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_copy_query_pool_results(m_query_pool_ptr.get(),
                                                                                            n_command_buffer,
                                                                                            1, /* in_query_count */
                                                                                            m_query_bo_ptr.get(),
                                                                                            m_n_bytes_per_query * n_command_buffer, /* in_dst_offset */
                                                                                            0,                                      /* in_dst_stride */
                                                                                            VK_QUERY_RESULT_WAIT_BIT);
            render_tri1_and_generate_ot_data_cmd_buffer_ptr->record_set_event              (m_query_data_copied_event.get(),
                                                                                            Anvil::PipelineStageFlagBits::TRANSFER_BIT);
        }
        render_tri1_and_generate_ot_data_cmd_buffer_ptr->stop_recording();

        render_tri2_and_quad_cmd_buffer_ptr->start_recording(false, /* one_time_submit          */
                                                             true); /* simultaneous_use_allowed */
        {
            /* Wait until query data arrives */
            Anvil::BufferBarrier query_data_barrier             (Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,
                                                                 Anvil::AccessFlagBits::UNIFORM_READ_BIT,
                                                                 universal_queue_family_indices[0],
                                                                 universal_queue_family_indices[0],
                                                                 m_query_bo_ptr.get(),
                                                                 query_result_offset,  /* in_offset */
                                                                 m_n_bytes_per_query); /* in_size   */
            auto                 query_data_copied_event_raw_ptr(m_query_data_copied_event.get() );

            render_tri2_and_quad_cmd_buffer_ptr->record_wait_events(1, /* in_event_count */
                                                                   &query_data_copied_event_raw_ptr,
                                                                    Anvil::PipelineStageFlagBits::TRANSFER_BIT,
                                                                    Anvil::PipelineStageFlagBits::VERTEX_SHADER_BIT,
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
                                                                          m_fbos[n_command_buffer].get(),
                                                                          render_area,
                                                                          m_renderpass_quad_ptr.get(),
                                                                          Anvil::SubpassContents::INLINE);
            {
                render_tri2_and_quad_cmd_buffer_ptr->record_bind_pipeline       (Anvil::PipelineBindPoint::GRAPHICS,
                                                                                 m_2ndpass_depth_test_off_tri_pipeline_id);
                render_tri2_and_quad_cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                                                 gfx_pipeline_manager_ptr->get_pipeline_layout(m_2ndpass_depth_test_off_tri_pipeline_id),
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
                render_tri2_and_quad_cmd_buffer_ptr->record_next_subpass(Anvil::SubpassContents::INLINE);

                render_tri2_and_quad_cmd_buffer_ptr->record_bind_pipeline       (Anvil::PipelineBindPoint::GRAPHICS,
                                                                                 m_2ndpass_depth_test_off_quad_pipeline_id);
                render_tri2_and_quad_cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                                                 gfx_pipeline_manager_ptr->get_pipeline_layout(m_2ndpass_depth_test_off_quad_pipeline_id),
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
    std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> dsg_1stpass_create_info_ptrs     (1);
    std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> quad_dsg_2ndpass_create_info_ptrs(1);
    std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> tri_dsg_2ndpass_create_info_ptrs (1);

    dsg_1stpass_create_info_ptrs     [0] = Anvil::DescriptorSetCreateInfo::create();
    quad_dsg_2ndpass_create_info_ptrs[0] = Anvil::DescriptorSetCreateInfo::create();
    tri_dsg_2ndpass_create_info_ptrs [0] = Anvil::DescriptorSetCreateInfo::create();


    dsg_1stpass_create_info_ptrs[0]->add_binding     (0, /* binding */
                                                      Anvil::DescriptorType::UNIFORM_BUFFER_DYNAMIC,
                                                      1, /* n_elements */
                                                      Anvil::ShaderStageFlagBits::VERTEX_BIT);
    tri_dsg_2ndpass_create_info_ptrs[0]->add_binding (0, /* binding */
                                                      Anvil::DescriptorType::UNIFORM_BUFFER_DYNAMIC,
                                                      1, /* n_elements */
                                                      Anvil::ShaderStageFlagBits::VERTEX_BIT);
    quad_dsg_2ndpass_create_info_ptrs[0]->add_binding(0, /* binding */
                                                      Anvil::DescriptorType::UNIFORM_BUFFER_DYNAMIC,
                                                      1, /* n_elements */
                                                      Anvil::ShaderStageFlagBits::VERTEX_BIT);

    m_1stpass_dsg_ptr      = Anvil::DescriptorSetGroup::create(m_device_ptr.get(),
                                                               dsg_1stpass_create_info_ptrs,
                                                               false); /* in_releaseable_sets */
    m_2ndpass_tri_dsg_ptr  = Anvil::DescriptorSetGroup::create(m_device_ptr.get(),
                                                               tri_dsg_2ndpass_create_info_ptrs,
                                                               false); /* in_releaseable_sets */
    m_2ndpass_quad_dsg_ptr = Anvil::DescriptorSetGroup::create(m_device_ptr.get(),
                                                               quad_dsg_2ndpass_create_info_ptrs,
                                                               false); /* in_releaseable_sets */

    m_1stpass_dsg_ptr->set_binding_item     (0, /* n_set         */
                                             0, /* binding_index */
                                             Anvil::DescriptorSet::DynamicUniformBufferBindingElement(m_time_bo_ptr.get(),
                                                                                                      0, /* in_start_offset */
                                                                                                      m_time_n_bytes_per_swapchain_image) );
    m_2ndpass_tri_dsg_ptr->set_binding_item (0, /* n_set         */
                                             0, /* binding_index */
                                             Anvil::DescriptorSet::DynamicUniformBufferBindingElement(m_time_bo_ptr.get(),
                                                                                                      0, /* in_start_offset */
                                                                                                      m_time_n_bytes_per_swapchain_image) );
    m_2ndpass_quad_dsg_ptr->set_binding_item(0, /* n_set         */
                                             0, /* binding_index */
                                             Anvil::DescriptorSet::DynamicUniformBufferBindingElement(m_query_bo_ptr.get(),
                                                                                                      0, /* in_start_offset */
                                                                                                      m_n_bytes_per_query));
}

void App::init_events()
{
    auto create_info_ptr = Anvil::EventCreateInfo::create(m_device_ptr.get() );

    m_query_data_copied_event = Anvil::Event::create(std::move(create_info_ptr) );
    m_query_data_copied_event->set_name("Query data copied event");
}

void App::init_framebuffers()
{
    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < N_SWAPCHAIN_IMAGES;
                ++n_swapchain_image)
    {
        {
            auto create_info_ptr = Anvil::FramebufferCreateInfo::create(m_device_ptr.get(),
                                                                        WINDOW_WIDTH,
                                                                        WINDOW_HEIGHT,
                                                                        1); /* n_layers */

            create_info_ptr->add_attachment(m_swapchain_ptr->get_image_view(n_swapchain_image),
                                            nullptr); /* out_opt_attachment_id_ptr */
            create_info_ptr->add_attachment(m_depth_image_view_ptr.get(),
                                            nullptr); /* out_opt_attachment_id_ptr */

            m_fbos[n_swapchain_image] = Anvil::Framebuffer::create(std::move(create_info_ptr) );
        }

        m_fbos[n_swapchain_image]->set_name_formatted("Framebuffer for swapchain image [%d]",
                                                      n_swapchain_image);

    }
}

void App::init_images()
{
    {
        auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(m_device_ptr.get(),
                                                                    Anvil::ImageType::_2D,
                                                                    Anvil::Format::D16_UNORM,
                                                                    Anvil::ImageTiling::OPTIMAL,
                                                                    Anvil::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
                                                                    WINDOW_WIDTH,
                                                                    WINDOW_HEIGHT,
                                                                    1, /* base_mipmap_depth */
                                                                    1, /* n_layers          */
                                                                    Anvil::SampleCountFlagBits::_1_BIT,
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    false,                              /* use_full_mipmap_chain */
                                                                    Anvil::MemoryFeatureFlagBits::NONE, /* in_memory_features    */
                                                                    Anvil::ImageCreateFlagBits::NONE,
                                                                    Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                    nullptr);

        m_depth_image_ptr = Anvil::Image::create(std::move(create_info_ptr) );
    }

    {
        auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(m_device_ptr.get(),
                                                                     m_depth_image_ptr.get(),
                                                                     0, /* n_base_layer        */
                                                                     0, /* n_base_mipmap_level */
                                                                     1, /* n_mipmaps           */
                                                                     Anvil::ImageAspectFlagBits::DEPTH_BIT,
                                                                     m_depth_image_ptr->get_create_info_ptr()->get_format(),
                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                     Anvil::ComponentSwizzle::IDENTITY);

        m_depth_image_view_ptr = Anvil::ImageView::create(std::move(create_info_ptr) );
    }
}

void App::init_query_pool()
{
    m_query_pool_ptr = Anvil::QueryPool::create_non_ps_query_pool(m_device_ptr.get(),
                                                                  VK_QUERY_TYPE_OCCLUSION,
                                                                  N_SWAPCHAIN_IMAGES);
}

void App::init_renderpasses()
{
    auto gfx_pipeline_manager_ptr(m_device_ptr->get_graphics_pipeline_manager() );


    for (uint32_t n_renderpass = 0;
                  n_renderpass < 2; /* quad, tri */
                ++n_renderpass)
    {
        Anvil::RenderPassAttachmentID color_attachment_id;
        Anvil::RenderPassAttachmentID ds_attachment_id;
        const bool                    is_2nd_renderpass = (n_renderpass == 1);
        Anvil::RenderPassUniquePtr    renderpass_ptr;

        {
            Anvil::RenderPassCreateInfoUniquePtr renderpass_info_ptr(
                new Anvil::RenderPassCreateInfo(m_device_ptr.get() )
            );

            renderpass_info_ptr->add_color_attachment(m_swapchain_ptr->get_create_info_ptr()->get_format(),
                                                      Anvil::SampleCountFlagBits::_1_BIT,
                                                      (!is_2nd_renderpass) ? Anvil::AttachmentLoadOp::CLEAR
                                                                           : Anvil::AttachmentLoadOp::LOAD,
                                                      Anvil::AttachmentStoreOp::STORE,
                                                      Anvil::ImageLayout::UNDEFINED,
#ifdef ENABLE_OFFSCREEN_RENDERING
                                                      Anvil::ImageLayout::GENERAL,
#else
                                                      Anvil::ImageLayout::PRESENT_SRC_KHR,
    #endif
                                                      false, /* may_alias */
                                                     &color_attachment_id);

            renderpass_info_ptr->add_depth_stencil_attachment(m_depth_image_ptr->get_create_info_ptr()->get_format(),
                                                              Anvil::SampleCountFlagBits::_1_BIT,
                                                              (!is_2nd_renderpass) ? Anvil::AttachmentLoadOp::CLEAR
                                                                                   : Anvil::AttachmentLoadOp::LOAD,
                                                              Anvil::AttachmentStoreOp::STORE,
                                                              Anvil::AttachmentLoadOp::DONT_CARE,  /* stencil_load_op  */
                                                              Anvil::AttachmentStoreOp::DONT_CARE, /* stencil_store_op */
                                                              Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                              Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                              false, /* may_alias */
                                                              &ds_attachment_id);

            if (is_2nd_renderpass)
            {
                renderpass_info_ptr->add_subpass(&m_renderpass_2ndpass_depth_test_off_tri_subpass_id);
                renderpass_info_ptr->add_subpass(&m_renderpass_2ndpass_depth_test_off_quad_subpass_id);

                renderpass_info_ptr->add_subpass_color_attachment(m_renderpass_2ndpass_depth_test_off_tri_subpass_id,
                                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                  color_attachment_id,
                                                                  0, /* location */
                                                                  nullptr); /* opt_attachment_resolve_ptr */
                renderpass_info_ptr->add_subpass_color_attachment(m_renderpass_2ndpass_depth_test_off_quad_subpass_id,
                                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                  color_attachment_id,
                                                                  0, /* location */
                                                                  nullptr); /* opt_attachment_resolve_ptr */

                /* Vulkan requires applications to synchronize subpass execution via subpass dependencies.
                 * Even though technically we would be fine with both subpasses being executed in parallel,
                 * as the rasterized geometry never overlaps, we have to define a dependency .. */
                renderpass_info_ptr->add_subpass_to_subpass_dependency(m_renderpass_2ndpass_depth_test_off_tri_subpass_id,
                                                                       m_renderpass_2ndpass_depth_test_off_quad_subpass_id,
                                                                       Anvil::PipelineStageFlagBits::VERTEX_SHADER_BIT,  /* in_source_stage_mask       */
                                                                       Anvil::PipelineStageFlagBits::TOP_OF_PIPE_BIT,    /* in_destination_stage_mask  */
                                                                       Anvil::AccessFlagBits::NONE,                      /* in_source_access_mask      */
                                                                       Anvil::AccessFlagBits::NONE,                      /* in_destination_access_mask */
                                                                       Anvil::DependencyFlagBits::BY_REGION_BIT);        /* in_by_region               */
            }
            else
            {
                renderpass_info_ptr->add_subpass(&m_renderpass_1stpass_depth_test_always_subpass_id);
                renderpass_info_ptr->add_subpass(&m_renderpass_1stpass_depth_test_equal_ot_subpass_id);

                renderpass_info_ptr->add_subpass_color_attachment(m_renderpass_1stpass_depth_test_always_subpass_id,
                                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                  color_attachment_id,
                                                                  0, /* location */
                                                                  nullptr); /* opt_attachment_resolve_ptr */
                renderpass_info_ptr->add_subpass_color_attachment(m_renderpass_1stpass_depth_test_equal_ot_subpass_id,
                                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                  color_attachment_id,
                                                                  0, /* location */
                                                                  nullptr); /* opt_attachment_resolve_ptr */

                renderpass_info_ptr->add_subpass_depth_stencil_attachment(m_renderpass_1stpass_depth_test_always_subpass_id,
                                                                          Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                          ds_attachment_id);
                renderpass_info_ptr->add_subpass_depth_stencil_attachment(m_renderpass_1stpass_depth_test_equal_ot_subpass_id,
                                                                          Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                          ds_attachment_id);


                /* depth_test_equal_ot subpass must not start before depth_test_always subpass finishes rasterizing */
                renderpass_info_ptr->add_subpass_to_subpass_dependency(m_renderpass_1stpass_depth_test_always_subpass_id,
                                                                       m_renderpass_1stpass_depth_test_equal_ot_subpass_id,
                                                                       Anvil::PipelineStageFlagBits::LATE_FRAGMENT_TESTS_BIT,  /* in_source_stage_mask      */
                                                                       Anvil::PipelineStageFlagBits::EARLY_FRAGMENT_TESTS_BIT, /* in_destination_stage_mask */
                                                                       Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                                                       Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                                                                       Anvil::DependencyFlagBits::BY_REGION_BIT);
            }

            renderpass_ptr = Anvil::RenderPass::create(std::move(renderpass_info_ptr),
                                                       m_swapchain_ptr.get() );
        }

        renderpass_ptr->set_name( (n_renderpass == 0) ? "Quad renderpass"
                                                      : "Triangle renderpass");

        if (is_2nd_renderpass)
        {
            auto depth_test_off_tri_subpass_gfx_pipeline_create_info_ptr  = Anvil::GraphicsPipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                                                                      renderpass_ptr.get(),
                                                                                                                      m_renderpass_2ndpass_depth_test_off_tri_subpass_id,
                                                                                                                      *m_tri_fs_ptr,
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* in_geometry_shader_entrypoint        */
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* in_tess_control_shader_entrypoint    */
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* in_tess_evaluation_shader_entrypoint */
                                                                                                                      *m_tri_vs_ptr);
            auto depth_test_off_quad_subpass_gfx_pipeline_create_info_ptr = Anvil::GraphicsPipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                                                                      renderpass_ptr.get(),
                                                                                                                      m_renderpass_2ndpass_depth_test_off_quad_subpass_id,
                                                                                                                     *m_quad_fs_ptr,
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* geometry_shader_entrypoint        */
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* tess_control_shader_entrypoint    */
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* tess_evaluation_shader_entrypoint */
                                                                                                                     *m_quad_vs_ptr);



            depth_test_off_tri_subpass_gfx_pipeline_create_info_ptr->set_descriptor_set_create_info (m_2ndpass_tri_dsg_ptr->get_descriptor_set_create_info () );
            depth_test_off_quad_subpass_gfx_pipeline_create_info_ptr->set_descriptor_set_create_info(m_2ndpass_quad_dsg_ptr->get_descriptor_set_create_info() );

            depth_test_off_quad_subpass_gfx_pipeline_create_info_ptr->set_primitive_topology(Anvil::PrimitiveTopology::TRIANGLE_STRIP);


            gfx_pipeline_manager_ptr->add_pipeline(std::move(depth_test_off_quad_subpass_gfx_pipeline_create_info_ptr),
                                                  &m_2ndpass_depth_test_off_quad_pipeline_id);
            gfx_pipeline_manager_ptr->add_pipeline(std::move(depth_test_off_tri_subpass_gfx_pipeline_create_info_ptr),
                                                  &m_2ndpass_depth_test_off_tri_pipeline_id);

            m_renderpass_quad_ptr = std::move(renderpass_ptr);
        }
        else
        {
            auto depth_test_always_subpass_gfx_pipeline_create_info_ptr   = Anvil::GraphicsPipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                                                                      renderpass_ptr.get(),
                                                                                                                      m_renderpass_1stpass_depth_test_always_subpass_id,
                                                                                                                     *m_tri_fs_ptr,
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* geometry_shader_entrypoint        */
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* tess_control_shader_entrypoint    */
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* tess_evaluation_shader_entrypoint */
                                                                                                                     *m_tri_vs_ptr);
            auto depth_test_equal_ot_subpass_gfx_pipeline_create_info_ptr = Anvil::GraphicsPipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                                                                      renderpass_ptr.get(),
                                                                                                                      m_renderpass_1stpass_depth_test_equal_ot_subpass_id,
                                                                                                                     *m_tri_fs_ptr,
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* geometry_shader_entrypoint        */
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* tess_control_shader_entrypoint    */
                                                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* tess_evaluation_shader_entrypoint */
                                                                                                                     *m_tri_vs_ptr);

            depth_test_always_subpass_gfx_pipeline_create_info_ptr->set_descriptor_set_create_info(m_1stpass_dsg_ptr->get_descriptor_set_create_info() );
            depth_test_always_subpass_gfx_pipeline_create_info_ptr->toggle_depth_test             (true, /* in_should_enable */
                                                                                                   Anvil::CompareOp::ALWAYS);
            depth_test_always_subpass_gfx_pipeline_create_info_ptr->toggle_depth_writes           (true); /* in_should_enable */

            depth_test_equal_ot_subpass_gfx_pipeline_create_info_ptr->set_descriptor_set_create_info(m_1stpass_dsg_ptr->get_descriptor_set_create_info() );
            depth_test_equal_ot_subpass_gfx_pipeline_create_info_ptr->toggle_depth_test             (true, /* in_should_enable */
                                                                                                     Anvil::CompareOp::EQUAL);
            depth_test_equal_ot_subpass_gfx_pipeline_create_info_ptr->toggle_depth_writes           (true); /* in_should_enable */


            gfx_pipeline_manager_ptr->add_pipeline(std::move(depth_test_always_subpass_gfx_pipeline_create_info_ptr),
                                                  &m_1stpass_depth_test_always_pipeline_id);
            gfx_pipeline_manager_ptr->add_pipeline(std::move(depth_test_equal_ot_subpass_gfx_pipeline_create_info_ptr),
                                                  &m_1stpass_depth_test_equal_pipeline_id);

            m_renderpass_tris_ptr = std::move(renderpass_ptr);
        }
    }
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

        new_signal_semaphore_ptr->set_name_formatted("Frame signal semaphore [%d]",
                                                     n_semaphore);
        new_wait_semaphore_ptr->set_name_formatted("Frame wait semaphore [%d]",
                                                  n_semaphore);

        m_frame_signal_semaphores.push_back(std::move(new_signal_semaphore_ptr) );
        m_frame_wait_semaphores.push_back  (std::move(new_wait_semaphore_ptr) );
    }
}

void App::init_shaders()
{
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr fs_quad_glsl_ptr;
    Anvil::ShaderModuleUniquePtr               fs_quad_module_ptr;
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr fs_tri_glsl_ptr;
    Anvil::ShaderModuleUniquePtr               fs_tri_module_ptr;
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr vs_quad_glsl_ptr;
    Anvil::ShaderModuleUniquePtr               vs_quad_module_ptr;
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr vs_tri_glsl_ptr;
    Anvil::ShaderModuleUniquePtr               vs_tri_module_ptr;

    fs_quad_glsl_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                                 Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                 g_glsl_render_quad_frag,
                                                                 Anvil::ShaderStage::FRAGMENT);
    fs_tri_glsl_ptr  = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                                 Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                 g_glsl_render_tri_frag,
                                                                 Anvil::ShaderStage::FRAGMENT);
    vs_quad_glsl_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                                 Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                 g_glsl_render_quad_vert,
                                                                 Anvil::ShaderStage::VERTEX);
    vs_tri_glsl_ptr  = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                                 Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                 g_glsl_render_tri_vert,
                                                                 Anvil::ShaderStage::VERTEX);

    vs_quad_glsl_ptr->add_definition_value_pair("N_SWAPCHAIN_IMAGES",
                                                N_SWAPCHAIN_IMAGES);


    fs_quad_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get    (),
                                                                          fs_quad_glsl_ptr.get() );
    fs_tri_module_ptr  = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get    (),
                                                                          fs_tri_glsl_ptr.get () );
    vs_quad_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get    (),
                                                                          vs_quad_glsl_ptr.get() );
    vs_tri_module_ptr  = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get    (),
                                                                          vs_tri_glsl_ptr.get () );

    fs_quad_module_ptr->set_name("Quad fragment shader module");
    fs_tri_module_ptr->set_name ("Triangle fragment shader module");
    vs_quad_module_ptr->set_name("Quad vertex shader module");
    vs_tri_module_ptr->set_name ("Triangle vertex shader module");
                       

    m_quad_fs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint("main",
                                               std::move(fs_quad_module_ptr),
                                               Anvil::ShaderStage::FRAGMENT)
    );
    m_quad_vs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint("main",
                                               std::move(vs_quad_module_ptr),
                                               Anvil::ShaderStage::VERTEX)
    );
    m_tri_fs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint("main",
                                               std::move(fs_tri_module_ptr),
                                               Anvil::ShaderStage::FRAGMENT)
    );
    m_tri_vs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint("main",
                                               std::move(vs_tri_module_ptr),
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


    m_swapchain_ptr = reinterpret_cast<Anvil::SGPUDevice*>(m_device_ptr.get() )->create_swapchain(m_rendering_surface_ptr.get(),
                                                                                                  m_window_ptr.get           (),
                                                                                                  Anvil::Format::B8G8R8A8_UNORM,
                                                                                                  Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,
                                                                                                  Anvil::PresentModeKHR::FIFO_KHR,
                                                                                                  Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
                                                                                                  m_n_swapchain_images);

    m_swapchain_ptr->set_name("Main swapchain");

    /* Cache the queue we are going to use for presentation */
    const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

    if (!m_rendering_surface_ptr->get_queue_families_with_present_support(reinterpret_cast<Anvil::SGPUDevice*>(m_device_ptr.get() )->get_physical_device(),
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
                                                                    true, /* in_enable_shader_module_cache */
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

