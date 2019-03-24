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

#define N_SWAPCHAIN_IMAGES (2)


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

    void draw_frame            ();
    void on_keypress_event     (Anvil::CallbackArgument*         in_callback_data_raw_ptr);
    void on_validation_callback(Anvil::DebugMessageSeverityFlags in_severity,
                                 const char*                     in_message_ptr);

    #if defined(ENABLE_MGPU_SUPPORT)
        std::vector<VkRect2D> get_render_areas(uint32_t in_afr_render_index = 0) const;
    #endif

    /* Private variables */
    Anvil::BaseDeviceUniquePtr m_device_ptr;

    Anvil::InstanceUniquePtr         m_instance_ptr;
    Anvil::Queue*                    m_present_queue_ptr;
    Anvil::QueryPoolUniquePtr        m_query_pool_ptr;
    Anvil::RenderingSurfaceUniquePtr m_rendering_surface_ptr;
    Anvil::SwapchainUniquePtr        m_swapchain_ptr;
    Anvil::WindowUniquePtr           m_window_ptr;

    #if defined(ENABLE_EXPLICIT_SWAPCHAIN_IMAGE_MEMORY_BINDING)
        std::vector<Anvil::ImageUniquePtr>     m_swapchain_images;
        std::vector<Anvil::ImageViewUniquePtr> m_swapchain_image_views;
    #endif

    Anvil::ImageUniquePtr                               m_depth_image_ptr;
    Anvil::ImageViewUniquePtr                           m_depth_image_view_ptr;
    std::vector<Anvil::FramebufferUniquePtr>            m_framebuffers;
    std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> m_fs_entrypoint_ptr;

    #if !defined(ENABLE_MGPU_SUPPORT)
        std::vector<Anvil::PrimaryCommandBufferUniquePtr> m_render_cmdbuffers_ooo_off;
        std::vector<Anvil::PrimaryCommandBufferUniquePtr> m_render_cmdbuffers_ooo_on;
    #else
        Anvil::PrimaryCommandBufferUniquePtr                                                               m_dummy_cmdbuffer_ptr;
        std::map<uint32_t /* physical device index */, std::vector<Anvil::PrimaryCommandBufferUniquePtr> > m_render_cmdbuffers_ooo_on;
        std::map<uint32_t /* physical device index */, std::vector<Anvil::PrimaryCommandBufferUniquePtr> > m_render_cmdbuffers_ooo_off;
    #endif

    std::vector<Anvil::RenderPassUniquePtr>             m_renderpasses;
    std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> m_vs_entrypoint_ptr;

    uint32_t               m_n_indices;
    uint32_t               m_n_last_semaphore_used;
    uint32_t               m_n_swapchain_images;
    bool                   m_ooo_enabled;
    bool                   m_should_rotate;
    std::unique_ptr<float> m_teapot_props_data_ptr;
    Anvil::Time            m_time;

    #if defined(USE_LOCAL_SFR_PRESENTATION_MODE)
        struct SwapchainPeerImages
        {
            /* Holds N_SWAPCHAIN_IMAGES peer images for consecutive swapchain image indices */
            std::vector<Anvil::ImageUniquePtr> peer_images;
        };

        uint32_t                         m_n_presenting_physical_device;
        std::vector<SwapchainPeerImages> m_swapchain_peer_images_per_physical_device;
    #endif
    #if defined(USE_LOCAL_AFR_PRESENTATION_MODE) || defined(USE_REMOTE_AFR_PRESENTATION_MODE)
        uint32_t m_n_rendering_physical_device;
    #endif

    typedef struct SemaphoreBundle
    {
        SemaphoreBundle()
        {
            /* Stub */
        }

        /* Holds as many semaphores as there are physical devices bound to a logical device */
        std::vector<Anvil::SemaphoreUniquePtr> semaphores;

    private:
        SemaphoreBundle           (const SemaphoreBundle&) = delete;
        SemaphoreBundle& operator=(const SemaphoreBundle&) = delete;
    } SemaphoreBundle;

    std::vector<Anvil::SemaphoreUniquePtr>          m_frame_acquisition_wait_semaphores;
    std::vector<std::unique_ptr<SemaphoreBundle> >  m_frame_signal_semaphore_bundles;
    std::vector<std::unique_ptr<SemaphoreBundle> >  m_frame_wait_semaphore_bundles;

    bool              m_frame_drawn_status[N_SWAPCHAIN_IMAGES];
    Anvil::PipelineID m_general_pipeline_id;
    Anvil::PipelineID m_ooo_disabled_pipeline_id;
    Anvil::PipelineID m_ooo_enabled_pipeline_id;

    Anvil::BufferUniquePtr m_index_buffer_ptr;
    Anvil::BufferUniquePtr m_query_results_buffer_ptr;
    Anvil::BufferUniquePtr m_vertex_buffer_ptr;

    std::vector<Anvil::DescriptorSetGroupUniquePtr> m_dsg_ptrs;
    std::vector<Anvil::BufferUniquePtr>             m_properties_buffer_ptrs;
    bool                                            m_properties_data_set;

    uint32_t              m_n_frames_drawn;
    std::vector<uint64_t> m_timestamp_deltas;
};
