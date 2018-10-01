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

/** Implements a wrapper for a single Vulkan fence. Implemented in order to:
 *
 *  - simplify life-time management of fences.
 *  - simplify fence usage.
 *  - let ObjectTracker detect leaking fence instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_FENCE_H
#define WRAPPERS_FENCE_H

#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    /* Wrapper class for Vulkan fences */
    class Fence : public DebugMarkerSupportProvider<Fence>,
                  public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Destructor.
         *
         *  Destroys the Vulkan counterpart and unregister the wrapper instance from the object tracker.
         **/
        virtual ~Fence();

        /** Creates a new Fence instance.
         *
         *  Creates a single Vulkan fence instance and registers the object in Object Tracker.
         */
        static Anvil::FenceUniquePtr create(Anvil::FenceCreateInfoUniquePtr in_create_info_ptr);

        /* Creates a new external fence handle of the user-specified type.
         *
         * For NT handle types, the function can only be called once per each NT handle type. Subsequent
         * calls will result in the function triggering an assertion failure and returning null.
         *
         * Returns nullptr if unsuccessful.
         *
         * Requires VK_KHR_external_fence_fd    under Linux.
         * Requires VK_KHR_external_fence_win32 under Windows.
         */
        ExternalHandleUniquePtr export_to_external_handle(const Anvil::ExternalFenceHandleTypeFlagBits& in_fence_handle_type);

        const Anvil::FenceCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        /** Retrieves a raw handle to the underlying Vulkan fence instance */
        VkFence get_fence() const
        {
            return m_fence;
        }

        /** Retrieves a pointer to the raw handle to the underlying Vulkan fence instance */
        const VkFence* get_fence_ptr() const
        {
            return &m_fence;
        }

        /* TODO
         *
         * Requires VK_KHR_external_fence_fd    under Linux.
         * Requires VK_KHR_external_fence_win32 under Windows.
         *
         * NOTE: Under Linux, @param in_handle is no longer valid if function returns true.
         *
         * @return true if successful, false otherwise.
         *
         * @param in_temporary_import True if a temporary import operation should be performed. False if
         *                            a permanent import is being requested.
         * @param in_handle_type      Type of the handle that is being imported.
         * @param in_handle           (Linux):   Handle to use.
         * @param in_opt_handle       (Windows): Handle to use. Must not be null if @param in_opt_name is null and vice versa.
         * @param in_opt_name         (Windows): Name of the handle to use. Must not be null if @param in_opt_handle is null and vice versa.
         */
        #if defined(_WIN32)
            bool import_from_external_handle(const bool&                                   in_temporary_import,
                                             const Anvil::ExternalFenceHandleTypeFlagBits& in_handle_type,
                                             const ExternalHandleType&                     in_opt_handle,
                                             const std::wstring&                           in_opt_name);
        #else
            bool import_from_external_handle(const bool&                                   in_temporary_import,
                                             const Anvil::ExternalFenceHandleTypeFlagBits& in_handle_type,
                                             const ExternalHandleType&                     in_handle);
        #endif

        /** Tells whether the fence is signalled at the time of the call.
         *
         *  @return true if the fence is set, false otherwise.
         **/
        bool is_set() const;

        /** Resets the specified Vulkan Fence, if set. If the fence is not set, this function is a nop.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool reset();

        /** Resets the specified number of Vulkan fences.
         *
         *  This function is expected to be more efficient than calling reset() for @param in_n_fences
         *  times, assuming @param in_n_fences is larger than 1.
         *
         *  @param in_n_fences Number of Fence instances accessible under @param in_fences.
         *  @param in_fences   An array of @param in_n_fences Fence instances to reset. Must not be nullptr,
         *                     unless @param in_n_fences is 0.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        static bool reset_fences(const uint32_t in_n_fences,
                                 Fence*         in_fences);

    private:
        /* Private functions */

        /** Constructor.
         *
         *  Please see documentation of create() for specification
         */
        Fence(Anvil::FenceCreateInfoUniquePtr in_create_info_ptr);

        Fence           (const Fence&);
        Fence& operator=(const Fence&);

        bool init         ();
        void release_fence();

        /* Private variables */
        Anvil::FenceCreateInfoUniquePtr                        m_create_info_ptr;
        std::map<Anvil::ExternalFenceHandleTypeFlagBits, bool> m_external_fence_created_for_handle_type;
        VkFence                                                m_fence;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_FENCE_H */
