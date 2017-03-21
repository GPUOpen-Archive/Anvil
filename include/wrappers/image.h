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

#include "misc/types.h"
#include "misc/page_tracker.h"

namespace Anvil
{
    /** A wrapper class for a VkImage and the bound VkMemory object. */
    class Image : public std::enable_shared_from_this<Image>
    {
    public:
        /* Public functions */
        /** Initializes a new non-sparse Image instance *without* a memory backing. A memory region should be bound
         *  to the object by calling Image::set_memory() before using the object for any operations.
         *
         *  The function can also optionally fill the image with data, as soon as memory backing is
         *  attached. To make it do so, pass a non-null ptr to a MipmapRawData vector via the @param
         *  mipmaps_ptr argument.
         *
         *  If this constructor is used, the image can be transformed automatically to the right layout
         *  at set_memory() call time by setting @param final_image_layout argument to a value other
         *  than VK_IMAGE_LAYOUT_UNDEFINED and VK_IMAGE_LAYOUT_PREINITIALIZED.
         *
         *  @param device_ptr               Device to use.
         *  @param type                     Vulkan image type to use.
         *  @param format                   Vulkan format to use.
         *  @param tiling                   Vulkan image tiling to use.
         *  @param usage                    Vulkan image usage pattern to use.
         *  @param base_mipmap_width        Width of the base mip-map.
         *  @param base_mipmap_height       Height of the base mip-map. Must be at least 1 for all image types.
         *  @param base_mipmap_depth        Depth of the base mip-map. Must be at least 1 for all image types.
         *  @param n_layers                 Number of layers to use. Must be at least 1 for all image types.
         *  @param sample_count             Sample count to use.
         *  @param queue_families           A combination of Anvil::QUEUE_FAMILY_* bits, indicating which device queues
         *                                  the image is going to be accessed by.
         *  @param sharing_mode             Vulkan sharing mode to use.
         *  @param use_full_mipmap_chain    true, if all mipmaps should be created for the image. False to only allocate
         *                                  storage for the base mip-map.
         *  @param is_mutable               true if the image should be initialized as a mutable object.
         *  @param post_create_image_layout Image layout to transfer the image to after it is created AND assigned memory
         *                                  backing. In order to leave the image intact, set this argument to
         *                                  VK_IMAGE_LAYOUT_UNDEFINED if @param opt_mipmaps_ptr is NULL, or to
         *                                  VK_IMAGE_LAYOUT_PREINITIALIZED if @param opt_mipmaps_ptr is not NULL.
         *                                  (which depends on what value is passed to @param opt_mipmaps_ptr), 
         *  @param opt_mipmaps_ptr          If not NULL, specified data will be used to initialize created image's mipmaps
         *                                  with content, as soon as the image is assigned a memory backing.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
         static std::shared_ptr<Image> create_nonsparse(std::weak_ptr<Anvil::BaseDevice>  device_ptr,
                                                        VkImageType                       type,
                                                        VkFormat                          format,
                                                        VkImageTiling                     tiling,
                                                        VkImageUsageFlags                 usage,
                                                        uint32_t                          base_mipmap_width,
                                                        uint32_t                          base_mipmap_height,
                                                        uint32_t                          base_mipmap_depth,
                                                        uint32_t                          n_layers,
                                                        VkSampleCountFlagBits             sample_count,
                                                        Anvil::QueueFamilyBits            queue_families,
                                                        VkSharingMode                     sharing_mode,
                                                        bool                              use_full_mipmap_chain,
                                                        bool                              is_mutable,
                                                        VkImageLayout                     post_create_image_layout,
                                                        const std::vector<MipmapRawData>* opt_mipmaps_ptr);

        /** Initializes a new non-sparse Image instance, along with a memory backing.
         *
         *  This constructor assumes the image should be initialized in UNDEFINED layout, if no mipmap data
         *  is specified, or PREINITIALIZED otherwise. In the latter case, it will then proceed with filling
         *  the storage with mipmap data (if @param mipmaps_ptr is not nullptr), and finally transition
         *  the image to the @param post_create_image_layout layout.
         *
         *  @param device_ptr                        Device to use.
         *  @param type                              Vulkan image type to use.
         *  @param format                            Vulkan format to use.
         *  @param tiling                            Vulkan image tiling to use.
         *  @param usage                             Vulkan image usage pattern to use.
         *  @param base_mipmap_width                 Width of the base mip-map.
         *  @param base_mipmap_height                Height of the base mip-map. Must be at least 1 for all image types.
         *  @param base_mipmap_depth                 Depth of the base mip-map. Must be at least 1 for all image types.
         *  @param n_layers                          Number of layers to use. Must be at least 1 for all image types.
         *  @param sample_count                      Sample count to use.
         *  @param queue_families                    A combination of Anvil::QUEUE_FAMILY_* bits, indicating which device queues
         *                                           the image is going to be accessed by.
         *  @param sharing_mode                      Vulkan sharing mode to use.
         *  @param use_full_mipmap_chain             true if all mipmaps should be created for the image. False to only allocate
         *                                           storage for the base mip-map.
         *  @param should_memory_backing_be_mappable true if the image should be host-vislble; false if the caller never intends to
         *                                           map the image's memory backing into process space.
         *  @param should_memory_backing_be_coherent true if the image's memory backing should come from a coherent memory heap.
         *                                           false if incoherent heaps are OK. Note that it is illegal to set this argument to
         *                                           true if @param should_memory_backing_be_mappable is false.
         *  @param is_mutable                        true if the image should be initialized as a mutable object.
         *  @param post_create_image_layout          Layout to transition the new image to. Must not be VK_IMAGE_LAYOUT_UNDEFINED or
         *                                           VK_IMAGE_LAYOUT_PREINITIALIZED.
         *  @param mipmaps_ptr                       If not nullptr, specified MipmapRawData items will be used to drive the mipmap contents
         *                                           initialization process. Ignored if nullptr.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
        static std::shared_ptr<Image> create_nonsparse(std::weak_ptr<Anvil::BaseDevice>  device_ptr,
                                                       VkImageType                       type,
                                                       VkFormat                          format,
                                                       VkImageTiling                     tiling,
                                                       VkImageUsageFlags                 usage,
                                                       uint32_t                          base_mipmap_width,
                                                       uint32_t                          base_mipmap_height,
                                                       uint32_t                          base_mipmap_depth,
                                                       uint32_t                          n_layers,
                                                       VkSampleCountFlagBits             sample_count,
                                                       Anvil::QueueFamilyBits            queue_families,
                                                       VkSharingMode                     sharing_mode,
                                                       bool                              use_full_mipmap_chain,
                                                       bool                              should_memory_backing_be_mappable,
                                                       bool                              should_memory_backing_be_coherent,
                                                       bool                              is_mutable,
                                                       VkImageLayout                     post_create_image_layout,
                                                       const std::vector<MipmapRawData>* mipmaps_ptr);

        /** Wrapper constructor for existing VkImage instances, as reported for
         *  swapchain images. Object instantiated with this constructor will NOT
         *  release the specified VkImage instance.
         *
         *  The image will NOT be transitioned to any specific image layout.
         *
         *  For argument discussion, see specification of the other create() functions.
         **/
        static std::shared_ptr<Image> create_nonsparse(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                       const VkSwapchainCreateInfoKHR&  swapchain_create_info,
                                                       VkImage                          image);

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
         *  @param device_ptr               Device to use.
         *  @param type                     Vulkan image type to use.
         *  @param format                   Vulkan format to use.
         *  @param tiling                   Vulkan image tiling to use.
         *  @param usage                    Vulkan image usage pattern to use.
         *  @param base_mipmap_width        Width of the base mip-map. Must be at least 1 for all image types.
         *  @param base_mipmap_height       Height of the base mip-map. Must be at least 1 for all image types.
         *  @param base_mipmap_depth        Depth of the base mip-map. Must be at least 1 for all image types.
         *  @param n_layers                 Number of layers to use. Must be at least 1 for all image types.
         *  @param sample_count             Sample count to use.
         *  @param queue_families           A combination of Anvil::QUEUE_FAMILY_* bits, indicating which device queues
         *                                  the image is going to be accessed by.
         *  @param sharing_mode             Vulkan sharing mode to use.
         *  @param use_full_mipmap_chain    true, if all mipmaps should be created for the image. False to make the image
         *                                  only use one mip.
         *  @param is_mutable               true if the image should be initialized as a mutable object.
         *  @param sparse_residency_scope   Scope of sparse residency to request for the image.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
         static std::shared_ptr<Image> create_sparse(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                     VkImageType                      type,
                                                     VkFormat                         format,
                                                     VkImageTiling                    tiling,
                                                     VkImageUsageFlags                usage,
                                                     uint32_t                         base_mipmap_width,
                                                     uint32_t                         base_mipmap_height,
                                                     uint32_t                         base_mipmap_depth,
                                                     uint32_t                         n_layers,
                                                     VkSampleCountFlagBits            sample_count,
                                                     Anvil::QueueFamilyBits           queue_families,
                                                     VkSharingMode                    sharing_mode,
                                                     bool                             use_full_mipmap_chain,
                                                     bool                             is_mutable,
                                                     Anvil::SparseResidencyScope      residency_scope);

        /** Destructor */
        virtual ~Image();

        /** Returns subresource layout for an aspect for user-specified mip of an user-specified layer.
         *
         *  May only be used for linear images.
         *
         *  NOTE: This information is cached at image creation time, so the driver's impl will not be
         *        called.
         */
        bool get_aspect_subresource_layout(VkImageAspectFlags   aspect,
                                           uint32_t             n_layer,
                                           uint32_t             n_mip,
                                           VkSubresourceLayout* out_subresource_layout_ptr) const;

        /** Returns the underlying VkImage instance */
        const VkImage& get_image() const
        {
            return m_image;
        }

        /** Returns information about the data alignment required by the underlying VkImage instance */
        VkDeviceSize get_image_alignment() const
        {
            return m_alignment;
        }

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

        /** Returns information about size of the mipmap at index @param n_mipmap.
         *
         *  @param n_mipmap           Index of the mipmap to use for the query.
         *  @param opt_out_width_ptr  If not nullptr, deref will be set to the width of the queried mipmap,
         *                            assuming the function returns ture.
         *  @param opt_out_height_ptr If not nullptr, deref will be set to the height of the queried mipmap,
         *                            assuming the function returns true.
         *  @param opt_out_depth_ptr  If not nullptr, deref will be set to the depth of the queried mipmap,
         *                            assuming the function returns true.
         *
         *  @return true if @param n_mipmap was a valid mipmap index; false otherwise.
         **/
        bool get_image_mipmap_size(uint32_t  n_mipmap,
                                   uint32_t* opt_out_width_ptr,
                                   uint32_t* opt_out_height_ptr,
                                   uint32_t* opt_out_depth_ptr) const;

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

        /** Returns memory block used by the Image wrapper */
        std::shared_ptr<Anvil::MemoryBlock> get_memory_block() const
        {
            return m_memory_block_ptr;
        }

        /** Returns VkMemoryRequirements structure, as reported by the driver for this Image instance. */
        const VkMemoryRequirements& get_memory_requirements() const
        {
            return m_memory_reqs;
        }

        /** Returns name associated with the image. */
        std::string get_name() const
        {
            return m_name;
        }

        /** Returns a structure filled with details required to correctly bind tiles to a sparse image.
         *
         *  This function can only be called against sparse images with ALIASED or NONALIASED residency.
         **/
        bool get_sparse_image_aspect_properties(const VkImageAspectFlagBits                aspect,
                                                const Anvil::SparseImageAspectProperties** out_result_ptr_ptr) const;

        /** Returns a filled subresource range descriptor, covering all layers & mipmaps of the image */
        VkImageSubresourceRange get_subresource_range() const;

        /** Tells whether this image provides data for the specified image aspects.
         *
         *  @param aspects A bitfield of image aspect bits which should be used for the query.
         *
         *  @return true if data for all specified aspects is provided by the image, false otherwise.
         */
        bool has_aspects(VkImageAspectFlags aspects) const;

        /** Tells whether the image object has been created with mutability enabled. Mutability lets you
         *  create image views with formats that are compatible with, but not necessarily the same as, the
         *  format the image has been initialized with.
         **/
        bool is_image_mutable() const
        {
            return m_is_mutable;
        }

        /** Tells whether a physical memory page is assigned to the specified texel location.
         *
         *  Must only be called for sparse images whose sparse residency is not NONE.
         *
         *  @param aspect  Image aspect to use for the query.
         *  @param n_layer Index of the layer to use for the query.
         *  @param n_mip   Index of the mip to use for the query
         *  @param x       X location of the texel.
         *  @param y       Y location of the texel.
         *  @param z       Z location of the texel.
         *
         *  @return true if physical memory is bound to the specified location, false otherwise.
         *
         */
        bool is_memory_bound_for_texel(VkImageAspectFlagBits aspect,
                                       uint32_t              n_layer,
                                       uint32_t              n_mip,
                                       uint32_t              x,
                                       uint32_t              y,
                                       uint32_t              z) const;

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
         *  @param memory_block_ptr Memory block to assign to the image
         *
         *  @return true if successful, false otherwise.
         **/
        bool set_memory(std::shared_ptr<Anvil::MemoryBlock> memory_block_ptr);

        /** Binds memory of swapchain image, specified at creation time, to this Image instance.
         *
         *  NOTE: This function must NOT be used for sparse images.
         *
         *  NOTE: It is illegal to change the memory backing, after one has been associated with an Image instance.
         *
         *  @param swapchain_image_index TODO
         *
         *  TODO
         **/
        bool set_memory(uint32_t swapchain_image_index);

        /** Assigns a name to the image. Used by DOT serializer. */
        void set_name(std::string name)
        {
            m_name = name;
        }

        /** Updates image with specified mip-map data. Blocks until the operation finishes executing.
         *
         *  Handles both linear and optimal images.
         *
         *  @param mipmaps_ptr              A vector of MipmapRawData items, holding mipmap data. Must not
         *                                  be NULL.
         *  @param current_image_layout     Image layout, that the image is in right now.
         *  @param out_new_image_layout_ptr Deref will be set to the image layout the image has been transitioned
         *                                  to, in order to perform the request. Must not be NULL.
         *
         **/
        void upload_mipmaps(const std::vector<MipmapRawData>* mipmaps_ptr,
                            VkImageLayout                     current_image_layout,
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
             *  @param x X location of the texel.
             *  @param y Y location of the texel.
             *  @param z Z location of the texel.
             *
             *  @return As per description.
             */
            uint32_t get_texture_space_xyz_to_block_mapping_index(uint32_t x,
                                                                  uint32_t y,
                                                                  uint32_t z) const
            {
                const uint32_t tile_x = x / tile_width;  
                const uint32_t tile_y = y / tile_height;
                const uint32_t tile_z = z / tile_depth;

                const uint32_t result = tile_z * n_tiles_x * n_tiles_y
                                      + tile_y * n_tiles_x
                                      + tile_x;

                anvil_assert(result < tile_to_block_mappings.size() );

                return result;
            }

            /** Converts a tile location, expressed in XYZ space, to a linearized index.
             *
             *  @param x X index of the tile.
             *  @param y Y index of the tile.
             *  @param z Z index of the tile.
             *
             *  @return As per description.
             */
            uint32_t get_tile_space_xyz_to_block_mapping_index(uint32_t tile_x,
                                                               uint32_t tile_y,
                                                               uint32_t tile_z) const
            {
                const uint32_t result = tile_z * n_tiles_x * n_tiles_y
                                      + tile_y * n_tiles_x
                                      + tile_x;

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
        Image(std::weak_ptr<Anvil::BaseDevice>  device_ptr,
              VkImageType                       type,
              VkFormat                          format,
              VkImageTiling                     tiling,
              VkSharingMode                     sharing_mode, 
              VkImageUsageFlags                 usage,
              uint32_t                          base_mipmap_width,
              uint32_t                          base_mipmap_height,
              uint32_t                          base_mipmap_depth,
              uint32_t                          n_layers,
              VkSampleCountFlagBits             sample_count,
              bool                              use_full_mipmap_chain,
              bool                              is_mutable,
              Anvil::QueueFamilyBits            queue_families,
              VkImageLayout                     post_create_image_layout,
              const std::vector<MipmapRawData>* opt_mipmaps_ptr);

        /** See corresponding create_nonsparse() function for specification **/
        Image(std::weak_ptr<Anvil::BaseDevice>  device_ptr,
              VkImageType                       type,
              VkFormat                          format,
              VkImageTiling                     tiling,
              VkSharingMode                     sharing_mode, 
              VkImageUsageFlags                 usage,
              uint32_t                          base_mipmap_width,
              uint32_t                          base_mipmap_height,
              uint32_t                          base_mipmap_depth,
              uint32_t                          n_layers,
              VkSampleCountFlagBits             sample_count,
              Anvil::QueueFamilyBits            queue_families,
              bool                              use_full_mipmap_chain,
              bool                              is_mutable,
              VkImageLayout                     post_create_image_layout,
              const std::vector<MipmapRawData>* mipmaps_ptr);

        /** See corresponding create_nonsparse() function for specification **/
        Image(std::weak_ptr<Anvil::BaseDevice> device_ptr,
              VkImage                          image,
              VkFormat                         format,
              VkImageTiling                    tiling,
              VkSharingMode                    sharing_mode, 
              VkImageUsageFlags                usage,
              uint32_t                         base_mipmap_width,
              uint32_t                         base_mipmap_height,
              uint32_t                         base_mipmap_depth,
              uint32_t                         n_layers,
              uint32_t                         n_mipmaps,
              VkSampleCountFlagBits            sample_count,
              uint32_t                         n_slices,
              bool                             is_mutable,
              Anvil::QueueFamilyBits           queue_families,
              VkImageCreateFlags               flags);

        /** See corresponding create_sparse() function for specification **/
        Image(std::weak_ptr<Anvil::BaseDevice> device_ptr,
              VkImageType                      type,
              VkFormat                         format,
              VkImageTiling                    tiling,
              VkImageUsageFlags                usage,
              uint32_t                         base_mipmap_width,
              uint32_t                         base_mipmap_height,
              uint32_t                         base_mipmap_depth,
              uint32_t                         n_layers,
              VkSampleCountFlagBits            sample_count,
              Anvil::QueueFamilyBits           queue_families,
              VkSharingMode                    sharing_mode,
              bool                             use_full_mipmap_chain,
              bool                             is_mutable,
              Anvil::SparseResidencyScope      residency_scope);

        Image           (const Image&);
        Image& operator=(const Image&);

        void init               (bool                 use_full_mipmap_chain,
                                 bool                 memory_mappable,
                                 bool                 memory_coherent,
                                 const VkImageLayout* start_image_layout_ptr);
        void init_mipmap_props  ();
        void init_page_occupancy(const std::vector<VkSparseImageMemoryRequirements>& memory_reqs);

        void set_memory_sparse(VkDeviceSize                        resource_offset,
                               VkDeviceSize                        size,
                               std::shared_ptr<Anvil::MemoryBlock> memory_block_ptr,
                               VkDeviceSize                        memory_block_start_offset);
        void set_memory_sparse(const VkImageSubresource&           subresource,
                               VkOffset3D                          offset,
                               VkExtent3D                          extent,
                               std::shared_ptr<Anvil::MemoryBlock> memory_block_ptr,
                               VkDeviceSize                        memory_block_start_offset);

        void transition_to_post_create_image_layout(VkAccessFlags src_access_mask,
                                                    VkImageLayout src_layout);

        /* Private members */
        VkSampleCountFlagsVariable(m_sample_count);
        VkImageUsageFlagsVariable (m_usage);

        typedef std::pair<uint32_t /* n_layer */, uint32_t /* n_mip */>         LayerMipKey;
        typedef std::map<LayerMipKey, VkSubresourceLayout>                      LayerMipToSubresourceLayoutMap;
        typedef std::map<VkImageAspectFlagBits, LayerMipToSubresourceLayoutMap> AspectToLayerMipToSubresourceLayoutMap;

        VkDeviceSize                           m_alignment;
        AspectToLayerMipToSubresourceLayoutMap m_aspects;
        uint32_t                               m_depth;
        std::weak_ptr<Anvil::BaseDevice>       m_device_ptr;
        VkImageCreateFlags                     m_flags;
        VkFormat                               m_format;
        bool                                   m_has_transitioned_to_post_create_layout;
        uint32_t                               m_height;
        VkImage                                m_image;
        bool                                   m_image_owner;
        bool                                   m_is_mutable;
        bool                                   m_is_sparse;
        bool                                   m_is_swapchain_image;
        VkMemoryRequirements                   m_memory_reqs;
        uint32_t                               m_memory_types;
        Mipmaps                                m_mipmaps;
        uint32_t                               m_n_layers;
        uint32_t                               m_n_mipmaps;
        uint32_t                               m_n_slices;
        std::string                            m_name;
        VkImageLayout                          m_post_create_layout;
        Anvil::QueueFamilyBits                 m_queue_families;
        Anvil::SparseResidencyScope            m_residency_scope;
        VkSharingMode                          m_sharing_mode;
        VkDeviceSize                           m_storage_size;
        bool                                   m_swapchain_memory_assigned; /* only used for images which can be bound swapchain memory */
        std::shared_ptr<Anvil::Swapchain>      m_swapchain_ptr;             /* only used for images which can be bound a swapchain      */
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
