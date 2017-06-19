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

/** Defines a command pool wrapper class which simplify the following tasks:
 *
 *  - Primary- and second-level command buffer allocation & resetting.
 *  - State caching
 *
 **/
#ifndef WRAPPERS_COMMAND_POOL_H
#define WRAPPERS_COMMAND_POOL_H

#include "misc/debug_marker.h"
#include "misc/types.h"


namespace Anvil
{
    /** Implements a command pool wrapper */
    class CommandPool : public DebugMarkerSupportProvider<CommandPool>,
                        public std::enable_shared_from_this<CommandPool>
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
        std::shared_ptr<Anvil::PrimaryCommandBuffer> alloc_primary_level_command_buffer();

        /** Allocates a new secondary-level command buffer instance from this command pool.
         *
         *  When no longer needed, the returned instance should be released by the app.
         *
         *  @return As per description.
         **/
        std::shared_ptr<Anvil::SecondaryCommandBuffer> alloc_secondary_level_command_buffer();

        /** Creates a new CommandPool object.
         *
         *  @param in_device_ptr                     Device to create the command pool for. Must not be nullptr.
         *  @param in_transient_allocations_friendly Set to true if the command pool should be created with the
         *                                           VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag set on.
         *  @param in_support_per_cmdbuf_reset_ops   Set to true if the command pool should be created with the
         *                                           VK_COMMAND_POOL_RESET_COMMAND_BUFFER_BIT flag set on.
         *  @param in_queue_family                   Index of the queue family the command pool should be created for.
         **/
        static std::shared_ptr<CommandPool> create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                   bool                             in_transient_allocations_friendly,
                                                   bool                             in_support_per_cmdbuf_reset_ops,
                                                   Anvil::QueueFamilyType           in_queue_family);

        /** Retrieves the raw Vulkan handle for the encapsulated command pool */
        VkCommandPool get_command_pool() const
        {
            return m_command_pool;
        }

        /** Tells what queue family this command pool instance has been created for */
        Anvil::QueueFamilyType get_queue_family_type() const
        {
            return m_queue_family;
        }

        /** Tells whether the command pool has been created with VK_COMMAND_POOL_CREATE_TRANSIENT_BIT 
         *  flag defined.
         ***/
        bool is_transient_allocations_friendly() const
        {
            return m_is_transient_allocations_friendly;
        }

        /** Reset the command pool.
         *
         *  @param in_release_resources true if the vkResetCommandPool() call should be invoked with
         *                              the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT flag.
         *
         *  @return true if successful, false otherwise.
         **/
        bool reset(bool in_release_resources);

        /** Tells whether the command buffers, allocated from this command pool, support reset operations */
        bool supports_per_cmdbuf_reset_ops() const
        {
            return m_supports_per_cmdbuf_reset_ops;
        }

        /* Trims the command buffer as per VK_KHR_maintenance1 extension spec.
         *
         * Requires VK_KHR_maintenance1 extension support. An assertion failure will occur if the
         * parent device has not been created with the extension enabled.
         */
        void trim();

    private:
        /* Private functions */

        /* Please seee create() documentation for more details */
        explicit CommandPool(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                             bool                             in_transient_allocations_friendly,
                             bool                             in_support_per_cmdbuf_reset_ops,
                             Anvil::QueueFamilyType           in_queue_family);

        CommandPool           (const CommandPool&);
        CommandPool& operator=(const CommandPool&);

        /* Private variables */
        VkCommandPool                    m_command_pool;
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        bool                             m_is_transient_allocations_friendly;
        Anvil::QueueFamilyType           m_queue_family;
        bool                             m_supports_per_cmdbuf_reset_ops;

        friend class Anvil::CommandBufferBase;
    };

}; /* namespace Anvil */
#endif /* WRAPPERS_COMMAND_POOL_H */