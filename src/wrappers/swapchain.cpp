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

#include "misc/debug.h"
#include "misc/dummy_window.h"
#include "misc/object_tracker.h"
#include "misc/window.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/instance.h"
#include "wrappers/semaphore.h"
#include "wrappers/swapchain.h"

/** Please see header for specification */
Anvil::Swapchain::Swapchain(std::weak_ptr<Anvil::BaseDevice>         in_device_ptr,
                            std::shared_ptr<Anvil::RenderingSurface> in_parent_surface_ptr,
                            std::shared_ptr<Anvil::Window>           in_window_ptr,
                            VkFormat                                 in_format,
                            VkPresentModeKHR                         in_present_mode,
                            VkImageUsageFlags                        in_usage_flags,
                            VkSwapchainCreateFlagsKHR                in_flags,
                            uint32_t                                 in_n_images,
                            const ExtensionKHRSwapchainEntrypoints&  in_khr_swapchain_entrypoints)
    :DebugMarkerSupportProvider                     (in_device_ptr,
                                                     VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT),
     m_destroy_swapchain_before_parent_window_closes(true),
     m_device_ptr                                   (in_device_ptr),
     m_flags                                        (in_flags),
     m_image_format                                 (in_format),
     m_last_acquired_image_index                    (UINT32_MAX),
     m_n_acquire_counter                            (0),
     m_n_acquire_counter_rounded                    (0),
     m_n_present_counter                            (0),
     m_n_swapchain_images                           (in_n_images),
     m_parent_surface_ptr                           (in_parent_surface_ptr),
     m_present_mode                                 (in_present_mode),
     m_swapchain                                    (0),
     m_window_ptr                                   (in_window_ptr),
     m_usage_flags                                  (static_cast<VkImageUsageFlagBits>(in_usage_flags) ),
     m_khr_swapchain_entrypoints                    (in_khr_swapchain_entrypoints)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(in_device_ptr);

    anvil_assert(in_n_images           >  0);
    anvil_assert(in_parent_surface_ptr != nullptr);
    anvil_assert(in_usage_flags        != 0);

    m_image_available_fence_ptrs.resize(in_n_images);
    m_image_ptrs.resize                (in_n_images);
    m_image_view_ptrs.resize           (in_n_images);

    for (uint32_t n_fence = 0;
                  n_fence < in_n_images;
                ++n_fence)
    {
        m_image_available_fence_ptrs[n_fence] = Anvil::Fence::create(m_device_ptr,
                                                                     false /* create_signalled */);
    }

    init();

    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_SWAPCHAIN,
                                                  this);
}

/** Please see header for specification */
Anvil::Swapchain::~Swapchain()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_SWAPCHAIN,
                                                    this);

    destroy_swapchain();

    if (m_parent_surface_ptr != nullptr)
    {
        m_parent_surface_ptr = nullptr;
    }

    m_image_ptrs.clear                ();
    m_image_available_fence_ptrs.clear();
    m_image_view_ptrs.clear           ();

    for (auto queue_ptr : m_observed_queues)
    {
        queue_ptr->unregister_from_callbacks(QUEUE_CALLBACK_ID_PRESENT_REQUEST_ISSUED,
                                             on_present_request_issued,
                                             this); /* in_user_arg */
    }

    m_window_ptr->unregister_from_callbacks(WINDOW_CALLBACK_ID_ABOUT_TO_CLOSE,
                                            on_parent_window_about_to_close,
                                            this);
}

/** Please see header for specification */
uint32_t Anvil::Swapchain::acquire_image(std::shared_ptr<Anvil::Semaphore> in_opt_semaphore_ptr,
                                         bool                              in_should_block)
{
    std::shared_ptr<Anvil::BaseDevice> device_locked_ptr              = m_device_ptr.lock();
    const RenderingSurfaceType         rendering_surface              = m_parent_surface_ptr->get_type();
    uint32_t                           result                         = UINT32_MAX;
    VkResult                           result_vk                      = VK_ERROR_INITIALIZATION_FAILED;
    const WindowPlatform               window_platform                = m_window_ptr->get_platform();
    const bool                         is_offscreen_rendering_enabled = (window_platform   == WINDOW_PLATFORM_DUMMY                     ||
                                                                         window_platform   == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS) &&
                                                                        (rendering_surface == Anvil::RENDERING_SURFACE_TYPE_GENERAL);

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    if (!is_offscreen_rendering_enabled)
    {
        VkFence fence_handle = VK_NULL_HANDLE;

        if (in_should_block)
        {
            m_image_available_fence_ptrs.at(m_n_acquire_counter_rounded)->reset();

            fence_handle = m_image_available_fence_ptrs.at(m_n_acquire_counter_rounded)->get_fence();
        }

        result_vk = m_khr_swapchain_entrypoints.vkAcquireNextImageKHR(device_locked_ptr->get_device_vk(),
                                                                      m_swapchain,
                                                                      UINT64_MAX,
                                                                      in_opt_semaphore_ptr->get_semaphore(),
                                                                      fence_handle,
                                                                     &result);

        anvil_assert_vk_call_succeeded(result_vk);

        if (fence_handle != VK_NULL_HANDLE)
        {
            result_vk = vkWaitForFences(device_locked_ptr->get_device_vk(),
                                        1, /* fenceCount */
                                       &fence_handle,
                                        VK_TRUE, /* waitAll */
                                        UINT64_MAX);

            anvil_assert_vk_call_succeeded(result_vk);
        }

        /* NOTE: Only bump the frame acquisition counter if we're not emulating a Vulkan swapchain */
        m_n_acquire_counter++;
        m_n_acquire_counter_rounded = (m_n_acquire_counter_rounded + 1) % m_n_swapchain_images;
    }
    else
    {
        /* We need to set the semaphore manually in this scenario */
        device_locked_ptr->get_universal_queue(0)->submit_command_buffer_with_signal_semaphores(nullptr, /* cmd_buffer_ptr         */
                                                                                                1,       /* n_semaphores_to_signal */
                                                                                               &in_opt_semaphore_ptr,
                                                                                                true); /* should_block */
    }

    if (is_offscreen_rendering_enabled)
    {
        result = m_n_acquire_counter_rounded;
    }

    m_last_acquired_image_index = result;

    return result;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Swapchain> Anvil::Swapchain::create(std::weak_ptr<Anvil::BaseDevice>         in_device_ptr,
                                                           std::shared_ptr<Anvil::RenderingSurface> in_parent_surface_ptr,
                                                           std::shared_ptr<Anvil::Window>           in_window_ptr,
                                                           VkFormat                                 in_format,
                                                           VkPresentModeKHR                         in_present_mode,
                                                           VkImageUsageFlags                        in_usage_flags,
                                                           uint32_t                                 in_n_images,
                                                           const ExtensionKHRSwapchainEntrypoints&  in_khr_swapchain_entrypoints,
                                                           VkSwapchainCreateFlagsKHR                in_flags)
{
    std::shared_ptr<Anvil::Swapchain> result_ptr;

    result_ptr.reset(
        new Anvil::Swapchain(in_device_ptr,
                             in_parent_surface_ptr,
                             in_window_ptr,
                             in_format,
                             in_present_mode,
                             in_usage_flags,
                             in_flags,
                             in_n_images,
                             in_khr_swapchain_entrypoints)
    );

    if (in_window_ptr                 != nullptr                                  &&
        in_window_ptr->get_platform() == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS)
    {
        std::dynamic_pointer_cast<Anvil::DummyWindowWithPNGSnapshots>(in_window_ptr)->set_swapchain(result_ptr);
    }

    return result_ptr;
}

/** TODO */
void Anvil::Swapchain::destroy_swapchain()
{
    /* If this assertion failure explodes, your application attempted to release a swapchain without presenting all acquired swapchain images.
     * That's illegal per Vulkan spec.
     */
    anvil_assert(m_n_acquire_counter == m_n_present_counter);
    if (m_swapchain != VK_NULL_HANDLE)
    {
        std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

        m_khr_swapchain_entrypoints.vkDestroySwapchainKHR(device_locked_ptr->get_device_vk(),
                                                          m_swapchain,
                                                          nullptr /* pAllocator */);

        m_swapchain = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
void Anvil::Swapchain::disable_destroy_swapchain_before_parent_window_closes_behavior()
{
    m_destroy_swapchain_before_parent_window_closes = false;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Image> Anvil::Swapchain::get_image(uint32_t in_n_swapchain_image) const
{
    anvil_assert(in_n_swapchain_image < m_n_swapchain_images);

    return m_image_ptrs[in_n_swapchain_image];
}

/** Please see header for specification */
std::shared_ptr<Anvil::ImageView> Anvil::Swapchain::get_image_view(uint32_t in_n_swapchain_image) const
{
    anvil_assert(in_n_swapchain_image < m_n_swapchain_images);

    return m_image_view_ptrs[in_n_swapchain_image];
}

/** Initializes the swapchain object. */
void Anvil::Swapchain::init()
{
    VkSwapchainCreateInfoKHR            create_info;
    std::shared_ptr<Anvil::BaseDevice>  device_locked_ptr              = m_device_ptr.lock();
    uint32_t                            n_swapchain_images             = 0;
    VkResult                            result                         = VK_ERROR_INITIALIZATION_FAILED;
    const RenderingSurfaceType          rendering_surface              = m_parent_surface_ptr->get_type();
    std::vector<VkImage>                swapchain_images;
    const VkSurfaceTransformFlagBitsKHR swapchain_transformation       = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    const WindowPlatform                window_platform                = m_window_ptr->get_platform();
    const bool                          is_offscreen_rendering_enabled = (window_platform   == WINDOW_PLATFORM_DUMMY                     ||
                                                                          window_platform   == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS) &&
                                                                         (rendering_surface == Anvil::RENDERING_SURFACE_TYPE_GENERAL);

    ANVIL_REDUNDANT_VARIABLE(result);

    m_image_view_format = m_image_format;
    m_size.width        = m_parent_surface_ptr->get_width ();
    m_size.height       = m_parent_surface_ptr->get_height();

    /* not doing offscreen rendering */
    if (!is_offscreen_rendering_enabled)
    {
        std::shared_ptr<Anvil::SGPUDevice> sgpu_device_locked_ptr(std::dynamic_pointer_cast<Anvil::SGPUDevice>(device_locked_ptr) );

        #ifdef _DEBUG
        {
            uint32_t                   n_physical_devices              = 1;
            bool                       result_bool                     = false;
            const char*                required_surface_extension_name = nullptr;
            VkSurfaceCapabilitiesKHR   surface_caps;
            VkCompositeAlphaFlagsKHR   supported_composite_alpha_flags = static_cast<VkCompositeAlphaFlagsKHR>(0);
            VkSurfaceTransformFlagsKHR supported_surface_transform_flags;

            #ifdef _WIN32
                #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
                    required_surface_extension_name = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
                #endif
            #else
                #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
                    required_surface_extension_name = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
                #endif
            #endif

            anvil_assert(required_surface_extension_name == nullptr                                                                          ||
                         m_device_ptr.lock()->get_parent_instance().lock()->is_instance_extension_supported(required_surface_extension_name) );

            for (uint32_t n_physical_device = 0;
                          n_physical_device < n_physical_devices;
                        ++n_physical_device)
            {
                std::shared_ptr<Anvil::PhysicalDevice> current_physical_device_ptr(sgpu_device_locked_ptr->get_physical_device() );

                /* Ensure opaque composite alpha mode is supported */
                anvil_assert(m_parent_surface_ptr->get_supported_composite_alpha_flags(current_physical_device_ptr,
                                                                                      &supported_composite_alpha_flags) );

                anvil_assert(supported_composite_alpha_flags & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

                /* Ensure we can use the swapchain image format  */
                anvil_assert(m_parent_surface_ptr->is_compatible_with_image_format(current_physical_device_ptr,
                                                                                   m_image_format,
                                                                                  &result_bool) );
                anvil_assert(result_bool);

                /* Ensure the transformation we're about to request is supported by the rendering surface */
                anvil_assert(m_parent_surface_ptr->get_supported_transformations(current_physical_device_ptr,
                                                                                &supported_surface_transform_flags) );

                anvil_assert(supported_surface_transform_flags & swapchain_transformation);

                /* Ensure the requested number of swapchain images is reasonable*/
                anvil_assert(m_parent_surface_ptr->get_capabilities(current_physical_device_ptr,
                                                                   &surface_caps) );

                anvil_assert(surface_caps.maxImageCount == 0                    ||
                             surface_caps.maxImageCount >= m_n_swapchain_images);
            }
        }
        #endif

        create_info.clipped               = true; /* we won't be reading from the presentable images */
        create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.flags                 = m_flags;
        create_info.imageArrayLayers      = 1;
        create_info.imageColorSpace       = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        create_info.imageExtent.height    = m_parent_surface_ptr->get_height();
        create_info.imageExtent.width     = m_parent_surface_ptr->get_width();
        create_info.imageFormat           = m_image_format;
        create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        create_info.imageUsage            = m_usage_flags;
        create_info.minImageCount         = m_n_swapchain_images;
        create_info.oldSwapchain          = VK_NULL_HANDLE;
        create_info.pNext                 = nullptr;
        create_info.pQueueFamilyIndices   = nullptr;
        create_info.presentMode           = m_present_mode;
        create_info.preTransform          = swapchain_transformation;
        create_info.queueFamilyIndexCount = 0;
        create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface               = m_parent_surface_ptr->get_surface();

        result = m_khr_swapchain_entrypoints.vkCreateSwapchainKHR(device_locked_ptr->get_device_vk(),
                                                                 &create_info,
                                                                  nullptr, /* pAllocator */
                                                                 &m_swapchain);
        anvil_assert_vk_call_succeeded(result);
        if (is_vk_call_successful(result) )
        {
            set_vk_handle(m_swapchain);
        }

        /* Retrieve swap-chain images */
        result = m_khr_swapchain_entrypoints.vkGetSwapchainImagesKHR(device_locked_ptr->get_device_vk(),
                                                                     m_swapchain,
                                                                    &n_swapchain_images,
                                                                     nullptr); /* pSwapchainImages */

        anvil_assert_vk_call_succeeded(result);
        anvil_assert                  (n_swapchain_images >  0);

        swapchain_images.resize(n_swapchain_images);

        result = m_khr_swapchain_entrypoints.vkGetSwapchainImagesKHR(device_locked_ptr->get_device_vk(),
                                                                     m_swapchain,
                                                                    &n_swapchain_images,
                                                                    &swapchain_images[0]);

        anvil_assert_vk_call_succeeded(result);
    }
    else /* offscreen rendering */
    {
        m_usage_flags      = static_cast<VkImageUsageFlagBits>(static_cast<VkImageUsageFlags>(m_usage_flags) | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        n_swapchain_images = m_n_swapchain_images;
    }

    for (size_t n_result_image = 0;
                n_result_image < n_swapchain_images;
              ++n_result_image)
    {
        /* Spawn an Image wrapper class for the swap-chain image. */
        if (!is_offscreen_rendering_enabled)
        {
            /* NOTE: The constructor we use below is special, in that the wrapped VkImage instance will not be destroyed
             *       when the Image instance goes out of scope.
             */
            m_image_ptrs[n_result_image] = Anvil::Image::create_nonsparse(m_device_ptr,
                                                                          create_info,
                                                                          swapchain_images[n_result_image]);
        }
        else
        {
            m_image_ptrs[n_result_image] = Anvil::Image::create_nonsparse(m_device_ptr,
                                                                          VK_IMAGE_TYPE_2D,
                                                                          m_image_format,
                                                                          VK_IMAGE_TILING_OPTIMAL,
                                                                          m_usage_flags,
                                                                          m_size.width,
                                                                          m_size.height,
                                                                          1, /* base_mipmap_depth */
                                                                          1,
                                                                          VK_SAMPLE_COUNT_1_BIT,
                                                                          QUEUE_FAMILY_GRAPHICS_BIT,
                                                                          VK_SHARING_MODE_EXCLUSIVE,
                                                                          false, /* in_use_full_mipmap_chain */
                                                                          0,     /* in_memory_features       */
                                                                          0,     /* in_create_flags          */
                                                                          VK_IMAGE_LAYOUT_GENERAL,
                                                                          nullptr);
        }

        /* For each swap-chain image, create a relevant view */
        m_image_view_ptrs[n_result_image] = Anvil::ImageView::create_2D(m_device_ptr,
                                                                         m_image_ptrs[n_result_image],
                                                                         0, /* n_base_layer */
                                                                         0, /* n_base_mipmap_level */
                                                                         1, /* n_mipmaps           */
                                                                         VK_IMAGE_ASPECT_COLOR_BIT,
                                                                         m_image_format,
                                                                         VK_COMPONENT_SWIZZLE_R,
                                                                         VK_COMPONENT_SWIZZLE_G,
                                                                         VK_COMPONENT_SWIZZLE_B,
                                                                         VK_COMPONENT_SWIZZLE_A);
    }

    /* Sign up for present submission notifications. This is needed to ensure that number of presented frames ==
     * number of acquired frames at destruction time.
     */
    {
        std::vector<std::shared_ptr<Anvil::Queue> > queues;

        const std::vector<uint32_t>*       queue_fams_with_present_support_ptr(nullptr);
        std::shared_ptr<Anvil::SGPUDevice> sgpu_device_locked_ptr             (std::dynamic_pointer_cast<Anvil::SGPUDevice>(device_locked_ptr) );
        auto                               physical_device_ptr                (sgpu_device_locked_ptr->get_physical_device() );

        if (!m_parent_surface_ptr->get_queue_families_with_present_support(physical_device_ptr,
                                                                          &queue_fams_with_present_support_ptr) )
        {
            anvil_assert_fail();
        }

        if (queue_fams_with_present_support_ptr == nullptr)
        {
            anvil_assert(queue_fams_with_present_support_ptr != nullptr);
        }
        else
        {
            for (const auto queue_fam : *queue_fams_with_present_support_ptr)
            {
                const uint32_t n_queues = sgpu_device_locked_ptr->get_n_queues(queue_fam);

                for (uint32_t n_queue = 0;
                              n_queue < n_queues;
                            ++n_queue)
                {
                    auto queue_ptr = sgpu_device_locked_ptr->get_queue(queue_fam,
                                                                       n_queue);

                    anvil_assert(queue_ptr != nullptr);

                    if (std::find(queues.begin(),
                                  queues.end(),
                                  queue_ptr) == queues.end() )
                    {
                        queues.push_back(queue_ptr);
                    }
                }
            }
        }

        for (auto queue_ptr : queues)
        {
            queue_ptr->register_for_callbacks(QUEUE_CALLBACK_ID_PRESENT_REQUEST_ISSUED,
                                              on_present_request_issued,
                                              this); /* in_user_arg */

            m_observed_queues.push_back(queue_ptr);
        }
    }

    /* Sign up for "about to close the parent window" notifications. Swapchain instance SHOULD be deinitialized
     * before the window is destroyed, so we're going to act as nice citizens.
     */
    m_window_ptr->register_for_callbacks(WINDOW_CALLBACK_ID_ABOUT_TO_CLOSE,
                                         on_parent_window_about_to_close,
                                         this);
}

/** TODO */
void Anvil::Swapchain::on_parent_window_about_to_close(void* in_window_ptr,
                                                       void* in_swapchain_raw_ptr)
{
    Anvil::Swapchain* swapchain_ptr = static_cast<Anvil::Swapchain*>(in_swapchain_raw_ptr);

    ANVIL_REDUNDANT_ARGUMENT(in_window_ptr);
    anvil_assert            (swapchain_ptr != nullptr);

    if (swapchain_ptr->m_destroy_swapchain_before_parent_window_closes)
    {
        swapchain_ptr->destroy_swapchain();
    }
}

/** TODO */
void Anvil::Swapchain::on_present_request_issued(void* in_queue_raw_ptr,
                                                 void* in_swapchain_raw_ptr)
{
    Anvil::Swapchain* swapchain_ptr = static_cast<Anvil::Swapchain*>(in_swapchain_raw_ptr);

    ANVIL_REDUNDANT_ARGUMENT(in_queue_raw_ptr);

    if (swapchain_ptr->m_swapchain != VK_NULL_HANDLE)
    {
        /* Only bump the present counter if a real (ie. non-emulated) swapchain is being used. */
        swapchain_ptr->m_n_present_counter++;
    }
}