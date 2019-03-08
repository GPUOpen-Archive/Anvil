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

/** Defines a descriptor pool wrapper class which simplifies the following processes:
 *
 *  - Life-time management of descriptor pools.
 *  - Encapsulation of all state related to descriptor pools.
 *  - Automatic re-baking of descriptor pools, whenever needed.
 **/
#ifndef WRAPPERS_DESCRIPTOR_POOL_H
#define WRAPPERS_DESCRIPTOR_POOL_H

#include "misc/callbacks.h"
#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    enum
    {
        /* Notification sent out whenever pool is reset.
         *
         * Callback arg: Pointer to OnDescriptorPoolResetCallbackArgument instance.
         */
        DESCRIPTOR_POOL_CALLBACK_ID_POOL_RESET,

        /* Always last */
        DESCRIPTOR_POOL_CALLBACK_ID_COUNT
    };

    class DescriptorPool : public CallbacksSupportProvider,
                           public DebugMarkerSupportProvider<DescriptorPool>,
                           public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Creates a new DescriptorPool instance. Sets up the wrapper, but does not immediately
         *  bake a new Vulkan pool.
         *
         **/
        static DescriptorPoolUniquePtr create(Anvil::DescriptorPoolCreateInfoUniquePtr in_create_info_ptr);

        /** Destructor. Releases the Vulkan pool object if instantiated. */
        virtual ~DescriptorPool();

        /** Allocates user-specified number of descriptors sets with user-defined layouts.
         *
         *  @param in_n_sets                     Number of sets to allocate.
         *  @param in_descriptor_set_layouts_ptr Pointer to an array of Vulkan DS layouts to use for the call.
         *                                       Must not be nullptr.
         *  @param out_descriptor_sets_ptr       Deref will be set to @param in_n_sets Anvil::DescriptorSet instances.
         *                                       Must not be nullptr.
         *  @param out_descriptor_sets_ptr       Deref will be set to DescriptorSet instances, wrapping Vulkan handles
         *                                       obtained by allocating the descriptor sets from internally managed pool.
         *                                       Must not be nullptr.
         *  @param out_descriptor_sets_vk_ptr    Deref will be set to raw Vulkan handles, created by allocating DSes
         *                                       from a pool. Must not be nullptr. 
         *  @param out_opt_result_ptr            If not null, deref will be set to the VkResult value, as returned by
         *                                       the vkAllocateDescriptorSets() invocation that this function makes.
         *                                       This may be useful for KHR_maintenance1-aware applications.
         *  @return true if successful, false otherwise.
         **/
        bool alloc_descriptor_sets(uint32_t                       in_n_sets,
                                   const DescriptorSetAllocation* in_ds_allocations_ptr,
                                   DescriptorSetUniquePtr*        out_descriptor_sets_ptr,
                                   VkResult*                      out_opt_result_ptr = nullptr);

        bool alloc_descriptor_sets(uint32_t                       in_n_sets,
                                   const DescriptorSetAllocation* in_ds_allocations_ptr,
                                   VkDescriptorSet*               out_descriptor_sets_vk_ptr,
                                   VkResult*                      out_opt_result_ptr = nullptr);

        const Anvil::DescriptorPoolCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        /** Returns a raw Vulkan handle for the pool.
         *
         *  This function may re-bake the pool object, if necessary.
         *
         *  @return As per description.
         **/
        const VkDescriptorPool& get_descriptor_pool()
        {
            return m_pool;
        }

        /** Resets the pool.
         *
         *  @return true if successful, false otherwise
         **/
        bool reset();

    private:
        /* Private functions */

        bool init();

        /** Constructor */
        DescriptorPool(Anvil::DescriptorPoolCreateInfoUniquePtr in_create_info_ptr,
                       bool                                     in_mt_safe);

        DescriptorPool           (const DescriptorPool&);
        DescriptorPool& operator=(const DescriptorPool&);

        /* Private variables */
        Anvil::DescriptorPoolCreateInfoUniquePtr m_create_info_ptr;
        std::vector<VkDescriptorSet>             m_ds_cache;
        std::vector<VkDescriptorSetLayout>       m_ds_layout_cache;
        VkDescriptorPool                         m_pool;
    };

}; /* namespace Anvil */

#endif /* WRAPPERS_DESCRIPTOR_POOL_H */