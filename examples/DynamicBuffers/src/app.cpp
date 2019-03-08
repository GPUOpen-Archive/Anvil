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
#include "misc/compute_pipeline_create_info.h"
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
#include "wrappers/compute_pipeline_manager.h"
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
#define APP_NAME "Dynamic buffers example"

/** Total number of sines to draw. */
#define N_SINE_PAIRS (4)

/** Number of vertices to approximate sine shape with. */
#define N_VERTICES_PER_SINE (128)

#define WINDOW_WIDTH  (1280)
#define WINDOW_HEIGHT (720)

/* When offscreen rendering is enabled, N_FRAMES_TO_RENDER tells how many frames should be
 * rendered before leaving */
#define N_FRAMES_TO_RENDER (8)


static const char* g_glsl_consumer_frag =
    "#version 430\n"
    "\n"
    "layout(location = 0) flat in  vec4 fs_color;\n"
    "layout(location = 0)      out vec4 result;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    result = fs_color;\n"
    "}\n";

static const char* g_glsl_consumer_vert =
    "#version 430\n"
    "\n"
    "layout(location = 0)      in  vec4 in_color;\n"
    "layout(location = 0) flat out vec4 fs_color;\n"
    "\n"
    "\n"
    "layout(std430, set = 0, binding = 0) restrict readonly buffer evenSineSB\n"
    "{\n"
    "    vec4 vertex_sine1[N_VERTICES_PER_SINE];\n"
    "};\n"
    "\n"
    "layout(std430, set = 1, binding = 0) restrict readonly buffer oddSineSB\n"
    "{\n"
    "    vec4 vertex_sine2[N_VERTICES_PER_SINE];\n"
    "};\n"
    "\n"
    "void main()\n"
    "{\n"
    "    fs_color = in_color;\n"
    "\n"
    "    switch (gl_InstanceIndex % 2)\n"
    "    {\n"
    "        case 0: gl_Position = vertex_sine1[gl_VertexIndex]; break;\n"
    "        case 1: gl_Position = vertex_sine2[gl_VertexIndex];\n"
    "    }\n"
    "}\n";

static const char* g_glsl_producer_comp =
    "#version 310 es\n"
    "\n"
    "layout(local_size_x = N_VERTICES_PER_SINE, local_size_y = 1, local_size_z = 1) in;\n"
    "\n"
    "\n"
    "layout(std140, set = 0, binding = 0) readonly restrict buffer dataOffsetBlock\n"
    "{\n"
    "    vec2 offsets;\n"
    "};\n"
    "layout(std140, set = 0, binding = 1) restrict writeonly buffer sineSB\n"
    "{\n"
    "    vec4 data[N_VERTICES_PER_SINE * 2];\n"
    "} result_vertex_sine;\n"
    "\n"
    "layout(std140, set = 1, binding = 0) uniform propsBlock\n"
    "{\n"
    "    float t;\n"
    "};\n"
    "\n"
    "layout (push_constant) uniform pushConstants\n"
    "{\n"
    "    int n_sine_pair;\n"
    "} pc;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    int   current_invocation_id = int(gl_GlobalInvocationID.x);\n"
    "    int   curve_index           = current_invocation_id / N_VERTICES_PER_SINE;\n"
    "    float result_y;\n"
    "    float result_z;\n"
    "    float x_normalized          = float(current_invocation_id % N_VERTICES_PER_SINE) / float(N_VERTICES_PER_SINE - 1);\n"
    "    float x                     = x_normalized * 3.14152965 * 2.0;\n"
    "\n"
    "    if (curve_index > 1)\n"
    "    {\n"
    "        return;\n"
    "    }\n"
    "\n"
    "    if (curve_index == 0)\n"
    "    {\n"
    "        result_y = sin(mod(t + offsets[0] + x, 3.14152965 * 2.0) );\n"
    "        result_z = float(2 * pc.n_sine_pair) / float(N_SINE_PAIRS * 2);\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "        result_y = sin(t + offsets[1] + x);\n"
    "        result_z = float(1 + 2 * pc.n_sine_pair) / float(N_SINE_PAIRS * 2);\n"
    "    }\n"
    "\n"
    "    result_vertex_sine.data[current_invocation_id] = vec4((2.0 * x_normalized - 1.0), result_y, result_z, 1.0);\n"
    "}\n";


App::App()
    :m_consumer_pipeline_id (UINT32_MAX),
     m_n_last_semaphore_used(0),
     m_n_swapchain_images   (N_SWAPCHAIN_IMAGES),
     m_producer_pipeline_id (UINT32_MAX)
{
    // ..
}

App::~App()
{
    deinit();
}

void App::deinit()
{
    auto compute_pipeline_manager_ptr = m_device_ptr->get_compute_pipeline_manager ();
    auto gfx_pipeline_manager_ptr     = m_device_ptr->get_graphics_pipeline_manager();

    Anvil::Vulkan::vkDeviceWaitIdle(m_device_ptr->get_device_vk() );

    if (m_consumer_pipeline_id != UINT32_MAX)
    {
        gfx_pipeline_manager_ptr->delete_pipeline(m_consumer_pipeline_id);

        m_consumer_pipeline_id = UINT32_MAX;
    }

    if (m_producer_pipeline_id != UINT32_MAX)
    {
        compute_pipeline_manager_ptr->delete_pipeline(m_producer_pipeline_id);

        m_producer_pipeline_id = UINT32_MAX;
    }

    m_frame_signal_semaphores.clear();
    m_frame_wait_semaphores.clear();

    for (uint32_t n_cmd_buffer = 0;
                  n_cmd_buffer < sizeof(m_command_buffers) / sizeof(m_command_buffers[0]);
                ++n_cmd_buffer)
    {
        m_command_buffers[n_cmd_buffer] = nullptr;
    }

    for (uint32_t n_depth_image = 0;
                  n_depth_image < sizeof(m_depth_images) / sizeof(m_depth_images[0]);
                ++n_depth_image)
    {
        m_depth_images[n_depth_image] = nullptr;
    }

    for (uint32_t n_depth_image_view = 0;
                  n_depth_image_view < sizeof(m_depth_image_views) / sizeof(m_depth_image_views[0]);
                ++n_depth_image_view)
    {
        m_depth_image_views[n_depth_image_view] = nullptr;
    }

    for (uint32_t n_fbo = 0;
                  n_fbo < sizeof(m_fbos) / sizeof(m_fbos[0]);
                ++n_fbo)
    {
        m_fbos[n_fbo] = nullptr;
    }

    m_consumer_dsg_ptr.reset();
    m_consumer_fs_ptr.reset();
    m_consumer_render_pass_ptr.reset();
    m_consumer_vs_ptr.reset();
    m_producer_cs_ptr.reset();
    m_producer_dsg_ptr.reset();
    m_sine_color_buffer_ptr.reset();
    m_sine_data_buffer_ptr.reset();
    m_sine_offset_data_buffer_ptr.reset();
    m_sine_props_data_buffer_ptr.reset();

    m_rendering_surface_ptr.reset();
    m_swapchain_ptr.reset();
    m_window_ptr.reset();

    m_device_ptr.reset();

    m_instance_ptr.reset();
}

void App::draw_frame()
{
    Anvil::Semaphore*               curr_frame_signal_semaphore_ptr = nullptr;
    Anvil::Semaphore*               curr_frame_wait_semaphore_ptr   = nullptr;
    static uint32_t                 n_frames_rendered               = 0;
    uint32_t                        n_swapchain_image;
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

    /* Update time value, used by the generator compute shader */
    const uint64_t time_msec = m_time.get_time_in_msec();
    const float    t         = time_msec / 1000.0f;

    m_sine_props_data_buffer_ptr->write(m_sine_props_data_buffer_size_per_swapchain_image * n_swapchain_image, /* start_offset */
                                        sizeof(float),                                                         /* size         */
                                       &t);

    /* Submit jobs to relevant queues and make sure they are correctly synchronized */
    m_device_ptr->get_universal_queue(0)->submit(
        Anvil::SubmitInfo::create_wait_execute_signal(m_command_buffers[n_swapchain_image].get(),
                                                       1, /* n_semaphores_to_signal */
                                                      &curr_frame_signal_semaphore_ptr,
                                                       1, /* n_semaphores_to_wait_on */
                                                      &curr_frame_wait_semaphore_ptr,
                                                      &wait_stage_mask,
                                                       false) /* should_block */
    );

    {
        Anvil::SwapchainOperationErrorCode present_result = Anvil::SwapchainOperationErrorCode::DEVICE_LOST;

        m_present_queue_ptr->present(m_swapchain_ptr.get(),
                                     n_swapchain_image,
                                     1, /* n_wait_semaphores */
                                    &present_wait_semaphore_ptr,
                                    &present_result);

        anvil_assert(present_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
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
    init_images    ();
    init_semaphores();
    init_shaders   ();

    init_compute_pipelines();
    init_framebuffers     ();
    init_gfx_pipelines    ();
    init_command_buffers  ();
}

void App::get_buffer_memory_offsets(uint32_t  n_sine_pair,
                                    uint32_t* out_opt_sine1SB_offset_ptr,
                                    uint32_t* out_opt_sine2SB_offset_ptr,
                                    uint32_t* out_opt_offset_data_offset_ptr)
{
    if (out_opt_sine1SB_offset_ptr != nullptr)
    {
        *out_opt_sine1SB_offset_ptr = static_cast<uint32_t>(m_sine_data_buffer_offsets[n_sine_pair * 2]);
    }

    if (out_opt_sine2SB_offset_ptr != nullptr)
    {
        *out_opt_sine2SB_offset_ptr = static_cast<uint32_t>(m_sine_data_buffer_offsets[n_sine_pair * 2 + 1]);
    }

    if (out_opt_offset_data_offset_ptr != nullptr)
    {
        const uint32_t sb_offset_alignment = static_cast<uint32_t>(m_physical_device_ptr->get_device_properties().core_vk1_0_properties_ptr->limits.min_storage_buffer_offset_alignment);

        *out_opt_offset_data_offset_ptr = Anvil::Utils::round_up(static_cast<uint32_t>(sizeof(float) * 2),
                                                                 sb_offset_alignment) * n_sine_pair;
    }
}

void App::init_buffers()
{
    Anvil::MemoryAllocatorUniquePtr memory_allocator_ptr;
    const VkDeviceSize              sb_data_alignment_requirement = m_physical_device_ptr->get_device_properties().core_vk1_0_properties_ptr->limits.min_storage_buffer_offset_alignment;
    std::unique_ptr<char[]>         sine_offset_data_raw_buffer_ptr;

    /* Set up allocators */
    memory_allocator_ptr = Anvil::MemoryAllocator::create_oneshot(m_device_ptr.get() );

    /* Prepare sine offset data */
    m_sine_offset_data_buffer_size = 0;

    for (uint32_t n_sine_pair = 0;
                  n_sine_pair < N_SINE_PAIRS + 1;
                ++n_sine_pair)
    {
        if (n_sine_pair < N_SINE_PAIRS)
        {
            /* Store current data offset */
            anvil_assert( (m_sine_offset_data_buffer_size % sb_data_alignment_requirement) == 0);

            m_sine_offset_data_buffer_offsets.push_back(m_sine_offset_data_buffer_size);
        }

        /* Account for space necessary to hold a vec2 and any padding required to meet the alignment requirement */
        m_sine_offset_data_buffer_size += sizeof(float) * 2;
        m_sine_offset_data_buffer_size += (sb_data_alignment_requirement - m_sine_offset_data_buffer_size % sb_data_alignment_requirement) % sb_data_alignment_requirement;
    }

    sine_offset_data_raw_buffer_ptr.reset( new char[static_cast<uintptr_t>(m_sine_offset_data_buffer_size)]);

    for (uint32_t n_sine_pair = 0;
                  n_sine_pair < N_SINE_PAIRS;
                ++n_sine_pair)
    {
        float* sine_pair_data_ptr = (float*) (sine_offset_data_raw_buffer_ptr.get() + m_sine_offset_data_buffer_offsets[n_sine_pair]);

        /* Compute the sine start offsets */
        *sine_pair_data_ptr = -float(2 * (n_sine_pair + 1) );
         sine_pair_data_ptr++;

        *sine_pair_data_ptr = -float(2 * (n_sine_pair + 1) + 1);
    }

    /* Prepare a buffer object to hold the sine offset data. Note that we fill it with data
     * after memory allocator actually assigns it a memory block.
     */
    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        m_sine_offset_data_buffer_size,
                                                                        Anvil::QueueFamilyFlagBits::COMPUTE_BIT | Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::CONCURRENT,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::STORAGE_BUFFER_BIT);

        m_sine_offset_data_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    m_sine_offset_data_buffer_ptr->set_name("Sine offset data buffer");

    memory_allocator_ptr->add_buffer(m_sine_offset_data_buffer_ptr.get(),
                                     Anvil::MemoryFeatureFlagBits::NONE); /* in_required_memory_features */

    /* Now prepare a memory block which is going to hold vertex data generated by
     * the producer CS */
    m_sine_data_buffer_size = 0;

    for (unsigned int n_sine_pair = 0;
                      n_sine_pair < N_SINE_PAIRS;
                    ++n_sine_pair)
    {
        for (unsigned int n_sine = 0;
                          n_sine < 2; /* sines in a pair */
                        ++n_sine)
        {
            /* Store current offset */
            m_sine_data_buffer_offsets.push_back(m_sine_data_buffer_size);

            /* Account for space necessary to hold the sine data */
            m_sine_data_buffer_size += sizeof(float) * 4 /* components */ * N_VERTICES_PER_SINE;

            /* Pad up as necessary */
            m_sine_data_buffer_size += (sb_data_alignment_requirement - m_sine_data_buffer_size % sb_data_alignment_requirement) % sb_data_alignment_requirement;

            anvil_assert(m_sine_data_buffer_size % sb_data_alignment_requirement == 0);
        }
    }

    m_sine_data_buffer_size *= 2;

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        m_sine_data_buffer_size,
                                                                        Anvil::QueueFamilyFlagBits::COMPUTE_BIT | Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::CONCURRENT,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::STORAGE_BUFFER_BIT);

        m_sine_data_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    m_sine_data_buffer_ptr->set_name("Sine data buffer");

    memory_allocator_ptr->add_buffer(m_sine_data_buffer_ptr.get(),
                                     Anvil::MemoryFeatureFlagBits::NONE);

    /* We also need some space for a uniform block which is going to hold time info. */
    const auto dynamic_ub_alignment_requirement                = m_device_ptr->get_physical_device_properties().core_vk1_0_properties_ptr->limits.min_uniform_buffer_offset_alignment;
    const auto sine_props_data_buffer_size_per_swapchain_image = Anvil::Utils::round_up(static_cast<decltype(dynamic_ub_alignment_requirement)>(sizeof(float)),
                                                                                        dynamic_ub_alignment_requirement);
    const auto sine_props_data_buffer_size_total               = sine_props_data_buffer_size_per_swapchain_image * N_SWAPCHAIN_IMAGES;

    m_sine_props_data_buffer_size_per_swapchain_image = sine_props_data_buffer_size_per_swapchain_image;

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        sine_props_data_buffer_size_total,
                                                                        Anvil::QueueFamilyFlagBits::COMPUTE_BIT | Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::CONCURRENT,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT);

        m_sine_props_data_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    m_sine_props_data_buffer_ptr->set_name("Sine properties data buffer");

    memory_allocator_ptr->add_buffer(m_sine_props_data_buffer_ptr.get(),
                                     Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT);

    /* Each sine needs to be assigned a different color. Compute the data and upload it to another buffer object. */
    std::unique_ptr<uint8_t[]> color_buffer_data_ptr;
    unsigned char*             color_buffer_data_traveller_ptr = nullptr;

    m_sine_color_buffer_size = N_SINE_PAIRS * 2 /* sines per pair */ * (2 * sizeof(char) ) /* R8G8 */;

    color_buffer_data_ptr.reset(
        new unsigned char[static_cast<uintptr_t>(m_sine_color_buffer_size)]
    );

    color_buffer_data_traveller_ptr = color_buffer_data_ptr.get();

    for (uint32_t n_sine = 0;
                  n_sine < 2 * N_SINE_PAIRS;
                ++n_sine)
    {
        *color_buffer_data_traveller_ptr= (unsigned char)((cos(float(n_sine) ) * 0.5f + 0.5f) * 255.0f);
         color_buffer_data_traveller_ptr ++;

        *color_buffer_data_traveller_ptr = (unsigned char)((sin(float(n_sine) ) * 0.5f + 0.5f) * 255.0f);
         color_buffer_data_traveller_ptr ++;
    }

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        m_sine_color_buffer_size,
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT);

        m_sine_color_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    m_sine_color_buffer_ptr->set_name("Sine color data buffer");

    memory_allocator_ptr->add_buffer(m_sine_color_buffer_ptr.get(),
                                     Anvil::MemoryFeatureFlagBits::NONE);

    /* Assign memory blocks to buffers and fill them with data */
    m_sine_offset_data_buffer_ptr->write(0, /* start_offset */
                                         m_sine_offset_data_buffer_ptr->get_create_info_ptr()->get_size(),
                                         sine_offset_data_raw_buffer_ptr.get()
    );
    m_sine_color_buffer_ptr->write(0, /* start_offset */
                                   m_sine_color_buffer_ptr->get_create_info_ptr()->get_size(),
                                   color_buffer_data_ptr.get()
    );
}

void App::init_command_buffers()
{
    auto                         gfx_pipeline_manager_ptr    (m_device_ptr->get_graphics_pipeline_manager() );
    const bool                   is_debug_marker_ext_present (m_device_ptr->get_extension_info()->ext_debug_marker() );
    Anvil::PipelineLayout*       producer_pipeline_layout_ptr(nullptr);
    Anvil::ImageSubresourceRange subresource_range;
    Anvil::Queue*                universal_queue_ptr         (m_device_ptr->get_universal_queue(0) );

    producer_pipeline_layout_ptr = m_device_ptr->get_compute_pipeline_manager()->get_pipeline_layout(m_producer_pipeline_id);

    subresource_range.aspect_mask      = Anvil::ImageAspectFlagBits::COLOR_BIT;
    subresource_range.base_array_layer = 0;
    subresource_range.base_mip_level   = 0;
    subresource_range.layer_count      = 1;
    subresource_range.level_count      = 1;

    /* Set up rendering command buffers. We need one per swap-chain image. */
    for (unsigned int n_current_swapchain_image = 0;
                      n_current_swapchain_image < N_SWAPCHAIN_IMAGES;
                    ++n_current_swapchain_image)
    {
        Anvil::PrimaryCommandBufferUniquePtr draw_cmd_buffer_ptr;

        draw_cmd_buffer_ptr = m_device_ptr->get_command_pool_for_queue_family_index(m_device_ptr->get_universal_queue(0)->get_queue_family_index() )->alloc_primary_level_command_buffer();

        /* Start recording commands */
        draw_cmd_buffer_ptr->start_recording(false, /* one_time_submit          */
                                             true   /* simultaneous_use_allowed */);

        /* Switch the swap-chain image layout to renderable */
        {
            Anvil::ImageBarrier image_barrier(Anvil::AccessFlags(),                              /* source_access_mask      */
                                              Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT, /* destination_access_mask */
                                              Anvil::ImageLayout::UNDEFINED,
                                              Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                              universal_queue_ptr->get_queue_family_index(),
                                              universal_queue_ptr->get_queue_family_index(),
                                              m_swapchain_ptr->get_image(n_current_swapchain_image),
                                              subresource_range);

            draw_cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::TOP_OF_PIPE_BIT,
                                                         Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
                                                         Anvil::DependencyFlagBits::NONE,
                                                         0,              /* in_memory_barrier_count        */
                                                         nullptr,        /* in_memory_barrier_ptrs         */
                                                         0,              /* in_buffer_memory_barrier_count */
                                                         nullptr,        /* in_buffer_memory_barrier_ptrs  */
                                                         1,              /* in_image_memory_barrier_count  */
                                                        &image_barrier);
        }

        /* Invalidate shader read cache. This is needed because t-value [sine_props_data] is written by CPU.
         *
         * We do not need to worry about offset buffer contents getting overwritten by subsequent frames
         * because we do not render frames ahead of time in this example.
         */
        Anvil::BufferBarrier t_value_buffer_barrier = Anvil::BufferBarrier(Anvil::AccessFlagBits::HOST_WRITE_BIT,   /* in_source_access_mask      */
                                                                           Anvil::AccessFlagBits::UNIFORM_READ_BIT, /* in_destination_access_mask */
                                                                           VK_QUEUE_FAMILY_IGNORED,
                                                                           VK_QUEUE_FAMILY_IGNORED,
                                                                           m_sine_props_data_buffer_ptr.get(),
                                                                           n_current_swapchain_image * m_sine_props_data_buffer_size_per_swapchain_image, /* in_start_offset */
                                                                           sizeof(float) );                                                               /* in_size         */

        draw_cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::HOST_BIT,
                                                     Anvil::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                     Anvil::DependencyFlagBits::NONE,
                                                     0,                       /* in_memory_barrier_count        */
                                                     nullptr,                 /* in_memory_barriers_ptr         */
                                                     1,                       /* in_buffer_memory_barrier_count */
                                                     &t_value_buffer_barrier, /* in_buffer_memory_barriers_ptr  */
                                                     0,                       /* in_image_memory_barrier_count  */
                                                     nullptr);                /* in_image_memory_barriers_ptr   */

        /* Let's generate some sine offset data using our compute shader */
        draw_cmd_buffer_ptr->record_bind_pipeline(Anvil::PipelineBindPoint::COMPUTE,
                                                  m_producer_pipeline_id);

        if (is_debug_marker_ext_present)
        {
            static const float region_color[4] =
            {
                0.0f,
                1.0f,
                0.0f,
                1.0f
            };
            draw_cmd_buffer_ptr->record_debug_marker_begin_EXT("Sine offset data computation",
                                                               region_color);
        }

        for (unsigned int n_sine_pair = 0;
                          n_sine_pair < N_SINE_PAIRS;
                        ++n_sine_pair)
        {
            uint32_t              dynamic_offsets[3];
            const uint32_t        n_dynamic_offsets = sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]);
            Anvil::DescriptorSet* producer_dses[]   =
            {
                m_producer_dsg_ptr->get_descriptor_set(0),
                m_producer_dsg_ptr->get_descriptor_set(1)
            };
            static const uint32_t n_producer_dses = sizeof(producer_dses) / sizeof(producer_dses[0]);


            get_buffer_memory_offsets(n_sine_pair,
                                      dynamic_offsets + 1,  /* out_opt_sine1SB_offset_ptr */
                                      nullptr,              /* out_opt_sine2SB_offset_ptr */
                                      dynamic_offsets + 0); /* out_opt_offset_data_ptr    */

            dynamic_offsets[2] = static_cast<uint32_t>(m_sine_props_data_buffer_size_per_swapchain_image * n_current_swapchain_image);

            draw_cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::COMPUTE,
                                                             producer_pipeline_layout_ptr,
                                                             0, /* firstSet */
                                                             n_producer_dses,
                                                             producer_dses,
                                                             n_dynamic_offsets,
                                                             dynamic_offsets);

            draw_cmd_buffer_ptr->record_push_constants(producer_pipeline_layout_ptr,
                                                       Anvil::ShaderStageFlagBits::COMPUTE_BIT,
                                                       0, /* in_offset */
                                                       4, /* in_size   */
                                                      &n_sine_pair);

            draw_cmd_buffer_ptr->record_dispatch(2,  /* x */
                                                 1,  /* y */
                                                 1); /* z */
        }

        if (is_debug_marker_ext_present)
        {
            draw_cmd_buffer_ptr->record_debug_marker_end_EXT();
        }

        /* Before we proceed with drawing, we need to flush the buffer data. This step is needed in order to ensure
         * that the data we have generated in CS is actually visible to the draw call. */
        Anvil::BufferBarrier vertex_buffer_barrier(Anvil::AccessFlagBits::SHADER_WRITE_BIT, /* in_source_access_mask      */
                                                   Anvil::AccessFlagBits::SHADER_READ_BIT,  /* in_destination_access_mask */
                                                   VK_QUEUE_FAMILY_IGNORED,
                                                   VK_QUEUE_FAMILY_IGNORED,
                                                   m_sine_data_buffer_ptr.get(),
                                                   0, /* in_offset */
                                                   m_sine_data_buffer_size);

        draw_cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                     Anvil::PipelineStageFlagBits::VERTEX_SHADER_BIT,
                                                     Anvil::DependencyFlagBits::NONE,
                                                     0,        /* in_memory_barrier_count        */
                                                     nullptr,  /* in_memory_barriers_ptr         */
                                                     1,        /* in_buffer_memory_barrier_count */
                                                    &vertex_buffer_barrier,
                                                     0,        /* in_image_memory_barrier_count */
                                                     nullptr); /* in_image_memory_barriers_ptr  */

        /* Now, use the generated data to draw stuff! */
        VkClearValue clear_values[2];
        VkRect2D     render_area;

        clear_values[0].color.float32[0]   = 0.25f;
        clear_values[0].color.float32[1]   = 0.5f;
        clear_values[0].color.float32[2]   = 0.75f;
        clear_values[0].color.float32[3]   = 1.0f;
        clear_values[1].depthStencil.depth = 1.0f;

        render_area.extent.height = WINDOW_HEIGHT;
        render_area.extent.width  = WINDOW_WIDTH;
        render_area.offset.x      = 0;
        render_area.offset.y      = 0;

        /* NOTE: The render-pass switches the swap-chain image back to the presentable layout
         *       after the draw call finishes.
         */
        draw_cmd_buffer_ptr->record_begin_render_pass(2, /* n_clear_values */
                                                      clear_values,
                                                      m_fbos[n_current_swapchain_image].get(),
                                                      render_area,
                                                      m_consumer_render_pass_ptr.get(),
                                                      Anvil::SubpassContents::INLINE);
        {
            const float           max_line_width            = m_device_ptr->get_physical_device_properties().core_vk1_0_properties_ptr->limits.line_width_range[1];
            Anvil::DescriptorSet* renderer_dses[]           =
            {
                m_consumer_dsg_ptr->get_descriptor_set(0),
                m_consumer_dsg_ptr->get_descriptor_set(1)
            };
            const uint32_t        n_renderer_dses           = sizeof(renderer_dses) / sizeof(renderer_dses[0]);
            auto                  sine_color_buffer_raw_ptr = m_sine_color_buffer_ptr.get();

            Anvil::PipelineLayout*    renderer_pipeline_layout_ptr   = nullptr;
            static const VkDeviceSize sine_color_buffer_start_offset = 0;

            renderer_pipeline_layout_ptr = gfx_pipeline_manager_ptr->get_pipeline_layout(m_consumer_pipeline_id);

            draw_cmd_buffer_ptr->record_bind_pipeline       (Anvil::PipelineBindPoint::GRAPHICS,
                                                             m_consumer_pipeline_id);
            draw_cmd_buffer_ptr->record_bind_vertex_buffers (0, /* startBinding */
                                                             1, /* bindingCount */
                                                            &sine_color_buffer_raw_ptr,
                                                            &sine_color_buffer_start_offset);

            for (unsigned int n_sine_pair = 0;
                              n_sine_pair < N_SINE_PAIRS;
                            ++n_sine_pair)
            {
                uint32_t dynamic_offsets[2];
                float    new_line_width     = float(n_sine_pair + 1) * 3;

                const uint32_t n_dynamic_offsets = sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]);

                /* Clamp the line width */
                if (new_line_width > max_line_width)
                {
                    new_line_width = max_line_width;
                }

                if (is_debug_marker_ext_present)
                {
                    std::stringstream marker_name_sstream;

                    marker_name_sstream << "Draw sine pair "
                                        << n_sine_pair;

                    draw_cmd_buffer_ptr->record_debug_marker_insert_EXT(marker_name_sstream.str().c_str(),
                                                                        nullptr);
                }

                get_buffer_memory_offsets(n_sine_pair,
                                          dynamic_offsets + 0,  /* out_opt_sine1SB_offset_ptr */
                                          dynamic_offsets + 1); /* out_opt_sine2SB_offset_ptr */

                draw_cmd_buffer_ptr->record_set_line_width(new_line_width);

                draw_cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                                 renderer_pipeline_layout_ptr,
                                                                 0, /* firstSet */
                                                                 n_renderer_dses,
                                                                 renderer_dses,
                                                                 n_dynamic_offsets,
                                                                 dynamic_offsets);

                draw_cmd_buffer_ptr->record_draw(N_VERTICES_PER_SINE,
                                                 2,                /* instanceCount */
                                                 0,                /* firstVertex   */
                                                 n_sine_pair * 2); /* firstInstance */
            }
        }
        draw_cmd_buffer_ptr->record_end_render_pass();

        /* Close the recording process */
        draw_cmd_buffer_ptr->stop_recording();

        m_command_buffers[n_current_swapchain_image] = std::move(draw_cmd_buffer_ptr);
    }
}

void App::init_compute_pipelines()
{
    auto compute_manager_ptr(m_device_ptr->get_compute_pipeline_manager() );
    bool result;

    /* Create & configure the compute pipeline */
    auto producer_pipeline_info_ptr = Anvil::ComputePipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                               *m_producer_cs_ptr);

    producer_pipeline_info_ptr->attach_push_constant_range    (0,  /* offset */
                                                               4,  /* size   */
                                                               Anvil::ShaderStageFlagBits::COMPUTE_BIT);
    producer_pipeline_info_ptr->set_descriptor_set_create_info(m_producer_dsg_ptr->get_descriptor_set_create_info() );

    result = compute_manager_ptr->add_pipeline(std::move(producer_pipeline_info_ptr),
                                              &m_producer_pipeline_id);
    anvil_assert(result);

    result = compute_manager_ptr->bake();
    anvil_assert(result);
}

void App::init_dsgs()
{
    auto consumer_dsg_create_info_ptrs = std::vector<Anvil::DescriptorSetCreateInfoUniquePtr>(2);
    auto producer_dsg_create_info_ptrs = std::vector<Anvil::DescriptorSetCreateInfoUniquePtr>(2);

    consumer_dsg_create_info_ptrs[0] = Anvil::DescriptorSetCreateInfo::create();
    consumer_dsg_create_info_ptrs[1] = Anvil::DescriptorSetCreateInfo::create();
    producer_dsg_create_info_ptrs[0] = Anvil::DescriptorSetCreateInfo::create();
    producer_dsg_create_info_ptrs[1] = Anvil::DescriptorSetCreateInfo::create();

    consumer_dsg_create_info_ptrs[0]->add_binding(0, /* binding    */
                                                  Anvil::DescriptorType::STORAGE_BUFFER_DYNAMIC,
                                                  1, /* n_elements */
                                                  Anvil::ShaderStageFlagBits::VERTEX_BIT);
    consumer_dsg_create_info_ptrs[1]->add_binding(0, /* binding    */
                                                  Anvil::DescriptorType::STORAGE_BUFFER_DYNAMIC,
                                                  1, /* n_elements */
                                                  Anvil::ShaderStageFlagBits::VERTEX_BIT);

    producer_dsg_create_info_ptrs[0]->add_binding(0, /* binding    */
                                                  Anvil::DescriptorType::STORAGE_BUFFER_DYNAMIC,
                                                  1, /* n_elements */
                                                  Anvil::ShaderStageFlagBits::COMPUTE_BIT);
    producer_dsg_create_info_ptrs[0]->add_binding(1, /* binding    */
                                                  Anvil::DescriptorType::STORAGE_BUFFER_DYNAMIC,
                                                  1, /* n_elements */
                                                  Anvil::ShaderStageFlagBits::COMPUTE_BIT);
    producer_dsg_create_info_ptrs[1]->add_binding(0, /* binding    */
                                                  Anvil::DescriptorType::UNIFORM_BUFFER_DYNAMIC,
                                                  1, /* n_elements */
                                                  Anvil::ShaderStageFlagBits::COMPUTE_BIT);

    /* Create the descriptor set layouts for the generator program. */
    m_producer_dsg_ptr = Anvil::DescriptorSetGroup::create(m_device_ptr.get(),
                                                           producer_dsg_create_info_ptrs,
                                                           false); /* releaseable_sets */


    m_producer_dsg_ptr->set_binding_item(0, /* n_set         */
                                         0, /* binding_index */
                                         Anvil::DescriptorSet::DynamicStorageBufferBindingElement(m_sine_offset_data_buffer_ptr.get(),
                                                                                                  0, /* in_start_offset */
                                                                                                  sizeof(float) * 2) );
    m_producer_dsg_ptr->set_binding_item(0, /* n_set         */
                                         1, /* binding_index */
                                         Anvil::DescriptorSet::DynamicStorageBufferBindingElement(m_sine_data_buffer_ptr.get(),
                                                                                                  0, /* in_start_offset */
                                                                                                  sizeof(float) * 4 * N_VERTICES_PER_SINE * 2) );
    m_producer_dsg_ptr->set_binding_item(1, /* n_set         */
                                         0, /* binding_index */
                                         Anvil::DescriptorSet::DynamicUniformBufferBindingElement(m_sine_props_data_buffer_ptr.get(),
                                                                                                  0, /* in_start_offset */
                                                                                                  m_sine_props_data_buffer_size_per_swapchain_image) );

    /* Set up the descriptor set layout for the renderer program.  */
    m_consumer_dsg_ptr = Anvil::DescriptorSetGroup::create(m_device_ptr.get(),
                                                           consumer_dsg_create_info_ptrs,
                                                           false); /* releaseable_sets */


    m_consumer_dsg_ptr->set_binding_item(0, /* n_set         */
                                         0, /* binding_index */
                                         Anvil::DescriptorSet::DynamicStorageBufferBindingElement(m_sine_data_buffer_ptr.get(),
                                                                                                  0, /* in_start_offset */
                                                                                                  sizeof(float) * 4 * N_VERTICES_PER_SINE) );
    m_consumer_dsg_ptr->set_binding_item(1, /* n_set         */
                                         0, /* binding_index */
                                         Anvil::DescriptorSet::DynamicStorageBufferBindingElement(m_sine_data_buffer_ptr.get(),
                                                                                                  0, /* in_start_offset */
                                                                                                  sizeof(float) * 4 * N_VERTICES_PER_SINE) );
}

void App::init_events()
{
    /* Stub */
}

void App::init_framebuffers()
{
    bool result;

    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < N_SWAPCHAIN_IMAGES;
                ++n_swapchain_image)
    {
        Anvil::FramebufferUniquePtr result_fb_ptr;

        {
            auto create_info_ptr = Anvil::FramebufferCreateInfo::create(m_device_ptr.get(),
                                                                        WINDOW_WIDTH,
                                                                        WINDOW_HEIGHT,
                                                                        1); /* n_layers */

            result = create_info_ptr->add_attachment(m_swapchain_ptr->get_image_view(n_swapchain_image),
                                                     nullptr); /* out_opt_attachment_id_ptr */
            anvil_assert(result);

            result = create_info_ptr->add_attachment(m_depth_image_views[n_swapchain_image].get(),
                                                     nullptr); /* out_opt_attachment_id_ptr */

            result_fb_ptr = Anvil::Framebuffer::create(std::move(create_info_ptr) );
        }

        result_fb_ptr->set_name_formatted("Framebuffer for swapchain image [%d]",
                                          n_swapchain_image);

        m_fbos[n_swapchain_image] = std::move(result_fb_ptr);
    }
}

void App::init_gfx_pipelines()
{
    auto gfx_manager_ptr            = m_device_ptr->get_graphics_pipeline_manager();
    auto renderpass_create_info_ptr = Anvil::RenderPassCreateInfoUniquePtr(new Anvil::RenderPassCreateInfo(m_device_ptr.get() ) );

    /* Create a renderpass instance */
    #ifdef ENABLE_OFFSCREEN_RENDERING
        const auto final_layout = Anvil::ImageLayout::GENERAL;
    #else
        const auto final_layout = Anvil::ImageLayout::PRESENT_SRC_KHR;
    #endif

    Anvil::RenderPassAttachmentID render_pass_color_attachment_id = -1;
    Anvil::RenderPassAttachmentID render_pass_depth_attachment_id = -1;
    Anvil::SubPassID              render_pass_subpass_id          = -1;

    renderpass_create_info_ptr->add_color_attachment(m_swapchain_ptr->get_create_info_ptr()->get_format(),
                                                     Anvil::SampleCountFlagBits::_1_BIT,
                                                     Anvil::AttachmentLoadOp::CLEAR,
                                                     Anvil::AttachmentStoreOp::STORE,
                                                     Anvil::ImageLayout::UNDEFINED,
                                                     final_layout,
                                                     false, /* may_alias */
                                                    &render_pass_color_attachment_id);

    renderpass_create_info_ptr->add_depth_stencil_attachment(m_depth_images[0]->get_create_info_ptr()->get_format(),
                                                             m_depth_images[0]->get_create_info_ptr()->get_sample_count(),
                                                             Anvil::AttachmentLoadOp::CLEAR,                       /* depth_load_op    */
                                                             Anvil::AttachmentStoreOp::DONT_CARE,                  /* depth_store_op   */
                                                             Anvil::AttachmentLoadOp::DONT_CARE,                   /* stencil_load_op  */
                                                             Anvil::AttachmentStoreOp::DONT_CARE,                  /* stencil_store_op */
                                                             Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, /* initial_layout   */
                                                             Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, /* final_layout     */
                                                             false,                                                /* may_alias        */
                                                            &render_pass_depth_attachment_id);

    renderpass_create_info_ptr->add_subpass(&render_pass_subpass_id);

    renderpass_create_info_ptr->add_subpass_color_attachment        (render_pass_subpass_id,
                                                                     Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                     render_pass_color_attachment_id,
                                                                     0,        /* location                      */
                                                                     nullptr); /* opt_attachment_resolve_id_ptr */
    renderpass_create_info_ptr->add_subpass_depth_stencil_attachment(render_pass_subpass_id,
                                                                     Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                     render_pass_depth_attachment_id);

    m_consumer_render_pass_ptr = Anvil::RenderPass::create(std::move(renderpass_create_info_ptr),
                                                           m_swapchain_ptr.get() );

    m_consumer_render_pass_ptr->set_name("Consumer renderpass");


    /* Set up the graphics pipeline for the main subpass */
    auto consumer_pipeline_info_ptr = Anvil::GraphicsPipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                                m_consumer_render_pass_ptr.get(),
                                                                                render_pass_subpass_id,
                                                                               *m_consumer_fs_ptr,
                                                                                Anvil::ShaderModuleStageEntryPoint(), /* geometry_shader        */
                                                                                Anvil::ShaderModuleStageEntryPoint(), /* tess_control_shader    */
                                                                                Anvil::ShaderModuleStageEntryPoint(), /* tess_evaluation_shader */
                                                                               *m_consumer_vs_ptr);

    consumer_pipeline_info_ptr->add_vertex_attribute          (0, /* location */
                                                               Anvil::Format::R8G8_UNORM,
                                                               0,                /* offset_in_bytes */
                                                               sizeof(char) * 2, /* stride_in_bytes */
                                                               Anvil::VertexInputRate::INSTANCE);
    consumer_pipeline_info_ptr->set_descriptor_set_create_info(m_consumer_dsg_ptr->get_descriptor_set_create_info() );
    consumer_pipeline_info_ptr->set_primitive_topology        (Anvil::PrimitiveTopology::LINE_STRIP);
    consumer_pipeline_info_ptr->set_rasterization_properties  (Anvil::PolygonMode::FILL,
                                                               Anvil::CullModeFlagBits::NONE,
                                                               Anvil::FrontFace::COUNTER_CLOCKWISE,
                                                               1.0f /* line_width */);
    consumer_pipeline_info_ptr->toggle_depth_test             (true, /* should_enable */
                                                               Anvil::CompareOp::LESS_OR_EQUAL);
    consumer_pipeline_info_ptr->toggle_depth_writes           (true); /* should_enable */
    consumer_pipeline_info_ptr->toggle_dynamic_state          (true, /* should_enable */
                                                               Anvil::DynamicState::LINE_WIDTH);

    gfx_manager_ptr->add_pipeline(std::move(consumer_pipeline_info_ptr),
                                 &m_consumer_pipeline_id);
}

void App::init_images()
{
    for (uint32_t n_depth_image = 0;
                  n_depth_image < N_SWAPCHAIN_IMAGES;
                ++n_depth_image)
    {
        {
            auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(m_device_ptr.get(),
                                                                        Anvil::ImageType::_2D,
                                                                        Anvil::Format::D16_UNORM,
                                                                        Anvil::ImageTiling::OPTIMAL,
                                                                        Anvil::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
                                                                        WINDOW_WIDTH,
                                                                        WINDOW_HEIGHT,
                                                                        1,                    /* in_base_mipmap_depth */
                                                                        1,                    /* in_n_layers          */
                                                                        Anvil::SampleCountFlagBits::_1_BIT,
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        false,                /* in_use_full_mipmap_chain */
                                                                        Anvil::MemoryFeatureFlagBits::NONE,
                                                                        Anvil::ImageCreateFlagBits::NONE,
                                                                        Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                        nullptr);             /* in_mipmaps_ptr */

            m_depth_images[n_depth_image] = Anvil::Image::create(std::move(create_info_ptr) );
        }

        {
            auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(m_device_ptr.get(),
                                                                         m_depth_images[n_depth_image].get(),
                                                                         0,                                              /* n_base_layer        */
                                                                         0,                                              /* n_base_mipmap_level */
                                                                         1,                                              /* n_mipmaps           */
                                                                         Anvil::ImageAspectFlagBits::DEPTH_BIT,
                                                                         m_depth_images[n_depth_image]->get_create_info_ptr()->get_format(),
                                                                         Anvil::ComponentSwizzle::IDENTITY,
                                                                         Anvil::ComponentSwizzle::IDENTITY,
                                                                         Anvil::ComponentSwizzle::IDENTITY,
                                                                         Anvil::ComponentSwizzle::IDENTITY);

            m_depth_image_views[n_depth_image] = Anvil::ImageView::create(std::move(create_info_ptr) );
        }

        m_depth_images     [n_depth_image]->set_name_formatted("Depth image [%d]",
                                                               n_depth_image);
        m_depth_image_views[n_depth_image]->set_name_formatted("Depth image view [%d]",
                                                               n_depth_image);
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

            new_signal_semaphore_ptr->set_name_formatted("Signal semaphore [%d]",
                                                         n_semaphore);
        }

        {
            auto create_info_ptr = Anvil::SemaphoreCreateInfo::create(m_device_ptr.get() );

            new_wait_semaphore_ptr = Anvil::Semaphore::create(std::move(create_info_ptr) );

            new_wait_semaphore_ptr->set_name_formatted("Wait semaphore [%d]",
                                                       n_semaphore);
        }

        m_frame_signal_semaphores.push_back(std::move(new_signal_semaphore_ptr) );
        m_frame_wait_semaphores.push_back  (std::move(new_wait_semaphore_ptr) );
    }
}

void App::init_shaders()
{
    Anvil::ShaderModuleUniquePtr               cs_module_ptr;
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr cs_ptr;
    Anvil::ShaderModuleUniquePtr               fs_module_ptr;
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr fs_ptr;
    Anvil::ShaderModuleUniquePtr               vs_module_ptr;
    Anvil::GLSLShaderToSPIRVGeneratorUniquePtr vs_ptr;

    cs_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                       Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                       g_glsl_producer_comp,
                                                       Anvil::ShaderStage::COMPUTE);
    fs_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                       Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                       g_glsl_consumer_frag,
                                                       Anvil::ShaderStage::FRAGMENT);
    vs_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr.get(),
                                                       Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                       g_glsl_consumer_vert,
                                                       Anvil::ShaderStage::VERTEX);

    /* Set up GLSLShader instances */
    cs_ptr->add_definition_value_pair("N_SINE_PAIRS",
                                      N_SINE_PAIRS);
    cs_ptr->add_definition_value_pair("N_VERTICES_PER_SINE",
                                      N_VERTICES_PER_SINE);

    vs_ptr->add_definition_value_pair("N_VERTICES_PER_SINE",
                                      N_VERTICES_PER_SINE);

    /* Initialize the shader modules */
    cs_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get(),
                                                                     cs_ptr.get      () );
    fs_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get(),
                                                                     fs_ptr.get      () );
    vs_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr.get(),
                                                                     vs_ptr.get      () );

    cs_module_ptr->set_name("Compute shader module");
    fs_module_ptr->set_name("Fragment shader module");
    vs_module_ptr->set_name("Vertex shader module");

    /* Prepare entrypoint descriptors. */
    m_producer_cs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint("main",
                                               std::move(cs_module_ptr),
                                               Anvil::ShaderStage::COMPUTE)
    );
    m_consumer_fs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint("main",
                                               std::move(fs_module_ptr),
                                               Anvil::ShaderStage::FRAGMENT)
    );
    m_consumer_vs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint("main",
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


    m_swapchain_ptr = reinterpret_cast<Anvil::SGPUDevice*>(m_device_ptr.get())->create_swapchain(m_rendering_surface_ptr.get(),
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
        auto create_info_ptr = Anvil::InstanceCreateInfo::create(APP_NAME,  /* app_name */
                                                                 APP_NAME,  /* engine_name */
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
                                                                    true,                                   /* in_enable_shader_module_cache */
                                                                    Anvil::DeviceExtensionConfiguration(),
                                                                    std::vector<std::string>(),             /* in_layers */
                                                                    Anvil::CommandPoolCreateFlagBits::NONE,
                                                                    false);                                 /* in_mt_safe */

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

