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
#ifndef MISC_TYPES_H
#define MISC_TYPES_H

#include <array>
#include <atomic>
#include <climits>
#include <cstdio>
#include <forward_list>
#include <mutex>
#include <string>

#include "config.h"
#include "misc/debug.h"

/* Disable some of the warnings we cannot work around because they are caused
 * by external dependencies (ie. Vulkan header)
 */
#ifdef _MSC_VER
    #pragma warning(disable : 4063)
#else
    #pragma GCC diagnostic ignored "-Wswitch"
    #pragma GCC diagnostic ignored "-Wreorder"
    #pragma GCC diagnostic ignored "-Wunused-value"
#endif

/* Determine endianness */
#if REG_DWORD == REG_DWORD_LITTLE_ENDIAN || __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define ANVIL_LITTLE_ENDIAN
#endif

/* The following #define is required to include Vulkan entry-point prototype declarations. */
#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#else
    #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        #define VK_USE_PLATFORM_XCB_KHR
    #endif
#endif

#ifdef _WIN32
    /* NOTE: Version clamp required for IsDebuggerPresent() */
    #define ANVIL_MIN_WIN32_WINNT_REQUIRED 0x0501

    #if !defined(_WIN32_WINNT)
        #define _WIN32_WINNT ANVIL_MIN_WIN32_WINNT_REQUIRED
    #else
        #if _WIN32_WINNT < ANVIL_MIN_WIN32_WINNT_REQUIRED
            #error Please update the _WIN32_WINNT macro in order for Anvil to compile successfully.
        #endif
    #endif

    #if _MSC_VER <= 1800
        #ifndef snprintf
            #define snprintf _snprintf
        #endif
    #endif

    #include <windows.h>

    #if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        typedef HWND WindowHandle;
    #else
        typedef void* WindowHandle;
    #endif
#else
    #if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        #include "xcb_loader.h"

        typedef xcb_window_t WindowHandle;
    #else
        typedef void* WindowHandle;
    #endif

    #include <string.h>
#endif

#include "misc/vulkan.h"
#include "vulkan/vk_platform.h"

#include <map>
#include <memory>
#include <vector>

/* Forward declarations */
namespace Anvil
{
    class  BaseDevice;
    class  BasePipelineCreateInfo;
    class  Buffer;
    class  BufferCreateInfo;
    class  BufferView;
    class  BufferViewCreateInfo;
    struct CallbackArgument;
    class  CommandBufferBase;
    class  CommandPool;
    class  ComputePipelineCreateInfo;
    class  ComputePipelineManager;
    class  DebugMessenger;
    class  DebugMessengerCreateInfo;
    class  DescriptorPool;
    class  DescriptorPoolCreateInfo;
    class  DescriptorSet;
    class  DescriptorSetCreateInfo;
    class  DescriptorSetGroup;
    class  DescriptorSetLayout;
    class  DescriptorSetLayoutManager;
    class  DescriptorUpdateTemplate;
    class  DeviceCreateInfo;
    class  ExternalHandle;
    class  Event;
    class  EventCreateInfo;
    class  Fence;
    class  FenceCreateInfo;
    class  Framebuffer;
    class  FramebufferCreateInfo;
    class  GLSLShaderToSPIRVGenerator;
    class  GraphicsPipelineCreateInfo;
    class  GraphicsPipelineManager;
    class  Image;
    class  ImageCreateInfo;
    class  ImageView;
    class  ImageViewCreateInfo;
    class  Instance;
    class  InstanceCreateInfo;
    class  MemoryAllocator;
    class  MemoryBlock;
    class  MemoryBlockCreateInfo;
    struct MemoryHeap;
    struct MemoryProperties;
    struct MemoryType;
    class  MGPUDevice;
    class  PhysicalDevice;
    class  PipelineCache;
    class  PipelineLayout;
    class  PipelineLayoutManager;
    class  PrimaryCommandBuffer;
    class  QueryPool;
    class  Queue;
    class  RenderingSurface;
    class  RenderingSurfaceCreateInfo;
    class  RenderPass;
    class  RenderPassCreateInfo;
    class  Sampler;
    class  SamplerCreateInfo;
    class  SamplerYCbCrConversion;
    class  SamplerYCbCrConversionCreateInfo;
    class  SecondaryCommandBuffer;
    class  Semaphore;
    class  SemaphoreCreateInfo;
    class  SGPUDevice;
    class  ShaderModule;
    class  ShaderModuleCache;
    class  Swapchain;
    class  SwapchainCreateInfo;
    class  Window;

    typedef std::unique_ptr<BaseDevice,                            std::function<void(BaseDevice*)> >                  BaseDeviceUniquePtr;
    typedef std::unique_ptr<BasePipelineCreateInfo>                                                                    BasePipelineCreateInfoUniquePtr;
    typedef std::unique_ptr<BufferCreateInfo>                                                                          BufferCreateInfoUniquePtr;
    typedef std::unique_ptr<Buffer,                                std::function<void(Buffer*)> >                      BufferUniquePtr;
    typedef std::unique_ptr<BufferViewCreateInfo>                                                                      BufferViewCreateInfoUniquePtr;
    typedef std::unique_ptr<BufferView,                            std::function<void(BufferView*)> >                  BufferViewUniquePtr;
    typedef std::unique_ptr<CommandBufferBase,                     std::function<void(CommandBufferBase*)> >           CommandBufferBaseUniquePtr;
    typedef std::unique_ptr<CommandPool,                           std::function<void(CommandPool*)> >                 CommandPoolUniquePtr;
    typedef std::unique_ptr<ComputePipelineCreateInfo>                                                                 ComputePipelineCreateInfoUniquePtr;
    typedef std::unique_ptr<DebugMessengerCreateInfo>                                                                  DebugMessengerCreateInfoUniquePtr;
    typedef std::unique_ptr<DebugMessenger,                        std::function<void(DebugMessenger*)> >              DebugMessengerUniquePtr;
    typedef std::unique_ptr<DescriptorPoolCreateInfo>                                                                  DescriptorPoolCreateInfoUniquePtr;
    typedef std::unique_ptr<DescriptorPool,                        std::function<void(DescriptorPool*)> >              DescriptorPoolUniquePtr;
    typedef std::unique_ptr<DescriptorSetCreateInfo>                                                                   DescriptorSetCreateInfoUniquePtr;
    typedef std::unique_ptr<DescriptorSetGroup,                    std::function<void(DescriptorSetGroup*)> >          DescriptorSetGroupUniquePtr;
    typedef std::unique_ptr<DescriptorSetLayout,                   std::function<void(DescriptorSetLayout*)> >         DescriptorSetLayoutUniquePtr;
    typedef std::unique_ptr<DescriptorSetLayoutManager,            std::function<void(DescriptorSetLayoutManager*)> >  DescriptorSetLayoutManagerUniquePtr;
    typedef std::unique_ptr<DescriptorSet,                         std::function<void(DescriptorSet*)> >               DescriptorSetUniquePtr;
    typedef std::unique_ptr<DescriptorUpdateTemplate,              std::function<void(DescriptorUpdateTemplate*)> >    DescriptorUpdateTemplateUniquePtr;
    typedef std::unique_ptr<DeviceCreateInfo>                                                                          DeviceCreateInfoUniquePtr;
    typedef std::unique_ptr<ExternalHandle,                        std::function<void(ExternalHandle*)> >              ExternalHandleUniquePtr;
    typedef std::unique_ptr<EventCreateInfo>                                                                           EventCreateInfoUniquePtr;
    typedef std::unique_ptr<Event,                                 std::function<void(Event*)> >                       EventUniquePtr;
    typedef std::unique_ptr<FenceCreateInfo>                                                                           FenceCreateInfoUniquePtr;
    typedef std::unique_ptr<Fence,                                 std::function<void(Fence*)> >                       FenceUniquePtr;
    typedef std::unique_ptr<FramebufferCreateInfo>                                                                     FramebufferCreateInfoUniquePtr;
    typedef std::unique_ptr<Framebuffer,                           std::function<void(Framebuffer*)> >                 FramebufferUniquePtr;
    typedef std::unique_ptr<GLSLShaderToSPIRVGenerator,            std::function<void(GLSLShaderToSPIRVGenerator*)> >  GLSLShaderToSPIRVGeneratorUniquePtr;
    typedef std::unique_ptr<GraphicsPipelineCreateInfo>                                                                GraphicsPipelineCreateInfoUniquePtr;
    typedef std::unique_ptr<GraphicsPipelineManager>                                                                   GraphicsPipelineManagerUniquePtr;
    typedef std::unique_ptr<ImageCreateInfo>                                                                           ImageCreateInfoUniquePtr;
    typedef std::unique_ptr<Image,                                 std::function<void(Image*)> >                       ImageUniquePtr;
    typedef std::unique_ptr<ImageViewCreateInfo>                                                                       ImageViewCreateInfoUniquePtr;
    typedef std::unique_ptr<ImageView,                             std::function<void(ImageView*)> >                   ImageViewUniquePtr;
    typedef std::unique_ptr<InstanceCreateInfo>                                                                        InstanceCreateInfoUniquePtr;
    typedef std::unique_ptr<Instance,                              std::function<void(Instance*)> >                    InstanceUniquePtr;
    typedef std::unique_ptr<MemoryAllocator,                       std::function<void(MemoryAllocator*)> >             MemoryAllocatorUniquePtr;
    typedef std::unique_ptr<MemoryBlockCreateInfo>                                                                     MemoryBlockCreateInfoUniquePtr;
    typedef std::unique_ptr<MemoryBlock,                           std::function<void(MemoryBlock*)> >                 MemoryBlockUniquePtr;
    typedef std::unique_ptr<MGPUDevice,                            std::function<void(MGPUDevice*)> >                  MGPUDeviceUniquePtr;
    typedef std::unique_ptr<PipelineCache,                         std::function<void(PipelineCache*)> >               PipelineCacheUniquePtr;
    typedef std::unique_ptr<PipelineLayoutManager,                 std::function<void(PipelineLayoutManager*)> >       PipelineLayoutManagerUniquePtr;
    typedef std::unique_ptr<PipelineLayout,                        std::function<void(PipelineLayout*)> >              PipelineLayoutUniquePtr;
    typedef std::unique_ptr<PrimaryCommandBuffer,                  std::function<void(PrimaryCommandBuffer*)> >        PrimaryCommandBufferUniquePtr;
    typedef std::unique_ptr<QueryPool,                             std::function<void(QueryPool*)> >                   QueryPoolUniquePtr;
    typedef std::unique_ptr<RenderingSurface,                      std::function<void(RenderingSurface*)> >            RenderingSurfaceUniquePtr;
    typedef std::unique_ptr<RenderingSurfaceCreateInfo>                                                                RenderingSurfaceCreateInfoUniquePtr;
    typedef std::unique_ptr<RenderPassCreateInfo>                                                                      RenderPassCreateInfoUniquePtr;
    typedef std::unique_ptr<RenderPass,                            std::function<void(RenderPass*)> >                  RenderPassUniquePtr;
    typedef std::unique_ptr<SamplerCreateInfo>                                                                         SamplerCreateInfoUniquePtr;
    typedef std::unique_ptr<Sampler,                               std::function<void(Sampler*)> >                     SamplerUniquePtr;
    typedef std::unique_ptr<SamplerYCbCrConversionCreateInfo>                                                          SamplerYCbCrConversionCreateInfoUniquePtr;
    typedef std::unique_ptr<SamplerYCbCrConversion,                std::function<void(SamplerYCbCrConversion*)> >      SamplerYCbCrConversionUniquePtr;
    typedef std::unique_ptr<SecondaryCommandBuffer,                std::function<void(SecondaryCommandBuffer*)> >      SecondaryCommandBufferUniquePtr;
    typedef std::unique_ptr<SemaphoreCreateInfo>                                                                       SemaphoreCreateInfoUniquePtr;
    typedef std::unique_ptr<Semaphore,                             std::function<void(Semaphore*)> >                   SemaphoreUniquePtr;
    typedef std::unique_ptr<SGPUDevice,                            std::function<void(SGPUDevice*)> >                  SGPUDeviceUniquePtr;
    typedef std::unique_ptr<ShaderModuleCache,                     std::function<void(ShaderModuleCache*)> >           ShaderModuleCacheUniquePtr;
    typedef std::unique_ptr<ShaderModule,                          std::function<void(ShaderModule*)> >                ShaderModuleUniquePtr;
    typedef std::unique_ptr<SwapchainCreateInfo>                                                                       SwapchainCreateInfoUniquePtr;
    typedef std::unique_ptr<Swapchain,                             std::function<void(Swapchain*)> >                   SwapchainUniquePtr;
    typedef std::unique_ptr<Window,                                std::function<void(Window*)> >                      WindowUniquePtr;
};

/* Defines various types used by Vulkan API wrapper classes. */
namespace Anvil
{
    #if defined(_WIN32)
        typedef HANDLE ExternalHandleType;
    #else
        typedef int ExternalHandleType;
    #endif

    /** ID of an Anvil framebuffer's attachment */
    typedef uint32_t FramebufferAttachmentID;

    typedef uint32_t            BindingElementIndex;
    typedef uint32_t            BindingIndex;
    typedef uint32_t            NumberOfBindingElements;
    typedef BindingElementIndex StartBindingElementIndex;

    typedef std::pair<StartBindingElementIndex, NumberOfBindingElements> BindingElementArrayRange;

    /** "About to be deleted" call-back function prototype. */
    typedef std::function<void (Anvil::MemoryBlock* in_memory_block_ptr)> OnMemoryBlockReleaseCallbackFunction;

    /** Base pipeline ID. Internal type, used to represent compute / graphics pipeline IDs */
    typedef uint32_t PipelineID;

    /* Index of a query within parent query pool instance */
    typedef uint32_t QueryIndex;

    /* Unique ID of a render-pass attachment within scope of a RenderPass instance. */
    typedef uint32_t RenderPassAttachmentID;

    /* Unique ID of a sparse memory bind update */
    typedef uint32_t SparseMemoryBindInfoID;

    /* Unique ID of a render-pass' sub-pass attachment within scope of a RenderPass instance. */
    typedef uint32_t SubPassAttachmentID;

    /* Unique ID of a sub-pass within scope of a RenderPass instance. */
    typedef uint32_t SubPassID;
};

#include "misc/types_enums.h"
#include "misc/types_macro.h"

#include "misc/types_classes.h"
#include "misc/types_struct.h"
#include "misc/types_utils.h"

#endif /* MISC_TYPES_H */
