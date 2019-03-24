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
#include "misc/mt_safety.h"
#include "misc/types.h"
#include "misc/page_tracker.h"
#include <unordered_map>


namespace Anvil
{
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
         * callback_arg: Pointer to IsImageMemoryAllocPendingQueryCallbackArgument instance.
         **/
        IMAGE_CALLBACK_ID_IS_ALLOC_PENDING,

        /* Call-back issued when no memory block is assigned to the image wrapper instance and
         * someone has just requested it.
         *
         * This call-back is needed for memory allocator to support implicit bake operations.
         *
         * callback_arg: Pointer to OnMemoryBlockNeededForImageCallbackArgument instance.
         **/
        IMAGE_CALLBACK_ID_MEMORY_BLOCK_NEEDED,

        /* Always last */
        IMAGE_CALLBACK_ID_COUNT
    };

    /** A wrapper class for a VkImage and the bound VkMemory object. */
    class Image : public CallbacksSupportProvider,
                  public DebugMarkerSupportProvider<Image>,
                  public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        static Anvil::ImageUniquePtr create(Anvil::ImageCreateInfoUniquePtr in_create_info_ptr);

        /* Transitions the image from one layout to another.
         *
         * This is a blocking call.
         *
         * @param in_queue_ptr                    Queue to use for the transition. The specified queue must support pipeline barrier
         *                                        command. Must not be null.
         * @param in_src_access_mask              Source access mask to use for the transition.
         * @param in_src_layout                   Image layout to transfer from.
         * @param in_dst_access_mask              Destination access mask to use for the transition.
         * @param in_dst_layout                   Image layout to transfer to.
         * @param in_subresource_range            Subresource range to use for the transfer operation.
         * @param in_opt_n_wait_semaphores        Number of wait semaphores specified at @param in_opt_wait_semaphore_ptrs.
         *                                        May be 0.
         * @param in_opt_wait_dst_stage_mask_ptrs A raw array of wait destination stage masks, to be used at submission time. May be null if
         *                                        @param in_opt_n_wait_semaphores is 0.
         * @param in_opt_wait_semaphore_ptrs      A raw array of semaphores to wait on before proceeding with submission of a cmd buffer
         *                                        which changes the layout of the image. May be null if @param in_opt_n_wait_semaphores is 0.
         * @param in_opt_n_set_semaphores         Number of set semaphores specified at @param in_opt_set_semaphore_ptrs. May be 0.
         * @param in_opt_set_semaphore_ptrs       A raw array of semaphores to set upon finished execution of the image layout transfer command buffer.
         *                                        May be null if @param in_opt_n_set_semaphores is 0.
         */
        void change_image_layout(Anvil::Queue*                       in_queue_ptr,
                                 Anvil::AccessFlags                  in_src_access_mask,
                                 Anvil::ImageLayout                  in_src_layout,
                                 Anvil::AccessFlags                  in_dst_access_mask,
                                 Anvil::ImageLayout                  in_dst_layout,
                                 const Anvil::ImageSubresourceRange& in_subresource_range,
                                 const uint32_t                      in_opt_n_wait_semaphores        = 0,
                                 const Anvil::PipelineStageFlags*    in_opt_wait_dst_stage_mask_ptrs = nullptr,
                                 Anvil::Semaphore* const*            in_opt_wait_semaphore_ptrs      = nullptr,
                                 const uint32_t                      in_opt_n_set_semaphores         = 0,
                                 Anvil::Semaphore* const*            in_opt_set_semaphore_ptrs       = nullptr);


        /** Destructor */
        virtual ~Image();

        /** Returns subresource layout for an aspect for user-specified mip of an user-specified layer.
         *
         *  May only be used for linear images.
         *
         *  NOTE: This information is cached at image creation time, so the driver's impl will not be
         *        called.
         */
        bool get_aspect_subresource_layout(Anvil::ImageAspectFlagBits in_aspect,
                                           uint32_t                   in_n_layer,
                                           uint32_t                   in_n_mip,
                                           Anvil::SubresourceLayout*  out_subresource_layout_ptr) const;

        const Anvil::ImageCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

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
         *
         *  This behavior may optionally be disabled by setting @param in_bake_memory_if_necessary to false.
         *  Should only be done in special circumstances.
         *
         */
        const VkImage& get_image(const bool& in_bake_memory_if_necessary = true);

        /** Returns information about the data alignment required by the underlying VkImage instance */
        VkDeviceSize get_image_alignment(const uint32_t& in_n_plane) const
        {
            return m_plane_index_to_memory_properties_map.at(in_n_plane).alignment;
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

        /** Returns information about the memory types the underlying VkImage instance supports */
        uint32_t get_image_memory_types(const uint32_t& in_n_plane) const
        {
            return m_plane_index_to_memory_properties_map.at(in_n_plane).memory_types;
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

        /** Returns information about the amount of memory the underlying VkImage instance requires
         *  to work correctly.
         **/
        VkDeviceSize get_image_storage_size(const uint32_t& in_n_plane) const
        {
            return m_plane_index_to_memory_properties_map.at(in_n_plane).storage_size;
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
         *
         *  NOTE: The function always returns nullptr for PRTs. However, the callback takes place nevertheless.
         *
         *  @param in_n_plane Must be 0 for non-YUV or joint YUV images. For disjoint YUV images, the value must be between 0
         *                    and 2 (inclusive).
         **/
        Anvil::MemoryBlock* get_memory_block(const uint32_t& in_n_plane = 0);

        const uint32_t& get_n_mipmaps() const
        {
            return m_n_mipmaps;
        }

        /** Returns SFR tile size for the image.
         *
         *  Can only be called if the following requirements are met:
         *
         *  1. Parent device is a mGPU device instance.
         *  2. Image has been initialized with the VK_IMAGE_CREATE_BIND_SFR_BIT_KHR flag.
         *
         *  TODO.
         *
         */
        bool get_SFR_tile_size(VkExtent2D* out_result_ptr) const;

        /** Returns a structure filled with details required to correctly bind tiles to a sparse image.
         *
         *  This function can only be called against sparse images with ALIASED or NONALIASED residency.
         **/
        bool get_sparse_image_aspect_properties(const Anvil::ImageAspectFlagBits           in_aspect,
                                                const Anvil::SparseImageAspectProperties** out_result_ptr_ptr) const;

        /** Returns a filled subresource range descriptor, covering all layers & mipmaps of the image */
        Anvil::ImageSubresourceRange get_subresource_range() const;

        /** Tells whether this image provides data for the specified image aspects.
         *
         *  @param in_aspects A bitfield of image aspect bits which should be used for the query.
         *
         *  @return true if data for all specified aspects is provided by the image, false otherwise.
         */
        bool has_aspects(const Anvil::ImageAspectFlags& in_aspects) const;

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
        bool is_memory_bound_for_texel(Anvil::ImageAspectFlagBits in_aspect,
                                       uint32_t                   in_n_layer,
                                       uint32_t                   in_n_mip,
                                       uint32_t                   in_x,
                                       uint32_t                   in_y,
                                       uint32_t                   in_z) const;

        bool prefers_dedicated_allocation(const uint32_t& in_n_plane) const
        {
            return m_plane_index_to_memory_properties_map.at(in_n_plane).prefers_dedicated_allocation;
        }

        bool requires_dedicated_allocation(const uint32_t& in_n_plane) const
        {
            return m_plane_index_to_memory_properties_map.at(in_n_plane).requires_dedicated_allocation;
        }

        /** Binds the specified region of a Vulkan memory object to an Image and caches information
         *  about the new binding.
         *
         *  NOTE: This function can be used for both single- and multi-GPU devices. In case of the latter:
         *        1. The image must NOT have been created for a particular swapchain instance.
         *        2. The image must NOT be sparse.
         *        3. If @param in_memory_block_ptr uses memory taken from a heap without VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR
         *           flag, each physical device attaches to its own instance of the memory.
         *        4. If @param in_memory_block_ptr uses memory taken from a heap WITH the flag, each physical device attaches
         *           to memory instance 0.
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
        bool set_memory(MemoryBlockUniquePtr in_memory_block_ptr);
        bool set_memory(Anvil::MemoryBlock*  in_memory_block_ptr);

        /** Binds the image to instances of memory for physical devices specified by the caller.
         *
         *  NOTE: This function must NOT be used for sparse images.
         *  NOTE: This function must NOT be used for images created for a single-GPU device.
         *  NOTE: This function can only be used for images, whose memory comes off a heap with the
         *        VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR flag.
         *  NOTE: It is illegal to change the memory backing, after one has been associated with an Image instance.
         *
         *  TODO
         **/
        bool set_memory(MemoryBlockUniquePtr    in_memory_block_ptr,
                        uint32_t                in_n_device_group_indices,
                        const uint32_t*         in_device_group_indices_ptr);
        bool set_memory(Anvil::MemoryBlock*     in_memory_block_ptr,
                        uint32_t                in_n_device_group_indices,
                        const uint32_t*         in_device_group_indices_ptr);

        /** Binds the image to instances of memory, according to SFR rectangles specified by the caller.
         *  For more information, please read VK_KHR_device_group extension specification.
         *
         *  NOTE: This function must NOT be used for sparse images.
         *  NOTE: This function must NOT be used for images created for a single-GPU device.
         *  NOTE: It is illegal to change the memory backing, after one has been associated with an Image instance.
         *
         *  TODO
         **/
        bool set_memory(MemoryBlockUniquePtr in_memory_block_ptr,
                        uint32_t             in_n_SFR_rects,
                        const VkRect2D*      in_SFRs_ptr);
        bool set_memory(Anvil::MemoryBlock*  in_memory_block_ptr,
                        uint32_t             in_n_SFR_rects,
                        const VkRect2D*      in_SFRs_ptr);

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
                            Anvil::ImageLayout                in_current_image_layout,
                            Anvil::ImageLayout*               out_new_image_layout_ptr);

    private:
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
            std::vector<Anvil::MemoryBlock*> tile_to_block_mappings;

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

            uint32_t                                n_total_tail_pages;
            std::map<Anvil::MemoryBlock*, uint32_t> tail_pages_per_binding;
            std::vector<PageOccupancyStatus>        tail_occupancy;

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

        Image(Anvil::ImageCreateInfoUniquePtr in_create_info_ptr);

        bool do_sanity_checks_for_physical_device_binding(const Anvil::MemoryBlock* in_memory_block_ptr,
                                                          uint32_t                  in_n_physical_devices) const;
        bool do_sanity_checks_for_sfr_binding            (uint32_t                  in_n_SFR_rects,
                                                          const VkRect2D*           in_SFRs_ptr) const;

        bool init               ();
        void init_mipmap_props  ();
        void init_page_occupancy(const std::vector<Anvil::SparseImageMemoryRequirements>& in_memory_reqs);
        void init_sfr_tile_size ();

        bool set_memory_internal          (Anvil::MemoryBlock*    in_memory_block_ptr,
                                           bool                   in_owned_by_image,
                                           uint32_t               in_n_device_group_indices,
                                           const uint32_t*        in_device_group_indices_ptr,
                                           uint32_t               in_n_SFR_rects,
                                           const VkRect2D*        in_SFRs_ptr);
        bool set_swapchain_memory_internal(uint32_t               in_swapchain_image_index,
                                           uint32_t               in_opt_n_SFR_rects,
                                           const VkRect2D*        in_opt_SFRs_ptr,
                                           uint32_t               in_opt_n_device_indices,
                                           const uint32_t*        in_opt_device_indices);


        void on_memory_backing_update       (const Anvil::ImageSubresource& in_subresource,
                                             VkOffset3D                     in_offset,
                                             VkExtent3D                     in_extent,
                                             Anvil::MemoryBlock*            in_memory_block_ptr,
                                             VkDeviceSize                   in_memory_block_start_offset,
                                             bool                           in_memory_block_owned_by_image);
        void on_memory_backing_opaque_update(uint32_t                       in_n_plane,
                                             VkDeviceSize                   in_resource_offset,
                                             VkDeviceSize                   in_size,
                                             Anvil::MemoryBlock*            in_memory_block_ptr,
                                             VkDeviceSize                   in_memory_block_start_offset,
                                             bool                           in_memory_block_owned_by_image);

        void transition_to_post_alloc_image_layout(Anvil::AccessFlags in_src_access_mask,
                                                   Anvil::ImageLayout in_src_layout);

        /* Private members */
        typedef std::pair<uint32_t /* n_layer */, uint32_t /* n_mip */>              LayerMipKey;
        typedef std::map<LayerMipKey, Anvil::SubresourceLayout>                      LayerMipToSubresourceLayoutMap;
        typedef std::map<Anvil::ImageAspectFlagBits, LayerMipToSubresourceLayoutMap> AspectToLayerMipToSubresourceLayoutMap;

        /* NOTE: For YUV images, this map uses .._PLANE_x_..       aspects as keys. Non-disjoint YUV images only use PLANE_0 key.
         *       For all others, this map uses COLOR/DEPTH/STENCIL aspect keys.
         *
         * NOTE: This field is only used for linear images.
         */
        AspectToLayerMipToSubresourceLayoutMap m_linear_image_aspect_data;

        Anvil::ImageCreateInfoUniquePtr        m_create_info_ptr;
        bool                                   m_has_transitioned_to_post_alloc_layout;
        VkImage                                m_image;
        Mipmaps                                m_mipmap_props;
        uint32_t                               m_n_mipmaps;
        bool                                   m_swapchain_memory_assigned;

        struct PerPlaneMemoryProperties
        {
            VkDeviceSize                        alignment;
            uint32_t                            memory_types;
            std::unique_ptr<Anvil::PageTracker> page_tracker_ptr; /* only used for sparse binding images */
            bool                                prefers_dedicated_allocation;
            bool                                requires_dedicated_allocation;
            VkDeviceSize                        storage_size;

            PerPlaneMemoryProperties()
                :alignment                    (UINT64_MAX),
                 memory_types                 (0),
                 prefers_dedicated_allocation (false),
                 requires_dedicated_allocation(false),
                 storage_size                 (0)
            {
                /* Stub */
            }

            PerPlaneMemoryProperties(const VkDeviceSize& in_alignment,
                                     const uint32_t&     in_memory_types,
                                     const bool&         in_prefers_dedicated_allocation,
                                     const bool&         in_requires_dedicated_allocation,
                                     const VkDeviceSize& in_storage_size)
                :alignment                    (in_alignment),
                 memory_types                 (in_memory_types),
                 prefers_dedicated_allocation (in_prefers_dedicated_allocation),
                 requires_dedicated_allocation(in_requires_dedicated_allocation),
                 storage_size                 (in_storage_size)
            {
                /* Stub */
            }

            ANVIL_DISABLE_ASSIGNMENT_OPERATOR(PerPlaneMemoryProperties);
        };

        /* NOTE: This map always holds exactly one item (key: 0) for single-planar (ie.non-YUV) and joint YUV images. */
        std::unordered_map<uint32_t, PerPlaneMemoryProperties> m_plane_index_to_memory_properties_map;

        std::vector<uint32_t> m_peer_device_indices;
        std::vector<VkRect2D> m_peer_sfr_rects;
        VkExtent2D            m_sfr_tile_size;

        MemoryBlockUniquePtr              m_metadata_memory_block_ptr;
        std::vector<MemoryBlockUniquePtr> m_memory_blocks_owned;

        std::map<Anvil::ImageAspectFlagBits, AspectPageOccupancyData*>           m_sparse_aspect_page_occupancy;
        std::vector<std::unique_ptr<AspectPageOccupancyData> >                   m_sparse_aspect_page_occupancy_data_items_owned;
        std::map<Anvil::ImageAspectFlagBits, Anvil::SparseImageAspectProperties> m_sparse_aspect_props;

        friend class Anvil::Queue;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(Image);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(Image);
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_IMAGE_H */
