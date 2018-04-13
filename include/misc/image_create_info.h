//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef MISC_IMAGE_CREATE_INFO_H
#define MISC_IMAGE_CREATE_INFO_H

#include "misc/types.h"

namespace Anvil
{
    class ImageCreateInfo
    {
    public:
        /* Public functions */

        void clear_mipmaps_to_upload()
        {
            m_mipmaps_to_upload.clear();
        }

        /** Returns an instance of the "create info" item which can be used to instantiate a new non-sparse Image
         *  instance *WITH* a memory backing. 
         *
         *  This constructor assumes the image should be initialized in UNDEFINED layout, if no mipmap data
         *  is specified, or PREINITIALIZED otherwise. In the latter case, it will then proceed with filling
         *  the storage with mipmap data (if @param in_mipmaps_ptr is not nullptr), and finally transition
         *  the image to the @param in_post_create_image_layout layout.
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - External memory handle types: none
         * - MT safety:                    MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
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
         *  @param in_use_full_mipmap_chain    true if all mipmaps should be created for the image. False to only allocate
         *                                     storage for the base mip-map.
         *  @param in_memory_features          Memory features for the memory backing.
         *  @param in_create_flags             Optional image features that the created image should support.
         *  @param in_post_alloc_image_layout  Image layout to transfer the image to after it is created, assigned memory,
         *                                     and optionally uploaded mip data (if @param in_opt_mipmaps_ptr is not null).
         *  @param in_opt_mipmaps_ptr          If not nullptr, specified MipmapRawData items will be used to drive the mipmap contents
         *                                     initialization process. Ignored if nullptr.
         *                                     Specifying a non-NULL in_opt_mipmaps_ptr argument will make the function OR
         *                                     @param in_usage with VK_IMAGE_USAGE_TRANSFER_DST_BIT.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
        static ImageCreateInfoUniquePtr create_nonsparse_alloc(const Anvil::BaseDevice*          in_device_ptr,
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
                                                               VkImageLayout                     in_post_alloc_image_layout,
                                                               const std::vector<MipmapRawData>* in_opt_mipmaps_ptr = nullptr);

        /** Returns an instance of the "create info" item which can be used to instantiate a new non-sparse Image
         *  instance *without* a memory backing. A memory region should be bound to the object by calling
         *  Image::set_memory() before using the object for any operations.
         *
         *  The function can also optionally fill the image with data, as soon as memory backing is
         *  attached. To make it do so, pass a non-null ptr to a MipmapRawData vector via the @param
         *  mipmaps_ptr argument.
         *
         *  If this constructor is used, the image can be transformed automatically to the right layout
         *  at set_memory() call time by setting @param in_final_image_layout argument to a value other
         *  than VK_IMAGE_LAYOUT_UNDEFINED and VK_IMAGE_LAYOUT_PREINITIALIZED.
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - External memory handle types: none
         * - MT safety:                    MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
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
         *  @param in_post_alloc_image_layout  Image layout to transfer the image to after it is created, assigned memory,
         *                                     and optionally uploaded mip data (if @param in_opt_mipmaps_ptr is not null).
         *  @param in_opt_mipmaps_ptr          If not NULL, specified data will be used to initialize created image's mipmaps
         *                                     with content, as soon as the image is assigned a memory backing.
         *                                     Specifying a non-NULL @param in_opt_mipmaps_ptr argument will make the function OR
         *                                     @param in_usage with VK_IMAGE_USAGE_TRANSFER_DST_BIT.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
        static ImageCreateInfoUniquePtr create_nonsparse_no_alloc(const Anvil::BaseDevice*          in_device_ptr,
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
                                                                  ImageCreateFlags                  in_create_flags,
                                                                  VkImageLayout                     in_post_alloc_image_layout,
                                                                  const std::vector<MipmapRawData>* in_opt_mipmaps_ptr = nullptr);

        /** Returns an instance of the "create info" item which can be used to instantiate a new sparse Image
         *  instance *without* a physical memory backing.
         *
         *  Memory region(s) should be bound to the object by calling Image::set_memory_block() before using the object
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
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - External memory handle types: none
         * - Initial layout:               VK_IMAGE_LAYOUT_UNDEFINED
         * - MT safety:                    MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
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
        static ImageCreateInfoUniquePtr create_sparse_no_alloc(const Anvil::BaseDevice*    in_device_ptr,
                                                               VkImageType                 in_type,
                                                               VkFormat                    in_format,
                                                               VkImageTiling               in_tiling,
                                                               VkImageUsageFlags           in_usage,
                                                               uint32_t                    in_base_mipmap_width,
                                                               uint32_t                    in_base_mipmap_height,
                                                               uint32_t                    in_base_mipmap_depth,
                                                               uint32_t                    in_n_layers,
                                                               VkSampleCountFlagBits       in_sample_count,
                                                               Anvil::QueueFamilyBits      in_queue_families,
                                                               VkSharingMode               in_sharing_mode,
                                                               bool                        in_use_full_mipmap_chain,
                                                               ImageCreateFlags            in_create_flags,
                                                               Anvil::SparseResidencyScope in_residency_scope);

        /** Returns an instance of the "create info" item which can be used to instantiate a special type of an Image,
         *  useful for embedding a swapchain image instance. Object instantiated with this create item will NOT
         *  release the specified VkImage instance at its tear-down time.
         *
         *  The image will NOT be transitioned to any specific image layout.
         *
         *  For argument discussion, see specification of the other create() functions.
         *
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety: MT_SAFETY_INHERIT_FROM_PARENT_DEVICE
         **/
        static ImageCreateInfoUniquePtr create_swapchain_wrapper(const Anvil::BaseDevice* in_device_ptr,
                                                                 const Anvil::Swapchain*  in_swapchain_ptr,
                                                                 const VkImage&           in_image,
                                                                 const uint32_t&          in_n_swapchain_image);

        const uint32_t& get_base_mip_depth() const
        {
            return m_depth;
        }

        const uint32_t& get_base_mip_height() const
        {
            return m_height;
        }

        const uint32_t& get_base_mip_width() const
        {
            return m_width;
        }

        const Anvil::ImageCreateFlags& get_create_flags() const
        {
            return m_create_flags;
        }

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        const Anvil::ExternalMemoryHandleTypeFlags& get_external_memory_handle_types() const
        {
            return m_external_memory_handle_types;
        }

        const VkFormat& get_format() const
        {
            return m_format;
        }

        const Anvil::MemoryFeatureFlags& get_memory_features() const
        {
            return m_memory_features;
        }

        /* NOTE: This func is a const since it should only be accessed by Anvil::Image. */
        const std::vector<MipmapRawData>& get_mipmaps_to_upload()
        {
            return m_mipmaps_to_upload;
        }

        const Anvil::MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }

        const uint32_t& get_n_layers() const
        {
            return m_n_layers;
        }

        const VkImageLayout& get_post_alloc_image_layout() const
        {
            return m_post_alloc_layout;
        }

        const VkImageLayout& get_post_create_image_layout() const
        {
            return m_post_create_layout;
        }

        /** Returns queue families compatible with the image */
        const Anvil::QueueFamilyBits& get_queue_families() const
        {
            return m_queue_families;
        }

        VkSampleCountFlagBits get_sample_count() const
        {
            return static_cast<VkSampleCountFlagBits>(m_sample_count);
        }

        const SparseResidencyScope& get_residency_scope() const
        {
            return m_residency_scope;
        }

        const VkSharingMode& get_sharing_mode() const
        {
            return m_sharing_mode;
        }

        const Anvil::Swapchain* get_swapchain() const
        {
            anvil_assert(m_type == Anvil::ImageType::SWAPCHAIN_WRAPPER);

            return m_swapchain_ptr;
        }

        const VkImage& get_swapchain_image() const
        {
            anvil_assert(m_type == Anvil::ImageType::SWAPCHAIN_WRAPPER);

            return m_swapchain_image;
        }

        const uint32_t& get_swapchain_image_index() const
        {
            anvil_assert(m_type == Anvil::ImageType::SWAPCHAIN_WRAPPER);

            return m_n_swapchain_image;
        }

        /** Returns image tiling */
        const VkImageTiling& get_tiling() const
        {
            return m_tiling;
        }

        const Anvil::ImageType& get_type() const
        {
            return m_type;
        }

        const VkImageType& get_type_vk() const
        {
            return m_type_vk;
        }

        const VkImageUsageFlags& get_usage_flags() const
        {
            return m_usage_flags;
        }

        /** Tells whether this Image wrapper instance holds a sparse image */
        bool is_sparse() const
        {
            return (m_type == Anvil::ImageType::SPARSE_NO_ALLOC);
        }

        //-
        void set_create_flags(const Anvil::ImageCreateFlags& in_create_flags)
        {
            m_create_flags = in_create_flags;
        }

        void set_depth(const uint32_t& in_depth)
        {
            m_depth = in_depth;
        }

        void set_external_memory_handle_types(const ExternalMemoryHandleTypeFlags& in_external_memory_handle_types)
        {
            m_external_memory_handle_types = in_external_memory_handle_types;
        }

        void set_format(const VkFormat& in_format)
        {
            m_format = in_format;
        }

        void set_height(const uint32_t& in_height)
        {
            m_height = in_height;
        }

        void set_memory_features(const Anvil::MemoryFeatureFlags& in_memory_features)
        {
            m_memory_features = in_memory_features;
        }

        void set_mipmaps_to_upload(const std::vector<MipmapRawData>& in_mipmaps_to_upload)
        {
            m_mipmaps_to_upload = in_mipmaps_to_upload;
        }

        void set_mt_safety(const Anvil::MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        void set_n_layers(const uint32_t& in_n_layers)
        {
            m_n_layers = in_n_layers;
        }

        void set_post_alloc_layout(const VkImageLayout& in_post_alloc_layout)
        {
            m_post_alloc_layout = in_post_alloc_layout;
        }

        void set_post_create_layout(const VkImageLayout& in_post_create_layout)
        {
            m_post_create_layout = in_post_create_layout;
        }

        void set_queue_families(const Anvil::QueueFamilyBits& in_queue_families)
        {
            m_queue_families = in_queue_families;
        }

        void set_residency_scope(const Anvil::SparseResidencyScope& in_residency_scope)
        {
            m_residency_scope = in_residency_scope;
        }

        void set_sample_count(const VkSampleCountFlags& in_sample_count)
        {
            m_sample_count = in_sample_count;
        }

        void set_sharing_mode(const VkSharingMode& in_sharing_mode)
        {
            m_sharing_mode = in_sharing_mode;
        }

        void set_tiling(const VkImageTiling& in_tiling)
        {
            m_tiling = in_tiling;
        }

        void set_usage_flags(const VkImageUsageFlags& in_usage_flags)
        {
            m_usage_flags = in_usage_flags;
        }

        void set_uses_full_mipmap_chain(const bool& in_use_full_mipmap_chain)
        {
            m_use_full_mipmap_chain = in_use_full_mipmap_chain;
        }

        void set_width(const uint32_t& in_width)
        {
            m_width = in_width;
        }

        const bool& uses_full_mipmap_chain() const
        {
            return m_use_full_mipmap_chain;
        }

    private:
        /* Private functions */

        ImageCreateInfo(Anvil::ImageType                     in_type,
                        const Anvil::BaseDevice*             in_device_ptr,
                        VkImageType                          in_type_vk,
                        VkFormat                             in_format,
                        VkImageTiling                        in_tiling,
                        VkSharingMode                        in_sharing_mode, 
                        VkImageUsageFlags                    in_usage,
                        uint32_t                             in_base_mipmap_width,
                        uint32_t                             in_base_mipmap_height,
                        uint32_t                             in_base_mipmap_depth,
                        uint32_t                             in_n_layers,
                        VkSampleCountFlagBits                in_sample_count,
                        bool                                 in_use_full_mipmap_chain,
                        ImageCreateFlags                     in_create_flags,
                        Anvil::QueueFamilyBits               in_queue_families,
                        VkImageLayout                        in_post_create_image_layout,
                        const VkImageLayout&                 in_post_alloc_image_layout,
                        const std::vector<MipmapRawData>*    in_opt_mipmaps_ptr,
                        const Anvil::MTSafety&               in_mt_safety,
                        Anvil::ExternalMemoryHandleTypeFlags in_external_memory_handle_types,
                        const Anvil::MemoryFeatureFlags&     in_memory_features,
                        const Anvil::SparseResidencyScope&   in_residency_scope);

        /* Private variables */

        Anvil::ImageCreateFlags                m_create_flags;
        uint32_t                               m_depth;
        const Anvil::BaseDevice*               m_device_ptr;
        Anvil::ExternalMemoryHandleTypeFlags   m_external_memory_handle_types;
        VkFormat                               m_format;
        uint32_t                               m_height;
        Anvil::MemoryFeatureFlags              m_memory_features;
        std::vector<MipmapRawData>             m_mipmaps_to_upload;
        Anvil::MTSafety                        m_mt_safety;
        uint32_t                               m_n_layers;
        VkImageLayout                          m_post_alloc_layout;
        VkImageLayout                          m_post_create_layout;
        Anvil::QueueFamilyBits                 m_queue_families;
        Anvil::SparseResidencyScope            m_residency_scope;
        VkSampleCountFlags                     m_sample_count;
        VkSharingMode                          m_sharing_mode;
        VkImageTiling                          m_tiling;
        const Anvil::ImageType                 m_type;
        const VkImageType                      m_type_vk;
        VkImageUsageFlags                      m_usage_flags;
        bool                                   m_use_full_mipmap_chain;
        uint32_t                               m_width;

        /* Only used for swapchain wrapper images */
        uint32_t                m_n_swapchain_image;
        VkImage                 m_swapchain_image;
        const Anvil::Swapchain* m_swapchain_ptr;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(ImageCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(ImageCreateInfo);
    };

}; /* namespace Anvil */

#endif /* MISC_IMAGE_CREATE_INFO_H */