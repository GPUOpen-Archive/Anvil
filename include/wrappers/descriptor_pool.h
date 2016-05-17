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

/** Defines a descriptor pool wrapper class which simplifies the following processes:
 *
 *  - Life-time management of descriptor pools.
 *  - Encapsulation of all state related to descriptor pools.
 *  - Automatic re-baking of descriptor pools, whenever needed.
 **/
#ifndef WRAPPERS_DESCRIPTOR_POOL_H
#define WRAPPERS_DESCRIPTOR_POOL_H

#include "misc/callbacks.h"
#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
    enum
    {
        /* Notification sent out whenever pool is reset.
         *
         * Callback arg: originating DescriptorPool instance
         */
        DESCRIPTOR_POOL_CALLBACK_ID_POOL_RESET,

        /* Always last */
        DESCRIPTOR_POOL_CALLBACK_ID_COUNT
    };

    class DescriptorPool : public CallbacksSupportProvider,
                           public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor. Sets up the wrapper, but does not bake a new Vulkan pool.
         *
         *  @param device_ptr       Device to use.
         *  @param n_max_sets       Maximum number of sets to be allocable from the pool. Must be at
         *                          least 1.
         *  @param releaseable_sets true if the sets should be releaseable with vkFreeDescriptorSet()
         *                          calls. false otherwise.
         **/
        DescriptorPool(Anvil::Device* device_ptr,
                       uint32_t       n_max_sets,
                       bool           releaseable_sets);

        /** Destructor. Releases the Vulkan pool object if instantiated. */
        virtual ~DescriptorPool();

        /** Allocates user-specified number of descriptors sets with user-defined layouts.
         *
         *  @param n_sets                     Number of sets to allocate.
         *  @param descriptor_set_layouts_ptr Pointer to an array of Vulkan DS layouts to use for the call.
         *                                    Must not be nullptr.
         *  @param out_descriptor_sets_ptr    Deref will be set to @param n_sets Anvil::DescriptorSet instances.
         *                                    Must not be nullptr.
         *  @param out_descriptor_sets_ptr    Deref will be set to DescriptorSet instances, wrapping Vulkan handles
         *                                    obtained by allocating the descriptor sets from internally managed pool.
         *                                    Must not be nullptr.
         *  @param out_descriptor_sets_vk_ptr Deref will be set to raw Vulkan handles, created by allocating DSes
         *                                    from a pool. Must not be nullptr. 
         *
         *  @return true if successful, false otherwise.
         **/
        bool alloc_descriptor_sets(uint32_t                     n_sets,
                                   Anvil::DescriptorSetLayout** descriptor_set_layouts_ptr,
                                   Anvil::DescriptorSet**       out_descriptor_sets_ptr);

        bool alloc_descriptor_sets(uint32_t                     n_sets,
                                   Anvil::DescriptorSetLayout** descriptor_set_layouts_ptr,
                                   VkDescriptorSet*             out_descriptor_sets_vk_ptr);

        /** Tells if the pool allocated from the pool can be freed with vkFreeDescriptorSet() call. */
        bool are_sets_releaseable() const
        {
            return m_releaseable_sets;
        }

        /** Releases previously baked Vulkan pool handle and instantiates a new Vulkan object, according
         *  to how the wrapper has been configured.
         **/
        void bake();

        /** Returns a raw Vulkan handle for the pool.
         *
         *  This function may re-bake the pool object, if necessary.
         *
         *  @return As per description.
         **/
        const VkDescriptorPool& get_descriptor_pool()
        {
            if (!m_baked)
            {
                bake();
            }

            return m_pool;
        }

        /** Resets the pool.
         *
         *  @return true if successful, false otherwise
         **/
        bool reset();

        /** Configures how many descriptors of user-specified type should be allocable for a single
         *  set, that's going to be allocated from the pool.
         *
         *  This function may mark the pool as dirty, meaning it will be re-baked at the next
         *  get_descriptor_pool() call time.
         *
         *  @param descriptor_type Type of the descriptor to adjust the number of slots.
         *  @param n_slots_in_pool Number of descriptors of the specified type to be allocable for
         *                         a single set.
         *
         **/
        void set_descriptor_array_size(VkDescriptorType descriptor_type,
                                       uint32_t         n_slots_in_pool)
        {
            anvil_assert(descriptor_type < VK_DESCRIPTOR_TYPE_RANGE_SIZE);

            if (m_descriptor_count[descriptor_type] != n_slots_in_pool)
            {
                m_baked                             = false;
                m_descriptor_count[descriptor_type] = n_slots_in_pool;
            }
        }

        /** Updates the maximum number of sets which can be allocated from the pool.
         *
         *  This function may mark the pool as dirty, meaning it will be re-baked at the next
         *  get_descriptor_pool() call time.
         *
         *  @param n_maximum_sets New maximum number of allocable sets.
         **/
        void set_n_maximum_sets(uint32_t n_maximum_sets)
        {
            anvil_assert(n_maximum_sets != 0);

            if (m_n_max_sets != n_maximum_sets)
            {
                m_baked      = false;
                m_n_max_sets = n_maximum_sets;
            }
        }
    private:
        /* Private functions */
        DescriptorPool           (const DescriptorPool&);
        DescriptorPool& operator=(const DescriptorPool&);

        /* Private variables */
        bool             m_baked;
        Anvil::Device*   m_device_ptr;
        VkDescriptorPool m_pool;

        uint32_t m_descriptor_count[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
        uint32_t m_n_max_sets;

        /* The instances stored in this vector are owned by alloc() callers - do not release
         * unless Vulkan object goes out of scope.
         */
        std::vector<Anvil::DescriptorSet* > m_alloced_dses;
        std::vector<VkDescriptorSet>        m_ds_cache;
        std::vector<VkDescriptorSetLayout>  m_ds_layout_cache;

        const bool m_releaseable_sets;
    };

}; /* namespace Anvil */

#endif /* WRAPPERS_DESCRIPTOR_POOL_H */