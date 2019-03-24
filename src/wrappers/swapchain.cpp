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
                                                     Anvil::ObjectType::SWAPCHAIN),
     MTSafetySupportProvider                        (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                                     in_create_info_ptr->get_device   () )),
     m_destroy_swapchain_before_parent_window_closes(true),
     m_last_acquired_image_index                    (UINT32_MAX),
     m_n_acquire_counter                            (0),
     m_n_acquire_counter_rounded                    (0),
     m_n_present_counter                            (0),
     m_swapchain                                    (VK_NULL_HANDLE)
{
    {
        auto create_info_ptr = Anvil::FenceCreateInfo::create(m_device_ptr,
                                                              false);  /* create_signalled */

        create_info_ptr->set_mt_safety(in_create_info_ptr->get_mt_safety() );

        m_image_available_fence_ptr = Anvil::Fence::create(std::move(create_info_ptr) );
    }

    m_create_info_ptr = std::move(in_create_info_ptr);

    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::SWAPCHAIN,
                                                  this);
}

/** Please see header for specification */
Anvil::Swapchain::~Swapchain()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::SWAPCHAIN,
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
Anvil::SwapchainOperationErrorCode Anvil::Swapchain::acquire_image(Anvil::Semaphore* in_opt_semaphore_ptr,
                                                                   uint32_t*         out_result_index_ptr,
                                                                   bool              in_should_block)
{
    uint32_t                           result        = UINT32_MAX;
    Anvil::SwapchainOperationErrorCode result_status = Anvil::SwapchainOperationErrorCode::SUCCESS;

    switch (m_device_ptr->get_type() )
    {
        case Anvil::DeviceType::MULTI_GPU:
        {
            const Anvil::MGPUDevice*     mgpu_device_ptr   (dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );
            const uint32_t               n_physical_devices(mgpu_device_ptr->get_n_physical_devices() );
            const Anvil::PhysicalDevice* physical_devices[32];
            
            anvil_assert(n_physical_devices < sizeof(physical_devices) / sizeof(physical_devices[0]) );

            for (uint32_t n_physical_device = 0;
                          n_physical_device < n_physical_devices;
                        ++n_physical_device)
            {
                physical_devices[n_physical_device] = mgpu_device_ptr->get_physical_device(n_physical_device);
            }

            result_status = acquire_image(in_opt_semaphore_ptr,
                                          n_physical_devices,
                                          physical_devices,
                                         &result,
                                          in_should_block);

            break;
        }

        case Anvil::DeviceType::SINGLE_GPU:
        {
            const Anvil::PhysicalDevice* physical_device_ptr(nullptr);
            const Anvil::SGPUDevice*     sgpu_device_ptr    (dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

            physical_device_ptr = sgpu_device_ptr->get_physical_device();

            result_status = acquire_image(in_opt_semaphore_ptr,
                                          1, /* n_mgpu_physical_devices */
                                         &physical_device_ptr,
                                         &result,
                                          in_should_block);

            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }

    *out_result_index_ptr = result;

    return result_status;
}

/** Please see header for specification */
Anvil::SwapchainOperationErrorCode Anvil::Swapchain::acquire_image(Anvil::Semaphore*                   in_opt_semaphore_ptr,
                                                                   uint32_t                            in_n_mgpu_physical_devices,
                                                                   const Anvil::PhysicalDevice* const* in_mgpu_physical_device_ptrs,
                                                                   uint32_t*                           out_result_index_ptr,
                                                                   bool                                in_should_block)
{
    const Anvil::DeviceType device_type                    = m_device_ptr->get_type();

    uint32_t                           result                         = UINT32_MAX;
    Anvil::SwapchainOperationErrorCode result_status                  = Anvil::SwapchainOperationErrorCode::SUCCESS;
    const WindowPlatform               window_platform                = m_create_info_ptr->get_window()->get_platform();
    const bool                         is_offscreen_rendering_enabled = (window_platform   == WINDOW_PLATFORM_DUMMY                     ||
                                                                         window_platform   == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS);

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

            if (device_type == Anvil::DeviceType::SINGLE_GPU)
            {
                const auto& khr_swapchain_entrypoints = m_device_ptr->get_extension_khr_swapchain_entrypoints();

                result_status = static_cast<Anvil::SwapchainOperationErrorCode>(khr_swapchain_entrypoints.vkAcquireNextImageKHR(m_device_ptr->get_device_vk(),
                                                                                                                                m_swapchain,
                                                                                                                                UINT64_MAX,
                                                                                                                                (in_opt_semaphore_ptr != nullptr) ? in_opt_semaphore_ptr->get_semaphore() : VK_NULL_HANDLE,
                                                                                                                                fence_handle,
                                                                                                                               &result) );
            }
            else
            {
                VkAcquireNextImageInfoKHR info;
                const auto&               khr_device_group_entrypoints(m_device_ptr->get_extension_khr_device_group_entrypoints() );
                const Anvil::MGPUDevice*  mgpu_device_ptr             (dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );

                anvil_assert(device_type == Anvil::DeviceType::MULTI_GPU);

                info.deviceMask = 0;
                info.fence      = fence_handle;
                info.pNext      = nullptr;
                info.semaphore  = (in_opt_semaphore_ptr != nullptr) ? in_opt_semaphore_ptr->get_semaphore() : VK_NULL_HANDLE;
                info.sType      = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
                info.swapchain  = m_swapchain;
                info.timeout    = UINT64_MAX;

                for (uint32_t n_physical_device = 0;
                              n_physical_device < in_n_mgpu_physical_devices;
                            ++n_physical_device)
                {
                    const Anvil::PhysicalDevice* physical_device_ptr  (in_mgpu_physical_device_ptrs[n_physical_device]);
                    const uint32_t               physical_device_index(physical_device_ptr->get_index() );

                    anvil_assert((info.deviceMask & (1 << physical_device_index)) == 0);

                    info.deviceMask |= (1 << physical_device_index);
                }

                result_status = static_cast<Anvil::SwapchainOperationErrorCode>(khr_device_group_entrypoints.vkAcquireNextImage2KHR(mgpu_device_ptr->get_device_vk(),
                                                                                                                                   &info,
                                                                                                                                   &result) );
            }

            if (fence_handle != VK_NULL_HANDLE)
            {
                result_status = static_cast<Anvil::SwapchainOperationErrorCode>(Anvil::Vulkan::vkWaitForFences(m_device_ptr->get_device_vk(),
                                                                                1, /* fenceCount */
                                                                               &fence_handle,
                                                                                VK_TRUE, /* waitAll */
                                                                                UINT64_MAX) );
            }
        }
        unlock();
        m_image_available_fence_ptr->unlock();

        if (in_opt_semaphore_ptr != nullptr)
        {
            in_opt_semaphore_ptr->unlock();
        }
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
    *out_result_index_ptr       = result;

    return result_status;
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
    anvil_assert(in_n_swapchain_image < m_n_images );

    return m_image_ptrs.at(in_n_swapchain_image).get();
}

/** Please see header for specification */
Anvil::ImageView* Anvil::Swapchain::get_image_view(uint32_t in_n_swapchain_image) const
{
    anvil_assert(in_n_swapchain_image < m_n_images );

    return m_image_view_ptrs.at(in_n_swapchain_image).get();
}

/** Initializes the swapchain object. */
bool Anvil::Swapchain::init()
{
    auto                                                  parent_surface_ptr             = m_create_info_ptr->get_rendering_surface();
    VkResult                                              result                         = VK_ERROR_INITIALIZATION_FAILED;
    Anvil::StructChainUniquePtr<VkSwapchainCreateInfoKHR> struct_chain_ptr;
    std::vector<VkImage>                                  swapchain_images;
    const Anvil::SurfaceTransformFlagBits                 swapchain_transformation       = Anvil::SurfaceTransformFlagBits::IDENTITY_BIT_KHR;
    const WindowPlatform                                  window_platform                = m_create_info_ptr->get_window()->get_platform();
    const bool                                            is_offscreen_rendering_enabled = (window_platform   == WINDOW_PLATFORM_DUMMY                     ||
                                                                                            window_platform   == WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS);

    parent_surface_ptr->update_surface_extents();

    m_n_images    = 0;
    m_size.width  = parent_surface_ptr->get_width ();
    m_size.height = parent_surface_ptr->get_height();

    /* not doing offscreen rendering */
    if (!is_offscreen_rendering_enabled)
    {
        Anvil::CompositeAlphaFlagBits                  composite_alpha           = Anvil::CompositeAlphaFlagBits::NONE;
        const auto&                                    khr_swapchain_entrypoints = m_device_ptr->get_extension_khr_swapchain_entrypoints();
        Anvil::StructChainer<VkSwapchainCreateInfoKHR> struct_chainer;

        {
            const Anvil::MGPUDevice* mgpu_device_ptr(dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );
            const Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );

            const Anvil::DeviceType      device_type                     = m_device_ptr->get_type();
            uint32_t                     n_physical_devices              = 0;
            bool                         result_bool                     = false;
            const char*                  required_surface_extension_name = nullptr;
            Anvil::SurfaceCapabilities   surface_caps;
            Anvil::CompositeAlphaFlags   supported_composite_alpha_flags;
            Anvil::SurfaceTransformFlags supported_surface_transform_flags;

            ANVIL_REDUNDANT_VARIABLE(required_surface_extension_name);
            ANVIL_REDUNDANT_VARIABLE(result_bool);

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
                case Anvil::DeviceType::MULTI_GPU:  n_physical_devices = mgpu_device_ptr->get_n_physical_devices(); break;
                case Anvil::DeviceType::SINGLE_GPU: n_physical_devices = 1;                                         break;

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

                ANVIL_REDUNDANT_VARIABLE(current_physical_device_ptr);

                switch (device_type)
                {
                    case Anvil::DeviceType::MULTI_GPU:  current_physical_device_ptr = mgpu_device_ptr->get_physical_device(n_physical_device); break;
                    case Anvil::DeviceType::SINGLE_GPU: current_physical_device_ptr = sgpu_device_ptr->get_physical_device();                  break;

                    default:
                    {
                        anvil_assert_fail();
                    }
                }

                /* Ensure opaque composite alpha mode is supported */
                anvil_assert(parent_surface_ptr->get_supported_composite_alpha_flags(current_physical_device_ptr,
                                                                                    &supported_composite_alpha_flags) );

                {
                    composite_alpha = Anvil::CompositeAlphaFlagBits::OPAQUE_BIT_KHR;

                    anvil_assert((supported_composite_alpha_flags & composite_alpha) != 0);
                }

                /* Ensure we can use the swapchain image format  */
                anvil_assert(parent_surface_ptr->is_compatible_with_image_format(current_physical_device_ptr,
                                                                                 m_create_info_ptr->get_format(),
                                                                                &result_bool) );
                anvil_assert(result_bool);

                /* Ensure the transformation we're about to request is supported by the rendering surface */
                anvil_assert(parent_surface_ptr->get_supported_transformations(current_physical_device_ptr,
                                                                              &supported_surface_transform_flags) );

                anvil_assert((supported_surface_transform_flags & swapchain_transformation) != 0);

                /* Ensure the requested number of swapchain images is reasonable*/
                anvil_assert(parent_surface_ptr->get_capabilities(current_physical_device_ptr,
                                                                 &surface_caps) );

                anvil_assert(surface_caps.max_image_count == 0                                 ||
                             surface_caps.max_image_count >= m_create_info_ptr->get_n_images() );
            }
        }

        {
            VkSwapchainCreateInfoKHR create_info;
            const auto&              old_swapchain_ptr = m_create_info_ptr->get_old_swapchain();

            create_info.clipped               = m_create_info_ptr->get_clipped();
            create_info.compositeAlpha        = static_cast<VkCompositeAlphaFlagBitsKHR>(composite_alpha);
            create_info.flags                 = m_create_info_ptr->get_flags().get_vk();
            create_info.imageArrayLayers      = 1;
            create_info.imageColorSpace       = static_cast<VkColorSpaceKHR>  (m_create_info_ptr->get_color_space() );
            create_info.imageExtent.height    = parent_surface_ptr->get_height();
            create_info.imageExtent.width     = parent_surface_ptr->get_width ();
            create_info.imageFormat           = static_cast<VkFormat>         (m_create_info_ptr->get_format() );
            create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            create_info.imageUsage            = m_create_info_ptr->get_usage_flags().get_vk();
            create_info.minImageCount         = m_create_info_ptr->get_n_images   ();
            create_info.oldSwapchain          = (old_swapchain_ptr != nullptr) ? old_swapchain_ptr->get_swapchain_vk()
                                                                               : VK_NULL_HANDLE;
            create_info.pNext                 = nullptr;
            create_info.pQueueFamilyIndices   = nullptr;
            create_info.presentMode           = static_cast<VkPresentModeKHR>             (m_create_info_ptr->get_present_mode() );
            create_info.preTransform          = static_cast<VkSurfaceTransformFlagBitsKHR>(swapchain_transformation);
            create_info.queueFamilyIndexCount = 0;
            create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            create_info.surface               = parent_surface_ptr->get_surface();

            struct_chainer.append_struct(create_info);
        }

        /* If present mode flags specific to mGPU support (exposed via VK_KHR_device_group) have been requested,
         * make sure to chain this information */
        const auto mgpu_present_mode_flags = m_create_info_ptr->get_mgpu_present_mode_flags();

        if (mgpu_present_mode_flags != Anvil::DeviceGroupPresentModeFlagBits::LOCAL_BIT_KHR)
        {
            VkDeviceGroupSwapchainCreateInfoKHR mgpu_create_info;

            mgpu_create_info.modes = mgpu_present_mode_flags.get_vk();
            mgpu_create_info.pNext = nullptr;
            mgpu_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_SWAPCHAIN_CREATE_INFO_KHR;

            struct_chainer.append_struct(mgpu_create_info);
        }

        if ((m_create_info_ptr->get_flags() & Anvil::SwapchainCreateFlagBits::CREATE_MUTABLE_FORMAT_BIT) != 0)
        {
            const Anvil::Format*           image_formats_ptr             = nullptr;
            VkImageFormatListCreateInfoKHR image_format_list_create_info;
            uint32_t                       n_image_formats               = 0;

            anvil_assert(m_device_ptr->get_extension_info()->khr_swapchain_mutable_format() );

            m_create_info_ptr->get_view_format_list(&image_formats_ptr,
                                                    &n_image_formats);

            anvil_assert(image_formats_ptr != nullptr);
            anvil_assert(n_image_formats   >  0);

            image_format_list_create_info.pNext           = nullptr;
            image_format_list_create_info.pViewFormats    = reinterpret_cast<const VkFormat*>(image_formats_ptr);
            image_format_list_create_info.sType           = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
            image_format_list_create_info.viewFormatCount = n_image_formats;

            struct_chainer.append_struct(image_format_list_create_info);
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
                                                                  &m_n_images,
                                                                   nullptr); /* pSwapchainImages */

        anvil_assert_vk_call_succeeded(result);
        anvil_assert                  (m_n_images > 0);

        swapchain_images.resize(m_n_images);

        result = khr_swapchain_entrypoints.vkGetSwapchainImagesKHR(m_device_ptr->get_device_vk(),
                                                                   m_swapchain,
                                                                  &m_n_images,
                                                                  &swapchain_images[0]);

        anvil_assert_vk_call_succeeded(result);
    }
    else /* offscreen rendering */
    {
        m_create_info_ptr->set_usage_flags(m_create_info_ptr->get_usage_flags() | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT);

        m_n_images = m_create_info_ptr->get_n_images();
    }

    /* Adjust capacity of m_image_ptrs and m_image_view_ptrs to the number of swapchain images actually created. */
    m_image_ptrs.resize     (m_n_images);
    m_image_view_ptrs.resize(m_n_images);

    for (uint32_t n_result_image = 0;
                  n_result_image < m_n_images;
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
            auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(m_device_ptr,
                                                                        Anvil::ImageType::_2D,
                                                                        m_create_info_ptr->get_format(),
                                                                        Anvil::ImageTiling::OPTIMAL,
                                                                        m_create_info_ptr->get_usage_flags(),
                                                                        m_size.width,
                                                                        m_size.height,
                                                                        1, /* base_mipmap_depth */
                                                                        1,
                                                                        Anvil::SampleCountFlagBits::_1_BIT,
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        false, /* in_use_full_mipmap_chain */
                                                                        Anvil::MemoryFeatureFlagBits::NONE,
                                                                        Anvil::ImageCreateFlagBits::NONE,
                                                                        Anvil::ImageLayout::GENERAL,
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
                                                                         Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                         m_create_info_ptr->get_format(),
                                                                         Anvil::ComponentSwizzle::R,
                                                                         Anvil::ComponentSwizzle::G,
                                                                         Anvil::ComponentSwizzle::B,
                                                                         Anvil::ComponentSwizzle::A);

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
            case Anvil::DeviceType::MULTI_GPU:
            {
                const Anvil::MGPUDevice* mgpu_device_ptr   (dynamic_cast<const Anvil::MGPUDevice*>(m_device_ptr) );
                const uint32_t           n_physical_devices(mgpu_device_ptr->get_n_physical_devices() );

                for (uint32_t n_physical_device = 0;
                              n_physical_device < n_physical_devices;
                            ++n_physical_device)
                {
                    auto                         physical_device_ptr                (mgpu_device_ptr->get_physical_device(n_physical_device) );
                    const std::vector<uint32_t>* queue_fams_with_present_support_ptr(nullptr);
                    auto                         rendering_surface_ptr              (m_create_info_ptr->get_rendering_surface() );

                    if (!rendering_surface_ptr->get_queue_families_with_present_support(physical_device_ptr,
                                                                                       &queue_fams_with_present_support_ptr) )
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
                            const uint32_t n_queues = mgpu_device_ptr->get_n_queues(queue_fam);

                            for (uint32_t n_queue = 0;
                                          n_queue < n_queues;
                                        ++n_queue)
                            {
                                auto queue_ptr = mgpu_device_ptr->get_queue_for_queue_family_index(queue_fam,
                                                                                                   n_queue);

                                if (std::find(queues.begin(),
                                              queues.end(),
                                              queue_ptr) == queues.end() )
                                {
                                    queues.push_back(queue_ptr);
                                }
                            }
                        }
                    }
                }

                break;
            }

            case Anvil::DeviceType::SINGLE_GPU:
            {
                const std::vector<uint32_t>* queue_fams_with_present_support_ptr(nullptr);
                const auto                   rendering_surface_ptr              (m_create_info_ptr->get_rendering_surface() );
                const Anvil::SGPUDevice*     sgpu_device_ptr                    (dynamic_cast<const Anvil::SGPUDevice*>(m_device_ptr) );
                auto                         physical_device_ptr                (sgpu_device_ptr->get_physical_device() );

                if (!rendering_surface_ptr->get_queue_families_with_present_support(physical_device_ptr,
                                                                                   &queue_fams_with_present_support_ptr) )
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

/** Please see header for specification */
void Anvil::Swapchain::set_hdr_metadata(const uint32_t&              in_n_swapchains,
                                        Anvil::Swapchain**           in_swapchains_ptr_ptr,
                                        const Anvil::HdrMetadataEXT* in_metadata_items_ptr)
{
    anvil_assert(in_n_swapchains > 0);

    auto        device_ptr       = in_swapchains_ptr_ptr[0]->get_create_info_ptr()->get_device();
    const auto& entrypoints      = device_ptr->get_extension_ext_hdr_metadata_entrypoints     ();
    auto        metadata_vk_vec  = std::vector<VkHdrMetadataEXT>(in_n_swapchains);
    auto        swapchain_vk_vec = std::vector<VkSwapchainKHR>  (in_n_swapchains);

    for (uint32_t n_swapchain = 0;
                  n_swapchain < in_n_swapchains;
                ++n_swapchain)
    {
        metadata_vk_vec.at (n_swapchain) = in_metadata_items_ptr[n_swapchain].get_vk           ();
        swapchain_vk_vec.at(n_swapchain) = in_swapchains_ptr_ptr[n_swapchain]->get_swapchain_vk();

        #if defined(_DEBUG)
        {
            anvil_assert(in_swapchains_ptr_ptr[n_swapchain]->get_create_info_ptr()->get_device() == device_ptr);
        }
        #endif
    }

    entrypoints.vkSetHdrMetadataEXT(device_ptr->get_device_vk(),
                                    in_n_swapchains,
                                   &swapchain_vk_vec.at(0),
                                   &metadata_vk_vec.at (0) );
}