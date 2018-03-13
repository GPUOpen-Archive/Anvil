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
#include "misc/struct_chainer.h"
#include "misc/window.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/instance.h"
#include "wrappers/semaphore.h"
#include "wrappers/swapchain.h"

/** Please see header for specification */
Anvil::Swapchain::Swapchain(const Anvil::BaseDevice*                 in_device_ptr,
                            Anvil::RenderingSurface*                 in_parent_surface_ptr,
                            Anvil::Window*                           in_window_ptr,
                            VkFormat                                 in_format,
                            VkPresentModeKHR                         in_present_mode,
                            VkImageUsageFlags                        in_usage_flags,
                            VkSwapchainCreateFlagsKHR                in_flags,
                            uint32_t                                 in_n_images,
                            const ExtensionKHRSwapchainEntrypoints&  in_khr_swapchain_entrypoints,
                            bool                                     in_mt_safe)
    :DebugMarkerSupportProvider                     (in_device_ptr,
                                                     VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT),
     MTSafetySupportProvider                        (in_mt_safe),
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
    anvil_assert(in_n_images           >  0);
    anvil_assert(in_parent_surface_ptr != nullptr);
    anvil_assert(in_usage_flags        != 0);

    m_image_ptrs.resize     (in_n_images);
    m_image_view_ptrs.resize(in_n_images);

    m_image_available_fence_ptr = Anvil::Fence::create(m_device_ptr,
                                                       false,  /* create_signalled */
                                                       Anvil::Utils::convert_boolean_to_mt_safety_enum(in_mt_safe) );

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

    m_image_ptrs.clear               ();
    m_image_available_fence_ptr.reset();
    m_image_view_ptrs.clear          ();

    for (auto queue_ptr : m_observed_queues)
    {
        queue_ptr->unregister_from_callbacks(
            QUEUE_CALLBACK_ID_PRESENT_REQUEST_ISSUED,
            std::bind(&Swapchain::on_present_request_issued,
                      this,
                      std::placeholders::_1),
            this
        );
    }

    m_window_ptr->unregister_from_callbacks(
        WINDOW_CALLBACK_ID_ABOUT_TO_CLOSE,
        std::bind(&Swapchain::on_parent_window_about_to_close,
                  this),
        this
    );
}

/** Please see header for specification */
uint32_t Anvil::Swapchain::acquire_image(Anvil::Semaphore* in_opt_semaphore_ptr,
                                         bool              in_should_block)
{
    uint32_t                 result                        (UINT32_MAX);
    VkResult                 result_vk                     (VK_ERROR_INITIALIZATION_FAILED);
    const WindowPlatform     window_platform               (m_window_ptr->get_platform());
    const bool               is_offscreen_rendering_enabled((window_platform   == WINDOW_PLATFORM_DUMMY                     ||
                                                             window_platform   == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS) );

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    if (!is_offscreen_rendering_enabled)
    {
        VkFence fence_handle = VK_NULL_HANDLE;

        if (in_opt_semaphore_ptr != nullptr)
        {
            in_opt_semaphore_ptr->lock();
        }

        m_image_available_fence_ptr->lock();
        lock();
        {
            if (in_should_block)
            {
                m_image_available_fence_ptr->reset();

                fence_handle = m_image_available_fence_ptr->get_fence();
            }

            result_vk = m_khr_swapchain_entrypoints.vkAcquireNextImageKHR(m_device_ptr->get_device_vk(),
                                                                          m_swapchain,
                                                                          UINT64_MAX,
                                                                          (in_opt_semaphore_ptr != nullptr) ? in_opt_semaphore_ptr->get_semaphore() : VK_NULL_HANDLE,
                                                                          fence_handle,
                                                                         &result);

            if (fence_handle != VK_NULL_HANDLE)
            {
                result_vk = vkWaitForFences(m_device_ptr->get_device_vk(),
                                            1, /* fenceCount */
                                           &fence_handle,
                                            VK_TRUE, /* waitAll */
                                            UINT64_MAX);

                anvil_assert_vk_call_succeeded(result_vk);
            }
        }
        unlock();
        m_image_available_fence_ptr->unlock();

        if (in_opt_semaphore_ptr != nullptr)
        {
            in_opt_semaphore_ptr->unlock();
        }

        anvil_assert_vk_call_succeeded(result_vk);
    }
    else
    {
        if (in_should_block)
        {
            m_device_ptr->wait_idle();
        }

        if (in_opt_semaphore_ptr != nullptr)
        {
            /* We need to set the semaphore manually in this scenario */
            m_device_ptr->get_universal_queue(0)->submit_command_buffer_with_signal_semaphores(nullptr, /* cmd_buffer_ptr         */
                                                                                                1,       /* n_semaphores_to_signal */
                                                                                               &in_opt_semaphore_ptr,
                                                                                                true); /* should_block */
        }

        result = m_n_acquire_counter_rounded;
    }

    m_n_acquire_counter++;
    m_n_acquire_counter_rounded = (m_n_acquire_counter_rounded + 1) % m_n_swapchain_images;

    m_last_acquired_image_index = result;

    return result;
}

/** Please see header for specification */
Anvil::SwapchainUniquePtr Anvil::Swapchain::create(const Anvil::BaseDevice*                in_device_ptr,
                                                   Anvil::RenderingSurface*                in_parent_surface_ptr,
                                                   Anvil::Window*                          in_window_ptr,
                                                   VkFormat                                in_format,
                                                   VkPresentModeKHR                        in_present_mode,
                                                   VkImageUsageFlags                       in_usage_flags,
                                                   uint32_t                                in_n_images,
                                                   const ExtensionKHRSwapchainEntrypoints& in_khr_swapchain_entrypoints,
                                                   MTSafety                                in_mt_safety,
                                                   VkSwapchainCreateFlagsKHR               in_flags)
{
    const bool         mt_safe    = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                    in_device_ptr);
    SwapchainUniquePtr result_ptr(nullptr,
                                  std::default_delete<Swapchain>() );

    result_ptr.reset(
        new Anvil::Swapchain(in_device_ptr,
                             in_parent_surface_ptr,
                             in_window_ptr,
                             in_format,
                             in_present_mode,
                             in_usage_flags,
                             in_flags,
                             in_n_images,
                             in_khr_swapchain_entrypoints,
                             mt_safe)
    );

    if (in_window_ptr                 != nullptr                                  &&
        in_window_ptr->get_platform() == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS)
    {
        dynamic_cast<Anvil::DummyWindowWithPNGSnapshots*>(in_window_ptr)->set_swapchain(result_ptr.get() );
    }

    return result_ptr;
}

/** TODO */
void Anvil::Swapchain::destroy_swapchain()
{
    /* If this assertion failure explodes, your application attempted to release a swapchain without presenting all acquired swapchain images.
     * That's illegal per Vulkan spec.
     */
    if (m_swapchain != VK_NULL_HANDLE)
    {
        anvil_assert(m_n_acquire_counter == m_n_present_counter);

        lock();
        {
            m_khr_swapchain_entrypoints.vkDestroySwapchainKHR(m_device_ptr->get_device_vk(),
                                                              m_swapchain,
                                                              nullptr /* pAllocator */);
        }
        unlock();

        m_swapchain = VK_NULL_HANDLE;
    }
}

/** Please see header for specification */
void Anvil::Swapchain::disable_destroy_swapchain_before_parent_window_closes_behavior()
{
    m_destroy_swapchain_before_parent_window_closes = false;
}

/** Please see header for specification */
Anvil::Image* Anvil::Swapchain::get_image(uint32_t in_n_swapchain_image) const
{
    anvil_assert(in_n_swapchain_image < m_n_swapchain_images);

    return m_image_ptrs[in_n_swapchain_image].get();
}

/** Please see header for specification */
Anvil::ImageView* Anvil::Swapchain::get_image_view(uint32_t in_n_swapchain_image) const
{
    anvil_assert(in_n_swapchain_image < m_n_swapchain_images);

    return m_image_view_ptrs[in_n_swapchain_image].get();
}

/** Initializes the swapchain object. */
void Anvil::Swapchain::init()
{
    uint32_t                                              n_swapchain_images             = 0;
    VkResult                                              result                         = VK_ERROR_INITIALIZATION_FAILED;
    Anvil::StructChainUniquePtr<VkSwapchainCreateInfoKHR> struct_chain_ptr;
    std::vector<VkImage>                                  swapchain_images;
    const VkSurfaceTransformFlagBitsKHR                   swapchain_transformation       = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    const WindowPlatform                                  window_platform                = m_window_ptr->get_platform();
    const bool                                            is_offscreen_rendering_enabled = (window_platform   == WINDOW_PLATFORM_DUMMY                     ||
                                                                                            window_platform   == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS);

    ANVIL_REDUNDANT_VARIABLE(result);

    m_image_view_format = m_image_format;
    m_size.width        = m_parent_surface_ptr->get_width ();
    m_size.height       = m_parent_surface_ptr->get_height();

    /* not doing offscreen rendering */
    if (!is_offscreen_rendering_enabled)
    {
        Anvil::StructChainer<VkSwapchainCreateInfoKHR> struct_chainer;

        #ifdef _DEBUG
        {
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

            anvil_assert(required_surface_extension_name == nullptr                                                            ||
                         m_device_ptr->get_parent_instance()->is_instance_extension_supported(required_surface_extension_name) );

            /* Ensure opaque composite alpha mode is supported */
            anvil_assert(m_parent_surface_ptr->get_supported_composite_alpha_flags(&supported_composite_alpha_flags) );

            anvil_assert(supported_composite_alpha_flags & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

            /* Ensure we can use the swapchain image format  */
            anvil_assert(m_parent_surface_ptr->is_compatible_with_image_format(m_image_format,
                                                                              &result_bool) );
            anvil_assert(result_bool);

            /* Ensure the transformation we're about to request is supported by the rendering surface */
            anvil_assert(m_parent_surface_ptr->get_supported_transformations(&supported_surface_transform_flags) );

            anvil_assert(supported_surface_transform_flags & swapchain_transformation);

            /* Ensure the requested number of swapchain images is reasonable*/
            anvil_assert(m_parent_surface_ptr->get_capabilities(&surface_caps) );

            anvil_assert(surface_caps.maxImageCount == 0                    ||
                         surface_caps.maxImageCount >= m_n_swapchain_images);
        }
        #endif

        {
            VkSwapchainCreateInfoKHR create_info;

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

            struct_chainer.append_struct(create_info);
        }

        struct_chain_ptr = struct_chainer.create_chain();

        m_parent_surface_ptr->lock();
        {
            result = m_khr_swapchain_entrypoints.vkCreateSwapchainKHR(m_device_ptr->get_device_vk(),
                                                                      struct_chain_ptr->get_root_struct(),
                                                                      nullptr, /* pAllocator */
                                                                     &m_swapchain);
        }
        m_parent_surface_ptr->unlock();

        anvil_assert_vk_call_succeeded(result);
        if (is_vk_call_successful(result) )
        {
            set_vk_handle(m_swapchain);
        }

        /* Retrieve swap-chain images */
        result = m_khr_swapchain_entrypoints.vkGetSwapchainImagesKHR(m_device_ptr->get_device_vk(),
                                                                     m_swapchain,
                                                                    &n_swapchain_images,
                                                                     nullptr); /* pSwapchainImages */

        anvil_assert_vk_call_succeeded(result);
        anvil_assert                  (n_swapchain_images >  0);

        swapchain_images.resize(n_swapchain_images);

        result = m_khr_swapchain_entrypoints.vkGetSwapchainImagesKHR(m_device_ptr->get_device_vk(),
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
                                                                          *struct_chain_ptr->get_root_struct(),
                                                                          swapchain_images[n_result_image],
                                                                          Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ) );
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
                                                                          nullptr,
                                                                          Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ) );
        }

        /* For each swap-chain image, create a relevant view */
        m_image_view_ptrs[n_result_image] = Anvil::ImageView::create_2D(m_device_ptr,
                                                                         m_image_ptrs[n_result_image].get(),
                                                                         0, /* n_base_layer */
                                                                         0, /* n_base_mipmap_level */
                                                                         1, /* n_mipmaps           */
                                                                         VK_IMAGE_ASPECT_COLOR_BIT,
                                                                         m_image_format,
                                                                         VK_COMPONENT_SWIZZLE_R,
                                                                         VK_COMPONENT_SWIZZLE_G,
                                                                         VK_COMPONENT_SWIZZLE_B,
                                                                         VK_COMPONENT_SWIZZLE_A,
                                                                         Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ) );
    }

    /* Sign up for present submission notifications. This is needed to ensure that number of presented frames ==
     * number of acquired frames at destruction time.
     */
    {
        std::vector<Anvil::Queue*> queues;

        const std::vector<uint32_t>* queue_fams_with_present_support_ptr(nullptr);
        const Anvil::SGPUDevice*     sgpu_device_ptr                    (dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

        m_parent_surface_ptr->get_queue_families_with_present_support(&queue_fams_with_present_support_ptr);

        if (queue_fams_with_present_support_ptr == nullptr)
        {
            anvil_assert(queue_fams_with_present_support_ptr != nullptr);
        }
        else
        {
            for (const auto queue_fam : *queue_fams_with_present_support_ptr)
            {
                const uint32_t n_queues = sgpu_device_ptr->get_n_queues(queue_fam);

                for (uint32_t n_queue = 0;
                              n_queue < n_queues;
                            ++n_queue)
                {
                    auto queue_ptr = sgpu_device_ptr->get_queue_for_queue_family_index(queue_fam,
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
            queue_ptr->register_for_callbacks(
                QUEUE_CALLBACK_ID_PRESENT_REQUEST_ISSUED,
                std::bind(&Swapchain::on_present_request_issued,
                          this,
                          std::placeholders::_1),
                this
            );

            m_observed_queues.push_back(queue_ptr);
        }
    }

    /* Sign up for "about to close the parent window" notifications. Swapchain instance SHOULD be deinitialized
     * before the window is destroyed, so we're going to act as nice citizens.
     */
    m_window_ptr->register_for_callbacks(
        WINDOW_CALLBACK_ID_ABOUT_TO_CLOSE,
        std::bind(&Swapchain::on_parent_window_about_to_close,
                  this),
        this
    );
}

/** TODO */
void Anvil::Swapchain::on_parent_window_about_to_close()
{
    if (m_destroy_swapchain_before_parent_window_closes)
    {
        destroy_swapchain();
    }
}

/** TODO */
void Anvil::Swapchain::on_present_request_issued(Anvil::CallbackArgument* in_callback_raw_ptr)
{
    auto* callback_arg_ptr = dynamic_cast<Anvil::OnPresentRequestIssuedCallbackArgument*>(in_callback_raw_ptr);

    anvil_assert(callback_arg_ptr != nullptr);
    if (callback_arg_ptr != nullptr)
    {
        if (callback_arg_ptr->swapchain_ptr == this)
        {
            m_n_present_counter++;
        }
    }
}