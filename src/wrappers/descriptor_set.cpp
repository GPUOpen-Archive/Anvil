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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/buffer.h"
#include "wrappers/buffer_view.h"
#include "wrappers/descriptor_pool.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/device.h"
#include "wrappers/image_view.h"
#include "wrappers/sampler.h"

/** Please see header for specification */
Anvil::DescriptorSet::BindingItem& Anvil::DescriptorSet::BindingItem::operator=(const BufferBindingElement& in_element)
{
    buffer_ptr      = in_element.buffer_ptr;
    buffer_view_ptr = nullptr;
    dirty           = true;
    image_layout    = VK_IMAGE_LAYOUT_UNDEFINED;
    image_view_ptr  = nullptr;
    sampler_ptr     = nullptr;
    size            = in_element.size;
    start_offset    = in_element.start_offset;
    type_vk         = in_element.get_type();

    return *this;
}

/** Please see header for specification */
Anvil::DescriptorSet::BindingItem& Anvil::DescriptorSet::BindingItem::operator=(const CombinedImageSamplerBindingElement& in_element)
{
    buffer_ptr      = nullptr;
    buffer_view_ptr = nullptr;
    dirty           = true;
    image_layout    = in_element.image_layout;
    image_view_ptr  = in_element.image_view_ptr;
    sampler_ptr     = in_element.sampler_ptr;
    size            = UINT64_MAX;
    start_offset    = UINT64_MAX;
    type_vk         = in_element.get_type();

    return *this;
}

/** Please see header for specification */
Anvil::DescriptorSet::BindingItem& Anvil::DescriptorSet::BindingItem::operator=(const ImageBindingElement& in_element)
{
    buffer_ptr      = nullptr;
    buffer_view_ptr = nullptr;
    dirty           = true;
    image_layout    = in_element.image_layout;
    image_view_ptr  = in_element.image_view_ptr;
    sampler_ptr     = nullptr;
    size            = UINT64_MAX;
    start_offset    = UINT64_MAX;
    type_vk         = in_element.get_type();

    return *this;
}

/** Please see header for specification */
Anvil::DescriptorSet::BindingItem& Anvil::DescriptorSet::BindingItem::operator=(const SamplerBindingElement& in_element)
{
    buffer_ptr      = nullptr;
    buffer_view_ptr = nullptr;
    dirty           = true;
    image_layout    = VK_IMAGE_LAYOUT_UNDEFINED;
    image_view_ptr  = nullptr;
    sampler_ptr     = in_element.sampler_ptr;
    size            = UINT64_MAX;
    start_offset    = UINT64_MAX;
    type_vk         = in_element.get_type();

    return *this;
}

/** Please see header for specification */
Anvil::DescriptorSet::BindingItem& Anvil::DescriptorSet::BindingItem::operator=(const TexelBufferBindingElement& in_element)
{
    buffer_ptr      = nullptr;
    buffer_view_ptr = in_element.buffer_view_ptr;
    dirty           = true;
    image_layout    = VK_IMAGE_LAYOUT_UNDEFINED;
    image_view_ptr  = nullptr;
    sampler_ptr     = nullptr;
    size            = UINT64_MAX;
    start_offset    = UINT64_MAX;
    type_vk         = in_element.get_type();

    return *this;
}

/** Please see header for specification */
Anvil::DescriptorSet::BindingItem::~BindingItem()
{
    /* Stub */
}

/** Please see header for specification */
Anvil::DescriptorSet::BufferBindingElement::BufferBindingElement(std::shared_ptr<Anvil::Buffer> in_buffer_ptr)
{
    anvil_assert(in_buffer_ptr != nullptr);

    buffer_ptr   = in_buffer_ptr;
    size         = UINT64_MAX;
    start_offset = UINT64_MAX;
}

/** Please see header for specification */
Anvil::DescriptorSet::BufferBindingElement::BufferBindingElement(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                                                 VkDeviceSize                   in_start_offset,
                                                                 VkDeviceSize                   in_size)
{
    anvil_assert(in_buffer_ptr != nullptr);

    if (in_size != VK_WHOLE_SIZE)
    {
        anvil_assert(in_start_offset + in_size <= in_buffer_ptr->get_size() );
    }

    buffer_ptr   = in_buffer_ptr;
    size         = in_size;
    start_offset = in_start_offset;
}

/** Please see header for specification */
Anvil::DescriptorSet::BufferBindingElement::~BufferBindingElement()
{
    /* Stub */
}

/** Please see header for specification */
Anvil::DescriptorSet::BufferBindingElement::BufferBindingElement(const BufferBindingElement& in)
{
    buffer_ptr = in.buffer_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::CombinedImageSamplerBindingElement::CombinedImageSamplerBindingElement(VkImageLayout                     in_image_layout,
                                                                                             std::shared_ptr<Anvil::ImageView> in_image_view_ptr,
                                                                                             std::shared_ptr<Anvil::Sampler>   in_sampler_ptr)
{
    anvil_assert(in_image_view_ptr != nullptr);

    image_layout   = in_image_layout;
    image_view_ptr = in_image_view_ptr;
    sampler_ptr    = in_sampler_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::CombinedImageSamplerBindingElement::~CombinedImageSamplerBindingElement()
{
    /* Stub */
}

/** Please see header for specification */
Anvil::DescriptorSet::CombinedImageSamplerBindingElement::CombinedImageSamplerBindingElement(const CombinedImageSamplerBindingElement& in)
{
    image_layout   = in.image_layout;
    image_view_ptr = in.image_view_ptr;
    sampler_ptr    = in.sampler_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::ImageBindingElement::ImageBindingElement(VkImageLayout                     in_image_layout,
                                                               std::shared_ptr<Anvil::ImageView> in_image_view_ptr)
{
    anvil_assert(in_image_view_ptr != nullptr);

    image_layout   = in_image_layout;
    image_view_ptr = in_image_view_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::ImageBindingElement::ImageBindingElement(const ImageBindingElement& in)
{
    image_layout   = in.image_layout;
    image_view_ptr = in.image_view_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::ImageBindingElement::~ImageBindingElement()
{
    /* Stub */
}

/** Please see header for specification */
Anvil::DescriptorSet::SamplerBindingElement::SamplerBindingElement(std::shared_ptr<Anvil::Sampler> in_sampler_ptr)
{
    anvil_assert(in_sampler_ptr != nullptr);

    sampler_ptr = in_sampler_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::SamplerBindingElement::SamplerBindingElement(const SamplerBindingElement& in)
{
    sampler_ptr = in.sampler_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::SamplerBindingElement::~SamplerBindingElement()
{
    /* Stub */
}

/** Please see header for specification */
Anvil::DescriptorSet::TexelBufferBindingElement::TexelBufferBindingElement(std::shared_ptr<Anvil::BufferView> in_buffer_view_ptr)
{
    anvil_assert(in_buffer_view_ptr != nullptr);

    buffer_view_ptr = in_buffer_view_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::TexelBufferBindingElement::~TexelBufferBindingElement()
{
    /* Stub */
}

/** Please see header for specification */
Anvil::DescriptorSet::TexelBufferBindingElement::TexelBufferBindingElement(const TexelBufferBindingElement& in)
{
    buffer_view_ptr = in.buffer_view_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::DescriptorSet(std::weak_ptr<Anvil::BaseDevice>            in_device_ptr,
                                    std::shared_ptr<Anvil::DescriptorPool>      in_parent_pool_ptr,
                                    std::shared_ptr<Anvil::DescriptorSetLayout> in_layout_ptr,
                                    VkDescriptorSet                             in_descriptor_set)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT),
     m_descriptor_set          (in_descriptor_set),
     m_device_ptr              (in_device_ptr),
     m_dirty                   (true),
     m_layout_ptr              (in_layout_ptr),
     m_parent_pool_ptr         (in_parent_pool_ptr),
     m_unusable                (false)
{
    alloc_bindings();

    in_layout_ptr->register_for_callbacks    (Anvil::DESCRIPTOR_SET_LAYOUT_CALLBACK_ID_BINDING_ADDED,
                                              on_binding_added_to_layout,
                                              this);
    m_parent_pool_ptr->register_for_callbacks(Anvil::DESCRIPTOR_POOL_CALLBACK_ID_POOL_RESET,
                                              on_parent_pool_reset,
                                              this);

    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_DESCRIPTOR_SET,
                                                 this);
}

/** Please see header for specification */
Anvil::DescriptorSet::~DescriptorSet()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_DESCRIPTOR_SET,
                                                   this);
}

/** Takes the descriptor set layout and:
 *
 *  1) resizes m_caches_ds_write_items_vk to hold as many bindings as the layout describes
 *  2) resizes each m_bindings item, described by the layout, to hold as many descriptor
 *     items, as described by the layout object.
 **/
void Anvil::DescriptorSet::alloc_bindings()
{
    const uint32_t n_bindings = m_layout_ptr->get_n_bindings();

    m_cached_ds_write_items_vk.resize(n_bindings);

    for (uint32_t n_binding = 0;
                  n_binding < n_bindings;
                ++n_binding)
    {
        uint32_t array_size = 0;
        uint32_t binding_index;

        m_layout_ptr->get_binding_properties(n_binding,
                                            &binding_index,
                                             nullptr, /* out_opt_descriptor_type_ptr */
                                            &array_size,
                                             nullptr,  /* out_opt_stage_flags_ptr                */
                                             nullptr); /* out_opt_immutable_samplers_enabled_ptr */

        if (m_bindings[binding_index].size() != array_size)
        {
            m_bindings[binding_index] = BindingItems();

            m_bindings[binding_index].resize(array_size);
        }
    }
}

/* Please see header for specification */
bool Anvil::DescriptorSet::bake()
{
    bool result = false;

    anvil_assert(!m_unusable);

    if (m_dirty)
    {
        uint32_t       cached_ds_buffer_info_items_array_offset       = 0;
        uint32_t       cached_ds_image_info_items_array_offset        = 0;
        uint32_t       cached_ds_texel_buffer_info_items_array_offset = 0;
        const uint32_t n_bindings                                     = static_cast<uint32_t>(m_bindings.size() );
        uint32_t       n_max_ds_info_items_to_cache                   = 0;

        m_cached_ds_info_buffer_info_items_vk.clear();
        m_cached_ds_info_image_info_items_vk.clear();
        m_cached_ds_info_texel_buffer_info_items_vk.clear();
        m_cached_ds_write_items_vk.clear();

        for (auto& binding : m_bindings)
        {
            const uint32_t n_current_binding_items = static_cast<uint32_t>(binding.second.size() );

            n_max_ds_info_items_to_cache += n_current_binding_items;
        }

        m_cached_ds_info_buffer_info_items_vk.reserve      (n_max_ds_info_items_to_cache);
        m_cached_ds_info_image_info_items_vk.reserve       (n_max_ds_info_items_to_cache);
        m_cached_ds_info_texel_buffer_info_items_vk.reserve(n_max_ds_info_items_to_cache);

        for (uint32_t n_binding = 0;
                      n_binding < n_bindings;
                    ++n_binding)
        {
            uint32_t              current_binding_index;
            VkDescriptorType      descriptor_type;
            bool                  immutable_samplers_enabled                    = false;
            const uint32_t        start_ds_buffer_info_items_array_offset       = cached_ds_buffer_info_items_array_offset;
            const uint32_t        start_ds_image_info_items_array_offset        = cached_ds_image_info_items_array_offset;
            const uint32_t        start_ds_texel_buffer_info_items_array_offset = cached_ds_texel_buffer_info_items_array_offset;
            VkWriteDescriptorSet  write_ds_vk;

            if (!m_layout_ptr->get_binding_properties(n_binding,
                                                     &current_binding_index,
                                                     &descriptor_type,
                                                      nullptr, /* out_opt_descriptor_array_size_ptr */
                                                      nullptr, /* out_opt_stage_flags_ptr           */
                                                     &immutable_samplers_enabled) )
            {
                anvil_assert_fail();
            }

            /* For each array item, initialize a descriptor info item.. */
            BindingItems&  current_binding_items   = m_bindings[current_binding_index];
            const uint32_t n_current_binding_items = static_cast<uint32_t>(current_binding_items.size() );

            for (uint32_t n_current_binding_item = 0;
                          n_current_binding_item < n_current_binding_items;
                        ++n_current_binding_item)
            {
                BindingItem& current_binding_item = current_binding_items[n_current_binding_item];

                if (!current_binding_item.dirty)
                {
                    continue;
                }

                if (current_binding_item.buffer_ptr != nullptr)
                {
                    VkDescriptorBufferInfo buffer_info;

                    buffer_info.buffer = current_binding_item.buffer_ptr->get_buffer();

                    if (current_binding_item.start_offset != UINT64_MAX)
                    {
                        buffer_info.offset = current_binding_item.start_offset;
                        buffer_info.range  = current_binding_item.size;
                    }
                    else
                    {
                        buffer_info.offset = current_binding_item.buffer_ptr->get_start_offset();
                        buffer_info.range  = current_binding_item.buffer_ptr->get_size        ();
                    }

                    m_cached_ds_info_buffer_info_items_vk.push_back(buffer_info);

                    ++cached_ds_buffer_info_items_array_offset;
                }
                else
                if (current_binding_item.buffer_view_ptr != nullptr)
                {
                    m_cached_ds_info_texel_buffer_info_items_vk.push_back( current_binding_item.buffer_view_ptr->get_buffer_view() );

                    ++cached_ds_texel_buffer_info_items_array_offset;
                }
                else
                if (current_binding_item.image_view_ptr != nullptr)
                {
                    VkDescriptorImageInfo image_info;

                    image_info.imageLayout = current_binding_item.image_layout;
                    image_info.imageView   = current_binding_item.image_view_ptr->get_image_view();

                    if (false   == immutable_samplers_enabled       &&
                        nullptr != current_binding_item.sampler_ptr)
                    {
                        image_info.sampler = current_binding_item.sampler_ptr->get_sampler();
                    }
                    else
                    {
                        image_info.sampler = VK_NULL_HANDLE;
                    }

                    m_cached_ds_info_image_info_items_vk.push_back(image_info);

                    ++cached_ds_image_info_items_array_offset;
                }
                else
                {
                    anvil_assert_fail();

                    goto end;
                }
            }

            /* We can finally fill the write descriptor */
            if (current_binding_items.size() > 0)
            {
                write_ds_vk.descriptorCount  = (uint32_t) current_binding_items.size();
                write_ds_vk.descriptorType   = descriptor_type;
                write_ds_vk.dstArrayElement  = 0;
                write_ds_vk.dstBinding       = current_binding_index;
                write_ds_vk.dstSet           = m_descriptor_set;
                write_ds_vk.pBufferInfo      = (start_ds_buffer_info_items_array_offset != cached_ds_buffer_info_items_array_offset)             ? &m_cached_ds_info_buffer_info_items_vk[start_ds_buffer_info_items_array_offset]
                                                                                                                                                 : nullptr;
                write_ds_vk.pImageInfo       = (start_ds_image_info_items_array_offset  != cached_ds_image_info_items_array_offset)              ? &m_cached_ds_info_image_info_items_vk[start_ds_image_info_items_array_offset]
                                                                                                                                                 : nullptr;
                write_ds_vk.pNext            = nullptr;
                write_ds_vk.pTexelBufferView = (start_ds_texel_buffer_info_items_array_offset != cached_ds_texel_buffer_info_items_array_offset) ? &m_cached_ds_info_texel_buffer_info_items_vk[start_ds_texel_buffer_info_items_array_offset]
                                                                                                                                                 : nullptr;
                write_ds_vk.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

                m_cached_ds_write_items_vk.push_back(write_ds_vk);
            }
        }

        /* Issue the Vulkan call */
        if (m_cached_ds_write_items_vk.size() > 0)
        {
            std::shared_ptr<Anvil::BaseDevice> device_locked_ptr(m_device_ptr);

            vkUpdateDescriptorSets(device_locked_ptr->get_device_vk(),
                                   (uint32_t) m_cached_ds_write_items_vk.size(),
                                  &m_cached_ds_write_items_vk[0],
                                   0,        /* copyCount         */
                                   nullptr); /* pDescriptorCopies */
        }

        m_dirty = false;
    }

    result = true;

end:
    return result;
}

/* Please see header for specification */
std::shared_ptr<Anvil::DescriptorSet> Anvil::DescriptorSet::create(std::weak_ptr<Anvil::BaseDevice>            in_device_ptr,
                                                                   std::shared_ptr<Anvil::DescriptorPool>      in_parent_pool_ptr,
                                                                   std::shared_ptr<Anvil::DescriptorSetLayout> in_layout_ptr,
                                                                   VkDescriptorSet                             in_descriptor_set)
{
    std::shared_ptr<Anvil::DescriptorSet> result_ptr;

    result_ptr.reset(
        new Anvil::DescriptorSet(in_device_ptr,
                                 in_parent_pool_ptr,
                                 in_layout_ptr,
                                 in_descriptor_set)
    );

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_binding_array_size(uint32_t  in_n_binding,
                                                  uint32_t* out_result_ptr) const
{
    bool result = true;

    if (m_bindings.find(in_n_binding) == m_bindings.end() )
    {
        result = false;
        goto end;
    }

    *out_result_ptr = static_cast<uint32_t>(m_bindings.at(in_n_binding).size() );

end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_binding_descriptor_type(uint32_t          in_n_binding,
                                                       VkDescriptorType* out_descriptor_type_ptr) const
{
    bool result = true;

    if (m_bindings.find(in_n_binding) == m_bindings.end() )
    {
        anvil_assert(m_bindings.find(in_n_binding) != m_bindings.end() );

        result = false;
        goto end;
    }

    *out_descriptor_type_ptr = m_bindings.at(in_n_binding).at(0).type_vk;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_combined_image_sampler_binding_properties(uint32_t                           in_n_binding,
                                                                         uint32_t                           in_n_binding_array_item,
                                                                         VkImageLayout*                     out_opt_image_layout_ptr,
                                                                         std::shared_ptr<Anvil::ImageView>* out_opt_image_view_ptr,
                                                                         std::shared_ptr<Anvil::Sampler>*   out_opt_sampler_ptr)
{
    decltype(m_bindings)::const_iterator binding_iterator = m_bindings.find(in_n_binding);
    bool                                 result           = true;

    if (binding_iterator == m_bindings.end() )
    {
        anvil_assert(binding_iterator != m_bindings.end());

        result = false;
        goto end;
    }

    if (binding_iterator->second.size() <= in_n_binding_array_item)
    {
        anvil_assert(binding_iterator->second.size() > in_n_binding_array_item);

        result = false;
        goto end;
    }
    else
    {
        auto binding_item_iterator = binding_iterator->second.begin() + in_n_binding_array_item;

        if (out_opt_image_layout_ptr != nullptr)
        {
            *out_opt_image_layout_ptr = binding_item_iterator->image_layout;
        }

        if (out_opt_image_view_ptr != nullptr)
        {
            *out_opt_image_view_ptr = binding_item_iterator->image_view_ptr;
        }

        if (out_opt_sampler_ptr != nullptr)
        {
            *out_opt_sampler_ptr = binding_item_iterator->sampler_ptr;
        }
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_input_attachment_binding_properties(uint32_t                           in_n_binding,
                                                                   uint32_t                           in_n_binding_array_item,
                                                                   VkImageLayout*                     out_opt_image_layout_ptr,
                                                                   std::shared_ptr<Anvil::ImageView>* out_opt_image_view_ptr) const
{
    decltype(m_bindings)::const_iterator binding_iterator = m_bindings.find(in_n_binding);
    bool                                 result           = true;

    if (binding_iterator == m_bindings.end() )
    {
        anvil_assert(binding_iterator != m_bindings.end());

        result = false;
        goto end;
    }

    if (binding_iterator->second.size() <= in_n_binding_array_item)
    {
        anvil_assert(binding_iterator->second.size() > in_n_binding_array_item);

        result = false;
        goto end;
    }
    else
    {
        auto binding_item_iterator = binding_iterator->second.begin() + in_n_binding_array_item;

        if (out_opt_image_layout_ptr != nullptr)
        {
            *out_opt_image_layout_ptr = binding_item_iterator->image_layout;
        }

        if (out_opt_image_view_ptr != nullptr)
        {
            *out_opt_image_view_ptr = binding_item_iterator->image_view_ptr;
        }
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_sampler_binding_properties(uint32_t                         in_n_binding,
                                                          uint32_t                         in_n_binding_array_item,
                                                          std::shared_ptr<Anvil::Sampler>* out_sampler_ptr) const
{
    decltype(m_bindings)::const_iterator binding_iterator = m_bindings.find(in_n_binding);
    bool                                 result           = true;

    if (binding_iterator == m_bindings.end() )
    {
        anvil_assert(binding_iterator != m_bindings.end());

        result = false;
        goto end;
    }

    if (binding_iterator->second.size() <= in_n_binding_array_item)
    {
        anvil_assert(binding_iterator->second.size() > in_n_binding_array_item);

        result = false;
        goto end;
    }
    else
    {
        auto binding_item_iterator = binding_iterator->second.begin() + in_n_binding_array_item;

        if (out_sampler_ptr != nullptr)
        {
            *out_sampler_ptr = binding_item_iterator->sampler_ptr;
        }
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_storage_buffer_binding_properties(uint32_t                        in_n_binding,
                                                                 uint32_t                        in_n_binding_array_item,
                                                                 std::shared_ptr<Anvil::Buffer>* out_opt_buffer_ptr,
                                                                 VkDeviceSize*                   out_opt_size_ptr,
                                                                 VkDeviceSize*                   out_opt_start_offset_ptr) const
{
    decltype(m_bindings)::const_iterator binding_iterator = m_bindings.find(in_n_binding);
    bool                                 result           = true;

    if (binding_iterator == m_bindings.end() )
    {
        anvil_assert(binding_iterator != m_bindings.end());

        result = false;
        goto end;
    }

    if (binding_iterator->second.size() <= in_n_binding_array_item)
    {
        anvil_assert(binding_iterator->second.size() > in_n_binding_array_item);

        result = false;
        goto end;
    }
    else
    {
        auto binding_item_iterator = binding_iterator->second.begin() + in_n_binding_array_item;

        if (out_opt_buffer_ptr != nullptr)
        {
            *out_opt_buffer_ptr = binding_item_iterator->buffer_ptr;
        }

        if (out_opt_size_ptr != nullptr)
        {
            *out_opt_size_ptr = binding_item_iterator->size;
        }

        if (out_opt_start_offset_ptr != nullptr)
        {
            *out_opt_start_offset_ptr = binding_item_iterator->start_offset;
        }
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_storage_texel_buffer_binding_properties(uint32_t                            in_n_binding,
                                                                       uint32_t                            in_n_binding_array_item,
                                                                       std::shared_ptr<Anvil::BufferView>* out_opt_buffer_view_ptr) const
{
    decltype(m_bindings)::const_iterator binding_iterator = m_bindings.find(in_n_binding);
    bool                                 result           = true;

    if (binding_iterator == m_bindings.end() )
    {
        anvil_assert(binding_iterator != m_bindings.end());

        result = false;
        goto end;
    }

    if (binding_iterator->second.size() <= in_n_binding_array_item)
    {
        anvil_assert(binding_iterator->second.size() > in_n_binding_array_item);

        result = false;
        goto end;
    }
    else
    {
        auto binding_item_iterator = binding_iterator->second.begin() + in_n_binding_array_item;

        if (out_opt_buffer_view_ptr != nullptr)
        {
            *out_opt_buffer_view_ptr = binding_item_iterator->buffer_view_ptr;
        }
    }

end:
    return result;
}

/** Entry-point called back whenever a new binding is added to the parent layout.
 *
 *  This will resize a number of internally managed vectors.
 *
 *  @param layout_raw_ptr Ignored.
 *  @param ds_raw_ptr     Pointer to a DescriptorSet instance which uses the modified layout as the parent.
 *                        Never nullptr.
 *
 **/
void Anvil::DescriptorSet::on_binding_added_to_layout(void* in_layout_raw_ptr,
                                                      void* in_ds_raw_ptr)
{
    Anvil::DescriptorSet* ds_ptr = static_cast<Anvil::DescriptorSet*>(in_ds_raw_ptr);

    ANVIL_REDUNDANT_ARGUMENT(in_layout_raw_ptr);

    ds_ptr->alloc_bindings();
}

/** Called back whenever parent descriptor pool is reset.
 *
 *  Resets m_descriptor_set back to VK_NULL_HANDLE and marks the descriptor set as unusable.
 *
 *  @param pool_raw_ptr Ignored.
 *  @param ds_raw_ptr   Affected DescriptorSet instance. Never nullptr.
 *
 **/
void Anvil::DescriptorSet::on_parent_pool_reset(void* in_pool_raw_ptr,
                                                void* in_ds_raw_ptr)
{
    Anvil::DescriptorSet* ds_ptr = static_cast<Anvil::DescriptorSet*>(in_ds_raw_ptr);

    ANVIL_REDUNDANT_ARGUMENT(in_ds_raw_ptr);
    ANVIL_REDUNDANT_ARGUMENT(in_pool_raw_ptr);

    /* This descriptor set instance is no longer usable.
     *
     * To restore functionality, a new Vulkan DS handle should be assigned to this instance
     * by calling set_new_vk_handle() */
    ds_ptr->m_descriptor_set = VK_NULL_HANDLE;
    ds_ptr->m_unusable       = true;
}

/** Please see header for specification */
void Anvil::DescriptorSet::set_new_vk_handle(VkDescriptorSet in_ds)
{
    anvil_assert(m_unusable);
    anvil_assert(in_ds != VK_NULL_HANDLE);

    m_descriptor_set = in_ds;
    m_dirty          = true;
    m_unusable       = false;
}