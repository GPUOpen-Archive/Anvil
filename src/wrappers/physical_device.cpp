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
#include "misc/struct_chainer.h"
#include "wrappers/device.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"
#include <algorithm>

/* Please see header for specification */
bool Anvil::operator==(const std::unique_ptr<Anvil::PhysicalDevice>& in_physical_device_ptr,
                       const VkPhysicalDevice&                       in_physical_device_vk)
{
    return (in_physical_device_ptr->get_physical_device() == in_physical_device_vk);
}

/* Please see header for specification */
std::unique_ptr<Anvil::PhysicalDevice> Anvil::PhysicalDevice::create(Anvil::Instance* in_instance_ptr,
                                                                     uint32_t         in_index,
                                                                     VkPhysicalDevice in_physical_device)
{
    std::unique_ptr<Anvil::PhysicalDevice> physical_device_ptr(
        new Anvil::PhysicalDevice(
            in_instance_ptr,
            in_index,
            in_physical_device) );

    physical_device_ptr->init();

    /* Register the physical device instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_PHYSICAL_DEVICE,
                                                 physical_device_ptr.get() );

    return physical_device_ptr;
}

/* Please see header for specification */
Anvil::PhysicalDevice::~PhysicalDevice()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_PHYSICAL_DEVICE,
                                                    this);
}

/** Initializes _vulkan_physical_device descriptor with actual data by issuing a number
 *  of Vulkan API calls.
 *
 *  @param physical_device_ptr Pointer to a _vulkan_physical_device instance to be initialized.
 *                             Must not be nullptr.
 **/
bool Anvil::PhysicalDevice::init()
{
    const Anvil::ExtensionKHRGetPhysicalDeviceProperties2* gpdp2_entrypoints_ptr           = nullptr;
    VkPhysicalDeviceMemoryProperties                       memory_properties;
    uint32_t                                               n_extensions                    = 0;
    uint32_t                                               n_physical_device_layers        = 0;
    uint32_t                                               n_physical_device_queues        = 0;
    bool                                                   result                          = true;
    VkResult                                               result_vk                       = VK_ERROR_INITIALIZATION_FAILED;
    bool                                                   texture_gather_bias_lod_support = false;

    anvil_assert(m_physical_device != VK_NULL_HANDLE);

    if (m_instance_ptr->get_enabled_extensions_info()->khr_get_physical_device_properties2() )
    {
        gpdp2_entrypoints_ptr = &m_instance_ptr->get_extension_khr_get_physical_device_properties2_entrypoints();
    }

    /* Retrieve device extensions */
    result_vk = vkEnumerateDeviceExtensionProperties(m_physical_device,
                                                     nullptr, /* pLayerName */
                                                    &n_extensions,
                                                     nullptr); /* pProperties */

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert_vk_call_succeeded(result_vk);

        result = false;
        goto end;
    }

    if (n_extensions > 0)
    {
        std::map<std::string, bool>        extension_availability;
        std::vector<VkExtensionProperties> extension_props;

        extension_props.resize(n_extensions);

        result_vk = vkEnumerateDeviceExtensionProperties(m_physical_device,
                                                         nullptr, /* pLayerName */
                                                        &n_extensions,
                                                        &extension_props[0]);
        anvil_assert_vk_call_succeeded(result_vk);

        for (uint32_t n_extension = 0;
                      n_extension < n_extensions;
                    ++n_extension)
        {
            extension_availability[extension_props[n_extension].extensionName] = true;
        }

        m_extension_info_ptr = Anvil::ExtensionInfo<bool>::create_device_extension_info(extension_availability,
                                                                                        false); /* in_unspecified_extension_name_value */
    }

    /* Retrieve and cache device features */
    {
        Anvil::StructID          descriptor_indexing_features_struct_id = UINT32_MAX;
        VkPhysicalDeviceFeatures features_vk;

        vkGetPhysicalDeviceFeatures(m_physical_device,
                                   &features_vk);

        if (m_extension_info_ptr->get_device_extension_info()->ext_descriptor_indexing() )
        {
            const auto&                                               gpdp2_entrypoints = m_instance_ptr->get_extension_khr_get_physical_device_properties2_entrypoints();
            Anvil::StructChainUniquePtr<VkPhysicalDeviceFeatures2KHR> struct_chain_ptr;
            Anvil::StructChainer<VkPhysicalDeviceFeatures2KHR>        struct_chainer;

            {
                VkPhysicalDeviceFeatures2KHR features;

                features.pNext = nullptr;
                features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;

                struct_chainer.append_struct(features);
            }

            {
                VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing_features;

                descriptor_indexing_features.pNext = nullptr;
                descriptor_indexing_features.sType = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT);

                descriptor_indexing_features_struct_id = struct_chainer.append_struct(descriptor_indexing_features);
            }

            struct_chain_ptr = struct_chainer.create_chain();

            gpdp2_entrypoints.vkGetPhysicalDeviceFeatures2KHR(m_physical_device,
                                                              struct_chain_ptr->get_root_struct() );

            m_ext_descriptor_indexing_features_ptr.reset(
                new EXTDescriptorIndexingFeatures(*struct_chain_ptr->get_struct_with_id<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(descriptor_indexing_features_struct_id) )
            );

            if (m_ext_descriptor_indexing_features_ptr == nullptr)
            {
                anvil_assert(m_ext_descriptor_indexing_features_ptr != nullptr);

                result = false;
                goto end;
            }
        }

        if (m_extension_info_ptr->get_device_extension_info()->khr_16bit_storage() )
        {
            const auto&                                               gpdp2_entrypoints          = m_instance_ptr->get_extension_khr_get_physical_device_properties2_entrypoints();
            Anvil::StructID                                           storage_features_struct_id = UINT32_MAX;
            Anvil::StructChainUniquePtr<VkPhysicalDeviceFeatures2KHR> struct_chain_ptr;
            Anvil::StructChainer<VkPhysicalDeviceFeatures2KHR>        struct_chainer;

            {
                VkPhysicalDeviceFeatures2KHR features;

                features.pNext = nullptr;
                features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;

                struct_chainer.append_struct(features);
            }

            {
                VkPhysicalDevice16BitStorageFeaturesKHR storage_features;

                storage_features.pNext = nullptr;
                storage_features.sType = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR);

                storage_features_struct_id = struct_chainer.append_struct(storage_features);
            }

            struct_chain_ptr = struct_chainer.create_chain();

            gpdp2_entrypoints.vkGetPhysicalDeviceFeatures2KHR(m_physical_device,
                                                              struct_chain_ptr->get_root_struct() );

            m_khr_16_bit_storage_features_ptr.reset(
                new KHR16BitStorageFeatures(*struct_chain_ptr->get_struct_with_id<VkPhysicalDevice16BitStorageFeaturesKHR>(storage_features_struct_id) )
            );

            if (m_khr_16_bit_storage_features_ptr == nullptr)
            {
                anvil_assert(m_khr_16_bit_storage_features_ptr != nullptr);

                result = false;
                goto end;
            }
        }

        m_core_features_vk10_ptr.reset(
            new Anvil::PhysicalDeviceFeaturesCoreVK10(features_vk)
        );

        if (m_core_features_vk10_ptr == nullptr)
        {
            anvil_assert(m_core_features_vk10_ptr != nullptr);

            result = false;
            goto end;
        }

        m_features = Anvil::PhysicalDeviceFeatures(m_core_features_vk10_ptr.get              (),
                                                   m_ext_descriptor_indexing_features_ptr.get(),
                                                   m_khr_16_bit_storage_features_ptr.get     () );
    }

    /* Retrieve device format properties */
    texture_gather_bias_lod_support = m_extension_info_ptr->get_device_extension_info()->amd_texture_gather_bias_lod();

    for (VkFormat current_format = static_cast<VkFormat>(1); /* skip the _UNDEFINED format */
                  current_format < VK_FORMAT_RANGE_SIZE;
                  current_format = static_cast<VkFormat>(current_format + 1))
    {
        VkFormatProperties format_props;

        vkGetPhysicalDeviceFormatProperties(m_physical_device,
            current_format,
            &format_props);

        m_format_properties[current_format] = FormatProperties(format_props);

        /* Can this format be used with VK_AMD_texture_gather_bias_lod? */
        if (gpdp2_entrypoints_ptr            != nullptr &&
            texture_gather_bias_lod_support)
        {
            VkPhysicalDeviceImageFormatInfo2KHR                      format_info;
            Anvil::StructChainUniquePtr<VkImageFormatProperties2KHR> struct_chain_ptr;
            Anvil::StructChainer<VkImageFormatProperties2KHR>        struct_chainer;
            Anvil::StructID                                          texture_lod_gather_support_struct_id = UINT32_MAX;

            format_info.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2_KHR;
            format_info.pNext  = nullptr;
            format_info.format = current_format;
            format_info.type   = VK_IMAGE_TYPE_2D;           // irrelevant
            format_info.tiling = VK_IMAGE_TILING_LINEAR;     // irrelevant
            format_info.usage  = VK_IMAGE_USAGE_SAMPLED_BIT; // irrelevant
            format_info.flags  = 0;                          // irrelevant

            {
                VkImageFormatProperties2KHR image_format_props = {};

                image_format_props.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2_KHR;
                image_format_props.pNext = nullptr;

                struct_chainer.append_struct(image_format_props);
            }

            {
                VkTextureLODGatherFormatPropertiesAMD texture_lod_gather_support = {};

                texture_lod_gather_support.sType = VK_STRUCTURE_TYPE_TEXTURE_LOD_GATHER_FORMAT_PROPERTIES_AMD;
                texture_lod_gather_support.pNext = nullptr;

                texture_lod_gather_support_struct_id = struct_chainer.append_struct(texture_lod_gather_support);
            }

            struct_chain_ptr = struct_chainer.create_chain();

            if (gpdp2_entrypoints_ptr->vkGetPhysicalDeviceImageFormatProperties2KHR(m_physical_device,
                                                                                   &format_info,
                                                                                    struct_chain_ptr->get_root_struct() ) == VK_SUCCESS)
            {
                auto texture_lod_gather_support_ptr = struct_chain_ptr->get_struct_with_id<VkTextureLODGatherFormatPropertiesAMD>(texture_lod_gather_support_struct_id);

                m_format_properties.at(current_format).supports_amd_texture_gather_bias_lod = (texture_lod_gather_support_ptr->supportsTextureGatherLODBiasAMD == VK_TRUE);
            }
        }
    }

    /* Retrieve additional device info */
    if (m_instance_ptr->get_enabled_extensions_info()->khr_get_physical_device_properties2() )
    {
        const auto& gpdp2_entrypoints = m_instance_ptr->get_extension_khr_get_physical_device_properties2_entrypoints();

        if (m_instance_ptr->get_enabled_extensions_info()->khr_external_memory_capabilities() )
        {
            Anvil::StructID                                             device_id_props_struct_id  = UINT32_MAX;
            const VkPhysicalDeviceIDPropertiesKHR*                      result_device_id_props_ptr = nullptr;
            Anvil::StructChainUniquePtr<VkPhysicalDeviceProperties2KHR> struct_chain_ptr;
            Anvil::StructChainer<VkPhysicalDeviceProperties2KHR>        struct_chainer;
            {
                VkPhysicalDeviceProperties2KHR general_props;

                general_props.pNext = nullptr;
                general_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;

                struct_chainer.append_struct(general_props);
            }

            {
                VkPhysicalDeviceIDPropertiesKHR device_id_props;

                device_id_props.pNext = nullptr;
                device_id_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR;

                device_id_props_struct_id = struct_chainer.append_struct(device_id_props);
            }

            struct_chain_ptr           = struct_chainer.create_chain();
            result_device_id_props_ptr = struct_chain_ptr->get_struct_with_id<VkPhysicalDeviceIDPropertiesKHR>(device_id_props_struct_id);

            gpdp2_entrypoints.vkGetPhysicalDeviceProperties2KHR(m_physical_device,
                                                                struct_chain_ptr->get_root_struct() );

            if (result_device_id_props_ptr->deviceLUIDValid)
            {
                static_assert(sizeof(m_device_LUID) == sizeof(result_device_id_props_ptr->deviceLUID), "Anvil asserts a LUID size different than the Vulkan header");

                memcpy(m_device_LUID,
                      &result_device_id_props_ptr->deviceLUID,
                       sizeof(m_device_LUID) );

                m_device_LUID_available = true;
            }
            else
            {
                m_device_LUID_available = false;
            }

            static_assert(sizeof(m_device_UUID) == sizeof(result_device_id_props_ptr->deviceUUID), "Anvil asserts a UUID size different than the Vulkan header");
            static_assert(sizeof(m_driver_UUID) == sizeof(result_device_id_props_ptr->driverUUID), "Anvil asserts a UUID size different than the Vulkan header");

            memcpy(m_device_UUID,
                   result_device_id_props_ptr->deviceUUID,
                   sizeof(m_device_UUID) );
            memcpy(m_driver_UUID,
                   result_device_id_props_ptr->driverUUID,
                   sizeof(m_driver_UUID) );

            m_device_UUID_available = true;
            m_driver_UUID_available = true;
        }

        if (m_extension_info_ptr->get_device_extension_info()->amd_shader_core_properties() )
        {
            const VkPhysicalDeviceShaderCorePropertiesAMD*              result_shader_core_props_ptr = nullptr;
            Anvil::StructID                                             shader_core_struct_id        = UINT32_MAX;
            Anvil::StructChainUniquePtr<VkPhysicalDeviceProperties2KHR> struct_chain_ptr;
            Anvil::StructChainer<VkPhysicalDeviceProperties2KHR>        struct_chainer;

            {
                VkPhysicalDeviceProperties2KHR general_props;

                general_props.pNext = nullptr;
                general_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;

                struct_chainer.append_struct(general_props);
            }

            {
                VkPhysicalDeviceShaderCorePropertiesAMD shader_core_properties;

                shader_core_properties.pNext = nullptr;
                shader_core_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD;

                shader_core_struct_id = struct_chainer.append_struct(shader_core_properties);
            }

            struct_chain_ptr             = struct_chainer.create_chain();
            result_shader_core_props_ptr = struct_chain_ptr->get_struct_with_id<VkPhysicalDeviceShaderCorePropertiesAMD>(shader_core_struct_id);

            gpdp2_entrypoints.vkGetPhysicalDeviceProperties2KHR(m_physical_device,
                                                                struct_chain_ptr->get_root_struct() );

            m_amd_shader_core_properties_ptr.reset(
                new AMDShaderCoreProperties(*result_shader_core_props_ptr)
            );

            if (m_amd_shader_core_properties_ptr == nullptr)
            {
                anvil_assert(m_amd_shader_core_properties_ptr != nullptr);

                result = false;
                goto end;
            }
        }

        if (m_extension_info_ptr->get_device_extension_info()->ext_descriptor_indexing() )
        {
            Anvil::StructID                                             descriptor_indexing_props_struct_id         = UINT32_MAX;
            const VkPhysicalDeviceDescriptorIndexingPropertiesEXT*      result_descriptor_indexing_props_struct_ptr = nullptr;
            Anvil::StructChainUniquePtr<VkPhysicalDeviceProperties2KHR> struct_chain_ptr;
            Anvil::StructChainer<VkPhysicalDeviceProperties2KHR>        struct_chainer;

            {
                VkPhysicalDeviceProperties2KHR general_props;

                general_props.pNext = nullptr;
                general_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;

                struct_chainer.append_struct(general_props);
            }

            {
                VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptor_indexing_properties;

                descriptor_indexing_properties.pNext = nullptr;
                descriptor_indexing_properties.sType = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT);

                descriptor_indexing_props_struct_id = struct_chainer.append_struct(descriptor_indexing_properties);
            }

            struct_chain_ptr                            = struct_chainer.create_chain();
            result_descriptor_indexing_props_struct_ptr = struct_chain_ptr->get_struct_with_id<VkPhysicalDeviceDescriptorIndexingPropertiesEXT>(descriptor_indexing_props_struct_id);

            gpdp2_entrypoints.vkGetPhysicalDeviceProperties2KHR(m_physical_device,
                                                                struct_chain_ptr->get_root_struct() );

            m_ext_descriptor_indexing_properties_ptr.reset(
                new EXTDescriptorIndexingProperties(*result_descriptor_indexing_props_struct_ptr)
            );

            if (m_ext_descriptor_indexing_properties_ptr == nullptr)
            {
                anvil_assert(m_ext_descriptor_indexing_properties_ptr != nullptr);

                result = false;
                goto end;
            }
        }

        if (m_extension_info_ptr->get_device_extension_info()->khr_maintenance3() )
        {
            Anvil::StructID                                             maintenance3_struct_id        = UINT32_MAX;
            const VkPhysicalDeviceMaintenance3PropertiesKHR*            result_maintenanc3_struct_ptr = nullptr;
            Anvil::StructChainUniquePtr<VkPhysicalDeviceProperties2KHR> struct_chain_ptr;
            Anvil::StructChainer<VkPhysicalDeviceProperties2KHR>        struct_chainer;

            {
                VkPhysicalDeviceProperties2KHR general_props;

                general_props.pNext = nullptr;
                general_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;

                struct_chainer.append_struct(general_props);
            }
            {
                VkPhysicalDeviceMaintenance3PropertiesKHR maintenance3_props;

                maintenance3_props.pNext = nullptr;
                maintenance3_props.sType = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES_KHR);

                maintenance3_struct_id = struct_chainer.append_struct(maintenance3_props);
            }

            struct_chain_ptr              = struct_chainer.create_chain();
            result_maintenanc3_struct_ptr = struct_chain_ptr->get_struct_with_id<VkPhysicalDeviceMaintenance3PropertiesKHR>(maintenance3_struct_id);

            gpdp2_entrypoints.vkGetPhysicalDeviceProperties2KHR(m_physical_device,
                                                                struct_chain_ptr->get_root_struct() );

            m_khr_maintenance3_properties_ptr.reset(
                new KHRMaintenance3Properties(*result_maintenanc3_struct_ptr)
            );

            if (m_khr_maintenance3_properties_ptr == nullptr)
            {
                anvil_assert(m_khr_maintenance3_properties_ptr != nullptr);

                result = false;
                goto end;
            }
        }
    }

    /* Retrieve device properties */
    {
        VkPhysicalDeviceProperties props;

        vkGetPhysicalDeviceProperties(m_physical_device,
                                     &props);

        m_core_properties_vk10_ptr.reset(
            new Anvil::PhysicalDevicePropertiesCoreVK10(props)
        );

        if (m_core_properties_vk10_ptr == nullptr)
        {
            anvil_assert(m_core_properties_vk10_ptr != nullptr);

            result = false;
            goto end;
        }
    }

    m_properties = Anvil::PhysicalDeviceProperties(m_amd_shader_core_properties_ptr.get        (),
                                                   m_core_properties_vk10_ptr.get              (),
                                                   m_ext_descriptor_indexing_properties_ptr.get(),
                                                   m_khr_maintenance3_properties_ptr.get       () );

    /* Retrieve device queue data */
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device,
                                           &n_physical_device_queues, /* pCount                 */
                                            nullptr);                    /* pQueueFamilyProperties */

    if (n_physical_device_queues > 0)
    {
        std::vector<VkQueueFamilyProperties> queue_props;

        queue_props.resize(n_physical_device_queues);

        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device,
                                                &n_physical_device_queues,
                                                &queue_props[0]);

        for (uint32_t n_physical_device_queue = 0;
                      n_physical_device_queue < n_physical_device_queues;
                    ++n_physical_device_queue)
        {
            m_queue_families.push_back(QueueFamilyInfo(queue_props[n_physical_device_queue]) );
        }
    }

    /* Retrieve memory properties */
    vkGetPhysicalDeviceMemoryProperties(m_physical_device,
                                       &memory_properties);

    m_memory_properties.init(memory_properties);

    /* Retrieve device layers */
    result_vk = vkEnumerateDeviceLayerProperties(m_physical_device,
                                                &n_physical_device_layers,
                                                 nullptr); /* pProperties */

    if (!is_vk_call_successful(result_vk) )
    {
        anvil_assert_vk_call_succeeded(result_vk);

        result = false;
        goto end;
    }

    if (n_physical_device_layers > 0)
    {
        std::vector<VkLayerProperties> layer_props;

        layer_props.resize(n_physical_device_layers);

        result_vk = vkEnumerateDeviceLayerProperties(m_physical_device,
                                                    &n_physical_device_layers,
                                                    &layer_props[0]);

        if (!is_vk_call_successful(result_vk) )
        {
            anvil_assert(is_vk_call_successful(result_vk) );

            result = false;
            goto end;
        }

        for (uint32_t n_layer = 0;
                      n_layer < n_physical_device_layers;
                    ++n_layer)
        {
            m_layers.push_back(Anvil::Layer(layer_props[n_layer]) );
        }
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::PhysicalDevice::get_sparse_image_format_properties(VkFormat                                    in_format,
                                                               VkImageType                                 in_type,
                                                               VkSampleCountFlagBits                       in_sample_count,
                                                               VkImageUsageFlags                           in_usage,
                                                               VkImageTiling                               in_tiling,
                                                               std::vector<VkSparseImageFormatProperties>& out_result) const
{
    /* TODO: It might be a good idea to cache the retrieved properties */
    uint32_t n_properties = 0;

    out_result.clear();

    vkGetPhysicalDeviceSparseImageFormatProperties(m_physical_device,
                                                   in_format,
                                                   in_type,
                                                   in_sample_count,
                                                   in_usage,
                                                   in_tiling,
                                                  &n_properties,
                                                   nullptr); /* pProperties */

    if (n_properties > 0)
    {
        out_result.resize(n_properties);

        vkGetPhysicalDeviceSparseImageFormatProperties(m_physical_device,
                                                   in_format,
                                                   in_type,
                                                   in_sample_count,
                                                   in_usage,
                                                   in_tiling,
                                                  &n_properties,
                                                  &out_result[0]);
    }

    return true;
}

/* Please see header for specification */
bool Anvil::PhysicalDevice::is_device_extension_supported(const std::string& in_extension_name) const
{
    anvil_assert(m_extension_info_ptr != nullptr);

    return m_extension_info_ptr->get_device_extension_info()->by_name(in_extension_name);
}

/* Please see header for specification */
bool Anvil::PhysicalDevice::is_layer_supported(const std::string& in_layer_name) const
{
    return std::find(m_layers .begin(),
                     m_layers.end(),
                     in_layer_name) != m_layers.end();
}
