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
#include <memory>

#define N_SWAPCHAIN_IMAGES (3)


class App
{
public:
    /* Public functions */
     App();
    ~App();

    void init();
    void run ();

private:
    /* Private functions */
    App           (const App&);
    App& operator=(const App&);

    void deinit              ();
    void init_buffers        ();
    void init_command_buffers();
    void init_dsgs           ();
    void init_events         ();
    void init_framebuffers   ();
    void init_images         ();
    void init_query_pool     ();
    void init_renderpasses   ();
    void init_semaphores     ();
    void init_shaders        ();
    void init_swapchain      ();
    void init_window         ();
    void init_vulkan         ();

    void draw_frame            ();
    void on_validation_callback(Anvil::DebugMessageSeverityFlags in_severity,
                                const char*                      in_message_ptr);

    /* Private variables */
    Anvil::BaseDeviceUniquePtr       m_device_ptr;
    Anvil::InstanceUniquePtr         m_instance_ptr;
    const Anvil::PhysicalDevice*     m_physical_device_ptr;
    Anvil::Queue*                    m_present_queue_ptr;
    Anvil::RenderingSurfaceUniquePtr m_rendering_surface_ptr;
    Anvil::SwapchainUniquePtr        m_swapchain_ptr;
    Anvil::Time                      m_time;
    Anvil::WindowUniquePtr           m_window_ptr;

    Anvil::ImageUniquePtr     m_depth_image_ptr;
    Anvil::ImageViewUniquePtr m_depth_image_view_ptr;

    std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> m_quad_fs_ptr;
    std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> m_quad_vs_ptr;
    std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> m_tri_fs_ptr;
    std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> m_tri_vs_ptr;

    Anvil::DescriptorSetGroupUniquePtr m_1stpass_dsg_ptr;
    Anvil::DescriptorSetGroupUniquePtr m_2ndpass_quad_dsg_ptr;
    Anvil::DescriptorSetGroupUniquePtr m_2ndpass_tri_dsg_ptr;

    Anvil::QueryPoolUniquePtr m_query_pool_ptr;

    uint32_t               m_n_bytes_per_query;
    Anvil::BufferUniquePtr m_query_bo_ptr;
    Anvil::EventUniquePtr  m_query_data_copied_event;
    Anvil::BufferUniquePtr m_time_bo_ptr;
    uint32_t               m_time_n_bytes_per_swapchain_image;

    Anvil::PrimaryCommandBufferUniquePtr m_render_tri1_and_generate_ot_data_cmd_buffers[N_SWAPCHAIN_IMAGES];
    Anvil::PrimaryCommandBufferUniquePtr m_render_tri2_and_quad_cmd_buffers            [N_SWAPCHAIN_IMAGES];

    Anvil::PipelineID m_1stpass_depth_test_always_pipeline_id;
    Anvil::PipelineID m_1stpass_depth_test_equal_pipeline_id;
    Anvil::PipelineID m_2ndpass_depth_test_off_quad_pipeline_id;
    Anvil::PipelineID m_2ndpass_depth_test_off_tri_pipeline_id;

    Anvil::FramebufferUniquePtr m_fbos[N_SWAPCHAIN_IMAGES];

    Anvil::RenderPassUniquePtr m_renderpass_quad_ptr;
    Anvil::RenderPassUniquePtr m_renderpass_tris_ptr;

    Anvil::SubPassID                   m_renderpass_1stpass_depth_test_always_subpass_id;
    Anvil::SubPassID                   m_renderpass_1stpass_depth_test_equal_ot_subpass_id;
    Anvil::SubPassID                   m_renderpass_2ndpass_depth_test_off_quad_subpass_id;
    Anvil::SubPassID                   m_renderpass_2ndpass_depth_test_off_tri_subpass_id;

    uint32_t       m_n_last_semaphore_used;
    const uint32_t m_n_swapchain_images;

    std::vector<Anvil::SemaphoreUniquePtr> m_frame_signal_semaphores;
    std::vector<Anvil::SemaphoreUniquePtr> m_frame_wait_semaphores;
};
