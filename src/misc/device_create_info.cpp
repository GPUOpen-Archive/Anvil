//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/device_create_info.h"
#include "wrappers/physical_device.h"

Anvil::DeviceCreateInfoUniquePtr Anvil::DeviceCreateInfo::create_mgpu(const std::vector<const Anvil::PhysicalDevice*>& in_physical_device_ptrs,
                                                                      const bool&                                      in_enable_shader_module_cache,
                                                                      const DeviceExtensionConfiguration&              in_extension_configuration,
                                                                      const std::vector<std::string>&                  in_layers_to_enable,
                                                                      const Anvil::CommandPoolCreateFlags&             in_helper_command_pool_create_flags,
                                                                      const bool&                                      in_mt_safe)
{
    Anvil::DeviceCreateInfoUniquePtr result_ptr;

    result_ptr.reset(
        new Anvil::DeviceCreateInfo(in_physical_device_ptrs,
                                    in_enable_shader_module_cache,
                                    in_extension_configuration,
                                    in_layers_to_enable,
                                    in_helper_command_pool_create_flags,
                                    in_mt_safe)
    );

    anvil_assert(result_ptr != nullptr);
    return result_ptr;
}

Anvil::DeviceCreateInfoUniquePtr Anvil::DeviceCreateInfo::create_sgpu(const Anvil::PhysicalDevice*         in_physical_device_ptr,
                                                                      const bool&                          in_enable_shader_module_cache,
                                                                      const DeviceExtensionConfiguration&  in_extension_configuration,
                                                                      const std::vector<std::string>&      in_layers_to_enable,
                                                                      const Anvil::CommandPoolCreateFlags& in_helper_command_pool_create_flags,
                                                                      const bool&                          in_mt_safe)
{
    Anvil::DeviceCreateInfoUniquePtr result_ptr;

    result_ptr.reset(
        new Anvil::DeviceCreateInfo({in_physical_device_ptr},
                                    in_enable_shader_module_cache,
                                    in_extension_configuration,
                                    in_layers_to_enable,
                                    in_helper_command_pool_create_flags,
                                    in_mt_safe)
    );

    anvil_assert(result_ptr != nullptr);
    return result_ptr;
}

Anvil::DeviceCreateInfo::DeviceCreateInfo(const std::vector<const Anvil::PhysicalDevice*>& in_physical_device_ptrs,
                                          const bool&                                      in_enable_shader_module_cache,
                                          const DeviceExtensionConfiguration&              in_extension_configuration,
                                          const std::vector<std::string>&                  in_layers_to_enable,
                                          const Anvil::CommandPoolCreateFlags&             in_helper_command_pool_create_flags,
                                          const bool&                                      in_mt_safe)
    :m_extension_configuration          (in_extension_configuration),
     m_helper_command_pool_create_flags (in_helper_command_pool_create_flags),
     m_layers_to_enable                 (in_layers_to_enable),
     m_memory_overallocation_behavior   (Anvil::MemoryOverallocationBehavior::DEFAULT),
     m_mt_safe                          (in_mt_safe),
     m_physical_device_ptrs             (in_physical_device_ptrs),
     m_should_enable_shader_module_cache(in_enable_shader_module_cache)
{
    if (in_physical_device_ptrs.size() > 1)
    {
        for (const auto& current_physical_device_ptr : in_physical_device_ptrs)
        {
            ANVIL_REDUNDANT_VARIABLE_CONST(current_physical_device_ptr);

            anvil_assert(current_physical_device_ptr->get_instance() == in_physical_device_ptrs.at(0)->get_instance() );
        }
    }
}