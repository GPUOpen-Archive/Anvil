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

#include "misc/buffer_create_info.h"
#include "misc/debug.h"
#include "misc/descriptor_set_create_info.h"
#include "misc/object_tracker.h"
#include "wrappers/buffer.h"
#include "wrappers/buffer_view.h"
#include "wrappers/descriptor_pool.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/descriptor_update_template.h"
#include "wrappers/device.h"
#include "wrappers/image_view.h"
#include "wrappers/sampler.h"

#ifdef max
    #undef max
#endif

/** Please see header for specification */
Anvil::DescriptorSet::BindingItem& Anvil::DescriptorSet::BindingItem::operator=(const BufferBindingElement& in_element)
{
    buffer_ptr      = in_element.buffer_ptr;
    buffer_view_ptr = nullptr;
    dirty           = true;
    image_layout    = Anvil::ImageLayout::UNDEFINED;
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
    image_layout    = Anvil::ImageLayout::UNDEFINED;
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
    image_layout    = Anvil::ImageLayout::UNDEFINED;
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
    buffer_ptr = nullptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::BufferBindingElement::BufferBindingElement(Anvil::Buffer* in_buffer_ptr)
{
    anvil_assert(in_buffer_ptr != nullptr);

    buffer_ptr   = in_buffer_ptr;
    size         = UINT64_MAX;
    start_offset = UINT64_MAX;
}

/** Please see header for specification */
Anvil::DescriptorSet::BufferBindingElement::BufferBindingElement(Anvil::Buffer* in_buffer_ptr,
                                                                 VkDeviceSize   in_start_offset,
                                                                 VkDeviceSize   in_size)
{
    anvil_assert(in_buffer_ptr != nullptr);

    if (in_size != VK_WHOLE_SIZE)
    {
        anvil_assert(in_start_offset + in_size <= in_buffer_ptr->get_create_info_ptr()->get_size() );
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
    buffer_ptr   = in.buffer_ptr;
    size         = in.size;
    start_offset = in.start_offset;
}

/** Please see header for specification */
Anvil::DescriptorSet::CombinedImageSamplerBindingElement::CombinedImageSamplerBindingElement(Anvil::ImageLayout in_image_layout,
                                                                                             Anvil::ImageView*  in_image_view_ptr,
                                                                                             Anvil::Sampler*    in_sampler_ptr)
{
    anvil_assert(in_image_view_ptr != nullptr);

    image_layout   = in_image_layout;
    image_view_ptr = in_image_view_ptr;
    sampler_ptr    = in_sampler_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::CombinedImageSamplerBindingElement::~CombinedImageSamplerBindingElement()
{
    image_view_ptr = nullptr;
    sampler_ptr    = nullptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::CombinedImageSamplerBindingElement::CombinedImageSamplerBindingElement(const CombinedImageSamplerBindingElement& in)
{
    image_layout   = in.image_layout;
    image_view_ptr = in.image_view_ptr;
    sampler_ptr    = in.sampler_ptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::ImageBindingElement::ImageBindingElement(Anvil::ImageLayout in_image_layout,
                                                               Anvil::ImageView*  in_image_view_ptr)
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
Anvil::DescriptorSet::SamplerBindingElement::SamplerBindingElement(Anvil::Sampler* in_sampler_ptr)
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
    sampler_ptr = nullptr;
}

/** Please see header for specification */
Anvil::DescriptorSet::TexelBufferBindingElement::TexelBufferBindingElement(Anvil::BufferView* in_buffer_view_ptr)
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
Anvil::DescriptorSet::DescriptorSet(const Anvil::BaseDevice*          in_device_ptr,
                                    Anvil::DescriptorPool*            in_parent_pool_ptr,
                                    const Anvil::DescriptorSetLayout* in_layout_ptr,
                                    VkDescriptorSet                   in_descriptor_set,
                                    bool                              in_mt_safe)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT),
     MTSafetySupportProvider   (in_mt_safe),
     m_descriptor_set          (in_descriptor_set),
     m_device_ptr              (in_device_ptr),
     m_dirty                   (true),
     m_layout_ptr              (in_layout_ptr),
     m_parent_pool_ptr         (in_parent_pool_ptr),
     m_unusable                (false)
{
    alloc_bindings();

    m_parent_pool_ptr->register_for_callbacks(
        Anvil::DESCRIPTOR_POOL_CALLBACK_ID_POOL_RESET,
        std::bind(&DescriptorSet::on_parent_pool_reset,
                  this),
        this
    );

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
    const auto     layout_info_ptr                         = m_layout_ptr->get_create_info();
    bool           has_variable_descriptor_count_binding   = false;
    const uint32_t n_bindings                              = layout_info_ptr->get_n_bindings();
    uint32_t       variable_descriptor_count_binding_index = UINT32_MAX;
    uint32_t       variable_descriptor_count_binding_size  = 0;

    m_cached_ds_write_items_vk.resize(n_bindings);

    has_variable_descriptor_count_binding = layout_info_ptr->contains_variable_descriptor_count_binding(&variable_descriptor_count_binding_index,
                                                                                                        &variable_descriptor_count_binding_size);

    for (uint32_t n_binding = 0;
                  n_binding < n_bindings;
                ++n_binding)
    {
        uint32_t array_size    = 0;
        uint32_t binding_index = UINT32_MAX;

        layout_info_ptr->get_binding_properties_by_index_number(n_binding,
                                                               &binding_index,
                                                                nullptr, /* out_opt_descriptor_type_ptr */
                                                               &array_size,
                                                                nullptr,  /* out_opt_stage_flags_ptr                */
                                                                nullptr,  /* out_opt_immutable_samplers_enabled_ptr */
                                                                nullptr); /* out_opt_flags_ptr                      */

        if (has_variable_descriptor_count_binding                                            &&
            binding_index                         == variable_descriptor_count_binding_index)
        {
            array_size = variable_descriptor_count_binding_size;
        }

        if (m_bindings[binding_index].size() != array_size)
        {
            m_bindings[binding_index] = BindingItems();

            m_bindings[binding_index].resize(array_size);
        }
    }
}

/* Please see header for specification */
Anvil::DescriptorSetUniquePtr Anvil::DescriptorSet::create(const Anvil::BaseDevice*          in_device_ptr,
                                                           Anvil::DescriptorPool*            in_parent_pool_ptr,
                                                           const Anvil::DescriptorSetLayout* in_layout_ptr,
                                                           VkDescriptorSet                   in_descriptor_set,
                                                           MTSafety                          in_mt_safety)
{
    const bool                    is_mt_safe = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                               in_device_ptr);
    Anvil::DescriptorSetUniquePtr result_ptr(nullptr,
                                             std::default_delete<Anvil::DescriptorSet>() );

    result_ptr.reset(
        new Anvil::DescriptorSet(in_device_ptr,
                                 in_parent_pool_ptr,
                                 in_layout_ptr,
                                 in_descriptor_set,
                                 is_mt_safe)
    );

    return result_ptr;
}

void Anvil::DescriptorSet::fill_buffer_info_vk_descriptor(const Anvil::DescriptorSet::BindingItem& in_binding_item,
                                                          VkDescriptorBufferInfo*                  out_descriptor_ptr) const
{
    out_descriptor_ptr->buffer = in_binding_item.buffer_ptr->get_buffer();

    if (in_binding_item.start_offset != UINT64_MAX)
    {
        out_descriptor_ptr->offset = in_binding_item.start_offset;
        out_descriptor_ptr->range  = in_binding_item.size;
    }
    else
    {
        out_descriptor_ptr->offset = in_binding_item.buffer_ptr->get_create_info_ptr()->get_start_offset();
        out_descriptor_ptr->range  = in_binding_item.buffer_ptr->get_create_info_ptr()->get_size        ();
    }
}

void Anvil::DescriptorSet::fill_image_info_vk_descriptor(const Anvil::DescriptorSet::BindingItem& in_binding_item,
                                                         const bool&                              in_immutable_samplers_enabled,
                                                         VkDescriptorImageInfo*                   out_descriptor_ptr) const
{
    out_descriptor_ptr->imageLayout = static_cast<VkImageLayout>(in_binding_item.image_layout);
    out_descriptor_ptr->imageView   = (in_binding_item.image_view_ptr != nullptr) ? in_binding_item.image_view_ptr->get_image_view() : VK_NULL_HANDLE;

    if (false   == in_immutable_samplers_enabled       &&
        nullptr != in_binding_item.sampler_ptr)
    {
        out_descriptor_ptr->sampler = in_binding_item.sampler_ptr->get_sampler();
    }
    else
    {
        out_descriptor_ptr->sampler = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_combined_image_sampler_binding_properties(uint32_t            in_n_binding,
                                                                         uint32_t            in_n_binding_array_item,
                                                                         Anvil::ImageLayout* out_opt_image_layout_ptr,
                                                                         Anvil::ImageView**  out_opt_image_view_ptr_ptr,
                                                                         Anvil::Sampler**    out_opt_sampler_ptr_ptr)
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

        if (out_opt_image_view_ptr_ptr != nullptr)
        {
            *out_opt_image_view_ptr_ptr= binding_item_iterator->image_view_ptr;
        }

        if (out_opt_sampler_ptr_ptr != nullptr)
        {
            *out_opt_sampler_ptr_ptr = binding_item_iterator->sampler_ptr;
        }
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_input_attachment_binding_properties(uint32_t            in_n_binding,
                                                                   uint32_t            in_n_binding_array_item,
                                                                   Anvil::ImageLayout* out_opt_image_layout_ptr,
                                                                   Anvil::ImageView**  out_opt_image_view_ptr_ptr) const
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

        if (out_opt_image_view_ptr_ptr != nullptr)
        {
            *out_opt_image_view_ptr_ptr = binding_item_iterator->image_view_ptr;
        }
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_sampler_binding_properties(uint32_t         in_n_binding,
                                                          uint32_t         in_n_binding_array_item,
                                                          Anvil::Sampler** out_sampler_ptr_ptr) const
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

        if (out_sampler_ptr_ptr != nullptr)
        {
            *out_sampler_ptr_ptr = binding_item_iterator->sampler_ptr;
        }
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::get_storage_buffer_binding_properties(uint32_t        in_n_binding,
                                                                 uint32_t        in_n_binding_array_item,
                                                                 Anvil::Buffer** out_opt_buffer_ptr_ptr,
                                                                 VkDeviceSize*   out_opt_size_ptr,
                                                                 VkDeviceSize*   out_opt_start_offset_ptr) const
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

        if (out_opt_buffer_ptr_ptr != nullptr)
        {
            *out_opt_buffer_ptr_ptr = binding_item_iterator->buffer_ptr;
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
bool Anvil::DescriptorSet::get_storage_texel_buffer_binding_properties(uint32_t            in_n_binding,
                                                                       uint32_t            in_n_binding_array_item,
                                                                       Anvil::BufferView** out_opt_buffer_view_ptr_ptr) const
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

        if (out_opt_buffer_view_ptr_ptr != nullptr)
        {
            *out_opt_buffer_view_ptr_ptr = binding_item_iterator->buffer_view_ptr;
        }
    }

end:
    return result;
}

/** Called back whenever parent descriptor pool is reset.
 *
 *  Resets m_descriptor_set back to VK_NULL_HANDLE and marks the descriptor set as unusable.
 *
 **/
void Anvil::DescriptorSet::on_parent_pool_reset()
{
    /* This descriptor set instance is no longer usable. */
    m_descriptor_set = VK_NULL_HANDLE;
    m_unusable       = true;
}

bool Anvil::DescriptorSet::update(const DescriptorSetUpdateMethod& in_update_method) const
{
    bool result;

    lock();
    {
        switch (in_update_method)
        {
            case Anvil::DescriptorSetUpdateMethod::CORE:
            {
                result = update_using_core_method();

                break;
            }

            case Anvil::DescriptorSetUpdateMethod::TEMPLATE:
            {
                result = update_using_template_method();

                break;
            }

            default:
            {
                anvil_assert_fail();

                result = false;
            }
        }
    }
    unlock();

    return result;
}

/* Please see header for specification */
bool Anvil::DescriptorSet::update_using_core_method() const
{
    const auto layout_info_ptr = m_layout_ptr->get_create_info();
    bool       result          = false;

    anvil_assert(!m_unusable);

    if (m_dirty)
    {
        uint32_t       cached_ds_buffer_info_items_array_offset       = 0;
        uint32_t       cached_ds_image_info_items_array_offset        = 0;
        uint32_t       cached_ds_texel_buffer_info_items_array_offset = 0;
        const uint32_t n_bindings                                     = static_cast<uint32_t>(m_bindings.size() );

        m_cached_ds_info_buffer_info_items_vk.clear      ();
        m_cached_ds_info_image_info_items_vk.clear       ();
        m_cached_ds_info_texel_buffer_info_items_vk.clear();
        m_cached_ds_write_items_vk.clear                 ();

        {
            uint32_t n_max_ds_info_items_to_cache = 0;

            for (auto& binding : m_bindings)
            {
                const uint32_t n_current_binding_items = static_cast<uint32_t>(binding.second.size() );

                n_max_ds_info_items_to_cache += n_current_binding_items;
            }

            m_cached_ds_info_buffer_info_items_vk.reserve      (n_max_ds_info_items_to_cache);
            m_cached_ds_info_image_info_items_vk.reserve       (n_max_ds_info_items_to_cache);
            m_cached_ds_info_texel_buffer_info_items_vk.reserve(n_max_ds_info_items_to_cache);
        }

        for (uint32_t n_binding = 0;
                      n_binding < n_bindings;
                    ++n_binding)
        {
            Anvil::DescriptorBindingFlags current_binding_flags;
            uint32_t                      current_binding_index;
            Anvil::DescriptorType         descriptor_type;
            bool                          immutable_samplers_enabled                    = false;
            uint32_t                      start_ds_buffer_info_items_array_offset       = cached_ds_buffer_info_items_array_offset;
            uint32_t                      start_ds_image_info_items_array_offset        = cached_ds_image_info_items_array_offset;
            uint32_t                      start_ds_texel_buffer_info_items_array_offset = cached_ds_texel_buffer_info_items_array_offset;
            VkWriteDescriptorSet          write_ds_vk;

            if (!layout_info_ptr->get_binding_properties_by_index_number(n_binding,
                                                                        &current_binding_index,
                                                                        &descriptor_type,
                                                                         nullptr, /* out_opt_descriptor_array_size_ptr */
                                                                         nullptr, /* out_opt_stage_flags_ptr           */
                                                                        &immutable_samplers_enabled,
                                                                        &current_binding_flags) )
            {
                anvil_assert_fail();
            }

            /* For each array item, initialize a descriptor info item.. */
            BindingItems&  current_binding_items   = m_bindings.at(current_binding_index);
            const uint32_t n_current_binding_items = static_cast<uint32_t>(current_binding_items.size() );
            int32_t        n_last_binding_item     = -1;

            for (uint32_t n_current_binding_item = 0;
                          n_current_binding_item < n_current_binding_items;
                        ++n_current_binding_item)
            {
                BindingItem& current_binding_item = current_binding_items.at(n_current_binding_item);
                bool         needs_write_item     = ((n_current_binding_item + 1) == n_current_binding_items);

                /* TODO: For arrayed binding items, avoid updating all binding items every time baking is triggered. */
                if (!current_binding_item.dirty     &&
                     n_current_binding_item    == 0 &&
                     n_current_binding_items   == 1)
                {
                    continue;
                }

                if (current_binding_item.buffer_ptr != nullptr)
                {
                    VkDescriptorBufferInfo buffer_info;

                    fill_buffer_info_vk_descriptor(current_binding_item,
                                                  &buffer_info);

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
                if (current_binding_item.image_view_ptr != nullptr ||
                    current_binding_item.sampler_ptr    != nullptr)
                {
                    VkDescriptorImageInfo image_info;

                    fill_image_info_vk_descriptor(current_binding_item,
                                                  immutable_samplers_enabled,
                                                 &image_info);

                    m_cached_ds_info_image_info_items_vk.push_back(image_info);

                    ++cached_ds_image_info_items_array_offset;
                }
                else
                {
                    /* Arrayed bindings are only permitted if the binding has been created with the PARTIALLY_BOUND flag */
                    if ((current_binding_flags & Anvil::DescriptorBindingFlagBits::PARTIALLY_BOUND_BIT) == 0)
                    {
                        anvil_assert_fail();

                        goto end;
                    }

                    /* Need to cache a write item at this point since current binding has not been assigned a descriptor */
                    needs_write_item = true;
                }

                if (needs_write_item)
                {
                    const uint32_t n_descriptors = (cached_ds_buffer_info_items_array_offset       - start_ds_buffer_info_items_array_offset)       +
                                                   (cached_ds_image_info_items_array_offset        - start_ds_image_info_items_array_offset)        +
                                                   (cached_ds_texel_buffer_info_items_array_offset - start_ds_texel_buffer_info_items_array_offset);

                    if (n_descriptors > 0)
                    {
                        write_ds_vk.descriptorCount  = n_descriptors;
                        write_ds_vk.descriptorType   = static_cast<VkDescriptorType>(descriptor_type);
                        write_ds_vk.dstArrayElement  = n_last_binding_item + 1;
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

                        anvil_assert(write_ds_vk.descriptorCount != 0);

                        m_cached_ds_write_items_vk.push_back(write_ds_vk);
                    }

                    n_last_binding_item                           = n_current_binding_item;
                    start_ds_buffer_info_items_array_offset       = cached_ds_buffer_info_items_array_offset;
                    start_ds_image_info_items_array_offset        = cached_ds_image_info_items_array_offset;
                    start_ds_texel_buffer_info_items_array_offset = cached_ds_texel_buffer_info_items_array_offset;
                }

                current_binding_item.dirty = false;
            }
        }

        /* Issue the Vulkan call */
        if (m_cached_ds_write_items_vk.size() > 0)
        {
            Anvil::Vulkan::vkUpdateDescriptorSets(m_device_ptr->get_device_vk(),
                                                  static_cast<uint32_t>(m_cached_ds_write_items_vk.size() ),
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

bool Anvil::DescriptorSet::update_using_template_method() const
{
    std::vector<uint8_t> data_vector;
    const auto           layout_info_ptr = m_layout_ptr->get_create_info();
    bool                 result          = false;

    if (!m_device_ptr->get_extension_info()->khr_descriptor_update_template() )
    {
        anvil_assert(m_device_ptr->get_extension_info()->khr_descriptor_update_template() );

        goto end;
    }

    anvil_assert(!m_unusable);

    if (m_dirty)
    {
        /* First build up a vector of template entries we need the template to encapsulate. While on it,
         * also construct an array of descriptors we're going to pass along the template.
         */
        const uint32_t                                  n_bindings               = static_cast<uint32_t>(m_bindings.size() );
        decltype(m_template_object_map)::const_iterator template_object_iterator;

        m_template_entries.clear ();
        m_template_raw_data.clear();

        for (uint32_t n_binding = 0;
                      n_binding < n_bindings;
                    ++n_binding)
        {
            const std::vector<BindingItem>* binding_element_vec_ptr    = nullptr;
            uint32_t                        current_binding_index      = UINT32_MAX;
            Anvil::DescriptorType           descriptor_type            = Anvil::DescriptorType::UNKNOWN;
            bool                            immutable_samplers_enabled = false;
            uint32_t                        n_binding_elements         = 0;

            if (!layout_info_ptr->get_binding_properties_by_index_number(n_binding,
                                                                        &current_binding_index,
                                                                        &descriptor_type,
                                                                         nullptr,                     /* out_opt_descriptor_array_size_ptr */
                                                                         nullptr,                     /* out_opt_stage_flags_ptr           */
                                                                        &immutable_samplers_enabled,
                                                                         nullptr) )                   /* out_opt_flags_ptr                 */
            {
                anvil_assert_fail();

                result = false;
                goto end;
            }

            binding_element_vec_ptr = &m_bindings.at(current_binding_index);
            n_binding_elements      = static_cast<uint32_t>(binding_element_vec_ptr->size() );

            for (uint32_t n_binding_element = 0;
                          n_binding_element < n_binding_elements;
                        ++n_binding_element)
            {
                const auto&    current_binding_element        = binding_element_vec_ptr->at(n_binding_element);
                const uint32_t current_template_raw_data_size = static_cast<uint32_t>(m_template_raw_data.size() );

                if (!current_binding_element.dirty)
                {
                    continue;
                }

                /* Append the new descriptor to the raw data vector.
                 *
                 * TODO: Consecutive dirty binding elements could be merged into a single item here.
                 */
                if (current_binding_element.buffer_ptr != nullptr)
                {
                    m_template_raw_data.resize(current_template_raw_data_size + sizeof(VkDescriptorBufferInfo) );

                    fill_buffer_info_vk_descriptor(current_binding_element,
                                                   reinterpret_cast<VkDescriptorBufferInfo*>(&m_template_raw_data.at(current_template_raw_data_size) ));
                }
                else
                if (current_binding_element.buffer_view_ptr != nullptr)
                {
                    m_template_raw_data.resize(current_template_raw_data_size + sizeof(VkBufferView) );

                    *reinterpret_cast<VkBufferView*>(&m_template_raw_data.at(current_template_raw_data_size) ) = current_binding_element.buffer_view_ptr->get_buffer_view();
                }
                else
                if (current_binding_element.image_view_ptr != nullptr)
                {
                    m_template_raw_data.resize(current_template_raw_data_size + sizeof(VkDescriptorImageInfo) );

                    fill_image_info_vk_descriptor(current_binding_element,
                                                  immutable_samplers_enabled,
                                                  reinterpret_cast<VkDescriptorImageInfo*>(&m_template_raw_data.at(current_template_raw_data_size) ));
                }
                else
                {
                    anvil_assert_fail();

                    result = false;
                    goto end;
                }

                m_template_entries.push_back(
                    DescriptorUpdateTemplateEntry(descriptor_type,
                                                  n_binding_element,
                                                  current_binding_index,
                                                  1,                              /* in_n_descriptors */
                                                  current_template_raw_data_size,
                                                  0)                              /* in_stride */
                );
            }
        }

        if (m_template_entries.size() == 0)
        {
            goto end;
        }
        else
        {
            anvil_assert(m_template_raw_data.size() > 0);
        }

        /* Has a template object matching our needs already been created in the past? */
        template_object_iterator = m_template_object_map.find(m_template_entries);

        if (template_object_iterator == m_template_object_map.end() )
        {
            /* Need to create a new template object.. */
            m_template_object_map[m_template_entries] = std::move(
                Anvil::DescriptorUpdateTemplate::create_for_descriptor_set_updates(m_device_ptr,
                                                                                   m_layout_ptr,
                                                                                   m_template_entries,
                                                                                   Anvil::MTSafety::DISABLED) );

            /* Update the iterator */
            template_object_iterator = m_template_object_map.find(m_template_entries);
            anvil_assert(template_object_iterator != m_template_object_map.end() );

            if (template_object_iterator->second == nullptr)
            {
                anvil_assert(template_object_iterator->second != nullptr);

                result = false;
                goto end;
            }
        }

        /* Issue the Vulkan call.
         *
         * NOTE: The order MUST be reversed, since update_descriptor_set() calls DescriptorSet::get_descriptor_set_vk() which would invoke update()
         *       had m_dirty been set to true.
         */
        m_dirty = false;

        template_object_iterator->second->update_descriptor_set(this,
                                                               &m_template_raw_data.at(0) );
    }

    result = true;

end:

    return result;
}