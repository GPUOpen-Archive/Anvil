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
    void init_framebuffers   ();
    void init_gfx_pipelines  ();
    void init_semaphores     ();
    void init_shaders        ();
    void init_swapchain      ();
    void init_window         ();
    void init_vulkan         ();

    VkFormat                       get_mesh_color_data_format      ()                       const;
    uint32_t                       get_mesh_color_data_n_components()                       const;
    uint32_t                       get_mesh_color_data_start_offset (uint32_t n_stream,
                                                                     uint32_t n_vertex = 0) const;
    std::shared_ptr<unsigned char> get_mesh_data                    ()                      const;
    uint32_t                       get_mesh_data_size               ()                      const;
    uint32_t                       get_mesh_n_vertices              ()                      const;
    VkFormat                       get_mesh_vertex_data_format      ()                      const;
    uint32_t                       get_mesh_vertex_data_n_components()                      const;
    uint32_t                       get_mesh_vertex_data_start_offset(uint32_t n_vertex = 0) const;

    void get_scissor_viewport_info(VkRect2D*   out_scissors,
                                   VkViewport* out_viewports);

    void     draw_frame            ();
    VkBool32 on_validation_callback(VkDebugReportFlagsEXT      message_flags,
                                    VkDebugReportObjectTypeEXT object_type,
                                    const char*                layer_prefix,
                                    const char*                message);

    /* Private variables */
    std::weak_ptr<Anvil::SGPUDevice>         m_device_ptr;
    std::shared_ptr<Anvil::Instance>         m_instance_ptr;
    std::weak_ptr<Anvil::PhysicalDevice>     m_physical_device_ptr;
    std::shared_ptr<Anvil::Queue>            m_present_queue_ptr;
    std::shared_ptr<Anvil::RenderingSurface> m_rendering_surface_ptr;
    std::shared_ptr<Anvil::Swapchain>        m_swapchain_ptr;
    std::shared_ptr<Anvil::Window>           m_window_ptr;

    std::shared_ptr<Anvil::PrimaryCommandBuffer>        m_command_buffers[N_SWAPCHAIN_IMAGES];
    std::shared_ptr<Anvil::Framebuffer>                 m_fbos           [N_SWAPCHAIN_IMAGES];
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> m_fs_ptr;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> m_gs_ptr;
    Anvil::PipelineID                                   m_pipeline_id;
    std::shared_ptr<Anvil::RenderPass>                  m_renderpass_ptr;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> m_vs_ptr;

    std::shared_ptr<Anvil::Buffer>      m_mesh_data_buffer_ptr;

    uint32_t       m_n_last_semaphore_used;
    const uint32_t m_n_swapchain_images;

    std::vector<std::shared_ptr<Anvil::Semaphore> > m_frame_signal_semaphores;
    std::vector<std::shared_ptr<Anvil::Semaphore> > m_frame_wait_semaphores;
};
