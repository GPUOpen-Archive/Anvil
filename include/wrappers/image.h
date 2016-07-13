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

/** Defines an image wrapper class which simplifies the following processes:
 *
 *  - Image initialization and tear-down.
 *  - Mip-map data updates.
 *  - Mip-map size caching
 **/
#ifndef WRAPPERS_IMAGE_H
#define WRAPPERS_IMAGE_H

#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
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

    /** Defines data for a single image mip-map.
     *
     *  Use one of the static create_() functions to set up structure fields according to the target
     *  image type.
     **/
    typedef struct MipmapRawData
    {
        /* Image aspect the mip-map data is specified for. */
        VkImageAspectFlagBits aspect;

        /* Start layer index */
        uint32_t n_layer;

        /* Number of layers to update */
        uint32_t n_layers;

        /* Number of 3D texture slices to update. For non-3D texture types, this field
         * should be set to 1. */
        uint32_t n_slices;


        /* Index of the mip-map to update. */
        uint32_t  n_mipmap;


        /* Pointer to a buffer holding raw data representation. The data structure is characterized by
         * data_size, row_size and slice_size fields.
         *
         * It is assumed the data under the pointer is tightly packed, and stored in column->row->slice->layer
         * order.
         */
        const void* linear_tightly_packed_data_ptr;


        /* Total number of bytes available for reading under linear_tightly_packed_data_ptr */
        uint32_t data_size;

        /* Number of bytes each row takes */
        uint32_t row_size;


        /** Creates a MipmapRawData instance which can be used to upload data to 1D Image instances:
         *
         *  @param in_aspect                         Image aspect to modify.
         *  @param in_n_mipmap                       Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr Pointer to an array holding raw mip-map data.
         *  @param in_row_size                       Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_1d_texture_mipmap_raw_data(VkImageAspectFlagBits aspect,
                                                               uint32_t              n_mipmap,
                                                               const void*           linear_tightly_packed_data_ptr,
                                                               uint32_t              row_size);

        /** Creates a MipmapRawData instance which can be used to upload data to 1D Array Image instances:
         *
         *  @param in_aspect                         Image aspect to modify.
         *  @param in_n_layer                        Index of a texture layer the mip-map data should be uploaded to.
         *  @param in_n_layers                       Number of texture layers to be updated.
         *  @param in_n_mipmap                       Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr Pointer to an array holding raw mip-map data.
         *  @param in_row_size                       Number of bytes each texture row takes.
         *  @param in_data_size                      Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_1d_array_texture_mipmap_raw_data(VkImageAspectFlagBits in_aspect,
                                                                     uint32_t              in_n_layer,
                                                                     uint32_t              in_n_layers,
                                                                     uint32_t              in_n_mipmap,
                                                                     const void*           in_linear_tightly_packed_data_ptr,
                                                                     uint32_t              in_row_size,
                                                                     uint32_t              in_data_size);

        /** Creates a MipmapRawData instance which can be used to upload data to 2D Image instances:
         *
         *  @param in_aspect                         Image aspect to modify.
         *  @param in_n_mipmap                       Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr Pointer to an array holding raw mip-map data.
         *  @param in_data_size                      Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *  @param in_row_size                       Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_2d_texture_mipmap_raw_data(VkImageAspectFlagBits aspect,
                                                               uint32_t              n_mipmap,
                                                               const void*           linear_tightly_packed_data_ptr,
                                                               uint32_t              data_size,
                                                               uint32_t              row_size);

        /** Creates a MipmapRawData instance which can be used to upload data to 2D Array Image instances:
         *
         *  @param in_aspect                         Image aspect to modify.
         *  @param in_n_layer                        Index of a texture layer the mip-map data should be uploaded to.
         *  @param in_n_layers                       Number of texture layers to be updated.
         *  @param in_n_mipmap                       Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr Pointer to an array holding raw mip-map data.
         *  @param in_data_size                      Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *  @param in_row_size                       Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_2d_array_texture_mipmap_raw_data(VkImageAspectFlagBits aspect,
                                                                     uint32_t              n_layer,
                                                                     uint32_t              n_layers,
                                                                     uint32_t              n_mipmap,
                                                                     const void*           linear_tightly_packed_data_ptr,
                                                                     uint32_t              data_size,
                                                                     uint32_t              row_size);

        /** Creates a MipmapRawData instnce which can be used to upload data to 3D Image instances:
         *
         *  @param in_aspect                         Image aspect to modify.
         *  @param in_n_layer                        Index of a texture layer the mip-map data should be uploaded to.
         *  @param in_n_slices                       Number of texture slices to be updated.
         *  @param in_n_mipmap                       Index of the mipmap to be updated.
         *  @param in_linear_tightly_packed_data_ptr Pointer to an array holding raw mip-map data.
         *  @param in_data_size                      Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *  @param in_row_size                       Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_3d_texture_mipmap_raw_data(VkImageAspectFlagBits aspect,
                                                               uint32_t              n_layer,
                                                               uint32_t              n_layer_slices,
                                                               uint32_t              n_mipmap,
                                                               const void*           linear_tightly_packed_data_ptr,
                                                               uint32_t              data_size,
                                                               uint32_t              row_size);

        /** Creates a MipmapRawData instance which can be used to upload data to Cube Map Image instances:
         *
         *  @param aspect                         Image aspect to modify.
         *  @param n_layer                        Index of a texture layer the mip-map data should be uploaded to.
         *                                        Valid values and corresponding cube map faces: 0: -X, 1: -Y, 2: -Z, 3: +X, 4: +Y, 5: +Z
         *  @param n_mipmap                       Index of the mipmap to be updated.
         *  @param linear_tightly_packed_data_ptr Pointer to an array holding raw mip-map data.
         *  @param data_size                      Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *  @param row_size                       Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_cube_map_texture_mipmap_raw_data(VkImageAspectFlagBits aspect,
                                                                     uint32_t              n_layer,
                                                                     uint32_t              n_mipmap,
                                                                     const void*           linear_tightly_packed_data_ptr,
                                                                     uint32_t              data_size,
                                                                     uint32_t              row_size);

        /** Creates a MipmapRawData instance which can be used to upload data to Cube Map Array Image instances:
         *
         *  @param aspect                         Image aspect to modify.
         *  @param n_layer                        Index of a texture layer the mip-map data should be uploaded to.
         *                                        Cube map faces, as selected for layer at index (n_layer % 6), are:
         *                                        0: -X, 1: -Y, 2: -Z, 3: +X, 4: +Y, 5: +Z
         *  @param n_layers                       Number of texture layers to update.
         *  @param n_mipmap                       Index of the mipmap to be updated.
         *  @param linear_tightly_packed_data_ptr Pointer to an array holding raw mip-map data.
         *  @param data_size                      Number of bytes available for reading under @param in_linear_tightly_packed_data_ptr.
         *  @param row_size                       Number of bytes each texture row takes.
         *
         *  @return As per description.
         **/
        static MipmapRawData create_cube_map_array_texture_mipmap_raw_data(VkImageAspectFlagBits aspect,
                                                                           uint32_t              n_layer,
                                                                           uint32_t              n_layers,
                                                                           uint32_t              n_mipmap,
                                                                           const void*           linear_tightly_packed_data_ptr,
                                                                           uint32_t              data_size,
                                                                           uint32_t              row_size);
    } MipmapRawData;

    /** A wrapper class for a VkImage and the bound VkMemory object. */
    class Image : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Initializes a new Image instance *without* a memory backing. A memory region should be bound
         *  to the object by calling Image::set_memory() before using the object for any operations.
         *
         *  If this constructor is used, it is user's responsibility to transfer the image into the right layout
         *  Having done so, set_creation_time_image_layout() should be called to update the image property.
         *
         *  @param device_ptr            Device to use.
         *  @param type                  Vulkan image type to use.
         *  @param format                Vulkan format to use.
         *  @param tiling                Vulkan image tiling to use.
         *  @param usage                 Vulkan image usage pattern to use.
         *  @param base_mipmap_width     Width of the base mip-map.
         *  @param base_mipmap_height    Height of the base mip-map. Must be at least 1 for all image types.
         *  @param base_mipmap_depth     Depth of the base mip-map. Must be at least 1 for all image types.
         *  @param n_layers              Number of layers to use. Must be at least 1 for all image types.
         *  @param sample_count          Sample count to use.
         *  @param use_full_mipmap_chain true, if all mipmaps should be created for the image. False to only allocate
         *                               storage for the base mip-map.
         *  @param is_mutable            true if the image should be initialized as a mutable object.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
         Image(Anvil::Device*        device_ptr,
               VkImageType           type,
               VkFormat              format,
               VkImageTiling         tiling,
               VkImageUsageFlags     usage,
               uint32_t              base_mipmap_width,
               uint32_t              base_mipmap_height,
               uint32_t              base_mipmap_depth,
               uint32_t              n_layers,
               VkSampleCountFlagBits sample_count,
               bool                  use_full_mipmap_chain,
               bool                  is_mutable);

        /** Initializes a new Image instance, along with a memory backing.
         *
         *  This constructor assumes the image should be initialized in UNDEFINED layout. It will then
         *  fill the storage with mipmap data (if @param mipmaps_ptr is not nullptr), and finally transition
         *  the image to the @param image_layout layout.
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
         *  @param final_image_layout                Layout to transition the new image to. Must not be VK_IMAGE_LAYOUT_UNDEFINED or
         *                                           VK_IMAGE_LAYOUT_PREINITIALIZED.
         *  @param mipmaps_ptr                       If not nullptr, specified MipmapRawData items will be used to drive the mipmap contents
         *                                           initialization process. Ignored if nullptr.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
         Image(Anvil::Device*                    device_ptr,
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
               VkImageLayout                     final_image_layout,
               const std::vector<MipmapRawData>* mipmaps_ptr);

         /** Wrapper constructor for existing VkImage instances. Object instantiated with
          *  this constructor will NOT release the specified VkImage instance.
          *
          *  In general, you should only use this constructor for swap-chain images.
          *
          *  For argument discussion, see specification of the other constructors.
          **/
         Image(Anvil::Device*        device_ptr,
               VkImage               image,
               VkFormat              format,
               VkImageTiling         tiling,
               VkImageUsageFlags     usage,
               uint32_t              base_mipmap_width,
               uint32_t              base_mipmap_height,
               uint32_t              base_mipmap_depth,
               uint32_t              n_layers,
               uint32_t              n_mipmaps,
               VkSampleCountFlagBits sample_count,
               uint32_t              n_slices,
               bool                  is_mutable);

        /** Returns the underlying VkImage instance */
        const VkImage& get_image() const
        {
            return m_image;
        }

        /** Returns information about the data alignment required by the underlying VkImage instance */
        VkDeviceSize get_image_alignment() const
        {
            return m_image_alignment;
        }

        /** Returns information about the image format used to create the underlying VkImage instance */
        VkFormat get_image_format() const
        {
            return m_image_format;
        }

        /** Returns the image layout this image instance was transferred to at creation time.
         *
         *  NOTE: This function will fail if called for an image instance, which was initialized
         *        without a memory backing.
         *
         *  @param out_result_ptr If function succeeds, this deref will be set to the requested value.
         *                        Will not be touched otherwise.
         *
         *  @return true if the function succeeds, false otherwise.
         **/
        bool get_image_layout_at_creation_time(VkImageLayout* out_result_ptr) const;

        /** Returns information about the memory types the underlying VkImage instance supports */
        uint32_t get_image_memory_types() const
        {
            return m_image_memory_types;
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
            return m_image_n_layers;
        }

        /** Returns information about the number of mipmaps stored by the underlying VkImage instance */
        uint32_t get_image_n_mipmaps() const
        {
            return m_image_n_mipmaps;
        }

        /** Returns information about the number of samples stored by the underlying VkImage instance */
        VkSampleCountFlagBits get_image_sample_count() const
        {
            return m_image_sample_count;
        }

        /** Returns information about the amount of memory the underlying VkImage instance requires
         *  to work correctly.
         **/
        VkDeviceSize get_image_storage_size() const
        {
            return m_image_storage_size;
        }

        /** Returns a filled subresource range descriptor, covering all layers & mipmaps of the image */
        VkImageSubresourceRange get_subresource_range() const;

        /** Tells whether the image object has been created with mutability enabled. Mutability lets you
         *  create image views with formats that are compatible with, but not necessarily the same as, the
         *  format used to initialize the image.
         **/
        bool is_image_mutable() const
        {
            return m_image_is_mutable;
        }

        /* Each image instance maintains a field called "creation-time image layout" which determines, what layout
         * the image has been transferred to from LAYOUT_UNDEFINED or LAYOUT_PREINITIALIZED layouts.
         *
         * NOTE: This function can ONLY be called for Image instances which have been created using constructors
         *       that do not take VkImageLayout as a parameter.
         * NOTE: This function can ONLY be called once.
         */
        void set_creation_time_image_layout(VkImageLayout new_image_layout);

        /** Binds the specified region of a Vulkan memory object to the Image and caches information
         *  about the new binding.
         *
         *  The function retains the specified memory block.
         *
         *  It is currently illegal to change the memory backing, after one has been associated with
         *  an Image instance.
         *
         *  @param memory       Memory object to use for the backing.
         *  @param size         Size of the memory region to use.
         *  @param start_offset Start offset of the memory region to use. 
         **/
        void set_memory(Anvil::MemoryBlock* memory_block_ptr);

        /** Returns an access mask which has all the access bits, relevant to the user-specified image layout,
         *  enabled. */
        static VkAccessFlags get_access_mask_from_image_layout(VkImageLayout layout);

    private:
        /* Private type declarations */
        typedef std::vector<Mipmap>     Mipmaps;

        /* Private functions */
        Image           (const Image&);
        Image& operator=(const Image&);

        virtual ~Image();

        void init             (VkImageType                       type,
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
                               bool                              memory_mappable,
                               bool                              memory_coherent,
                               const VkImageLayout*              start_image_layout_ptr,
                               const VkImageLayout*              final_image_layout_ptr,
                               const std::vector<MipmapRawData>* mipmaps_ptr);
        void init_mipmap_props();
        void upload_mipmaps   (const std::vector<Anvil::MipmapRawData>& mipmaps,
                               VkImageLayout*                           out_new_image_layout_ptr);

        /* Private members */
        Anvil::Device*        m_device_ptr;
        VkImage               m_image;
        VkDeviceSize          m_image_alignment;
        uint32_t              m_image_depth;
        VkFormat              m_image_format;
        uint32_t              m_image_height;
        bool                  m_image_is_mutable;
        VkImageLayout         m_image_layout_at_creation;
        uint32_t              m_image_memory_types;
        Mipmaps               m_image_mipmaps;
        uint32_t              m_image_n_layers;
        uint32_t              m_image_n_mipmaps;
        uint32_t              m_image_n_slices;
        bool                  m_image_owner;
        VkSampleCountFlagBits m_image_sample_count;
        VkDeviceSize          m_image_storage_size;
        VkImageTiling         m_image_tiling;
        VkImageUsageFlagBits  m_image_usage;
        uint32_t              m_image_width;

        Anvil::MemoryBlock* m_memory_block_ptr;
        bool                m_memory_owner;
    };

    /** Delete functor. Useful if you need to wrap the image instance in an auto pointer */
    struct ImageDeleter
    {
        void operator()(Image* image_ptr) const
        {
            image_ptr->release();
        }
    };
}; /* Vulkan namespace */

#endif /* WRAPPERS_IMAGE_H */
