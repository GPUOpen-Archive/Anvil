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

/** Defines an image wrapper class which simplifies the following processes:
 *
 *  - Image initialization and tear-down.
 *  - Mip-map data updates.
 *  - Mip-map size caching
 **/
#ifndef WRAPPERS_IMAGE_H
#define WRAPPERS_IMAGE_H

#include "misc/callbacks.h"
#include "misc/debug_marker.h"
#include "misc/types.h"
#include "misc/page_tracker.h"

namespace Anvil
{
    typedef struct ImageCallbackIsAllocPendingQueryData
    {
        explicit ImageCallbackIsAllocPendingQueryData(std::shared_ptr<Anvil::Image> in_image_ptr)
            :image_ptr(in_image_ptr),
             result   (false)
        {
            /* Stub */
        }

        ImageCallbackIsAllocPendingQueryData& operator=(const ImageCallbackIsAllocPendingQueryData&) = delete;

        const std::shared_ptr<const Anvil::Image> image_ptr;
        bool                                      result;
    } ImageCallbackIsAllocPendingQueryData;

    /* Enumerates available image call-back types. */
    enum ImageCallbackID
    {
        /* Call-back issued by sparse image instances whenever the image needs to check if
         * there are any pending alloc operations for this image instance. Any recipient
         * should set callback_arg::result to true in case a bake operation *would* assign
         * new pages to the image instance. If no allocs are scheduled, the bool value MUST
         * be left untouched.
         *
         * This call-back is needed for memory allocator to support implicit bake operations
         * for sparse images.
         *
         * callback_arg: ImageCallbackIsAllocPendingQueryData*
         **/
        IMAGE_CALLBACK_ID_IS_ALLOC_PENDING,

        /* Call-back issued when no memory block is assigned to the image wrapper instance and
         * someone has just requested it.
         *
         * This call-back is needed for memory allocator to support implicit bake operations.
         *
         * callback_arg: Calling back image instance;
         **/
        IMAGE_CALLBACK_ID_MEMORY_BLOCK_NEEDED,

        /* Always last */
        IMAGE_CALLBACK_ID_COUNT
    };

    /** A wrapper class for a VkImage and the bound VkMemory object. */
    class Image : public CallbacksSupportProvider,
                  public DebugMarkerSupportProvider<Image>,
                  public std::enable_shared_from_this<Image>
    {
    public:
        /* Public functions */

        /* Transitions the image from one layout to another.
         *
         * This is a blocking call.
         *
         * @param in_queue_ptr         Queue to use for the transition. The specified queue must support pipeline barrier
         *                             command. Must not be null.
         * @param in_src_access_mask   Source access mask to use for the transition.
         * @param in_src_layout        Image layout to transfer from.
         * @param in_dst_access_mask   Destination access mask to use for the transition.
         * @param in_dst_layout        Image layout to transfer to.
         * @param in_subresource_range Subresource range to use for the transfer operation.
         *
         */
        void change_image_layout(std::shared_ptr<Anvil::Queue>  in_queue_ptr,
                                 VkAccessFlags                  in_src_access_mask,
                                 VkImageLayout                  in_src_layout,
                                 VkAccessFlags                  in_dst_access_mask,
                                 VkImageLayout                  in_dst_layout,
                                 const VkImageSubresourceRange& in_subresource_range);

        /** Initializes a new non-sparse Image instance *without* a memory backing. A memory region should be bound
         *  to the object by calling Image::set_memory() before using the object for any operations.
         *
         *  The function can also optionally fill the image with data, as soon as memory backing is
         *  attached. To make it do so, pass a non-null ptr to a MipmapRawData vector via the @param
         *  mipmaps_ptr argument.
         *
         *  If this constructor is used, the image can be transformed automatically to the right layout
         *  at set_memory() call time by setting @param in_final_image_layout argument to a value other
         *  than VK_IMAGE_LAYOUT_UNDEFINED and VK_IMAGE_LAYOUT_PREINITIALIZED.
         *
         *  @param in_device_ptr               Device to use.
         *  @param in_type                     Vulkan image type to use.
         *  @param in_format                   Vulkan format to use.
         *  @param in_tiling                   Vulkan image tiling to use.
         *  @param in_usage                    Vulkan image usage pattern to use.
         *  @param in_base_mipmap_width        Width of the base mip-map.
         *  @param in_base_mipmap_height       Height of the base mip-map. Must be at least 1 for all image types.
         *  @param in_base_mipmap_depth        Depth of the base mip-map. Must be at least 1 for all image types.
         *  @param in_n_layers                 Number of layers to use. Must be at least 1 for all image types.
         *  @param in_sample_count             Sample count to use.
         *  @param in_queue_families           A combination of Anvil::QUEUE_FAMILY_* bits, indicating which device queues
         *                                     the image is going to be accessed by.
         *  @param in_sharing_mode             Vulkan sharing mode to use.
         *  @param in_use_full_mipmap_chain    true, if all mipmaps should be created for the image. False to only allocate
         *                                     storage for the base mip-map.
         *  @param in_create_flags             Optional image features that the created image should support.
         *  @param in_post_create_image_layout Image layout to transfer the image to after it is created AND assigned memory
         *                                     backing. In order to leave the image intact, set this argument to
         *                                     VK_IMAGE_LAYOUT_UNDEFINED if @param in_opt_mipmaps_ptr is NULL, or to
         *                                     VK_IMAGE_LAYOUT_PREINITIALIZED if @param in_opt_mipmaps_ptr is not NULL.
         *                                     (which depends on what value is passed to @param in_opt_mipmaps_ptr), 
         *  @param in_opt_mipmaps_ptr          If not NULL, specified data will be used to initialize created image's mipmaps
         *                                     with content, as soon as the image is assigned a memory backing.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
         static std::shared_ptr<Image> create_nonsparse(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
                                                        VkImageType                       in_type,
                                                        VkFormat                          in_format,
                                                        VkImageTiling                     in_tiling,
                                                        VkImageUsageFlags                 in_usage,
                                                        uint32_t                          in_base_mipmap_width,
                                                        uint32_t                          in_base_mipmap_height,
                                                        uint32_t                          in_base_mipmap_depth,
                                                        uint32_t                          in_n_layers,
                                                        VkSampleCountFlagBits             in_sample_count,
                                                        Anvil::QueueFamilyBits            in_queue_families,
                                                        VkSharingMode                     in_sharing_mode,
                                                        bool                              in_use_full_mipmap_chain,
                                                        ImageCreateFlags                  in_flags,
                                                        VkImageLayout                     in_post_create_image_layout,
                                                        const std::vector<MipmapRawData>* in_opt_mipmaps_ptr);

        /** Initializes a new non-sparse Image instance, along with a memory backing.
         *
         *  This constructor assumes the image should be initialized in UNDEFINED layout, if no mipmap data
         *  is specified, or PREINITIALIZED otherwise. In the latter case, it will then proceed with filling
         *  the storage with mipmap data (if @param in_mipmaps_ptr is not nullptr), and finally transition
         *  the image to the @param in_post_create_image_layout layout.
         *
         *  @param in_device_ptr                              Device to use.
         *  @param in_type                                    Vulkan image type to use.
         *  @param in_format                                  Vulkan format to use.
         *  @param in_tiling                                  Vulkan image tiling to use.
         *  @param in_usage                                   Vulkan image usage pattern to use.
         *  @param in_base_mipmap_width                       Width of the base mip-map.
         *  @param in_base_mipmap_height                      Height of the base mip-map. Must be at least 1 for all image types.
         *  @param in_base_mipmap_depth                       Depth of the base mip-map. Must be at least 1 for all image types.
         *  @param in_n_layers                                Number of layers to use. Must be at least 1 for all image types.
         *  @param in_sample_count                            Sample count to use.
         *  @param in_queue_families                          A combination of Anvil::QUEUE_FAMILY_* bits, indicating which device queues
         *                                                    the image is going to be accessed by.
         *  @param in_sharing_mode                            Vulkan sharing mode to use.
         *  @param in_use_full_mipmap_chain                   true if all mipmaps should be created for the image. False to only allocate
         *                                                    storage for the base mip-map.
         *  @param in_memory_features                         Memory features for the memory backing.
         *  @param in_create_flags                            Optional image features that the created image should support.
         *  @param in_post_create_image_layout                Layout to transition the new image to. Must not be VK_IMAGE_LAYOUT_UNDEFINED or
         *                                                    VK_IMAGE_LAYOUT_PREINITIALIZED.
         *  @param in_opt_mipmaps_ptr                         If not nullptr, specified MipmapRawData items will be used to drive the mipmap contents
         *                                                    initialization process. Ignored if nullptr.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
        static std::shared_ptr<Image> create_nonsparse(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
                                                       VkImageType                       in_type,
                                                       VkFormat                          in_format,
                                                       VkImageTiling                     in_tiling,
                                                       VkImageUsageFlags                 in_usage,
                                                       uint32_t                          in_base_mipmap_width,
                                                       uint32_t                          in_base_mipmap_height,
                                                       uint32_t                          in_base_mipmap_depth,
                                                       uint32_t                          in_n_layers,
                                                       VkSampleCountFlagBits             in_sample_count,
                                                       Anvil::QueueFamilyBits            in_queue_families,
                                                       VkSharingMode                     in_sharing_mode,
                                                       bool                              in_use_full_mipmap_chain,
                                                       MemoryFeatureFlags                in_memory_features,
                                                       ImageCreateFlags                  in_create_flags,
                                                       VkImageLayout                     in_post_create_image_layout,
                                                       const std::vector<MipmapRawData>* in_mipmaps_ptr);

        /** Wrapper constructor for existing VkImage instances, as reported for
         *  swapchain images. Object instantiated with this constructor will NOT
         *  release the specified VkImage instance.
         *
         *  The image will NOT be transitioned to any specific image layout.
         *
         *  For argument discussion, see specification of the other create() functions.
         **/
        static std::shared_ptr<Image> create_nonsparse(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                       const VkSwapchainCreateInfoKHR&  in_swapchain_create_info,
                                                       VkImage                          in_image);

        /** Initializes a new sparse Image instance *without* physical memory backing. Memory region(s)
         *  should be bound to the object by calling Image::set_memory_block() before using the object
         *  for any operations. Whether or not all tiles need to be assigned memory blocks prior to accessing
         *  image contents depends on whether sparse binding or sparse residency has been requested at
         *  creation time.
         *
         *  If NONALIASED or ALIASED residency is requested and the device does not support requested image
         *  configuration, a NULL image will be returned.
         *
         *  User must manually configure sparse bindings for the image by using Queue::bind_sparse_memory(),
         *  before uploading any mip data. The mips can be uploaded using upload_mipmaps().
         *
         *  @param in_device_ptr             Device to use.
         *  @param in_type                   Vulkan image type to use.
         *  @param in_format                 Vulkan format to use.
         *  @param in_tiling                 Vulkan image tiling to use.
         *  @param in_usage                  Vulkan image usage pattern to use.
         *  @param in_base_mipmap_width      Width of the base mip-map. Must be at least 1 for all image types.
         *  @param in_base_mipmap_height     Height of the base mip-map. Must be at least 1 for all image types.
         *  @param in_base_mipmap_depth      Depth of the base mip-map. Must be at least 1 for all image types.
         *  @param in_n_layers               Number of layers to use. Must be at least 1 for all image types.
         *  @param in_sample_count           Sample count to use.
         *  @param in_queue_families         A combination of Anvil::QUEUE_FAMILY_* bits, indicating which device queues
         *                                   the image is going to be accessed by.
         *  @param in_sharing_mode           Vulkan sharing mode to use.
         *  @param in_use_full_mipmap_chain  true, if all mipmaps should be created for the image. False to make the image
         *                                   only use one mip.
         *  @param in_create_flags           Optional image features that the created image should support.
         *  @param in_sparse_residency_scope Scope of sparse residency to request for the image.
         *  @param in_initial_layout         Initial layout to use for the image. Must either be VK_IMAGE_LAYOUT_UNDEFINED or
         *                                   VK_IMAGE_LAYOUT_PREINITIALIZED.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
         static std::shared_ptr<Image> create_sparse(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                     VkImageType                      in_type,
                                                     VkFormat                         in_format,
                                                     VkImageTiling                    in_tiling,
                                                     VkImageUsageFlags                in_usage,
                                                     uint32_t                         in_base_mipmap_width,
                                                     uint32_t                         in_base_mipmap_height,
                                                     uint32_t                         in_base_mipmap_depth,
                                                     uint32_t                         in_n_layers,
                                                     VkSampleCountFlagBits            in_sample_count,
                                                     Anvil::QueueFamilyBits           in_queue_families,
                                                     VkSharingMode                    in_sharing_mode,
                                                     bool                             in_use_full_mipmap_chain,
                                                     ImageCreateFlags                 in_create_flags,
                                                     Anvil::SparseResidencyScope      in_residency_scope,
                                                     VkImageLayout                    in_initial_layout = VK_IMAGE_LAYOUT_UNDEFINED);

        /** Destructor */
        virtual ~Image();

        /** Returns subresource layout for an aspect for user-specified mip of an user-specified layer.
         *
         *  May only be used for linear images.
         *
         *  NOTE: This information is cached at image creation time, so the driver's impl will not be
         *        called.
         */
        bool get_aspect_subresource_layout(VkImageAspectFlags   in_aspect,
                                           uint32_t             in_n_layer,
                                           uint32_t             in_n_mip,
                                           VkSubresourceLayout* out_subresource_layout_ptr) const;

        /** Returns the underlying VkImage instance.
         *
         *  For non-sparse images, in case no memory block has been assigned to the image,
         *  the function will issue a BUFFER_CALLBACK_ID_MEMORY_BLOCK_NEEDED call-back, so that
         *  any memory allocator, which has this buffer scheduled for deferred memory allocation,
         *  gets a chance to allocate & bind a memory block to the instance. A non-sparse image instance
         *  without any memory block bound msut not be used for any GPU-side operation.
         *
         *  In case of sparse images, the callback will only occur if at least one memory allocation
         *  has been scheduled for this image instance.
         */
        const VkImage& get_image();

        /** Returns information about the data alignment required by the underlying VkImage instance */
        VkDeviceSize get_image_alignment() const
        {
            return m_alignment;
        }

        /** Returns flags specified at image creation time.
         **/
        Anvil::ImageCreateFlags get_image_create_flags() const
        {
            return m_create_flags;
        }

        /** Returns extent of a user-specified image mip as a VkExtent2D structure.
         *
         *  @param in_n_mipmap Index of the mipmap to use for the query.
         *
         *  @return As per summary.
         */
        VkExtent2D get_image_extent_2D(uint32_t in_n_mipmap) const;

        /** Returns extent of a user-specified image mip as a VkExtent3D structure.
         *
         *  @param in_n_mipmap Index of the mipmap to use for the query.
         *
         *  @return As per summary.
         */
        VkExtent3D get_image_extent_3D(uint32_t in_n_mipmap) const;

        /** Returns information about the image format used to create the underlying VkImage instance */
        VkFormat get_image_format() const
        {
            return m_format;
        }

        /** Returns information about the memory types the underlying VkImage instance supports */
        uint32_t get_image_memory_types() const
        {
            return m_memory_types;
        }

        /** Returns information about size of the mipmap at index @param in_n_mipmap.
         *
         *  @param in_n_mipmap        Index of the mipmap to use for the query.
         *  @param out_opt_width_ptr  If not nullptr, deref will be set to the width of the queried mipmap,
         *                            assuming the function returns ture.
         *  @param out_opt_height_ptr If not nullptr, deref will be set to the height of the queried mipmap,
         *                            assuming the function returns true.
         *  @param out_opt_depth_ptr  If not nullptr, deref will be set to the depth of the queried mipmap,
         *                            assuming the function returns true.
         *
         *  @return true if @param in_n_mipmap was a valid mipmap index; false otherwise.
         **/
        bool get_image_mipmap_size(uint32_t  in_n_mipmap,
                                   uint32_t* out_opt_width_ptr,
                                   uint32_t* out_opt_height_ptr,
                                   uint32_t* out_opt_depth_ptr) const;

        /** Returns information about the number of layers stored by the underlying VkImage instance */
        uint32_t get_image_n_layers() const
        {
            return m_n_layers;
        }

        /** Returns information about the number of mipmaps stored by the underlying VkImage instance */
        uint32_t get_image_n_mipmaps() const
        {
            return m_n_mipmaps;
        }

        /** Returns queue families compatible with the image */
        Anvil::QueueFamilyBits get_image_queue_families() const
        {
            return m_queue_families;
        }

        /** Returns information about the number of samples stored by the underlying VkImage instance */
        VkSampleCountFlagBits get_image_sample_count() const
        {
            return static_cast<VkSampleCountFlagBits>(m_sample_count);
        }

        /** Returns image tiling */
        VkImageTiling get_image_tiling() const
        {
            return m_tiling;
        }

        /** Returns image sharing mode, as specified at creation time */
        VkSharingMode get_image_sharing_mode() const
        {
            return m_sharing_mode;
        }

        /** Returns information about the amount of memory the underlying VkImage instance requires
         *  to work correctly.
         **/
        VkDeviceSize get_image_storage_size() const
        {
            return m_storage_size;
        }

        /* Returns image type */
        VkImageType get_image_type() const
        {
            return m_type;
        }

        /* Returns image usage flags */
        VkImageUsageFlags get_image_usage() const
        {
            return m_usage;
        }

        /** Returns a pointer to the underlying memory block wrapper instance.
         *
         *  In case no memory block has been assigned to the non-sparse image OR some pages of
         *  a sparse image are left without memory backing, the function will issue an
         *  IMAGE_CALLBACK_ID_MEMORY_BLOCK_NEEDED call-back, so that any memory allocator, which
         *  has this image scheduled for deferred memory allocation, gets a chance to allocate
         *  & bind a memory block to the instance.
         *
         *  In case of sparse images, the callback will only occur if at least one memory allocation
         *  has been scheduled for this image instance.
         **/
        std::shared_ptr<Anvil::MemoryBlock> get_memory_block();

        /** Returns VkMemoryRequirements structure, as reported by the driver for this Image instance. */
        const VkMemoryRequirements& get_memory_requirements() const
        {
            return m_memory_reqs;
        }

        /** Returns a structure filled with details required to correctly bind tiles to a sparse image.
         *
         *  This function can only be called against sparse images with ALIASED or NONALIASED residency.
         **/
        bool get_sparse_image_aspect_properties(const VkImageAspectFlagBits                in_aspect,
                                                const Anvil::SparseImageAspectProperties** out_result_ptr_ptr) const;

        /** Returns a filled subresource range descriptor, covering all layers & mipmaps of the image */
        VkImageSubresourceRange get_subresource_range() const;

        /** Tells whether this image provides data for the specified image aspects.
         *
         *  @param in_aspects A bitfield of image aspect bits which should be used for the query.
         *
         *  @return true if data for all specified aspects is provided by the image, false otherwise.
         */
        bool has_aspects(VkImageAspectFlags in_aspects) const;

        /** Tells whether a physical memory page is assigned to the specified texel location.
         *
         *  Must only be called for sparse images whose sparse residency is not NONE.
         *
         *  @param in_aspect  Image aspect to use for the query.
         *  @param in_n_layer Index of the layer to use for the query.
         *  @param in_n_mip   Index of the mip to use for the query
         *  @param in_x       X location of the texel.
         *  @param in_y       Y location of the texel.
         *  @param in_z       Z location of the texel.
         *
         *  @return true if physical memory is bound to the specified location, false otherwise.
         *
         */
        bool is_memory_bound_for_texel(VkImageAspectFlagBits in_aspect,
                                       uint32_t              in_n_layer,
                                       uint32_t              in_n_mip,
                                       uint32_t              in_x,
                                       uint32_t              in_y,
                                       uint32_t              in_z) const;

        /** Tells whether this Image wrapper instance holds a sparse image */
        bool is_sparse() const
        {
            return m_is_sparse;
        }

        /** Tells whether this Image wrapper instance holds a swapchain image */
        bool is_swapchain_image() const
        {
            return m_is_swapchain_image;
        }

        /** Binds the specified region of a Vulkan memory object to an Image and caches information
         *  about the new binding.
         *
         *  NOTE: If used against sparse images, the assigned memory block's size must EXACTLY match
         *        the amount of memory required by the image. Otherwise, the function will fail.
         *
         *  NOTE: If used against non-sparse images, it is illegal to change the memory backing, after
         *        one has been associated with an Image instance.
         *
         *  @param in_memory_block_ptr Memory block to assign to the image
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_memory(std::shared_ptr<Anvil::MemoryBlock> in_memory_block_ptr);

        /** Updates image with specified mip-map data. Blocks until the operation finishes executing.
         *
         *  Handles both linear and optimal images.
         *
         *  @param in_mipmaps_ptr           A vector of MipmapRawData items, holding mipmap data. Must not
         *                                  be NULL.
         *  @param in_current_image_layout  Image layout, that the image is in right now.
         *  @param out_new_image_layout_ptr Deref will be set to the image layout the image has been transitioned
         *                                  to, in order to perform the request. Must not be NULL.
         *
         **/
        void upload_mipmaps(const std::vector<MipmapRawData>* in_mipmaps_ptr,
                            VkImageLayout                     in_current_image_layout,
                            VkImageLayout*                    out_new_image_layout_ptr);

    private:
        /* Private type declarations */

        /** Defines dimensions of a single image mip-map */
        typedef struct Mipmap
        {
            uint32_t depth;
            uint32_t height;
            uint32_t width;

            Mipmap(uint32_t in_width,
                   uint32_t in_height,
                   uint32_t in_depth)
            {
                depth  = in_depth;
                height = in_height;
                width  = in_width;
            }
        } Mipmap;

        typedef std::vector<Mipmap> Mipmaps;

        /** Holds information on page occupancy for a single layer-mip for a specific image aspect */
        typedef struct AspectPageOccupancyLayerMipData
        {
            /* Each item in this vector holds a reference to a MemoryBlock instance, which provides memory
            *  backing for the corresponding linearized image tile location */
            std::vector<std::shared_ptr<Anvil::MemoryBlock> > tile_to_block_mappings;

            uint32_t n_tiles_x;
            uint32_t n_tiles_y;
            uint32_t n_tiles_z;
            uint32_t tile_depth;
            uint32_t tile_height;
            uint32_t tile_width;

            /** Converts user-specified XYZ location to a linearized index.
             *
             *  @param in_x X location of the texel.
             *  @param in_y Y location of the texel.
             *  @param in_z Z location of the texel.
             *
             *  @return As per description.
             */
            uint32_t get_texture_space_xyz_to_block_mapping_index(uint32_t in_x,
                                                                  uint32_t in_y,
                                                                  uint32_t in_z) const
            {
                const uint32_t tile_x = in_x / tile_width;  
                const uint32_t tile_y = in_y / tile_height;
                const uint32_t tile_z = in_z / tile_depth;

                const uint32_t result = tile_z * n_tiles_x * n_tiles_y
                                      + tile_y * n_tiles_x
                                      + tile_x;

                anvil_assert(result < tile_to_block_mappings.size() );

                return result;
            }

            /** Converts a tile location, expressed in XYZ space, to a linearized index.
             *
             *  @param in_tile_x X index of the tile.
             *  @param in_tile_y Y index of the tile.
             *  @param in_tile_z Z index of the tile.
             *
             *  @return As per description.
             */
            uint32_t get_tile_space_xyz_to_block_mapping_index(uint32_t in_tile_x,
                                                               uint32_t in_tile_y,
                                                               uint32_t in_tile_z) const
            {
                const uint32_t result = in_tile_z * n_tiles_x * n_tiles_y
                                      + in_tile_y * n_tiles_x
                                      + in_tile_x;

                anvil_assert(result < tile_to_block_mappings.size() );

                return result;
            }

            /** Constructor. Fills the structure and preallocates memory.
             *
             *  @param in_mip_width   Width of the mip to represent.
             *  @param in_mip_height  Height of the mip.
             *  @param in_mip_depth   Depth of the mip.
             *  @param in_tile_width  Width of the mip's tile.
             *  @param in_tile_height Height of the mip's tile.
             *  @param in_tile_depth  Depth of the mip's tile.
             **/
            AspectPageOccupancyLayerMipData(uint32_t in_mip_width,
                                            uint32_t in_mip_height,
                                            uint32_t in_mip_depth,
                                            uint32_t in_tile_width,
                                            uint32_t in_tile_height,
                                            uint32_t in_tile_depth)
            {
                tile_depth  = in_tile_depth;
                tile_height = in_tile_height;
                tile_width  = in_tile_width;
                n_tiles_x   = Anvil::Utils::round_up(in_mip_width,  tile_width)  / tile_width;
                n_tiles_y   = Anvil::Utils::round_up(in_mip_height, tile_height) / tile_height;
                n_tiles_z   = Anvil::Utils::round_up(in_mip_depth,  tile_depth)  / tile_depth;

                anvil_assert(n_tiles_x >= 1 && n_tiles_y >= 1 && n_tiles_z >= 1);

                const uint32_t n_total_tiles = n_tiles_x * n_tiles_y * n_tiles_z;

                tile_to_block_mappings.resize(n_total_tiles);
            }
        } AspectPageOccupancyLayerMipData;

        /** Holds page occupancy data for a single layer */
        typedef struct AspectPageOccupancyLayerData
        {
            std::vector<AspectPageOccupancyLayerMipData> mips;

            uint32_t                                                n_total_tail_pages;
            std::map<std::shared_ptr<Anvil::MemoryBlock>, uint32_t> tail_pages_per_binding;
            std::vector<PageOccupancyStatus>                        tail_occupancy;

            AspectPageOccupancyLayerData()
            {
                n_total_tail_pages = 0;
            }
        } AspectPageOccupancyLayerData;

        /** Holds page occupancy data for all layers */
        typedef struct AspectPageOccupancyData
        {
            std::vector<AspectPageOccupancyLayerData> layers;

            AspectPageOccupancyData()
            {
                /* Stub */
            }
        } AspectPageOccupancyData;

        /* Private functions */
        /** See corresponding create_nonsparse() function for specification */
        Image(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
              VkImageType                       in_type,
              VkFormat                          in_format,
              VkImageTiling                     in_tiling,
              VkSharingMode                     in_sharing_mode, 
              VkImageUsageFlags                 in_usage,
              uint32_t                          in_base_mipmap_width,
              uint32_t                          in_base_mipmap_height,
              uint32_t                          in_base_mipmap_depth,
              uint32_t                          in_n_layers,
              VkSampleCountFlagBits             in_sample_count,
              bool                              in_use_full_mipmap_chain,
              ImageCreateFlags                  in_create_flags,
              Anvil::QueueFamilyBits            in_queue_families,
              VkImageLayout                     in_post_create_image_layout,
              const std::vector<MipmapRawData>* in_opt_mipmaps_ptr);

        /** See corresponding create_nonsparse() function for specification **/
        Image(std::weak_ptr<Anvil::BaseDevice>  in_device_ptr,
              VkImageType                       in_type,
              VkFormat                          in_format,
              VkImageTiling                     in_tiling,
              VkSharingMode                     in_sharing_mode, 
              VkImageUsageFlags                 in_usage,
              uint32_t                          in_base_mipmap_width,
              uint32_t                          in_base_mipmap_height,
              uint32_t                          in_base_mipmap_depth,
              uint32_t                          in_n_layers,
              VkSampleCountFlagBits             in_sample_count,
              Anvil::QueueFamilyBits            in_queue_families,
              bool                              in_use_full_mipmap_chain,
              ImageCreateFlags                  in_create_flags,
              VkImageLayout                     in_post_create_image_layout,
              const std::vector<MipmapRawData>* in_opt_mipmaps_ptr);

        /** See corresponding create_nonsparse() function for specification **/
        Image(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
              VkImage                          in_image,
              VkFormat                         in_format,
              VkImageTiling                    in_tiling,
              VkSharingMode                    in_sharing_mode, 
              VkImageUsageFlags                in_usage,
              uint32_t                         in_base_mipmap_width,
              uint32_t                         in_base_mipmap_height,
              uint32_t                         in_base_mipmap_depth,
              uint32_t                         in_n_layers,
              uint32_t                         in_n_mipmaps,
              VkSampleCountFlagBits            in_sample_count,
              uint32_t                         in_n_slices,
              Anvil::ImageCreateFlags          in_create_flags,
              Anvil::QueueFamilyBits           in_queue_families);

        /** See corresponding create_sparse() function for specification **/
        Image(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
              VkImageType                      in_type,
              VkFormat                         in_format,
              VkImageTiling                    in_tiling,
              VkImageUsageFlags                in_usage,
              uint32_t                         in_base_mipmap_width,
              uint32_t                         in_base_mipmap_height,
              uint32_t                         in_base_mipmap_depth,
              uint32_t                         in_n_layers,
              VkSampleCountFlagBits            in_sample_count,
              Anvil::QueueFamilyBits           in_queue_families,
              VkSharingMode                    in_sharing_mode,
              bool                             in_use_full_mipmap_chain,
              ImageCreateFlags                 in_create_flags,
              Anvil::SparseResidencyScope      in_residency_scope);

        Image           (const Image&);
        Image& operator=(const Image&);

        void init               (bool                 in_use_full_mipmap_chain,
                                 MemoryFeatureFlags   in_memory_features,
                                 const VkImageLayout* in_start_image_layout_ptr);
        void init_mipmap_props  ();
        void init_page_occupancy(const std::vector<VkSparseImageMemoryRequirements>& memory_reqs);

        void on_memory_backing_update       (const VkImageSubresource&           in_subresource,
                                             VkOffset3D                          in_offset,
                                             VkExtent3D                          in_extent,
                                             std::shared_ptr<Anvil::MemoryBlock> in_memory_block_ptr,
                                             VkDeviceSize                        in_memory_block_start_offset);
        void on_memory_backing_opaque_update(VkDeviceSize                        in_resource_offset,
                                             VkDeviceSize                        in_size,
                                             std::shared_ptr<Anvil::MemoryBlock> in_memory_block_ptr,
                                             VkDeviceSize                        in_memory_block_start_offset);

        void set_memory_sparse(VkDeviceSize                        in_resource_offset,
                               VkDeviceSize                        in_size,
                               std::shared_ptr<Anvil::MemoryBlock> in_memory_block_ptr,
                               VkDeviceSize                        in_memory_block_start_offset);
        void set_memory_sparse(const VkImageSubresource&           in_subresource,
                               VkOffset3D                          in_offset,
                               VkExtent3D                          in_extent,
                               std::shared_ptr<Anvil::MemoryBlock> in_memory_block_ptr,
                               VkDeviceSize                        in_memory_block_start_offset);

        void transition_to_post_create_image_layout(VkAccessFlags in_src_access_mask,
                                                    VkImageLayout in_src_layout);

        /** TODO.
         *
         *  Does NOT set ::pQueueFamilyIndices and ::queueFamilyIndexCount!
         */
        static VkImageCreateInfo get_create_info_for_swapchain(std::shared_ptr<const Anvil::Swapchain> in_swapchain_ptr);

        /* Private members */
        VkSampleCountFlagsVariable(m_sample_count);
        VkImageUsageFlagsVariable (m_usage);

        typedef std::pair<uint32_t /* n_layer */, uint32_t /* n_mip */>         LayerMipKey;
        typedef std::map<LayerMipKey, VkSubresourceLayout>                      LayerMipToSubresourceLayoutMap;
        typedef std::map<VkImageAspectFlagBits, LayerMipToSubresourceLayoutMap> AspectToLayerMipToSubresourceLayoutMap;

        VkDeviceSize                           m_alignment;
        AspectToLayerMipToSubresourceLayoutMap m_aspects;                   /* only used for linear images */
        Anvil::ImageCreateFlags                m_create_flags;
        uint32_t                               m_depth;
        std::weak_ptr<Anvil::BaseDevice>       m_device_ptr;
        VkFormat                               m_format;
        bool                                   m_has_transitioned_to_post_create_layout;
        uint32_t                               m_height;
        VkImage                                m_image;
        bool                                   m_image_owner;
        bool                                   m_is_sparse;
        bool                                   m_is_swapchain_image;
        VkMemoryRequirements                   m_memory_reqs;
        uint32_t                               m_memory_types;
        Mipmaps                                m_mipmaps;
        uint32_t                               m_n_layers;
        uint32_t                               m_n_mipmaps;
        uint32_t                               m_n_slices;
        VkImageLayout                          m_post_create_layout;
        Anvil::QueueFamilyBits                 m_queue_families;
        Anvil::SparseResidencyScope            m_residency_scope;
        VkSharingMode                          m_sharing_mode;
        VkDeviceSize                           m_storage_size;
        VkImageTiling                          m_tiling;
        VkImageType                            m_type;
        bool                                   m_uses_full_mipmap_chain;
        uint32_t                               m_width;

        std::shared_ptr<Anvil::MemoryBlock> m_metadata_memory_block_ptr;
        std::shared_ptr<Anvil::MemoryBlock> m_memory_block_ptr;
        bool                                m_memory_owner;

        std::vector<MipmapRawData>                                                 m_mipmaps_to_upload;
        std::unique_ptr<Anvil::PageTracker>                                        m_page_tracker_ptr; /* only used for sparse non-resident images */
        std::map<VkImageAspectFlagBits, std::shared_ptr<AspectPageOccupancyData> > m_sparse_aspect_page_occupancy;
        std::map<VkImageAspectFlagBits, Anvil::SparseImageAspectProperties>        m_sparse_aspect_props;

        friend class Anvil::Queue;
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_IMAGE_H */
