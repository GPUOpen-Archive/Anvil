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
    :m_destroyed               (false),
     m_device                  (VK_NULL_HANDLE),
     m_ext_debug_marker_enabled(false),
     m_parent_instance_ptr     (in_parent_instance_ptr)
{
    std::shared_ptr<Anvil::Instance> instance_locked_ptr(in_parent_instance_ptr);

    m_khr_surface_extension_entrypoints = instance_locked_ptr->get_extension_khr_surface_entrypoints();

    /* Register the instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_DEVICE,
                                                 this);
}

Anvil::BaseDevice::~BaseDevice()
{
    anvil_assert(m_destroyed);

    /* Unregister the instance. Tihs needs to happen before actual Vulkan object destruction, as there might
     * be observers who postpone their destruction until the device is about to go down.
     */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_DEVICE,
                                                    this);

    if (m_device != nullptr)
    {
        vkDeviceWaitIdle(m_device);
        vkDestroyDevice (m_device,
                         nullptr /* pAllocator */);

        m_device = nullptr;
    }
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
    m_queue_fams.clear();

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
uint32_t Anvil::BaseDevice::get_n_queues(uint32_t in_n_queue_family) const
{
    auto     map_iterator = m_queue_fams.find(in_n_queue_family);
    uint32_t result       = 0;

    if (map_iterator != m_queue_fams.end())
    {
        result = static_cast<uint32_t>(map_iterator->second.size() );
    }

    return result;
}

/** Please see header for specification */
std::shared_ptr<Anvil::Queue> Anvil::BaseDevice::get_queue(uint32_t in_n_queue_family,
                                                           uint32_t in_n_queue) const
{
    auto                          map_iterator = m_queue_fams.find(in_n_queue_family);
    std::shared_ptr<Anvil::Queue> result_ptr;

    if (map_iterator != m_queue_fams.end())
    {
        if (map_iterator->second.size() > in_n_queue)
        {
            result_ptr = map_iterator->second.at(in_n_queue);
        }
    }

    return result_ptr;
}

/* Please see header for specification */
void Anvil::BaseDevice::get_queue_family_indices_for_physical_device(std::weak_ptr<Anvil::PhysicalDevice> in_physical_device_ptr,
                                                                     DeviceQueueFamilyInfo*               out_device_queue_family_info_ptr) const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(in_physical_device_ptr);

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

    out_device_queue_family_info_ptr->family_index[Anvil::QUEUE_FAMILY_TYPE_COMPUTE]           = result_compute_queue_family_index;
    out_device_queue_family_info_ptr->family_index[Anvil::QUEUE_FAMILY_TYPE_TRANSFER]          = result_dma_queue_family_index;
    out_device_queue_family_info_ptr->family_index[Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL]         = result_universal_queue_family_index;

    for (uint32_t n_queue_family_type = 0;
                  n_queue_family_type < static_cast<uint32_t>(Anvil::QUEUE_FAMILY_TYPE_COUNT);
                ++n_queue_family_type)
    {
        out_device_queue_family_info_ptr->family_type[n_queue_family_type] = Anvil::QUEUE_FAMILY_TYPE_UNDEFINED;
    }

    if (result_compute_queue_family_index != UINT32_MAX)
    {
        out_device_queue_family_info_ptr->family_type[result_compute_queue_family_index] = Anvil::QUEUE_FAMILY_TYPE_COMPUTE;
    }

    if (result_dma_queue_family_index != UINT32_MAX)
    {
        out_device_queue_family_info_ptr->family_type[result_dma_queue_family_index] = Anvil::QUEUE_FAMILY_TYPE_TRANSFER;
    }

    if (result_universal_queue_family_index != UINT32_MAX)
    {
        out_device_queue_family_info_ptr->family_type[result_universal_queue_family_index] = Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL;
    }

    out_device_queue_family_info_ptr->n_queues[Anvil::QUEUE_FAMILY_TYPE_COMPUTE]   = result_n_compute_queues;
    out_device_queue_family_info_ptr->n_queues[Anvil::QUEUE_FAMILY_TYPE_TRANSFER]  = result_n_dma_queues;
    out_device_queue_family_info_ptr->n_queues[Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL] = result_n_universal_queues;
}

/* Please see header for specification */
bool Anvil::BaseDevice::get_sample_locations(VkSampleCountFlagBits        in_sample_count,
                                             std::vector<SampleLocation>* out_result_ptr) const
{
    bool                        result                            = true;
    std::vector<SampleLocation> sample_locations;
    bool                        standard_sample_locations_support = false;

    switch (get_type() )
    {
        case Anvil::DEVICE_TYPE_SINGLE_GPU:
        {
            const Anvil::SGPUDevice* sgpu_device_ptr = reinterpret_cast<const Anvil::SGPUDevice*>(this);
            anvil_assert(sgpu_device_ptr != nullptr);

            standard_sample_locations_support = (sgpu_device_ptr->get_physical_device_properties().limits.standardSampleLocations == VK_TRUE);
            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }

    if (!standard_sample_locations_support)
    {
        result = false;

        goto end;
    }

    switch (in_sample_count)
    {
        case VK_SAMPLE_COUNT_1_BIT:
        {
            out_result_ptr->clear();

            out_result_ptr->push_back(SampleLocation(0.5f, 0.5f) );

            break;
        }

        case VK_SAMPLE_COUNT_2_BIT:
        {
            out_result_ptr->clear();

            out_result_ptr->push_back(SampleLocation(0.25f, 0.25f) );
            out_result_ptr->push_back(SampleLocation(0.75f, 0.75f) );

            break;

        }

        case VK_SAMPLE_COUNT_4_BIT:
        {
            out_result_ptr->clear();

            out_result_ptr->push_back(SampleLocation(0.375f, 0.125f) );
            out_result_ptr->push_back(SampleLocation(0.875f, 0.375f) );
            out_result_ptr->push_back(SampleLocation(0.125f, 0.625f) );
            out_result_ptr->push_back(SampleLocation(0.625f, 0.875f) );

            break;
        }

        case VK_SAMPLE_COUNT_8_BIT:
        {
            out_result_ptr->clear();

            out_result_ptr->push_back(SampleLocation(0.5625f, 0.3125f) );
            out_result_ptr->push_back(SampleLocation(0.4375f, 0.6875f) );
            out_result_ptr->push_back(SampleLocation(0.8125f, 0.5625f) );
            out_result_ptr->push_back(SampleLocation(0.3125f, 0.1875f) );
            out_result_ptr->push_back(SampleLocation(0.1875f, 0.8125f) );
            out_result_ptr->push_back(SampleLocation(0.0625f, 0.4375f) );
            out_result_ptr->push_back(SampleLocation(0.6875f, 0.9375f) );
            out_result_ptr->push_back(SampleLocation(0.9375f, 0.0625f) );

            break;
        }

        case VK_SAMPLE_COUNT_16_BIT:
        {
            out_result_ptr->clear();

            out_result_ptr->push_back(SampleLocation(0.5625f,  0.5625f) );
            out_result_ptr->push_back(SampleLocation(0.4375f,  0.3125f) );
            out_result_ptr->push_back(SampleLocation(0.3125f,  0.625f)  );
            out_result_ptr->push_back(SampleLocation(0.75f,    0.4375f) );
            out_result_ptr->push_back(SampleLocation(0.1875f,  0.375f)  );
            out_result_ptr->push_back(SampleLocation(0.625f,   0.8125f) );
            out_result_ptr->push_back(SampleLocation(0.8125f,  0.6875f) );
            out_result_ptr->push_back(SampleLocation(0.6875f,  0.1875f) );
            out_result_ptr->push_back(SampleLocation(0.375f,   0.875f)  );
            out_result_ptr->push_back(SampleLocation(0.5f,     0.0625f) );
            out_result_ptr->push_back(SampleLocation(0.25f,    0.125f)  );
            out_result_ptr->push_back(SampleLocation(0.125f,   0.75f)   );
            out_result_ptr->push_back(SampleLocation(0.0f,     0.5f)    );
            out_result_ptr->push_back(SampleLocation(0.9375f,  0.25f)   );
            out_result_ptr->push_back(SampleLocation(0.875f,   0.9375f) );
            out_result_ptr->push_back(SampleLocation(0.0625f,  0.0f)    );

            break;
        }

        default:
        {
            anvil_assert_fail();

            goto end;
        }
    }

end:
    return result;
}

/* Please see header for specification */
std::shared_ptr<Anvil::Queue> Anvil::BaseDevice::get_sparse_binding_queue(uint32_t     in_n_queue,
                                                                          VkQueueFlags in_opt_required_queue_flags) const
{
    uint32_t                      n_queues_found = 0;
    std::shared_ptr<Anvil::Queue> result_ptr;

    for (auto queue_ptr : m_sparse_binding_queues)
    {
        const uint32_t queue_family_index    = queue_ptr->get_queue_family_index();
        const auto&    queue_family_info_ptr = get_queue_family_info            (queue_family_index);

        if ((queue_family_info_ptr->flags & in_opt_required_queue_flags) == in_opt_required_queue_flags)
        {
            if (n_queues_found == in_n_queue)
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
void Anvil::BaseDevice::init(const DeviceExtensionConfiguration& in_extensions,
                             const std::vector<std::string>&     in_layers,
                             bool                                in_transient_command_buffer_allocs_only,
                             bool                                in_support_resettable_command_buffer_allocs)
{

    std::vector<const char*>         extensions_final;
    VkPhysicalDeviceFeatures         features_to_enable;
    std::shared_ptr<Anvil::Instance> instance_locked_ptr  (m_parent_instance_ptr);
    const bool                       is_validation_enabled(instance_locked_ptr->is_validation_enabled() );
    std::vector<const char*>         layers_final;

    /* If validation is enabled, retrieve names of all suported validation layers and
     * append them to the list of layers the user has alreaedy specified. **/
    for (auto current_layer : in_layers)
    {
        layers_final.push_back(current_layer.c_str() );
    }

    if (is_validation_enabled)
    {
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

    /* Go through the extension struct, verify availability of the requested extensions,
     * and cache the ones that have been requested and which are available in a linear vector. */
    typedef struct ExtensionItem
    {
        const char*           name;
        ExtensionAvailability requested_availability;
        bool*                 ext_enabled_flag_ptr;

        ExtensionItem(const char*           in_name,
                      ExtensionAvailability in_availability,
                      bool*                 in_ext_enabled_flag_ptr)
        {
            ext_enabled_flag_ptr   = in_ext_enabled_flag_ptr;
            name                   = in_name;
            requested_availability = in_availability;
        }
    } ExtensionItem;

    std::vector<ExtensionItem> specified_extensions =
    {
        ExtensionItem(VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME,              in_extensions.amd_draw_indirect_count,              &m_amd_draw_indirect_count_enabled),
        ExtensionItem(VK_AMD_GCN_SHADER_EXTENSION_NAME,                       in_extensions.amd_gcn_shader,                       &m_amd_gcn_shader_enabled),
        ExtensionItem(VK_AMD_GPU_SHADER_HALF_FLOAT_EXTENSION_NAME,            in_extensions.amd_gpu_shader_half_float,            &m_amd_gpu_shader_half_float_enabled),
        ExtensionItem(VK_AMD_GPU_SHADER_INT16_EXTENSION_NAME,                 in_extensions.amd_gpu_shader_int16,                 &m_amd_gpu_shader_int16_enabled),
        ExtensionItem(VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME,         in_extensions.amd_negative_viewport_height,         &m_amd_negative_viewport_height_enabled),
        ExtensionItem(VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME,              in_extensions.amd_rasterization_order,              &m_amd_rasterization_order_enabled),
        ExtensionItem(VK_AMD_SHADER_BALLOT_EXTENSION_NAME,                    in_extensions.amd_shader_ballot,                    &m_amd_shader_ballot_enabled),
        ExtensionItem(VK_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER_EXTENSION_NAME, in_extensions.amd_shader_explicit_vertex_parameter, &m_amd_shader_explicit_vertex_parameter_enabled),
        ExtensionItem(VK_AMD_SHADER_TRINARY_MINMAX_EXTENSION_NAME,            in_extensions.amd_shader_trinary_minmax,            &m_amd_shader_trinary_minmax_enabled),
        ExtensionItem(VK_AMD_TEXTURE_GATHER_BIAS_LOD_EXTENSION_NAME,          in_extensions.amd_texture_gather_bias_lod,          &m_amd_texture_gather_bias_lod_enabled),
        ExtensionItem(VK_EXT_DEBUG_MARKER_EXTENSION_NAME,                     in_extensions.ext_debug_marker,                     &m_ext_debug_marker_enabled),
        ExtensionItem(VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME,           in_extensions.ext_shader_subgroup_ballot,           &m_ext_shader_subgroup_ballot_enabled),
        ExtensionItem(VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME,             in_extensions.ext_shader_subgroup_vote,             &m_ext_shader_subgroup_vote_enabled),
        ExtensionItem(VK_KHR_16BIT_STORAGE_EXTENSION_NAME,                    in_extensions.khr_16bit_storage,                    &m_khr_16bit_storage_enabled),
        ExtensionItem(VK_KHR_MAINTENANCE1_EXTENSION_NAME,                     in_extensions.khr_maintenance1,                     &m_khr_maintenance1_enabled),
        ExtensionItem("VK_KHR_storage_buffer_storage_class",                  in_extensions.khr_storage_buffer_storage_class,     &m_khr_storage_buffer_storage_class_enabled),
        ExtensionItem(VK_KHR_SURFACE_EXTENSION_NAME,                          in_extensions.khr_surface,                          &m_khr_surface_enabled),
        ExtensionItem(VK_KHR_SWAPCHAIN_EXTENSION_NAME,                        in_extensions.khr_swapchain,                        &m_khr_swapchain_enabled),
    };

    for (const auto& misc_extension : in_extensions.other_extensions)
    {
        specified_extensions.push_back(
            ExtensionItem(misc_extension.first.c_str(),
                          misc_extension.second,
                          nullptr)
        );
    }

    for (const auto& current_extension : specified_extensions)
    {
        const bool is_ext_supported = is_physical_device_extension_supported(current_extension.name);

        if (current_extension.ext_enabled_flag_ptr != nullptr)
        {
            *current_extension.ext_enabled_flag_ptr = false;
        }

        switch (current_extension.requested_availability)
        {
            case EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE:
            {
                if (is_ext_supported)
                {
                    extensions_final.push_back(current_extension.name);
                }

                break;
            }

            case EXTENSION_AVAILABILITY_IGNORE:
            {
                continue;
            }

            case EXTENSION_AVAILABILITY_REQUIRE:
            {
                if (!is_ext_supported)
                {
                    char temp[1024];

                    if (snprintf(temp,
                                 sizeof(temp),
                                 "Device extension [%s] is unsupported",
                                 current_extension.name) > 0)
                    {
                        fprintf(stderr,
                                "%s",
                                temp);
                    }

                    anvil_assert_fail();
                    continue;
                }

                if (is_ext_supported)
                {
                    extensions_final.push_back(current_extension.name);
                }

                break;
            }

            default:
            {
                anvil_assert_fail();
            }
        }

        if (is_ext_supported)
        {
            m_enabled_extensions.push_back(current_extension.name);
        }

        if (current_extension.ext_enabled_flag_ptr != nullptr)
        {
            *current_extension.ext_enabled_flag_ptr = is_ext_supported;
        }
    }

    if (m_amd_negative_viewport_height_enabled &&
        m_khr_maintenance1_enabled)
    {
        /* VK_AMD_negative_viewport_height and VK_KHR_maintenance1 extensions are mutually exclusive. */
        anvil_assert_fail();
    }

    /* Instantiate the device. Actual behavior behind this is implemented by the overriding class. */
    features_to_enable = get_physical_device_features();

    anvil_assert(m_device == VK_NULL_HANDLE);
    {
        create_device(extensions_final,
                      layers_final,
                      features_to_enable,
                     &m_device_queue_families);
    }
    anvil_assert(m_device != VK_NULL_HANDLE);

    /* Retrieve device-specific func pointers */
    if (m_amd_draw_indirect_count_enabled)
    {
        m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndexedIndirectCountAMD = reinterpret_cast<PFN_vkCmdDrawIndexedIndirectCountAMD>(get_proc_address("vkCmdDrawIndexedIndirectCountAMD") );
        m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndirectCountAMD        = reinterpret_cast<PFN_vkCmdDrawIndirectCountAMD>       (get_proc_address("vkCmdDrawIndirectCountAMD") );

        anvil_assert(m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndexedIndirectCountAMD != nullptr);
        anvil_assert(m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndirectCountAMD        != nullptr);
    }

    if (m_ext_debug_marker_enabled)
    {
        m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerBeginEXT      = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>     (get_proc_address("vkCmdDebugMarkerBeginEXT") );
        m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerEndEXT        = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>       (get_proc_address("vkCmdDebugMarkerEndEXT") );
        m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerInsertEXT     = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>    (get_proc_address("vkCmdDebugMarkerInsertEXT") );
        m_ext_debug_marker_extension_entrypoints.vkDebugMarkerSetObjectNameEXT = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(get_proc_address("vkDebugMarkerSetObjectNameEXT") );
        m_ext_debug_marker_extension_entrypoints.vkDebugMarkerSetObjectTagEXT  = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT> (get_proc_address("vkDebugMarkerSetObjectTagEXT") );

        anvil_assert(m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerBeginEXT      != nullptr);
        anvil_assert(m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerEndEXT        != nullptr);
        anvil_assert(m_ext_debug_marker_extension_entrypoints.vkCmdDebugMarkerInsertEXT     != nullptr);
        anvil_assert(m_ext_debug_marker_extension_entrypoints.vkDebugMarkerSetObjectNameEXT != nullptr);
        anvil_assert(m_ext_debug_marker_extension_entrypoints.vkDebugMarkerSetObjectTagEXT  != nullptr);
    }

    if (m_khr_maintenance1_enabled)
    {
        m_khr_maintenance1_extension_entrypoints.vkTrimCommandPoolKHR = reinterpret_cast<PFN_vkTrimCommandPoolKHR>(get_proc_address("vkTrimCommandPoolKHR") );

        anvil_assert(m_khr_maintenance1_extension_entrypoints.vkTrimCommandPoolKHR != nullptr);
    }

    if (m_khr_swapchain_enabled)
    {
        m_khr_swapchain_extension_entrypoints.vkAcquireNextImageKHR   = reinterpret_cast<PFN_vkAcquireNextImageKHR>  (get_proc_address("vkAcquireNextImageKHR") );
        m_khr_swapchain_extension_entrypoints.vkCreateSwapchainKHR    = reinterpret_cast<PFN_vkCreateSwapchainKHR>   (get_proc_address("vkCreateSwapchainKHR") );
        m_khr_swapchain_extension_entrypoints.vkDestroySwapchainKHR   = reinterpret_cast<PFN_vkDestroySwapchainKHR>  (get_proc_address("vkDestroySwapchainKHR") );
        m_khr_swapchain_extension_entrypoints.vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(get_proc_address("vkGetSwapchainImagesKHR") );
        m_khr_swapchain_extension_entrypoints.vkQueuePresentKHR       = reinterpret_cast<PFN_vkQueuePresentKHR>      (get_proc_address("vkQueuePresentKHR") );

        anvil_assert(m_khr_swapchain_extension_entrypoints.vkAcquireNextImageKHR   != nullptr);
        anvil_assert(m_khr_swapchain_extension_entrypoints.vkCreateSwapchainKHR    != nullptr);
        anvil_assert(m_khr_swapchain_extension_entrypoints.vkDestroySwapchainKHR   != nullptr);
        anvil_assert(m_khr_swapchain_extension_entrypoints.vkGetSwapchainImagesKHR != nullptr);
        anvil_assert(m_khr_swapchain_extension_entrypoints.vkQueuePresentKHR       != nullptr);
    }

    /* Spawn queue wrappers */
    for (Anvil::QueueFamilyType queue_family_type = Anvil::QUEUE_FAMILY_TYPE_FIRST;
                                queue_family_type < Anvil::QUEUE_FAMILY_TYPE_COUNT;
                                queue_family_type = static_cast<Anvil::QueueFamilyType>(queue_family_type + 1))
    {
        decltype(m_queue_fams)::iterator             current_queue_fam_queues_vec_iterator;
        const uint32_t                               n_queues                              = m_device_queue_families.n_queues[queue_family_type];
        std::vector<std::shared_ptr<Anvil::Queue> >& queues                                = (queue_family_type == Anvil::QUEUE_FAMILY_TYPE_COMPUTE)   ? m_compute_queues
                                                                                           : (queue_family_type == Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL) ? m_universal_queues
                                                                                                                                                       : m_transfer_queues;

        if (n_queues == 0)
        {
            continue;
        }

        /* Create a dummy queue vector and instantiate an iterator we're going to fill up with queue instances */
        anvil_assert(m_queue_fams.find(m_device_queue_families.family_index[queue_family_type]) == m_queue_fams.end() );
        m_queue_fams[m_device_queue_families.family_index[queue_family_type] ];

        current_queue_fam_queues_vec_iterator = m_queue_fams.find(m_device_queue_families.family_index[queue_family_type]);

        for (uint32_t n_queue = 0;
                      n_queue < n_queues;
                    ++n_queue)
        {
            std::shared_ptr<Anvil::Queue> new_queue_ptr = Anvil::Queue::create(shared_from_this(),
                                                                               m_device_queue_families.family_index[queue_family_type],
                                                                               n_queue);

            queues.push_back(new_queue_ptr);

            anvil_assert(std::find(current_queue_fam_queues_vec_iterator->second.begin(),
                                   current_queue_fam_queues_vec_iterator->second.end(),
                                   new_queue_ptr) == current_queue_fam_queues_vec_iterator->second.end() );

            current_queue_fam_queues_vec_iterator->second.push_back(new_queue_ptr);

            /* If this queue supports sparse binding ops, cache it in a separate vector as well */
            if (new_queue_ptr->supports_sparse_bindings() )
            {
                m_sparse_binding_queues.push_back(new_queue_ptr);
            }
        }
    }

    /* Instantiate per-queue family command pools */
    for (Anvil::QueueFamilyType queue_family_type = Anvil::QUEUE_FAMILY_TYPE_FIRST;
                                queue_family_type < Anvil::QUEUE_FAMILY_TYPE_COUNT;
                                queue_family_type = static_cast<Anvil::QueueFamilyType>(queue_family_type + 1))
    {
        if (get_queue_family_index(queue_family_type) != UINT32_MAX)
        {
            m_command_pool_ptrs[queue_family_type] = Anvil::CommandPool::create(shared_from_this(),
                                                                                in_transient_command_buffer_allocs_only,
                                                                                in_support_resettable_command_buffer_allocs,
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
Anvil::SGPUDevice::SGPUDevice(std::weak_ptr<Anvil::PhysicalDevice> in_physical_device_ptr)
    :BaseDevice                  (in_physical_device_ptr.lock()->get_instance() ),
     m_parent_physical_device_ptr(in_physical_device_ptr)
{
    /* Stub */
}

/* Please see header for specification */
Anvil::SGPUDevice::~SGPUDevice()
{
    /* Stub */
}


/* Please see header for specification */
std::weak_ptr<Anvil::SGPUDevice> Anvil::SGPUDevice::create(std::weak_ptr<Anvil::PhysicalDevice> in_physical_device_ptr,
                                                           const DeviceExtensionConfiguration&  in_extensions,
                                                           const std::vector<std::string>&      in_layers,
                                                           bool                                 in_transient_command_buffer_allocs_only,
                                                           bool                                 in_support_resettable_command_buffer_allocs)
{
    std::shared_ptr<Anvil::SGPUDevice> result_ptr;

    result_ptr = std::shared_ptr<Anvil::SGPUDevice>(new Anvil::SGPUDevice(in_physical_device_ptr),
                                                    Anvil::SGPUDevice::SGPUDeviceDeleter() );

    result_ptr->init(in_extensions,
                     in_layers,
                     in_transient_command_buffer_allocs_only,
                     in_support_resettable_command_buffer_allocs);

    return result_ptr;
}

/** Please see header for specification */
void Anvil::SGPUDevice::create_device(const std::vector<const char*>& in_extensions,
                                      const std::vector<const char*>& in_layers,
                                      const VkPhysicalDeviceFeatures& in_features,
                                      DeviceQueueFamilyInfo*          out_queue_families_ptr)
{
    VkDeviceCreateInfo                     create_info;
    std::vector<VkDeviceQueueCreateInfo>   device_queue_create_info_items;
    std::vector<float>                     device_queue_priorities;
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);
    const auto&                            physical_device_queue_fams(physical_device_locked_ptr->get_queue_families() );
    VkResult                               result                    (VK_ERROR_INITIALIZATION_FAILED);

    ANVIL_REDUNDANT_VARIABLE(result);

    /* Set up queue create info structure instances.
     *
     * Use up all available queues.
     */
    for (uint32_t n_queue_fam = 0;
                  n_queue_fam < static_cast<uint32_t>(physical_device_queue_fams.size() );
                ++n_queue_fam)
    {
        const auto& current_queue_fam(physical_device_queue_fams.at(n_queue_fam) );

        if (device_queue_priorities.size() < current_queue_fam.n_queues)
        {
            device_queue_priorities.resize(current_queue_fam.n_queues,
                                           1.0f);
        }
    }

    for (uint32_t n_queue_fam = 0;
                  n_queue_fam < static_cast<uint32_t>(physical_device_queue_fams.size() );
                ++n_queue_fam)
    {
        const auto& current_queue_fam(physical_device_queue_fams.at(n_queue_fam) );

        if (current_queue_fam.n_queues > 0)
        {
            VkDeviceQueueCreateInfo queue_create_info;

            queue_create_info.flags            = 0;
            queue_create_info.pNext            = nullptr;
            queue_create_info.pQueuePriorities = &device_queue_priorities.at(0);
            queue_create_info.queueCount       = current_queue_fam.n_queues;
            queue_create_info.queueFamilyIndex = n_queue_fam;
            queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

            device_queue_create_info_items.push_back(queue_create_info);
        }
    }

    /* Set up the device create info descriptor. Enable all features available. */
    create_info.enabledExtensionCount   = static_cast<uint32_t>(in_extensions.size() );
    create_info.enabledLayerCount       = static_cast<uint32_t>(in_layers.size() );
    create_info.flags                   = 0;
    create_info.pEnabledFeatures        = &in_features;
    create_info.pNext                   = nullptr;
    create_info.ppEnabledExtensionNames = (in_extensions.size() > 0) ? &in_extensions[0] : nullptr;
    create_info.ppEnabledLayerNames     = (in_layers.size()     > 0) ? &in_layers    [0] : nullptr;
    create_info.pQueueCreateInfos       = &device_queue_create_info_items[0];
    create_info.queueCreateInfoCount    = static_cast<uint32_t>(device_queue_create_info_items.size() );
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    result = vkCreateDevice(m_parent_physical_device_ptr.lock()->get_physical_device(),
                           &create_info,
                            nullptr, /* pAllocator */
                           &m_device);
    anvil_assert_vk_call_succeeded(result);

    /* Now that all queues are available, assign them to queue families Anvil recognizes. */
    get_queue_family_indices(out_queue_families_ptr);
}

/** Please see header for specification */
std::shared_ptr<Anvil::Swapchain> Anvil::SGPUDevice::create_swapchain(std::shared_ptr<Anvil::RenderingSurface> in_parent_surface_ptr,
                                                                      std::shared_ptr<Anvil::Window>           in_window_ptr,
                                                                      VkFormat                                 in_image_format,
                                                                      VkPresentModeKHR                         in_present_mode,
                                                                      VkImageUsageFlags                        in_usage,
                                                                      uint32_t                                 in_n_swapchain_images)
{
    std::shared_ptr<Anvil::Swapchain> result_ptr;

    result_ptr = Anvil::Swapchain::create(shared_from_this(),
                                          in_parent_surface_ptr,
                                          in_window_ptr,
                                          in_image_format,
                                          in_present_mode,
                                          in_usage,
                                          in_n_swapchain_images,
                                          m_khr_swapchain_extension_entrypoints);

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
const Anvil::QueueFamilyInfo* Anvil::SGPUDevice::get_queue_family_info(uint32_t in_queue_family_index) const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);
    const auto&                            queue_fams                (physical_device_locked_ptr->get_queue_families() );
    const Anvil::QueueFamilyInfo*          result_ptr                (nullptr);

    if (queue_fams.size() > in_queue_family_index)
    {
        result_ptr = &queue_fams.at(in_queue_family_index);
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
bool Anvil::SGPUDevice::is_layer_supported(const std::string& in_layer_name) const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    return physical_device_locked_ptr->is_layer_supported(in_layer_name);
}

/** Please see header for specification */
bool Anvil::SGPUDevice::is_physical_device_extension_supported(const std::string& in_extension_name) const
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_locked_ptr(m_parent_physical_device_ptr);

    return physical_device_locked_ptr->is_device_extension_supported(in_extension_name);
}
