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
#include "misc/object_tracker.h"
#include "misc/semaphore_create_info.h"
#include "misc/struct_chainer.h"
#include "wrappers/device.h"
#include "wrappers/semaphore.h"


/* Please see header for specification */
Anvil::Semaphore::Semaphore(Anvil::SemaphoreCreateInfoUniquePtr in_create_info_ptr)
    :DebugMarkerSupportProvider(in_create_info_ptr->get_device(),
                                Anvil::ObjectType::SEMAPHORE),
     MTSafetySupportProvider   (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                in_create_info_ptr->get_device   () )),
     m_semaphore               (VK_NULL_HANDLE)
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::SEMAPHORE,
                                                  this);
}

/* Please see header for specification */
Anvil::Semaphore::~Semaphore()
{
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::SEMAPHORE,
                                                    this);

    release_semaphore();
}

/** Please see header for specification */
Anvil::SemaphoreUniquePtr Anvil::Semaphore::create(Anvil::SemaphoreCreateInfoUniquePtr in_create_info_ptr)
{
    SemaphoreUniquePtr result_ptr(nullptr,
                                  std::default_delete<Semaphore>() );

    result_ptr.reset(
        new Anvil::Semaphore(std::move(in_create_info_ptr) )
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->reset() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/* Please see header for specification */
Anvil::ExternalHandleUniquePtr Anvil::Semaphore::export_to_external_handle(const Anvil::ExternalSemaphoreHandleTypeFlagBits& in_semaphore_handle_type)
{
    #if defined(_WIN32)
        const auto invalid_handle                 = nullptr;
        const bool is_autorelease_handle          = Anvil::Utils::is_nt_handle(in_semaphore_handle_type);
        const bool only_one_handle_ever_permitted = Anvil::Utils::is_nt_handle(in_semaphore_handle_type);
    #else
        const int  invalid_handle                 = -1;
        const bool is_autorelease_handle          = true;
        const bool only_one_handle_ever_permitted = false;
    #endif

    ExternalHandleType             result_handle = invalid_handle;
    Anvil::ExternalHandleUniquePtr result_ptr;

    /* Sanity checks */
    #if defined(_WIN32)
    {
        if (!m_create_info_ptr->get_device()->get_extension_info()->khr_external_semaphore_win32() )
        {
            anvil_assert(m_create_info_ptr->get_device()->get_extension_info()->khr_external_semaphore_win32() );

            goto end;
        }
    }
    #else
    {
        if (!m_create_info_ptr->get_device()->get_extension_info()->khr_external_semaphore_fd() )
        {
            anvil_assert(m_create_info_ptr->get_device()->get_extension_info()->khr_external_semaphore_fd() );

            goto end;
        }
    }
    #endif

    if (only_one_handle_ever_permitted                                                                                                    &&
        m_external_semaphore_created_for_handle_type.find(in_semaphore_handle_type) != m_external_semaphore_created_for_handle_type.end() )
    {
        anvil_assert_fail();

        goto end;
    }

    /* Go and try to open a new handle. */
    {
        #if defined(_WIN32)
            const auto                       entrypoints_ptr = &m_create_info_ptr->get_device()->get_extension_khr_external_semaphore_win32_entrypoints();
            VkSemaphoreGetWin32HandleInfoKHR info;
        #else
            const auto              entrypoints_ptr = &m_create_info_ptr->get_device()->get_extension_khr_external_semaphore_fd_entrypoints();
            VkSemaphoreGetFdInfoKHR info;
        #endif


        anvil_assert(m_semaphore != VK_NULL_HANDLE);

        info.handleType = static_cast<VkExternalSemaphoreHandleTypeFlagBits>(in_semaphore_handle_type);
        info.pNext      = nullptr;
        info.semaphore  = m_semaphore;

        #if defined(_WIN32)
        {
            info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
        }
        #else
        {
            info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
        }
        #endif

        #if defined(_WIN32)
            if (entrypoints_ptr->vkGetSemaphoreWin32HandleKHR(m_create_info_ptr->get_device()->get_device_vk(),
                                                             &info,
                                                             &result_handle) != VK_SUCCESS)
        #else
            if (entrypoints_ptr->vkGetSemaphoreFdKHR(m_create_info_ptr->get_device()->get_device_vk(),
                                                    &info,
                                                    &result_handle) != VK_SUCCESS)
        #endif
        {
            anvil_assert_fail();

            goto end;
        }

        if (result_handle == invalid_handle)
        {
            anvil_assert(result_handle != invalid_handle);

            goto end;
        }
    }

    /* Cache the newly created handle if it's a NT handle  */
    if (only_one_handle_ever_permitted)
    {
        m_external_semaphore_created_for_handle_type[in_semaphore_handle_type] = true;
    }

    result_ptr = Anvil::ExternalHandle::create(result_handle,
                                               is_autorelease_handle); /* in_close_at_destruction_time */

end:
    return result_ptr;
}

#if defined(_WIN32)
    bool Anvil::Semaphore::import_from_external_handle(const bool&                                       in_temporary_import,
                                                       const Anvil::ExternalSemaphoreHandleTypeFlagBits& in_handle_type,
                                                       const ExternalHandleType&                         in_opt_handle,
                                                       const std::wstring&                               in_opt_name)
#else
    bool Anvil::Semaphore::import_from_external_handle(const bool&                                       in_temporary_import,
                                                       const Anvil::ExternalSemaphoreHandleTypeFlagBits& in_handle_type,
                                                       const ExternalHandleType&                         in_handle)
#endif
{
    #if defined(_WIN32)
        const auto entrypoints_ptr = &m_device_ptr->get_extension_khr_external_semaphore_win32_entrypoints();
    #else
        const auto entrypoints_ptr = &m_device_ptr->get_extension_khr_external_semaphore_fd_entrypoints();
    #endif

    bool result = false;

    /* Sanity checks */
    #if defined(_WIN32)
    {
        if (!m_device_ptr->get_extension_info()->khr_external_semaphore_win32() )
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_external_semaphore_win32() );

            goto end;
        }
    }
    #else
    {
        if (!m_device_ptr->get_extension_info()->khr_external_semaphore_fd() )
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_external_semaphore_fd() );

            goto end;
        }
    }
    #endif

    /* Proceed */
    {
        #if defined(_WIN32)
            VkImportSemaphoreWin32HandleInfoKHR info_vk;
        #else
            VkImportSemaphoreFdInfoKHR info_vk;
        #endif

        info_vk.flags      = (in_temporary_import) ? VK_SEMAPHORE_IMPORT_TEMPORARY_BIT_KHR
                                                   : 0;
        info_vk.handleType = static_cast<VkExternalSemaphoreHandleTypeFlagBitsKHR>(in_handle_type);
        info_vk.pNext      = nullptr;
        info_vk.semaphore  = m_semaphore;

        #if defined(_WIN32)
        {
            info_vk.handle = in_opt_handle;
            info_vk.name   = (in_opt_name.size() > 0) ? &in_opt_name.at(0)
                                                      : nullptr;
            info_vk.sType      = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
        }
        #else
        {
            info_vk.fd    = in_handle;
            info_vk.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
        }
        #endif

        #if defined(_WIN32)
        {
            result = (entrypoints_ptr->vkImportSemaphoreWin32HandleKHR(m_device_ptr->get_device_vk(),
                                                                      &info_vk) == VK_SUCCESS);
        }
        #else
        {
            result = (entrypoints_ptr->vkImportSemaphoreFdKHR(m_device_ptr->get_device_vk(),
                                                             &info_vk) == VK_SUCCESS);
        }
        #endif
    }

end:
    return result;
}

/** Destroys the underlying Vulkan Semaphore instance. */
void Anvil::Semaphore::release_semaphore()
{
    if (m_semaphore != VK_NULL_HANDLE)
    {
        lock();
        {
            Anvil::Vulkan::vkDestroySemaphore(m_device_ptr->get_device_vk(),
                                              m_semaphore,
                                              nullptr /* pAllocator */);
        }
        unlock();

        m_semaphore = VK_NULL_HANDLE;
        set_vk_handle(m_semaphore);
    }
}

/* Please see header for specification */
bool Anvil::Semaphore::reset()
{
    VkResult                                           result           (VK_ERROR_INITIALIZATION_FAILED);
    Anvil::StructChainer<VkSemaphoreCreateInfo>        struct_chainer;
    Anvil::StructChainUniquePtr<VkSemaphoreCreateInfo> struct_chain_ptr;

    release_semaphore();

    /* Sanity checks */
    if (m_create_info_ptr->get_exportable_external_semaphore_handle_types() != Anvil::ExternalSemaphoreHandleTypeFlagBits::NONE)
    {
        if (!m_device_ptr->get_extension_info()->khr_external_semaphore() )
        {
            anvil_assert(m_device_ptr->get_extension_info()->khr_external_semaphore() );

            goto end;
        }
    }

    /* Spawn a new semaphore */
    {
        VkSemaphoreCreateInfo semaphore_create_info;

        semaphore_create_info.flags = 0;
        semaphore_create_info.pNext = nullptr;
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        struct_chainer.append_struct(semaphore_create_info);
    }

    if (m_create_info_ptr->get_exportable_external_semaphore_handle_types() != Anvil::ExternalSemaphoreHandleTypeFlagBits::NONE)
    {
        VkExportSemaphoreCreateInfo create_info;

        create_info.handleTypes = m_create_info_ptr->get_exportable_external_semaphore_handle_types().get_vk();
        create_info.pNext       = nullptr;
        create_info.sType       = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;

        struct_chainer.append_struct(create_info);
    }

    #if defined(_WIN32)
    {
        const Anvil::ExternalNTHandleInfo* nt_handle_info_ptr = nullptr;

        if (m_create_info_ptr->get_exportable_nt_handle_info(&nt_handle_info_ptr) )
        {
            VkExportSemaphoreWin32HandleInfoKHR handle_info;

            anvil_assert( nt_handle_info_ptr                                                                                                                   != nullptr);
            anvil_assert(((m_create_info_ptr->get_exportable_external_semaphore_handle_types() & Anvil::ExternalSemaphoreHandleTypeFlagBits::OPAQUE_WIN32_BIT) != 0)       ||
                         ((m_create_info_ptr->get_exportable_external_semaphore_handle_types() & Anvil::ExternalSemaphoreHandleTypeFlagBits::D3D12_FENCE_BIT)  != 0));

            handle_info.dwAccess    = nt_handle_info_ptr->access;
            handle_info.name        = (nt_handle_info_ptr->name.size() > 0) ? &nt_handle_info_ptr->name.at(0)
                                                                            : nullptr;
            handle_info.pAttributes = nt_handle_info_ptr->attributes_ptr;
            handle_info.pNext       = nullptr;
            handle_info.sType       = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;

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

    result = Anvil::Vulkan::vkCreateSemaphore(m_device_ptr->get_device_vk(),
                                              struct_chain_ptr->get_root_struct(),
                                              nullptr, /* pAllocator */
                                             &m_semaphore);

    anvil_assert_vk_call_succeeded(result);
    if (is_vk_call_successful(result) )
    {
        set_vk_handle(m_semaphore);
    }

end:
    return is_vk_call_successful(result);
}
