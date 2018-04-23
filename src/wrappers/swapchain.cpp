//
// Copyright (c) 2017-2018 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/fence_create_info.h"
#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/object_tracker.h"
#include "misc/struct_chainer.h"
#include "misc/swapchain_create_info.h"
#include "misc/window.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/instance.h"
#include "wrappers/semaphore.h"
#include "wrappers/swapchain.h"

/** Please see header for specification */
Anvil::Swapchain::Swapchain(Anvil::SwapchainCreateInfoUniquePtr in_create_info_ptr)
    :DebugMarkerSupportProvider                     (in_create_info_ptr->get_device(),
                                                     VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT),
     MTSafetySupportProvider                        (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                                     in_create_info_ptr->get_device   () )),
     m_destroy_swapchain_before_parent_window_closes(true),
     m_last_acquired_image_index                    (UINT32_MAX),
     m_n_acquire_counter                            (0),
     m_n_acquire_counter_rounded                    (0),
     m_n_present_counter                            (0),
     m_swapchain                                    (VK_NULL_HANDLE)
{
    const auto n_images = in_create_info_ptr->get_n_images();

    m_image_ptrs.resize     (n_images);
    m_image_view_ptrs.resize(n_images);

    {
        auto create_info_ptr = Anvil::FenceCreateInfo::create(m_device_ptr,
                                                              false);  /* create_signalled */

        create_info_ptr->set_mt_safety(in_create_info_ptr->get_mt_safety() );

        m_image_available_fence_ptr = Anvil::Fence::create(std::move(create_info_ptr) );
    }

    m_create_info_ptr = std::move(in_create_info_ptr);

    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_SWAPCHAIN,
                                                  this);
}

/** Please see header for specification */
Anvil::Swapchain::~Swapchain()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_SWAPCHAIN,
                                                    this);

    destroy_swapchain();

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

    m_create_info_ptr->get_window()->unregister_from_callbacks(
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
    uint32_t             result                        (UINT32_MAX);
    VkResult             result_vk                     (VK_ERROR_INITIALIZATION_FAILED);
    const WindowPlatform window_platform               (m_create_info_ptr->get_window()->get_platform() );
    const bool           is_offscreen_rendering_enabled( (window_platform   == WINDOW_PLATFORM_DUMMY                     ||
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
            const auto& khr_swapchain_entrypoints = m_device_ptr->get_extension_khr_swapchain_entrypoints();

            if (in_should_block)
            {
                m_image_available_fence_ptr->reset();

                fence_handle = m_image_available_fence_ptr->get_fence();
            }

            result_vk = khr_swapchain_entrypoints.vkAcquireNextImageKHR(m_device_ptr->get_device_vk(),
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
            m_device_ptr->get_universal_queue(0)->submit(
                Anvil::SubmitInfo::create_signal(1,       /* n_semaphores_to_signal */
                                                &in_opt_semaphore_ptr)
            );
        }

        result = m_n_acquire_counter_rounded;
    }

    m_n_acquire_counter++;
    m_n_acquire_counter_rounded = (m_n_acquire_counter_rounded + 1) % m_create_info_ptr->get_n_images();

    m_last_acquired_image_index = result;

    return result;
}

/** Please see header for specification */
Anvil::SwapchainUniquePtr Anvil::Swapchain::create(Anvil::SwapchainCreateInfoUniquePtr in_create_info_ptr)
{
    SwapchainUniquePtr result_ptr(nullptr,
                                  std::default_delete<Swapchain>() );
    auto               window_ptr(in_create_info_ptr->get_window() );

    result_ptr.reset(
        new Anvil::Swapchain(
            std::move(in_create_info_ptr)
        )
    );

    if (result_ptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();

            goto end;
        }

        if (window_ptr                 != nullptr                                  &&
            window_ptr->get_platform() == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS)
        {
            dynamic_cast<Anvil::DummyWindowWithPNGSnapshots*>(window_ptr)->set_swapchain(result_ptr.get() );
        }
    }

end:
    return result_ptr;
}

/** TODO */
void Anvil::Swapchain::destroy_swapchain()
{
    /* If this assertion failure explodes, your application attempted to release a swapchain without presenting all acquired swapchain images.
     * That's illegal per Vulkan spec.
     */
    lock();
    {
        if (m_swapchain != VK_NULL_HANDLE)
        {
            const auto& khr_swapchain_entrypoints = m_device_ptr->get_extension_khr_swapchain_entrypoints();

            anvil_assert(m_n_acquire_counter == m_n_present_counter);

            khr_swapchain_entrypoints.vkDestroySwapchainKHR(m_device_ptr->get_device_vk(),
                                                            m_swapchain,
                                                            nullptr /* pAllocator */);
        }

        m_swapchain = VK_NULL_HANDLE;
    }
    unlock();
}

/** Please see header for specification */
Anvil::Image* Anvil::Swapchain::get_image(uint32_t in_n_swapchain_image) const
{
    anvil_assert(in_n_swapchain_image < m_create_info_ptr->get_n_images() );

    return m_image_ptrs.at(in_n_swapchain_image).get();
}

/** Please see header for specification */
Anvil::ImageView* Anvil::Swapchain::get_image_view(uint32_t in_n_swapchain_image) const
{
    anvil_assert(in_n_swapchain_image < m_create_info_ptr->get_n_images());

    return m_image_view_ptrs.at(in_n_swapchain_image).get();
}

/** Initializes the swapchain object. */
bool Anvil::Swapchain::init()
{
    uint32_t                                              n_swapchain_images             = 0;
    auto                                                  parent_surface_ptr             = m_create_info_ptr->get_rendering_surface();
    VkResult                                              result                         = VK_ERROR_INITIALIZATION_FAILED;
    Anvil::StructChainUniquePtr<VkSwapchainCreateInfoKHR> struct_chain_ptr;
    std::vector<VkImage>                                  swapchain_images;
    const VkSurfaceTransformFlagBitsKHR                   swapchain_transformation       = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    const WindowPlatform                                  window_platform                = m_create_info_ptr->get_window()->get_platform();
    const bool                                            is_offscreen_rendering_enabled = (window_platform   == WINDOW_PLATFORM_DUMMY                     ||
                                                                                            window_platform   == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS);

    m_size.width  = parent_surface_ptr->get_width ();
    m_size.height = parent_surface_ptr->get_height();

    /* not doing offscreen rendering */
    if (!is_offscreen_rendering_enabled)
    {
        const auto&                                    khr_swapchain_entrypoints = m_device_ptr->get_extension_khr_swapchain_entrypoints();
        Anvil::StructChainer<VkSwapchainCreateInfoKHR> struct_chainer;

        #ifdef _DEBUG
        {
            const Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

            const Anvil::DeviceType    device_type                     = m_device_ptr->get_type();
            uint32_t                   n_physical_devices              = 0;
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

            switch (device_type)
            {
                case Anvil::DEVICE_TYPE_SINGLE_GPU: n_physical_devices = 1; break;

                default:
                {
                    anvil_assert_fail();
                }
            }

            for (uint32_t n_physical_device = 0;
                          n_physical_device < n_physical_devices;
                        ++n_physical_device)
            {
                const Anvil::PhysicalDevice* current_physical_device_ptr = nullptr;

                switch (device_type)
                {
                    case Anvil::DEVICE_TYPE_SINGLE_GPU: current_physical_device_ptr = sgpu_device_ptr->get_physical_device(); break;

                    default:
                    {
                        anvil_assert_fail();
                    }
                }

                /* Ensure opaque composite alpha mode is supported */
                anvil_assert(parent_surface_ptr->get_supported_composite_alpha_flags(&supported_composite_alpha_flags) );

                anvil_assert(supported_composite_alpha_flags & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

                /* Ensure we can use the swapchain image format  */
                anvil_assert(parent_surface_ptr->is_compatible_with_image_format(m_create_info_ptr->get_format(),
                                                                                &result_bool) );
                anvil_assert(result_bool);

                /* Ensure the transformation we're about to request is supported by the rendering surface */
                anvil_assert(parent_surface_ptr->get_supported_transformations(&supported_surface_transform_flags) );

                anvil_assert(supported_surface_transform_flags & swapchain_transformation);

                /* Ensure the requested number of swapchain images is reasonable*/
                anvil_assert(parent_surface_ptr->get_capabilities(&surface_caps) );

                anvil_assert(surface_caps.maxImageCount == 0                                 ||
                             surface_caps.maxImageCount >= m_create_info_ptr->get_n_images() );
            }
        }
        #endif

        {
            VkSwapchainCreateInfoKHR create_info;

            create_info.clipped               = true; /* we won't be reading from the presentable images */
            create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            create_info.flags                 = m_create_info_ptr->get_flags();
            create_info.imageArrayLayers      = 1;
            create_info.imageColorSpace       = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            create_info.imageExtent.height    = parent_surface_ptr->get_height();
            create_info.imageExtent.width     = parent_surface_ptr->get_width ();
            create_info.imageFormat           = m_create_info_ptr->get_format ();
            create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            create_info.imageUsage            = m_create_info_ptr->get_usage_flags();
            create_info.minImageCount         = m_create_info_ptr->get_n_images   ();
            create_info.oldSwapchain          = VK_NULL_HANDLE;
            create_info.pNext                 = nullptr;
            create_info.pQueueFamilyIndices   = nullptr;
            create_info.presentMode           = m_create_info_ptr->get_present_mode();
            create_info.preTransform          = swapchain_transformation;
            create_info.queueFamilyIndexCount = 0;
            create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            create_info.surface               = parent_surface_ptr->get_surface();

            struct_chainer.append_struct(create_info);
        }

        struct_chain_ptr = struct_chainer.create_chain();

        parent_surface_ptr->lock();
        {
            result = khr_swapchain_entrypoints.vkCreateSwapchainKHR(m_device_ptr->get_device_vk(),
                                                                    struct_chain_ptr->get_root_struct(),
                                                                    nullptr, /* pAllocator */
                                                                   &m_swapchain);
        }
        parent_surface_ptr->unlock();

        anvil_assert_vk_call_succeeded(result);
        if (is_vk_call_successful(result) )
        {
            set_vk_handle(m_swapchain);
        }

        /* Retrieve swap-chain images */
        result = khr_swapchain_entrypoints.vkGetSwapchainImagesKHR(m_device_ptr->get_device_vk(),
                                                                   m_swapchain,
                                                                  &n_swapchain_images,
                                                                   nullptr); /* pSwapchainImages */

        anvil_assert_vk_call_succeeded(result);
        anvil_assert                  (n_swapchain_images >  0);

        swapchain_images.resize(n_swapchain_images);

        result = khr_swapchain_entrypoints.vkGetSwapchainImagesKHR(m_device_ptr->get_device_vk(),
                                                                   m_swapchain,
                                                                  &n_swapchain_images,
                                                                  &swapchain_images[0]);

        anvil_assert_vk_call_succeeded(result);
    }
    else /* offscreen rendering */
    {
        m_create_info_ptr->set_usage_flags(m_create_info_ptr->get_usage_flags() | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        n_swapchain_images = m_create_info_ptr->get_n_images();
    }

    for (uint32_t n_result_image = 0;
                  n_result_image < n_swapchain_images;
                ++n_result_image)
    {
        /* Spawn an Image wrapper class for the swap-chain image. */
        if (!is_offscreen_rendering_enabled)
        {
            auto create_info_ptr = Anvil::ImageCreateInfo::create_swapchain_wrapper(m_device_ptr,
                                                                                    this,
                                                                                    swapchain_images[n_result_image],
                                                                                    n_result_image);

            create_info_ptr->set_mt_safety(Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ) );

            m_image_ptrs[n_result_image] = Anvil::Image::create(std::move(create_info_ptr) );
        }
        else
        {
            auto create_info_ptr = Anvil::ImageCreateInfo::create_nonsparse_alloc(m_device_ptr,
                                                                                  VK_IMAGE_TYPE_2D,
                                                                                  m_create_info_ptr->get_format(),
                                                                                  VK_IMAGE_TILING_OPTIMAL,
                                                                                  m_create_info_ptr->get_usage_flags(),
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

            create_info_ptr->set_mt_safety(Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ) );

            m_image_ptrs[n_result_image] = Anvil::Image::create(std::move(create_info_ptr) );
        }

        /* For each swap-chain image, create a relevant view */
        {
            auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(m_device_ptr,
                                                                         m_image_ptrs[n_result_image].get(),
                                                                         0, /* n_base_layer */
                                                                         0, /* n_base_mipmap_level */
                                                                         1, /* n_mipmaps           */
                                                                         VK_IMAGE_ASPECT_COLOR_BIT,
                                                                         m_create_info_ptr->get_format(),
                                                                         VK_COMPONENT_SWIZZLE_R,
                                                                         VK_COMPONENT_SWIZZLE_G,
                                                                         VK_COMPONENT_SWIZZLE_B,
                                                                         VK_COMPONENT_SWIZZLE_A);

            create_info_ptr->set_mt_safety(Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe() ) );

            m_image_view_ptrs[n_result_image] = Anvil::ImageView::create(std::move(create_info_ptr) );
        }

        result = VK_SUCCESS;
    }

    /* Sign up for present submission notifications. This is needed to ensure that number of presented frames ==
     * number of acquired frames at destruction time.
     */
    {
        std::vector<Anvil::Queue*> queues;

        switch (m_device_ptr->get_type() )
        {
            case Anvil::DEVICE_TYPE_SINGLE_GPU:
            {
                const std::vector<uint32_t>* queue_fams_with_present_support_ptr(nullptr);
                const auto                   rendering_surface_ptr              (m_create_info_ptr->get_rendering_surface() );
                const Anvil::SGPUDevice*     sgpu_device_ptr                    (dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

                if (!rendering_surface_ptr->get_queue_families_with_present_support(&queue_fams_with_present_support_ptr) )
                {
                    break;
                }

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

                break;
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
    m_create_info_ptr->get_window()->register_for_callbacks(
        WINDOW_CALLBACK_ID_ABOUT_TO_CLOSE,
        std::bind(&Swapchain::on_parent_window_about_to_close,
                  this),
        this
    );

    return is_vk_call_successful(result);
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