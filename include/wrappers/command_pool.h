//
// Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef WRAPPERS_COMMAND_POOL_H
#define WRAPPERS_COMMAND_POOL_H

#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"


namespace Anvil
{
    /** Implements a command pool wrapper */
    class CommandPool : public MTSafetySupportProvider,
                        public DebugMarkerSupportProvider<CommandPool>
    {
    public:
        /* Public functions */

        /** Destroys the Vulkan object and unregisters the object from the Object Tracker. */
        virtual ~CommandPool();

        /** Allocates a new primary-level command buffer instance from this command pool.
         *
         *  When no longer needed, the returned instance should be released by the app.
         *
         *  @return As per description.
         **/
        Anvil::PrimaryCommandBufferUniquePtr alloc_primary_level_command_buffer();

        /** Allocates a new secondary-level command buffer instance from this command pool.
         *
         *  When no longer needed, the returned instance should be released by the app.
         *
         *  @return As per description.
         **/
        Anvil::SecondaryCommandBufferUniquePtr alloc_secondary_level_command_buffer();

        /** Creates a new CommandPool object.
         *
         *  @param in_device_ptr         Device to create the command pool for. Must not be nullptr.
         *  @param in_create_flags       Create flags to use.
         *  @param in_queue_family_index Index of the Vulkan queue family the command pool should be created for.
         *  @param in_mt_safe            Enable if your application is going to be calling any of the
         *                               alloc_*() functions from more than one thread at a time.
         **/
        static CommandPoolUniquePtr create(Anvil::BaseDevice*                   in_device_ptr,
                                           const Anvil::CommandPoolCreateFlags& in_create_flags,
                                           uint32_t                             in_queue_family_index,
                                           MTSafety                             in_mt_safety = MTSafety::INHERIT_FROM_PARENT_DEVICE);

        /** Retrieves the raw Vulkan handle for the encapsulated command pool */
        VkCommandPool get_command_pool() const
        {
            return m_command_pool;
        }

        /** Returns create flags specified at instantiaton time */
        const Anvil::CommandPoolCreateFlags& get_create_flags() const
        {
            return m_create_flags;
        }

        /** Tells which Vulkan queue family this command pool instance has been created for */
        uint32_t get_queue_family_index() const
        {
            return m_queue_family_index;
        }

        /** Reset the command pool.
         *
         *  @param in_release_resources true if the vkResetCommandPool() call should be invoked with
         *                              the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT flag.
         *
         *  @return true if successful, false otherwise.
         **/
        bool reset(bool in_release_resources);

        /* Trims the command buffer as per VK_KHR_maintenance1 extension spec.
         *
         * Requires VK_KHR_maintenance1 extension support. An assertion failure will occur if the
         * parent device has not been created with the extension enabled.
         */
        void trim();

    private:
        /* Private functions */

        /* Please seee create() documentation for more details */
        explicit CommandPool(Anvil::BaseDevice*                   in_device_ptr,
                             const Anvil::CommandPoolCreateFlags& in_create_flags,
                             uint32_t                             in_queue_family_index,
                             bool                                 in_mt_safe);

        CommandPool           (const CommandPool&);
        CommandPool& operator=(const CommandPool&);

        /* Private variables */
        VkCommandPool                 m_command_pool;
        Anvil::CommandPoolCreateFlags m_create_flags;
        Anvil::BaseDevice*            m_device_ptr;
        uint32_t                      m_queue_family_index;

        friend class Anvil::CommandBufferBase;
    };

}; /* namespace Anvil */
#endif /* WRAPPERS_COMMAND_POOL_H */