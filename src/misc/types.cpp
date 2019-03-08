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
#include "misc/types.h"

INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::AccessFlags,                      VkAccessFlags,                         Anvil::AccessFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::BufferCreateFlags,                VkBufferCreateFlags,                   Anvil::BufferCreateFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::BufferUsageFlags,                 VkBufferUsageFlags,                    Anvil::BufferUsageFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::ColorComponentFlags,              VkColorComponentFlags,                 Anvil::ColorComponentFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::CompositeAlphaFlags,              VkCompositeAlphaFlagsKHR,              Anvil::CompositeAlphaFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::CullModeFlags,                    VkCullModeFlags,                       Anvil::CullModeFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::DebugMessageSeverityFlags,        VkDebugUtilsMessageSeverityFlagsEXT,   Anvil::DebugMessageSeverityFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::DebugMessageTypeFlags,            VkDebugUtilsMessageTypeFlagsEXT,       Anvil::DebugMessageTypeFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::DependencyFlags,                  VkDependencyFlags,                     Anvil::DependencyFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::DescriptorBindingFlags,           VkDescriptorBindingFlagsEXT,           Anvil::DescriptorBindingFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::DescriptorPoolCreateFlags,        VkDescriptorPoolCreateFlags,           Anvil::DescriptorPoolCreateFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::DeviceGroupPresentModeFlags,      VkDeviceGroupPresentModeFlagsKHR,      Anvil::DeviceGroupPresentModeFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::ExternalFenceHandleTypeFlags,     VkExternalFenceHandleTypeFlagsKHR,     Anvil::ExternalFenceHandleTypeFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::ExternalMemoryHandleTypeFlags,    VkExternalMemoryHandleTypeFlagsKHR,    Anvil::ExternalMemoryHandleTypeFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::ExternalSemaphoreHandleTypeFlags, VkExternalSemaphoreHandleTypeFlagsKHR, Anvil::ExternalSemaphoreHandleTypeFlagBits)
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::FormatFeatureFlags,               VkFormatFeatureFlags,                  Anvil::FormatFeatureFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::ImageAspectFlags,                 VkImageAspectFlags,                    Anvil::ImageAspectFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::ImageCreateFlags,                 VkImageCreateFlags,                    Anvil::ImageCreateFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::ImageUsageFlags,                  VkImageUsageFlags,                     Anvil::ImageUsageFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::MemoryFeatureFlags,               uint32_t,                              Anvil::MemoryFeatureFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::MemoryHeapFlags,                  VkMemoryHeapFlags,                     Anvil::MemoryHeapFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::MemoryPropertyFlags,              VkMemoryPropertyFlags,                 Anvil::MemoryPropertyFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::PeerMemoryFeatureFlags,           VkPeerMemoryFeatureFlagsKHR,           Anvil::PeerMemoryFeatureFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::PipelineCreateFlags,              VkPipelineCreateFlags,                 Anvil::PipelineCreateFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::PipelineStageFlags,               VkPipelineStageFlags,                  Anvil::PipelineStageFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::QueueFamilyFlags,                 uint32_t,                              Anvil::QueueFamilyFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::QueueFlags,                       VkQueueFlags,                          Anvil::QueueFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::QueryControlFlags,                VkQueryControlFlags,                   Anvil::QueryControlFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::QueryPipelineStatisticFlags,      VkQueryPipelineStatisticFlags,         Anvil::QueryPipelineStatisticFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::QueryResultFlags,                 VkQueryResultFlags,                    Anvil::QueryResultFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::ResolveModeFlags,                 VkResolveModeFlagsKHR,                 Anvil::ResolveModeFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::SampleCountFlags,                 VkSampleCountFlags,                    Anvil::SampleCountFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::ShaderStageFlags,                 VkShaderStageFlags,                    Anvil::ShaderStageFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::SparseImageFormatFlags,           VkSparseImageFormatFlags,              Anvil::SparseImageFormatFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::SparseMemoryBindFlags,            VkSparseMemoryBindFlags,               Anvil::SparseMemoryBindFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::StencilFaceFlags,                 VkStencilFaceFlags,                    Anvil::StencilFaceFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::SurfaceTransformFlags,            VkSurfaceTransformFlagsKHR,            Anvil::SurfaceTransformFlagBits);
INJECT_BITFIELD_HELPER_FUNC_IMPLEMENTATION(Anvil::SwapchainCreateFlags,             VkSwapchainCreateFlagsKHR,             Anvil::SwapchainCreateFlagBits);