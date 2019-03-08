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
#ifndef MISC_VULKAN_H
#define MISC_VULKAN_H

#include <config.h>
#include "vulkan/vulkan.h"

namespace Anvil
{
    /* Anvil::Vulkan exposes raw pointers to Vulkan entrypoints.
     *
     * These func ptrs are initialized, first time a Vulkan instance is created. Applications MUST NOT
     * assume the entrypoints are available prior to the time.
     */
    namespace Vulkan
    {
        /* VK 1.0 core */
        extern PFN_vkCreateInstance                                vkCreateInstance;
        extern PFN_vkDestroyInstance                               vkDestroyInstance;
        extern PFN_vkEnumeratePhysicalDevices                      vkEnumeratePhysicalDevices;
        extern PFN_vkGetPhysicalDeviceFeatures                     vkGetPhysicalDeviceFeatures;
        extern PFN_vkGetPhysicalDeviceFormatProperties             vkGetPhysicalDeviceFormatProperties;
        extern PFN_vkGetPhysicalDeviceImageFormatProperties        vkGetPhysicalDeviceImageFormatProperties;
        extern PFN_vkGetPhysicalDeviceProperties                   vkGetPhysicalDeviceProperties;
        extern PFN_vkGetPhysicalDeviceQueueFamilyProperties        vkGetPhysicalDeviceQueueFamilyProperties;
        extern PFN_vkGetPhysicalDeviceMemoryProperties             vkGetPhysicalDeviceMemoryProperties;
        extern PFN_vkGetInstanceProcAddr                           vkGetInstanceProcAddr;
        extern PFN_vkGetDeviceProcAddr                             vkGetDeviceProcAddr;
        extern PFN_vkCreateDevice                                  vkCreateDevice;
        extern PFN_vkDestroyDevice                                 vkDestroyDevice;
        extern PFN_vkEnumerateInstanceExtensionProperties          vkEnumerateInstanceExtensionProperties;
        extern PFN_vkEnumerateDeviceExtensionProperties            vkEnumerateDeviceExtensionProperties;
        extern PFN_vkEnumerateInstanceLayerProperties              vkEnumerateInstanceLayerProperties;
        extern PFN_vkEnumerateDeviceLayerProperties                vkEnumerateDeviceLayerProperties;
        extern PFN_vkGetDeviceQueue                                vkGetDeviceQueue;
        extern PFN_vkQueueSubmit                                   vkQueueSubmit;
        extern PFN_vkQueueWaitIdle                                 vkQueueWaitIdle;
        extern PFN_vkDeviceWaitIdle                                vkDeviceWaitIdle;
        extern PFN_vkAllocateMemory                                vkAllocateMemory;
        extern PFN_vkFreeMemory                                    vkFreeMemory;
        extern PFN_vkMapMemory                                     vkMapMemory;
        extern PFN_vkUnmapMemory                                   vkUnmapMemory;
        extern PFN_vkFlushMappedMemoryRanges                       vkFlushMappedMemoryRanges;
        extern PFN_vkInvalidateMappedMemoryRanges                  vkInvalidateMappedMemoryRanges;
        extern PFN_vkGetDeviceMemoryCommitment                     vkGetDeviceMemoryCommitment;
        extern PFN_vkBindBufferMemory                              vkBindBufferMemory;
        extern PFN_vkBindImageMemory                               vkBindImageMemory;
        extern PFN_vkGetBufferMemoryRequirements                   vkGetBufferMemoryRequirements;
        extern PFN_vkGetImageMemoryRequirements                    vkGetImageMemoryRequirements;
        extern PFN_vkGetImageSparseMemoryRequirements              vkGetImageSparseMemoryRequirements;
        extern PFN_vkGetPhysicalDeviceSparseImageFormatProperties  vkGetPhysicalDeviceSparseImageFormatProperties;
        extern PFN_vkQueueBindSparse                               vkQueueBindSparse;
        extern PFN_vkCreateFence                                   vkCreateFence;
        extern PFN_vkDestroyFence                                  vkDestroyFence;
        extern PFN_vkResetFences                                   vkResetFences;
        extern PFN_vkGetFenceStatus                                vkGetFenceStatus;
        extern PFN_vkWaitForFences                                 vkWaitForFences;
        extern PFN_vkCreateSemaphore                               vkCreateSemaphore;
        extern PFN_vkDestroySemaphore                              vkDestroySemaphore;
        extern PFN_vkCreateEvent                                   vkCreateEvent;
        extern PFN_vkDestroyEvent                                  vkDestroyEvent;
        extern PFN_vkGetEventStatus                                vkGetEventStatus;
        extern PFN_vkSetEvent                                      vkSetEvent;
        extern PFN_vkResetEvent                                    vkResetEvent;
        extern PFN_vkCreateQueryPool                               vkCreateQueryPool;
        extern PFN_vkDestroyQueryPool                              vkDestroyQueryPool;
        extern PFN_vkGetQueryPoolResults                           vkGetQueryPoolResults;
        extern PFN_vkCreateBuffer                                  vkCreateBuffer;
        extern PFN_vkDestroyBuffer                                 vkDestroyBuffer;
        extern PFN_vkCreateBufferView                              vkCreateBufferView;
        extern PFN_vkDestroyBufferView                             vkDestroyBufferView;
        extern PFN_vkCreateImage                                   vkCreateImage;
        extern PFN_vkDestroyImage                                  vkDestroyImage;
        extern PFN_vkGetImageSubresourceLayout                     vkGetImageSubresourceLayout;
        extern PFN_vkCreateImageView                               vkCreateImageView;
        extern PFN_vkDestroyImageView                              vkDestroyImageView;
        extern PFN_vkCreateShaderModule                            vkCreateShaderModule;
        extern PFN_vkDestroyShaderModule                           vkDestroyShaderModule;
        extern PFN_vkCreatePipelineCache                           vkCreatePipelineCache;
        extern PFN_vkDestroyPipelineCache                          vkDestroyPipelineCache;
        extern PFN_vkGetPipelineCacheData                          vkGetPipelineCacheData;
        extern PFN_vkMergePipelineCaches                           vkMergePipelineCaches;
        extern PFN_vkCreateGraphicsPipelines                       vkCreateGraphicsPipelines;
        extern PFN_vkCreateComputePipelines                        vkCreateComputePipelines;
        extern PFN_vkDestroyPipeline                               vkDestroyPipeline;
        extern PFN_vkCreatePipelineLayout                          vkCreatePipelineLayout;
        extern PFN_vkDestroyPipelineLayout                         vkDestroyPipelineLayout;
        extern PFN_vkCreateSampler                                 vkCreateSampler;
        extern PFN_vkDestroySampler                                vkDestroySampler;
        extern PFN_vkCreateDescriptorSetLayout                     vkCreateDescriptorSetLayout;
        extern PFN_vkDestroyDescriptorSetLayout                    vkDestroyDescriptorSetLayout;
        extern PFN_vkCreateDescriptorPool                          vkCreateDescriptorPool;
        extern PFN_vkDestroyDescriptorPool                         vkDestroyDescriptorPool;
        extern PFN_vkResetDescriptorPool                           vkResetDescriptorPool;
        extern PFN_vkAllocateDescriptorSets                        vkAllocateDescriptorSets;
        extern PFN_vkFreeDescriptorSets                            vkFreeDescriptorSets;
        extern PFN_vkUpdateDescriptorSets                          vkUpdateDescriptorSets;
        extern PFN_vkCreateFramebuffer                             vkCreateFramebuffer;
        extern PFN_vkDestroyFramebuffer                            vkDestroyFramebuffer;
        extern PFN_vkCreateRenderPass                              vkCreateRenderPass;
        extern PFN_vkDestroyRenderPass                             vkDestroyRenderPass;
        extern PFN_vkGetRenderAreaGranularity                      vkGetRenderAreaGranularity;
        extern PFN_vkCreateCommandPool                             vkCreateCommandPool;
        extern PFN_vkDestroyCommandPool                            vkDestroyCommandPool;
        extern PFN_vkResetCommandPool                              vkResetCommandPool;
        extern PFN_vkAllocateCommandBuffers                        vkAllocateCommandBuffers;
        extern PFN_vkFreeCommandBuffers                            vkFreeCommandBuffers;
        extern PFN_vkBeginCommandBuffer                            vkBeginCommandBuffer;
        extern PFN_vkEndCommandBuffer                              vkEndCommandBuffer;
        extern PFN_vkResetCommandBuffer                            vkResetCommandBuffer;
        extern PFN_vkCmdBindPipeline                               vkCmdBindPipeline;
        extern PFN_vkCmdSetViewport                                vkCmdSetViewport;
        extern PFN_vkCmdSetScissor                                 vkCmdSetScissor;
        extern PFN_vkCmdSetLineWidth                               vkCmdSetLineWidth;
        extern PFN_vkCmdSetDepthBias                               vkCmdSetDepthBias;
        extern PFN_vkCmdSetBlendConstants                          vkCmdSetBlendConstants;
        extern PFN_vkCmdSetDepthBounds                             vkCmdSetDepthBounds;
        extern PFN_vkCmdSetStencilCompareMask                      vkCmdSetStencilCompareMask;
        extern PFN_vkCmdSetStencilWriteMask                        vkCmdSetStencilWriteMask;
        extern PFN_vkCmdSetStencilReference                        vkCmdSetStencilReference;
        extern PFN_vkCmdBindDescriptorSets                         vkCmdBindDescriptorSets;
        extern PFN_vkCmdBindIndexBuffer                            vkCmdBindIndexBuffer;
        extern PFN_vkCmdBindVertexBuffers                          vkCmdBindVertexBuffers;
        extern PFN_vkCmdDraw                                       vkCmdDraw;
        extern PFN_vkCmdDrawIndexed                                vkCmdDrawIndexed;
        extern PFN_vkCmdDrawIndirect                               vkCmdDrawIndirect;
        extern PFN_vkCmdDrawIndexedIndirect                        vkCmdDrawIndexedIndirect;
        extern PFN_vkCmdDispatch                                   vkCmdDispatch;
        extern PFN_vkCmdDispatchIndirect                           vkCmdDispatchIndirect;
        extern PFN_vkCmdCopyBuffer                                 vkCmdCopyBuffer;
        extern PFN_vkCmdCopyImage                                  vkCmdCopyImage;
        extern PFN_vkCmdBlitImage                                  vkCmdBlitImage;
        extern PFN_vkCmdCopyBufferToImage                          vkCmdCopyBufferToImage;
        extern PFN_vkCmdCopyImageToBuffer                          vkCmdCopyImageToBuffer;
        extern PFN_vkCmdUpdateBuffer                               vkCmdUpdateBuffer;
        extern PFN_vkCmdFillBuffer                                 vkCmdFillBuffer;
        extern PFN_vkCmdClearColorImage                            vkCmdClearColorImage;
        extern PFN_vkCmdClearDepthStencilImage                     vkCmdClearDepthStencilImage;
        extern PFN_vkCmdClearAttachments                           vkCmdClearAttachments;
        extern PFN_vkCmdResolveImage                               vkCmdResolveImage;
        extern PFN_vkCmdSetEvent                                   vkCmdSetEvent;
        extern PFN_vkCmdResetEvent                                 vkCmdResetEvent;
        extern PFN_vkCmdWaitEvents                                 vkCmdWaitEvents;
        extern PFN_vkCmdPipelineBarrier                            vkCmdPipelineBarrier;
        extern PFN_vkCmdBeginQuery                                 vkCmdBeginQuery;
        extern PFN_vkCmdEndQuery                                   vkCmdEndQuery;
        extern PFN_vkCmdResetQueryPool                             vkCmdResetQueryPool;
        extern PFN_vkCmdWriteTimestamp                             vkCmdWriteTimestamp;
        extern PFN_vkCmdCopyQueryPoolResults                       vkCmdCopyQueryPoolResults;
        extern PFN_vkCmdPushConstants                              vkCmdPushConstants;
        extern PFN_vkCmdBeginRenderPass                            vkCmdBeginRenderPass;
        extern PFN_vkCmdNextSubpass                                vkCmdNextSubpass;
        extern PFN_vkCmdEndRenderPass                              vkCmdEndRenderPass;
        extern PFN_vkCmdExecuteCommands                            vkCmdExecuteCommands;

        /* VK 1.1 core - only available if implementation reports VK 1.1 support!
         *
         * These function pointers are always retrieved at run-time.
         */
        extern PFN_vkBindBufferMemory2                             vkBindBufferMemory2;
        extern PFN_vkBindImageMemory2                              vkBindImageMemory2;
        extern PFN_vkCmdDispatchBase                               vkCmdDispatchBase;
        extern PFN_vkCmdSetDeviceMask                              vkCmdSetDeviceMask;
        extern PFN_vkCreateDescriptorUpdateTemplate                vkCreateDescriptorUpdateTemplate;
        extern PFN_vkCreateSamplerYcbcrConversion                  vkCreateSamplerYcbcrConversion;
        extern PFN_vkDestroyDescriptorUpdateTemplate               vkDestroyDescriptorUpdateTemplate;
        extern PFN_vkDestroySamplerYcbcrConversion                 vkDestroySamplerYcbcrConversion;
        extern PFN_vkEnumerateInstanceVersion                      vkEnumerateInstanceVersion;
        extern PFN_vkEnumeratePhysicalDeviceGroups                 vkEnumeratePhysicalDeviceGroups;
        extern PFN_vkGetBufferMemoryRequirements2                  vkGetBufferMemoryRequirements2;
        extern PFN_vkGetDescriptorSetLayoutSupport                 vkGetDescriptorSetLayoutSupport;
        extern PFN_vkGetDeviceGroupPeerMemoryFeatures              vkGetDeviceGroupPeerMemoryFeatures;
        extern PFN_vkGetDeviceQueue2                               vkGetDeviceQueue2;
        extern PFN_vkGetImageMemoryRequirements2                   vkGetImageMemoryRequirements2;
        extern PFN_vkGetImageSparseMemoryRequirements2             vkGetImageSparseMemoryRequirements2;
        extern PFN_vkGetPhysicalDeviceExternalBufferProperties     vkGetPhysicalDeviceExternalBufferProperties;
        extern PFN_vkGetPhysicalDeviceExternalFenceProperties      vkGetPhysicalDeviceExternalFenceProperties;
        extern PFN_vkGetPhysicalDeviceExternalSemaphoreProperties  vkGetPhysicalDeviceExternalSemaphoreProperties;
        extern PFN_vkGetPhysicalDeviceFeatures2                    vkGetPhysicalDeviceFeatures2;
        extern PFN_vkGetPhysicalDeviceFormatProperties2            vkGetPhysicalDeviceFormatProperties2;
        extern PFN_vkGetPhysicalDeviceImageFormatProperties2       vkGetPhysicalDeviceImageFormatProperties2;
        extern PFN_vkGetPhysicalDeviceMemoryProperties2            vkGetPhysicalDeviceMemoryProperties2;
        extern PFN_vkGetPhysicalDeviceProperties2                  vkGetPhysicalDeviceProperties2;
        extern PFN_vkGetPhysicalDeviceQueueFamilyProperties2       vkGetPhysicalDeviceQueueFamilyProperties2;
        extern PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 vkGetPhysicalDeviceSparseImageFormatProperties2;
        extern PFN_vkTrimCommandPool                               vkTrimCommandPool;
        extern PFN_vkUpdateDescriptorSetWithTemplate               vkUpdateDescriptorSetWithTemplate;

        /* Func pointers to extensions are exposed to apps via relevant functions implemented by Anvil::*Device and Anvil::Instance. */
    }
}

#endif /* MISC_VULKAN_H */
