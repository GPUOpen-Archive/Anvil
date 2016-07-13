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
#include "wrappers/command_pool.h"
#include "wrappers/compute_pipeline_manager.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/device.h"
#include "wrappers/graphics_pipeline_manager.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"
#include "wrappers/pipeline_cache.h"
#include "wrappers/queue.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/swapchain.h"

/* Please see header for specification */
Anvil::Device::Device(Anvil::PhysicalDevice*          physical_device_ptr,
                      const std::vector<const char*>& extensions,
                      const std::vector<const char*>& layers,
                      bool                            transient_command_buffer_allocs_only,
                      bool                            support_resettable_command_buffer_allocs)
    :m_compute_pipeline_manager_ptr (nullptr),
     m_device                       (VK_NULL_HANDLE),
     m_dummy_dsg_ptr                (nullptr),
     m_graphics_pipeline_manager_ptr(nullptr),
     m_parent_physical_device_ptr   (physical_device_ptr),
     m_pipeline_cache_ptr           (nullptr),
     m_vkAcquireNextImageKHR        (nullptr),
     m_vkCreateSwapchainKHR         (nullptr),
     m_vkDestroySwapchainKHR        (nullptr),
     m_vkGetSwapchainImagesKHR      (nullptr),
     m_vkQueuePresentKHR            (nullptr)
{
    VkDeviceCreateInfo                         create_info;
    std::vector<VkDeviceQueueCreateInfo>       device_queue_create_info_items;
    VkPhysicalDeviceFeatures                   features_to_enable;
    const bool                                 is_validation_enabled          = physical_device_ptr->get_instance()->is_validation_enabled();
    std::vector<const char*>                   layers_final;
    uint32_t                                   n_max_queue_priorities_needed  = 0;
    std::map<Anvil::QueueFamilyType, uint32_t> queue_family_type_to_index_map;
    std::vector<float>                         queue_priorities;
    VkResult                                   result;

    struct
    {
        uint32_t family_index;
        uint32_t n_queues;
    } device_queue_family_info_items[Anvil::QUEUE_FAMILY_TYPE_COUNT];

    static_assert(Anvil::QUEUE_FAMILY_TYPE_COUNT == 3, "");

    memset(m_command_pool_ptrs,
           0,
           sizeof(m_command_pool_ptrs) );

    /* Cache the physical device */
    m_parent_physical_device_ptr = physical_device_ptr;

    /* If validation is enabled, retrieve names of all suported validation layers and
     * append them to the list of layers the user has alreaedy specified. **/
    layers_final = layers;

    if (is_validation_enabled)
    {
#ifdef VK_API_VERSION
        /* This is a backward-compatible solution, which should work for older SDK versions. */
        const Anvil::Layers& physical_device_layers = physical_device_ptr->get_layers();

        for (auto& current_physical_device_layer : physical_device_layers)
        {
            const std::string& current_layer_description = current_physical_device_layer.description;
            const std::string& current_layer_name        = current_physical_device_layer.name;

            if (current_layer_description == "LunarG Validation Layer" &&
                std::find(layers_final.begin(),
                          layers_final.end(),
                          current_layer_name) == layers_final.end() )
            {
                layers_final.push_back(current_layer_name.c_str() );
            }
        }
#else
        /* This is needed for SDK 1.0.11.0, likely future versions too. 
         *
         * NOTE: Older SDK versions used VK_LAYER_LUNARG_standard_validation, but the latest ones
         *       appear to have switched to VK_LAYER_LUNARG_core_validation. Need to take this
         *       into account here.
         */
        if (physical_device_ptr->is_layer_supported("VK_LAYER_LUNARG_standard_validation") )
        {
            layers_final.push_back("VK_LAYER_LUNARG_standard_validation");
        }
        else
        {
            anvil_assert(physical_device_ptr->is_layer_supported("VK_LAYER_LUNARG_core_validation") );

            layers_final.push_back("VK_LAYER_LUNARG_core_validation");
        }
#endif
    }

    /* Make sure all required device extensions are supported */
    for (auto extension_name : extensions)
    {
        if (!physical_device_ptr->is_device_extension_supported(extension_name) )
        {
            char temp[1024];

            snprintf(temp,
                     sizeof(temp),
                     "Device extension [%s] is unsupported",
                     extension_name);

            #ifdef _WIN32
            {
                MessageBoxA(HWND_DESKTOP,
                            temp,
                            "Error",
                            MB_OK | MB_ICONERROR);

                exit(1);
            }
            #else
            {
                fprintf(stderr,
                        "%s",
                        temp);

                fflush(stderr);
                exit(1);
            }
            #endif
        }
    }

    /* Determine compute, graphics and transfer queue family indices */
    get_queue_family_indices(&device_queue_family_info_items[Anvil::QUEUE_FAMILY_TYPE_COMPUTE].family_index,
                             &device_queue_family_info_items[Anvil::QUEUE_FAMILY_TYPE_COMPUTE].n_queues,
                             &device_queue_family_info_items[Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL].family_index,
                             &device_queue_family_info_items[Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL].n_queues,
                             &device_queue_family_info_items[Anvil::QUEUE_FAMILY_TYPE_TRANSFER].family_index,
                             &device_queue_family_info_items[Anvil::QUEUE_FAMILY_TYPE_TRANSFER].n_queues);

    for (uint32_t n_queue_family = 0;
                  n_queue_family < Anvil::QUEUE_FAMILY_TYPE_COUNT;
                ++n_queue_family)
    {
        if (device_queue_family_info_items[n_queue_family].n_queues > 0)
        {
            VkDeviceQueueCreateInfo queue_create_info;

            queue_create_info.flags            = 0;
            queue_create_info.pNext            = nullptr;
            queue_create_info.pQueuePriorities = nullptr; /* set later */
            queue_create_info.queueCount       = device_queue_family_info_items[n_queue_family].n_queues;
            queue_create_info.queueFamilyIndex = device_queue_family_info_items[n_queue_family].family_index;
            queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

            queue_family_type_to_index_map[static_cast<QueueFamilyType>(n_queue_family)] = device_queue_family_info_items[n_queue_family].family_index;

            device_queue_create_info_items.push_back(queue_create_info);

            if (device_queue_create_info_items.back().queueCount > n_max_queue_priorities_needed)
            {
                n_max_queue_priorities_needed = device_queue_create_info_items.back().queueCount;
            }
        }
    }

    /* Prepare a dummy queue priority array */
    queue_priorities.resize(n_max_queue_priorities_needed,
                            1.0f);

    for (auto& create_info_item : device_queue_create_info_items)
    {
        create_info_item.pQueuePriorities = &queue_priorities[0];
    }

    /* Set up the device create info descriptor. Enable all features available. */
    features_to_enable = physical_device_ptr->get_device_features();

    create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size() );
    create_info.enabledLayerCount       = static_cast<uint32_t>(layers_final.size() );
    create_info.flags                   = 0;
    create_info.pEnabledFeatures        = &features_to_enable;
    create_info.pNext                   = nullptr;
    create_info.ppEnabledExtensionNames = (extensions.size()   > 0) ? &extensions[0]   : nullptr;
    create_info.ppEnabledLayerNames     = (layers_final.size() > 0) ? &layers_final[0] : nullptr;
    create_info.pQueueCreateInfos       = &device_queue_create_info_items[0];
    create_info.queueCreateInfoCount    = static_cast<uint32_t>(device_queue_create_info_items.size() );
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    result = vkCreateDevice(physical_device_ptr->get_physical_device(),
                           &create_info,
                            nullptr, /* pAllocator */
                           &m_device);
    anvil_assert_vk_call_succeeded(result);

    /* Cache the enabled extensions */
    for (auto extension : extensions)
    {
        m_enabled_extensions.push_back(extension);
    }

    /* Retrieve device-specific func pointers */
    m_vkAcquireNextImageKHR   = reinterpret_cast<PFN_vkAcquireNextImageKHR>  (get_proc_address("vkAcquireNextImageKHR") );
    m_vkCreateSwapchainKHR    = reinterpret_cast<PFN_vkCreateSwapchainKHR>   (get_proc_address("vkCreateSwapchainKHR") );
    m_vkDestroySwapchainKHR   = reinterpret_cast<PFN_vkDestroySwapchainKHR>  (get_proc_address("vkDestroySwapchainKHR") );
    m_vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(get_proc_address("vkGetSwapchainImagesKHR") );
    m_vkQueuePresentKHR       = reinterpret_cast<PFN_vkQueuePresentKHR>      (get_proc_address("vkQueuePresentKHR") );

    anvil_assert(m_vkAcquireNextImageKHR   != nullptr);
    anvil_assert(m_vkCreateSwapchainKHR    != nullptr);
    anvil_assert(m_vkDestroySwapchainKHR   != nullptr);
    anvil_assert(m_vkGetSwapchainImagesKHR != nullptr);
    anvil_assert(m_vkQueuePresentKHR       != nullptr);

    /* Retrieve queue handles */
    for (Anvil::QueueFamilyType queue_family_type = Anvil::QUEUE_FAMILY_TYPE_FIRST;
                                queue_family_type < Anvil::QUEUE_FAMILY_TYPE_COUNT;
                        ++(int&)queue_family_type)
    {
        const uint32_t              n_queues = device_queue_family_info_items[queue_family_type].n_queues;
        std::vector<Anvil::Queue*>& queues   = (queue_family_type == Anvil::QUEUE_FAMILY_TYPE_COMPUTE)   ? m_compute_queues
                                             : (queue_family_type == Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL) ? m_universal_queues
                                                                                                         : m_transfer_queues;

        if (n_queues == 0)
        {
            continue;
        }

        for (uint32_t n_queue = 0;
                      n_queue < n_queues;
                    ++n_queue)
        {
            queues.push_back(new Anvil::Queue(this,
                                              queue_family_type_to_index_map[queue_family_type],
                                              n_queue,
                                              m_vkQueuePresentKHR) );
        }

        m_command_pool_ptrs[queue_family_type] = new Anvil::CommandPool(this,
                                                                         transient_command_buffer_allocs_only,
                                                                         support_resettable_command_buffer_allocs,
                                                                         queue_family_type);
    }

    /* Initialize a dummy descriptor set group */
    m_dummy_dsg_ptr = new Anvil::DescriptorSetGroup(this,
                                                    false, /* releaseable_sets */
                                                    1);    /* n_sets           */

    m_dummy_dsg_ptr->add_binding(0, /* n_set     */
                                 0, /* n_binding */
                                 VK_DESCRIPTOR_TYPE_SAMPLER,
                                 0, /* n_elements */
                                 VK_SHADER_STAGE_ALL);
    m_dummy_dsg_ptr->get_descriptor_set_layout(0)->bake();
    m_dummy_dsg_ptr->get_descriptor_set(0)->bake();

    /* Set up the pipeline cache */
    m_pipeline_cache_ptr = new Anvil::PipelineCache(this);

    /* Initialize compute & graphics pipeline managers
     *
     * NOTE: We force pipeline cache usage here because of LunarG#223 and LunarG#227 */
    m_compute_pipeline_manager_ptr  = new Anvil::ComputePipelineManager (this,
                                                                         true /* use_pipeline_cache */,
                                                                         m_pipeline_cache_ptr);
    m_graphics_pipeline_manager_ptr = new Anvil::GraphicsPipelineManager(this,
                                                                         true /* use_pipeline_cache */,
                                                                         m_pipeline_cache_ptr);

    /* Register the instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_DEVICE,
                                                  this);
}

/* Please see header for specification */
Anvil::Device::~Device()
{
    for (uint32_t n_command_pool = 0;
                  n_command_pool < sizeof(m_command_pool_ptrs) / sizeof(m_command_pool_ptrs[0]);
                ++n_command_pool)
    {
        if (m_command_pool_ptrs[n_command_pool] != nullptr)
        {
            m_command_pool_ptrs[n_command_pool]->release();

            m_command_pool_ptrs[n_command_pool] = nullptr;
        }
    }

    if (m_dummy_dsg_ptr != nullptr)
    {
        m_dummy_dsg_ptr->release();

        m_dummy_dsg_ptr = nullptr;
    }

    if (m_compute_pipeline_manager_ptr != nullptr)
    {
        delete m_compute_pipeline_manager_ptr;

        m_compute_pipeline_manager_ptr = nullptr;
    }

    if (m_graphics_pipeline_manager_ptr != nullptr)
    {
        delete m_graphics_pipeline_manager_ptr;

        m_graphics_pipeline_manager_ptr = nullptr;
    }

    if (m_pipeline_cache_ptr != nullptr)
    {
        m_pipeline_cache_ptr->release();

        m_pipeline_cache_ptr = nullptr;
    }

    for (Anvil::QueueFamilyType queue_family_type = Anvil::QUEUE_FAMILY_TYPE_FIRST;
                                queue_family_type < Anvil::QUEUE_FAMILY_TYPE_COUNT;
                        ++(int&)queue_family_type)
    {
        std::vector<Anvil::Queue*>& queues   = (queue_family_type == Anvil::QUEUE_FAMILY_TYPE_COMPUTE)   ? m_compute_queues
                                             : (queue_family_type == Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL) ? m_universal_queues
                                                                                                         : m_transfer_queues;

        while (queues.size() > 0)
        {
            delete queues.back();

            queues.pop_back();
        }
    }

    if (m_device != nullptr)
    {
        vkDestroyDevice(m_device,
                        nullptr /* pAllocator */);

        m_device = nullptr;
    }

    /* Unregister the instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_DEVICE,
                                                    this);
}

/** Please see header for specification */
Anvil::Swapchain* Anvil::Device::create_swapchain(Anvil::RenderingSurface* parent_surface_ptr,
                                                  Anvil::Window*           window_ptr,
                                                  VkFormat                 image_format,
                                                  VkPresentModeKHR         present_mode,
                                                  VkImageUsageFlags        usage,
                                                  uint32_t                 n_swapchain_images)
{
    Anvil::Swapchain* result_ptr = nullptr;

    result_ptr = new Anvil::Swapchain(this,
                                      parent_surface_ptr,
                                      window_ptr,
                                      image_format,
                                      present_mode,
                                      usage,
                                      n_swapchain_images,
                                      m_vkAcquireNextImageKHR,
                                      m_vkCreateSwapchainKHR,
                                      m_vkDestroySwapchainKHR,
                                      m_vkGetSwapchainImagesKHR);

    if (result_ptr != nullptr)
    {
        /* For each queue instantiated for the device, we need to update its presentability status.
         * This is done by assigning a swap-chain to each queue. *
         *
         * NOTE: This will not work correctly if more than one swapchain is ever created for the same device.
         */
        for (Anvil::QueueFamilyType queue_family = Anvil::QUEUE_FAMILY_TYPE_FIRST;
                                    queue_family < Anvil::QUEUE_FAMILY_TYPE_COUNT;
                            ++(int&)queue_family)
        {
            for (auto& queue_ptr :  ((queue_family == Anvil::QUEUE_FAMILY_TYPE_COMPUTE)  ? m_compute_queues
                                 :   (queue_family == Anvil::QUEUE_FAMILY_TYPE_TRANSFER) ? m_transfer_queues
                                                                                         : m_universal_queues))
            {
                queue_ptr->set_swapchain(result_ptr);
            }
        }
    }

    return result_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet* Anvil::Device::get_dummy_descriptor_set() const
{
    return m_dummy_dsg_ptr->get_descriptor_set(0);
}

/** Please see header for specification */
Anvil::DescriptorSetLayout* Anvil::Device::get_dummy_descriptor_set_layout() const
{
    return m_dummy_dsg_ptr->get_descriptor_set_layout(0);
}

/** Retrieves family indices of compute, graphics and DMA queue families.
 *
 *  @param out_compute_queue_family_index_ptr   Deref will be set to the index of a queue which supports compute
 *                                              operations, but does not handle graphics operations. Must not be nullptr.
 *  @param out_n_compute_queues_available_ptr   Deref will be set to the number of compute queues supported by
 *                                              a queue family at index *@param out_compute_queue_family_index_ptr.
 *                                              Must not be nullptr.
 *  @param out_universal_queue_family_index_ptr Deref will be set to the index of a universal queue which supports compute+fx
 *                                              operations. Must not be nullptr.
 *  @param out_n_universal_queues_available_ptr Deref will be set to the number of universal queues supported by
 *                                              a queue family at index *@param out_universal_queue_family_index_ptr.
 *                                              Must not be nullptr.
 *  @param out_dma_queue_family_index_ptr       Deref will be set to the index of a queue which supports DMA operations,
 *                                              and is different from the previous two indices.
 *  @param out_n_universal_queues_available_ptr Deref will be set to the number of transfer queues supported by
 *                                              a queue family at index *@param out_dma_queue_family_index_ptr.
 *                                              Must not be nullptr.
 *
 **/
void Anvil::Device::get_queue_family_indices(uint32_t* out_compute_queue_family_index_ptr,
                                             uint32_t* out_n_compute_queues_available_ptr,
                                             uint32_t* out_universal_queue_family_index_ptr,
                                             uint32_t* out_n_universal_queues_available_ptr,
                                             uint32_t* out_dma_queue_family_index_ptr,
                                             uint32_t* out_n_dma_queues_available_ptr) const
{
    /* Retrieve a compute-only queue, and then look for another queue which can handle graphics tasks. */
    const uint32_t physical_device_index               = m_parent_physical_device_ptr->get_index();
    const size_t   n_queue_families                    = m_parent_physical_device_ptr->get_queue_families().size();
          uint32_t result_compute_queue_family_index   = -1;
          uint32_t result_dma_queue_family_index       = -1;
          uint32_t result_universal_queue_family_index = -1;
          uint32_t result_n_compute_queues             = 0;
          uint32_t result_n_dma_queues                 = 0;
          uint32_t result_n_universal_queues           = 0;

    for (uint32_t n_iteration = 0;
                  n_iteration < 3;
                ++n_iteration)
    {
        for (size_t n_queue_family_index = 0;
                    n_queue_family_index < n_queue_families;
                  ++n_queue_family_index)
        {
            const Anvil::QueueFamilyInfo& current_queue_family = m_parent_physical_device_ptr->get_queue_families()[n_queue_family_index];

            if (n_iteration == 0)
            {
                if ( (current_queue_family.flags & VK_QUEUE_COMPUTE_BIT)  &&
                    !(current_queue_family.flags & VK_QUEUE_GRAPHICS_BIT) )
                {
                    result_compute_queue_family_index = (uint32_t) n_queue_family_index;
                    result_n_compute_queues           = current_queue_family.n_queues;

                    break;
                }
            }
            else
            if (n_iteration == 1)
            {
                if (current_queue_family.flags & VK_QUEUE_GRAPHICS_BIT        &&
                    n_queue_family_index != result_compute_queue_family_index)
                {
                    result_universal_queue_family_index = (uint32_t) n_queue_family_index;
                    result_n_universal_queues           = current_queue_family.n_queues;

                    break;
                }
            }
            else
            {
                if (current_queue_family.flags & VK_QUEUE_TRANSFER_BIT         &&
                    n_queue_family_index != result_compute_queue_family_index  &&
                    n_queue_family_index != result_universal_queue_family_index)
                {
                    result_dma_queue_family_index = (uint32_t) n_queue_family_index;
                    result_n_dma_queues           = current_queue_family.n_queues;

                    break;
                }
            }
        }
    }

    /* NOTE: Vulkan API only guarantees universal queue family's support */
    anvil_assert(result_universal_queue_family_index != -1);

    *out_compute_queue_family_index_ptr   = result_compute_queue_family_index;
    *out_dma_queue_family_index_ptr       = result_dma_queue_family_index;
    *out_universal_queue_family_index_ptr = result_universal_queue_family_index;
    *out_n_compute_queues_available_ptr   = result_n_compute_queues;
    *out_n_dma_queues_available_ptr       = result_n_dma_queues;
    *out_n_universal_queues_available_ptr = result_n_universal_queues;
}

/* Please see header for specification */
uint32_t Anvil::Device::get_queue_family_index(Anvil::QueueFamilyType family_type) const
{
    uint32_t result    = -1;
    Queue*   queue_ptr = nullptr;

    switch (family_type)
    {
        case Anvil::QUEUE_FAMILY_TYPE_COMPUTE:
        {
            queue_ptr = get_compute_queue(0);

            break;
        }

        case Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL:
        {
            queue_ptr = get_universal_queue(0);

            break;
        }

        case Anvil::QUEUE_FAMILY_TYPE_TRANSFER:
        {
            queue_ptr = get_transfer_queue(0);

            break;
        }

        default:
        {
            anvil_assert(false);
        }
    }

    if (queue_ptr != nullptr)
    {
        result = queue_ptr->get_queue_family_index();
    }

    return result;
}