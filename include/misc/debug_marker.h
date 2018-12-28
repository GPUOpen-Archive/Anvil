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

/* Provides support for functionality introduced by VK_EXT_debug_marker extension, later subsumed by VK_EXT_debug_utils. */
#ifndef MISC_DEBUG_MARKER_H
#define MISC_DEBUG_MARKER_H

#include "misc/debug.h"
#include "misc/types.h"
#include <stdarg.h>


namespace Anvil
{
    /* Implements name + tag caching mechanism. Should only be used by DebugMarkerSupportProvider. */
    class DebugMarkerSupportProviderWorker
    {
    public:
        /* Public functions */

        /** Constructor. */
        DebugMarkerSupportProviderWorker(const Anvil::BaseDevice* in_device_ptr,
                                         const Anvil::ObjectType& in_vk_object_type);

        /** Destructor. */
        ~DebugMarkerSupportProviderWorker()
        {
            /* Stub - neither names nor tags need to be manually destroyed */
        }

        /** Returns the name associated with the worker instance */
        const std::string& get_name() const
        {
            return m_object_name;
        }

        /** Returns tag data associated with the worker instance.
         *
         *  @param out_object_tag_data_ptr Deref will be set to a ptr to the vector holding user-specified tag data.
         *                                 Must not be null.
         *  @param out_object_tag_name_ptr Deref will be set to the tag name. Must not be null.
         **/
        void get_tag(const std::vector<char>** out_object_tag_data_ptr,
                     uint64_t*                 out_object_tag_name_ptr) const
        {
            *out_object_tag_data_ptr = &m_object_tag_data;
            *out_object_tag_name_ptr = m_object_tag_name;
        }

        /** Returns a Vulkan object handle associated with the worker instance */
        uint64_t get_vk_handle_internal() const
        {
            return m_vk_object_handle;
        }

        /** Updates the name associated with the maintained Vulkan object handle.
         *
         *  The function will optionally perform a relevant VK_EXT_debug_marker API invocation, if the
         *  device specified at creation time supports the extension.
         *
         *  @param in_object_name  New object name to use. Must not be null.
         *  @param in_should_force True if the name should be updated, even in cases where the same name been already specified
         *                         in a preceding invocation.
         */
        void set_name_internal(const std::string& in_object_name,
                               bool               in_should_force = false);

        /** Updates tag data associated with the maintained Vulkan object handle.
         *
         *  The function will optionally perform a relevant VK_EXT_debug_marker API invocation, if the
         *  device specified at creation time supports the extension.
         *
         *  @param in_tag_name     Meaning as per VK_EXT_debug_marker extension specification.
         *  @param in_tag_size     Meaning as per VK_EXT_debug_marker extension specification.
         *  @param in_tag_ptr      Meaning as per VK_EXT_debug_marker extension specification.
         *  @param in_should_force True if tag data should be updated, even in cases where the same properties have already been
         *                         a preceding invocations
         */
        void set_tag_internal(const uint64_t in_tag_name,
                              size_t         in_tag_size,
                              const void*    in_tag_ptr,
                              bool           in_should_force = false);

        /** Associates a new Vulkan handle with the instance.
         *
         *  @param in_vk_object_handle New Vulkan object handle. May be VK_NULL_HANDLE if previously
         *                             assigned a non-null handle.
         */
        void set_vk_handle_internal(uint64_t in_vk_object_handle);

    private:
        /* Private type definitions */
        enum class DebugAPI
        {
            EXT_DEBUG_MARKER,
            EXT_DEBUG_UTILS,

            NONE
        };

        /* Private variables */
        const Anvil::BaseDevice*   m_device_ptr;
        std::mutex                 m_mutex;
        std::string                m_object_name;
        std::vector<char>          m_object_tag_data;
        uint64_t                   m_object_tag_name;
        DebugAPI                   m_used_api;
        uint64_t                   m_vk_object_handle;
        Anvil::ObjectType          m_vk_object_type;
    };

    /** This class needs to be inherited from by all wrapper classes that wrap Vulkan objects.
     *
     *  It supports two different modes, depending on the use case:
     *
     *  1) If delegate workers are not requested at creation time, only one Vulkan handle can be cached.
     *     Any attempt to assign more handles without first calling set_vk_handle() with VK_NULL_HANDLE arg
     *     will trigger an assertion failure.
     *  2) If delegate workers are enabled at creation mode, more than one Vulkan handle can be associated
     *     with the instance. In this case, add_delegate() and remove_delegate() functions shoudl be used.
     *     Calling set_vk_handle() triggers an assertion failure in this mode.
     *
     *  No matter which mode is active, only one name & one tag can be associated with maintained set of
     *  Vulkan handles. set_*() function invocations will automatically update corresponding information
     *  for all associated Vulkan handles.
     *
     *  If VK_EXT_debug_marker extension is enabled, relevant API calls will share the information with
     *  the implementation(s).
     */
    template<class Wrapper>
    class DebugMarkerSupportProvider
    {
    public:
        /* Public functions */

        /* Constructor.
         *
         * @param in_device_ptr           Base Vulkan device wrapper instance to use. Must not be nullptr.
         * @param in_vk_object_type       Vulkan object type.
         * @param in_use_delegate_workers False if only one handle can be associated with the provider instance.
         *                                True to permit more than one handle to be used.
         *
         **/
        DebugMarkerSupportProvider(const Anvil::BaseDevice* in_device_ptr,
                                   const Anvil::ObjectType& in_object_type,
                                   bool                     in_use_delegate_workers = false)
        {
            anvil_assert(in_device_ptr != nullptr);

            m_device_ptr = in_device_ptr;

            if (!in_use_delegate_workers)
            {
                m_worker_ptr.reset(
                    new DebugMarkerSupportProviderWorker(in_device_ptr,
                                                         in_object_type)
                );
            }
            else
            {
                m_vk_object_type = in_object_type;
            }
        }

        /** Destructor */
        virtual ~DebugMarkerSupportProvider()
        {
            /* Stub */
        }

        /** Associates a new Vulkan object handle with the provider instance.
         *
         *  Must not be called if the provider instance was created with @param in_use_delegate_workers arg
         *  set to false.
         *
         *  @param in_vk_object_handle New Vulkan object handle to use. Must not be null. Must not duplicate
         *                             previously submitted handles, unless it has been removed with
         *                             a remove_delegate() call.
         */
        void add_delegate(uint64_t in_vk_object_handle)
        {
            Anvil::DebugMarkerSupportProviderWorker* new_delegate_raw_ptr = nullptr;

            {
                std::unique_ptr<Anvil::DebugMarkerSupportProviderWorker> new_delegate_ptr;

                anvil_assert(m_worker_ptr == nullptr);

                #ifdef _DEBUG
                {
                    for (auto& delegate_ptr : m_delegate_workers)
                    {
                        anvil_assert(delegate_ptr->get_vk_handle_internal() != in_vk_object_handle);
                    }
                }
                #endif

                new_delegate_ptr.reset(
                    new Anvil::DebugMarkerSupportProviderWorker(m_device_ptr,
                                                                m_vk_object_type)
                );

                new_delegate_ptr->set_vk_handle_internal(in_vk_object_handle);

                new_delegate_raw_ptr = new_delegate_ptr.get();

                m_delegate_workers.push_back(
                    std::move(new_delegate_ptr)
                );
            }

            if (m_delegate_workers.size() > 1)
            {
                /* Make sure to copy already assigned name & tag to the new delegate */
                auto&                    existing_delegate_worker_ptr          = m_delegate_workers.at(0);
                auto                     existing_delegate_worker_name         = existing_delegate_worker_ptr->get_name();
                const std::vector<char>* existing_delegate_worker_tag_data_ptr = nullptr; 
                uint64_t                 existing_delegate_worker_tag_name     = 0;

                existing_delegate_worker_ptr->get_tag(&existing_delegate_worker_tag_data_ptr,
                                                      &existing_delegate_worker_tag_name);

                new_delegate_raw_ptr->set_name_internal(existing_delegate_worker_name.c_str() );

                if (existing_delegate_worker_tag_data_ptr->size() > 0)
                {
                    new_delegate_raw_ptr->set_tag_internal (existing_delegate_worker_tag_name,
                                                            existing_delegate_worker_tag_data_ptr->size(),
                                                           &existing_delegate_worker_tag_data_ptr->at(0) );
                }
            }
        }

        template<class T>
        void add_delegate(T* in_vk_object_handle_ptr)
        {
            uint64_t handle_uint64 = reinterpret_cast<uint64_t>(in_vk_object_handle_ptr);

            add_delegate(handle_uint64);
        }

        /** Drops a Vulkan object handle previously registered with an add_delegate() call.
         *
         *  Must not be called if the provider instance was created with @param in_use_delegate_workers arg
         *  set to false.
         *
         *  @param in_vk_object_handle Vulkan object handle to "forget". Must not be null.
         */
        void remove_delegate(uint64_t in_vk_object_handle)
        {
            std::vector<std::unique_ptr<DebugMarkerSupportProviderWorker> >::iterator worker_iterator;

            anvil_assert(m_worker_ptr == nullptr);

            for (worker_iterator  = m_delegate_workers.begin();
                 worker_iterator != m_delegate_workers.end();
               ++worker_iterator)
            {
                auto& current_worker_ptr = *worker_iterator;

                if (current_worker_ptr->get_vk_handle_internal() == in_vk_object_handle)
                {
                    break;
                }
            }

            anvil_assert(worker_iterator != m_delegate_workers.end() );
            m_delegate_workers.erase(worker_iterator);
        }

        template<class T>
        void remove_delegate(T* in_vk_object_handle_ptr)
        {
            uint64_t handle_uint64 = reinterpret_cast<uint64_t>(in_vk_object_handle_ptr);

            remove_delegate(handle_uint64);
        }

        /** Associates a user-specified name to with all maintained Vulkan object handles.
         *
         *  Passed string's contents is cached internally, so @param in_object_name may be released
         *  after this function leaves.
         *
         *  May be called more than once.
         *
         *  @param in_object_name New object name to use. Must not be null.
         */
        void set_name(const std::string& in_object_name)
        {
            if (m_worker_ptr != nullptr)
            {
                m_worker_ptr->set_name_internal(in_object_name);
            }
            else
            {
                for (auto& worker_ptr : m_delegate_workers)
                {
                    worker_ptr->set_name_internal(in_object_name);
                }
            }
        }

        /** Forms a name using info passed via a variable number of arguments (just like *printf() )
         *  and then behaves exactly like set_name().
         *
         *  Uses a 1024-character stack-based buffer for string formatting purposes.
         *
         *  @param in_format Same as first argument of *printf().
         */
        void set_name_formatted(const char* in_format,
                                ...)
        {
            char    buffer[1024];
            va_list list;

            va_start(list,
                     in_format);
            {
                vsnprintf(buffer,
                          sizeof(buffer),
                          in_format,
                          list);
            }
            va_end(list);

            if (m_worker_ptr != nullptr)
            {
                m_worker_ptr->set_name_internal(buffer);
            }
            else
            {
                for (auto& worker_ptr : m_delegate_workers)
                {
                    worker_ptr->set_name_internal(buffer);
                }
            }
        }

        /** Associates a user-specified tag data to with all maintained Vulkan object handles.
         *
         *  May be called more than once.
         *
         *  @param in_tag_name Meaning as per VK_EXT_debug_marker extension specification.
         *  @param in_tag_size Meaning as per VK_EXT_debug_marker extension specification.
         *  @param in_tag_ptr  Meaning as per VK_EXT_debug_marker extension specification.
         *
         */
        void set_tag(const uint64_t in_tag_name,
                     size_t         in_tag_size,
                     const void*    in_tag_ptr)
        {
            if (m_worker_ptr != nullptr)
            {
                m_worker_ptr->set_tag_internal(in_tag_name,
                                               in_tag_size,
                                               in_tag_ptr);
            }
            else
            {
                for (auto& worker_ptr : m_delegate_workers)
                {
                    worker_ptr->set_tag_internal(in_tag_name,
                                                 in_tag_size,
                                                 in_tag_ptr);
                }
            }
        }

    private:
        /* Private functions */

        /** Associates a new Vulkan handle with the provider instance. Must only be used for
         *  providers instantiated without delegate worker support.
         *
         *  @param in_vk_object_handle New Vulkan object handle. May be VK_NULL_HANDLE if previously
         *                             assigned a non-null handle.
         */
        void set_vk_handle(uint64_t in_vk_object_handle)
        {
            anvil_assert(m_worker_ptr != nullptr);

            m_worker_ptr->set_vk_handle_internal(in_vk_object_handle);
        }

        template<class T>
        void set_vk_handle(T* in_vk_object_handle_ptr)
        {
            uint64_t handle_uint64 = reinterpret_cast<uint64_t>(in_vk_object_handle_ptr);

            set_vk_handle(handle_uint64);
        }

        /* Private variables */

        /* Only used if delegate workers have been requested at creation time: ==> */
        std::vector<std::unique_ptr<DebugMarkerSupportProviderWorker> > m_delegate_workers;
        const Anvil::BaseDevice*                                        m_device_ptr;
        Anvil::ObjectType                                               m_vk_object_type;

        /* <== */

        /* Otherwise: ==> */
        std::unique_ptr<DebugMarkerSupportProviderWorker> m_worker_ptr;
        /* <== */

        friend Wrapper;
    };
};

#endif /* MISC_DEBUG_MARKER_H */