//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
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

#include "misc/ref_counter.h"
#include "misc/types.h"


namespace Anvil
{
    /** Implements a command pool wrapper */
    class CommandPool : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  @param device_ptr                     Device to create the command pool for. Must not be nullptr.
         *  @param transient_allocations_friendly Set to true if the command pool should be created with the
         *                                        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag set on.
         *  @param support_per_cmdbuf_reset_ops   Set to true if the command pool should be created with the
         *                                        VK_COMMAND_POOL_RESET_COMMAND_BUFFER_BIT flag set on.
         *  @param queue_family                   Index of the queue family the command pool should be created for.
         **/
        explicit CommandPool(Anvil::Device*         device_ptr,
                             bool                   transient_allocations_friendly,
                             bool                   support_per_cmdbuf_reset_ops,
                             Anvil::QueueFamilyType queue_family);

        /** Allocates a new primary-level command buffer instance from this command pool.
         *
         *  When no longer needed, the returned instance should be released by the app.
         *
         *  @return As per description.
         **/
        Anvil::PrimaryCommandBuffer* alloc_primary_level_command_buffer();

        /** Allocates a new secondary-level command buffer instance from this command pool.
         *
         *  When no longer needed, the returned instance should be released by the app.
         *
         *  @return As per description.
         **/
        Anvil::SecondaryCommandBuffer* alloc_secondary_level_command_buffer();

        /** Retrieves the raw Vulkan handle for the encapsulated command pool */
        VkCommandPool get_command_pool() const
        {
            return m_command_pool;
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
         *  @param release_resources true if the vkResetCommandPool() call should be invoked with
         *                           the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT flag.
         *
         *  @return true if successful, false otherwise.
         **/
        bool reset(bool release_resources);

        /** Tells whether the command buffers, allocated from this command pool, support reset operations */
        bool supports_per_cmdbuf_reset_ops() const
        {
            return m_supports_per_cmdbuf_reset_ops;
        }

    private:
        /* Private functions */
        CommandPool           (const CommandPool&);
        CommandPool& operator=(const CommandPool&);

        void on_command_buffer_wrapper_destroyed(CommandBufferBase* command_buffer_ptr);

        ~CommandPool();

        /* Private variables */
        std::vector<CommandBufferBase*> m_allocs_in_flight;

        VkCommandPool  m_command_pool;
        Anvil::Device* m_device_ptr;
        bool           m_is_transient_allocations_friendly;
        bool           m_supports_per_cmdbuf_reset_ops;

        friend class Anvil::CommandBufferBase;
    };

}; /* namespace Anvil */
#endif /* WRAPPERS_COMMAND_POOL_H */