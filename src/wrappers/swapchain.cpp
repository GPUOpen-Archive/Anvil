//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/object_tracker.h"
#include "misc/window.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/semaphore.h"
#include "wrappers/swapchain.h"

/** Please see header for specification */
Anvil::Swapchain::Swapchain(Anvil::Device*              device_ptr,
                            Anvil::RenderingSurface*    parent_surface_ptr,
                            Anvil::Window*              window_ptr,
                            VkFormat                    format,
                            VkPresentModeKHR            present_mode,
                            VkImageUsageFlags           usage_flags,
                            uint32_t                    n_images,
                            PFN_vkAcquireNextImageKHR   pfn_acquire_next_image_proc,
                            PFN_vkCreateSwapchainKHR    pfn_create_swapchain_proc,
                            PFN_vkDestroySwapchainKHR   pfn_destroy_swapchain_proc,
                            PFN_vkGetSwapchainImagesKHR pfn_get_swapchain_images_proc)
    :m_device_ptr                (device_ptr),
     m_window_ptr                (window_ptr),
     m_image_available_fence_ptrs(nullptr),
     m_image_format              (format),
     m_image_ptrs                (nullptr),
     m_image_view_ptrs           (nullptr),
     m_n_acquire_counter         (0),
     m_n_swapchain_images        (n_images),
     m_parent_surface_ptr        (parent_surface_ptr),
     m_present_mode              (present_mode),
     m_swapchain                 (0),
     m_usage_flags               (static_cast<VkImageUsageFlagBits>(usage_flags) ),
     m_vkAcquireNextImageKHR     (pfn_acquire_next_image_proc),
     m_vkCreateSwapchainKHR      (pfn_create_swapchain_proc),
     m_vkDestroySwapchainKHR     (pfn_destroy_swapchain_proc),
     m_vkGetSwapchainImagesKHR   (pfn_get_swapchain_images_proc)

{
    anvil_assert(n_images           >  0);
    anvil_assert(parent_surface_ptr != nullptr);
    anvil_assert(usage_flags        != 0);

    m_image_available_fence_ptrs = new Anvil::Fence*    [n_images];
    m_image_ptrs                 = new Anvil::Image*    [n_images];
    m_image_view_ptrs            = new Anvil::ImageView*[n_images];

    m_parent_surface_ptr->retain();

    for (uint32_t n_fence = 0;
                  n_fence < n_images;
                ++n_fence)
    {
        m_image_available_fence_ptrs[n_fence] = new Anvil::Fence(m_device_ptr,
                                                                 false /* create_signalled */);
    }


    init();

    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_SWAPCHAIN,
                                                  this);
}

/** Please see header for specification */
Anvil::Swapchain::~Swapchain()
{
    if (m_swapchain != VK_NULL_HANDLE)
    {
        m_vkDestroySwapchainKHR(m_device_ptr->get_device_vk(),
                                m_swapchain,
                                nullptr /* pAllocator */);

        m_swapchain = VK_NULL_HANDLE;
    }

    if (m_parent_surface_ptr != nullptr)
    {
        m_parent_surface_ptr->release();

        m_parent_surface_ptr = nullptr;
    }

    if (m_image_ptrs != nullptr)
    {
        for (uint32_t n_image = 0;
                      n_image < m_n_swapchain_images;
                    ++n_image)
        {
            if (m_image_ptrs[n_image] != nullptr)
            {
                m_image_ptrs[n_image]->release();
            }
        }

        delete [] m_image_ptrs;
        m_image_ptrs = nullptr;
    }

    if (m_image_available_fence_ptrs != nullptr)
    {
        for (uint32_t n_fence = 0;
                      n_fence < m_n_swapchain_images;
                    ++n_fence)
        {
            m_image_available_fence_ptrs[n_fence]->release();

            m_image_available_fence_ptrs[n_fence] = nullptr;
        }

        delete [] m_image_available_fence_ptrs;
        m_image_available_fence_ptrs = nullptr;
    }

    if (m_image_view_ptrs != nullptr)
    {
        for (uint32_t n_image_view = 0;
                      n_image_view < m_n_swapchain_images;
                    ++n_image_view)
        {
            if (m_image_view_ptrs != nullptr)
            {
                m_image_view_ptrs[n_image_view]->release();
            }
        }

        delete [] m_image_view_ptrs;
        m_image_view_ptrs = nullptr;
    }

    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_SWAPCHAIN,
                                                    this);
}

/** Please see header for specification */
uint32_t Anvil::Swapchain::acquire_image_by_blocking()
{
    uint32_t result                         = -1;
    VkResult result_vk;
    bool     is_offscreen_rendering_enabled = m_window_ptr->is_dummy();

    /* Which swap-chain image should we render to? */
    m_image_available_fence_ptrs[m_n_acquire_counter]->reset();
    if (!is_offscreen_rendering_enabled)
    {
        result_vk = m_vkAcquireNextImageKHR(m_device_ptr->get_device_vk(),
                                            m_swapchain,
                                            UINT64_MAX,
                                            VK_NULL_HANDLE, /* semaphore */
                                            m_image_available_fence_ptrs[m_n_acquire_counter]->get_fence(),
                                           &result);

        anvil_assert_vk_call_succeeded(result_vk);

        result_vk = vkWaitForFences(m_device_ptr->get_device_vk(),
                                    1, /* fenceCount */
                                    m_image_available_fence_ptrs[m_n_acquire_counter]->get_fence_ptr(),
                                    VK_TRUE, /* waitAll */
                                    UINT64_MAX);

        anvil_assert_vk_call_succeeded(result_vk);
    }

    m_n_acquire_counter = (m_n_acquire_counter + 1) % m_n_swapchain_images;

    if (is_offscreen_rendering_enabled)
    {
        result          = m_n_acquire_counter;
    }

    return result;
}

/** Please see header for specification */
uint32_t Anvil::Swapchain::acquire_image_by_setting_semaphore(Anvil::Semaphore* semaphore_ptr)
{
    uint32_t                   result = -1;
    VkResult                   result_vk;
    const VkPipelineStageFlags semaphore_wait_pipeline_stage  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    bool                       is_offscreen_rendering_enabled = m_window_ptr->is_dummy();

    if (!is_offscreen_rendering_enabled)
    {
        result_vk = m_vkAcquireNextImageKHR(m_device_ptr->get_device_vk(),
                                            m_swapchain,
                                            UINT64_MAX,
                                            semaphore_ptr->get_semaphore(),
                                            VK_NULL_HANDLE,
                                           &result);

        anvil_assert_vk_call_succeeded(result_vk);
    }

    m_n_acquire_counter = (m_n_acquire_counter + 1) % m_n_swapchain_images;

    if (is_offscreen_rendering_enabled)
    {
        result          = m_n_acquire_counter;
    }

    return result;
}

/** Please see header for specification */
Anvil::Image* Anvil::Swapchain::get_image(uint32_t n_swapchain_image) const
{
    anvil_assert(n_swapchain_image < m_n_swapchain_images);

    return m_image_ptrs[n_swapchain_image];
}

/** Please see header for specification */
Anvil::ImageView* Anvil::Swapchain::get_image_view(uint32_t n_swapchain_image) const
{
    anvil_assert(n_swapchain_image < m_n_swapchain_images);

    return m_image_view_ptrs[n_swapchain_image];
}

/** Initializes the swapchain object. */
void Anvil::Swapchain::init()
{
    VkSwapchainCreateInfoKHR            create_info;
    uint32_t                            n_swapchain_images             = 0;
    VkResult                            result;
    VkImage*                            swapchain_images               = nullptr;
    const VkSurfaceTransformFlagBitsKHR swapchain_transformation       = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    bool                                is_offscreen_rendering_enabled = m_window_ptr->is_dummy();

    m_image_view_format                                          = m_image_format;

    /* not offscreen rendering */
    if (!is_offscreen_rendering_enabled)
    {
        /* Ensure opaque composite alpha mode is supported */
        anvil_assert(m_parent_surface_ptr->get_supported_composite_alpha_flags() & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

        /* Ensure we can use the swapchain image format  */
        anvil_assert(m_parent_surface_ptr->is_compatible_with_image_format(m_image_format) );

        /* Ensure identity transformation is supported by the rendering surface */
        anvil_assert(m_parent_surface_ptr->get_supported_transformations() & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR);

        /* Fill the swap chain create info descriptor */
        anvil_assert(m_parent_surface_ptr->get_capabilities().maxImageCount == 0                    ||
                     m_parent_surface_ptr->get_capabilities().maxImageCount >= m_n_swapchain_images);

        create_info.clipped               = true; /* we won't be reading from the presentable images */
        create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.flags                 = 0;
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

        result = m_vkCreateSwapchainKHR(m_device_ptr->get_device_vk(),
                                       &create_info,
                                        nullptr, /* pAllocator */
                                       &m_swapchain);

        anvil_assert_vk_call_succeeded(result);

        /* Retrieve swap-chain images */
        result = m_vkGetSwapchainImagesKHR(m_device_ptr->get_device_vk(),
                                           m_swapchain,
                                          &n_swapchain_images,
                                           nullptr); /* pSwapchainImages */

        anvil_assert_vk_call_succeeded(result);
        anvil_assert                  (n_swapchain_images >  0);

        swapchain_images = new VkImage[n_swapchain_images];
        anvil_assert(swapchain_images != nullptr);

        result = m_vkGetSwapchainImagesKHR(m_device_ptr->get_device_vk(),
                                           m_swapchain,
                                          &n_swapchain_images,
                                           swapchain_images);

        anvil_assert_vk_call_succeeded(result);
    }
    else /* offscreen rendering */
    {
        n_swapchain_images = m_n_swapchain_images;
    }

    for (size_t n_result_image = 0;
                n_result_image < n_swapchain_images;
              ++n_result_image)
    {
        /* Spawn an Image wrapper class for the swap-chain image.
         *
         * NOTE: The constructor we use is special, in that the wrapped VkImage instance will not be destroyed
         *       when the Image instance goes out of scope.
         */
        if (!is_offscreen_rendering_enabled)
        {
            m_image_ptrs[n_result_image] = new Anvil::Image(m_device_ptr,
                                                            swapchain_images[n_result_image],
                                                            create_info.imageFormat,
                                                            VK_IMAGE_TILING_OPTIMAL,
                                                            create_info.imageUsage,
                                                            create_info.imageExtent.width,
                                                            create_info.imageExtent.height,
                                                            1,                            /* base_mipmap_depth */
                                                            create_info.imageArrayLayers,
                                                            1,                           /* n_mipmaps          */
                                                            VK_SAMPLE_COUNT_1_BIT,
                                                            1,                           /* n_slices           */
                                                            true);                       /* is_mutable         */
        }
        else
        {
            m_image_ptrs[n_result_image] = new Anvil::Image(m_device_ptr,
                                                            VK_IMAGE_TYPE_2D,
                                                            m_image_format,
                                                            VK_IMAGE_TILING_OPTIMAL,
                                                            m_usage_flags,
                                                            m_parent_surface_ptr->get_width(),
                                                            m_parent_surface_ptr->get_height(),
                                                            1,                            /* base_mipmap_depth */
                                                            1,
                                                            VK_SAMPLE_COUNT_1_BIT,
                                                            QUEUE_FAMILY_GRAPHICS_BIT | QUEUE_FAMILY_DMA_BIT,
                                                            VK_SHARING_MODE_CONCURRENT,
                                                            false,
                                                            true,
                                                            true,
                                                            true,                         /* is_mutable         */
                                                            VK_IMAGE_LAYOUT_GENERAL,
                                                            nullptr);
        }

        /* For each swap-chain image, create a relevant view */
        m_image_view_ptrs[n_result_image] = Anvil::ImageView::create_2D_image_view(m_device_ptr,
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

        /* Note: swapchain images must not be transitioned before they are acquired. We can't do that here.
         *
         * See issue #1 for more details
         */
        m_image_ptrs[n_result_image]->set_creation_time_image_layout( (is_offscreen_rendering_enabled) ? VK_IMAGE_LAYOUT_GENERAL 
                                                                                                       : VK_IMAGE_LAYOUT_UNDEFINED);
    }

    /* Clean up */
    if (swapchain_images != nullptr)
    {
        delete [] swapchain_images;

        swapchain_images = nullptr;
    }
}

/** Please see header for specification */
void Anvil::Swapchain::set_image_view_format(VkFormat new_view_format)
{
    anvil_assert(new_view_format != m_image_view_format);

    m_image_view_format = new_view_format;

    for (uint32_t n_image_view = 0;
                  n_image_view < m_n_swapchain_images;
                ++n_image_view)
    {
        m_image_view_ptrs[n_image_view]->release();

        m_image_view_ptrs[n_image_view] = Anvil::ImageView::create_2D_image_view(m_device_ptr,
                                                                                  m_image_ptrs[n_image_view],
                                                                                  0, /* n_base_layer */
                                                                                  0, /* n_base_mipmap_level */
                                                                                  1, /* n_mipmaps           */
                                                                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                                                                  m_image_view_format,
                                                                                  VK_COMPONENT_SWIZZLE_R,
                                                                                  VK_COMPONENT_SWIZZLE_G,
                                                                                  VK_COMPONENT_SWIZZLE_B,
                                                                                  VK_COMPONENT_SWIZZLE_A);
    }
}