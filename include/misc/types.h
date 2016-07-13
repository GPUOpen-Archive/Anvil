//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
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

/* The following #define is required to include Vulkan entry-point prototypes. */
#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#else
    #define VK_USE_PLATFORM_XCB_KHR
#endif


#ifdef _WIN32
    #if _MSC_VER <= 1800
        #ifndef snprintf
            #define snprintf _snprintf
        #endif
    #endif

    #include <windows.h>

    typedef HWND WindowHandle;
#else
    #include "xcb_loader_for_anvil.h"
    #include <string.h>

    #ifndef nullptr
        #define nullptr NULL
    #endif

    typedef xcb_window_t WindowHandle;
#endif

#ifdef _WIN32
    #include "vulkan\vulkan.h"
    #include "vulkan\vk_sdk_platform.h"
#else
    #include "vulkan/vulkan.h"
    #include "vulkan/vk_sdk_platform.h"
#endif


#include <map>
#include <vector>

/* Defines various enums used by Vulkan API wrapper classes. */
namespace Anvil
{
    /* Forward declarations */
    class  Buffer;
    class  BufferView;
    class  CommandBufferBase;
    class  CommandPool;
    class  ComputePipelineManager;
    class  DAGRenderer;
    class  DescriptorPool;
    class  DescriptorSet;
    class  DescriptorSetGroup;
    class  DescriptorSetLayout;
    class  Device;
    class  Event;
    class  Fence;
    class  Framebuffer;
    class  GraphicsPipelineManager;
    class  Image;
    class  ImageView;
    class  Instance;
    class  MemoryAllocator;
    class  MemoryBlock;
    struct MemoryHeap;
    struct MemoryProperties;
    struct MemoryType;
    class  PhysicalDevice;
    class  PipelineCache;
    class  PipelineLayout;
    class  PipelineLayoutManager;
    class  PrimaryCommandBuffer;
    class  PrimaryCommandBufferPool;
    class  QueryPool;
    class  Queue;
    class  RenderingSurface;
    class  RenderPass;
    class  Sampler;
    class  SecondaryCommandBuffer;
    class  SecondaryCommandBufferPool;
    class  Semaphore;
    class  ShaderModule;
    class  Swapchain;
    class  Window;

    /** Describes a buffer memory barrier. */
    typedef struct BufferBarrier
    {
        VkBuffer              buffer;
        VkBufferMemoryBarrier buffer_barrier_vk;
        Anvil::Buffer*       buffer_ptr;
        VkAccessFlagBits      dst_access_mask;
        uint32_t              dst_queue_family_index;
        VkDeviceSize          offset;
        VkDeviceSize          size;
        VkAccessFlagBits      src_access_mask;
        uint32_t              src_queue_family_index;

        /** Constructor.
         *
         *  Note that @param buffer_ptr is retained by this function.
         *
         *  @param in_source_access_mask      Source access mask to use for the barrier.
         *  @param in_destination_access_mask Destination access mask to use for the barrier.
         *  @param in_src_queue_family_index  Source queue family index to use for the barrier.
         *  @param in_dst_queue_family_index  Destination queue family index to use for the barrier.
         *  @param in_buffer_ptr              Pointer to a Buffer instance the instantiated barrier
         *                                    refers to. Must not be nullptr.
         *                                    The object will be retained.
         *  @param in_offset                  Start offset of the region described by the barrier.
         *  @param in_size                    Size of the region described by the barrier.
         **/
        explicit BufferBarrier(VkAccessFlags  in_source_access_mask,
                               VkAccessFlags  in_destination_access_mask,
                               uint32_t       in_src_queue_family_index,
                               uint32_t       in_dst_queue_family_index,
                               Anvil::Buffer* in_buffer_ptr,
                               VkDeviceSize   in_offset,
                               VkDeviceSize   in_size);

        /** Destructor.
         *
         *  Releases the encapsulated Buffer instance.
         **/
        virtual ~BufferBarrier();

        /** Copy constructor.
         *
         *  Retains the Buffer instance stored in the input barrier.
         *
         *  @param in Barrier instance to copy data from.
         **/
        BufferBarrier(const BufferBarrier& in);

        /** Returns a Vulkan buffer memory barrier descriptor, whose configuration corresponds to
         *  to the configuration of this descriptor.
         **/
        virtual VkBufferMemoryBarrier get_barrier_vk() const
        {
            return buffer_barrier_vk;
        }

        /** Returns a pointer to the Vulkan descriptor, whose configuration corresponds to
         *  the configuration of this descriptor.
         *
         *  The returned pointer remains valid for the duration of the Barrier descriptor's
         *  life-time.
         **/
        const VkBufferMemoryBarrier* get_barrier_vk_ptr() const
        {
            return &buffer_barrier_vk;
        }

    private:
        BufferBarrier& operator=(const BufferBarrier&);
    } BufferBarrier;

    /** Describes component layout of a format */
    typedef enum
    {
        /* NOTE: If the ordering used below needs to be changed, make sure to also update formats.cpp::layout_to_n_components */
        COMPONENT_LAYOUT_ABGR,
        COMPONENT_LAYOUT_ARGB,
        COMPONENT_LAYOUT_BGR,
        COMPONENT_LAYOUT_BGRA,
        COMPONENT_LAYOUT_D,
        COMPONENT_LAYOUT_DS,
        COMPONENT_LAYOUT_EBGR,
        COMPONENT_LAYOUT_R,
        COMPONENT_LAYOUT_RG,
        COMPONENT_LAYOUT_RGB,
        COMPONENT_LAYOUT_RGBA,
        COMPONENT_LAYOUT_S,
        COMPONENT_LAYOUT_XD,

        COMPONENT_LAYOUT_UNKNOWN
    } ComponentLayout;

    /** Holds properties of a single Vulkan Extension */
    typedef struct Extension
    {
        std::string name;
        uint32_t    version;

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param extension_props Vulkan structure to use for initialization.
         **/
        explicit Extension(const VkExtensionProperties& extension_props)
        {
            name    = extension_props.extensionName;
            version = extension_props.specVersion;
        }

        /** Returns true if @param extension_name matches the extension described by the instance. */
        bool operator==(const std::string& extension_name) const
        {
            return name == extension_name;
        }
    } Extension;

    typedef std::vector<Extension> Extensions;

    /** Holds driver-specific format capabilities */
    typedef struct FormatProperties
    {
        VkFormatFeatureFlagBits buffer_capabilities;
        VkFormatFeatureFlagBits linear_tiling_capabilities;
        VkFormatFeatureFlagBits optimal_tiling_capabilities;

        /** Dummy constructor */
        FormatProperties()
        {
            memset(this,
                   0,
                   sizeof(*this) );
        }

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param format_props Vulkan structure to use for initialization.
         **/
        FormatProperties(const VkFormatProperties& format_props)
        {
            buffer_capabilities         = static_cast<VkFormatFeatureFlagBits>(format_props.bufferFeatures);
            linear_tiling_capabilities  = static_cast<VkFormatFeatureFlagBits>(format_props.linearTilingFeatures);
            optimal_tiling_capabilities = static_cast<VkFormatFeatureFlagBits>(format_props.optimalTilingFeatures);
        }
    } FormatProperties;

    typedef enum
    {
        FORMAT_TYPE_SFLOAT,
        FORMAT_TYPE_SFLOAT_UINT,
        FORMAT_TYPE_SINT,
        FORMAT_TYPE_SNORM,
        FORMAT_TYPE_SRGB,
        FORMAT_TYPE_SSCALED,
        FORMAT_TYPE_UFLOAT,
        FORMAT_TYPE_UINT,
        FORMAT_TYPE_UNORM,
        FORMAT_TYPE_UNORM_UINT,
        FORMAT_TYPE_USCALED,

        FORMAT_TYPE_UNKNOWN,
    } FormatType;

    /** ID of an Anvil framebuffer's attachment */
    typedef uint32_t FramebufferAttachmentID;

    /** Describes an image memory barrier. */
    typedef struct ImageBarrier
    {
        bool                    by_region;
        VkAccessFlagBits        dst_access_mask;
        uint32_t                dst_queue_family_index;
        VkImage                 image;
        VkImageMemoryBarrier    image_barrier_vk;
        Anvil::Image*           image_ptr;
        VkImageLayout           new_layout;
        VkImageLayout           old_layout;
        VkAccessFlagBits        src_access_mask;
        uint32_t                src_queue_family_index;
        VkImageSubresourceRange subresource_range;

        /** Constructor.
         *
         *  Note that @param image_ptr is retained by this function.
         *
         *  @param in_source_access_mask      Source access mask to use for the barrier.
         *  @param in_destination_access_mask Destination access mask to use for the barrier.
         *  @param in_by_region_barrier       true if this is a by-region barrier.
         *  @param in_old_layout              Old layout of @param in_image_ptr to use for the barrier.
         *  @param in_new_layout              New layout of @param in_image_ptr to use for the barrier.
         *  @param in_src_queue_family_index  Source queue family index to use for the barrier.
         *  @param in_dst_queue_family_index  Destination queue family index to use for the barrier.
         *  @param in_image_ptr               Image instance the barrier refers to. May be nullptr, in which case
         *                                    "image" and "image_ptr" fields will be set to nullptr.
         *                                    The instance will be retained by this function.
         *  @param in_image_subresource_range Subresource range to use for the barrier.
         *
         **/
        ImageBarrier(VkAccessFlags           in_source_access_mask,
                     VkAccessFlags           in_destination_access_mask,
                     bool                    in_by_region_barrier,
                     VkImageLayout           in_old_layout,
                     VkImageLayout           in_new_layout,
                     uint32_t                in_src_queue_family_index,
                     uint32_t                in_dst_queue_family_index,
                     Anvil::Image*           in_image_ptr,
                     VkImageSubresourceRange in_image_subresource_range);

        /** Destructor.
         *
         *  Releases the encapsulated Image instance.
         **/
       virtual ~ImageBarrier();

       /** Copy constructor.
         *
         *  Retains the Image instance stored in the input barrier.
         *
         *  @param in Barrier instance to copy data from.
         **/
       ImageBarrier(const ImageBarrier& in);

       /** Returns a Vulkan memory barrier descriptor, whose configuration corresponds to
         *  to the configuration of this descriptor.
         **/
       virtual VkImageMemoryBarrier get_barrier_vk() const
       {
           return image_barrier_vk;
       }

       /** Returns a pointer to the Vulkan descriptor, whose configuration corresponds to
         *  the configuration of this descriptor.
         *
         *  The returned pointer remains valid for the duration of the Barrier descriptor's
         *  life-time.
         **/
       const VkImageMemoryBarrier* get_barrier_vk_ptr() const
       {
           return &image_barrier_vk;
       }

    private:
        ImageBarrier& operator=(const ImageBarrier&);
    } ImageBarrier;

    /** Holds properties of a single Vulkan Layer. */
    typedef struct Layer
    {
        std::string            description;
        std::vector<Extension> extensions;
        uint32_t               implementation_version;
        std::string            name;
        uint32_t               spec_version;

        /** Dummy constructor.
         *
         *  @param layer_name Name to use for the layer.
         **/
        Layer(const std::string& layer_name)
        {
            implementation_version = 0;
            name                   = layer_name;
            spec_version           = 0;
        }

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param layer_props Vulkan structure to use for initialization.
         **/
        Layer(const VkLayerProperties& layer_props)
        {
            description            = layer_props.description;
            implementation_version = layer_props.implementationVersion;
            name                   = layer_props.layerName;
            spec_version           = layer_props.specVersion;
        }

        /** Returns true if @param layer_name matches the layer name described by the instance. */
        bool operator==(const std::string& layer_name) const
        {
            return name == layer_name;
        }
    } Layer;

    typedef std::vector<Layer> Layers;

    /** Describes a Vulkan memory barrier. */
    typedef struct MemoryBarrier
    {
        VkAccessFlagBits destination_access_mask;
        VkMemoryBarrier  memory_barrier_vk;
        VkAccessFlagBits source_access_mask;

        /** Constructor.
         *
         *  @param in_source_access_mask      Source access mask of the Vulkan memory barrier.
         *  @param in_destination_access_mask Destination access mask of the Vulkan memory barrier.
         *
         **/
        explicit MemoryBarrier(VkAccessFlags in_destination_access_mask,
                               VkAccessFlags in_source_access_mask)
        {
            destination_access_mask = static_cast<VkAccessFlagBits>(in_destination_access_mask);
            source_access_mask      = static_cast<VkAccessFlagBits>(in_source_access_mask);

            memory_barrier_vk.dstAccessMask = destination_access_mask;
            memory_barrier_vk.pNext         = nullptr;
            memory_barrier_vk.srcAccessMask = source_access_mask;
            memory_barrier_vk.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        }

        /** Destructor. */
        virtual ~MemoryBarrier()
        {
            /* Stub */
        }

        /** Returns a Vulkan memory barrier descriptor, whose configuration corresponds to
         *  to the configuration of this descriptor.
         **/
        virtual VkMemoryBarrier get_barrier_vk() const
        {
            return memory_barrier_vk;
        }

        /** Returns a pointer to the Vulkan descriptor, whose configuration corresponds to
         *  the configuration of this descriptor.
         *
         *  The returned pointer remains valid for the duration of the Barrier descriptor's
         *  life-time.
         **/
        virtual const VkMemoryBarrier* get_barrier_vk_ptr() const
        {
            return &memory_barrier_vk;
        }
    } MemoryBarrier;

    /** Holds properties of a single Vulkan Memory Heap. */
    typedef struct MemoryHeap
    {
        VkMemoryHeapFlagBits flags;
        VkDeviceSize         size;

        /** Stub constructor */
        MemoryHeap()
        {
            flags = static_cast<VkMemoryHeapFlagBits>(0);
            size  = 0;
        }
    } MemoryHeap;

    typedef std::vector<MemoryHeap> MemoryHeaps;

    /** Holds properties of a single Vulkan Memory Type. */
    typedef struct MemoryType
    {
        MemoryHeap*              heap_ptr;
        VkMemoryPropertyFlagBits flags;

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param type             Vulkan structure to use for initialization.
         *  @param memory_props_ptr Used to initialize the MemoryHeap pointer member. Must not be nullptr.
         **/
        explicit MemoryType(const VkMemoryType&      type,
                            struct MemoryProperties* memory_props_ptr);
    } MemoryType;

    typedef std::vector<MemoryType> MemoryTypes;

    /** Holds information about available memory heaps & types for a specific physical device. */
    typedef struct MemoryProperties
    {
        MemoryHeap* heaps;
        MemoryTypes types;

        MemoryProperties()
        {
            heaps = nullptr;
        }

        /** Destructor */
        ~MemoryProperties()
        {
            if (heaps != nullptr)
            {
                delete heaps;

                heaps = nullptr;
            }
        }

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param mem_properties Vulkan structure to use for initialization.
         **/
        void init(const VkPhysicalDeviceMemoryProperties& mem_properties);

    private:
        MemoryProperties           (const MemoryProperties&);
        MemoryProperties& operator=(const MemoryProperties&);
    } MemoryProperties;

    /* Dummy delete functor */
    template<class Type>
    struct NullDeleter
    {
        void operator()(Type* unused_ptr)
        {
            unused_ptr;
        }
    };

    /* A single push constant range descriptor */
    typedef struct PushConstantRange
    {
        uint32_t              offset;
        uint32_t              size;
        VkShaderStageFlagBits stages;

        /** Constructor
         *
         *  @param in_offset Start offset for the range.
         *  @param in_size   Size of the range.
         *  @param in_stages Valid pipeline stages for the range.
         */
        PushConstantRange(uint32_t           in_offset,
                          uint32_t           in_size,
                          VkShaderStageFlags in_stages)
        {
            offset = in_offset;
            size   = in_size;
            stages = static_cast<VkShaderStageFlagBits>(in_stages);
        }

        /** Comparison operator. Used internally. */
        bool operator==(const PushConstantRange& in) const
        {
            return (offset == in.offset &&
                    size   == in.size);
        }
    } PushConstantRange;

    typedef uint32_t            BindingElementIndex;
    typedef uint32_t            BindingIndex;
    typedef uint32_t            NumberOfBindingElements;
    typedef BindingElementIndex StartBindingElementIndex;

    typedef std::pair<StartBindingElementIndex, NumberOfBindingElements> BindingElementArrayRange;

    typedef std::vector<Anvil::DescriptorSetGroup*> DescriptorSetGroups;
    typedef std::vector<PushConstantRange>          PushConstantRanges;

    /** Holds information about a single Vulkan Queue Family. */
    typedef struct QueueFamilyInfo
    {
        VkQueueFlagBits flags;
        VkExtent3D      min_image_transfer_granularity;
        uint32_t        n_queues;
        uint32_t        n_timestamp_bits;

        /** Constructor. Initializes the instance using data provided by the driver.
         *
         *  @param props Vulkan structure to use for initialization.
         **/
        explicit QueueFamilyInfo(const VkQueueFamilyProperties& props)
        {
            flags                          = static_cast<VkQueueFlagBits>(props.queueFlags);
            min_image_transfer_granularity = props.minImageTransferGranularity;
            n_queues                       = props.queueCount;
            n_timestamp_bits               = props.timestampValidBits;
        }
    } QueueFamilyInfo;

    typedef std::vector<QueueFamilyInfo> QueueFamilyInfoItems;

    /** Enumerates all available queue family types */
    typedef enum
    {
        QUEUE_FAMILY_TYPE_COMPUTE,
        QUEUE_FAMILY_TYPE_TRANSFER,
        QUEUE_FAMILY_TYPE_UNIVERSAL, /* compute + queue */

        /* Always last */
        QUEUE_FAMILY_TYPE_COUNT,
        QUEUE_FAMILY_TYPE_FIRST     = QUEUE_FAMILY_TYPE_COMPUTE,
        QUEUE_FAMILY_TYPE_UNDEFINED = QUEUE_FAMILY_TYPE_COUNT
    } QueueFamilyType;

    /* Keyboard character IDs */
    #ifdef _WIN32
        #define ANVIL_KEY_HELPER(key) VK_##key
    #else
        #define ANVIL_KEY_HELPER(key) XK_##key
    #endif

    typedef enum
    {
#ifdef _WIN32
        KEY_ID_ESCAPE = ANVIL_KEY_HELPER(ESCAPE),
        KEY_ID_LEFT   = ANVIL_KEY_HELPER(LEFT),
        KEY_ID_RETURN = ANVIL_KEY_HELPER(RETURN),
        KEY_ID_RIGHT  = ANVIL_KEY_HELPER(RIGHT),
        KEY_ID_SPACE  = ANVIL_KEY_HELPER(SPACE)
#else
        KEY_ID_ESCAPE = ANVIL_KEY_HELPER(Escape),
        KEY_ID_LEFT   = ANVIL_KEY_HELPER(Left),
        KEY_ID_RETURN = ANVIL_KEY_HELPER(Return),
        KEY_ID_RIGHT  = ANVIL_KEY_HELPER(Right),
        KEY_ID_SPACE  = ANVIL_KEY_HELPER(space)
#endif
    } KeyID;

    /** Base pipeline ID. Internal type, used to represent compute / graphics pipeline IDs */
    typedef uint32_t PipelineID;

    /** Compute Pipeline ID */
    typedef PipelineID ComputePipelineID;

    /** Graphics Pipeline ID */
    typedef PipelineID GraphicsPipelineID;


    /* Index of a query within parent query pool instance */
    typedef uint32_t QueryIndex;

    /* Unique ID of a render-pass attachment within scope of a RenderPass instance. */
    typedef uint32_t RenderPassAttachmentID;

    /* Specifies one of the compute / rendering pipeline stages. */
    typedef enum
    {
        SHADER_STAGE_COMPUTE,
        SHADER_STAGE_FRAGMENT,
        SHADER_STAGE_GEOMETRY,
        SHADER_STAGE_TESSELLATION_CONTROL,
        SHADER_STAGE_TESSELLATION_EVALUATION,
        SHADER_STAGE_VERTEX,

        SHADER_STAGE_COUNT,
        SHADER_STAGE_UNKNOWN = SHADER_STAGE_COUNT
    } ShaderStage;

    /** Holds all information related to a specific shader module stage entry-point. */
    typedef struct ShaderModuleStageEntryPoint
    {
        const char*          name;
        Anvil::ShaderModule* shader_module_ptr;
        Anvil::ShaderStage   stage;

        /** Dummy constructor */
        ShaderModuleStageEntryPoint();

        /** Copy constructor. */
        ShaderModuleStageEntryPoint(const ShaderModuleStageEntryPoint& in);

        /** Constructor.
         *
         *  @param in_name              Entry-point name. Must not be nullptr.
         *  @param in_shader_module_ptr ShaderModule instance to use. Will be retained.
         *  @param in_stage             Shader stage the entry-point implements.
         */
        ShaderModuleStageEntryPoint(const char*   in_name,
                                    ShaderModule* in_shader_module_ptr,
                                    ShaderStage   in_stage);

        /** Destructor. */
        ~ShaderModuleStageEntryPoint();

        ShaderModuleStageEntryPoint& operator=(const ShaderModuleStageEntryPoint&);
    } ShaderModuleStageEntryPoint;

    /* Unique ID of a render-pass' sub-pass attachment within scope of a RenderPass instance. */
    typedef uint32_t SubPassAttachmentID;

    /* Unique ID of a sub-pass within scope of a RenderPass instance. */
    typedef uint32_t SubPassID;

    /** Defines supported timestamp capture modes. */
    typedef enum
    {
        /* No timestamps should be captured */
        TIMESTAMP_CAPTURE_MODE_DISABLED,

        /* Two timestamps should be captured:
         *
         * 1. top-of-pipe timestamp, preceding actual commands.
         * 2. tof-of-pipe timestamp, after all commands are recorded.
         */
        TIMESTAMP_CAPTURE_MODE_ENABLED_COMMAND_SUBMISSION_TIME,

        /* Two timestamps should be captured:
        *
        * 1. top-of-pipe timestamp, preceding actual commands.
        * 2. bottom-of-pipe timestamp, after all commands are recorded.
        */
        TIMESTAMP_CAPTURE_MODE_ENABLED_COMMAND_EXECUTION_TIME
    } TimestampCaptureMode;

    /** A bitmask defining one or more queue family usage.*/
    typedef enum
    {
        QUEUE_FAMILY_COMPUTE_BIT  = 1 << 0,
        QUEUE_FAMILY_DMA_BIT      = 1 << 1,
        QUEUE_FAMILY_GRAPHICS_BIT = 1 << 2 
    } QueueFamily;
    typedef int QueueFamilyBits;
}; /* Vulkan namespace */

#endif /* MISC_TYPES_H */
