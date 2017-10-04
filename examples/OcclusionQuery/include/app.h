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

    static void     draw_frame            (void*                      app_raw_ptr);
    static VkBool32 on_validation_callback(VkDebugReportFlagsEXT      message_flags,
                                           VkDebugReportObjectTypeEXT object_type,
                                           const char*                layer_prefix,
                                           const char*                message,
                                           void*                      user_arg);

    /* Private variables */
    std::weak_ptr<Anvil::SGPUDevice>         m_device_ptr;
    std::shared_ptr<Anvil::Instance>         m_instance_ptr;
    std::weak_ptr<Anvil::PhysicalDevice>     m_physical_device_ptr;
    std::shared_ptr<Anvil::Queue>            m_present_queue_ptr;
    std::shared_ptr<Anvil::RenderingSurface> m_rendering_surface_ptr;
    std::shared_ptr<Anvil::Swapchain>        m_swapchain_ptr;
    Anvil::Time                              m_time;
    std::shared_ptr<Anvil::Window>           m_window_ptr;

    std::shared_ptr<Anvil::Image>     m_depth_image_ptr;
    std::shared_ptr<Anvil::ImageView> m_depth_image_view_ptr;

    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> m_quad_fs_ptr;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> m_quad_vs_ptr;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> m_tri_fs_ptr;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> m_tri_vs_ptr;

    std::shared_ptr<Anvil::DescriptorSetGroup> m_1stpass_dsg_ptr;
    std::shared_ptr<Anvil::DescriptorSetGroup> m_2ndpass_quad_dsg_ptr;
    std::shared_ptr<Anvil::DescriptorSetGroup> m_2ndpass_tri_dsg_ptr;

    std::shared_ptr<Anvil::QueryPool> m_query_pool_ptr;

    uint32_t                       m_n_bytes_per_query;
    std::shared_ptr<Anvil::Buffer> m_query_bo_ptr;
    std::shared_ptr<Anvil::Event>  m_query_data_copied_event;
    std::shared_ptr<Anvil::Buffer> m_time_bo_ptr;
    uint32_t                       m_time_n_bytes_per_swapchain_image;

    std::shared_ptr<Anvil::PrimaryCommandBuffer> m_render_tri1_and_generate_ot_data_cmd_buffers[N_SWAPCHAIN_IMAGES];
    std::shared_ptr<Anvil::PrimaryCommandBuffer> m_render_tri2_and_quad_cmd_buffers            [N_SWAPCHAIN_IMAGES];

    Anvil::GraphicsPipelineID m_1stpass_depth_test_always_pipeline_id;
    Anvil::GraphicsPipelineID m_1stpass_depth_test_equal_pipeline_id;
    Anvil::GraphicsPipelineID m_2ndpass_depth_test_off_quad_pipeline_id;
    Anvil::GraphicsPipelineID m_2ndpass_depth_test_off_tri_pipeline_id;

    std::shared_ptr<Anvil::Framebuffer> m_fbos[N_SWAPCHAIN_IMAGES];

    std::shared_ptr<Anvil::RenderPass> m_renderpass_quad_ptr;
    std::shared_ptr<Anvil::RenderPass> m_renderpass_tris_ptr;

    Anvil::SubPassID                   m_renderpass_1stpass_depth_test_always_subpass_id;
    Anvil::SubPassID                   m_renderpass_1stpass_depth_test_equal_ot_subpass_id;
    Anvil::SubPassID                   m_renderpass_2ndpass_depth_test_off_quad_subpass_id;
    Anvil::SubPassID                   m_renderpass_2ndpass_depth_test_off_tri_subpass_id;

    uint32_t       m_n_last_semaphore_used;
    const uint32_t m_n_swapchain_images;

    std::vector<std::shared_ptr<Anvil::Semaphore> > m_frame_signal_semaphores;
    std::vector<std::shared_ptr<Anvil::Semaphore> > m_frame_wait_semaphores;
};
