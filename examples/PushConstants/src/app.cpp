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
#include "misc/io.h"
#include "misc/memory_allocator.h"
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
    vkDeviceWaitIdle(m_device_ptr.lock()->get_device_vk() );

    m_frame_signal_semaphores.clear();
    m_frame_wait_semaphores.clear();

    m_present_queue_ptr.reset();
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

    /* Submit work chunk and present */
    app_ptr->update_data_ub_contents(n_swapchain_image);

    present_queue_ptr->submit_command_buffer_with_signal_wait_semaphores(app_ptr->m_command_buffers[n_swapchain_image],
                                                                         1, /* n_semaphores_to_signal */
                                                                        &curr_frame_signal_semaphore_ptr,
                                                                         1, /* n_semaphores_to_wait_on */
                                                                        &curr_frame_wait_semaphore_ptr,
                                                                        &wait_stage_mask,
                                                                         false /* should_block */);

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

void App::get_luminance_data(std::shared_ptr<float>* out_result_ptr,
                             uint32_t*               out_result_size_ptr) const
{
    std::shared_ptr<float> luminance_data_ptr;
    float*                 luminance_data_raw_ptr;
    uint32_t               luminance_data_size;

    static_assert(N_TRIANGLES == 16,
                  "Shader and the app logic assumes N_TRIANGLES will always be 16");

    luminance_data_size = sizeof(float) * N_TRIANGLES;

    luminance_data_ptr.reset(new float[luminance_data_size / sizeof(float)], std::default_delete<float[]>());

    luminance_data_raw_ptr = luminance_data_ptr.get();

    for (uint32_t n_tri = 0;
                  n_tri < N_TRIANGLES;
                ++n_tri)
    {
        luminance_data_raw_ptr[n_tri] = float(n_tri) / float(N_TRIANGLES - 1);
    }

    *out_result_ptr      = luminance_data_ptr;
    *out_result_size_ptr = luminance_data_size;
}

const unsigned char* App::get_mesh_data() const
{
    return reinterpret_cast<const unsigned char*>(g_mesh_data);
}

VkFormat App::get_mesh_data_color_format() const
{
    return VK_FORMAT_R32G32B32_SFLOAT;
}

uint32_t App::get_mesh_data_color_start_offset() const
{
    return g_mesh_data_color_start_offset;
}

uint32_t App::get_mesh_data_color_stride() const
{
    return g_mesh_data_color_stride;
}

VkFormat App::get_mesh_data_position_format() const
{
    return VK_FORMAT_R32G32B32A32_SFLOAT;
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
    const auto           ub_data_alignment_requirement    = m_device_ptr.lock()->get_physical_device_properties().limits.minUniformBufferOffsetAlignment;
    const auto           ub_data_size_total               = N_SWAPCHAIN_IMAGES * (Anvil::Utils::round_up(ub_data_size_per_swapchain_image,
                                                                                                         ub_data_alignment_requirement) );

    m_ub_data_size_per_swapchain_image = ub_data_size_total / N_SWAPCHAIN_IMAGES;

    /* Use a memory allocator to re-use memory blocks wherever possible */
    std::shared_ptr<Anvil::MemoryAllocator> allocator_ptr = Anvil::MemoryAllocator::create_oneshot(m_device_ptr);

    /* Set up a buffer to hold uniform data */
    m_data_buffer_ptr = Anvil::Buffer::create_nonsparse(m_device_ptr,
                                                        ub_data_size_total,
                                                        Anvil::QUEUE_FAMILY_COMPUTE_BIT | Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
                                                        VK_SHARING_MODE_EXCLUSIVE,
                                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    m_data_buffer_ptr->set_name("Data buffer");

    allocator_ptr->add_buffer(m_data_buffer_ptr,
                              0); /* in_required_memory_features */

    /* Set up a buffer to hold mesh data */
    m_mesh_data_buffer_ptr = Anvil::Buffer::create_nonsparse(
        m_device_ptr,
        get_mesh_data_size(),
        Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    m_mesh_data_buffer_ptr->set_name("Mesh vertexdata buffer");

    allocator_ptr->add_buffer(m_mesh_data_buffer_ptr,
                              0); /* in_required_memory_features */

    /* Allocate memory blocks and copy data where applicable */
    m_mesh_data_buffer_ptr->write(0, /* start_offset */
                                  mesh_data_size,
                                  mesh_data);
}

void App::init_command_buffers()
{
    std::shared_ptr<Anvil::SGPUDevice>              device_locked_ptr       (m_device_ptr);
    std::shared_ptr<Anvil::GraphicsPipelineManager> gfx_pipeline_manager_ptr(device_locked_ptr->get_graphics_pipeline_manager() );
    VkImageSubresourceRange                         image_subresource_range;
    std::shared_ptr<float>                          luminance_data_ptr;
    uint32_t                                        luminance_data_size;
    std::shared_ptr<Anvil::Queue>                   universal_queue_ptr     (device_locked_ptr->get_universal_queue(0) );

    image_subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.baseMipLevel   = 0;
    image_subresource_range.layerCount     = 1;
    image_subresource_range.levelCount     = 1;

    get_luminance_data(&luminance_data_ptr,
                       &luminance_data_size);

    for (uint32_t n_command_buffer = 0;
                  n_command_buffer < N_SWAPCHAIN_IMAGES;
                ++n_command_buffer)
    {
        std::shared_ptr<Anvil::PrimaryCommandBuffer> cmd_buffer_ptr;

        cmd_buffer_ptr = device_locked_ptr->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)->alloc_primary_level_command_buffer();

        /* Start recording commands */
        cmd_buffer_ptr->start_recording(false, /* one_time_submit          */
                                        true); /* simultaneous_use_allowed */

        /* Switch the swap-chain image to the color_attachment_optimal image layout */
        {
            Anvil::ImageBarrier image_barrier(0,                                              /* source_access_mask       */
                                              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           /* destination_access_mask  */
                                              false,
                                              VK_IMAGE_LAYOUT_UNDEFINED,                      /* old_image_layout */
                                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,       /* new_image_layout */
                                              universal_queue_ptr->get_queue_family_index(),
                                              universal_queue_ptr->get_queue_family_index(),
                                              m_swapchain_ptr->get_image(n_command_buffer),
                                              image_subresource_range);

            cmd_buffer_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,            /* src_stage_mask                 */
                                                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,/* dst_stage_mask                 */
                                                    VK_FALSE,                                     /* in_by_region                   */
                                                    0,                                            /* in_memory_barrier_count        */
                                                    nullptr,                                      /* in_memory_barrier_ptrs         */
                                                    0,                                            /* in_buffer_memory_barrier_count */
                                                    nullptr,                                      /* in_buffer_memory_barrier_ptrs  */
                                                    1,                                            /* in_image_memory_barrier_count  */
                                                   &image_barrier);
        }

        /* Make sure CPU-written data is flushed before we start rendering */
        Anvil::BufferBarrier buffer_barrier(VK_ACCESS_HOST_WRITE_BIT,                               /* in_source_access_mask      */
                                             VK_ACCESS_UNIFORM_READ_BIT,                            /* in_destination_access_mask */
                                             universal_queue_ptr->get_queue_family_index(),         /* in_src_queue_family_index  */
                                             universal_queue_ptr->get_queue_family_index(),         /* in_dst_queue_family_index  */
                                             m_data_buffer_ptr,
                                             m_ub_data_size_per_swapchain_image * n_command_buffer, /* in_offset                  */
                                             m_ub_data_size_per_swapchain_image);

        cmd_buffer_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_HOST_BIT,
                                                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                                VK_FALSE,        /* in_by_region                   */
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
                                                 m_fbos[n_command_buffer],
                                                 render_area,
                                                 m_renderpass_ptr,
                                                 VK_SUBPASS_CONTENTS_INLINE);
        {
            const uint32_t                         data_ub_offset          = static_cast<uint32_t>(m_ub_data_size_per_swapchain_image * n_command_buffer);
            std::shared_ptr<Anvil::DescriptorSet>  ds_ptr                  = m_dsg_ptr->get_descriptor_set                         (0 /* n_set */);
            const VkDeviceSize                     mesh_data_buffer_offset = 0;
            std::shared_ptr<Anvil::PipelineLayout> pipeline_layout_ptr     = gfx_pipeline_manager_ptr->get_graphics_pipeline_layout(m_pipeline_id);

            cmd_buffer_ptr->record_bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                 m_pipeline_id);

            cmd_buffer_ptr->record_push_constants(pipeline_layout_ptr,
                                                  VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                                                  0, /* in_offset */
                                                  luminance_data_size,
                                                  luminance_data_ptr.get() );

            cmd_buffer_ptr->record_bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                        pipeline_layout_ptr,
                                                        0, /* firstSet */
                                                        1, /* setCount */
                                                       &ds_ptr,
                                                        1,               /* dynamicOffsetCount */
                                                       &data_ub_offset); /* pDynamicOffsets    */

            cmd_buffer_ptr->record_bind_vertex_buffers(0, /* startBinding */ 
                                                       1, /* bindingCount */
                                                      &m_mesh_data_buffer_ptr,
                                                      &mesh_data_buffer_offset);

            cmd_buffer_ptr->record_draw(3,            /* in_vertex_count   */
                                        N_TRIANGLES,  /* in_instance_count */
                                        0,            /* in_first_vertex   */
                                        0);           /* in_first_instance */
        }
        cmd_buffer_ptr->record_end_render_pass();

        /* Close the recording process */
        cmd_buffer_ptr->stop_recording();

        m_command_buffers[n_command_buffer] = cmd_buffer_ptr;
    }
}

void App::init_dsgs()
{
    m_dsg_ptr = Anvil::DescriptorSetGroup::create(m_device_ptr,
                                                  false, /* releaseable_sets */
                                                  1      /* n_sets           */);

    m_dsg_ptr->add_binding     (0, /* n_set     */
                                0, /* n_binding */
                                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                1, /* n_elements */
                                VK_SHADER_STAGE_VERTEX_BIT);
    m_dsg_ptr->set_binding_item(0, /* n_set     */
                                0, /* n_binding */
                                Anvil::DescriptorSet::DynamicUniformBufferBindingElement(m_data_buffer_ptr,
                                                                                         0, /* in_start_offset */
                                                                                         m_ub_data_size_per_swapchain_image) );
}

void App::init_events()
{
}

void App::init_framebuffers()
{
    bool result;

    /* We need to instantiate 1 framebuffer object per each used swap-chain image */
    for (uint32_t n_fbo = 0;
                  n_fbo < N_SWAPCHAIN_IMAGES;
                ++n_fbo)
    {
        std::shared_ptr<Anvil::ImageView> attachment_image_view_ptr;

        attachment_image_view_ptr = m_swapchain_ptr->get_image_view(n_fbo);

        /* Create the internal framebuffer object */
        m_fbos[n_fbo] = Anvil::Framebuffer::create(
            m_device_ptr,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            1 /* n_layers */);

        m_fbos[n_fbo]->set_name_formatted("Framebuffer for swapchain image [%d]",
                                          n_fbo);

        result = m_fbos[n_fbo]->add_attachment(attachment_image_view_ptr,
                                               nullptr /* out_opt_attachment_id_ptr */);

        anvil_assert(result);
    }
}

void App::init_gfx_pipelines()
{
    std::shared_ptr<Anvil::SGPUDevice>              device_locked_ptr       (m_device_ptr);
    std::shared_ptr<Anvil::GraphicsPipelineManager> gfx_pipeline_manager_ptr(device_locked_ptr->get_graphics_pipeline_manager() );
    bool                                            result;

    /* Create a renderpass for the pipeline */
    Anvil::RenderPassAttachmentID render_pass_color_attachment_id;
    Anvil::SubPassID              render_pass_subpass_id;

    m_renderpass_ptr = Anvil::RenderPass::create(m_device_ptr,
                                                 m_swapchain_ptr);

    m_renderpass_ptr->set_name("Main renderpass");

    result = m_renderpass_ptr->add_color_attachment(m_swapchain_ptr->get_image_format(),
                                                     VK_SAMPLE_COUNT_1_BIT,
                                                     VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                     VK_ATTACHMENT_STORE_OP_STORE,
                                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
#ifdef ENABLE_OFFSCREEN_RENDERING
                                                     VK_IMAGE_LAYOUT_GENERAL,
#else
                                                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
#endif
                                                     false, /* may_alias */
                                                    &render_pass_color_attachment_id);
    anvil_assert(result);


    result = m_renderpass_ptr->add_subpass(*m_fs_ptr,
                                            Anvil::ShaderModuleStageEntryPoint(), /* geometry_shader        */
                                            Anvil::ShaderModuleStageEntryPoint(), /* tess_control_shader    */
                                            Anvil::ShaderModuleStageEntryPoint(), /* tess_evaluation_shader */
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


    result  = gfx_pipeline_manager_ptr->set_pipeline_dsg                      (m_pipeline_id,
                                                                               m_dsg_ptr);
    result &= gfx_pipeline_manager_ptr->attach_push_constant_range_to_pipeline(m_pipeline_id,
                                                                               0, /* offset */
                                                                               sizeof(float) * 4 /* vec4 */ * 4 /* vec4 values */,
                                                                               VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT);

    anvil_assert(result);


    gfx_pipeline_manager_ptr->set_rasterization_properties         (m_pipeline_id,
                                                                    VK_POLYGON_MODE_FILL,
                                                                    VK_CULL_MODE_NONE,
                                                                    VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                                                    1.0f); /* line_width */
    gfx_pipeline_manager_ptr->set_color_blend_attachment_properties(m_pipeline_id,
                                                                    0, /* attachment_id */
                                                                    true, /* blending_enabled */
                                                                    VK_BLEND_OP_ADD,
                                                                    VK_BLEND_OP_ADD,
                                                                    VK_BLEND_FACTOR_SRC_ALPHA,
                                                                    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                                                    VK_BLEND_FACTOR_SRC_ALPHA,
                                                                    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                                                    VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT);

    result = gfx_pipeline_manager_ptr->add_vertex_attribute(m_pipeline_id,
                                                            0, /* location */
                                                            get_mesh_data_position_format(),
                                                            get_mesh_data_position_start_offset(),
                                                            get_mesh_data_position_stride(),
                                                            VK_VERTEX_INPUT_RATE_VERTEX);
    anvil_assert(result);

    result = gfx_pipeline_manager_ptr->add_vertex_attribute(m_pipeline_id,
                                                            1, /* location */
                                                            get_mesh_data_color_format(),
                                                            get_mesh_data_color_start_offset(),
                                                            get_mesh_data_color_stride(),
                                                            VK_VERTEX_INPUT_RATE_VERTEX);
    anvil_assert(result);
}

void App::init_images()
{
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
    std::shared_ptr<Anvil::GLSLShaderToSPIRVGenerator> vertex_shader_ptr;
    std::shared_ptr<Anvil::ShaderModule>               vertex_shader_module_ptr;

    fragment_shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    g_glsl_frag,
                                                                    Anvil::SHADER_STAGE_FRAGMENT);
    vertex_shader_ptr   = Anvil::GLSLShaderToSPIRVGenerator::create(m_device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    g_glsl_vert,
                                                                    Anvil::SHADER_STAGE_VERTEX);

    fragment_shader_ptr->add_definition_value_pair("N_TRIANGLES",
                                                   N_TRIANGLES);
    vertex_shader_ptr->add_definition_value_pair  ("N_TRIANGLES",
                                                   N_TRIANGLES);


    fragment_shader_module_ptr = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr,
                                                                                  fragment_shader_ptr);
    vertex_shader_module_ptr   = Anvil::ShaderModule::create_from_spirv_generator(m_device_ptr,
                                                                                  vertex_shader_ptr);

    fragment_shader_module_ptr->set_name("Fragment shader module");
    vertex_shader_module_ptr->set_name  ("Vertex shader module");

    m_fs_ptr.reset(
        new Anvil::ShaderModuleStageEntryPoint(
            "main",
            fragment_shader_module_ptr,
            Anvil::SHADER_STAGE_FRAGMENT)
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
                             m_device_ptr.lock()->get_universal_queue(0) );

    ++n_frames_rendered;
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

