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

/* Uncomment the #define below to enable mGPU support.
 *
 * When enabled, one (and only one!) of the USE_xxx_PRESENTATION_MODE also needs to be enabled.
 *
 * When enabled, N_SWAPCHAIN_IMAGES must equal the number of logical devices assigned to the physical device.
 * This is due to simplification in the rendering code of the app and could be improved when needed.
 */
//#define ENABLE_MGPU_SUPPORT

/* If mGPU support is enabled, uncomment one (and only one!) of the following #defines to make the app use the specified
 * present mode.
 *
 * NOTE: Path for LOCAL_MULTI_DEVICE presentation mode is not supported at the moment. If needed, just let me know.
 *
 * NOTE: If the implementation does not support a given presentation mode, you'll get an assertion failure related to missing caps.
 *
 */
//#define USE_LOCAL_AFR_PRESENTATION_MODE
//#define USE_LOCAL_SFR_PRESENTATION_MODE
//#define USE_REMOTE_AFR_PRESENTATION_MODE
//#define USE_SUM_SFR_PRESENTATION_MODE


#if defined (USE_LOCAL_AFR_PRESENTATION_MODE)
    /* Uncomment the #define below to explicitly bind image memory to the created swapchain.
      *
      * Optional.
     */
    // #define ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING

#elif defined (USE_LOCAL_SFR_PRESENTATION_MODE)
    /* No extra knobs available */

#elif defined (USE_REMOTE_AFR_PRESENTATION_MODE)
    /* Uncomment the #define below to explicitly bind image memory to the created swapchain.
      *
      * Optional.
     */
    //#define ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING

#elif defined (USE_SUM_SFR_PRESENTATION_MODE)
     /* Uncomment the #define below to explicitly bind image memory to the created swapchain.
      *
      * Optional.
     */
    // #define ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING

    /* Uncomment the #define below to enable explicit SFR rectangle definitions.
     * Optional. Requires ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING to be defined.
     */
    // #define ENABLE_EXPLICIT_SFR_RECT_DEFINITIONS
#endif



/* Uncomment the #define below to enable off-screen rendering */
// #define ENABLE_OFFSCREEN_RENDERING

/* Uncomment the #define below to enable validation */
// #define ENABLE_VALIDATION


#include <string>
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
#include "misc/rendering_surface_create_info.h"
#include "misc/render_pass_create_info.h"
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
#include "teapot_data.h"
#include "app.h"

/* Sanity checks */
#if defined(ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING) && !defined(ENABLE_MGPU_SUPPORT)
    #error If ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING is enabled, ENABLE_MGPU_SUPPORT must also be defined.
#endif

#if defined(ENABLE_EXPLICIT_SFR_RECT_DEFINITIONS) && !defined(ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING)
    #error If ENABLE_EXPLICIT_SFR_RECT_DEFINITIONS is enabled, ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING must also be defined.
#endif

#if defined(ENABLE_MGPU_SUPPORT)
    #if !defined(USE_LOCAL_AFR_PRESENTATION_MODE) && !defined(USE_LOCAL_SFR_PRESENTATION_MODE) && !defined(USE_REMOTE_AFR_PRESENTATION_MODE) && !defined(USE_SUM_SFR_PRESENTATION_MODE)
        #error One of the USE_*_PRESENTATION_MODE #defines need to be commented out.
    #endif
#endif

#if defined(USE_LOCAL_AFR_PRESENTATION_MODE)
    #if defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
        #error More than one presentation mode #defines enabled.
    #endif

    #if !defined(ENABLE_MGPU_SUPPORT)
        #error If USE_LOCAL_AFR_PRESENTATION_MODE is enabled, ENABLE_MGPU_SUPPORT must also be defined.
    #endif
#endif

#if defined(USE_LOCAL_SFR_PRESENTATION_MODE)
    #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
        #error More than one presentation mode #defines enabled.
    #endif

    #if !defined(ENABLE_MGPU_SUPPORT)
        #error If USE_LOCAL_SFR_PRESENTATION_MODE is enabled, ENABLE_MGPU_SUPPORT must also be defined.
    #endif
#endif

#if defined(USE_REMOTE_AFR_PRESENTATION_MODE)
    #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
        #error More than one presentation mode #defines enabled.
    #endif

    #if !defined(ENABLE_MGPU_SUPPORT)
        #error If USE_REMOTE_AFR_PRESENTATION_MODE is enabled, ENABLE_MGPU_SUPPORT must also be defined.
    #endif
#endif

#if defined(USE_SUM_SFR_PRESENTATION_MODE)
    #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
        #error More than one presentation mode #defines enabled.
    #endif

    #if !defined(ENABLE_MGPU_SUPPORT)
        #error If USE_SUM_SFR_PRESENTATION_MODE is enabled, ENABLE_MGPU_SUPPORT must also be defined.
    #endif
#endif

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

#define MAX_DEPTH     (40)
#define MAX_TEAPOT_X  (12)
#define MAX_TEAPOT_Y  (7)
#define MAX_TEAPOT_Z  (30)
#define MIN_TEAPOT_X  (-12)
#define MIN_TEAPOT_Y  (-7)
#define MIN_TEAPOT_Z  (20)
#define N_TEAPOTS     (10000)
#define U_GRANULARITY (8)
#define WINDOW_WIDTH  (1280)
#define WINDOW_HEIGHT (720)
#define V_GRANULARITY (8)

#define N_TIMESTAMP_DELTAS_PER_AVERAGE_FPS_CALCULATION (100)
#define NSEC_PER_SEC                                   1e+9


static const char* fs_body =
    "#version 430\n"
    "\n"
    "layout(location = 0)      in  float depth;\n"
    "layout(location = 1) flat in  uint  instance_id;\n"
    "layout(location = 0)      out vec4  color;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    color = vec4(fract(12675.0 / float(1 + instance_id)), fract(73472.0 / float(1 + instance_id)), depth, 1.0);\n"
    "}";

static const char* vs_body =
    "#version 430\n"
    "\n"
    "layout(location = 0)      in  vec3  vertex_data;\n"
    "layout(location = 0)      out float depth;\n"
    "layout(location = 1) flat out uint  instance_id;\n"
    "\n"
    "layout(std140, binding = 0) restrict readonly buffer sb\n"
    "{\n"
    "    vec4 pos_xyz_size[N_TEAPOTS];\n"
    "    vec4 rot_xyz     [N_TEAPOTS];\n"
    "} in_data;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    const int   teapot_index = gl_InstanceIndex;\n"
    "    const vec3  pos_xyz      = in_data.pos_xyz_size[teapot_index].xyz;\n"
    "    const vec4  ref_vertex   = vec4(vertex_data.xyz, 1.0);\n"
    "    const vec3  rot_xyz      = in_data.rot_xyz[teapot_index].xyz;\n"
    "    const float size         = in_data.pos_xyz_size[teapot_index].w;\n"
    "    vec3        vertex_rx;\n"
    "    vec3        vertex_rx_ry;\n"
    "    vec3        vertex_rx_ry_rz;\n"
    "\n"
         /* Rotations */
    "    vertex_rx       = mat3(vec3(1.0,            0.0,            0.0),              vec3(0.0,             cos(rot_xyz.x), sin(rot_xyz.x)),   vec3(0.0,            -sin(rot_xyz.x), cos(rot_xyz.x))) * ref_vertex.xyz;\n"
    "    vertex_rx_ry    = mat3(vec3(cos(rot_xyz.y), 0.0,            -sin(rot_xyz.y)),  vec3(0.0,             1.0,            0.0),              vec3(sin(rot_xyz.y), 0.0,             cos(rot_xyz.y))) * vertex_rx;\n"
    "    vertex_rx_ry_rz = mat3(vec3(cos(rot_xyz.z), sin(rot_xyz.z), 0.0),              vec3(-sin(rot_xyz.z), cos(rot_xyz.z), 0.0),              vec3(0.0,            0.0,             1.0))            * vertex_rx_ry;\n"
    "\n"
         /* Perspective projection */
    "    float fov_rad     = 38.0 / 360.0 * 2.0 * 3.14152965; /*radians(38.0); */\n"
    "    float ar          = float(RT_WIDTH) / float(RT_HEIGHT);\n"
    "    float z_near      = 0.1;\n"
    "    float z_far       = float(MAX_DEPTH);\n"
    "\n"
    "    float y_scale = 1.0     / tan(fov_rad / 2.0);\n"
    "    float x_scale = y_scale / ar;\n"
    "\n"
    "    mat4 perspective_matrix = mat4(\n"
    "       vec4(x_scale, 0.0,              0.0,                                0.0),\n"
    "       vec4(0.0,     y_scale,          0.0,                                0.0),\n"
    "       vec4(0.0,     0.0,              z_far          / (z_near - z_far), -1.0),\n"
    "       vec4(0.0,     0.0,              z_far * z_near / (z_near - z_far),  0.0));\n"
    "\n"
    "    vec4 final_vertex = perspective_matrix * vec4(vec3(size) * vertex_rx_ry_rz.xyz + pos_xyz, 1.0);\n"
    "\n"
    "    switch (gl_VertexIndex % 3)\n"
    "    {\n"
    "       case 0: depth = 0.0; break;\n"
    "       case 1: depth = 0.5; break;\n"
    "       case 2: depth = 1.0; break;\n"
    "    }\n"
    "\n"
    "    gl_Position = final_vertex;\n"
    "    instance_id = teapot_index;\n"
    "}";


App::App()
    :m_general_pipeline_id        (-1),
     m_n_frames_drawn             ( 0),
     m_n_indices                  ( 0),
     m_n_last_semaphore_used      (-1),
#if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
     m_n_rendering_physical_device(0),
#endif
#if defined(USE_LOCAL_SFR_PRESENTATION_MODE)
     m_n_presenting_physical_device(0),
#endif
     m_n_swapchain_images         (N_SWAPCHAIN_IMAGES),
     m_ooo_disabled_pipeline_id   (-1),
     m_ooo_enabled                (false),
     m_ooo_enabled_pipeline_id    (-1),
     m_should_rotate              (true)
{
    memset(m_frame_drawn_status,
           0,
           sizeof(m_frame_drawn_status) );

    m_properties_data_set = false;

    m_teapot_props_data_ptr.reset(new float[N_TEAPOTS * sizeof(float) * 8 /* pos + rot */]);
    m_timestamp_deltas.reserve   (N_TIMESTAMP_DELTAS_PER_AVERAGE_FPS_CALCULATION);
}

App::~App()
{
    deinit();
}

void App::clear_console_line()
{
    printf("\r");

    for (uint32_t n_character = 0;
                  n_character < 40;
                ++n_character)
    {
        printf(" ");
    }

    printf("\r");
}

void App::deinit()
{
    Anvil::Vulkan::vkDeviceWaitIdle(m_device_ptr->get_device_vk() );

    const Anvil::PipelineID gfx_pipeline_ids[] =
    {
        m_general_pipeline_id,
        m_ooo_disabled_pipeline_id,
        m_ooo_enabled_pipeline_id
    };
    const uint32_t n_gfx_pipeline_ids = sizeof(gfx_pipeline_ids) / sizeof(gfx_pipeline_ids[0]);

    {
        auto gfx_pipeline_manager_ptr(m_device_ptr->get_graphics_pipeline_manager() );

        for (uint32_t n_gfx_pipeline_id = 0;
                      n_gfx_pipeline_id < n_gfx_pipeline_ids;
                    ++n_gfx_pipeline_id)
        {
            const Anvil::PipelineID& pipeline_id = gfx_pipeline_ids[n_gfx_pipeline_id];

            gfx_pipeline_manager_ptr->delete_pipeline(pipeline_id);
        }
    }

    #if defined(ENABLE_MGPU_SUPPORT)
        m_frame_acquisition_wait_semaphores.clear();
    #endif

    #if defined(ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING)
        m_swapchain_image_views.clear();
        m_swapchain_images.clear();
    #endif

    m_dsg_ptrs.clear();
    m_frame_signal_semaphore_bundles.clear();
    m_frame_wait_semaphore_bundles.clear();
    m_framebuffers.clear();
    m_properties_buffer_ptrs.clear();
    m_render_cmdbuffers_ooo_on.clear();
    m_render_cmdbuffers_ooo_off.clear();
    m_renderpasses.clear();

    #if defined(ENABLE_MGPU_SUPPORT)
        m_dummy_cmdbuffer_ptr.reset();
    #endif

    #if defined(USE_LOCAL_SFR_PRESENTATION_MODE)
        m_swapchain_peer_images_per_physical_device.clear();
    #endif

    m_depth_image_ptr.reset();
    m_depth_image_view_ptr.reset();
    m_fs_entrypoint_ptr.reset();
    m_index_buffer_ptr.reset();
    m_query_pool_ptr.reset();
    m_query_results_buffer_ptr.reset();
    m_vertex_buffer_ptr.reset();
    m_vs_entrypoint_ptr.reset();

    m_rendering_surface_ptr.reset();
    m_swapchain_ptr.reset();

    m_device_ptr.reset();
    m_instance_ptr.reset();

    m_window_ptr.reset();
}

void App::draw_frame()
{
    const Anvil::DeviceType                device_type                         = m_device_ptr->get_type();
    static const Anvil::PipelineStageFlags dst_stage_mask                      = Anvil::PipelineStageFlagBits::TOP_OF_PIPE_BIT;
    Anvil::Semaphore*                      frame_ready_for_present_semaphores  [4];
    Anvil::SemaphoreMGPUSubmission         frame_ready_for_present_submissions [4];
    Anvil::SemaphoreMGPUSubmission         frame_ready_to_render_submissions   [4];
    uint32_t                               n_physical_devices;
    uint32_t                               n_swapchain_image;
    const Anvil::PhysicalDevice*           physical_device_ptr                 = nullptr;
    const Anvil::PhysicalDevice* const*    physical_devices_ptr                = nullptr;
    Anvil::PrimaryCommandBuffer*           render_cmdbuffer_ptr                = nullptr;
    const Anvil::PipelineStageFlags        wait_stage_mask                     = Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT;

    switch (device_type)
    {
        case Anvil::DeviceType::MULTI_GPU:
        {
            const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr.get() ) );

            n_physical_devices   = mgpu_device_ptr->get_n_physical_devices();
            physical_devices_ptr = mgpu_device_ptr->get_physical_devices  ();

            #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
                m_n_rendering_physical_device = (m_n_rendering_physical_device + 1) % n_physical_devices;
            #endif

            break;
        }

        case Anvil::DeviceType::SINGLE_GPU:
        {
            const Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr.get() ) );

            n_physical_devices   = 1;
            physical_device_ptr  = sgpu_device_ptr->get_physical_device();
            physical_devices_ptr = &physical_device_ptr;

            break;
        }

        default:
        {
            anvil_assert(false);
        }
    }

    /* Determine the signal + wait semaphores to use for drawing this frame */
    m_n_last_semaphore_used = (m_n_last_semaphore_used + 1) % m_n_swapchain_images;

    auto& curr_frame_signal_semaphores_ptr = m_frame_signal_semaphore_bundles.at  (m_n_last_semaphore_used);
    auto& curr_frame_wait_semaphores_ptr   = m_frame_wait_semaphore_bundles.at    (m_n_last_semaphore_used);

    #if defined(ENABLE_MGPU_SUPPORT)
        const auto& curr_frame_acqusition_wait_semaphore_ptr = m_frame_acquisition_wait_semaphores[m_n_last_semaphore_used];
    #else
        auto& curr_frame_acqusition_wait_semaphore_ptr = curr_frame_wait_semaphores_ptr->semaphores.at(0);
    #endif

    /* Determine the semaphore which the swapchain image */
    #if !defined(ENABLE_MGPU_SUPPORT) || defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
    {
        const auto acquire_result = m_swapchain_ptr->acquire_image(curr_frame_acqusition_wait_semaphore_ptr.get(),
                                                                  &n_swapchain_image,
                                                                   true); /* in_should_block */

        ANVIL_REDUNDANT_VARIABLE_CONST(acquire_result);
        anvil_assert                 (acquire_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }
    #elif defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
    {
        m_swapchain_ptr->acquire_image(curr_frame_acqusition_wait_semaphore_ptr.get(),
                                       1, /* n_mgpu_physical_device */
                                      &physical_devices_ptr[m_n_rendering_physical_device],
                                      &n_swapchain_image,
                                       true); /* in_should_block */
    }
    #else
        #error Not supported?
    #endif

    /* Set up semaphores we're going to use to render this frame. */
    anvil_assert(n_physical_devices < sizeof(frame_ready_to_render_submissions) / sizeof(frame_ready_to_render_submissions[0]) );

    for (uint32_t n_signal_sem = 0;
                  n_signal_sem < n_physical_devices;
                ++n_signal_sem)
    {
        frame_ready_for_present_submissions[n_signal_sem].device_index  = n_signal_sem;
        frame_ready_for_present_submissions[n_signal_sem].semaphore_ptr = curr_frame_signal_semaphores_ptr->semaphores[n_signal_sem].get();

        frame_ready_for_present_semaphores[n_signal_sem]                = frame_ready_for_present_submissions[n_signal_sem].semaphore_ptr;

        frame_ready_to_render_submissions[n_signal_sem].device_index    = n_signal_sem;
        frame_ready_to_render_submissions[n_signal_sem].semaphore_ptr   = curr_frame_wait_semaphores_ptr->semaphores[n_signal_sem].get();
    }

    #if defined(ENABLE_MGPU_SUPPORT)
    {
        Anvil::CommandBufferMGPUSubmission dummy_submission;
        Anvil::SemaphoreMGPUSubmission     wait_semaphore_submission;

        #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
        {
            wait_semaphore_submission.device_index = m_n_rendering_physical_device;
        }
        #elif defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
        {
            /* It shouldn't matter which physical device we wait on for the frame acquisition semaphore. */
            wait_semaphore_submission.device_index = 0;
        }
        #else
            #error Not supported?
        #endif

        wait_semaphore_submission.semaphore_ptr = curr_frame_acqusition_wait_semaphore_ptr.get();

        #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
        {
            dummy_submission.cmd_buffer_ptr = m_dummy_cmdbuffer_ptr.get();
            dummy_submission.device_mask    = (1 << m_n_rendering_physical_device);

            m_present_queue_ptr->submit(
                Anvil::SubmitInfo::create_wait_execute_signal(&dummy_submission,
                                                             1, /* n_command_buffer_submissions */
                                                             1, /* n_signal_semaphore_submissions */
                                                             frame_ready_to_render_submissions + m_n_rendering_physical_device,
                                                             1, /* n_wait_semaphore_submissions */
                                                            &wait_semaphore_submission,
                                                            &dst_stage_mask,
                                                             false) /* should_block */
            );
        }
        #elif defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
        {
            dummy_submission.cmd_buffer_ptr = m_dummy_cmdbuffer_ptr.get();
            dummy_submission.device_mask    = (1 << n_physical_devices) - 1;

            m_present_queue_ptr->submit(
                Anvil::SubmitInfo::create_wait_execute_signal(&dummy_submission,
                                                              1, /* n_command_buffer_submissions */
                                                              n_physical_devices, /* n_signal_semaphore_submissions */
                                                              frame_ready_to_render_submissions,
                                                              1, /* n_wait_semaphore_submissions */
                                                             &wait_semaphore_submission,
                                                             &dst_stage_mask,
                                                              false) /* should_block */
            );
        }
        #else
            #error Not supported?
        #endif
    }
    #endif

    /* if the frame has already been rendered to in the past, then given the fact we use FIFO presentation mode,
     * we should be safe to extract the timestamps which must have been written by now.
     */
    if (m_frame_drawn_status[n_swapchain_image])
    {
        uint64_t timestamps[2]; /* top of pipe, bottom of pipe */

        #if defined(ENABLE_MGPU_SUPPORT)
        {
            /* See ENABLE_MGPU_SUPPORT documentation for more details reg. the assertion check below. */
            anvil_assert(n_physical_devices == N_SWAPCHAIN_IMAGES);
        }
        #endif

        /* TODO: Do better than this. */
        Anvil::Vulkan::vkDeviceWaitIdle(m_device_ptr->get_device_vk() );

        #if !defined(ENABLE_MGPU_SUPPORT) || (!defined(USE_SUM_SFR_PRESENTATION_MODE) && !defined(USE_LOCAL_SFR_PRESENTATION_MODE) )
            const uint32_t n_iterations = 1;
        #elif defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
            const uint32_t n_iterations = n_physical_devices;
        #endif

        for (uint32_t n_iteration = 0;
                      n_iteration < n_iterations;
                    ++n_iteration)
        {
            #if !defined(ENABLE_MGPU_SUPPORT) || defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
                auto device_mask = (1 << n_iteration);
            #else
                auto device_mask = (1 << m_n_rendering_physical_device);
            #endif

            m_query_results_buffer_ptr->read(n_swapchain_image * sizeof(uint64_t) * 2, /* top of pipe, bottom of pipe */
                                             sizeof(timestamps),
#if defined(ENABLE_MGPU_SUPPORT)
                                             device_mask,
#endif
                                             timestamps);

            // anvil_assert(timestamps[1] != timestamps[0]);

            m_timestamp_deltas.push_back(timestamps[1] - timestamps[0]);
        }

        if (m_timestamp_deltas.size() >= N_TIMESTAMP_DELTAS_PER_AVERAGE_FPS_CALCULATION)
        {
            update_fps();
        }
    }

    /* Update the teapot properties data for the current swapchain image */
    update_teapot_props(n_swapchain_image);

    /* Submit work chunks and present */
    if (m_ooo_enabled)
    {
        #if defined(ENABLE_MGPU_SUPPORT)
            #if defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
                anvil_assert(m_render_cmdbuffers_ooo_on.size() == 1);

                render_cmdbuffer_ptr = m_render_cmdbuffers_ooo_on.at(0).at(n_swapchain_image).get();
            #elif defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
                render_cmdbuffer_ptr = m_render_cmdbuffers_ooo_on.at(m_n_rendering_physical_device).at(n_swapchain_image).get();
            #else
                #error Not supported?
            #endif
        #else
            render_cmdbuffer_ptr = m_render_cmdbuffers_ooo_on.at(n_swapchain_image).get();
        #endif
    }
    else
    {
        #if defined(ENABLE_MGPU_SUPPORT)
            #if defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
                anvil_assert(m_render_cmdbuffers_ooo_on.size() == 1);

                render_cmdbuffer_ptr = m_render_cmdbuffers_ooo_off.at(0).at(n_swapchain_image).get();
            #elif defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
                render_cmdbuffer_ptr = m_render_cmdbuffers_ooo_off.at(m_n_rendering_physical_device).at(n_swapchain_image).get();
            #else
                #error Not supported?
            #endif
        #else
            render_cmdbuffer_ptr = m_render_cmdbuffers_ooo_off.at(n_swapchain_image).get();
        #endif
    }

    #if !defined(ENABLE_MGPU_SUPPORT)
    {
        m_present_queue_ptr->submit(
            Anvil::SubmitInfo::create_wait_execute_signal(render_cmdbuffer_ptr,
                                                          1, /* n_semaphores_to_signal */
                                                         &frame_ready_for_present_submissions[0].semaphore_ptr,
                                                          1, /* n_semaphores_to_wait_on */
                                                         &frame_ready_to_render_submissions[0].semaphore_ptr,
                                                         &wait_stage_mask,
                                                          false /* should_block */)
        );
    }
    #else
    {
        Anvil::CommandBufferMGPUSubmission cmd_buffer_submission;

        cmd_buffer_submission.cmd_buffer_ptr = render_cmdbuffer_ptr;

        #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
        {
            cmd_buffer_submission.device_mask = (1 << m_n_rendering_physical_device);

            m_present_queue_ptr->submit(
                Anvil::SubmitInfo::create_wait_execute_signal(&cmd_buffer_submission,
                                                             1,                                  /* n_command_buffer_submissions */
                                                             1,                                  /* n_signal_semaphore_submissions */
                                                             frame_ready_for_present_submissions + m_n_rendering_physical_device,
                                                             1,                                  /* n_wait_semaphore_submissions */
                                                             frame_ready_to_render_submissions + m_n_rendering_physical_device,
                                                            &wait_stage_mask,
                                                             false /* should_block */)
            );
        }
        #elif defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
        {
            cmd_buffer_submission.device_mask = (1 << n_physical_devices) - 1;

            m_present_queue_ptr->submit(
                Anvil::SubmitInfo::create_wait_execute_signal(&cmd_buffer_submission,
                                                               1, /* n_command_buffer_submissions */
                                                               n_physical_devices,
                                                               frame_ready_for_present_submissions,
                                                               n_physical_devices, /* n_wait_semaphore_submissions */
                                                               frame_ready_to_render_submissions,
                                                              &wait_stage_mask,
                                                               false /* should_block */)
            );
        }
        #else
            #error Not supported?
        #endif
    }
    #endif

    #if !defined(ENABLE_MGPU_SUPPORT)
    {
        Anvil::SwapchainOperationErrorCode present_result = Anvil::SwapchainOperationErrorCode::DEVICE_LOST;

        m_present_queue_ptr->present(m_swapchain_ptr.get(),
                                                   n_swapchain_image,
                                                   n_physical_devices, /* n_wait_semaphores */
                                                   frame_ready_for_present_semaphores,
                                                  &present_result);

        ANVIL_REDUNDANT_VARIABLE(present_result);
        anvil_assert            (present_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }
    #else
    {
        #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_LOCAL_SFR_PRESENTATION_MODE)
        {
            const Anvil::MGPUDevice*             mgpu_device_ptr                (dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr.get() ));
            Anvil::SwapchainOperationErrorCode   present_result                 (Anvil::SwapchainOperationErrorCode::DEVICE_LOST);
            Anvil::LocalModePresentationItem     presentation_item;
            const Anvil::PhysicalDevice*         presenting_physical_device_ptr (nullptr);
            Anvil::Semaphore*                    wait_semaphore_ptr             (nullptr);

            #if defined(USE_LOCAL_SFR_PRESENTATION_MODE)
                presenting_physical_device_ptr = mgpu_device_ptr->get_physical_device(m_n_presenting_physical_device);
                wait_semaphore_ptr             = frame_ready_for_present_semaphores[m_n_presenting_physical_device];
            #else
                presenting_physical_device_ptr = mgpu_device_ptr->get_physical_device(m_n_rendering_physical_device);
                wait_semaphore_ptr             = frame_ready_for_present_semaphores[m_n_rendering_physical_device];
            #endif

            presentation_item.physical_device_ptr   = presenting_physical_device_ptr;
            presentation_item.swapchain_image_index = n_swapchain_image;
            presentation_item.swapchain_ptr         = m_swapchain_ptr.get();

            m_present_queue_ptr->present_in_local_presentation_mode(1,
                                                                   &presentation_item,
                                                                    1, /* n_wait_semaphores */
                                                                   &wait_semaphore_ptr,
                                                                   &present_result);

            ANVIL_REDUNDANT_VARIABLE(present_result);
            anvil_assert            (present_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
        }
        #elif defined(USE_REMOTE_AFR_PRESENTATION_MODE)
        {
            const Anvil::MGPUDevice*           mgpu_device_ptr  (dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr.get() ));
            Anvil::SwapchainOperationErrorCode present_result   (Anvil::SwapchainOperationErrorCode::DEVICE_LOST);
            Anvil::RemoteModePresentationItem  presentation_item;

            presentation_item.physical_device_ptr   = mgpu_device_ptr->get_physical_device(m_n_rendering_physical_device);
            presentation_item.swapchain_image_index = n_swapchain_image;
            presentation_item.swapchain_ptr         = m_swapchain_ptr.get();

            m_present_queue_ptr->present_in_remote_presentation_mode(1, /* n_remote_mode_presentation_items */
                                                                    &presentation_item,
                                                                     1, /* n_wait_semaphores */
                                                                     frame_ready_for_present_semaphores + m_n_rendering_physical_device,
                                                                    &present_result);

            ANVIL_REDUNDANT_VARIABLE(present_result);
            anvil_assert            (present_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
        }
        #elif defined(USE_SUM_SFR_PRESENTATION_MODE)
        {
            Anvil::SwapchainOperationErrorCode present_result     = Anvil::SwapchainOperationErrorCode::DEVICE_LOST;
            Anvil::SumModePresentationItem     presentation_item;

            presentation_item.n_physical_devices    = n_physical_devices;
            presentation_item.physical_devices_ptr  = physical_devices_ptr;
            presentation_item.swapchain_image_index = n_swapchain_image;
            presentation_item.swapchain_ptr         = m_swapchain_ptr.get();

            m_present_queue_ptr->present_in_sum_presentation_mode(1, /* n_sum_mode_presentation_items */
                                                                 &presentation_item,
                                                                  n_physical_devices, /* n_wait_semaphores */
                                                                  frame_ready_for_present_semaphores,
                                                                 &present_result);

            ANVIL_REDUNDANT_VARIABLE(present_result);
            anvil_assert            (present_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
        }
        #else
            #error You must enable one of the USE_*_PRESENTATION_MODE #defines.
        #endif
    }
    #endif

    ++m_n_frames_drawn;

    m_frame_drawn_status[n_swapchain_image] = true;

    #if defined(ENABLE_OFFSCREEN_RENDERING)
    {
        if (m_n_frames_drawn >= N_FRAMES_TO_RENDER)
        {
            m_window_ptr->close();
        }
    }
    #endif
}

#if defined(ENABLE_MGPU_SUPPORT)

std::vector<VkRect2D> App::get_render_areas(uint32_t in_afr_render_index) const
{
    const Anvil::MGPUDevice* mgpu_device_locked_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr.get() ));
    const uint32_t           n_physical_devices    (mgpu_device_locked_ptr->get_n_physical_devices() );
    std::vector<VkRect2D>    render_areas;
    bool                     result;
    VkExtent2D               sfr_tile_size;
    VkExtent2D               split_chunk_size;
    const uint32_t           window_height         (m_window_ptr->get_height_at_creation_time());
    const uint32_t           window_width          (m_window_ptr->get_width_at_creation_time () );

    if ((m_swapchain_ptr->get_create_info_ptr()->get_flags() & Anvil::SwapchainCreateFlagBits::SPLIT_INSTANCE_BIND_REGIONS_BIT) != 0)
    {
        result = m_swapchain_ptr->get_image(0)->get_SFR_tile_size(&sfr_tile_size);
        anvil_assert(result);
    }
    else
    {
        /* SFR is disabled - we don't need to follow any specific alignment requirements */
        anvil_assert((window_width % n_physical_devices) == 0);

        sfr_tile_size.width  = window_width / n_physical_devices;
        sfr_tile_size.height = window_height;
    }

    split_chunk_size.width  = Anvil::Utils::round_up(window_width / n_physical_devices, sfr_tile_size.width);
    split_chunk_size.height = Anvil::Utils::round_up(window_height,                     sfr_tile_size.height);

    #if defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
    {
        for (uint32_t n_render_area = 0;
                      n_render_area < n_physical_devices;
                    ++n_render_area)
        {
            /* Split the frame vertically. Make sure the render area never exceeds the framebuffer's extent. */
            VkRect2D render_area_chunk;

            render_area_chunk.offset.x      = n_render_area * split_chunk_size.width;
            render_area_chunk.offset.y      = 0;
            render_area_chunk.extent.height = split_chunk_size.height;
            render_area_chunk.extent.width  = split_chunk_size.width;

            if (render_area_chunk.offset.x + render_area_chunk.extent.width > window_width)
            {
                render_area_chunk.extent.width = window_width - render_area_chunk.offset.x;
            }

            if (render_area_chunk.offset.y + render_area_chunk.extent.height > window_height)
            {
                render_area_chunk.extent.height = window_height - render_area_chunk.offset.y;
            }

            render_areas.push_back(render_area_chunk);
        }
    }
    #elif defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
    {
        VkRect2D dummy_render_area;
        VkRect2D render_area;

        dummy_render_area.extent.height = 0;
        dummy_render_area.extent.width  = 0;
        dummy_render_area.offset.x      = 0;
        dummy_render_area.offset.y      = 0;

        render_area.extent.width  = window_width;
        render_area.extent.height = window_height;
        render_area.offset.x      = 0;
        render_area.offset.y      = 0;

        for (uint32_t n_pre_physical_device = 0;
                      n_pre_physical_device < in_afr_render_index;
                    ++n_pre_physical_device)
        {
            render_areas.push_back(dummy_render_area);
        }

        render_areas.push_back(render_area);

        for (uint32_t n_post_physical_device = in_afr_render_index + 1;
                      n_post_physical_device < n_physical_devices;
                    ++n_post_physical_device)
        {
            render_areas.push_back(dummy_render_area);
        }
    }
    #else
        #error Not supported?
    #endif

    return render_areas;
}

#endif

void App::init()
{
    init_vulkan   ();
    init_window   ();
    init_swapchain();

    init_buffers   ();
    init_dsgs      ();
    init_images    ();
    init_query_pool();
    init_semaphores();
    init_shaders   ();

    init_renderpasses   ();
    init_gfx_pipelines  ();
    init_command_buffers();
}

void App::init_buffers()
{
    Anvil::MemoryAllocatorUniquePtr   allocator_ptr;
    TeapotData                        data         (U_GRANULARITY, V_GRANULARITY);
    const Anvil::DeviceType           device_type  (m_device_ptr->get_type() );

    const VkDeviceSize                index_data_size                    = data.get_index_data_size();
    const VkDeviceSize                properties_data_size               = N_TEAPOTS * sizeof(float) * 8; /* rot_xyzX + pos_xyzX */
    const Anvil::MemoryFeatureFlags   required_feature_flags             = (device_type == Anvil::DeviceType::SINGLE_GPU) ? Anvil::MemoryFeatureFlagBits::NONE
                                                                                                                          : Anvil::MemoryFeatureFlagBits::MULTI_INSTANCE_BIT;
    Anvil::MGPUPeerMemoryRequirements required_peer_memory_feature_flags;
    const VkDeviceSize                vertex_data_size                   = data.get_vertex_data_size();

    allocator_ptr = Anvil::MemoryAllocator::create_oneshot(m_device_ptr.get() );

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        index_data_size,
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::INDEX_BUFFER_BIT);

        m_index_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        sizeof(uint64_t) * m_n_swapchain_images * 2, /* top of pipe, bottom of pipe */
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT);

        m_query_results_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        vertex_data_size,
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT);

        m_vertex_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    m_index_buffer_ptr->set_name        ("Teapot index buffer");
    m_query_results_buffer_ptr->set_name("Query results buffer");
    m_vertex_buffer_ptr->set_name       ("Teapot vertex buffer");

    allocator_ptr->add_buffer(m_query_results_buffer_ptr.get(),
                              required_feature_flags);

    allocator_ptr->add_buffer(m_index_buffer_ptr.get(),
                              required_feature_flags);
    allocator_ptr->add_buffer(m_vertex_buffer_ptr.get(),
                              required_feature_flags);

    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < m_n_swapchain_images;
                ++n_swapchain_image)
    {
        Anvil::BufferUniquePtr new_buffer_ptr;

        {
            auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                            properties_data_size,
                                                                            Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                            Anvil::SharingMode::EXCLUSIVE,
                                                                            Anvil::BufferCreateFlagBits::NONE,
                                                                            Anvil::BufferUsageFlagBits::STORAGE_BUFFER_BIT);

            new_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
        }

        new_buffer_ptr->set_name("Properties buffer");

        allocator_ptr->add_buffer(new_buffer_ptr.get(),
                                  required_feature_flags); /* in_required_memory_features */

        m_properties_buffer_ptrs.push_back(
            std::move(new_buffer_ptr)
        );
    }

    m_index_buffer_ptr->write (0, /* start_offset */
                               index_data_size,
                               data.get_index_data() );
    m_vertex_buffer_ptr->write(0, /* start_offset */
                               vertex_data_size,
                               data.get_vertex_data() );

    m_n_indices = static_cast<uint32_t>(index_data_size / sizeof(uint32_t) );
}

void App::init_command_buffers()
{
    VkClearValue                   clear_values[2];
    const Anvil::DeviceType        device_type                  = m_device_ptr->get_type();
    auto                           gfx_manager_ptr              = m_device_ptr->get_graphics_pipeline_manager();
    const uint32_t                 n_swapchain_images           = m_swapchain_ptr->get_n_images();
    const uint32_t                 universal_queue_family_index = m_device_ptr->get_universal_queue(0)->get_queue_family_index();
    Anvil::Buffer*                 vertex_buffers[]             =
    {
        m_vertex_buffer_ptr.get()
    };
    VkDeviceSize vertex_buffer_offsets[] =
    {
        0
    };

    const uint32_t n_vertex_buffers        = sizeof(vertex_buffers)        / sizeof(vertex_buffers       [0]);
    const uint32_t n_vertex_buffer_offsets = sizeof(vertex_buffer_offsets) / sizeof(vertex_buffer_offsets[0]);

    static_assert(n_vertex_buffers                   == n_vertex_buffer_offsets, "");
    anvil_assert (m_framebuffers.size()              == n_swapchain_images);
    anvil_assert (m_render_cmdbuffers_ooo_off.size() == 0);
    anvil_assert (m_render_cmdbuffers_ooo_on.size()  == 0);
    anvil_assert (m_renderpasses.size()              == n_swapchain_images);

    #if defined(ENABLE_MGPU_SUPPORT)
        const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr.get() ) );
        #if !defined(USE_LOCAL_AFR_PRESENTATION_MODE)
            std::vector<VkRect2D>    render_areas   (get_render_areas() );
        #endif

        #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
            const uint32_t n_physical_device_iterations(mgpu_device_ptr->get_n_physical_devices() );
        #elif defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
            const uint32_t n_physical_device_iterations(1);
        #else
            #error Not supported?
        #endif
    #else
        const uint32_t n_physical_device_iterations(1);
        VkRect2D       render_area;

        render_area.extent.height = m_window_ptr->get_height_at_creation_time();
        render_area.extent.width  = m_window_ptr->get_width_at_creation_time ();
        render_area.offset.x      = 0;
        render_area.offset.y      = 0;
    #endif

    clear_values[0].color.float32[0]   = 0.0f;
    clear_values[0].color.float32[1]   = 1.0f;
    clear_values[0].color.float32[2]   = 0.0f;
    clear_values[0].color.float32[3]   = 1.0f;
    clear_values[1].depthStencil.depth = 1.0f;

    for (uint32_t n_physical_device_iteration = 0;
                  n_physical_device_iteration < n_physical_device_iterations;
                ++n_physical_device_iteration)
    {
        #if defined(ENABLE_MGPU_SUPPORT) && defined (USE_LOCAL_AFR_PRESENTATION_MODE)
            std::vector<VkRect2D>    render_areas(get_render_areas(n_physical_device_iteration));
        #endif

        for (uint32_t n_ooo_iteration = 0;
                      n_ooo_iteration < 2; /* off, on */
                    ++n_ooo_iteration)
        {
            const bool              is_ooo_enabled      = (n_ooo_iteration == 1);
            const Anvil::PipelineID pipeline_id         = (is_ooo_enabled) ? m_ooo_enabled_pipeline_id
                                                                           : m_ooo_disabled_pipeline_id;
            Anvil::PipelineLayout*  pipeline_layout_ptr = gfx_manager_ptr->get_pipeline_layout(pipeline_id);

            #if defined(ENABLE_MGPU_SUPPORT)
                std::vector<Anvil::PrimaryCommandBufferUniquePtr>& render_cmdbuffers = (is_ooo_enabled) ? m_render_cmdbuffers_ooo_on [n_physical_device_iteration]
                                                                                                        : m_render_cmdbuffers_ooo_off[n_physical_device_iteration];
            #else
                std::vector<Anvil::PrimaryCommandBufferUniquePtr>& render_cmdbuffers = (is_ooo_enabled) ? m_render_cmdbuffers_ooo_on
                                                                                                        : m_render_cmdbuffers_ooo_off;
            #endif

            for (uint32_t n_render_cmdbuffer = 0;
                          n_render_cmdbuffer < n_swapchain_images;
                        ++n_render_cmdbuffer)
            {
                Anvil::PrimaryCommandBufferUniquePtr cmdbuffer_ptr;
                Anvil::DescriptorSet*                ds_ptr         (m_dsg_ptrs    [n_render_cmdbuffer]->get_descriptor_set(0));
                Anvil::Framebuffer*                  framebuffer_ptr(m_framebuffers[n_render_cmdbuffer].get() );
                Anvil::RenderPass*                   renderpass_ptr (m_renderpasses[n_render_cmdbuffer].get() );

                const Anvil::BufferBarrier query_result_barrier(Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,
                                                                Anvil::AccessFlagBits::HOST_READ_BIT | Anvil::AccessFlagBits::TRANSFER_READ_BIT,
                                                                universal_queue_family_index,
                                                                universal_queue_family_index,
                                                                m_query_results_buffer_ptr.get(),
                                                                sizeof(uint64_t) * n_render_cmdbuffer * 2,
                                                                sizeof(uint64_t) * 2);
                const Anvil::BufferBarrier props_buffer_barrier(Anvil::AccessFlagBits::HOST_WRITE_BIT,
                                                                Anvil::AccessFlagBits::SHADER_READ_BIT,
                                                                universal_queue_family_index,
                                                                universal_queue_family_index,
                                                                m_properties_buffer_ptrs[n_render_cmdbuffer].get(),
                                                                0,
                                                                N_TEAPOTS * sizeof(float) * 2 * 4 /* pos + rot */);


                cmdbuffer_ptr = m_device_ptr->get_command_pool_for_queue_family_index(universal_queue_family_index)->alloc_primary_level_command_buffer();

                cmdbuffer_ptr->set_name_formatted("Rendering command buffer (OoO:%s)",
                                                  ((n_ooo_iteration == 0) ? "off" : "on") );

                cmdbuffer_ptr->start_recording(false,  /* one_time_submit          */
                                               true);  /* simultaneous_use_allowed */
                {
                    clear_values[0].color.float32[0] = (is_ooo_enabled) ? 1.0f : 0.0f;

                    #if 1
                        /* Useful if you need to visually determine which GPU rendered which frame. */
                        #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
                            clear_values[0].color.float32[2] = float(n_physical_device_iteration) / (n_physical_device_iterations - 1);
                        #endif
                    #endif

                    cmdbuffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::HOST_BIT,
                                                           Anvil::PipelineStageFlagBits::VERTEX_SHADER_BIT,
                                                           Anvil::DependencyFlagBits::NONE,
                                                           0,        /* in_memory_barrier_count        */
                                                           nullptr,  /* in_memory_barriers_ptr         */
                                                           1,        /* in_buffer_memory_barrier_count */
                                                          &props_buffer_barrier,
                                                           0,         /* in_image_memory_barrier_count */
                                                           nullptr);  /* in_image_memory_barrier_ptrs  */

                    cmdbuffer_ptr->record_write_timestamp(Anvil::PipelineStageFlagBits::TOP_OF_PIPE_BIT,
                                                          m_query_pool_ptr.get(),
                                                          n_render_cmdbuffer * 2 /* top of pipe, bottom of pipe*/ + 0);

                    #if !defined(ENABLE_MGPU_SUPPORT)
                    {
                        cmdbuffer_ptr->record_begin_render_pass(sizeof(clear_values) / sizeof(clear_values[0]),
                                                                clear_values,
                                                                framebuffer_ptr,
                                                                render_area,
                                                                renderpass_ptr,
                                                                Anvil::SubpassContents::INLINE);
                    }
                    #else
                    {
                        const Anvil::MGPUDevice* mgpu_device_ptr (dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr.get() ));
                        const auto&              render_areas_ptr(&render_areas.at(0) );

                        #if defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE) || defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
                            const uint32_t device_mask = (1 << mgpu_device_ptr->get_n_physical_devices()) - 1;
                        #else
                            #error Not supported?
                        #endif

                        anvil_assert(render_areas.size() == mgpu_device_ptr->get_n_physical_devices());

                        cmdbuffer_ptr->record_begin_render_pass(sizeof(clear_values) / sizeof(clear_values[0]),
                                                                clear_values,
                                                                framebuffer_ptr,
                                                                device_mask,
                                                                static_cast<uint32_t>(render_areas.size()),
                                                                render_areas_ptr,
                                                                renderpass_ptr,
                                                                Anvil::SubpassContents::INLINE);
                    }
                    #endif
                    {
                        #if defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
                            const uint32_t                      n_physical_devices(mgpu_device_ptr->get_n_physical_devices());
                            const Anvil::PhysicalDevice* const* physical_devices  (mgpu_device_ptr->get_physical_devices  ());
                        #elif defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
                            const uint32_t                      n_physical_devices(1);
                            const Anvil::PhysicalDevice* const* physical_devices  (mgpu_device_ptr->get_physical_devices  () + n_physical_device_iteration);
                        #else
                            const uint32_t n_physical_devices(1);
                        #endif

                        cmdbuffer_ptr->record_bind_pipeline(Anvil::PipelineBindPoint::GRAPHICS,
                                                            pipeline_id);

                        cmdbuffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                                   pipeline_layout_ptr,
                                                                   0, /* in_first_set */
                                                                   1, /* in_set_count */
                                                                  &ds_ptr,
                                                                   0,        /* in_dynamic_offset_count */
                                                                   nullptr); /* in_dynamic_offset_ptrs  */

                        cmdbuffer_ptr->record_bind_index_buffer  (m_index_buffer_ptr.get(),
                                                                  0, /* in_offset */
                                                                  Anvil::IndexType::UINT32);
                        cmdbuffer_ptr->record_bind_vertex_buffers(0, /* in_start_binding */
                                                                  n_vertex_buffers,
                                                                  vertex_buffers,
                                                                  vertex_buffer_offsets);

                        for (uint32_t n_physical_device = 0;
                                      n_physical_device < n_physical_devices;
                                    ++n_physical_device)
                        {
                            #if defined(ENABLE_MGPU_SUPPORT)
                            {
                                #if defined(USE_LOCAL_AFR_PRESENTATION_MODE)
                                    VkRect2D   scissor  = render_areas.at(n_physical_device_iteration);
                                #else
                                    VkRect2D   scissor = render_areas.at(n_physical_device);
                                #endif
                                VkViewport viewport;

                                viewport.height   = static_cast<float>(scissor.extent.height);
                                viewport.maxDepth = 1.0f;
                                viewport.minDepth = 0.0f;
                                viewport.width    = static_cast<float>(WINDOW_WIDTH);
                                viewport.x        = 0.0f;
                                viewport.y        = 0.0f;

                                #if defined(USE_LOCAL_SFR_PRESENTATION_MODE) || defined(USE_SUM_SFR_PRESENTATION_MODE)
                                {
                                    cmdbuffer_ptr->record_set_device_mask_KHR(1 << n_physical_device);
                                }
                                #endif

                                cmdbuffer_ptr->record_set_scissor (0, /* in_first_scissor */
                                                                   1, /* in_scissor_count */
                                                                  &scissor);
                                cmdbuffer_ptr->record_set_viewport(0, /* in_first_viewport */
                                                                   1, /* in_viewport_count */
                                                                  &viewport);
                            }
                            #endif

                            /* Draw the teapots! */
                            cmdbuffer_ptr->record_draw_indexed(m_n_indices,
                                                               N_TEAPOTS, /* in_instance_count */
                                                               0,         /* in_first_index    */
                                                               0,         /* in_vertex_offset  */
                                                               0);        /* in_first_instance */
                        }
                    }
                    cmdbuffer_ptr->record_end_render_pass();

                    #if defined(USE_LOCAL_SFR_PRESENTATION_MODE)
                    {
                        /* Once all GPUs have finished rendering, we need to copy all content rendered by non-presenting
                         * devices to the presenting device's swapchain image instance. */
                        std::vector<Anvil::ImageBarrier> image_barriers_pre;
                        const uint32_t                   n_total_physical_devices(mgpu_device_ptr->get_n_physical_devices() );

                        /* First, submit all the barriers we're going to need to submit before we can use the copy op. */
                        for (uint32_t n_current_physical_device = 0;
                                      n_current_physical_device < n_total_physical_devices;
                                    ++n_current_physical_device)
                        {
                            cmdbuffer_ptr->record_set_device_mask_KHR(1 << n_current_physical_device);

                            if (n_current_physical_device == m_n_presenting_physical_device)
                            {
                                const Anvil::ImageBarrier renderable_to_dst_transferrable_layout_barrier(Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT,
                                                                                                         Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,
                                                                                                         Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                                                         Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,
                                                                                                         VK_QUEUE_FAMILY_IGNORED,
                                                                                                         VK_QUEUE_FAMILY_IGNORED,
                                                                                                         m_swapchain_ptr->get_image(n_render_cmdbuffer),
                                                                                                         m_swapchain_ptr->get_image(n_render_cmdbuffer)->get_subresource_range() );

                                cmdbuffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
                                                                       Anvil::PipelineStageFlagBits::TRANSFER_BIT,
                                                                       Anvil::DependencyFlagBits::BY_REGION_BIT,
                                                                       0,        /* in_memory_barrier_count        */
                                                                       nullptr,  /* in_memory_barriers_ptr         */
                                                                       0,        /* in_buffer_memory_barrier_count */
                                                                       nullptr,  /* in_buffer_memory_barriers_ptr  */
                                                                       1,        /* in_image_memory_barrier_count  */
                                                                      &renderable_to_dst_transferrable_layout_barrier);
                            }
                            else
                            {
                                const auto src_image_ptr = m_swapchain_peer_images_per_physical_device.at(n_current_physical_device).peer_images.at(n_render_cmdbuffer).get();

                                const Anvil::ImageBarrier renderable_to_src_transferrable_layout_barrier(Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT,
                                                                                                         Anvil::AccessFlagBits::TRANSFER_READ_BIT,
                                                                                                         Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                                                         Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,
                                                                                                         VK_QUEUE_FAMILY_IGNORED,
                                                                                                         VK_QUEUE_FAMILY_IGNORED,
                                                                                                         src_image_ptr,
                                                                                                         src_image_ptr->get_subresource_range() );

                                cmdbuffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
                                                                       Anvil::PipelineStageFlagBits::TRANSFER_BIT,
                                                                       Anvil::DependencyFlagBits::BY_REGION_BIT,
                                                                       0,        /* in_memory_barrier_count        */
                                                                       nullptr,  /* in_memory_barriers_ptr         */
                                                                       0,        /* in_buffer_memory_barrier_count */
                                                                       nullptr,  /* in_buffer_memory_barriers_ptr  */
                                                                       1,        /* in_image_memory_barrier_count  */
                                                                      &renderable_to_src_transferrable_layout_barrier);
                            }
                        }

                        /* Next, copy rendered image chunks to the swapchain image instance which is going to be presented. */
                        const auto presentable_peer_image_ptr(m_swapchain_peer_images_per_physical_device.at(m_n_presenting_physical_device).peer_images.at(n_render_cmdbuffer).get() );

                        for (uint32_t n_current_physical_device = 0;
                                      n_current_physical_device < n_total_physical_devices;
                                    ++n_current_physical_device)
                        {
                            Anvil::ImageCopy copy_region;
                            const auto       src_image_ptr   = m_swapchain_peer_images_per_physical_device.at(n_current_physical_device).peer_images.at(n_render_cmdbuffer).get();
                            const auto&      src_render_area = render_areas[n_current_physical_device];

                            if (n_current_physical_device == m_n_presenting_physical_device)
                            {
                                continue;
                            }

                            copy_region.dst_offset.x                     = src_render_area.offset.x;
                            copy_region.dst_offset.y                     = src_render_area.offset.y;
                            copy_region.dst_offset.z                     = 0;
                            copy_region.dst_subresource.aspect_mask      = Anvil::ImageAspectFlagBits::COLOR_BIT;
                            copy_region.dst_subresource.base_array_layer = 0;
                            copy_region.dst_subresource.layer_count      = 1;
                            copy_region.dst_subresource.mip_level        = 0;
                            copy_region.extent.depth                     = 1;
                            copy_region.extent.height                    = src_render_area.extent.height;
                            copy_region.extent.width                     = src_render_area.extent.width;
                            copy_region.src_offset.x                     = src_render_area.offset.x;
                            copy_region.src_offset.y                     = src_render_area.offset.y;
                            copy_region.src_offset.z                     = 0;
                            copy_region.src_subresource                  = copy_region.dst_subresource;

                            cmdbuffer_ptr->record_set_device_mask_KHR(1 << n_current_physical_device);

                            cmdbuffer_ptr->record_copy_image(src_image_ptr,
                                                             Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL, /* in_src_image_layout */
                                                             presentable_peer_image_ptr,
                                                             Anvil::ImageLayout::TRANSFER_DST_OPTIMAL, /* in_dst_image_layout */
                                                             1, /* in_region_count */
                                                            &copy_region);
                        }

                        /* Finally, transfer the swapchain image instance we are about to present to presentable layout. */
                        const Anvil::ImageBarrier dst_transferrable_to_presentable_layout_barrier(Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,
                                                                                                  Anvil::AccessFlagBits::MEMORY_READ_BIT,
                                                                                                  Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,
                                                                                                  Anvil::ImageLayout::PRESENT_SRC_KHR,
                                                                                                  VK_QUEUE_FAMILY_IGNORED,
                                                                                                  VK_QUEUE_FAMILY_IGNORED,
                                                                                                  presentable_peer_image_ptr,
                                                                                                  presentable_peer_image_ptr->get_subresource_range() );

                        cmdbuffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::TRANSFER_BIT,
                                                               Anvil::PipelineStageFlagBits::BOTTOM_OF_PIPE_BIT,
                                                               Anvil::DependencyFlagBits::BY_REGION_BIT,
                                                               0,        /* in_memory_barrier_count        */
                                                               nullptr,  /* in_memory_barriers_ptr         */
                                                               0,        /* in_buffer_memory_barrier_count */
                                                               nullptr,  /* in_buffer_memory_barriers_ptr  */
                                                               1,        /* in_image_memory_barrier_count  */
                                                              &dst_transferrable_to_presentable_layout_barrier);
                    }
                    #endif

                    #if defined(ENABLE_MGPU_SUPPORT)
                    {
                        cmdbuffer_ptr->record_set_device_mask_KHR((1 << mgpu_device_ptr->get_n_physical_devices()) - 1);
                    }
                    #endif

                    cmdbuffer_ptr->record_write_timestamp        (Anvil::PipelineStageFlagBits::ALL_GRAPHICS_BIT,
                                                                  m_query_pool_ptr.get(),
                                                                  n_render_cmdbuffer * 2 /* top of pipe, bottom of pipe */ + 1);
                    cmdbuffer_ptr->record_copy_query_pool_results(m_query_pool_ptr.get(),
                                                                  n_render_cmdbuffer * 2, /* top of pipe, bottom of pipe */
                                                                  2, /* in_query_count */
                                                                  m_query_results_buffer_ptr.get(),
                                                                  sizeof(uint64_t) * n_render_cmdbuffer * 2,
                                                                  sizeof(uint64_t),
                                                                  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

                    cmdbuffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::TRANSFER_BIT,
                                                           Anvil::PipelineStageFlagBits::HOST_BIT | Anvil::PipelineStageFlagBits::TRANSFER_BIT,
                                                           Anvil::DependencyFlagBits::NONE,
                                                           0,        /* in_memory_barrier_count        */
                                                           nullptr,  /* in_memory_barriers_ptr         */
                                                           1,        /* in_buffer_memory_barrier_count */
                                                           &query_result_barrier,
                                                           0,        /* in_image_memory_barrier_count */
                                                           nullptr); /* in_image_memory_barriers_ptr  */
                }
                cmdbuffer_ptr->stop_recording();

                render_cmdbuffers.push_back(std::move(cmdbuffer_ptr) );
            }
        }
    }

    #if defined(ENABLE_MGPU_SUPPORT)
    {
        m_dummy_cmdbuffer_ptr = mgpu_device_ptr->get_command_pool_for_queue_family_index(universal_queue_family_index)->alloc_primary_level_command_buffer();

        m_dummy_cmdbuffer_ptr->start_recording(false, /* one_time_submit          */
                                               true); /* simultaneous_use_allowed */
        {
            /* Stub */
        }
        m_dummy_cmdbuffer_ptr->stop_recording();
    }
    #endif
}

void App::init_dsgs()
{
    anvil_assert(m_properties_buffer_ptrs.size() == m_n_swapchain_images);

    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < m_n_swapchain_images;
                ++n_swapchain_image)
    {
        Anvil::DescriptorSetGroupUniquePtr new_dsg_ptr;

        {
            std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> new_dsg_create_info_ptr(1);

            new_dsg_create_info_ptr[0] = Anvil::DescriptorSetCreateInfo::create();

            new_dsg_create_info_ptr[0]->add_binding(0, /* in_binding */
                                                    Anvil::DescriptorType::STORAGE_BUFFER,
                                                    1, /* in_n_elements */
                                                    Anvil::ShaderStageFlagBits::VERTEX_BIT);

            new_dsg_ptr = Anvil::DescriptorSetGroup::create(m_device_ptr.get(),
                                                            new_dsg_create_info_ptr,
                                                            false); /* in_releaseable_sets */
        }

        new_dsg_ptr->set_binding_item(0, /* n_set         */
                                      0, /* binding_index */
                                      Anvil::DescriptorSet::StorageBufferBindingElement(m_properties_buffer_ptrs[n_swapchain_image].get() ));

        m_dsg_ptrs.push_back(
            std::move(new_dsg_ptr)
        );
    }
}

void App::init_events()
{
    /* Stub */
}

void App::init_gfx_pipelines()
{
    auto gfx_manager_ptr = m_device_ptr->get_graphics_pipeline_manager();

    for (uint32_t n_pipeline = 0;
                  n_pipeline < 2 /* ooo on, ooo off */;
                ++n_pipeline)
    {
        bool               is_ooo_disabled = (n_pipeline == 0);
        Anvil::PipelineID* pipeline_id_ptr = (is_ooo_disabled) ? &m_ooo_disabled_pipeline_id
                                                               : &m_ooo_enabled_pipeline_id;

        {
            auto pipeline_create_info_ptr = Anvil::GraphicsPipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                                      m_renderpasses[0].get(),
                                                                                      0, /* in_subpass_id */
                                                                                     *m_fs_entrypoint_ptr,
                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* in_gs_entrypoint */
                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* in_tc_entrypoint */
                                                                                      Anvil::ShaderModuleStageEntryPoint(), /* in_te_entrypoint */
                                                                                     *m_vs_entrypoint_ptr);

            pipeline_create_info_ptr->add_vertex_attribute(0, /* location */
                                                           Anvil::Format::R32G32B32_SFLOAT,
                                                           0,                 /* offset_in_bytes */
                                                           sizeof(float) * 3, /* stride_in_bytes */
                                                           Anvil::VertexInputRate::VERTEX);

            pipeline_create_info_ptr->set_descriptor_set_create_info(m_dsg_ptrs[0]->get_descriptor_set_create_info() );

            #ifdef ENABLE_MGPU_SUPPORT
            {
                pipeline_create_info_ptr->set_n_dynamic_scissor_boxes(1);
                pipeline_create_info_ptr->set_n_dynamic_viewports    (1);

                pipeline_create_info_ptr->toggle_dynamic_states(true, /* should_enable */
                                                                {Anvil::DynamicState::SCISSOR, Anvil::DynamicState::VIEWPORT});
            }
            #endif

            pipeline_create_info_ptr->set_primitive_topology      (Anvil::PrimitiveTopology::TRIANGLE_LIST);
            pipeline_create_info_ptr->set_rasterization_properties(Anvil::PolygonMode::FILL,
                                                                   Anvil::CullModeFlagBits::BACK_BIT,
                                                                   Anvil::FrontFace::CLOCKWISE,
                                                                   4.0f); /* line_width */
            pipeline_create_info_ptr->toggle_depth_test           (true, /* should_enable */
                                                                   Anvil::CompareOp::LESS);
            pipeline_create_info_ptr->toggle_depth_writes         (true);

            if (!is_ooo_disabled)
            {
                if (m_device_ptr->is_extension_enabled("VK_AMD_rasterization_order") )
                {
                    pipeline_create_info_ptr->set_rasterization_order(Anvil::RasterizationOrderAMD::RELAXED);
                }
            }
            else
            {
                pipeline_create_info_ptr->set_rasterization_order(Anvil::RasterizationOrderAMD::STRICT);
            }

            gfx_manager_ptr->add_pipeline(std::move(pipeline_create_info_ptr),
                                          pipeline_id_ptr);
        }
    }
}

void App::init_images()
{
    {
        auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(m_device_ptr.get(),
                                                                    Anvil::ImageType::_2D,
                                                                    Anvil::Format::D32_SFLOAT,
                                                                    Anvil::ImageTiling::OPTIMAL,
                                                                    Anvil::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
                                                                    m_window_ptr->get_width_at_creation_time (),
                                                                    m_window_ptr->get_height_at_creation_time(),
                                                                    1, /* base_mipmap_depth */
                                                                    1, /* n_layers */
                                                                    Anvil::SampleCountFlagBits::_1_BIT,
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    false, /* in_use_full_mipmap_chain */
                                                                    (m_device_ptr->get_type() == Anvil::DeviceType::MULTI_GPU) ? Anvil::MemoryFeatureFlagBits::MULTI_INSTANCE_BIT : Anvil::MemoryFeatureFlagBits::NONE,
                                                                    Anvil::ImageCreateFlagBits::NONE,
                                                                    Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, /* in_final_image_layout    */
                                                                    nullptr);                                             /* in_mipmaps_ptr           */

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

    #if defined(USE_LOCAL_SFR_PRESENTATION_MODE)
    {
        const Anvil::MGPUDevice* mgpu_device_ptr      (dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr.get() ));
        const uint32_t           n_physical_devices   (mgpu_device_ptr->get_n_physical_devices() );
        std::vector<uint32_t>    device_group_indices (n_physical_devices);

        /* Create peer images. */
        for (uint32_t n_physical_device = 0;
                      n_physical_device < n_physical_devices;
                    ++n_physical_device)
        {
            SwapchainPeerImages* current_peer_image_set_ptr (nullptr);

            m_swapchain_peer_images_per_physical_device.push_back(SwapchainPeerImages() );

            current_peer_image_set_ptr = &m_swapchain_peer_images_per_physical_device.back();

            for (uint32_t n_device_index = 0;
                          n_device_index < n_physical_devices;
                        ++n_device_index)
            {
                /* We want all logical devices to be able to access n_physical_device's image instance
                 * through this peer image */
                device_group_indices.at(n_device_index) = n_physical_device;
            }

            for (uint32_t n_swapchain_image = 0;
                          n_swapchain_image < m_n_swapchain_images;
                        ++n_swapchain_image)
            {
                Anvil::ImageUniquePtr peer_image_ptr;

                {
                    auto create_info_ptr = Anvil::ImageCreateInfo::create_peer_no_alloc(m_device_ptr.get   (),
                                                                                        m_swapchain_ptr.get(),
                                                                                        n_swapchain_image);

                    create_info_ptr->set_device_indices(n_physical_devices,
                                                       &device_group_indices.at(0) );

                    peer_image_ptr = Anvil::Image::create(std::move(create_info_ptr) );
                }

                current_peer_image_set_ptr->peer_images.push_back(std::move(peer_image_ptr) );
            }
        }

    }
    #endif
}

void App::init_query_pool()
{
    m_query_pool_ptr = Anvil::QueryPool::create_non_ps_query_pool(m_device_ptr.get(),
                                                                  VK_QUERY_TYPE_TIMESTAMP,
                                                                  m_n_swapchain_images * 2 /* top of pipe, bottom of pipe */);
}

void App::init_renderpasses()
{
    /* We are rendering directly to the swapchain image, so need one renderpass per image */
    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < m_n_swapchain_images;
                ++n_swapchain_image)
    {
        Anvil::RenderPassAttachmentID color_attachment_id;
        Anvil::RenderPassAttachmentID depth_attachment_id;
        Anvil::RenderPassUniquePtr    renderpass_ptr;
        Anvil::SubPassID              subpass_id;

        {
            Anvil::RenderPassCreateInfoUniquePtr renderpass_create_info_ptr(new Anvil::RenderPassCreateInfo(m_device_ptr.get() ) );

            renderpass_create_info_ptr->add_color_attachment(m_swapchain_ptr->get_create_info_ptr()->get_format(),
                                                             Anvil::SampleCountFlagBits::_1_BIT,
                                                             Anvil::AttachmentLoadOp::CLEAR,
                                                             Anvil::AttachmentStoreOp::STORE,

#ifndef ENABLE_OFFSCREEN_RENDERING
                                                             Anvil::ImageLayout::UNDEFINED,
    #if !defined(USE_LOCAL_SFR_PRESENTATION_MODE)
                                                             Anvil::ImageLayout::PRESENT_SRC_KHR,
    #else
                                                             /* In local SFR presentation mode, we want to avoid ->finalLayout transition, since some swapchain image instances
                                                              * will need to be switched to TRANSFER_SRC layout, and one of them to TRANSFER_DST.
                                                              */
                                                             Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
    #endif
#else
                                                             Anvil::ImageLayout::GENERAL,
                                                             Anvil::ImageLayout::GENERAL,
#endif
                                                             false, /* may_alias */
                                                            &color_attachment_id);

            renderpass_create_info_ptr->add_depth_stencil_attachment(m_depth_image_ptr->get_create_info_ptr()->get_format(),
                                                                     Anvil::SampleCountFlagBits::_1_BIT,
                                                                     Anvil::AttachmentLoadOp::CLEAR,
                                                                     Anvil::AttachmentStoreOp::STORE,
                                                                     Anvil::AttachmentLoadOp::DONT_CARE,
                                                                     Anvil::AttachmentStoreOp::DONT_CARE,
                                                                     Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                     Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                     false, /* may_alias */
                                                                    &depth_attachment_id);

            /* Define the only subpass we're going to use there */
            renderpass_create_info_ptr->add_subpass                         (&subpass_id);
            renderpass_create_info_ptr->add_subpass_color_attachment        (subpass_id,
                                                                             Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                             color_attachment_id,
                                                                             0,        /* in_location                      */
                                                                             nullptr); /* in_opt_attachment_resolve_id_ptr */
            renderpass_create_info_ptr->add_subpass_depth_stencil_attachment(subpass_id,
                                                                             Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                             depth_attachment_id);

            renderpass_ptr = Anvil::RenderPass::create(std::move(renderpass_create_info_ptr),
                                                       m_swapchain_ptr.get() );
        }

        renderpass_ptr->set_name_formatted("Renderpass for swapchain image [%d]",
                                           n_swapchain_image);

        /* If no general pipeline has been set up yet, do it now. This pipeline is only used to form a pipeline layout
         * so we only need to configure DSGs & attributes here.
         *
         * This layout will be compatible with actual OoO layouts we will be binding at frame rendering time.
         **/
        if (m_general_pipeline_id == -1)
        {
            auto gfx_manager_ptr              = m_device_ptr->get_graphics_pipeline_manager();
            auto gfx_pipeline_create_info_ptr = Anvil::GraphicsPipelineCreateInfo::create  (Anvil::PipelineCreateFlagBits::NONE,
                                                                                            renderpass_ptr.get(),
                                                                                            subpass_id,
                                                                                           *m_fs_entrypoint_ptr,
                                                                                            Anvil::ShaderModuleStageEntryPoint(), /* in_gs_entrypoint */
                                                                                            Anvil::ShaderModuleStageEntryPoint(), /* in_tc_entrypoint */
                                                                                            Anvil::ShaderModuleStageEntryPoint(), /* in_te_entrypoint */
                                                                                           *m_vs_entrypoint_ptr);

            gfx_pipeline_create_info_ptr->add_vertex_attribute          (0, /* location */
                                                                         Anvil::Format::R32G32B32_SFLOAT,
                                                                         0,                 /* offset_in_bytes */
                                                                         sizeof(float) * 3, /* stride_in_bytes */
                                                                         Anvil::VertexInputRate::VERTEX);
            gfx_pipeline_create_info_ptr->set_descriptor_set_create_info(m_dsg_ptrs[0]->get_descriptor_set_create_info() );

            gfx_manager_ptr->add_pipeline(std::move(gfx_pipeline_create_info_ptr),
                                         &m_general_pipeline_id);
        }

        m_renderpasses.push_back(
            std::move(renderpass_ptr)
        );

        /* Set up a framebuffer we will use for the renderpass */
        Anvil::FramebufferUniquePtr framebuffer_ptr;

        {
            auto create_info_ptr = Anvil::FramebufferCreateInfo::create(m_device_ptr.get(),
                                                                        m_window_ptr->get_width_at_creation_time (),
                                                                        m_window_ptr->get_height_at_creation_time(),
                                                                        1); /* n_layers */

            #if defined(ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING)
            {
                create_info_ptr->add_attachment(m_swapchain_image_views.at(n_swapchain_image).get(),
                                                nullptr);
            }
            #else
            {
                create_info_ptr->add_attachment(m_swapchain_ptr->get_image_view(n_swapchain_image),
                                                nullptr);
            }
            #endif

            create_info_ptr->add_attachment(m_depth_image_view_ptr.get(),
                                            nullptr);

            framebuffer_ptr = Anvil::Framebuffer::create(std::move(create_info_ptr) );
        }

        framebuffer_ptr->set_name("Main framebuffer");


        m_framebuffers.push_back(
            std::move(framebuffer_ptr)
        );
    }
}

void App::init_semaphores()
{
    uint32_t n_physical_devices(0);

    switch (m_device_ptr->get_type() )
    {
        case Anvil::DeviceType::MULTI_GPU:
        {
            const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr.get() ) );

            n_physical_devices = mgpu_device_ptr->get_n_physical_devices();

            break;
        }

        case Anvil::DeviceType::SINGLE_GPU:
        {
            n_physical_devices = 1;

            break;
        }

        default:
        {
            anvil_assert(false);
        }
    }

    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < m_n_swapchain_images;
                ++n_swapchain_image)
    {
        Anvil::SemaphoreUniquePtr new_frame_acquisition_wait_semaphore_ptr;
        auto                      new_signal_sem_bundle_ptr                = std::unique_ptr<SemaphoreBundle>(new SemaphoreBundle() );
        auto                      new_wait_sem_bundle_ptr                  = std::unique_ptr<SemaphoreBundle>(new SemaphoreBundle() );

        {
            auto create_info_ptr = Anvil::SemaphoreCreateInfo::create(m_device_ptr.get() );

            new_frame_acquisition_wait_semaphore_ptr = Anvil::Semaphore::create(std::move(create_info_ptr) );
        }

        new_frame_acquisition_wait_semaphore_ptr->set_name_formatted("New frame acquisition wait semaphore [%d]",
                                                                     n_swapchain_image);

        #if defined(ENABLE_MGPU_SUPPORT)
        {
            m_frame_acquisition_wait_semaphores.push_back(std::move(new_frame_acquisition_wait_semaphore_ptr) );
        }
        #endif

        for (uint32_t n_physical_device = 0;
                      n_physical_device < n_physical_devices;
                    ++n_physical_device)
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
                                                         n_swapchain_image);
            new_wait_semaphore_ptr->set_name_formatted  ("Wait semaphore [%d]",
                                                         n_swapchain_image);

            new_signal_sem_bundle_ptr->semaphores.push_back(std::move(new_signal_semaphore_ptr ));
            new_wait_sem_bundle_ptr->semaphores.push_back  (std::move(new_wait_semaphore_ptr) );
        }

        m_frame_signal_semaphore_bundles.push_back(std::move(new_signal_sem_bundle_ptr) );
        m_frame_wait_semaphore_bundles.push_back  (std::move(new_wait_sem_bundle_ptr) );
    }
}

void App::init_shaders()
{
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr fs_ptr;
    Anvil::ShaderModuleUniquePtr               fs_sm_ptr;
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr vs_ptr;
    Anvil::ShaderModuleUniquePtr               vs_sm_ptr;

    fs_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                       Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                       fs_body,
                                                       Anvil::ShaderStage::FRAGMENT);
    vs_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                       Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                       vs_body,
                                                       Anvil::ShaderStage::VERTEX);

    vs_ptr->add_definition_value_pair("MAX_DEPTH",
                                      MAX_DEPTH);
    vs_ptr->add_definition_value_pair("RT_HEIGHT",
                                      m_window_ptr->get_height_at_creation_time() );
    vs_ptr->add_definition_value_pair("RT_WIDTH",
                                      m_window_ptr->get_width_at_creation_time() );
    vs_ptr->add_definition_value_pair("N_TEAPOTS",
                                      N_TEAPOTS);

    fs_sm_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get(),
                                                                 fs_ptr.get      () );
    vs_sm_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get(),
                                                                 vs_ptr.get      () );

    fs_sm_ptr->set_name("Fragment shader");
    vs_sm_ptr->set_name("Vertex shader");

    m_fs_entrypoint_ptr.reset(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                     std::move(fs_sm_ptr),
                                                                     Anvil::ShaderStage::FRAGMENT) );
    m_vs_entrypoint_ptr.reset(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                     std::move(vs_sm_ptr),
                                                                     Anvil::ShaderStage::VERTEX) );
}

void App::init_swapchain()
{
    static const Anvil::Format          swapchain_format      (Anvil::Format::B8G8R8A8_UNORM);
    static const Anvil::PresentModeKHR  swapchain_present_mode(Anvil::PresentModeKHR::FIFO_KHR);
    static const Anvil::ImageUsageFlags swapchain_usage       (Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT);

    {
        auto create_info_ptr = Anvil::RenderingSurfaceCreateInfo::create(m_instance_ptr.get(),
                                                                         m_device_ptr.get  (),
                                                                         m_window_ptr.get  () );

        m_rendering_surface_ptr = Anvil::RenderingSurface::create(std::move(create_info_ptr) );
    }

    m_rendering_surface_ptr->set_name("Main rendering surface");


    switch (m_device_ptr->get_type() )
    {
#if defined(ENABLE_MGPU_SUPPORT)
    case Anvil::DeviceType::MULTI_GPU:
        {
            Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<Anvil::MGPUDevice*>(m_device_ptr.get() ) );

            #if defined(USE_SUM_SFR_PRESENTATION_MODE)
                static const Anvil::DeviceGroupPresentModeFlagBits swapchain_device_group_present_mode(Anvil::DeviceGroupPresentModeFlagBits::SUM_BIT_KHR);
            #elif defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_LOCAL_SFR_PRESENTATION_MODE)
                static const Anvil::DeviceGroupPresentModeFlagBits swapchain_device_group_present_mode(Anvil::DeviceGroupPresentModeFlagBits::LOCAL_BIT_KHR);
            #elif defined(USE_REMOTE_AFR_PRESENTATION_MODE)
                static const Anvil::DeviceGroupPresentModeFlagBits swapchain_device_group_present_mode(Anvil::DeviceGroupPresentModeFlagBits::REMOTE_BIT_KHR);
            #else
                #error Not supported?
            #endif

            anvil_assert((mgpu_device_ptr->get_supported_present_modes() & swapchain_device_group_present_mode) != 0);

            m_swapchain_ptr = mgpu_device_ptr->create_swapchain(m_rendering_surface_ptr.get(),
                                                                m_window_ptr.get           (),
                                                                swapchain_format,
                                                                Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,
                                                                swapchain_present_mode,
                                                                swapchain_usage,
                                                                m_n_swapchain_images,
#if defined(ENABLE_EXPLICIT_SFR_RECT_DEFINITIONS)
                                                                true, /* support_SFR */
#else
                                                                false, /* support_SFR */
#endif
                                                                swapchain_device_group_present_mode);

            m_n_swapchain_images = m_swapchain_ptr->get_n_images();

            #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_LOCAL_SFR_PRESENTATION_MODE)
            {
                /* This example app assumes all physical devices can be used to render & present their output
                 * using local presentation mode. Verify this assertion
                 */
                anvil_assert((mgpu_device_ptr->get_supported_present_modes_for_surface(m_rendering_surface_ptr.get() ) & Anvil::DeviceGroupPresentModeFlagBits::LOCAL_BIT_KHR) != 0);
            }
            #elif defined(USE_REMOTE_AFR_PRESENTATION_MODE)
                /* This example app assumes all physical devices can be used to render & present their output
                 * using remote presentation mode. Verify this assertion
                 */
                anvil_assert((mgpu_device_ptr->get_supported_present_modes_for_surface(m_rendering_surface_ptr.get() ) & Anvil::DeviceGroupPresentModeFlagBits::REMOTE_BIT_KHR) != 0);
            #elif defined(USE_SUM_SFR_PRESENTATION_MODE)
                /* This example app assumes all physical devices can be used to render & present their output
                 * using sum presentation mode. Verify this assertion
                 */
                anvil_assert((mgpu_device_ptr->get_supported_present_modes_for_surface(m_rendering_surface_ptr.get() ) & Anvil::DeviceGroupPresentModeFlagBits::SUM_BIT_KHR) != 0);
            #endif

            #if defined(ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING)
            {
                for (uint32_t n_swapchain_image = 0;
                              n_swapchain_image < m_n_swapchain_images;
                            ++n_swapchain_image)
                {
                    Anvil::ImageUniquePtr     new_image_ptr;
                    Anvil::ImageViewUniquePtr new_image_view_ptr;

                    #if defined(ENABLE_EXPLICIT_SFR_RECT_DEFINITIONS)
                    {
                        /* For split-frame rendering, the rendering surface is split into N vertical areas, where
                         * N corresponds to the number of logical devices. Since GPU at index X will always render
                         * only to a dedicated slab stored in its own memory instance, we are going to use dummy
                         * rects for all other cases.
                         *
                         * As a refresh, SFR rect at index i * N + j is the rectangle used by physical device i for memory instance j.
                         *
                         */
                        const uint32_t        n_logical_devices(mgpu_device_ptr->get_n_physical_devices() );
                        const auto            render_areas     (get_render_areas() );
                        std::vector<VkRect2D> sfr_rects        (n_logical_devices * n_logical_devices);

                        for (uint32_t n_logical_device = 0;
                                      n_logical_device < n_logical_devices;
                                    ++n_logical_device)
                        {
                            for (uint32_t n_memory_instance = 0;
                                          n_memory_instance < n_logical_devices;
                                        ++n_memory_instance)
                            {
                                const uint32_t n_sfr_rect       = n_logical_device * n_logical_devices + n_memory_instance;
                                auto&          current_sfr_rect = sfr_rects.at(n_sfr_rect);

                                if (n_logical_device != n_memory_instance)
                                {
                                    current_sfr_rect.extent.height = 0;
                                    current_sfr_rect.extent.width  = 0;
                                    current_sfr_rect.offset.x      = 0;
                                    current_sfr_rect.offset.y      = 0;
                                }
                                else
                                {
                                    current_sfr_rect = render_areas.at(n_logical_device);
                                }
                            }
                        }

                        {
                            auto create_info_ptr = Anvil::ImageCreateInfo::create_peer_no_alloc(m_device_ptr.get   (),
                                                                                                m_swapchain_ptr.get(),
                                                                                                n_swapchain_image);

                            create_info_ptr->set_SFR_rectangles(static_cast<uint32_t>(sfr_rects.size() ),
                                                               &sfr_rects.at(0) );

                            new_image_ptr = Anvil::Image::create(std::move(create_info_ptr) );
                        }
                    }
                    #else
                    {
                        /* No ISV would ever do this, right? */
                        auto create_info_ptr = Anvil::ImageCreateInfo::create_peer_no_alloc(m_device_ptr.get   (),
                                                                                            m_swapchain_ptr.get(),
                                                                                            n_swapchain_image);

                        new_image_ptr = Anvil::Image::create(std::move(create_info_ptr) );
                    }
                    #endif

                    {
                        auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(m_device_ptr.get (),
                                                                                     new_image_ptr.get(),
                                                                                     0, /* n_base_layer        */
                                                                                     0, /* n_base_mipmap_level */
                                                                                     1, /* n_mipmaps           */
                                                                                     Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                                     new_image_ptr->get_create_info_ptr()->get_format(),
                                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                                     Anvil::ComponentSwizzle::IDENTITY);

                        new_image_view_ptr = Anvil::ImageView::create(std::move(create_info_ptr) );
                    }

                    m_swapchain_image_views.push_back(std::move(new_image_view_ptr) );
                    m_swapchain_images.push_back     (std::move(new_image_ptr) );
                }
            }
            #endif

            /* Cache the queue we are going to use for presentation */
            for (uint32_t n_physical_device = 0;
                          n_physical_device < mgpu_device_ptr->get_n_physical_devices();
                        ++n_physical_device)
            {
                const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

                if (!m_rendering_surface_ptr->get_queue_families_with_present_support(mgpu_device_ptr->get_physical_device(n_physical_device),
                                                                                     &present_queue_fams_ptr) )
                {
                    continue;
                }

                if (present_queue_fams_ptr->size() > 0)
                {
                    m_present_queue_ptr = m_device_ptr->get_queue_for_queue_family_index(present_queue_fams_ptr->at(0),
                                                                                         0); /* in_n_queue */

                    break;
                }
            }

            anvil_assert(m_present_queue_ptr != nullptr);

            break;
        }
#endif /* ENABLE_MGPU_SUPPORT */

        case Anvil::DeviceType::SINGLE_GPU:
        {
            Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<Anvil::SGPUDevice*>(m_device_ptr.get() ) );

            m_swapchain_ptr = sgpu_device_ptr->create_swapchain(m_rendering_surface_ptr.get(),
                                                                m_window_ptr.get           (),
                                                                swapchain_format,
                                                                Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,
                                                                swapchain_present_mode,
                                                                swapchain_usage,
                                                                m_n_swapchain_images);

            /* Cache the queue we are going to use for presentation */
            const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

            if (!m_rendering_surface_ptr->get_queue_families_with_present_support(sgpu_device_ptr->get_physical_device(),
                                                                                 &present_queue_fams_ptr) )
            {
                anvil_assert_fail();
            }

            m_present_queue_ptr = sgpu_device_ptr->get_queue_for_queue_family_index(present_queue_fams_ptr->at(0),
                                                                                    0); /* in_n_queue */

            break;
        }

        default:
        {
            anvil_assert(false);
        }
    }
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
                                                       "OutOfOrderRasterization example",
                                                       WINDOW_WIDTH,
                                                       WINDOW_HEIGHT,
                                                       true, /* in_closable */
                                                       std::bind(&App::draw_frame,
                                                                 this) );

    /* Sign up for keypress notifications */
    m_window_ptr->register_for_callbacks(Anvil::WINDOW_CALLBACK_ID_KEYPRESS_RELEASED,
                                         std::bind(&App::on_keypress_event,
                                                   this,
                                                   std::placeholders::_1),
                                         this);
}

void App::init_vulkan()
{
    /* Create a Vulkan instance */
    {
        auto create_info_ptr = Anvil::InstanceCreateInfo::create("OutOfOrderRasterization",  /* app_name    */
                                                                 "OutOfOrderRasterization",  /* engine_name */
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

    /* Determine which extensions we need to request for */
    #if !defined(ENABLE_MGPU_SUPPORT)
    {
        /* Create a Vulkan device */
        auto create_info_ptr = Anvil::DeviceCreateInfo::create_sgpu(m_instance_ptr->get_physical_device(0),
                                                                    true,                                    /* in_enable_shader_module_cache */
                                                                    Anvil::DeviceExtensionConfiguration(),
                                                                    std::vector<std::string>(),              /* in_layers */
                                                                    Anvil::CommandPoolCreateFlagBits::NONE,
                                                                    false);                                  /* in_mt_safe */

        m_device_ptr = Anvil::SGPUDevice::create(std::move(create_info_ptr) );
    }
    #else
    {
        Anvil::DeviceExtensionConfiguration ext_config;
        auto                                physical_devices = m_instance_ptr->get_physical_device_group(0).physical_device_ptrs;

        ext_config.extension_status[VK_KHR_DEVICE_GROUP_EXTENSION_NAME]  = Anvil::ExtensionAvailability::REQUIRE;
        ext_config.extension_status[VK_KHR_BIND_MEMORY_2_EXTENSION_NAME] = Anvil::ExtensionAvailability::REQUIRE;

        auto create_info_ptr = Anvil::DeviceCreateInfo::create_mgpu(physical_devices,
                                                                    true,                                   /* in_enable_shader_module_cache */
                                                                    ext_config,
                                                                    std::vector<std::string>(),             /* layers */
                                                                    Anvil::CommandPoolCreateFlagBits::NONE,
                                                                    false);                                 /* in_mt_safe */

        m_device_ptr = Anvil::MGPUDevice::create(std::move(create_info_ptr) );
    }
    #endif
}

void App::on_keypress_event(Anvil::CallbackArgument* callback_data_raw_ptr)
{
    auto callback_data_ptr = static_cast<Anvil::OnKeypressReleasedCallbackArgument*>(callback_data_raw_ptr);

    #ifndef ENABLE_OFFSCREEN_RENDERING
    {
        if (callback_data_ptr->released_key_id == Anvil::KEY_ID_SPACE)
        {
            printf("\n\n");

            if (m_device_ptr->is_extension_enabled("VK_AMD_rasterization_order") )
            {
                m_ooo_enabled = !m_ooo_enabled;

                /* Note: this code should be wrapped in a critical section */
                {
                    m_timestamp_deltas.clear();
                }

                printf("[!] Now using %s rasterization order.\n\n",
                       (m_ooo_enabled) ? "relaxed" : "strict");
            }
            else
            {
                printf("[!] This device does not support VK_AMD_rasterization_order extension; running in strict rasterization mode only.\n\n");
            }
        }
        else
        if (callback_data_ptr->released_key_id == 'r' || callback_data_ptr->released_key_id == 'R')
        {
            m_should_rotate = !m_should_rotate;
        }
    }
    #endif
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
    #ifndef ENABLE_OFFSCREEN_RENDERING
    {
        printf("While focused on the window, press:\n"
               "\n"
               " r     - to pause or resume rotation.\n"
               " space - to switch between relaxed & strict rasterization.\n"
               "\n");
    }
    #endif

    m_window_ptr->run();
}

void App::update_fps()
{
    uint64_t average_delta = 0;

    /* Compute average delta from all the samples we have cached so far */
    for (uint64_t delta : m_timestamp_deltas)
    {
        average_delta += delta;
    }

    average_delta /= m_timestamp_deltas.size();

    /* Convert the delta to human-readable information */
    const double time_ns     = double(average_delta) * double(m_device_ptr->get_physical_device_properties().core_vk1_0_properties_ptr->limits.timestamp_period);
    const double time_s      = time_ns      / NSEC_PER_SEC;
    const float  average_fps = float(1.0    / time_s);

    /* Print the new info */
    clear_console_line();

    #if !defined(ENABLE_MGPU_SUPPORT)
        printf("Average FPS: %.3f", average_fps);
    #else
        printf("Average FPS for all GPUs: %.3f", average_fps);
    #endif

    /* Purge the timestamps */
    m_timestamp_deltas.clear();
}

void App::update_teapot_props(uint32_t n_current_swapchain_image)
{
    static uint32_t n_call     = 0;
    const  uint32_t n_teapots  = N_TEAPOTS;

    /* NOTE: For fluent animation, time_msec would need to take screen refresh rate into account */
    const uint64_t time_msec  = m_time.get_time_in_msec();

    const float span_pos_x = float(MAX_TEAPOT_X - MIN_TEAPOT_X);
    const float span_pos_y = float(MAX_TEAPOT_Y - MIN_TEAPOT_Y);
    const float span_pos_z = float(MAX_TEAPOT_Z - MIN_TEAPOT_Z);

    for (uint32_t n_teapot = 0;
                  n_teapot < n_teapots;
                ++n_teapot)
    {
        float* teapot_pos_size_data_ptr = m_teapot_props_data_ptr.get() + (n_teapot)             * 4 /* xyz + size */;
        float* teapot_rot_data_ptr      = m_teapot_props_data_ptr.get() + (n_teapots + n_teapot) * 4 /* xyz + stub  */;

        if (!m_properties_data_set)
        {
            teapot_pos_size_data_ptr[0] =   float(MIN_TEAPOT_X) + span_pos_x * float(rand() % RAND_MAX) / float(RAND_MAX);
            teapot_pos_size_data_ptr[1] =   float(MIN_TEAPOT_Y) + span_pos_y * float(rand() % RAND_MAX) / float(RAND_MAX);
            teapot_pos_size_data_ptr[2] = -(float(MIN_TEAPOT_Z) + span_pos_z * float(rand() % RAND_MAX) / float(RAND_MAX));
            teapot_pos_size_data_ptr[3] =   float(rand() % RAND_MAX) / float(RAND_MAX);
        }

        if (m_should_rotate)
        {
            teapot_rot_data_ptr[0] = 0.0f;
            teapot_rot_data_ptr[1] = (float(n_teapot * 48 + time_msec) / 1000.0f) / 15.0f * 2.0f * 3.14152965f;
            teapot_rot_data_ptr[2] = (float(n_teapot * 75 + time_msec) / 1000.0f) / 5.0f  * 2.0f * 3.14152965f;
            teapot_rot_data_ptr[3] = 0.0f;
        }
    }

    if (!m_properties_data_set)
    {
        m_properties_data_set = true;

        for (uint32_t n_swapchain_image = 0;
                      n_swapchain_image < m_n_swapchain_images;
                    ++n_swapchain_image)
        {
            m_properties_buffer_ptrs[n_swapchain_image]->write(0, /* start_offset */
                                                               N_TEAPOTS * sizeof(float) * 8, /* pos + rot */
                                                               m_teapot_props_data_ptr.get() );
        }
    }
    else
    {
        /* Only need to update rotation data */
        const uint32_t rot_data_offset = N_TEAPOTS * 4 /* pos */ * sizeof(float);

        m_properties_buffer_ptrs[n_current_swapchain_image]->write(rot_data_offset,
                                                                   N_TEAPOTS * sizeof(float) * 4 /* pos */,
                                                                   m_teapot_props_data_ptr.get() + rot_data_offset / sizeof(float) );

    }

    ++n_call;
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

