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

#include "misc/debug.h"
#include "misc/external_handle.h"
#include "misc/fence_create_info.h"
#include "misc/object_tracker.h"
#include "misc/struct_chainer.h"
#include "wrappers/device.h"
#include "wrappers/fence.h"
#include <algorithm>


/* Please see header for specification */
Anvil::Fence::Fence(Anvil::FenceCreateInfoUniquePtr in_create_info_ptr)
    :DebugMarkerSupportProvider(in_create_info_ptr->get_device(),
                                Anvil::ObjectType::FENCE),
     MTSafetySupportProvider   (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                in_create_info_ptr->get_device   () )),
     m_fence                   (VK_NULL_HANDLE)
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    /* Register the event instance */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::FENCE,
                                                  this);
}

/* Please see header for specification */
Anvil::Fence::~Fence()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::FENCE,
                                                    this);

    release_fence();
}

/* Please see header for specification */
Anvil::FenceUniquePtr Anvil::Fence::create(Anvil::FenceCreateInfoUniquePtr in_create_info_ptr)
{
    Anvil::FenceUniquePtr result_ptr(nullptr,
                                     std::default_delete<Anvil::Fence>() );

    result_ptr.reset(
        new Anvil::Fence(std::move(in_create_info_ptr) )
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/* Please see header for specification */
Anvil::ExternalHandleUniquePtr Anvil::Fence::export_to_external_handle(const Anvil::ExternalFenceHandleTypeFlagBits& in_fence_handle_type)
{
    #if defined(_WIN32)
        const auto invalid_handle                 = nullptr;
        const bool handle_needs_manual_release    = Anvil::Utils::is_nt_handle(in_fence_handle_type);
        const bool only_one_handle_ever_permitted = Anvil::Utils::is_nt_handle(in_fence_handle_type);
    #else
        const int  invalid_handle                 = -1;
        const bool handle_needs_manual_release    = true;
        const bool only_one_handle_ever_permitted = false;
    #endif

    ExternalHandleType             result_handle = 0;
    Anvil::ExternalHandleUniquePtr result_ptr;

    /* Sanity checks */
    #if defined(_WIN32)
    {
        if (!m_create_info_ptr->get_device()->get_extension_info()->khr_external_fence_win32() )
        {
            anvil_assert(m_create_info_ptr->get_device()->get_extension_info()->khr_external_fence_win32() );

            goto end;
        }
    }
    #else
    {
        if (!m_create_info_ptr->get_device()->get_extension_info()->khr_external_fence_fd() )
        {
            anvil_assert(m_create_info_ptr->get_device()->get_extension_info()->khr_external_fence_fd() );

            goto end;
        }
    }
    #endif

    if (only_one_handle_ever_permitted                                                                                        &&
        m_external_fence_created_for_handle_type.find(in_fence_handle_type) != m_external_fence_created_for_handle_type.end() )
    {
        anvil_assert_fail();

        goto end;
    }

    /* Go and try to open a new handle. */
    #if defined(_WIN32)
    {
        const auto                   entrypoints_ptr = &m_create_info_ptr->get_device()->get_extension_khr_external_fence_win32_entrypoints();
        VkFenceGetWin32HandleInfoKHR info;

        anvil_assert(m_fence != VK_NULL_HANDLE);

        info.fence      = m_fence;
        info.handleType = static_cast<VkExternalFenceHandleTypeFlagBitsKHR>(in_fence_handle_type);
        info.pNext      = nullptr;
        info.sType      = VK_STRUCTURE_TYPE_FENCE_GET_WIN32_HANDLE_INFO_KHR;

        if (entrypoints_ptr->vkGetFenceWin32HandleKHR(m_create_info_ptr->get_device()->get_device_vk(),
                                                      &info,
                                                      &result_handle) != VK_SUCCESS)
        {
            anvil_assert_fail();

            goto end;
        }
    }
    #else
    {
        const auto          entrypoints_ptr = &m_create_info_ptr->get_device()->get_extension_khr_external_fence_fd_entrypoints();
        VkFenceGetFdInfoKHR info;

        anvil_assert(m_fence != VK_NULL_HANDLE);

        info.fence      = m_fence;
        info.handleType = static_cast<VkExternalFenceHandleTypeFlagBits>(in_fence_handle_type);
        info.pNext      = nullptr;
        info.sType      = VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR;

        if (entrypoints_ptr->vkGetFenceFdKHR(m_create_info_ptr->get_device()->get_device_vk(),
                                            &info,
                                            &result_handle) != VK_SUCCESS)
        {
            anvil_assert_fail();

            goto end;
        }
    }
    #endif

    if (result_handle == invalid_handle)
    {
        anvil_assert(result_handle != invalid_handle);

        goto end;
    }

    /* If this is necessary, cache the newly created handle so that we do not let the app attempt to re-create the external handle
     * for the same Vulkan fence handle.
     */
    if (only_one_handle_ever_permitted)
    {
        m_external_fence_created_for_handle_type[in_fence_handle_type] = true;
    }

    result_ptr = Anvil::ExternalHandle::create(result_handle,
                                               handle_needs_manual_release); /* in_close_at_destruction_time */

end:
    return result_ptr;
}

#if defined(_WIN32)
    bool Anvil::Fence::import_from_external_handle(const bool&                                   in_temporary_import,
                                                   const Anvil::ExternalFenceHandleTypeFlagBits& in_handle_type,
                                                   const ExternalHandleType&                     in_opt_handle,
                                                   const std::wstring&                           in_opt_name)
#else
    bool Anvil::Fence::import_from_external_handle(const bool&                                   in_temporary_import,
                                                   const Anvil::ExternalFenceHandleTypeFlagBits& in_handle_type,
                                                   const ExternalHandleType&                      in_handle)
#endif
{
    #if defined(_WIN32)
        const auto entrypoints_ptr = &m_device_ptr->get_extension_khr_external_fence_win32_entrypoints();
    #else
        const auto entrypoints_ptr = &m_device_ptr->get_extension_khr_external_fence_fd_entrypoints();
    #endif

    bool result = false;

    /* Sanity checks */
    #if defined(_WIN32)
    {
        if (!m_device_ptr->get_extension_info()->khr_external_fence_win32() )
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_external_fence_win32() );

            goto end;
        }
    }
    #else
    {
        if (!m_device_ptr->get_extension_info()->khr_external_fence_fd() )
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_external_fence_fd() );

            goto end;
        }
    }
    #endif

    /* Proceed */
    #if defined(_WIN32)
    {
        VkImportFenceWin32HandleInfoKHR info_vk;

        info_vk.fence      = m_fence;
        info_vk.flags      = (in_temporary_import) ? VK_FENCE_IMPORT_TEMPORARY_BIT_KHR
                                                   : 0;
        info_vk.handle     = in_opt_handle;
        info_vk.handleType = static_cast<VkExternalFenceHandleTypeFlagBitsKHR>(in_handle_type);
        info_vk.name       = (in_opt_name.size() > 0) ? &in_opt_name.at(0)
                                                      : nullptr;
        info_vk.pNext      = nullptr;
        info_vk.sType      = VK_STRUCTURE_TYPE_IMPORT_FENCE_WIN32_HANDLE_INFO_KHR;

        result = (entrypoints_ptr->vkImportFenceWin32HandleKHR(m_device_ptr->get_device_vk(),
                                                              &info_vk) == VK_SUCCESS);
    }
    #else
    {
        VkImportFenceFdInfoKHR info_vk;

        info_vk.fd         = in_handle;
        info_vk.fence      = m_fence;
        info_vk.flags      = (in_temporary_import) ? VK_FENCE_IMPORT_TEMPORARY_BIT_KHR
                                                   : 0;
        info_vk.handleType = static_cast<VkExternalFenceHandleTypeFlagBitsKHR>(in_handle_type);
        info_vk.pNext      = nullptr;
        info_vk.sType      = VK_STRUCTURE_TYPE_IMPORT_FENCE_FD_INFO_KHR;

        result = (entrypoints_ptr->vkImportFenceFdKHR(m_device_ptr->get_device_vk(),
                                                     &info_vk) == VK_SUCCESS);
    }
    #endif
end:
    return result;
}

bool Anvil::Fence::init()
{
    VkFenceCreateInfo                              fence_create_info;
    VkResult                                       result           (VK_ERROR_INITIALIZATION_FAILED);
    Anvil::StructChainer<VkFenceCreateInfo>        struct_chainer;
    Anvil::StructChainUniquePtr<VkFenceCreateInfo> struct_chain_ptr;

    /* Sanity checks */
    if (m_create_info_ptr->get_exportable_external_fence_handle_types() != Anvil::ExternalFenceHandleTypeFlagBits::NONE)
    {
        if (!m_device_ptr->get_extension_info()->khr_external_fence() )
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_external_fence() );

            goto end;
        }
    }

    /* Spawn a new fence */
    {
        fence_create_info.flags = (m_create_info_ptr->should_create_signalled() ) ? VK_FENCE_CREATE_SIGNALED_BIT
                                                                                  : 0u;
        fence_create_info.pNext = nullptr;
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        struct_chainer.append_struct(fence_create_info);
    }

    if (m_create_info_ptr->get_exportable_external_fence_handle_types() != Anvil::ExternalFenceHandleTypeFlagBits::NONE)
    {
        VkExportFenceCreateInfo create_info;

        create_info.handleTypes = m_create_info_ptr->get_exportable_external_fence_handle_types().get_vk();
        create_info.pNext       = nullptr;
        create_info.sType       = VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO_KHR;

        struct_chainer.append_struct(create_info);
    }

    #if defined(_WIN32)
    {
        const Anvil::ExternalNTHandleInfo* nt_handle_info_ptr = nullptr;

        if (m_create_info_ptr->get_exportable_nt_handle_info(&nt_handle_info_ptr) )
        {
            VkExportFenceWin32HandleInfoKHR handle_info;

            anvil_assert(nt_handle_info_ptr                                                                                                           != nullptr);
            anvil_assert((m_create_info_ptr->get_exportable_external_fence_handle_types() & Anvil::ExternalFenceHandleTypeFlagBits::OPAQUE_WIN32_BIT) != 0);

            handle_info.dwAccess    = nt_handle_info_ptr->access;
            handle_info.name        = (nt_handle_info_ptr->name.size() > 0) ? &nt_handle_info_ptr->name.at(0)
                                                                            : nullptr;
            handle_info.pAttributes = nt_handle_info_ptr->attributes_ptr;
            handle_info.pNext       = nullptr;
            handle_info.sType       = VK_STRUCTURE_TYPE_EXPORT_FENCE_WIN32_HANDLE_INFO_KHR;

            struct_chainer.append_struct(handle_info);
        }
    }
    #endif

    struct_chain_ptr = struct_chainer.create_chain();
    if (struct_chain_ptr == nullptr)
    {
        anvil_assert(struct_chain_ptr != nullptr);

        goto end;
    }

    result = Anvil::Vulkan::vkCreateFence(m_device_ptr->get_device_vk(),
                                          struct_chain_ptr->get_root_struct(),
                                          nullptr, /* pAllocator */
                                         &m_fence);

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        set_vk_handle(m_fence);
    }

end:
    return is_vk_call_successful(result);
}

/* Please see header for specification */
bool Anvil::Fence::is_set() const
{
    VkResult result;

    result = Anvil::Vulkan::vkGetFenceStatus(m_device_ptr->get_device_vk(),
                                             m_fence);

    anvil_assert(result == VK_SUCCESS  ||
                 result == VK_NOT_READY);

    return (result == VK_SUCCESS);
}

/** Destroys the underlying Vulkan Fence instance. */
void Anvil::Fence::release_fence()
{
    if (m_fence != VK_NULL_HANDLE)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyFence(m_device_ptr->get_device_vk(),
                                          m_fence,
                                          nullptr /* pAllocator */);
        }
        unlock();

        m_fence = VK_NULL_HANDLE;
    }
}

/* Please see header for specification */
bool Anvil::Fence::reset()
{
    VkResult result;

    lock();
    {
        result = Anvil::Vulkan::vkResetFences(m_device_ptr->get_device_vk(),
                                              1, /* fenceCount */
                                             &m_fence);
    }
    unlock();

    return (result == VK_SUCCESS);
}

/* Please see header for specification */
bool Anvil::Fence::reset_fences(const uint32_t in_n_fences,
                                Fence*         in_fences)
{
    const Anvil::BaseDevice* device_ptr           = nullptr;
    auto                     fence_cache          = std::vector<VkFence>(in_n_fences);
    static const uint32_t    fence_cache_capacity = sizeof(fence_cache) / sizeof(fence_cache[0]);
    bool                     result               = true;
    VkResult                 result_vk;

    if (in_n_fences == 0)
    {
        goto end;
    }

    for (uint32_t n_fence_batch = 0;
                  n_fence_batch < 1 + in_n_fences / fence_cache_capacity;
                ++n_fence_batch)
    {
        const uint32_t n_fences_remaining = in_n_fences - n_fence_batch * fence_cache_capacity;

        for (uint32_t n_fence = 0;
                      n_fence < n_fences_remaining;
                    ++n_fence)
        {
            Anvil::Fence& current_fence = in_fences[n_fence_batch * fence_cache_capacity + n_fence];

            anvil_assert((device_ptr == nullptr)                                          ||
                         (device_ptr != nullptr && current_fence.m_device_ptr != nullptr) );

            device_ptr           = current_fence.m_device_ptr;
            fence_cache[n_fence] = current_fence.m_fence;

            current_fence.lock();
        }
        {
            result_vk = Anvil::Vulkan::vkResetFences(device_ptr->get_device_vk(),
                                                     n_fences_remaining,
                                                     (n_fences_remaining > 0) ? &fence_cache.at(0) : nullptr);
        }
        for (uint32_t n_fence = 0;
                      n_fence < n_fences_remaining;
                    ++n_fence)
        {
            Anvil::Fence& current_fence = in_fences[n_fence_batch * fence_cache_capacity + n_fence];

            current_fence.unlock();
        }

        anvil_assert_vk_call_succeeded(result_vk);

        if (!is_vk_call_successful(result_vk) )
        {
            result = false;
        }
    }

end:
    return result;
}
