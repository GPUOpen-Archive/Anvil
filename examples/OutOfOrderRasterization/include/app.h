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

    void clear_console_line();

    void deinit              ();
    void init_buffers        ();
    void init_command_buffers();
    void init_dsgs           ();
    void init_events         ();
    void init_images         ();
    void init_gfx_pipelines  ();
    void init_query_pool     ();
    void init_renderpasses   ();
    void init_semaphores     ();
    void init_shaders        ();
    void init_swapchain      ();
    void init_window         ();
    void init_vulkan         ();

    void update_fps         ();
    void update_teapot_props(uint32_t n_current_swapchain_image);

    static void     draw_frame            (void*                      app_raw_ptr);
    static void     on_keypress_event     (void*                      callback_data_raw_ptr,
                                           void*                      app_raw_ptr);
    static VkBool32 on_validation_callback(VkDebugReportFlagsEXT      message_flags,
                                           VkDebugReportObjectTypeEXT object_type,
                                           const char*                layer_prefix,
                                           const char*                message,
                                           void*                      user_arg);

    /* Private variables */
    std::weak_ptr<Anvil::BaseDevice>         m_device_ptr;
    std::shared_ptr<Anvil::Instance>         m_instance_ptr;
    std::shared_ptr<Anvil::Queue>            m_present_queue_ptr;
    std::shared_ptr<Anvil::QueryPool>        m_query_pool_ptr;
    std::shared_ptr<Anvil::RenderingSurface> m_rendering_surface_ptr;
    std::shared_ptr<Anvil::Swapchain>        m_swapchain_ptr;
    std::shared_ptr<Anvil::Window>           m_window_ptr;

    std::shared_ptr<Anvil::Image>                              m_depth_image_ptr;
    std::shared_ptr<Anvil::ImageView>                          m_depth_image_view_ptr;
    std::vector<std::shared_ptr<Anvil::Framebuffer> >          m_framebuffers;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint>        m_fs_entrypoint_ptr;
    std::vector<std::shared_ptr<Anvil::PrimaryCommandBuffer> > m_render_cmdbuffers_ooo_on;
    std::vector<std::shared_ptr<Anvil::PrimaryCommandBuffer> > m_render_cmdbuffers_ooo_off;
    std::vector<std::shared_ptr<Anvil::RenderPass> >           m_renderpasses;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint>        m_vs_entrypoint_ptr;

    uint32_t               m_n_indices;
    uint32_t               m_n_last_semaphore_used;
    const uint32_t         m_n_swapchain_images;
    bool                   m_ooo_enabled;
    bool                   m_should_rotate;
    std::shared_ptr<float> m_teapot_props_data_ptr;
    Anvil::Time            m_time;

    typedef struct
    {
        /* Holds as many semaphores as there are physical devices bound to a logical device */
        std::vector<std::shared_ptr<Anvil::Semaphore> > semaphores;
    } SemaphoreBundle;

    std::vector<SemaphoreBundle> m_frame_signal_semaphore_bundles;
    std::vector<SemaphoreBundle> m_frame_wait_semaphore_bundles;

    bool                      m_frame_drawn_status[N_SWAPCHAIN_IMAGES];
    Anvil::GraphicsPipelineID m_general_pipeline_id;
    Anvil::GraphicsPipelineID m_ooo_disabled_pipeline_id;
    Anvil::GraphicsPipelineID m_ooo_enabled_pipeline_id;

    std::shared_ptr<Anvil::Buffer> m_index_buffer_ptr;
    std::shared_ptr<Anvil::Buffer> m_query_results_buffer_ptr;
    std::shared_ptr<Anvil::Buffer> m_vertex_buffer_ptr;

    std::vector<std::shared_ptr<Anvil::DescriptorSetGroup> > m_dsg_ptrs;
    std::vector<std::shared_ptr<Anvil::Buffer> >             m_properties_buffer_ptrs;
    bool                                                     m_properties_data_set;

    uint32_t              m_n_frames_drawn;
    std::vector<uint64_t> m_timestamp_deltas;
};
