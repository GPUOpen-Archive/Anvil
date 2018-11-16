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

/** Implements a wrapper for a single Vulkan semaphore. Implemented in order to:
 *
 *  - simplify life-time management of semaphores.
 *  - simplify semaphore usage.
 *  - let ObjectTracker detect leaking semaphore instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_SEMAPHORE_H
#define WRAPPERS_SEMAPHORE_H

#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    /* Wrapper class for Vulkan semaphores */
    class Semaphore : public DebugMarkerSupportProvider<Semaphore>,
                      public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Creates a single Vulkan semaphore instance and registers the object in Object Tracker. */
        static Anvil::SemaphoreUniquePtr create(Anvil::SemaphoreCreateInfoUniquePtr in_create_info_ptr);

        /* Creates a new external semaphore handle of the user-specified type.
         *
         * For NT handle types, the function can only be called once per each NT handle type. Subsequent
         * calls will result in the function triggering an assertion failure and returning null.
         *
         * Returns nullptr if unsuccessful.
         *
         * Requires VK_KHR_external_semaphore_fd    under Linux.
         * Requires VK_KHR_external_semaphore_win32 under Windows.
         */
        ExternalHandleUniquePtr export_to_external_handle(const Anvil::ExternalSemaphoreHandleTypeFlagBits& in_semaphore_handle_type);

        /** Destructor.
         *
         *  Destroys the Vulkan counterpart and unregisters the wrapper instance from the Object Tracker.
         **/
        virtual ~Semaphore();

        const Anvil::SemaphoreCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        /** Retrieves a raw handle to the underlying Vulkan semaphore instance  */
        VkSemaphore get_semaphore() const
        {
            return m_semaphore;
        }

        /** Retrieves a pointer to a raw handle of the underlying Vulkan semaphore instance */
        const VkSemaphore* get_semaphore_ptr() const
        {
            return &m_semaphore;
        }

        /* TODO
         *
         * Requires VK_KHR_external_semaphore_fd    under Linux.
         * Requires VK_KHR_external_semaphore_win32 under Windows.
         *
         * NOTE: Under Linux, @param in_handle is no longer valid if function returns true.
         *
         * @return true if successful, false otherwise.
         *
         * @param in_temporary_import True if a temporary import operation should be performed. False if
         *                            a permanent import is being requested.
         * @param in_handle_type      Type of the handle that is being imported.
         * @param in_handle           (Linux):   Handle to use. Must not be -1.
         * @param in_opt_handle       (Windows): Win32 NT handle to use. Must not be null if @param in_opt_name is null and vice versa.
         * @param in_opt_name         (Windows): Name of the handle to use. Must not be null if @param in_opt_handle is null and vice versa.
         */
        #if defined(_WIN32)
            bool import_from_external_handle(const bool&                                       in_temporary_import,
                                             const Anvil::ExternalSemaphoreHandleTypeFlagBits& in_handle_type,
                                             const ExternalHandleType&                         in_opt_handle,
                                             const std::wstring&                               in_opt_name);
        #else
            bool import_from_external_handle(const bool&                                       in_temporary_import,
                                             const Anvil::ExternalSemaphoreHandleTypeFlagBits& in_handle_type,
                                             const ExternalHandleType&                         in_handle);
        #endif

        /** Releases the underlying Vulkan Semaphore instance and creates a new Vulkan object. */
        bool reset();

    private:
        /* Private functions */

        /* Constructor. Please see create() for specification */
        Semaphore(Anvil::SemaphoreCreateInfoUniquePtr in_create_info_ptr);

        void release_semaphore();

        /* Private variables */
        Anvil::SemaphoreCreateInfoUniquePtr                        m_create_info_ptr;
        std::map<Anvil::ExternalSemaphoreHandleTypeFlagBits, bool> m_external_semaphore_created_for_handle_type;
        VkSemaphore                                                m_semaphore;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(Semaphore);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(Semaphore);
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SEMAPHORE_H */
