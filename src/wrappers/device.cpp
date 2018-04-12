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
#include "misc/object_tracker.h"
#include "misc/shader_module_cache.h"
#include "misc/struct_chainer.h"
#include "misc/swapchain_create_info.h"
#include "wrappers/command_pool.h"
#include "wrappers/compute_pipeline_manager.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/descriptor_set_layout_manager.h"
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
Anvil::BaseDevice::BaseDevice(const Anvil::Instance* in_parent_instance_ptr,
                              bool                   in_mt_safe)
    :MTSafetySupportProvider(in_mt_safe),
     m_device               (VK_NULL_HANDLE),
     m_parent_instance_ptr  (in_parent_instance_ptr)
{
    m_khr_surface_extension_entrypoints = m_parent_instance_ptr->get_extension_khr_surface_entrypoints();

    /* Register the instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_DEVICE,
                                                 this);
}

Anvil::BaseDevice::~BaseDevice()
{
    /* Unregister the instance. Tihs needs to happen before actual Vulkan object destruction, as there might
     * be observers who postpone their destruction until the device is about to go down.
     */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_DEVICE,
                                                    this);

    if (m_device != VK_NULL_HANDLE)
    {
        wait_idle();
    }

    m_command_pool_ptr_per_vk_queue_fam.clear();
    m_compute_pipeline_manager_ptr.reset     ();
    m_descriptor_set_layout_manager_ptr.reset();
    m_dummy_dsg_ptr.reset                    ();
    m_graphics_pipeline_manager_ptr.reset    ();
    m_pipeline_cache_ptr.reset               ();
    m_pipeline_layout_manager_ptr.reset      ();
    m_owned_queues.clear                     ();

    if (m_device != VK_NULL_HANDLE)
    {
        lock();
        {
            vkDestroyDevice(m_device,
                            nullptr); /* pAllocator */
        }
        unlock();

        m_device = nullptr;
    }
}

/** Please see header for specification */
const Anvil::DescriptorSet* Anvil::BaseDevice::get_dummy_descriptor_set() const
{
    return m_dummy_dsg_ptr->get_descriptor_set(0);
}

/** Please see header for specification */
Anvil::DescriptorSetLayout* Anvil::BaseDevice::get_dummy_descriptor_set_layout() const
{
    return m_dummy_dsg_ptr->get_descriptor_set_layout(0);
}

/** Please see header for specification */
uint32_t Anvil::BaseDevice::get_n_queues(uint32_t in_n_queue_family) const
{
    auto     map_iterator = m_queue_ptrs_per_vk_queue_fam.find(in_n_queue_family);
    uint32_t result       = 0;

    if (map_iterator != m_queue_ptrs_per_vk_queue_fam.end())
    {
        result = static_cast<uint32_t>(map_iterator->second.size() );
    }

    return result;
}

/* Please see header for specification */
std::unique_ptr<Anvil::StructChain<VkPhysicalDeviceFeatures2KHR > > Anvil::BaseDevice::get_physical_device_features_chain(const VkPhysicalDeviceFeatures* in_opt_features_ptr) const
{
    const auto&                                                           features           = get_physical_device_features();
    std::unique_ptr<Anvil::StructChainer<VkPhysicalDeviceFeatures2KHR > > struct_chainer_ptr;

    struct_chainer_ptr.reset(
        new Anvil::StructChainer<VkPhysicalDeviceFeatures2KHR>()
    );

    {
        VkPhysicalDeviceFeatures2KHR features_khr;

        features_khr.features = (in_opt_features_ptr != nullptr) ? *in_opt_features_ptr
                                                                 : features.core_vk1_0_features_ptr->get_vk_physical_device_features();
        features_khr.pNext    = nullptr;
        features_khr.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;

        struct_chainer_ptr->append_struct(features_khr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_descriptor_indexing() )
    {
        struct_chainer_ptr->append_struct(features.ext_descriptor_indexing_features_ptr->get_vk_physical_device_descriptor_indexing_features() );
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_16bit_storage() )
    {
        struct_chainer_ptr->append_struct(features.khr_16bit_storage_features_ptr->get_vk_physical_device_16_bit_storage_features() );
    }

    return struct_chainer_ptr->create_chain();
}

/* Please see header for specification */
Anvil::Queue* Anvil::BaseDevice::get_queue(const Anvil::QueueFamilyType& in_queue_family_type,
                                           uint32_t                      in_n_queue) const
{
    Anvil::Queue* result_ptr = nullptr;

    switch (in_queue_family_type)
    {
        case Anvil::QueueFamilyType::COMPUTE:   result_ptr = get_compute_queue     (in_n_queue); break;
        case Anvil::QueueFamilyType::TRANSFER:  result_ptr = get_transfer_queue    (in_n_queue); break;
        case Anvil::QueueFamilyType::UNIVERSAL: result_ptr = get_universal_queue   (in_n_queue); break;

        default:
        {
            anvil_assert_fail();
        }
    }

    return result_ptr;
}

/* Please see header for specification */
void Anvil::BaseDevice::get_queue_family_indices_for_physical_device(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                                                     DeviceQueueFamilyInfo*       out_device_queue_family_info_ptr) const
{
    const auto                               n_queue_families                   = static_cast<uint32_t>(in_physical_device_ptr->get_queue_families().size() );
    std::vector<DeviceQueueFamilyMemberInfo> result_compute_queue_families;
    std::vector<DeviceQueueFamilyMemberInfo> result_transfer_queue_families;
    std::vector<DeviceQueueFamilyMemberInfo> result_universal_queue_families;
    
    for (uint32_t n_iteration = 0;
                  n_iteration < 3;
                ++n_iteration)
    {
        for (uint32_t n_queue_family_index = 0;
                      n_queue_family_index < n_queue_families;
                    ++n_queue_family_index)
        {
            const Anvil::QueueFamilyInfo& current_queue_family = in_physical_device_ptr->get_queue_families()[n_queue_family_index];

            if (n_iteration == 0)
            {
                if ( (current_queue_family.flags & VK_QUEUE_COMPUTE_BIT)  &&
                    !(current_queue_family.flags & VK_QUEUE_GRAPHICS_BIT) )
                {
                    result_compute_queue_families.push_back(
                        DeviceQueueFamilyMemberInfo(n_queue_family_index,
                                                    current_queue_family.n_queues)
                    );
                }
            }
            else
            if (n_iteration == 1)
            {
                if (current_queue_family.flags & VK_QUEUE_GRAPHICS_BIT)
                {
                    if (std::find(result_compute_queue_families.begin(),
                                  result_compute_queue_families.end  (),
                                  n_queue_family_index) == result_compute_queue_families.end() )
                    {
                        result_universal_queue_families.push_back(
                            DeviceQueueFamilyMemberInfo(n_queue_family_index,
                                                        current_queue_family.n_queues)
                        );
                    }
                }
            }
            else
            if (n_iteration == 2)
            {
                if (current_queue_family.flags & VK_QUEUE_TRANSFER_BIT)
                {
                    if (std::find(result_compute_queue_families.begin(),
                                  result_compute_queue_families.end  (),
                                  n_queue_family_index) == result_compute_queue_families.end()   &&
                        std::find(result_universal_queue_families.begin(),
                                  result_universal_queue_families.end  (),
                                  n_queue_family_index) == result_universal_queue_families.end() )
                    {
                        result_transfer_queue_families.push_back(
                            DeviceQueueFamilyMemberInfo(n_queue_family_index,
                                                        current_queue_family.n_queues)
                        );
                    }
                }
            }
        }
    }

    /* NOTE: Vulkan API only guarantees universal queue family's availability */
    anvil_assert(result_universal_queue_families.size() > 0);

    out_device_queue_family_info_ptr->queue_families[Anvil::QueueFamilyType::COMPUTE]   = result_compute_queue_families;
    out_device_queue_family_info_ptr->queue_families[Anvil::QueueFamilyType::TRANSFER]  = result_transfer_queue_families;
    out_device_queue_family_info_ptr->queue_families[Anvil::QueueFamilyType::UNIVERSAL] = result_universal_queue_families;

    for (Anvil::QueueFamilyType current_queue_family_type  = Anvil::QueueFamilyType::FIRST;
                                current_queue_family_type != Anvil::QueueFamilyType::COUNT;
                                current_queue_family_type  = static_cast<Anvil::QueueFamilyType>(static_cast<uint32_t>(current_queue_family_type) + 1) )
    {
        uint32_t n_total_queues = 0;

        for (const auto& current_queue_fam : out_device_queue_family_info_ptr->queue_families[current_queue_family_type])
        {
            n_total_queues += current_queue_fam.n_queues;
        }

        out_device_queue_family_info_ptr->n_total_queues_per_family[static_cast<uint32_t>(current_queue_family_type)] = n_total_queues;
    }
}

/** Please see header for specification */
bool Anvil::BaseDevice::get_queue_family_indices_for_queue_family_type(const Anvil::QueueFamilyType& in_queue_family_type,
                                                                       uint32_t*                     out_opt_n_queue_family_indices_ptr,
                                                                       const uint32_t**              out_opt_queue_family_indices_ptr_ptr) const
{
    bool            result                          = false;
    uint32_t        result_n_queue_family_indices   = 0;
    const uint32_t* result_queue_family_indices_ptr = nullptr;

    auto map_iterator = m_queue_family_type_to_queue_family_indices.find(in_queue_family_type);

    if (map_iterator != m_queue_family_type_to_queue_family_indices.end() )
    {
        result                          = true;
        result_n_queue_family_indices   = static_cast<uint32_t>(map_iterator->second.size() );
        result_queue_family_indices_ptr = (map_iterator->second.size() > 0) ? &map_iterator->second.at(0)
                                                                            : nullptr;
    }
    else
    {
        result = true;
    }

    if (out_opt_n_queue_family_indices_ptr != nullptr)
    {
        *out_opt_n_queue_family_indices_ptr = result_n_queue_family_indices;
    }

    if (out_opt_queue_family_indices_ptr_ptr != nullptr)
    {
        *out_opt_queue_family_indices_ptr_ptr = result_queue_family_indices_ptr;
    }

    return result;
}

/** Please see header for specification */
Anvil::QueueFamilyType Anvil::BaseDevice::get_queue_family_type(uint32_t in_queue_family_index) const
{
    return m_queue_family_index_to_type.at(in_queue_family_index);
}

/** Please see header for specification */
Anvil::Queue* Anvil::BaseDevice::get_queue_for_queue_family_index(uint32_t in_n_queue_family,
                                                                  uint32_t in_n_queue) const
{
    auto          map_iterator = m_queue_ptrs_per_vk_queue_fam.find(in_n_queue_family);
    Anvil::Queue* result_ptr   = nullptr;

    if (map_iterator != m_queue_ptrs_per_vk_queue_fam.end())
    {
        if (map_iterator->second.size() > in_n_queue)
        {
            result_ptr = map_iterator->second.at(in_n_queue);
        }
    }

    return result_ptr;
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

            standard_sample_locations_support = (sgpu_device_ptr->get_physical_device_properties().core_vk1_0_properties_ptr->limits.standard_sample_locations);
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
Anvil::Queue* Anvil::BaseDevice::get_sparse_binding_queue(uint32_t     in_n_queue,
                                                          VkQueueFlags in_opt_required_queue_flags) const
{
    uint32_t      n_queues_found = 0;
    Anvil::Queue* result_ptr     = nullptr;

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
                             bool                                in_support_resettable_command_buffer_allocs,
                             bool                                in_enable_shader_module_cache)
{
    std::map<std::string, bool> extensions_final_enabled_status;
    VkPhysicalDeviceFeatures    features_to_enable;
    const bool                  is_validation_enabled(m_parent_instance_ptr->is_validation_enabled() );
    std::vector<const char*>    layers_final;
    const auto                  mt_safety            (Anvil::Utils::convert_boolean_to_mt_safety_enum(is_mt_safe()) );

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
    {
        bool is_amd_negative_viewport_height_defined = false;
        bool is_khr_maintenance1_defined             = false;

        for (const auto& current_extension : in_extensions.extension_status)
        {
            const bool is_ext_supported = is_physical_device_extension_supported(current_extension.first);

            is_amd_negative_viewport_height_defined |= (current_extension.first  == VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME &&
                                                        current_extension.second != EXTENSION_AVAILABILITY_IGNORE);
            is_khr_maintenance1_defined             |= (current_extension.first  == VK_KHR_MAINTENANCE1_EXTENSION_NAME              &&
                                                        current_extension.second != EXTENSION_AVAILABILITY_IGNORE);

            switch (current_extension.second)
            {
                case EXTENSION_AVAILABILITY_ENABLE_IF_AVAILABLE:
                {
                    extensions_final_enabled_status[current_extension.first] = is_ext_supported;

                    break;
                }

                case EXTENSION_AVAILABILITY_IGNORE:
                {
                    extensions_final_enabled_status[current_extension.first] = false;

                    break;
                }

                case EXTENSION_AVAILABILITY_REQUIRE:
                {
                    if (!is_ext_supported)
                    {
                        char temp[1024];

                        if (snprintf(temp,
                                     sizeof(temp),
                                     "Device extension [%s] is unsupported",
                                     current_extension.first.c_str() ) > 0)
                        {
                            fprintf(stderr,
                                    "%s",
                                    temp);
                        }

                        anvil_assert_fail();
                    }

                    extensions_final_enabled_status[current_extension.first.c_str()] = is_ext_supported;

                    break;
                }

                default:
                {
                    anvil_assert_fail();
                }
            }
        }

        if (is_amd_negative_viewport_height_defined &&
            is_khr_maintenance1_defined)
        {
            /* VK_AMD_negative_viewport_height and VK_KHR_maintenance1 extensions are mutually exclusive. */
            anvil_assert_fail();
        }

        m_extension_enabled_info_ptr = Anvil::ExtensionInfo<bool>::create_device_extension_info(extensions_final_enabled_status,
                                                                                                true); /* in_unspecified_extension_name_value */
    }

    /* Instantiate the device. Actual behavior behind this is implemented by the overriding class. */
    features_to_enable = get_physical_device_features().core_vk1_0_features_ptr->get_vk_physical_device_features();

    anvil_assert(m_device == VK_NULL_HANDLE);
    {
        std::vector<const char*> extension_name_raw_ptrs;

        extension_name_raw_ptrs.reserve(extensions_final_enabled_status.size() );

        for (const auto& current_extension_data : extensions_final_enabled_status)
        {
            if (current_extension_data.second)
            {
                extension_name_raw_ptrs.push_back(current_extension_data.first.c_str() );
            }
        }

        create_device(extension_name_raw_ptrs,
                      layers_final,
                      features_to_enable,
                     &m_device_queue_families);
    }
    anvil_assert(m_device != VK_NULL_HANDLE);

    if (m_extension_enabled_info_ptr->get_device_extension_info()->amd_draw_indirect_count() )
    {
        m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndexedIndirectCountAMD = reinterpret_cast<PFN_vkCmdDrawIndexedIndirectCountAMD>(get_proc_address("vkCmdDrawIndexedIndirectCountAMD") );
        m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndirectCountAMD        = reinterpret_cast<PFN_vkCmdDrawIndirectCountAMD>       (get_proc_address("vkCmdDrawIndirectCountAMD") );

        anvil_assert(m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndexedIndirectCountAMD != nullptr);
        anvil_assert(m_amd_draw_indirect_count_extension_entrypoints.vkCmdDrawIndirectCountAMD        != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->amd_shader_info() )
    {
        m_amd_shader_info_extension_entrypoints.vkGetShaderInfoAMD = reinterpret_cast<PFN_vkGetShaderInfoAMD>(get_proc_address("vkGetShaderInfoAMD") );

        anvil_assert(m_amd_shader_info_extension_entrypoints.vkGetShaderInfoAMD != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->ext_debug_marker() )
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

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_descriptor_update_template() )
    {
        m_khr_descriptor_update_template_extension_entrypoints.vkCreateDescriptorUpdateTemplateKHR  = reinterpret_cast<PFN_vkCreateDescriptorUpdateTemplateKHR> (get_proc_address("vkCreateDescriptorUpdateTemplateKHR") );
        m_khr_descriptor_update_template_extension_entrypoints.vkDestroyDescriptorUpdateTemplateKHR = reinterpret_cast<PFN_vkDestroyDescriptorUpdateTemplateKHR>(get_proc_address("vkDestroyDescriptorUpdateTemplateKHR") );
        m_khr_descriptor_update_template_extension_entrypoints.vkUpdateDescriptorSetWithTemplateKHR = reinterpret_cast<PFN_vkUpdateDescriptorSetWithTemplateKHR>(get_proc_address("vkUpdateDescriptorSetWithTemplateKHR") );

        anvil_assert(m_khr_descriptor_update_template_extension_entrypoints.vkCreateDescriptorUpdateTemplateKHR  != nullptr);
        anvil_assert(m_khr_descriptor_update_template_extension_entrypoints.vkDestroyDescriptorUpdateTemplateKHR != nullptr);
        anvil_assert(m_khr_descriptor_update_template_extension_entrypoints.vkUpdateDescriptorSetWithTemplateKHR != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_bind_memory2() )
    {
        m_khr_bind_memory2_extension_entrypoints.vkBindBufferMemory2KHR = reinterpret_cast<PFN_vkBindBufferMemory2KHR>(get_proc_address("vkBindBufferMemory2KHR"));
        m_khr_bind_memory2_extension_entrypoints.vkBindImageMemory2KHR  = reinterpret_cast<PFN_vkBindImageMemory2KHR> (get_proc_address("vkBindImageMemory2KHR"));

        anvil_assert(m_khr_bind_memory2_extension_entrypoints.vkBindBufferMemory2KHR != nullptr);
        anvil_assert(m_khr_bind_memory2_extension_entrypoints.vkBindImageMemory2KHR  != nullptr);
    }


    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_maintenance1() )
    {
        m_khr_maintenance1_extension_entrypoints.vkTrimCommandPoolKHR = reinterpret_cast<PFN_vkTrimCommandPoolKHR>(get_proc_address("vkTrimCommandPoolKHR") );

        anvil_assert(m_khr_maintenance1_extension_entrypoints.vkTrimCommandPoolKHR != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_maintenance3() )
    {
        m_khr_maintenance3_extension_entrypoints.vkGetDescriptorSetLayoutSupportKHR = reinterpret_cast<PFN_vkGetDescriptorSetLayoutSupportKHR>(get_proc_address("vkGetDescriptorSetLayoutSupportKHR") );

        anvil_assert(m_khr_maintenance3_extension_entrypoints.vkGetDescriptorSetLayoutSupportKHR != nullptr);
    }

    if (m_extension_enabled_info_ptr->get_device_extension_info()->khr_swapchain() )
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
    for (Anvil::QueueFamilyType queue_family_type = Anvil::QueueFamilyType::FIRST;
                                queue_family_type < Anvil::QueueFamilyType::COUNT;
                                queue_family_type = static_cast<Anvil::QueueFamilyType>(static_cast<uint32_t>(queue_family_type) + 1))
    {
        const uint32_t              n_queues       = m_device_queue_families.n_total_queues_per_family[static_cast<uint32_t>(queue_family_type)];
        std::vector<Anvil::Queue*>* out_queues_ptr = nullptr;

        if (n_queues == 0)
        {
            continue;
        }

        switch (queue_family_type)
        {
            case Anvil::QueueFamilyType::COMPUTE:   out_queues_ptr = &m_compute_queues;   break;
            case Anvil::QueueFamilyType::TRANSFER:  out_queues_ptr = &m_transfer_queues;  break;
            case Anvil::QueueFamilyType::UNIVERSAL: out_queues_ptr = &m_universal_queues; break;

            default:
            {
                anvil_assert_fail();
            }
        }

        for (const auto& current_queue_fam : m_device_queue_families.queue_families[queue_family_type])
        {
            auto queue_ptr_storage_ptr = &m_queue_ptrs_per_vk_queue_fam[current_queue_fam.family_index];

            anvil_assert(m_queue_family_index_to_type.find(current_queue_fam.family_index)                 == m_queue_family_index_to_type.end() );
            anvil_assert(std::find(m_queue_family_type_to_queue_family_indices[queue_family_type].begin(),
                                   m_queue_family_type_to_queue_family_indices[queue_family_type].end(),
                                   current_queue_fam.family_index)                                         == m_queue_family_type_to_queue_family_indices[queue_family_type].end() );

            m_queue_family_index_to_type               [current_queue_fam.family_index] = queue_family_type;
            m_queue_family_type_to_queue_family_indices[queue_family_type].push_back(current_queue_fam.family_index);

            for (uint32_t n_queue = 0;
                          n_queue < current_queue_fam.n_queues;
                        ++n_queue)
            {
                std::unique_ptr<Anvil::Queue> new_queue_ptr = Anvil::Queue::create(this,
                                                                                   current_queue_fam.family_index,
                                                                                   n_queue,
                                                                                   is_mt_safe() );

                anvil_assert(out_queues_ptr != nullptr);

                out_queues_ptr->push_back(new_queue_ptr.get() );

                /* If this queue supports sparse binding ops, cache it in a separate vector as well */
                if (new_queue_ptr->supports_sparse_bindings() )
                {
                    m_sparse_binding_queues.push_back(new_queue_ptr.get() );
                }

                /* Cache the queue in general-purpose storage as well */
                if (std::find(queue_ptr_storage_ptr->cbegin(),
                              queue_ptr_storage_ptr->cend  (),
                              new_queue_ptr.get() ) == queue_ptr_storage_ptr->cend() )
                {
                    queue_ptr_storage_ptr->push_back(new_queue_ptr.get() );
                }

                m_owned_queues.push_back(
                    std::move(new_queue_ptr)
                );
            }
        }
    }

    /* Instantiate per-queue family command pools */
    m_command_pool_ptr_per_vk_queue_fam.resize(m_device_queue_families.queue_families.size() );

    for (const auto& current_queue_fam : m_device_queue_families.queue_families)
    {
        for (const auto& current_queue_fam_queue : current_queue_fam.second)
        {
            if (m_command_pool_ptr_per_vk_queue_fam.size() <= current_queue_fam_queue.family_index)
            {
                m_command_pool_ptr_per_vk_queue_fam.resize(current_queue_fam_queue.family_index + 1);
            }

            if (m_command_pool_ptr_per_vk_queue_fam[current_queue_fam_queue.family_index] == nullptr)
            {
                m_command_pool_ptr_per_vk_queue_fam[current_queue_fam_queue.family_index] =
                    Anvil::CommandPool::create(this,
                                               in_transient_command_buffer_allocs_only,
                                               in_support_resettable_command_buffer_allocs,
                                               current_queue_fam_queue.family_index,
                                               mt_safety);
            }
        }
    }

    /* Initialize a dummy descriptor set group */
    {
        std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> dummy_ds_create_info_ptrs(1);
        std::vector<Anvil::OverheadAllocation>               dummy_overhead_allocs;

        dummy_overhead_allocs.push_back(
            Anvil::OverheadAllocation(VK_DESCRIPTOR_TYPE_SAMPLER,
                                      1) /* in_n_overhead_allocs */
        );

        dummy_ds_create_info_ptrs[0] = Anvil::DescriptorSetCreateInfo::create();
        dummy_ds_create_info_ptrs[0]->add_binding(0, /* n_binding */
                                                 VK_DESCRIPTOR_TYPE_MAX_ENUM,
                                                 0,  /* n_elements  */
                                                 0); /* stage_flags */

        m_dummy_dsg_ptr = Anvil::DescriptorSetGroup::create(this,
                                                            dummy_ds_create_info_ptrs,
                                                            false, /* releaseable_sets */
                                                            MT_SAFETY_DISABLED,
                                                            dummy_overhead_allocs);

        m_dummy_dsg_ptr->get_descriptor_set(0)->update();
    }

    /* Set up shader module cache, if one was requested. */
    if (in_enable_shader_module_cache)
    {
        m_shader_module_cache_ptr = Anvil::ShaderModuleCache::create();
    }

    /* Set up the pipeline cache */
    m_pipeline_cache_ptr = Anvil::PipelineCache::create(this,
                                                        is_mt_safe() );

    /* Cache a pipeline layout manager instance. */
    m_pipeline_layout_manager_ptr = Anvil::PipelineLayoutManager::create(this,
                                                                         is_mt_safe() );

    /* Cache a descriptor set layout manager. */
    m_descriptor_set_layout_manager_ptr = Anvil::DescriptorSetLayoutManager::create(this,
                                                                                    is_mt_safe() );

    /* Initialize compute & graphics pipeline managers */
    m_compute_pipeline_manager_ptr  = Anvil::ComputePipelineManager::create (this,
                                                                             is_mt_safe() ,
                                                                             true /* use_pipeline_cache */,
                                                                             m_pipeline_cache_ptr.get() );
    m_graphics_pipeline_manager_ptr = Anvil::GraphicsPipelineManager::create(this,
                                                                             is_mt_safe() ,
                                                                             true /* use_pipeline_cache */,
                                                                             m_pipeline_cache_ptr.get() );

    /* Continue with specialized initialization */
    init_device();
}

/** Please see header for specification */
bool Anvil::BaseDevice::is_compute_queue_family_index(const uint32_t& in_queue_family_index) const
{
    return (m_queue_family_index_to_type.find(in_queue_family_index) != m_queue_family_index_to_type.end()) &&
           (m_queue_family_index_to_type.at  (in_queue_family_index) == Anvil::QueueFamilyType::COMPUTE);
}

/** Please see header for specification */
bool Anvil::BaseDevice::is_transfer_queue_family_index(const uint32_t& in_queue_family_index) const
{
    return (m_queue_family_index_to_type.find(in_queue_family_index) != m_queue_family_index_to_type.end()) &&
           (m_queue_family_index_to_type.at  (in_queue_family_index) == Anvil::QueueFamilyType::TRANSFER);
}

/** Please see header for specification */
bool Anvil::BaseDevice::is_universal_queue_family_index(const uint32_t& in_queue_family_index) const
{
    return (m_queue_family_index_to_type.find(in_queue_family_index) != m_queue_family_index_to_type.end() ) &&
           (m_queue_family_index_to_type.at  (in_queue_family_index) == Anvil::QueueFamilyType::UNIVERSAL);
}

bool Anvil::BaseDevice::supports_external_memory_handles(const Anvil::ExternalMemoryHandleTypeFlags& in_types) const
{
    bool result = m_extension_enabled_info_ptr->get_device_extension_info()->khr_external_memory();

    /* NOTE: Anvil does not support any external memory handle types YET. */
    result &= (in_types == 0);

    return result;
}

/* Please see header for specification */
bool Anvil::BaseDevice::wait_idle() const
{
    const bool mt_safe = is_mt_safe();

    VkResult result_vk;

    if (mt_safe)
    {
        for (const auto& queue_fam : m_queue_ptrs_per_vk_queue_fam)
        {
            for (const auto& queue_ptr : queue_fam.second)
            {
                queue_ptr->lock();
            }
        }
    }

    result_vk = vkDeviceWaitIdle(m_device);

    if (mt_safe)
    {
        for (const auto& queue_fam : m_queue_ptrs_per_vk_queue_fam)
        {
            for (const auto& queue_ptr : queue_fam.second)
            {
                queue_ptr->unlock();
            }
        }
    }

    return is_vk_call_successful(result_vk);
}

/* Please see header for specification */
Anvil::SGPUDevice::SGPUDevice(const Anvil::PhysicalDevice* in_physical_device_ptr,
                              bool                         in_mt_safe)
    :BaseDevice                  (in_physical_device_ptr->get_instance(),
                                  in_mt_safe),
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
Anvil::SGPUDeviceUniquePtr Anvil::SGPUDevice::create(const Anvil::PhysicalDevice*         in_physical_device_ptr,
                                                     bool                                 in_enable_shader_module_cache,
                                                     const DeviceExtensionConfiguration&  in_extensions,
                                                     const std::vector<std::string>&      in_layers,
                                                     bool                                 in_transient_command_buffer_allocs_only,
                                                     bool                                 in_support_resettable_command_buffer_allocs,
                                                     bool                                 in_mt_safe)
{
    SGPUDeviceUniquePtr result_ptr(nullptr,
                                   std::default_delete<Anvil::SGPUDevice>() );

    result_ptr = std::unique_ptr<Anvil::SGPUDevice>(
        new Anvil::SGPUDevice(in_physical_device_ptr,
                               in_mt_safe)
    );

    result_ptr->init(in_extensions,
                     in_layers,
                     in_transient_command_buffer_allocs_only,
                     in_support_resettable_command_buffer_allocs,
                     in_enable_shader_module_cache);

    return result_ptr;
}

/** Please see header for specification */
void Anvil::SGPUDevice::create_device(const std::vector<const char*>& in_extensions,
                                      const std::vector<const char*>& in_layers,
                                      const VkPhysicalDeviceFeatures& in_features,
                                      DeviceQueueFamilyInfo*          out_queue_families_ptr)
{
    VkDeviceCreateInfo                   create_info;
    std::vector<VkDeviceQueueCreateInfo> device_queue_create_info_items;
    std::vector<float>                   device_queue_priorities;
    auto                                 features_chain_ptr            (get_physical_device_features_chain              (&in_features) );
    auto                                 features_chain_root_struct_ptr(features_chain_ptr->get_root_struct             () );
    const auto&                          physical_device_queue_fams    (m_parent_physical_device_ptr->get_queue_families() );
    VkResult                             result                        (VK_ERROR_INITIALIZATION_FAILED);

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
    create_info.pEnabledFeatures        = nullptr;
    create_info.pNext                   = (m_parent_physical_device_ptr->get_instance()->get_enabled_extensions_info()->khr_get_physical_device_properties2() ) ? features_chain_root_struct_ptr
                                                                                                                                                                : features_chain_root_struct_ptr->pNext;
    create_info.ppEnabledExtensionNames = (in_extensions.size() > 0) ? &in_extensions[0] : nullptr;
    create_info.ppEnabledLayerNames     = (in_layers.size()     > 0) ? &in_layers    [0] : nullptr;
    create_info.pQueueCreateInfos       = &device_queue_create_info_items[0];
    create_info.queueCreateInfoCount    = static_cast<uint32_t>(device_queue_create_info_items.size() );
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    result = vkCreateDevice(m_parent_physical_device_ptr->get_physical_device(),
                           &create_info,
                            nullptr, /* pAllocator */
                           &m_device);
    anvil_assert_vk_call_succeeded(result);

    /* Now that all queues are available, assign them to queue families Anvil recognizes. */
    get_queue_family_indices(out_queue_families_ptr);
}

/** Please see header for specification */
Anvil::SwapchainUniquePtr Anvil::SGPUDevice::create_swapchain(Anvil::RenderingSurface* in_parent_surface_ptr,
                                                              Anvil::Window*           in_window_ptr,
                                                              VkFormat                 in_image_format,
                                                              VkPresentModeKHR         in_present_mode,
                                                              VkImageUsageFlags        in_usage,
                                                              uint32_t                 in_n_swapchain_images)
{
    SwapchainUniquePtr result_ptr(nullptr,
                                  std::default_delete<Anvil::Swapchain>() );

    {
        auto create_info_ptr = Anvil::SwapchainCreateInfo::create(this,
                                                                  in_parent_surface_ptr,
                                                                  in_window_ptr,
                                                                  in_image_format,
                                                                  in_present_mode,
                                                                  in_usage,
                                                                  in_n_swapchain_images);

        create_info_ptr->set_mt_safety(Anvil::MT_SAFETY_ENABLED);

        result_ptr = Anvil::Swapchain::create(std::move(create_info_ptr) );
    }

    return result_ptr;
}

/** Please see header for specification */
const Anvil::PhysicalDeviceFeatures& Anvil::SGPUDevice::get_physical_device_features() const
{
    return m_parent_physical_device_ptr->get_device_features();
}

/** Please see header for specification */
const Anvil::FormatProperties& Anvil::SGPUDevice::get_physical_device_format_properties(VkFormat in_format) const
{
    return m_parent_physical_device_ptr->get_format_properties(in_format);
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

    result = is_vk_call_successful(vkGetPhysicalDeviceImageFormatProperties(m_parent_physical_device_ptr->get_physical_device(),
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
    return m_parent_physical_device_ptr->get_memory_properties();
}

/** Please see header for specification */
const Anvil::PhysicalDeviceProperties& Anvil::SGPUDevice::get_physical_device_properties() const
{
    return m_parent_physical_device_ptr->get_device_properties();
}

/** Please see header for specification */
const Anvil::QueueFamilyInfoItems& Anvil::SGPUDevice::get_physical_device_queue_families() const
{
    return m_parent_physical_device_ptr->get_queue_families();
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

    vkGetPhysicalDeviceSparseImageFormatProperties(m_parent_physical_device_ptr->get_physical_device(),
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

        vkGetPhysicalDeviceSparseImageFormatProperties(m_parent_physical_device_ptr->get_physical_device(),
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
bool Anvil::SGPUDevice::get_physical_device_surface_capabilities(Anvil::RenderingSurface*  in_surface_ptr,
                                                                 VkSurfaceCapabilitiesKHR* out_result_ptr) const
{
    return (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_parent_physical_device_ptr->get_physical_device(),
                                                      in_surface_ptr->get_surface(),
                                                      out_result_ptr) == VK_SUCCESS);
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
    const auto&                   queue_fams(m_parent_physical_device_ptr->get_queue_families() );
    const Anvil::QueueFamilyInfo* result_ptr(nullptr);

    if (queue_fams.size() > in_queue_family_index)
    {
        result_ptr = &queue_fams.at(in_queue_family_index);
    }

    return result_ptr;
}

void Anvil::SGPUDevice::init_device()
{
    /* Stub */
}

/** Please see header for specification */
bool Anvil::SGPUDevice::is_layer_supported(const std::string& in_layer_name) const
{
    return m_parent_physical_device_ptr->is_layer_supported(in_layer_name);
}

/** Please see header for specification */
bool Anvil::SGPUDevice::is_physical_device_extension_supported(const std::string& in_extension_name) const
{
    return m_parent_physical_device_ptr->is_device_extension_supported(in_extension_name);
}
