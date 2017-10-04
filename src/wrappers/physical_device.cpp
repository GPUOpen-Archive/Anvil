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
#include "wrappers/device.h"
#include "wrappers/instance.h"
#include "wrappers/physical_device.h"
#include <algorithm>

/* Please see header for specification */
bool Anvil::operator==(const std::shared_ptr<Anvil::PhysicalDevice>& in_physical_device_ptr,
                       const VkPhysicalDevice&                       in_physical_device_vk)
{
    return (in_physical_device_ptr->get_physical_device() == in_physical_device_vk);
}

/* Please see header for specification */
std::weak_ptr<Anvil::PhysicalDevice> Anvil::PhysicalDevice::create(std::shared_ptr<Anvil::Instance> in_instance_ptr,
                                                                   uint32_t                         in_index,
                                                                   VkPhysicalDevice                 in_physical_device)
{
    std::shared_ptr<Anvil::PhysicalDevice> physical_device_ptr(new Anvil::PhysicalDevice(in_instance_ptr,
                                                                                         in_index,
                                                                                         in_physical_device) );

    physical_device_ptr->init                ();
    in_instance_ptr->register_physical_device(physical_device_ptr);

    /* Register the physical device instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_PHYSICAL_DEVICE,
                                                 physical_device_ptr.get() );

    return physical_device_ptr;
}

/* Please see header for specification */
Anvil::PhysicalDevice::~PhysicalDevice()
{
    anvil_assert(m_destroyed);

    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_PHYSICAL_DEVICE,
                                                    this);
}

/* Please see header for specification */
void Anvil::PhysicalDevice::destroy()
{
    anvil_assert(!m_destroyed);

    m_destroyed = true;

    while (m_cached_devices.size() > 0)
    {
        m_cached_devices.back()->destroy();
    }

    m_instance_ptr->unregister_physical_device(shared_from_this() );

}

/** Initializes _vulkan_physical_device descriptor with actual data by issuing a number
 *  of Vulkan API calls.
 *
 *  @param physical_device_ptr Pointer to a _vulkan_physical_device instance to be initialized.
 *                             Must not be nullptr.
 **/
void Anvil::PhysicalDevice::init()
{
    const Anvil::ExtensionKHRGetPhysicalDeviceProperties2* gpdp2_entrypoints_ptr           = nullptr;
    VkPhysicalDeviceMemoryProperties                       memory_properties;
    uint32_t                                               n_extensions                    = 0;
    uint32_t                                               n_physical_device_layers        = 0;
    uint32_t                                               n_physical_device_queues        = 0;
    VkResult                                               result                          = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result);

    anvil_assert(m_physical_device != VK_NULL_HANDLE);

    if (m_instance_ptr->is_instance_extension_supported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) )
    {
        gpdp2_entrypoints_ptr = &m_instance_ptr->get_extension_khr_get_physical_device_properties2_entrypoints();
    }

    /* Retrieve device extensions */
    result = vkEnumerateDeviceExtensionProperties(m_physical_device,
                                                  nullptr, /* pLayerName */
                                                 &n_extensions,
                                                  nullptr); /* pProperties */
    anvil_assert_vk_call_succeeded(result);

    if (n_extensions > 0)
    {
        std::vector<VkExtensionProperties> extension_props;

        extension_props.resize(n_extensions);

        result = vkEnumerateDeviceExtensionProperties(m_physical_device,
                                                      nullptr, /* pLayerName */
                                                     &n_extensions,
                                                     &extension_props[0]);
        anvil_assert_vk_call_succeeded(result);

        for (uint32_t n_extension = 0;
                      n_extension < n_extensions;
                    ++n_extension)
        {
            m_extensions.push_back(Anvil::Extension(extension_props[n_extension]) );
        }
    }

    /* Retrieve device features */
    vkGetPhysicalDeviceFeatures(m_physical_device,
                               &m_features);

    /* Retrieve device format properties */
    const bool texture_gather_bias_lod_support = is_device_extension_supported(VK_AMD_TEXTURE_GATHER_BIAS_LOD_EXTENSION_NAME);

    for (VkFormat current_format = (VkFormat)1; /* skip the _UNDEFINED format */
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
            VkPhysicalDeviceImageFormatInfo2KHR   format_info;
            VkImageFormatProperties2KHR           image_format_props         = {};
            VkTextureLODGatherFormatPropertiesAMD texture_lod_gather_support = {};

            texture_lod_gather_support.sType = VK_STRUCTURE_TYPE_TEXTURE_LOD_GATHER_FORMAT_PROPERTIES_AMD;
            texture_lod_gather_support.pNext = nullptr;

            format_info.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2_KHR;
            format_info.pNext  = nullptr;
            format_info.format = current_format;
            format_info.type   = VK_IMAGE_TYPE_2D;           // irrelevant
            format_info.tiling = VK_IMAGE_TILING_LINEAR;     // irrelevant
            format_info.usage  = VK_IMAGE_USAGE_SAMPLED_BIT; // irrelevant
            format_info.flags  = 0;                          // irrelevant

            image_format_props.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2_KHR;
            image_format_props.pNext = &texture_lod_gather_support;

            if (gpdp2_entrypoints_ptr->vkGetPhysicalDeviceImageFormatProperties2KHR(m_physical_device,
                                                                                   &format_info,
                                                                                   &image_format_props) == VK_SUCCESS)
            {
                m_format_properties.at(current_format).supports_amd_texture_gather_bias_lod = (texture_lod_gather_support.supportsTextureGatherLODBiasAMD == VK_TRUE);
            }
        }
    }

    if (is_device_extension_supported                  ("VK_KHR_16bit_storage")                   &&
        m_instance_ptr->is_instance_extension_supported("VK_KHR_get_physical_device_properties2") )
    {
        const auto& gpdp2_entrypoints = m_instance_ptr->get_extension_khr_get_physical_device_properties2_entrypoints();

        if (is_device_extension_supported(VK_KHR_16BIT_STORAGE_EXTENSION_NAME) )
        {
            VkPhysicalDeviceFeatures2KHR            features;
            VkPhysicalDevice16BitStorageFeaturesKHR storage_features;

            features.pNext = &storage_features;
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;

            storage_features.pNext = nullptr;
            storage_features.sType = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR);

            gpdp2_entrypoints.vkGetPhysicalDeviceFeatures2KHR(m_physical_device,
                                                             &features);

            m_16bit_storage_features           = StorageFeatures16Bit( (storage_features.storageInputOutput16               == VK_TRUE),
                                                                       (storage_features.storagePushConstant16              == VK_TRUE),
                                                                       (storage_features.storageBuffer16BitAccess           == VK_TRUE),
                                                                       (storage_features.uniformAndStorageBuffer16BitAccess == VK_TRUE));
            m_16bit_storage_features_available = true;
        }
    }

    /* Retrieve device properties */
    vkGetPhysicalDeviceProperties(m_physical_device,
                                 &m_properties);

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
    result = vkEnumerateDeviceLayerProperties(m_physical_device,
                                             &n_physical_device_layers,
                                              nullptr); /* pProperties */
    anvil_assert_vk_call_succeeded(result);

    if (n_physical_device_layers > 0)
    {
        std::vector<VkLayerProperties> layer_props;

        layer_props.resize(n_physical_device_layers);

        result = vkEnumerateDeviceLayerProperties(m_physical_device,
                                                 &n_physical_device_layers,
                                                 &layer_props[0]);

        for (uint32_t n_layer = 0;
                      n_layer < n_physical_device_layers;
                    ++n_layer)
        {
            m_layers.push_back(Anvil::Layer(layer_props[n_layer]) );
        }
    }
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
    return std::find(m_extensions.begin(),
                     m_extensions.end(),
                     in_extension_name) != m_extensions.end();
}

/* Please see header for specification */
bool Anvil::PhysicalDevice::is_layer_supported(const std::string& in_layer_name) const
{
    return std::find(m_layers .begin(),
                     m_layers.end(),
                     in_layer_name) != m_layers.end();
}

/* Please see header for specification */
void Anvil::PhysicalDevice::register_device(std::shared_ptr<Anvil::BaseDevice> in_device_ptr)
{
    auto device_iterator = std::find(m_cached_devices.begin(),
                                     m_cached_devices.end(),
                                     in_device_ptr);

    anvil_assert(device_iterator == m_cached_devices.end() );

    if (device_iterator == m_cached_devices.end() )
    {
        m_cached_devices.push_back(in_device_ptr);
    }
}

/* Please see header for specification */
void Anvil::PhysicalDevice::unregister_device(std::shared_ptr<Anvil::BaseDevice> in_device_ptr)
{
    auto device_iterator = std::find(m_cached_devices.begin(),
                                     m_cached_devices.end(),
                                     in_device_ptr);

    anvil_assert(device_iterator != m_cached_devices.end() );

    if (device_iterator != m_cached_devices.end() )
    {
        m_cached_devices.erase(device_iterator);
    }
}
