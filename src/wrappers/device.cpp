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
#include "wrappers/pipeline_layout_manager.h"
#include "wrappers/queue.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/swapchain.h"


/* Please see header for specification */
Anvil::BaseDevice::BaseDevice(std::weak_ptr<Anvil::Instance> in_parent_instance_ptr)
    :m_destroyed          (false),
     m_device             (VK_NULL_HANDLE),
     m_parent_instance_ptr(in_parent_instance_ptr)
{
    std::shared_ptr<Anvil::Instance> instance_locked_ptr(in_parent_instance_ptr);

    m_khr_surface_entrypoints = instance_locked_ptr->get_extension_khr_surface_entrypoints();

    /* Register the instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_DEVICE,
                                                 this);
}

Anvil::BaseDevice::~BaseDevice()
{
    anvil_assert(m_destroyed);

    if (m_device != nullptr)
    {
        vkDeviceWaitIdle(m_device);
        vkDestroyDevice (m_device,
                         nullptr /* pAllocator */);

        m_device = nullptr;
    }

    /* Unregister the instance */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_DEVICE,
                                                    this);
}

/** Please see header for specification */
void Anvil::BaseDevice::destroy()
{
    anvil_assert(!m_destroyed);

    m_destroyed = true;

    for (uint32_t n_command_pool = 0;
                  n_command_pool < sizeof(m_command_pool_ptrs) / sizeof(m_command_pool_ptrs[0]);
                ++n_command_pool)
    {
        m_command_pool_ptrs[n_command_pool] = nullptr;
    }

    m_compute_pipeline_manager_ptr  = nullptr;
    m_dummy_dsg_ptr                 = nullptr;
    m_graphics_pipeline_manager_ptr = nullptr;
    m_pipeline_cache_ptr            = nullptr;
    m_pipeline_layout_manager_ptr   = nullptr;

    /* Proceed with device-specific instances */
    for (Anvil::QueueFamilyType queue_family_type = Anvil::QUEUE_FAMILY_TYPE_FIRST;
                                queue_family_type < Anvil::QUEUE_FAMILY_TYPE_COUNT + 1;
                                queue_family_type = static_cast<Anvil::QueueFamilyType>(queue_family_type + 1))
    {
        std::vector<std::shared_ptr<Anvil::Queue> >& queues = (queue_family_type == Anvil::QUEUE_FAMILY_TYPE_COMPUTE)           ? m_compute_queues
                                                            : (queue_family_type == Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)         ? m_universal_queues
                                                                                                                                : m_transfer_queues;

        queues.clear();
    }
}

/** Please see header for specification */
std::shared_ptr<Anvil::DescriptorSet> Anvil::BaseDevice::get_dummy_descriptor_set() const
{
    return m_dummy_dsg_ptr->get_descriptor_set(0);
}

/** Please see header for specification */
std::shared_ptr<Anvil::DescriptorSetLayout> Anvil::BaseDevice::get_dummy_descriptor_set_layout() const
{
    return m_dummy_dsg_ptr->get_descriptor_set_layout(0);
}

/** Please see header for specification */
const Anvil::ExtensionAMDDrawIndirectCountEntrypoints& Anvil::BaseDevice::get_extension_amd_draw_indirect_count_entrypoints() const
{
    anvil_assert(std::find(m_enabled_extensions.begin(),
                           m_enabled_extensions.end(),
                           "VK_AMD_draw_indirect_count") != m_enabled_extensions.end() );

    return m_amd_draw_indirect_count_extension_entrypoints;
}

/** Please see header for specification */
const Anvil::ExtensionKHRSwapchainEntrypoints& Anvil::BaseDevice::get_extension_khr_swapchain_entrypoints() const
{
    return m_khr_swapchain_entrypoints;
}

/* Please see header for specification */
void Anvil::BaseDevice::get_queue_family_indices_for_physical_device(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr,
                                                                     DeviceQueueFamilyInfo*               out_device_queue_family_info_ptr) const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(physical_device_ptr);

    /* Retrieve a compute-only queue, and then look for another queue which can handle graphics tasks. */
    const size_t   n_queue_families                    = physical_device_locked_ptr->get_queue_families().size();
    uint32_t       result_compute_queue_family_index   = UINT32_MAX;
    uint32_t       result_dma_queue_family_index       = UINT32_MAX;
    uint32_t       result_n_compute_queues             = 0;
    uint32_t       result_n_dma_queues                 = 0;
    uint32_t       result_n_universal_queues           = 0;
    uint32_t       result_universal_queue_family_index = UINT32_MAX;

    for (uint32_t n_iteration = 0;
                  n_iteration < 3;
                ++n_iteration)
    {
        for (size_t n_queue_family_index = 0;
                    n_queue_family_index < n_queue_families;
                  ++n_queue_family_index)
        {
            const Anvil::QueueFamilyInfo& current_queue_family = physical_device_locked_ptr->get_queue_families()[n_queue_family_index];

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
            if (n_iteration == 2)
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

    /* NOTE: Vulkan API only guarantees universal queue family's availability */
    anvil_assert(result_universal_queue_family_index != UINT32_MAX);

    out_device_queue_family_info_ptr->family_index[Anvil::QUEUE_FAMILY_TYPE_COMPUTE]   = result_compute_queue_family_index;
    out_device_queue_family_info_ptr->family_index[Anvil::QUEUE_FAMILY_TYPE_TRANSFER]  = result_dma_queue_family_index;
    out_device_queue_family_info_ptr->family_index[Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL] = result_universal_queue_family_index;

    out_device_queue_family_info_ptr->n_queues[Anvil::QUEUE_FAMILY_TYPE_COMPUTE]   = result_n_compute_queues;
    out_device_queue_family_info_ptr->n_queues[Anvil::QUEUE_FAMILY_TYPE_TRANSFER]  = result_n_dma_queues;
    out_device_queue_family_info_ptr->n_queues[Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL] = result_n_universal_queues;
}

/** Returns a vector filled with float values of 1.0, with as many elements as there are queue families described
 *  under @param device_queue_families_ptr.
 */
std::vector<float> Anvil::BaseDevice::get_queue_priorities(const DeviceQueueFamilyInfo* device_queue_families_ptr)
{
    /* TODO: Queue priority support */
    uint32_t           n_max_queue_priorities_needed = 0;
    std::vector<float> result;

    for (uint32_t n_queue_family = 0;
                  n_queue_family < Anvil::QUEUE_FAMILY_TYPE_COUNT;
                ++n_queue_family)
    {
        if (device_queue_families_ptr->n_queues[n_queue_family] > n_max_queue_priorities_needed)
        {
            n_max_queue_priorities_needed = device_queue_families_ptr->n_queues[n_queue_family];
        }
    }

    /* Prepare a dummy queue priority array */
    result.resize(n_max_queue_priorities_needed,
                  1.0f);

    return result;
}

/* Please see header for specification */
std::shared_ptr<Anvil::Queue> Anvil::BaseDevice::get_sparse_binding_queue(uint32_t     n_queue,
                                                                          VkQueueFlags opt_required_queue_flags) const
{
    uint32_t                      n_queues_found = 0;
    std::shared_ptr<Anvil::Queue> result_ptr;

    for (auto queue_ptr : m_sparse_binding_queues)
    {
        const uint32_t queue_family_index    = queue_ptr->get_queue_family_index();
        const auto&    queue_family_info_ptr = get_queue_family_info            (queue_family_index);

        if ((queue_family_info_ptr->flags & opt_required_queue_flags) == opt_required_queue_flags)
        {
            if (n_queues_found == n_queue)
            {
                result_ptr = queue_ptr;
                break;
            }
            else
            {
                ++n_queues_found;
            }
        }
    }

    return result_ptr;
}

/* Initializes a new Device instance */
void Anvil::BaseDevice::init(const std::vector<const char*>& extensions,
                             const std::vector<const char*>& layers,
                             bool                            transient_command_buffer_allocs_only,
                             bool                            support_resettable_command_buffer_allocs)
{

    VkPhysicalDeviceFeatures         features_to_enable;
    std::shared_ptr<Anvil::Instance> instance_locked_ptr  (m_parent_instance_ptr);
    const bool                       is_validation_enabled(instance_locked_ptr->is_validation_enabled() );
    std::vector<const char*>         layers_final;

    /* If validation is enabled, retrieve names of all suported validation layers and
     * append them to the list of layers the user has alreaedy specified. **/
    layers_final = layers;

    if (is_validation_enabled)
    {
        #ifdef VK_API_VERSION
        {
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
        }
        #else
        {
            /* This is needed for SDK 1.0.11.0, likely future versions too. 
             *
             * NOTE: Older SDK versions used VK_LAYER_LUNARG_standard_validation, but the latest ones
             *       appear to have switched to VK_LAYER_LUNARG_core_validation. Need to take this
             *       into account here.
             */
            if (is_layer_supported("VK_LAYER_LUNARG_standard_validation") )
            {
                layers_final.push_back("VK_LAYER_LUNARG_standard_validation");
            }
            else
            {
                anvil_assert(is_layer_supported("VK_LAYER_LUNARG_core_validation") );

                layers_final.push_back("VK_LAYER_LUNARG_core_validation");
            }
        }
        #endif
    }

    /* Make sure all required device extensions are supported */
    for (auto extension_name : extensions)
    {
        if (!is_physical_device_extension_supported(extension_name) )
        {
            char temp[1024];

            snprintf(temp,
                     sizeof(temp),
                     "Device extension [%s] is unsupported",
                     extension_name);
            fprintf(stderr,
                    "%s",
                    temp);

            anvil_assert(false);
        }
    }

    /* Instantiate the device. Actual behavior behind this is implemented by the overriding class. */
    features_to_enable = get_physical_device_features();

    anvil_assert(m_device == VK_NULL_HANDLE);
    {
        create_device(extensions,
                    layers_final,
                    features_to_enable,
                   &m_device_queue_families);
    }
    anvil_assert(m_device != VK_NULL_HANDLE);

    /* Spawn queue wrappers */
    for (Anvil::QueueFamilyType queue_family_type = Anvil::QUEUE_FAMILY_TYPE_FIRST;
                                queue_family_type < Anvil::QUEUE_FAMILY_TYPE_COUNT;
                                queue_family_type = static_cast<Anvil::QueueFamilyType>(queue_family_type + 1))
    {
        const uint32_t                               n_queues = m_device_queue_families.n_queues[queue_family_type];
        std::vector<std::shared_ptr<Anvil::Queue> >& queues   = (queue_family_type == Anvil::QUEUE_FAMILY_TYPE_COMPUTE)   ? m_compute_queues
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
            std::shared_ptr<Anvil::Queue> new_queue_ptr;

            new_queue_ptr = Anvil::Queue::create(shared_from_this(),
                                                 m_device_queue_families.family_index[queue_family_type],
                                                 n_queue);

            queues.push_back(new_queue_ptr);

            /* If this queue supports sparse binding ops, cache it in a separate vector as well */
            if (new_queue_ptr->supports_sparse_bindings() )
            {
                m_sparse_binding_queues.push_back(new_queue_ptr);
            }
        }
    }

    /* Cache the enabled extensions */
    for (auto extension : extensions)
    {
        m_enabled_extensions.push_back(extension);
    }

    /* Retrieve device-specific func pointers */
    m_khr_swapchain_entrypoints.vkAcquireNextImageKHR   = reinterpret_cast<PFN_vkAcquireNextImageKHR>  (get_proc_address("vkAcquireNextImageKHR") );
    m_khr_swapchain_entrypoints.vkCreateSwapchainKHR    = reinterpret_cast<PFN_vkCreateSwapchainKHR>   (get_proc_address("vkCreateSwapchainKHR") );
    m_khr_swapchain_entrypoints.vkDestroySwapchainKHR   = reinterpret_cast<PFN_vkDestroySwapchainKHR>  (get_proc_address("vkDestroySwapchainKHR") );
    m_khr_swapchain_entrypoints.vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(get_proc_address("vkGetSwapchainImagesKHR") );
    m_khr_swapchain_entrypoints.vkQueuePresentKHR       = reinterpret_cast<PFN_vkQueuePresentKHR>      (get_proc_address("vkQueuePresentKHR") );

    anvil_assert(m_khr_swapchain_entrypoints.vkAcquireNextImageKHR   != nullptr);
    anvil_assert(m_khr_swapchain_entrypoints.vkCreateSwapchainKHR    != nullptr);
    anvil_assert(m_khr_swapchain_entrypoints.vkDestroySwapchainKHR   != nullptr);
    anvil_assert(m_khr_swapchain_entrypoints.vkGetSwapchainImagesKHR != nullptr);
    anvil_assert(m_khr_swapchain_entrypoints.vkQueuePresentKHR       != nullptr);

    if (std::find(m_enabled_extensions.begin(),
                  m_enabled_extensions.end(),
                  "VK_AMD_draw_indirect_count") != m_enabled_extensions.end() )
    {
        m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndexedIndirectCountAMD = reinterpret_cast<PFN_vkCmdDrawIndexedIndirectCountAMD>(get_proc_address("vkCmdDrawIndexedIndirectCountAMD") );
        m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndirectCountAMD        = reinterpret_cast<PFN_vkCmdDrawIndirectCountAMD>       (get_proc_address("vkCmdDrawIndirectCountAMD") );

        anvil_assert(m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndexedIndirectCountAMD != nullptr);
        anvil_assert(m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndirectCountAMD        != nullptr);
    }

    /* Instantiate per-queue family command pools */
    for (Anvil::QueueFamilyType queue_family_type = Anvil::QUEUE_FAMILY_TYPE_FIRST;
                                queue_family_type < Anvil::QUEUE_FAMILY_TYPE_COUNT;
                                queue_family_type = static_cast<Anvil::QueueFamilyType>(queue_family_type + 1))
    {
        if (get_queue_family_index(queue_family_type) != UINT32_MAX)
        {
            m_command_pool_ptrs[queue_family_type] = Anvil::CommandPool::create(shared_from_this(),
                                                                                transient_command_buffer_allocs_only,
                                                                                support_resettable_command_buffer_allocs,
                                                                                queue_family_type);
        }
    }

    /* Initialize a dummy descriptor set group */
    m_dummy_dsg_ptr = Anvil::DescriptorSetGroup::create(shared_from_this(),
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
    m_pipeline_cache_ptr = Anvil::PipelineCache::create(shared_from_this() );

    /* Cache a pipeline layout manager. This is needed to ensure the manager nevers goes out of scope while
     * the device is alive */
    m_pipeline_layout_manager_ptr = Anvil::PipelineLayoutManager::create(shared_from_this() );

    /* Initialize compute & graphics pipeline managers
     *
     * NOTE: We force pipeline cache usage here because of LunarG#223 and LunarG#227 */
    m_compute_pipeline_manager_ptr  = Anvil::ComputePipelineManager::create (shared_from_this(),
                                                                             true /* use_pipeline_cache */,
                                                                             m_pipeline_cache_ptr);
    m_graphics_pipeline_manager_ptr = Anvil::GraphicsPipelineManager::create(shared_from_this(),
                                                                             true /* use_pipeline_cache */,
                                                                             m_pipeline_cache_ptr);

    /* Continue with specialized initialization */
    init_device();
}


/* Please see header for specification */
Anvil::SGPUDevice::SGPUDevice(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr)
    :BaseDevice(physical_device_ptr.lock()->get_instance() ),
     m_parent_physical_device_ptr(physical_device_ptr)
{
    /* Stub */
}

/* Please see header for specification */
Anvil::SGPUDevice::~SGPUDevice()
{
    /* Stub */
}


/* Please see header for specification */
std::weak_ptr<Anvil::SGPUDevice> Anvil::SGPUDevice::create(std::weak_ptr<Anvil::PhysicalDevice> physical_device_ptr,
                                                           const std::vector<const char*>&      extensions,
                                                           const std::vector<const char*>&      layers,
                                                           bool                                 transient_command_buffer_allocs_only,
                                                           bool                                 support_resettable_command_buffer_allocs)
{
    std::shared_ptr<Anvil::SGPUDevice> result_ptr;

    result_ptr = std::shared_ptr<Anvil::SGPUDevice>(new Anvil::SGPUDevice(physical_device_ptr),
                                                    Anvil::SGPUDevice::SGPUDeviceDeleter() );

    result_ptr->init(extensions,
                     layers,
                     transient_command_buffer_allocs_only,
                     support_resettable_command_buffer_allocs);

    return result_ptr;
}

/** Please see header for specification */
void Anvil::SGPUDevice::create_device(const std::vector<const char*>& extensions,
                                      const std::vector<const char*>& layers,
                                      const VkPhysicalDeviceFeatures& features,
                                      DeviceQueueFamilyInfo*          out_queue_families_ptr)
{
    VkDeviceCreateInfo                     create_info;
    std::vector<VkDeviceQueueCreateInfo>   device_queue_create_info_items;
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr     (m_parent_physical_device_ptr);
    VkResult                               result                         (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Set up queue create info structure. */
    DeviceQueueFamilyInfo device_queue_families;
    std::vector<float>    device_queue_priorities;

    get_queue_family_indices(&device_queue_families);

    device_queue_priorities = get_queue_priorities(&device_queue_families);

    for (uint32_t n_queue_family = 0;
                  n_queue_family < Anvil::QUEUE_FAMILY_TYPE_COUNT;
                ++n_queue_family)
    {
        if (device_queue_families.n_queues[n_queue_family] > 0)
        {
            VkDeviceQueueCreateInfo queue_create_info;

            queue_create_info.flags            = 0;
            queue_create_info.pNext            = nullptr;
            queue_create_info.pQueuePriorities = &device_queue_priorities[0];
            queue_create_info.queueCount       = device_queue_families.n_queues    [n_queue_family];
            queue_create_info.queueFamilyIndex = device_queue_families.family_index[n_queue_family];
            queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

            device_queue_create_info_items.push_back(queue_create_info);
        }
    }

    /* Set up the device create info descriptor. Enable all features available. */
    create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size() );
    create_info.enabledLayerCount       = static_cast<uint32_t>(layers.size() );
    create_info.flags                   = 0;
    create_info.pEnabledFeatures        = &features;
    create_info.pNext                   = nullptr;
    create_info.ppEnabledExtensionNames = (extensions.size() > 0) ? &extensions[0] : nullptr;
    create_info.ppEnabledLayerNames     = (layers.size()     > 0) ? &layers    [0] : nullptr;
    create_info.pQueueCreateInfos       = &device_queue_create_info_items[0];
    create_info.queueCreateInfoCount    = static_cast<uint32_t>(device_queue_create_info_items.size() );
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    result = vkCreateDevice(physical_device_locked_ptr->get_physical_device(),
                           &create_info,
                            nullptr, /* pAllocator */
                           &m_device);
    anvil_assert_vk_call_succeeded(result);

    *out_queue_families_ptr = device_queue_families;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Swapchain> Anvil::SGPUDevice::create_swapchain(std::shared_ptr<Anvil::RenderingSurface> parent_surface_ptr,
                                                                      std::shared_ptr<Anvil::Window>           window_ptr,
                                                                      VkFormat                                 image_format,
                                                                      VkPresentModeKHR                         present_mode,
                                                                      VkImageUsageFlags                        usage,
                                                                      uint32_t                                 n_swapchain_images)
{
    std::shared_ptr<Anvil::Swapchain> result_ptr;

    result_ptr = Anvil::Swapchain::create(shared_from_this(),
                                          parent_surface_ptr,
                                          window_ptr,
                                          image_format,
                                          present_mode,
                                          usage,
                                          n_swapchain_images,
                                          m_khr_swapchain_entrypoints);

    return result_ptr;
}

/** Please see header for specification */
void Anvil::SGPUDevice::destroy()
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    /* Release common stuff first */
    Anvil::BaseDevice::destroy();

    /* Unregister the instance from physical device, so that Device instance gets released as soon
     * as all external shared pointers go out of scope.
     */
    physical_device_locked_ptr->unregister_device(shared_from_this() );
}

/** Please see header for specification */
const VkPhysicalDeviceFeatures& Anvil::SGPUDevice::get_physical_device_features() const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    return physical_device_locked_ptr->get_device_features();
}

/** Please see header for specification */
const Anvil::FormatProperties& Anvil::SGPUDevice::get_physical_device_format_properties(VkFormat in_format) const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    return physical_device_locked_ptr->get_format_properties(in_format);
}

/** Please see header for specification */
bool Anvil::SGPUDevice::get_physical_device_image_format_properties(VkFormat                 in_format,
                                                                    VkImageType              in_type,
                                                                    VkImageTiling            in_tiling,
                                                                    VkImageUsageFlags        in_usage,
                                                                    VkImageCreateFlags       in_flags,
                                                                    VkImageFormatProperties& out_result) const
{
    bool result;

    result = is_vk_call_successful(vkGetPhysicalDeviceImageFormatProperties(m_parent_physical_device_ptr.lock()->get_physical_device(),
                                                                            in_format,
                                                                            in_type,
                                                                            in_tiling,
                                                                            in_usage,
                                                                            in_flags,
                                                                           &out_result) );

    return result;
}

/** Please see header for specification */
const Anvil::MemoryProperties& Anvil::SGPUDevice::get_physical_device_memory_properties() const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    return physical_device_locked_ptr->get_memory_properties();
}

/** Please see header for specification */
const VkPhysicalDeviceProperties& Anvil::SGPUDevice::get_physical_device_properties() const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    return physical_device_locked_ptr->get_device_properties();
}

/** Please see header for specification */
const Anvil::QueueFamilyInfoItems& Anvil::SGPUDevice::get_physical_device_queue_families() const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    return physical_device_locked_ptr->get_queue_families();
}

/** Please see header for specification */
bool Anvil::SGPUDevice::get_physical_device_sparse_image_format_properties(VkFormat                                    in_format,
                                                                           VkImageType                                 in_type,
                                                                           VkSampleCountFlagBits                       in_sample_count,
                                                                           VkImageUsageFlags                           in_usage,
                                                                           VkImageTiling                               in_tiling,
                                                                           std::vector<VkSparseImageFormatProperties>& out_result) const
{
    uint32_t n_props = 0;

    vkGetPhysicalDeviceSparseImageFormatProperties(m_parent_physical_device_ptr.lock()->get_physical_device(),
                                                   in_format,
                                                   in_type,
                                                   in_sample_count,
                                                   in_usage,
                                                   in_tiling,
                                                  &n_props,
                                                   nullptr); /* pProperties */

    if (n_props > 0)
    {
        out_result.resize(n_props);

        vkGetPhysicalDeviceSparseImageFormatProperties(m_parent_physical_device_ptr.lock()->get_physical_device(),
                                                       in_format,
                                                       in_type,
                                                       in_sample_count,
                                                       in_usage,
                                                       in_tiling,
                                                      &n_props,
                                                      &out_result[0]);
    }

    return true;
}

/* Please see header for specification */
void Anvil::SGPUDevice::get_queue_family_indices(DeviceQueueFamilyInfo* out_device_queue_family_info_ptr) const
{
    get_queue_family_indices_for_physical_device(m_parent_physical_device_ptr,
                                                 out_device_queue_family_info_ptr);
}

/** Please see header for specification */
const Anvil::QueueFamilyInfo* Anvil::SGPUDevice::get_queue_family_info(uint32_t queue_family_index) const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);
    const auto&                            queue_fams                (physical_device_locked_ptr->get_queue_families() );
    const Anvil::QueueFamilyInfo*          result_ptr                (nullptr);

    if (queue_fams.size() > queue_family_index)
    {
        result_ptr = &queue_fams.at(queue_family_index);
    }

    return result_ptr;
}

/** Register this SGPUDevice with the owning physical device. This is necessary to ensure this device instance
 *  is not released prematurely.
 **/
void Anvil::SGPUDevice::init_device()
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    /* Cache a shared pointer owning this instance in PhysicalDevice. We will release it at destroy() time,
     * having destroyed all children objects we also own and which take a weak pointer to this Device.
     */
    physical_device_locked_ptr->register_device(shared_from_this() );
}

/** Please see header for specification */
bool Anvil::SGPUDevice::is_layer_supported(const char* layer_name) const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    return physical_device_locked_ptr->is_layer_supported(layer_name);
}

/** Please see header for specification */
bool Anvil::SGPUDevice::is_physical_device_extension_supported(const char* extension_name) const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    return physical_device_locked_ptr->is_device_extension_supported(extension_name);
}
