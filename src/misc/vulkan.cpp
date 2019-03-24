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
#define ANVIL_VULKAN_CPP

#include "misc/vulkan.h"
#include <mutex>

/* For Anvil builds which statically link with Vulkan DLL,  the pointers are set to the system-provided func ptrs.
 * For Anvil builds which link dynamically with Vulkan DLL, the pointers are initialized at Anvil::Instance creation time.
 */

#if !defined(ANVIL_LINK_STATICALLY_WITH_VULKAN_LIB)
    PFN_vkCreateInstance                                Anvil::Vulkan::vkCreateInstance                               = nullptr;
    PFN_vkDestroyInstance                               Anvil::Vulkan::vkDestroyInstance                              = nullptr;
    PFN_vkEnumeratePhysicalDevices                      Anvil::Vulkan::vkEnumeratePhysicalDevices                     = nullptr;
    PFN_vkGetPhysicalDeviceFeatures                     Anvil::Vulkan::vkGetPhysicalDeviceFeatures                    = nullptr;
    PFN_vkGetPhysicalDeviceFormatProperties             Anvil::Vulkan::vkGetPhysicalDeviceFormatProperties            = nullptr;
    PFN_vkGetPhysicalDeviceImageFormatProperties        Anvil::Vulkan::vkGetPhysicalDeviceImageFormatProperties       = nullptr;
    PFN_vkGetPhysicalDeviceProperties                   Anvil::Vulkan::vkGetPhysicalDeviceProperties                  = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties        Anvil::Vulkan::vkGetPhysicalDeviceQueueFamilyProperties       = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties             Anvil::Vulkan::vkGetPhysicalDeviceMemoryProperties            = nullptr;
    PFN_vkGetInstanceProcAddr                           Anvil::Vulkan::vkGetInstanceProcAddr                          = nullptr;
    PFN_vkGetDeviceProcAddr                             Anvil::Vulkan::vkGetDeviceProcAddr                            = nullptr;
    PFN_vkCreateDevice                                  Anvil::Vulkan::vkCreateDevice                                 = nullptr;
    PFN_vkDestroyDevice                                 Anvil::Vulkan::vkDestroyDevice                                = nullptr;
    PFN_vkEnumerateInstanceExtensionProperties          Anvil::Vulkan::vkEnumerateInstanceExtensionProperties         = nullptr;
    PFN_vkEnumerateDeviceExtensionProperties            Anvil::Vulkan::vkEnumerateDeviceExtensionProperties           = nullptr;
    PFN_vkEnumerateInstanceLayerProperties              Anvil::Vulkan::vkEnumerateInstanceLayerProperties             = nullptr;
    PFN_vkEnumerateDeviceLayerProperties                Anvil::Vulkan::vkEnumerateDeviceLayerProperties               = nullptr;
    PFN_vkGetDeviceQueue                                Anvil::Vulkan::vkGetDeviceQueue                               = nullptr;
    PFN_vkQueueSubmit                                   Anvil::Vulkan::vkQueueSubmit                                  = nullptr;
    PFN_vkQueueWaitIdle                                 Anvil::Vulkan::vkQueueWaitIdle                                = nullptr;
    PFN_vkDeviceWaitIdle                                Anvil::Vulkan::vkDeviceWaitIdle                               = nullptr;
    PFN_vkAllocateMemory                                Anvil::Vulkan::vkAllocateMemory                               = nullptr;
    PFN_vkFreeMemory                                    Anvil::Vulkan::vkFreeMemory                                   = nullptr;
    PFN_vkMapMemory                                     Anvil::Vulkan::vkMapMemory                                    = nullptr;
    PFN_vkUnmapMemory                                   Anvil::Vulkan::vkUnmapMemory                                  = nullptr;
    PFN_vkFlushMappedMemoryRanges                       Anvil::Vulkan::vkFlushMappedMemoryRanges                      = nullptr;
    PFN_vkInvalidateMappedMemoryRanges                  Anvil::Vulkan::vkInvalidateMappedMemoryRanges                 = nullptr;
    PFN_vkGetDeviceMemoryCommitment                     Anvil::Vulkan::vkGetDeviceMemoryCommitment                    = nullptr;
    PFN_vkBindBufferMemory                              Anvil::Vulkan::vkBindBufferMemory                             = nullptr;
    PFN_vkBindImageMemory                               Anvil::Vulkan::vkBindImageMemory                              = nullptr;
    PFN_vkGetBufferMemoryRequirements                   Anvil::Vulkan::vkGetBufferMemoryRequirements                  = nullptr;
    PFN_vkGetImageMemoryRequirements                    Anvil::Vulkan::vkGetImageMemoryRequirements                   = nullptr;
    PFN_vkGetImageSparseMemoryRequirements              Anvil::Vulkan::vkGetImageSparseMemoryRequirements             = nullptr;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties  Anvil::Vulkan::vkGetPhysicalDeviceSparseImageFormatProperties = nullptr;
    PFN_vkQueueBindSparse                               Anvil::Vulkan::vkQueueBindSparse                              = nullptr;
    PFN_vkCreateFence                                   Anvil::Vulkan::vkCreateFence                                  = nullptr;
    PFN_vkDestroyFence                                  Anvil::Vulkan::vkDestroyFence                                 = nullptr;
    PFN_vkResetFences                                   Anvil::Vulkan::vkResetFences                                  = nullptr;
    PFN_vkGetFenceStatus                                Anvil::Vulkan::vkGetFenceStatus                               = nullptr;
    PFN_vkWaitForFences                                 Anvil::Vulkan::vkWaitForFences                                = nullptr;
    PFN_vkCreateSemaphore                               Anvil::Vulkan::vkCreateSemaphore                              = nullptr;
    PFN_vkDestroySemaphore                              Anvil::Vulkan::vkDestroySemaphore                             = nullptr;
    PFN_vkCreateEvent                                   Anvil::Vulkan::vkCreateEvent                                  = nullptr;
    PFN_vkDestroyEvent                                  Anvil::Vulkan::vkDestroyEvent                                 = nullptr;
    PFN_vkGetEventStatus                                Anvil::Vulkan::vkGetEventStatus                               = nullptr;
    PFN_vkSetEvent                                      Anvil::Vulkan::vkSetEvent                                     = nullptr;
    PFN_vkResetEvent                                    Anvil::Vulkan::vkResetEvent                                   = nullptr;
    PFN_vkCreateQueryPool                               Anvil::Vulkan::vkCreateQueryPool                              = nullptr;
    PFN_vkDestroyQueryPool                              Anvil::Vulkan::vkDestroyQueryPool                             = nullptr;
    PFN_vkGetQueryPoolResults                           Anvil::Vulkan::vkGetQueryPoolResults                          = nullptr;
    PFN_vkCreateBuffer                                  Anvil::Vulkan::vkCreateBuffer                                 = nullptr;
    PFN_vkDestroyBuffer                                 Anvil::Vulkan::vkDestroyBuffer                                = nullptr;
    PFN_vkCreateBufferView                              Anvil::Vulkan::vkCreateBufferView                             = nullptr;
    PFN_vkDestroyBufferView                             Anvil::Vulkan::vkDestroyBufferView                            = nullptr;
    PFN_vkCreateImage                                   Anvil::Vulkan::vkCreateImage                                  = nullptr;
    PFN_vkDestroyImage                                  Anvil::Vulkan::vkDestroyImage                                 = nullptr;
    PFN_vkGetImageSubresourceLayout                     Anvil::Vulkan::vkGetImageSubresourceLayout                    = nullptr;
    PFN_vkCreateImageView                               Anvil::Vulkan::vkCreateImageView                              = nullptr;
    PFN_vkDestroyImageView                              Anvil::Vulkan::vkDestroyImageView                             = nullptr;
    PFN_vkCreateShaderModule                            Anvil::Vulkan::vkCreateShaderModule                           = nullptr;
    PFN_vkDestroyShaderModule                           Anvil::Vulkan::vkDestroyShaderModule                          = nullptr;
    PFN_vkCreatePipelineCache                           Anvil::Vulkan::vkCreatePipelineCache                          = nullptr;
    PFN_vkDestroyPipelineCache                          Anvil::Vulkan::vkDestroyPipelineCache                         = nullptr;
    PFN_vkGetPipelineCacheData                          Anvil::Vulkan::vkGetPipelineCacheData                         = nullptr;
    PFN_vkMergePipelineCaches                           Anvil::Vulkan::vkMergePipelineCaches                          = nullptr;
    PFN_vkCreateGraphicsPipelines                       Anvil::Vulkan::vkCreateGraphicsPipelines                      = nullptr;
    PFN_vkCreateComputePipelines                        Anvil::Vulkan::vkCreateComputePipelines                       = nullptr;
    PFN_vkDestroyPipeline                               Anvil::Vulkan::vkDestroyPipeline                              = nullptr;
    PFN_vkCreatePipelineLayout                          Anvil::Vulkan::vkCreatePipelineLayout                         = nullptr;
    PFN_vkDestroyPipelineLayout                         Anvil::Vulkan::vkDestroyPipelineLayout                        = nullptr;
    PFN_vkCreateSampler                                 Anvil::Vulkan::vkCreateSampler                                = nullptr;
    PFN_vkDestroySampler                                Anvil::Vulkan::vkDestroySampler                               = nullptr;
    PFN_vkCreateDescriptorSetLayout                     Anvil::Vulkan::vkCreateDescriptorSetLayout                    = nullptr;
    PFN_vkDestroyDescriptorSetLayout                    Anvil::Vulkan::vkDestroyDescriptorSetLayout                   = nullptr;
    PFN_vkCreateDescriptorPool                          Anvil::Vulkan::vkCreateDescriptorPool                         = nullptr;
    PFN_vkDestroyDescriptorPool                         Anvil::Vulkan::vkDestroyDescriptorPool                        = nullptr;
    PFN_vkResetDescriptorPool                           Anvil::Vulkan::vkResetDescriptorPool                          = nullptr;
    PFN_vkAllocateDescriptorSets                        Anvil::Vulkan::vkAllocateDescriptorSets                       = nullptr;
    PFN_vkFreeDescriptorSets                            Anvil::Vulkan::vkFreeDescriptorSets                           = nullptr;
    PFN_vkUpdateDescriptorSets                          Anvil::Vulkan::vkUpdateDescriptorSets                         = nullptr;
    PFN_vkCreateFramebuffer                             Anvil::Vulkan::vkCreateFramebuffer                            = nullptr;
    PFN_vkDestroyFramebuffer                            Anvil::Vulkan::vkDestroyFramebuffer                           = nullptr;
    PFN_vkCreateRenderPass                              Anvil::Vulkan::vkCreateRenderPass                             = nullptr;
    PFN_vkDestroyRenderPass                             Anvil::Vulkan::vkDestroyRenderPass                            = nullptr;
    PFN_vkGetRenderAreaGranularity                      Anvil::Vulkan::vkGetRenderAreaGranularity                     = nullptr;
    PFN_vkCreateCommandPool                             Anvil::Vulkan::vkCreateCommandPool                            = nullptr;
    PFN_vkDestroyCommandPool                            Anvil::Vulkan::vkDestroyCommandPool                           = nullptr;
    PFN_vkResetCommandPool                              Anvil::Vulkan::vkResetCommandPool                             = nullptr;
    PFN_vkAllocateCommandBuffers                        Anvil::Vulkan::vkAllocateCommandBuffers                       = nullptr;
    PFN_vkFreeCommandBuffers                            Anvil::Vulkan::vkFreeCommandBuffers                           = nullptr;
    PFN_vkBeginCommandBuffer                            Anvil::Vulkan::vkBeginCommandBuffer                           = nullptr;
    PFN_vkEndCommandBuffer                              Anvil::Vulkan::vkEndCommandBuffer                             = nullptr;
    PFN_vkResetCommandBuffer                            Anvil::Vulkan::vkResetCommandBuffer                           = nullptr;
    PFN_vkCmdBindPipeline                               Anvil::Vulkan::vkCmdBindPipeline                              = nullptr;
    PFN_vkCmdSetViewport                                Anvil::Vulkan::vkCmdSetViewport                               = nullptr;
    PFN_vkCmdSetScissor                                 Anvil::Vulkan::vkCmdSetScissor                                = nullptr;
    PFN_vkCmdSetLineWidth                               Anvil::Vulkan::vkCmdSetLineWidth                              = nullptr;
    PFN_vkCmdSetDepthBias                               Anvil::Vulkan::vkCmdSetDepthBias                              = nullptr;
    PFN_vkCmdSetBlendConstants                          Anvil::Vulkan::vkCmdSetBlendConstants                         = nullptr;
    PFN_vkCmdSetDepthBounds                             Anvil::Vulkan::vkCmdSetDepthBounds                            = nullptr;
    PFN_vkCmdSetStencilCompareMask                      Anvil::Vulkan::vkCmdSetStencilCompareMask                     = nullptr;
    PFN_vkCmdSetStencilWriteMask                        Anvil::Vulkan::vkCmdSetStencilWriteMask                       = nullptr;
    PFN_vkCmdSetStencilReference                        Anvil::Vulkan::vkCmdSetStencilReference                       = nullptr;
    PFN_vkCmdBindDescriptorSets                         Anvil::Vulkan::vkCmdBindDescriptorSets                        = nullptr;
    PFN_vkCmdBindIndexBuffer                            Anvil::Vulkan::vkCmdBindIndexBuffer                           = nullptr;
    PFN_vkCmdBindVertexBuffers                          Anvil::Vulkan::vkCmdBindVertexBuffers                         = nullptr;
    PFN_vkCmdDraw                                       Anvil::Vulkan::vkCmdDraw                                      = nullptr;
    PFN_vkCmdDrawIndexed                                Anvil::Vulkan::vkCmdDrawIndexed                               = nullptr;
    PFN_vkCmdDrawIndirect                               Anvil::Vulkan::vkCmdDrawIndirect                              = nullptr;
    PFN_vkCmdDrawIndexedIndirect                        Anvil::Vulkan::vkCmdDrawIndexedIndirect                       = nullptr;
    PFN_vkCmdDispatch                                   Anvil::Vulkan::vkCmdDispatch                                  = nullptr;
    PFN_vkCmdDispatchIndirect                           Anvil::Vulkan::vkCmdDispatchIndirect                          = nullptr;
    PFN_vkCmdCopyBuffer                                 Anvil::Vulkan::vkCmdCopyBuffer                                = nullptr;
    PFN_vkCmdCopyImage                                  Anvil::Vulkan::vkCmdCopyImage                                 = nullptr;
    PFN_vkCmdBlitImage                                  Anvil::Vulkan::vkCmdBlitImage                                 = nullptr;
    PFN_vkCmdCopyBufferToImage                          Anvil::Vulkan::vkCmdCopyBufferToImage                         = nullptr;
    PFN_vkCmdCopyImageToBuffer                          Anvil::Vulkan::vkCmdCopyImageToBuffer                         = nullptr;
    PFN_vkCmdUpdateBuffer                               Anvil::Vulkan::vkCmdUpdateBuffer                              = nullptr;
    PFN_vkCmdFillBuffer                                 Anvil::Vulkan::vkCmdFillBuffer                                = nullptr;
    PFN_vkCmdClearColorImage                            Anvil::Vulkan::vkCmdClearColorImage                           = nullptr;
    PFN_vkCmdClearDepthStencilImage                     Anvil::Vulkan::vkCmdClearDepthStencilImage                    = nullptr;
    PFN_vkCmdClearAttachments                           Anvil::Vulkan::vkCmdClearAttachments                          = nullptr;
    PFN_vkCmdResolveImage                               Anvil::Vulkan::vkCmdResolveImage                              = nullptr;
    PFN_vkCmdSetEvent                                   Anvil::Vulkan::vkCmdSetEvent                                  = nullptr;
    PFN_vkCmdResetEvent                                 Anvil::Vulkan::vkCmdResetEvent                                = nullptr;
    PFN_vkCmdWaitEvents                                 Anvil::Vulkan::vkCmdWaitEvents                                = nullptr;
    PFN_vkCmdPipelineBarrier                            Anvil::Vulkan::vkCmdPipelineBarrier                           = nullptr;
    PFN_vkCmdBeginQuery                                 Anvil::Vulkan::vkCmdBeginQuery                                = nullptr;
    PFN_vkCmdEndQuery                                   Anvil::Vulkan::vkCmdEndQuery                                  = nullptr;
    PFN_vkCmdResetQueryPool                             Anvil::Vulkan::vkCmdResetQueryPool                            = nullptr;
    PFN_vkCmdWriteTimestamp                             Anvil::Vulkan::vkCmdWriteTimestamp                            = nullptr;
    PFN_vkCmdCopyQueryPoolResults                       Anvil::Vulkan::vkCmdCopyQueryPoolResults                      = nullptr;
    PFN_vkCmdPushConstants                              Anvil::Vulkan::vkCmdPushConstants                             = nullptr;
    PFN_vkCmdBeginRenderPass                            Anvil::Vulkan::vkCmdBeginRenderPass                           = nullptr;
    PFN_vkCmdNextSubpass                                Anvil::Vulkan::vkCmdNextSubpass                               = nullptr;
    PFN_vkCmdEndRenderPass                              Anvil::Vulkan::vkCmdEndRenderPass                             = nullptr;
    PFN_vkCmdExecuteCommands                            Anvil::Vulkan::vkCmdExecuteCommands                           = nullptr;

    PFN_vkBindBufferMemory2                             Anvil::Vulkan::vkBindBufferMemory2                             = nullptr;
    PFN_vkBindImageMemory2                              Anvil::Vulkan::vkBindImageMemory2                              = nullptr;
    PFN_vkCmdDispatchBase                               Anvil::Vulkan::vkCmdDispatchBase                               = nullptr;
    PFN_vkCmdSetDeviceMask                              Anvil::Vulkan::vkCmdSetDeviceMask                              = nullptr;
    PFN_vkCreateDescriptorUpdateTemplate                Anvil::Vulkan::vkCreateDescriptorUpdateTemplate                = nullptr;
    PFN_vkCreateSamplerYcbcrConversion                  Anvil::Vulkan::vkCreateSamplerYcbcrConversion                  = nullptr;
    PFN_vkDestroyDescriptorUpdateTemplate               Anvil::Vulkan::vkDestroyDescriptorUpdateTemplate               = nullptr;
    PFN_vkDestroySamplerYcbcrConversion                 Anvil::Vulkan::vkDestroySamplerYcbcrConversion                 = nullptr;
    PFN_vkEnumerateInstanceVersion                      Anvil::Vulkan::vkEnumerateInstanceVersion                      = nullptr;
    PFN_vkEnumeratePhysicalDeviceGroups                 Anvil::Vulkan::vkEnumeratePhysicalDeviceGroups                 = nullptr;
    PFN_vkGetBufferMemoryRequirements2                  Anvil::Vulkan::vkGetBufferMemoryRequirements2                  = nullptr;
    PFN_vkGetDescriptorSetLayoutSupport                 Anvil::Vulkan::vkGetDescriptorSetLayoutSupport                 = nullptr;
    PFN_vkGetDeviceGroupPeerMemoryFeatures              Anvil::Vulkan::vkGetDeviceGroupPeerMemoryFeatures              = nullptr;
    PFN_vkGetDeviceQueue2                               Anvil::Vulkan::vkGetDeviceQueue2                               = nullptr;
    PFN_vkGetImageMemoryRequirements2                   Anvil::Vulkan::vkGetImageMemoryRequirements2                   = nullptr;
    PFN_vkGetImageSparseMemoryRequirements2             Anvil::Vulkan::vkGetImageSparseMemoryRequirements2             = nullptr;
    PFN_vkGetPhysicalDeviceExternalBufferProperties     Anvil::Vulkan::vkGetPhysicalDeviceExternalBufferProperties     = nullptr;
    PFN_vkGetPhysicalDeviceExternalFenceProperties      Anvil::Vulkan::vkGetPhysicalDeviceExternalFenceProperties      = nullptr;
    PFN_vkGetPhysicalDeviceExternalSemaphoreProperties  Anvil::Vulkan::vkGetPhysicalDeviceExternalSemaphoreProperties  = nullptr;
    PFN_vkGetPhysicalDeviceFeatures2                    Anvil::Vulkan::vkGetPhysicalDeviceFeatures2                    = nullptr;
    PFN_vkGetPhysicalDeviceFormatProperties2            Anvil::Vulkan::vkGetPhysicalDeviceFormatProperties2            = nullptr;
    PFN_vkGetPhysicalDeviceImageFormatProperties2       Anvil::Vulkan::vkGetPhysicalDeviceImageFormatProperties2       = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties2            Anvil::Vulkan::vkGetPhysicalDeviceMemoryProperties2            = nullptr;
    PFN_vkGetPhysicalDeviceProperties2                  Anvil::Vulkan::vkGetPhysicalDeviceProperties2                  = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2       Anvil::Vulkan::vkGetPhysicalDeviceQueueFamilyProperties2       = nullptr;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 Anvil::Vulkan::vkGetPhysicalDeviceSparseImageFormatProperties2 = nullptr;
    PFN_vkTrimCommandPool                               Anvil::Vulkan::vkTrimCommandPool                               = nullptr;
    PFN_vkUpdateDescriptorSetWithTemplate               Anvil::Vulkan::vkUpdateDescriptorSetWithTemplate               = nullptr;

#else
    PFN_vkCreateInstance                                Anvil::Vulkan::vkCreateInstance                               = ::vkCreateInstance;
    PFN_vkDestroyInstance                               Anvil::Vulkan::vkDestroyInstance                              = ::vkDestroyInstance;
    PFN_vkEnumeratePhysicalDevices                      Anvil::Vulkan::vkEnumeratePhysicalDevices                     = ::vkEnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceFeatures                     Anvil::Vulkan::vkGetPhysicalDeviceFeatures                    = ::vkGetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceFormatProperties             Anvil::Vulkan::vkGetPhysicalDeviceFormatProperties            = ::vkGetPhysicalDeviceFormatProperties;
    PFN_vkGetPhysicalDeviceImageFormatProperties        Anvil::Vulkan::vkGetPhysicalDeviceImageFormatProperties       = ::vkGetPhysicalDeviceImageFormatProperties;
    PFN_vkGetPhysicalDeviceProperties                   Anvil::Vulkan::vkGetPhysicalDeviceProperties                  = ::vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties        Anvil::Vulkan::vkGetPhysicalDeviceQueueFamilyProperties       = ::vkGetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties             Anvil::Vulkan::vkGetPhysicalDeviceMemoryProperties            = ::vkGetPhysicalDeviceMemoryProperties;
    PFN_vkGetInstanceProcAddr                           Anvil::Vulkan::vkGetInstanceProcAddr                          = ::vkGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr                             Anvil::Vulkan::vkGetDeviceProcAddr                            = ::vkGetDeviceProcAddr;
    PFN_vkCreateDevice                                  Anvil::Vulkan::vkCreateDevice                                 = ::vkCreateDevice;
    PFN_vkDestroyDevice                                 Anvil::Vulkan::vkDestroyDevice                                = ::vkDestroyDevice;
    PFN_vkEnumerateInstanceExtensionProperties          Anvil::Vulkan::vkEnumerateInstanceExtensionProperties         = ::vkEnumerateInstanceExtensionProperties;
    PFN_vkEnumerateDeviceExtensionProperties            Anvil::Vulkan::vkEnumerateDeviceExtensionProperties           = ::vkEnumerateDeviceExtensionProperties;
    PFN_vkEnumerateInstanceLayerProperties              Anvil::Vulkan::vkEnumerateInstanceLayerProperties             = ::vkEnumerateInstanceLayerProperties;
    PFN_vkEnumerateDeviceLayerProperties                Anvil::Vulkan::vkEnumerateDeviceLayerProperties               = ::vkEnumerateDeviceLayerProperties;
    PFN_vkGetDeviceQueue                                Anvil::Vulkan::vkGetDeviceQueue                               = ::vkGetDeviceQueue;
    PFN_vkQueueSubmit                                   Anvil::Vulkan::vkQueueSubmit                                  = ::vkQueueSubmit;
    PFN_vkQueueWaitIdle                                 Anvil::Vulkan::vkQueueWaitIdle                                = ::vkQueueWaitIdle;
    PFN_vkDeviceWaitIdle                                Anvil::Vulkan::vkDeviceWaitIdle                               = ::vkDeviceWaitIdle;
    PFN_vkAllocateMemory                                Anvil::Vulkan::vkAllocateMemory                               = ::vkAllocateMemory;
    PFN_vkFreeMemory                                    Anvil::Vulkan::vkFreeMemory                                   = ::vkFreeMemory;
    PFN_vkMapMemory                                     Anvil::Vulkan::vkMapMemory                                    = ::vkMapMemory;
    PFN_vkUnmapMemory                                   Anvil::Vulkan::vkUnmapMemory                                  = ::vkUnmapMemory;
    PFN_vkFlushMappedMemoryRanges                       Anvil::Vulkan::vkFlushMappedMemoryRanges                      = ::vkFlushMappedMemoryRanges;
    PFN_vkInvalidateMappedMemoryRanges                  Anvil::Vulkan::vkInvalidateMappedMemoryRanges                 = ::vkInvalidateMappedMemoryRanges;
    PFN_vkGetDeviceMemoryCommitment                     Anvil::Vulkan::vkGetDeviceMemoryCommitment                    = ::vkGetDeviceMemoryCommitment;
    PFN_vkBindBufferMemory                              Anvil::Vulkan::vkBindBufferMemory                             = ::vkBindBufferMemory;
    PFN_vkBindImageMemory                               Anvil::Vulkan::vkBindImageMemory                              = ::vkBindImageMemory;
    PFN_vkGetBufferMemoryRequirements                   Anvil::Vulkan::vkGetBufferMemoryRequirements                  = ::vkGetBufferMemoryRequirements;
    PFN_vkGetImageMemoryRequirements                    Anvil::Vulkan::vkGetImageMemoryRequirements                   = ::vkGetImageMemoryRequirements;
    PFN_vkGetImageSparseMemoryRequirements              Anvil::Vulkan::vkGetImageSparseMemoryRequirements             = ::vkGetImageSparseMemoryRequirements;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties  Anvil::Vulkan::vkGetPhysicalDeviceSparseImageFormatProperties = ::vkGetPhysicalDeviceSparseImageFormatProperties;
    PFN_vkQueueBindSparse                               Anvil::Vulkan::vkQueueBindSparse                              = ::vkQueueBindSparse;
    PFN_vkCreateFence                                   Anvil::Vulkan::vkCreateFence                                  = ::vkCreateFence;
    PFN_vkDestroyFence                                  Anvil::Vulkan::vkDestroyFence                                 = ::vkDestroyFence;
    PFN_vkResetFences                                   Anvil::Vulkan::vkResetFences                                  = ::vkResetFences;
    PFN_vkGetFenceStatus                                Anvil::Vulkan::vkGetFenceStatus                               = ::vkGetFenceStatus;
    PFN_vkWaitForFences                                 Anvil::Vulkan::vkWaitForFences                                = ::vkWaitForFences;
    PFN_vkCreateSemaphore                               Anvil::Vulkan::vkCreateSemaphore                              = ::vkCreateSemaphore;
    PFN_vkDestroySemaphore                              Anvil::Vulkan::vkDestroySemaphore                             = ::vkDestroySemaphore;
    PFN_vkCreateEvent                                   Anvil::Vulkan::vkCreateEvent                                  = ::vkCreateEvent;
    PFN_vkDestroyEvent                                  Anvil::Vulkan::vkDestroyEvent                                 = ::vkDestroyEvent;
    PFN_vkGetEventStatus                                Anvil::Vulkan::vkGetEventStatus                               = ::vkGetEventStatus;
    PFN_vkSetEvent                                      Anvil::Vulkan::vkSetEvent                                     = ::vkSetEvent;
    PFN_vkResetEvent                                    Anvil::Vulkan::vkResetEvent                                   = ::vkResetEvent;
    PFN_vkCreateQueryPool                               Anvil::Vulkan::vkCreateQueryPool                              = ::vkCreateQueryPool;
    PFN_vkDestroyQueryPool                              Anvil::Vulkan::vkDestroyQueryPool                             = ::vkDestroyQueryPool;
    PFN_vkGetQueryPoolResults                           Anvil::Vulkan::vkGetQueryPoolResults                          = ::vkGetQueryPoolResults;
    PFN_vkCreateBuffer                                  Anvil::Vulkan::vkCreateBuffer                                 = ::vkCreateBuffer;
    PFN_vkDestroyBuffer                                 Anvil::Vulkan::vkDestroyBuffer                                = ::vkDestroyBuffer;
    PFN_vkCreateBufferView                              Anvil::Vulkan::vkCreateBufferView                             = ::vkCreateBufferView;
    PFN_vkDestroyBufferView                             Anvil::Vulkan::vkDestroyBufferView                            = ::vkDestroyBufferView;
    PFN_vkCreateImage                                   Anvil::Vulkan::vkCreateImage                                  = ::vkCreateImage;
    PFN_vkDestroyImage                                  Anvil::Vulkan::vkDestroyImage                                 = ::vkDestroyImage;
    PFN_vkGetImageSubresourceLayout                     Anvil::Vulkan::vkGetImageSubresourceLayout                    = ::vkGetImageSubresourceLayout;
    PFN_vkCreateImageView                               Anvil::Vulkan::vkCreateImageView                              = ::vkCreateImageView;
    PFN_vkDestroyImageView                              Anvil::Vulkan::vkDestroyImageView                             = ::vkDestroyImageView;
    PFN_vkCreateShaderModule                            Anvil::Vulkan::vkCreateShaderModule                           = ::vkCreateShaderModule;
    PFN_vkDestroyShaderModule                           Anvil::Vulkan::vkDestroyShaderModule                          = ::vkDestroyShaderModule;
    PFN_vkCreatePipelineCache                           Anvil::Vulkan::vkCreatePipelineCache                          = ::vkCreatePipelineCache;
    PFN_vkDestroyPipelineCache                          Anvil::Vulkan::vkDestroyPipelineCache                         = ::vkDestroyPipelineCache;
    PFN_vkGetPipelineCacheData                          Anvil::Vulkan::vkGetPipelineCacheData                         = ::vkGetPipelineCacheData;
    PFN_vkMergePipelineCaches                           Anvil::Vulkan::vkMergePipelineCaches                          = ::vkMergePipelineCaches;
    PFN_vkCreateGraphicsPipelines                       Anvil::Vulkan::vkCreateGraphicsPipelines                      = ::vkCreateGraphicsPipelines;
    PFN_vkCreateComputePipelines                        Anvil::Vulkan::vkCreateComputePipelines                       = ::vkCreateComputePipelines;
    PFN_vkDestroyPipeline                               Anvil::Vulkan::vkDestroyPipeline                              = ::vkDestroyPipeline;
    PFN_vkCreatePipelineLayout                          Anvil::Vulkan::vkCreatePipelineLayout                         = ::vkCreatePipelineLayout;
    PFN_vkDestroyPipelineLayout                         Anvil::Vulkan::vkDestroyPipelineLayout                        = ::vkDestroyPipelineLayout;
    PFN_vkCreateSampler                                 Anvil::Vulkan::vkCreateSampler                                = ::vkCreateSampler;
    PFN_vkDestroySampler                                Anvil::Vulkan::vkDestroySampler                               = ::vkDestroySampler;
    PFN_vkCreateDescriptorSetLayout                     Anvil::Vulkan::vkCreateDescriptorSetLayout                    = ::vkCreateDescriptorSetLayout;
    PFN_vkDestroyDescriptorSetLayout                    Anvil::Vulkan::vkDestroyDescriptorSetLayout                   = ::vkDestroyDescriptorSetLayout;
    PFN_vkCreateDescriptorPool                          Anvil::Vulkan::vkCreateDescriptorPool                         = ::vkCreateDescriptorPool;
    PFN_vkDestroyDescriptorPool                         Anvil::Vulkan::vkDestroyDescriptorPool                        = ::vkDestroyDescriptorPool;
    PFN_vkResetDescriptorPool                           Anvil::Vulkan::vkResetDescriptorPool                          = ::vkResetDescriptorPool;
    PFN_vkAllocateDescriptorSets                        Anvil::Vulkan::vkAllocateDescriptorSets                       = ::vkAllocateDescriptorSets;
    PFN_vkFreeDescriptorSets                            Anvil::Vulkan::vkFreeDescriptorSets                           = ::vkFreeDescriptorSets;
    PFN_vkUpdateDescriptorSets                          Anvil::Vulkan::vkUpdateDescriptorSets                         = ::vkUpdateDescriptorSets;
    PFN_vkCreateFramebuffer                             Anvil::Vulkan::vkCreateFramebuffer                            = ::vkCreateFramebuffer;
    PFN_vkDestroyFramebuffer                            Anvil::Vulkan::vkDestroyFramebuffer                           = ::vkDestroyFramebuffer;
    PFN_vkCreateRenderPass                              Anvil::Vulkan::vkCreateRenderPass                             = ::vkCreateRenderPass;
    PFN_vkDestroyRenderPass                             Anvil::Vulkan::vkDestroyRenderPass                            = ::vkDestroyRenderPass;
    PFN_vkGetRenderAreaGranularity                      Anvil::Vulkan::vkGetRenderAreaGranularity                     = ::vkGetRenderAreaGranularity;
    PFN_vkCreateCommandPool                             Anvil::Vulkan::vkCreateCommandPool                            = ::vkCreateCommandPool;
    PFN_vkDestroyCommandPool                            Anvil::Vulkan::vkDestroyCommandPool                           = ::vkDestroyCommandPool;
    PFN_vkResetCommandPool                              Anvil::Vulkan::vkResetCommandPool                             = ::vkResetCommandPool;
    PFN_vkAllocateCommandBuffers                        Anvil::Vulkan::vkAllocateCommandBuffers                       = ::vkAllocateCommandBuffers;
    PFN_vkFreeCommandBuffers                            Anvil::Vulkan::vkFreeCommandBuffers                           = ::vkFreeCommandBuffers;
    PFN_vkBeginCommandBuffer                            Anvil::Vulkan::vkBeginCommandBuffer                           = ::vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer                              Anvil::Vulkan::vkEndCommandBuffer                             = ::vkEndCommandBuffer;
    PFN_vkResetCommandBuffer                            Anvil::Vulkan::vkResetCommandBuffer                           = ::vkResetCommandBuffer;
    PFN_vkCmdBindPipeline                               Anvil::Vulkan::vkCmdBindPipeline                              = ::vkCmdBindPipeline;
    PFN_vkCmdSetViewport                                Anvil::Vulkan::vkCmdSetViewport                               = ::vkCmdSetViewport;
    PFN_vkCmdSetScissor                                 Anvil::Vulkan::vkCmdSetScissor                                = ::vkCmdSetScissor;
    PFN_vkCmdSetLineWidth                               Anvil::Vulkan::vkCmdSetLineWidth                              = ::vkCmdSetLineWidth;
    PFN_vkCmdSetDepthBias                               Anvil::Vulkan::vkCmdSetDepthBias                              = ::vkCmdSetDepthBias;
    PFN_vkCmdSetBlendConstants                          Anvil::Vulkan::vkCmdSetBlendConstants                         = ::vkCmdSetBlendConstants;
    PFN_vkCmdSetDepthBounds                             Anvil::Vulkan::vkCmdSetDepthBounds                            = ::vkCmdSetDepthBounds;
    PFN_vkCmdSetStencilCompareMask                      Anvil::Vulkan::vkCmdSetStencilCompareMask                     = ::vkCmdSetStencilCompareMask;
    PFN_vkCmdSetStencilWriteMask                        Anvil::Vulkan::vkCmdSetStencilWriteMask                       = ::vkCmdSetStencilWriteMask;
    PFN_vkCmdSetStencilReference                        Anvil::Vulkan::vkCmdSetStencilReference                       = ::vkCmdSetStencilReference;
    PFN_vkCmdBindDescriptorSets                         Anvil::Vulkan::vkCmdBindDescriptorSets                        = ::vkCmdBindDescriptorSets;
    PFN_vkCmdBindIndexBuffer                            Anvil::Vulkan::vkCmdBindIndexBuffer                           = ::vkCmdBindIndexBuffer;
    PFN_vkCmdBindVertexBuffers                          Anvil::Vulkan::vkCmdBindVertexBuffers                         = ::vkCmdBindVertexBuffers;
    PFN_vkCmdDraw                                       Anvil::Vulkan::vkCmdDraw                                      = ::vkCmdDraw;
    PFN_vkCmdDrawIndexed                                Anvil::Vulkan::vkCmdDrawIndexed                               = ::vkCmdDrawIndexed;
    PFN_vkCmdDrawIndirect                               Anvil::Vulkan::vkCmdDrawIndirect                              = ::vkCmdDrawIndirect;
    PFN_vkCmdDrawIndexedIndirect                        Anvil::Vulkan::vkCmdDrawIndexedIndirect                       = ::vkCmdDrawIndexedIndirect;
    PFN_vkCmdDispatch                                   Anvil::Vulkan::vkCmdDispatch                                  = ::vkCmdDispatch;
    PFN_vkCmdDispatchIndirect                           Anvil::Vulkan::vkCmdDispatchIndirect                          = ::vkCmdDispatchIndirect;
    PFN_vkCmdCopyBuffer                                 Anvil::Vulkan::vkCmdCopyBuffer                                = ::vkCmdCopyBuffer;
    PFN_vkCmdCopyImage                                  Anvil::Vulkan::vkCmdCopyImage                                 = ::vkCmdCopyImage;
    PFN_vkCmdBlitImage                                  Anvil::Vulkan::vkCmdBlitImage                                 = ::vkCmdBlitImage;
    PFN_vkCmdCopyBufferToImage                          Anvil::Vulkan::vkCmdCopyBufferToImage                         = ::vkCmdCopyBufferToImage;
    PFN_vkCmdCopyImageToBuffer                          Anvil::Vulkan::vkCmdCopyImageToBuffer                         = ::vkCmdCopyImageToBuffer;
    PFN_vkCmdUpdateBuffer                               Anvil::Vulkan::vkCmdUpdateBuffer                              = ::vkCmdUpdateBuffer;
    PFN_vkCmdFillBuffer                                 Anvil::Vulkan::vkCmdFillBuffer                                = ::vkCmdFillBuffer;
    PFN_vkCmdClearColorImage                            Anvil::Vulkan::vkCmdClearColorImage                           = ::vkCmdClearColorImage;
    PFN_vkCmdClearDepthStencilImage                     Anvil::Vulkan::vkCmdClearDepthStencilImage                    = ::vkCmdClearDepthStencilImage;
    PFN_vkCmdClearAttachments                           Anvil::Vulkan::vkCmdClearAttachments                          = ::vkCmdClearAttachments;
    PFN_vkCmdResolveImage                               Anvil::Vulkan::vkCmdResolveImage                              = ::vkCmdResolveImage;
    PFN_vkCmdSetEvent                                   Anvil::Vulkan::vkCmdSetEvent                                  = ::vkCmdSetEvent;
    PFN_vkCmdResetEvent                                 Anvil::Vulkan::vkCmdResetEvent                                = ::vkCmdResetEvent;
    PFN_vkCmdWaitEvents                                 Anvil::Vulkan::vkCmdWaitEvents                                = ::vkCmdWaitEvents;
    PFN_vkCmdPipelineBarrier                            Anvil::Vulkan::vkCmdPipelineBarrier                           = ::vkCmdPipelineBarrier;
    PFN_vkCmdBeginQuery                                 Anvil::Vulkan::vkCmdBeginQuery                                = ::vkCmdBeginQuery;
    PFN_vkCmdEndQuery                                   Anvil::Vulkan::vkCmdEndQuery                                  = ::vkCmdEndQuery;
    PFN_vkCmdResetQueryPool                             Anvil::Vulkan::vkCmdResetQueryPool                            = ::vkCmdResetQueryPool;
    PFN_vkCmdWriteTimestamp                             Anvil::Vulkan::vkCmdWriteTimestamp                            = ::vkCmdWriteTimestamp;
    PFN_vkCmdCopyQueryPoolResults                       Anvil::Vulkan::vkCmdCopyQueryPoolResults                      = ::vkCmdCopyQueryPoolResults;
    PFN_vkCmdPushConstants                              Anvil::Vulkan::vkCmdPushConstants                             = ::vkCmdPushConstants;
    PFN_vkCmdBeginRenderPass                            Anvil::Vulkan::vkCmdBeginRenderPass                           = ::vkCmdBeginRenderPass;
    PFN_vkCmdNextSubpass                                Anvil::Vulkan::vkCmdNextSubpass                               = ::vkCmdNextSubpass;
    PFN_vkCmdEndRenderPass                              Anvil::Vulkan::vkCmdEndRenderPass                             = ::vkCmdEndRenderPass;
    PFN_vkCmdExecuteCommands                            Anvil::Vulkan::vkCmdExecuteCommands                           = ::vkCmdExecuteCommands;

    PFN_vkBindBufferMemory2                             Anvil::Vulkan::vkBindBufferMemory2                             = nullptr;
    PFN_vkBindImageMemory2                              Anvil::Vulkan::vkBindImageMemory2                              = nullptr;
    PFN_vkCmdDispatchBase                               Anvil::Vulkan::vkCmdDispatchBase                               = nullptr;
    PFN_vkCmdSetDeviceMask                              Anvil::Vulkan::vkCmdSetDeviceMask                              = nullptr;
    PFN_vkCreateDescriptorUpdateTemplate                Anvil::Vulkan::vkCreateDescriptorUpdateTemplate                = nullptr;
    PFN_vkCreateSamplerYcbcrConversion                  Anvil::Vulkan::vkCreateSamplerYcbcrConversion                  = nullptr;
    PFN_vkDestroyDescriptorUpdateTemplate               Anvil::Vulkan::vkDestroyDescriptorUpdateTemplate               = nullptr;
    PFN_vkDestroySamplerYcbcrConversion                 Anvil::Vulkan::vkDestroySamplerYcbcrConversion                 = nullptr;
    PFN_vkEnumerateInstanceVersion                      Anvil::Vulkan::vkEnumerateInstanceVersion                      = nullptr;
    PFN_vkEnumeratePhysicalDeviceGroups                 Anvil::Vulkan::vkEnumeratePhysicalDeviceGroups                 = nullptr;
    PFN_vkGetBufferMemoryRequirements2                  Anvil::Vulkan::vkGetBufferMemoryRequirements2                  = nullptr;
    PFN_vkGetDescriptorSetLayoutSupport                 Anvil::Vulkan::vkGetDescriptorSetLayoutSupport                 = nullptr;
    PFN_vkGetDeviceGroupPeerMemoryFeatures              Anvil::Vulkan::vkGetDeviceGroupPeerMemoryFeatures              = nullptr;
    PFN_vkGetDeviceQueue2                               Anvil::Vulkan::vkGetDeviceQueue2                               = nullptr;
    PFN_vkGetImageMemoryRequirements2                   Anvil::Vulkan::vkGetImageMemoryRequirements2                   = nullptr;
    PFN_vkGetImageSparseMemoryRequirements2             Anvil::Vulkan::vkGetImageSparseMemoryRequirements2             = nullptr;
    PFN_vkGetPhysicalDeviceExternalBufferProperties     Anvil::Vulkan::vkGetPhysicalDeviceExternalBufferProperties     = nullptr;
    PFN_vkGetPhysicalDeviceExternalFenceProperties      Anvil::Vulkan::vkGetPhysicalDeviceExternalFenceProperties      = nullptr;
    PFN_vkGetPhysicalDeviceExternalSemaphoreProperties  Anvil::Vulkan::vkGetPhysicalDeviceExternalSemaphoreProperties  = nullptr;
    PFN_vkGetPhysicalDeviceFeatures2                    Anvil::Vulkan::vkGetPhysicalDeviceFeatures2                    = nullptr;
    PFN_vkGetPhysicalDeviceFormatProperties2            Anvil::Vulkan::vkGetPhysicalDeviceFormatProperties2            = nullptr;
    PFN_vkGetPhysicalDeviceImageFormatProperties2       Anvil::Vulkan::vkGetPhysicalDeviceImageFormatProperties2       = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties2            Anvil::Vulkan::vkGetPhysicalDeviceMemoryProperties2            = nullptr;
    PFN_vkGetPhysicalDeviceProperties2                  Anvil::Vulkan::vkGetPhysicalDeviceProperties2                  = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2       Anvil::Vulkan::vkGetPhysicalDeviceQueueFamilyProperties2       = nullptr;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 Anvil::Vulkan::vkGetPhysicalDeviceSparseImageFormatProperties2 = nullptr;
    PFN_vkTrimCommandPool                               Anvil::Vulkan::vkTrimCommandPool                               = nullptr;
    PFN_vkUpdateDescriptorSetWithTemplate               Anvil::Vulkan::vkUpdateDescriptorSetWithTemplate               = nullptr;
#endif
