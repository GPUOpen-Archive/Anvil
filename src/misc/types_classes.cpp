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
#include "misc/buffer_create_info.h"
#include "misc/image_create_info.h"
#include "misc/types.h"
#include "wrappers/buffer.h"
#include "wrappers/image.h"
#include "wrappers/memory_block.h"
#include "wrappers/semaphore.h"

/** Please see header for specification */
Anvil::SparseMemoryBindInfoID Anvil::SparseMemoryBindingUpdateInfo::add_bind_info(uint32_t                 in_n_signal_semaphores,
                                                                                  Anvil::Semaphore* const* in_opt_signal_semaphores_ptr,
                                                                                  uint32_t                 in_n_wait_semaphores,
                                                                                  Anvil::Semaphore* const* in_opt_wait_semaphores_ptr)
{
    Anvil::SparseMemoryBindInfoID result_id = static_cast<Anvil::SparseMemoryBindInfoID>(m_bindings.size() );
    BindingInfo                   new_binding;

    for (uint32_t n_signal_sem = 0;
                  n_signal_sem < in_n_signal_semaphores;
                ++n_signal_sem)
    {
        new_binding.signal_semaphores.push_back(in_opt_signal_semaphores_ptr[n_signal_sem]);
    }

    for (uint32_t n_wait_sem = 0;
                  n_wait_sem < in_n_wait_semaphores;
                ++n_wait_sem)
    {
        new_binding.wait_semaphores.push_back(in_opt_wait_semaphores_ptr[n_wait_sem]);
    }

    m_bindings.push_back(new_binding);

    return result_id;
}

/** Please see header for specification */
void Anvil::SparseMemoryBindingUpdateInfo::append_buffer_memory_update(SparseMemoryBindInfoID in_bind_info_id,
                                                                       Anvil::Buffer*         in_buffer_ptr,
                                                                       VkDeviceSize           in_buffer_memory_start_offset,
                                                                       Anvil::MemoryBlock*    in_memory_block_ptr,
                                                                       VkDeviceSize           in_memory_block_start_offset,
                                                                       bool                   in_memory_block_owned_by_buffer,
                                                                       VkDeviceSize           in_size)
{
    /* Sanity checks */
    anvil_assert(in_buffer_ptr                                 != nullptr);
    anvil_assert(m_bindings.size()                             >  in_bind_info_id);
    anvil_assert(in_buffer_ptr->get_memory_requirements().size >= in_buffer_memory_start_offset + in_size);

    if (in_memory_block_ptr != nullptr)
    {
        anvil_assert(in_memory_block_ptr->get_create_info_ptr()->get_size() >= in_memory_block_start_offset + in_size);
    }

    /* Cache the update */
    auto&              binding = m_bindings.at(in_bind_info_id);
    GeneralBindInfo    update;
    VkSparseMemoryBind update_vk;

    update.memory_block_owned_by_target = in_memory_block_owned_by_buffer;
    update.memory_block_ptr             = in_memory_block_ptr;
    update.memory_block_start_offset    = in_memory_block_start_offset;
    update.size                         = in_size;
    update.start_offset                 = in_buffer_memory_start_offset;

    update_vk.flags                     = 0;
    update_vk.memory                    = (in_memory_block_ptr != nullptr) ? in_memory_block_ptr->get_memory()
                                                                           : VK_NULL_HANDLE;
    update_vk.memoryOffset              = (in_memory_block_ptr != nullptr) ? (in_memory_block_ptr->get_start_offset() + in_memory_block_start_offset)
                                                                           : UINT32_MAX;
    update_vk.resourceOffset            = (in_buffer_ptr->get_create_info_ptr()->get_start_offset() + in_buffer_memory_start_offset);
    update_vk.size                      = in_size;

    binding.buffer_updates[in_buffer_ptr].first.push_back (update);
    binding.buffer_updates[in_buffer_ptr].second.push_back(update_vk);
}

/** Please see header for specification */
void Anvil::SparseMemoryBindingUpdateInfo::append_image_memory_update(SparseMemoryBindInfoID         in_bind_info_id,
                                                                      Anvil::Image*                  in_image_ptr,
                                                                      const Anvil::ImageSubresource& in_subresource,
                                                                      const VkOffset3D&              in_offset,
                                                                      const VkExtent3D&              in_extent,
                                                                      Anvil::SparseMemoryBindFlags   in_flags,
                                                                      Anvil::MemoryBlock*            in_opt_memory_block_ptr,
                                                                      VkDeviceSize                   in_opt_memory_block_start_offset,
                                                                      bool                           in_opt_memory_block_owned_by_image)
{
    /* Sanity checks .. */
    anvil_assert(in_image_ptr      != nullptr);
    anvil_assert(in_flags          == 0);
    anvil_assert(m_bindings.size() > in_bind_info_id);

    anvil_assert(in_image_ptr->get_create_info_ptr()->get_n_layers()                           > in_subresource.array_layer);
    anvil_assert(in_image_ptr->get_n_mipmaps                      ()                           > in_subresource.mip_level);
    anvil_assert(in_image_ptr->has_aspects                        (in_subresource.aspect_mask) );

    if (in_opt_memory_block_ptr != nullptr)
    {
        anvil_assert(in_opt_memory_block_ptr->get_create_info_ptr()->get_size() > in_opt_memory_block_start_offset);
    }

    /* Cache the update */
    auto&                   binding = m_bindings.at(in_bind_info_id);
    ImageBindInfo           update;
    VkSparseImageMemoryBind update_vk;

    update.extent                      = in_extent;
    update.flags                       = in_flags;
    update.memory_block_owned_by_image = in_opt_memory_block_owned_by_image;
    update.memory_block_ptr            = in_opt_memory_block_ptr;
    update.memory_block_start_offset   = in_opt_memory_block_start_offset;
    update.offset                      = in_offset;
    update.subresource                 = in_subresource;

    update_vk.extent       = in_extent;
    update_vk.flags        = in_flags.get_vk();
    update_vk.memory       = (in_opt_memory_block_ptr != nullptr) ? in_opt_memory_block_ptr->get_memory()
                                                                  : VK_NULL_HANDLE;
    update_vk.memoryOffset = (in_opt_memory_block_ptr != nullptr) ? in_opt_memory_block_ptr->get_start_offset() + in_opt_memory_block_start_offset
                                                                  : UINT32_MAX;
    update_vk.offset       = in_offset;
    update_vk.subresource  = in_subresource.get_vk();

    binding.image_updates[in_image_ptr].first.push_back (update);
    binding.image_updates[in_image_ptr].second.push_back(update_vk);
}

/** Please see header for specification */
void Anvil::SparseMemoryBindingUpdateInfo::append_opaque_image_memory_update(SparseMemoryBindInfoID       in_bind_info_id,
                                                                             Anvil::Image*                in_image_ptr,
                                                                             VkDeviceSize                 in_resource_offset,
                                                                             VkDeviceSize                 in_size,
                                                                             Anvil::SparseMemoryBindFlags in_flags,
                                                                             Anvil::MemoryBlock*          in_opt_memory_block_ptr,
                                                                             VkDeviceSize                 in_opt_memory_block_start_offset,
                                                                             bool                         in_opt_memory_block_owned_by_image,
                                                                             uint32_t                     in_n_plane)
{
    /* Sanity checks */
    anvil_assert(in_image_ptr                                     != nullptr);
    anvil_assert(m_bindings.size                     ()           >  in_bind_info_id);
    anvil_assert(in_image_ptr->get_image_storage_size(in_n_plane) >= in_resource_offset + in_size);

    if (in_opt_memory_block_ptr != nullptr)
    {
        anvil_assert(in_opt_memory_block_ptr->get_create_info_ptr()->get_size() >= in_opt_memory_block_start_offset + in_size);
    }

    /* Cache the update */
    auto&              binding = m_bindings.at(in_bind_info_id);
    GeneralBindInfo    update;
    VkSparseMemoryBind update_vk;

    update.flags                        = in_flags;
    update.memory_block_owned_by_target = in_opt_memory_block_owned_by_image;
    update.memory_block_ptr             = in_opt_memory_block_ptr;
    update.memory_block_start_offset    = in_opt_memory_block_start_offset;
    update.n_plane                      = in_n_plane;
    update.size                         = in_size;
    update.start_offset                 = in_resource_offset;

    update_vk.flags                     = in_flags.get_vk();
    update_vk.memory                    = (in_opt_memory_block_ptr != nullptr) ? in_opt_memory_block_ptr->get_memory()
                                                                               : VK_NULL_HANDLE;
    update_vk.memoryOffset              = (in_opt_memory_block_ptr != nullptr) ? (in_opt_memory_block_ptr->get_start_offset() + in_opt_memory_block_start_offset)
                                                                               : UINT32_MAX;
    update_vk.resourceOffset            = in_resource_offset;
    update_vk.size                      = in_size;

    binding.image_opaque_updates[in_image_ptr].first.push_back (update);
    binding.image_opaque_updates[in_image_ptr].second.push_back(update_vk);
}

/** Please see header for specification */
void Anvil::SparseMemoryBindingUpdateInfo::bake()
{
    const uint32_t n_bindings = static_cast<uint32_t>(m_bindings.size() );
    
    anvil_assert(m_dirty);

    if (n_bindings > 0)
    {
        uint32_t n_buffer_updates_used       = 0;
        uint32_t n_image_updates_used        = 0;
        uint32_t n_opaque_image_updates_used = 0;

        m_bindings_vk.resize             (n_bindings);
        m_device_group_bindings_vk.resize(n_bindings);

        {
            uint32_t n_total_buffer_updates       = 0;
            uint32_t n_total_image_updates        = 0;
            uint32_t n_total_opaque_image_updates = 0;

            for (uint32_t n_binding = 0;
                          n_binding < n_bindings;
                        ++n_binding)
            {
                const auto& bind_info = m_bindings.at(n_binding);

                n_total_buffer_updates       += static_cast<uint32_t>(bind_info.buffer_updates.size      () );
                n_total_image_updates        += static_cast<uint32_t>(bind_info.image_updates.size       () );
                n_total_opaque_image_updates += static_cast<uint32_t>(bind_info.image_opaque_updates.size() );
            }

            m_buffer_bindings_vk.resize      (n_total_buffer_updates);
            m_image_bindings_vk.resize       (n_total_image_updates);
            m_image_opaque_bindings_vk.resize(n_total_opaque_image_updates);
        }

        for (uint32_t n_binding = 0;
                      n_binding < n_bindings;
                    ++n_binding)
        {
            auto&    bind_info                           = m_bindings   [n_binding];
            uint32_t n_buffer_bindings_start_index       = ~0u;
            uint32_t n_image_bindings_start_index        = ~0u;
            uint32_t n_image_opaque_bindings_start_index = ~0u;
            auto&    vk_binding                          = m_bindings_vk[n_binding];
            void**   next_ptr_ptr                        = const_cast<void**>(&vk_binding.pNext);

            bind_info.signal_semaphores_vk.clear();
            bind_info.wait_semaphores_vk.clear  ();

            bind_info.signal_semaphores_vk.reserve(bind_info.signal_semaphores.size() );
            bind_info.wait_semaphores_vk.reserve  (bind_info.wait_semaphores.size  () );

            for (auto& signal_semaphore_ptr : bind_info.signal_semaphores)
            {
                bind_info.signal_semaphores_vk.push_back(signal_semaphore_ptr->get_semaphore() );
            }

            for (auto& wait_semaphore_ptr : bind_info.wait_semaphores)
            {
                bind_info.wait_semaphores_vk.push_back(wait_semaphore_ptr->get_semaphore() );
            }

            vk_binding.bufferBindCount      = static_cast<uint32_t>(bind_info.buffer_updates.size() );
            vk_binding.imageBindCount       = static_cast<uint32_t>(bind_info.image_updates.size() );
            vk_binding.imageOpaqueBindCount = static_cast<uint32_t>(bind_info.image_opaque_updates.size() );
            vk_binding.pNext                = nullptr;
            vk_binding.pSignalSemaphores    = (bind_info.signal_semaphores_vk.size() > 0) ? &bind_info.signal_semaphores_vk[0] : nullptr;
            vk_binding.pWaitSemaphores      = (bind_info.wait_semaphores_vk.size()   > 0) ? &bind_info.wait_semaphores_vk  [0] : nullptr;
            vk_binding.signalSemaphoreCount = static_cast<uint32_t>(bind_info.signal_semaphores_vk.size() );
            vk_binding.sType                = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
            vk_binding.waitSemaphoreCount   = static_cast<uint32_t>(bind_info.wait_semaphores_vk.size() );

            if (bind_info.memory_device_index   != 0 ||
                bind_info.resource_device_index != 0)
            {
                auto& device_group_binding_info = m_device_group_bindings_vk.at(n_binding);

                device_group_binding_info.memoryDeviceIndex   = bind_info.memory_device_index;
                device_group_binding_info.pNext               = nullptr;
                device_group_binding_info.resourceDeviceIndex = bind_info.resource_device_index;
                device_group_binding_info.sType               = VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO_KHR;

                *next_ptr_ptr = &device_group_binding_info;
                next_ptr_ptr  = const_cast<void**>(&device_group_binding_info.pNext);
            }

            n_buffer_bindings_start_index       = n_buffer_updates_used;
            n_image_bindings_start_index        = n_image_updates_used;
            n_image_opaque_bindings_start_index = n_opaque_image_updates_used;

            for (auto& buffer_update : bind_info.buffer_updates)
            {
                const VkBuffer               current_buffer_vk = buffer_update.first->get_buffer();
                VkSparseBufferMemoryBindInfo buffer_bind_info;

                anvil_assert(buffer_update.second.second.size() > 0);

                buffer_bind_info.bindCount = static_cast<uint32_t>(buffer_update.second.second.size() );
                buffer_bind_info.buffer    = current_buffer_vk;
                buffer_bind_info.pBinds    = &buffer_update.second.second[0];

                m_buffer_bindings_vk.at(n_buffer_updates_used) = buffer_bind_info;
                ++n_buffer_updates_used;
            }

            for (auto& image_update : bind_info.image_updates)
            {
                const VkImage               current_image_vk = image_update.first->get_image();
                VkSparseImageMemoryBindInfo image_bind_info;

                anvil_assert(image_update.second.second.size() > 0);

                image_bind_info.bindCount = static_cast<uint32_t>(image_update.second.second.size() );
                image_bind_info.image     = current_image_vk;
                image_bind_info.pBinds    = &image_update.second.second[0];

                m_image_bindings_vk.at(n_image_updates_used) = image_bind_info;
                ++n_image_updates_used;
            }

            for (auto& image_opaque_update : bind_info.image_opaque_updates)
            {
                const VkImage                     current_image_vk = image_opaque_update.first->get_image();
                VkSparseImageOpaqueMemoryBindInfo image_opaque_bind_info;

                anvil_assert(image_opaque_update.second.second.size() > 0);

                image_opaque_bind_info.bindCount = static_cast<uint32_t>(image_opaque_update.second.second.size() );
                image_opaque_bind_info.image     = current_image_vk;
                image_opaque_bind_info.pBinds    = &image_opaque_update.second.second[0];

                m_image_opaque_bindings_vk.at(n_opaque_image_updates_used) = image_opaque_bind_info;
                ++n_opaque_image_updates_used;
            }

            vk_binding.pBufferBinds      = (bind_info.buffer_updates.size()       > 0) ? &m_buffer_bindings_vk.at      (n_buffer_bindings_start_index)       : nullptr;
            vk_binding.pImageBinds       = (bind_info.image_updates.size()        > 0) ? &m_image_bindings_vk.at       (n_image_bindings_start_index)        : nullptr;
            vk_binding.pImageOpaqueBinds = (bind_info.image_opaque_updates.size() > 0) ? &m_image_opaque_bindings_vk.at(n_image_opaque_bindings_start_index) : nullptr;
        }
    }
    else
    {
        m_bindings_vk.clear();
    }

    m_dirty = false;
}

/** Please see header for specification */
bool Anvil::SparseMemoryBindingUpdateInfo::get_bind_info_properties(SparseMemoryBindInfoID in_bind_info_id,
                                                                    uint32_t* const        out_opt_n_buffer_memory_updates_ptr,
                                                                    uint32_t* const        out_opt_n_image_memory_updates_ptr,
                                                                    uint32_t* const        out_opt_n_image_opaque_memory_updates_ptr,
                                                                    uint32_t* const        out_opt_n_signal_semaphores_ptr,
                                                                    Anvil::Semaphore***    out_opt_signal_semaphores_ptr_ptr_ptr,
                                                                    uint32_t* const        out_opt_n_wait_semaphores_ptr,
                                                                    Anvil::Semaphore***    out_opt_wait_semaphores_ptr_ptr_ptr)
{
    decltype(m_bindings)::iterator binding_iterator;
    bool                           result = false;

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(m_bindings.size() > in_bind_info_id);

        goto end;
    }

    binding_iterator = m_bindings.begin() + static_cast<int>(in_bind_info_id);

    if (out_opt_n_buffer_memory_updates_ptr != nullptr)
    {
        uint32_t n_buffer_mem_updates = 0;

        for (const auto& buffer_update_iterator : binding_iterator->buffer_updates)
        {
            n_buffer_mem_updates += static_cast<uint32_t>(buffer_update_iterator.second.first.size() );
        }

        *out_opt_n_buffer_memory_updates_ptr = n_buffer_mem_updates;
    }

    if (out_opt_n_image_memory_updates_ptr != nullptr)
    {
        uint32_t n_image_mem_updates = 0;

        for (const auto& image_update_iterator : binding_iterator->image_updates)
        {
            n_image_mem_updates += static_cast<uint32_t>(image_update_iterator.second.first.size() );
        }

        *out_opt_n_image_memory_updates_ptr = n_image_mem_updates;
    }

    if (out_opt_n_image_opaque_memory_updates_ptr != nullptr)
    {
        uint32_t n_image_opaque_mem_updates = 0;

        for (const auto& image_opaque_update_iterator : binding_iterator->image_opaque_updates)
        {
            n_image_opaque_mem_updates += static_cast<uint32_t>(image_opaque_update_iterator.second.first.size() );
        }

        *out_opt_n_image_opaque_memory_updates_ptr = n_image_opaque_mem_updates;
    }

    if (out_opt_n_signal_semaphores_ptr != nullptr)
    {
        *out_opt_n_signal_semaphores_ptr = static_cast<uint32_t>(binding_iterator->signal_semaphores.size() );
    }

    if (out_opt_signal_semaphores_ptr_ptr_ptr      != nullptr &&
        binding_iterator->signal_semaphores.size() >  0)
    {
        *out_opt_signal_semaphores_ptr_ptr_ptr = &binding_iterator->signal_semaphores.at(0);
    }

    if (out_opt_n_wait_semaphores_ptr != nullptr)
    {
        *out_opt_n_wait_semaphores_ptr = static_cast<uint32_t>(binding_iterator->wait_semaphores.size() );
    }

    if (out_opt_wait_semaphores_ptr_ptr_ptr      != nullptr &&
        binding_iterator->wait_semaphores.size() >  0)
    {
        *out_opt_wait_semaphores_ptr_ptr_ptr = &binding_iterator->wait_semaphores.at(0);
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
void Anvil::SparseMemoryBindingUpdateInfo::get_bind_sparse_call_args(uint32_t*                out_bind_info_count_ptr,
                                                                     const VkBindSparseInfo** out_bind_info_ptr,
                                                                     Anvil::Fence**           out_fence_to_set_ptr)
{
    if (m_dirty)
    {
        bake();

        anvil_assert(!m_dirty);
    }

    *out_bind_info_count_ptr = static_cast<uint32_t>(m_bindings.size() );
    *out_bind_info_ptr       = (m_bindings_vk.size() > 0) ? &m_bindings_vk.at(0) : nullptr;
    *out_fence_to_set_ptr    = m_fence_ptr;
}

/** Please see header for specification */
bool Anvil::SparseMemoryBindingUpdateInfo::get_buffer_memory_update_properties(SparseMemoryBindInfoID in_bind_info_id,
                                                                               uint32_t               in_n_update,
                                                                               Anvil::Buffer**        out_opt_buffer_ptr_ptr,
                                                                               VkDeviceSize*          out_opt_buffer_memory_start_offset_ptr,
                                                                               Anvil::MemoryBlock**   out_opt_memory_block_ptr_ptr,
                                                                               VkDeviceSize*          out_opt_memory_block_start_offset_ptr,
                                                                               bool*                  out_opt_memory_block_owned_by_buffer_ptr,
                                                                               VkDeviceSize*          out_opt_size_ptr) const
{
    GeneralBindInfo                      buffer_bind;
    BufferBindUpdateMap::const_iterator  buffer_binding_map_iterator;
    decltype(m_bindings)::const_iterator binding_iterator;
    uint32_t                             n_current_update = 0;
    bool                                 result           = false;

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= in_bind_info_id) );

        goto end;
    }

    binding_iterator            = m_bindings.cbegin() + static_cast<int>(in_bind_info_id);
    buffer_binding_map_iterator = binding_iterator->buffer_updates.begin();

    while (buffer_binding_map_iterator != binding_iterator->buffer_updates.end() )
    {
        const uint32_t n_buffer_bindings = static_cast<uint32_t>(buffer_binding_map_iterator->second.first.size() );

        if (n_current_update + n_buffer_bindings > in_n_update)
        {
            buffer_bind = buffer_binding_map_iterator->second.first.at(in_n_update - n_current_update);

            break;
        }
        else
        {
            n_current_update            += n_buffer_bindings;
            buffer_binding_map_iterator ++;
        }
    }

    if (buffer_binding_map_iterator == binding_iterator->buffer_updates.end() )
    {
        anvil_assert(!(buffer_binding_map_iterator == binding_iterator->buffer_updates.end()) );

        goto end;
    }

    if (out_opt_buffer_ptr_ptr != nullptr)
    {
        *out_opt_buffer_ptr_ptr = buffer_binding_map_iterator->first;
    }

    if (out_opt_buffer_memory_start_offset_ptr != nullptr)
    {
        *out_opt_buffer_memory_start_offset_ptr = buffer_bind.start_offset;
    }

    if (out_opt_memory_block_owned_by_buffer_ptr != nullptr)
    {
        *out_opt_memory_block_owned_by_buffer_ptr = buffer_bind.memory_block_owned_by_target;
    }

    if (out_opt_memory_block_ptr_ptr != nullptr)
    {
        *out_opt_memory_block_ptr_ptr = buffer_bind.memory_block_ptr;
    }

    if (out_opt_memory_block_start_offset_ptr != nullptr)
    {
        *out_opt_memory_block_start_offset_ptr = buffer_bind.memory_block_start_offset;
    }

    if (out_opt_size_ptr != nullptr)
    {
        *out_opt_size_ptr = buffer_bind.size;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::SparseMemoryBindingUpdateInfo::get_device_indices(SparseMemoryBindInfoID in_bind_info_id,
                                                              uint32_t*              out_opt_resource_device_index_ptr,
                                                              uint32_t*              out_opt_memory_device_index_ptr) const
{
    decltype(m_bindings)::const_iterator binding_iterator;
    bool                                 result          (false);

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= in_bind_info_id) );

        goto end;
    }

    binding_iterator = m_bindings.cbegin() + static_cast<int>(in_bind_info_id);

    if (out_opt_resource_device_index_ptr != nullptr)
    {
        *out_opt_resource_device_index_ptr = binding_iterator->resource_device_index;
    }

    if (out_opt_memory_device_index_ptr != nullptr)
    {
        *out_opt_memory_device_index_ptr = binding_iterator->memory_device_index;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::SparseMemoryBindingUpdateInfo::get_image_memory_update_properties(SparseMemoryBindInfoID        in_bind_info_id,
                                                                              uint32_t                      in_n_update,
                                                                              Anvil::Image**                out_opt_image_ptr_ptr,
                                                                              Anvil::ImageSubresource*      out_opt_subresource_ptr,
                                                                              VkOffset3D*                   out_opt_offset_ptr,
                                                                              VkExtent3D*                   out_opt_extent_ptr,
                                                                              Anvil::SparseMemoryBindFlags* out_opt_flags_ptr,
                                                                              Anvil::MemoryBlock**          out_opt_memory_block_ptr_ptr,
                                                                              VkDeviceSize*                 out_opt_memory_block_start_offset_ptr,
                                                                              bool*                         out_opt_memory_block_owned_by_image_ptr) const
{
    decltype(m_bindings)::const_iterator binding_iterator;
    ImageBindInfo                        image_bind;
    ImageBindUpdateMap::const_iterator   image_binding_map_iterator;
    uint32_t                             n_current_update           = 0;
    bool                                 result                     = false;

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= in_bind_info_id) );

        goto end;
    }

    binding_iterator           = m_bindings.cbegin() + static_cast<int>(in_bind_info_id);
    image_binding_map_iterator = binding_iterator->image_updates.begin();

    while (image_binding_map_iterator != binding_iterator->image_updates.end() )
    {
        const uint32_t n_image_bindings = static_cast<uint32_t>(image_binding_map_iterator->second.first.size() );

        if (n_current_update + n_image_bindings > in_n_update)
        {
            image_bind = image_binding_map_iterator->second.first.at(in_n_update - n_current_update);

            break;
        }
        else
        {
            n_current_update           += n_image_bindings;
            image_binding_map_iterator ++;
        }
    }

    if (image_binding_map_iterator == binding_iterator->image_updates.end() )
    {
        anvil_assert(!(image_binding_map_iterator == binding_iterator->image_updates.end()) );

        goto end;
    }

    if (out_opt_image_ptr_ptr != nullptr)
    {
        *out_opt_image_ptr_ptr = image_binding_map_iterator->first;
    }

    if (out_opt_subresource_ptr != nullptr)
    {
        *out_opt_subresource_ptr = image_bind.subresource;
    }

    if (out_opt_offset_ptr != nullptr)
    {
        *out_opt_offset_ptr = image_bind.offset;
    }

    if (out_opt_extent_ptr != nullptr)
    {
        *out_opt_extent_ptr = image_bind.extent;
    }

    if (out_opt_flags_ptr != nullptr)
    {
        *out_opt_flags_ptr = image_bind.flags;
    }

    if (out_opt_memory_block_ptr_ptr != nullptr)
    {
        *out_opt_memory_block_ptr_ptr = image_bind.memory_block_ptr;
    }

    if (out_opt_memory_block_start_offset_ptr != nullptr)
    {
        *out_opt_memory_block_start_offset_ptr = image_bind.memory_block_start_offset;
    }

    if (out_opt_memory_block_owned_by_image_ptr != nullptr)
    {
        *out_opt_memory_block_owned_by_image_ptr = image_bind.memory_block_owned_by_image;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::SparseMemoryBindingUpdateInfo::get_image_opaque_memory_update_properties(SparseMemoryBindInfoID        in_bind_info_id,
                                                                                     uint32_t                      in_n_update,
                                                                                     Anvil::Image**                out_opt_image_ptr_ptr,
                                                                                     VkDeviceSize*                 out_opt_resource_offset_ptr,
                                                                                     VkDeviceSize*                 out_opt_size_ptr,
                                                                                     Anvil::SparseMemoryBindFlags* out_opt_flags_ptr,
                                                                                     Anvil::MemoryBlock**          out_opt_memory_block_ptr_ptr,
                                                                                     VkDeviceSize*                 out_opt_memory_block_start_offset_ptr,
                                                                                     bool*                         out_opt_memory_block_owned_by_image_ptr,
                                                                                     uint32_t*                     out_opt_n_plane_ptr) const
{
    decltype(m_bindings)::const_iterator     binding_iterator;
    GeneralBindInfo                          image_opaque_bind;
    ImageOpaqueBindUpdateMap::const_iterator image_opaque_binding_map_iterator;
    uint32_t                                 n_current_update           = 0;
    bool                                     result                     = false;

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= in_bind_info_id) );

        goto end;
    }

    binding_iterator                  = m_bindings.cbegin() + static_cast<int>(in_bind_info_id);
    image_opaque_binding_map_iterator = binding_iterator->image_opaque_updates.begin();

    while (image_opaque_binding_map_iterator != binding_iterator->image_opaque_updates.end() )
    {
        const uint32_t n_image_opaque_bindings = static_cast<uint32_t>(image_opaque_binding_map_iterator->second.first.size() );

        if (n_current_update + n_image_opaque_bindings > in_n_update)
        {
            image_opaque_bind = image_opaque_binding_map_iterator->second.first.at(in_n_update - n_current_update);

            break;
        }
        else
        {
            n_current_update                 += n_image_opaque_bindings;
            image_opaque_binding_map_iterator ++;
        }
    }

    if (image_opaque_binding_map_iterator == binding_iterator->image_opaque_updates.end() )
    {
        anvil_assert(!(image_opaque_binding_map_iterator == binding_iterator->image_opaque_updates.end()) );

        goto end;
    }

    if (out_opt_image_ptr_ptr != nullptr)
    {
        *out_opt_image_ptr_ptr = image_opaque_binding_map_iterator->first;
    }

    if (out_opt_resource_offset_ptr != nullptr)
    {
        *out_opt_resource_offset_ptr = image_opaque_bind.start_offset;
    }

    if (out_opt_size_ptr != nullptr)
    {
        *out_opt_size_ptr = image_opaque_bind.size;
    }

    if (out_opt_flags_ptr != nullptr)
    {
        *out_opt_flags_ptr = image_opaque_bind.flags;
    }

    if (out_opt_memory_block_ptr_ptr != nullptr)
    {
        *out_opt_memory_block_ptr_ptr = image_opaque_bind.memory_block_ptr;
    }

    if (out_opt_memory_block_start_offset_ptr != nullptr)
    {
        *out_opt_memory_block_start_offset_ptr = image_opaque_bind.memory_block_start_offset;
    }

    if (out_opt_memory_block_owned_by_image_ptr != nullptr)
    {
        *out_opt_memory_block_owned_by_image_ptr = image_opaque_bind.memory_block_owned_by_target;
    }

    if (out_opt_n_plane_ptr != nullptr)
    {
        *out_opt_n_plane_ptr = image_opaque_bind.n_plane;
    }

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::SparseMemoryBindingUpdateInfo::is_device_group_support_required() const
{
    bool result(false);

    for (const auto& binding_iterator : m_bindings)
    {
        if (binding_iterator.memory_device_index   != 0 ||
            binding_iterator.resource_device_index != 0)
        {
            result = true;

            break;
        }
    }

    return result;
}

/* Updates memory device index, associated with this batch.
 *
 * Do not modify unless VK_KHR_device_group is supported
 *
 * @param in_memory_device_index New memory device index to use.
 *
 **/
void Anvil::SparseMemoryBindingUpdateInfo::set_memory_device_index(SparseMemoryBindInfoID in_bind_info_id,
                                                                   const uint32_t&        in_memory_device_index)
{
    decltype(m_bindings)::iterator binding_iterator;

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= in_bind_info_id) );

        goto end;
    }

    binding_iterator = m_bindings.begin() + static_cast<int>(in_bind_info_id);

    if (binding_iterator->memory_device_index != in_memory_device_index)
    {
        m_dirty                               = true;
        binding_iterator->memory_device_index = in_memory_device_index;
    }

end:
    ;
}

/* Updates resource device index, associated with this batch.
 *
 * Do not modify unless VK_KHR_device_group is supported
 *
 * @param in_resource_device_index New resource device index to use.
 *
 **/
void Anvil::SparseMemoryBindingUpdateInfo::set_resource_device_index(SparseMemoryBindInfoID in_bind_info_id,
                                                                     const uint32_t&        in_resource_device_index)
{
    decltype(m_bindings)::iterator binding_iterator;

    if (m_bindings.size() <= in_bind_info_id)
    {
        anvil_assert(!(m_bindings.size() <= in_bind_info_id) );

        goto end;
    }

    binding_iterator = m_bindings.begin() + static_cast<int>(in_bind_info_id);

    if (binding_iterator->resource_device_index != in_resource_device_index)
    {
        m_dirty                                 = true;
        binding_iterator->resource_device_index = in_resource_device_index;
    }

end:
    ;
}
