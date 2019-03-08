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

        /** Returns an instance of the "create info" item which can be used to instantiate a new Image
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
         * - Image format list:            empty (ie. image views created from the image can use any compatible format)
         * - MT safety:                    Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
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
         *  @param in_queue_families           A combination of Anvil::QueueFamilyFlagBits::* bits, indicating which device queues
         *                                     the image is going to be accessed by.
         *  @param in_sharing_mode             Vulkan sharing mode to use.
         *  @param in_use_full_mipmap_chain    true if all mipmaps should be created for the image. False to only allocate
         *                                     storage for the base mip-map.
         *  @param in_memory_features          Memory features for the memory backing.
         *  @param in_create_flags             Optional image features that the created image should support. Must not include SPARSE_ALIASED,
         *                                     SPARSE_BINDING and SPARSE_RESIDENCY bits.
         *  @param in_post_alloc_image_layout  Image layout to transfer the image to after it is created, assigned memory,
         *                                     and optionally uploaded mip data (if @param in_opt_mipmaps_ptr is not null).
         *  @param in_opt_mipmaps_ptr          If not nullptr, specified MipmapRawData items will be used to drive the mipmap contents
         *                                     initialization process. Ignored if nullptr.
         *                                     Specifying a non-NULL in_opt_mipmaps_ptr argument will make the function OR
         *                                     @param in_usage with Anvil::IMAGE_USAGE_FLAG_TRANSFER_DST_BIT.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
        static ImageCreateInfoUniquePtr create_alloc(const Anvil::BaseDevice*          in_device_ptr,
                                                     Anvil::ImageType                  in_type,
                                                     Anvil::Format                     in_format,
                                                     Anvil::ImageTiling                in_tiling,
                                                     Anvil::ImageUsageFlags            in_usage,
                                                     uint32_t                          in_base_mipmap_width,
                                                     uint32_t                          in_base_mipmap_height,
                                                     uint32_t                          in_base_mipmap_depth,
                                                     uint32_t                          in_n_layers,
                                                     Anvil::SampleCountFlagBits        in_sample_count,
                                                     Anvil::QueueFamilyFlags           in_queue_families,
                                                     Anvil::SharingMode                in_sharing_mode,
                                                     bool                              in_use_full_mipmap_chain,
                                                     Anvil::MemoryFeatureFlags         in_memory_features,
                                                     Anvil::ImageCreateFlags           in_create_flags,
                                                     Anvil::ImageLayout                in_post_alloc_image_layout,
                                                     const std::vector<MipmapRawData>* in_opt_mipmaps_ptr = nullptr);

        /** Returns an instance of the "create info" item which can be used to instantiate a new Image
         *  instance *without* a memory backing. A memory region should be bound to the object by calling
         *  Image::set_memory() before using the object for any operations.
         *
         *  The function can also optionally fill the image with data, as soon as memory backing is
         *  attached. To make it do so, pass a non-null ptr to a MipmapRawData vector via the @param
         *  mipmaps_ptr argument.
         *
         *  If this constructor is used, the image can be transformed automatically to the right layout
         *  at set_memory() call time by setting @param in_final_image_layout argument to a value other
         *  than Anvil::ImageLayout::UNDEFINED and Anvil::ImageLayout::PREINITIALIZED.
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - External memory handle types: none
         * - Image format list:            empty (ie. image views created from the image can use any compatible format)
         * - MT safety:                    Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         *
         *  @param in_device_ptr                  Device to use.
         *  @param in_type                        Vulkan image type to use.
         *  @param in_format                      Vulkan format to use.
         *  @param in_tiling                      Vulkan image tiling to use.
         *  @param in_usage                       Vulkan image usage pattern to use.
         *  @param in_base_mipmap_width           Width of the base mip-map.
         *  @param in_base_mipmap_height          Height of the base mip-map. Must be at least 1 for all image types.
         *  @param in_base_mipmap_depth           Depth of the base mip-map. Must be at least 1 for all image types.
         *  @param in_n_layers                    Number of layers to use. Must be at least 1 for all image types.
         *  @param in_sample_count                Sample count to use.
         *  @param in_queue_families              A combination of Anvil::QueueFamilyFlagBits::* bits, indicating which device queues
         *                                        the image is going to be accessed by.
         *  @param in_sharing_mode                Vulkan sharing mode to use.
         *  @param in_use_full_mipmap_chain       true, if all mipmaps should be created for the image. False to only allocate
         *                                        storage for the base mip-map.
         *  @param in_create_flags                Optional image features that the created image should support.
         *  @param in_opt_post_alloc_image_layout Image layout to transfer the image to after it is created, assigned memory,
         *                                        and optionally uploaded mip data (if @param in_opt_mipmaps_ptr is not null). Ignored for
         *                                        images with SPARSE_ALIASED, SPARSE_BINDING or SPARSE_RESIDENCY bits set.
         *  @param in_opt_mipmaps_ptr             If not NULL, specified data will be used to initialize created image's mipmaps
         *                                        with content, as soon as the image is assigned a memory backing.
         *                                        Specifying a non-NULL @param in_opt_mipmaps_ptr argument will make the function OR
         *                                        @param in_usage with Anvil::IMAGE_USAGE_FLAG_TRANSFER_DST_BIT.
         *
         *  @return New image instance, if successful, or nullptr otherwise.
         **/
        static ImageCreateInfoUniquePtr create_no_alloc(const Anvil::BaseDevice*          in_device_ptr,
                                                        Anvil::ImageType                  in_type,
                                                        Anvil::Format                     in_format,
                                                        Anvil::ImageTiling                in_tiling,
                                                        ImageUsageFlags                   in_usage,
                                                        uint32_t                          in_base_mipmap_width,
                                                        uint32_t                          in_base_mipmap_height,
                                                        uint32_t                          in_base_mipmap_depth,
                                                        uint32_t                          in_n_layers,
                                                        Anvil::SampleCountFlagBits        in_sample_count,
                                                        Anvil::QueueFamilyFlags           in_queue_families,
                                                        Anvil::SharingMode                in_sharing_mode,
                                                        bool                              in_use_full_mipmap_chain,
                                                        Anvil::ImageCreateFlags           in_create_flags,
                                                        Anvil::ImageLayout                in_post_alloc_image_layout = Anvil::ImageLayout::UNDEFINED,
                                                        const std::vector<MipmapRawData>* in_opt_mipmaps_ptr         = nullptr);

        /** Returns an instance of the "create info" item which can be used to instantiate a new non-sparse Image
         *  instance, later to be bound to the user-specified swapchain memory.
         *
         *  This function prototype may only be called for sGPU or mGPU devices which support the VK_KHR_device_group
         *  extension.
         *
         *  Requires VK_KHR_device_group support.
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - External memory handle types: none
         * - Image format list:            empty (ie. image views created from the image can use any compatible format)
         * - MT safety:                    Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         * - Physical devices:             none
         * - SFR rectangles:               none
         *
         *  For argument discussion, see specification of the other create() functions.
         **/
        static ImageCreateInfoUniquePtr create_peer_no_alloc(const Anvil::BaseDevice* in_device_ptr,
                                                             const Anvil::Swapchain*  in_swapchain_ptr,
                                                             uint32_t                 in_n_swapchain_image);

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
         * - MT safety: Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
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

        Anvil::ImageCreateFlags get_create_flags() const
        {
            return m_create_flags;
        }

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        const Anvil::ExternalMemoryHandleTypeFlags& get_external_memory_handle_types() const
        {
            return m_exportable_external_memory_handle_types;
        }

        const Anvil::Format& get_format() const
        {
            return m_format;
        }

        /* Tells which compatible image view formats have been specified for the image.
         *
         * Requires VK_KHR_image_format_list.
         *
         * @param out_n_image_view_formats_ptr   Deref will be set to the number of formats the array under *out_image_view_formats_ptr_ptr
         *                                       holds. Must not be nullptr.
         * @param out_image_view_formats_ptr_ptr Deref will be set to a ptr to an array of compatible formats.
         *
         **/
        void get_image_view_formats(uint32_t*             out_n_image_view_formats_ptr,
                                    const Anvil::Format** out_image_view_formats_ptr_ptr) const;

        const Anvil::ImageInternalType& get_internal_type() const
        {
            return m_internal_type;
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

        const std::vector<uint32_t> get_device_indices() const
        {
            anvil_assert(m_internal_type == Anvil::ImageInternalType::PEER_NO_ALLOC     ||
                         m_internal_type == Anvil::ImageInternalType::SWAPCHAIN_WRAPPER);

            return m_device_indices;
        }

        const Anvil::ImageLayout& get_post_alloc_image_layout() const
        {
            return m_post_alloc_layout;
        }

        const Anvil::ImageLayout& get_post_create_image_layout() const
        {
            return m_post_create_layout;
        }

        /** Returns queue families compatible with the image */
        const Anvil::QueueFamilyFlags& get_queue_families() const
        {
            return m_queue_families;
        }

        Anvil::SampleCountFlagBits get_sample_count() const
        {
            return m_sample_count;
        }

        const std::vector<VkRect2D>& get_sfr_rects() const
        {
            anvil_assert(m_internal_type == Anvil::ImageInternalType::PEER_NO_ALLOC     ||
                         m_internal_type == Anvil::ImageInternalType::SWAPCHAIN_WRAPPER);

            return m_sfr_rects;
        }

        const Anvil::SharingMode& get_sharing_mode() const
        {
            return m_sharing_mode;
        }

        const Anvil::ImageUsageFlags& get_stencil_image_aspect_usage() const
        {
            return m_usage_flags_stencil;
        }

        const Anvil::Swapchain* get_swapchain() const
        {
            anvil_assert(m_internal_type == Anvil::ImageInternalType::PEER_NO_ALLOC     ||
                         m_internal_type == Anvil::ImageInternalType::SWAPCHAIN_WRAPPER);

            return m_swapchain_ptr;
        }

        const VkImage& get_swapchain_image() const
        {
            anvil_assert(m_internal_type == Anvil::ImageInternalType::PEER_NO_ALLOC     ||
                         m_internal_type == Anvil::ImageInternalType::SWAPCHAIN_WRAPPER);

            return m_swapchain_image;
        }

        const uint32_t& get_swapchain_image_index() const
        {
            anvil_assert(m_internal_type == Anvil::ImageInternalType::PEER_NO_ALLOC     ||
                         m_internal_type == Anvil::ImageInternalType::SWAPCHAIN_WRAPPER);

            return m_n_swapchain_image;
        }

        /** Returns image tiling */
        const Anvil::ImageTiling& get_tiling() const
        {
            return m_tiling;
        }

        const Anvil::ImageType& get_type() const
        {
            return m_type_vk;
        }

        const ImageUsageFlags& get_usage_flags() const
        {
            return m_usage_flags;
        }

        /** Tells whether this Image wrapper instance holds a sparse image */
        bool is_sparse() const
        {
            return (m_create_flags & Anvil::ImageCreateFlagBits::SPARSE_BINDING_BIT) != 0;
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

        void set_exportable_external_memory_handle_types(const ExternalMemoryHandleTypeFlags& in_external_memory_handle_types)
        {
            m_exportable_external_memory_handle_types = in_external_memory_handle_types;
        }

        void set_format(const Anvil::Format& in_format)
        {
            m_format = in_format;
        }

        void set_height(const uint32_t& in_height)
        {
            m_height = in_height;
        }

        /* Lets the application specify what formats image views created off this image will use.
         *
         * This information will be chained to the image create info struct.
         *
         * Requires VK_KHR_image_format_list
         *
         * @param in_n_image_view_formats   Number of formats available for reading under @param in_image_view_formats_ptr.
         *                                  Must not be 0.
         * @param in_image_view_formats_ptr Image view formats to specify for the image. Must not be nullptr.
         *
         */
        void set_image_view_formats(const uint32_t&      in_n_image_view_formats,
                                    const Anvil::Format* in_image_view_formats_ptr);

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

        void set_post_alloc_layout(const Anvil::ImageLayout& in_post_alloc_layout)
        {
            m_post_alloc_layout = in_post_alloc_layout;
        }

        void set_post_create_layout(const Anvil::ImageLayout& in_post_create_layout)
        {
            m_post_create_layout = in_post_create_layout;
        }

        void set_queue_families(const Anvil::QueueFamilyFlags& in_queue_families)
        {
            m_queue_families = in_queue_families;
        }

        void set_sample_count(const Anvil::SampleCountFlagBits& in_sample_count)
        {
            m_sample_count = in_sample_count;
        }

        void set_device_indices(const uint32_t& in_device_index_count,
                                const uint32_t* in_devices_indices_ptr)
        {
            m_device_indices.clear();
            m_device_indices.insert(m_device_indices.begin(),
                                    in_devices_indices_ptr,
                                    in_devices_indices_ptr + in_device_index_count);
        }

        void set_SFR_rectangles(const uint32_t& in_n_SFR_rects,
                                const VkRect2D* in_SFRs_ptr)
        {
            m_sfr_rects.clear();

            for (uint32_t n_sfr_rect = 0;
                          n_sfr_rect < in_n_SFR_rects;
                        ++n_sfr_rect)
            {
                m_sfr_rects.push_back(in_SFRs_ptr[n_sfr_rect]);
            }
        }

        void set_sharing_mode(const Anvil::SharingMode& in_sharing_mode)
        {
            m_sharing_mode = in_sharing_mode;
        }

        /* Use this function to specify usage patterns for the stencil part of the image which is about to be created.
         * As per VK_EXT_separate_stencil_usage, various restrictions apply, amongst which the most important is that
         * @param in_usage has to be a subset of the usage flags specified globally for the image.
         *
         * Requires VK_EXT_separate_stencil_usage.
         */
        void set_stencil_image_aspect_usage(const Anvil::ImageUsageFlags& in_usage)
        {
            m_usage_flags_stencil = in_usage;
        }

        void set_swapchain(const Anvil::Swapchain* in_swapchain_ptr)
        {
            m_swapchain_ptr = in_swapchain_ptr;
        }

        void set_swapchain_image_index(const uint32_t& in_n_swapchain_image_index)
        {
            m_n_swapchain_image = in_n_swapchain_image_index;
        }

        void set_tiling(const Anvil::ImageTiling& in_tiling)
        {
            m_tiling = in_tiling;
        }

        void set_usage_flags(const ImageUsageFlags& in_usage_flags)
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

        ImageCreateInfo(Anvil::ImageInternalType             in_type,
                        const Anvil::BaseDevice*             in_device_ptr,
                        Anvil::ImageType                     in_type_vk,
                        Anvil::Format                        in_format,
                        Anvil::ImageTiling                   in_tiling,
                        Anvil::SharingMode                   in_sharing_mode,
                        ImageUsageFlags                      in_usage,
                        uint32_t                             in_base_mipmap_width,
                        uint32_t                             in_base_mipmap_height,
                        uint32_t                             in_base_mipmap_depth,
                        uint32_t                             in_n_layers,
                        Anvil::SampleCountFlagBits           in_sample_count,
                        bool                                 in_use_full_mipmap_chain,
                        ImageCreateFlags                     in_create_flags,
                        Anvil::QueueFamilyFlags              in_queue_families,
                        Anvil::ImageLayout                   in_post_create_image_layout,
                        const Anvil::ImageLayout&            in_post_alloc_image_layout,
                        const std::vector<MipmapRawData>*    in_opt_mipmaps_ptr,
                        const Anvil::MTSafety&               in_mt_safety,
                        Anvil::ExternalMemoryHandleTypeFlags in_exportable_external_memory_handle_types,
                        const Anvil::MemoryFeatureFlags&     in_memory_features);

        /* Private variables */

        Anvil::ImageCreateFlags              m_create_flags;
        uint32_t                             m_depth;
        const Anvil::BaseDevice*             m_device_ptr;
        Anvil::ExternalMemoryHandleTypeFlags m_exportable_external_memory_handle_types;
        Anvil::Format                        m_format;
        uint32_t                             m_height;
        std::vector<Anvil::Format>           m_image_view_formats;
        const Anvil::ImageInternalType       m_internal_type;
        Anvil::MemoryFeatureFlags            m_memory_features;
        std::vector<MipmapRawData>           m_mipmaps_to_upload;
        Anvil::MTSafety                      m_mt_safety;
        uint32_t                             m_n_layers;
        Anvil::ImageLayout                   m_post_alloc_layout;
        Anvil::ImageLayout                   m_post_create_layout;
        Anvil::QueueFamilyFlags              m_queue_families;
        Anvil::SampleCountFlagBits           m_sample_count;
        Anvil::SharingMode                   m_sharing_mode;
        Anvil::ImageTiling                   m_tiling;
        const Anvil::ImageType               m_type_vk;
        ImageUsageFlags                      m_usage_flags;
        ImageUsageFlags                      m_usage_flags_stencil;
        bool                                 m_use_full_mipmap_chain;
        uint32_t                             m_width;

        /* Only used for peer images */
        std::vector<uint32_t>   m_device_indices;
        std::vector<VkRect2D>   m_sfr_rects;

        /* Only used for swapchain wrapper images */
        uint32_t                m_n_swapchain_image;
        VkImage                 m_swapchain_image;
        const Anvil::Swapchain* m_swapchain_ptr;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(ImageCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(ImageCreateInfo);
    };

}; /* namespace Anvil */

#endif /* MISC_IMAGE_CREATE_INFO_H */
