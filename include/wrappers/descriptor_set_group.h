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

/** Descriptor Set Group is a wrapper construct, encapsulating Vulkan's Descriptor Sets, Descriptor Set Layouts and
 *  Descriptor Set Pools (to some extent).
 *
 *  The class can encapsulate one or more Descriptor Sets. The Descriptor Set Layout is specified by the caller by
 *  issuing one or more add_binding() calls. New bindings can be added at any time, but this will result in
 *  a destruction of previously constructed VkDescriptorSetLayout instances.
 *
 *  Objects or object arrays can then be configured by invoking set_binding(). Any of the 11 descriptor types,
 *  supported by Vulkan at the time of writing, can be used. Object bindings can be changed at any time, but such
 *  action will invalidate any previously returned VkDescriptorSet instances.
 *
 *  To retrieve a DescriptorSetLayout instance, corresponding to the specified configuration, call get_descriptor_set_layout().
 *  This call will return a VkDescriptorSetLayout instance you can use with the Vulkan API. Subsequent calls will
 *  return the same object, until a new binding is added or existing binding is deleted.
 *
 *  To retrieve a DescriptorSet for the specified set index, call get_descriptor_set(). This call will return a VkDescriptorSet
 *  instance you can use with the Vulkan API. If the layout has changed, or the descriptor pool has not been allocated earlier,
 *  this call will also instantiate a new internal VkDescriptorPool instance. Subsequent calls will return the same VkDescriptorSet
 *  instance, until one or more bindings are modified with a set_binding() call, or the descriptor set layout is modified.
 *
 *  Additionally, DescriptorSetGroup can be initialized with another instance of DescriptorSetGroup. This will cause the new
 *  instance to re-use parent's DS layouts. Non-orphaned DescriptorSetGroup instances will throw an assertion failure if any
 *  call that would have modified the layout is issued.
 *
 *  Each DescriptorSetGroup instance uses its own VkDescriptorPool instance.
 *
 *  DescriptorSetGroup instances are reference-counted.
 **/
#ifndef WRAPPERS_DESCRIPTOR_SET_GROUP_H
#define WRAPPERS_DESCRIPTOR_SET_GROUP_H

#include "misc/descriptor_set_create_info.h"
#include "misc/mt_safety.h"
#include "misc/types.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_layout.h"
#include <unordered_map>

namespace Anvil
{
    typedef struct OverheadAllocation
    {
        Anvil::DescriptorType descriptor_type;
        uint32_t              n_overhead_allocations;

        OverheadAllocation()
        {
            descriptor_type        = Anvil::DescriptorType::UNKNOWN;
            n_overhead_allocations = UINT32_MAX;
        }

        OverheadAllocation(Anvil::DescriptorType in_descriptor_type,
                           uint32_t              in_n_overhead_allocations)
        {
            descriptor_type        = in_descriptor_type;
            n_overhead_allocations = in_n_overhead_allocations;
        }
    } OverheadAllocation;

    class DescriptorSetGroup : public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Destructor */
        virtual ~DescriptorSetGroup();

        /** Creates a new DescriptorSetGroup instance.
         *
         *  Apart from the usual stuff, this function also preallocates memory for a number of
         *  helper arrays
         *
         *  By using this function, you're explicitly stating you'd like the instance to maintain
         *  its own copy of DescriptorSetLayout and DescriptorSet objects. Such object can then be used
         *  as a parent to other DescriptorSetGroup class instances, initialized with another constructor that
         *  takes a ptr to DescriptorSetGroup instance, causing objects created in such fashion to treat the
         *  specified DescriptorSetGroup instance as a parent.
         *
         *  @param in_device_ptr           Device to use.
         *  @param in_ds_create_info_ptrs  TODO.
         *  @param in_opt_pool_extra_flags Flags to include when creating a descriptor pool. Note that DSG may also specify
         *                                 other flags not included in this set, too.
         */
        static Anvil::DescriptorSetGroupUniquePtr create(const Anvil::BaseDevice*                              in_device_ptr,
                                                         std::vector<Anvil::DescriptorSetCreateInfoUniquePtr>& in_ds_create_info_ptrs,
                                                         bool                                                  in_releaseable_sets,
                                                         MTSafety                                              in_mt_safety                = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE,
                                                         const std::vector<OverheadAllocation>&                in_opt_overhead_allocations = std::vector<OverheadAllocation>(),
                                                         const Anvil::DescriptorPoolCreateFlags&               in_opt_pool_extra_flags     = Anvil::DescriptorPoolCreateFlagBits::NONE);

        /** Creates a new DescriptorSetGroup instance.
         *
         *  By using this function, you explicitly state you'd like this DescriptorSetGroup instance
         *  to re-use layout of another DSG. This is useful if you'd like to re-use the same layout with
         *  a different combination of descriptor sets.
         *
         *  @param in_parent_dsg_ptr   Pointer to a DSG without a parent. Must not be nullptr.
         *  @param in_releaseable_sets See the documentation above for more details.
         **/
        static DescriptorSetGroupUniquePtr create(const DescriptorSetGroup* in_parent_dsg_ptr,
                                                  bool                      in_releaseable_sets);

        /** Retrieves a Vulkan instance of the descriptor set, as configured for the DSG instance's set
         *  at index @param in_n_set.
         *
         *  This function may re-create internal Vulkan DS and DS layout instances if the DSG's configuration
         *  has been altered since the last time a get_() call has been made.
         *
         *  @param in_n_set As per description.
         *
         *  @return Pointer to the requested Anvil::DescriptorSet instance.
         **/
        Anvil::DescriptorSet* get_descriptor_set(uint32_t in_n_set);

        const std::vector<const Anvil::DescriptorSetCreateInfo*>* get_descriptor_set_create_info() const;
        const Anvil::DescriptorSetCreateInfo*                     get_descriptor_set_create_info(uint32_t in_n_set) const;

        /** Retrieves a Vulkan instace of the descriptor set layout, as configured for the DSG instance's
         *  set at index @param in_n_set.
         *
         *  @param n_set As per description.
         *
         *  @return Requested Anvil::DescriptorSetLayout instance.
         */
        Anvil::DescriptorSetLayout* get_descriptor_set_layout(uint32_t in_n_set) const;

        /** Returns the total number of added descriptor sets.
         *
         *  NOTE: Descriptor set bindings need not form a continuous range set. For instance, even if
         *        this function returns 3, get_descriptor_set() may return nullptr for set at index 1,
         *        if no layout info has been provided for this set index at creation time.
         *
         **/
        uint32_t get_n_descriptor_sets() const
        {
            return static_cast<uint32_t>(m_ds_create_info_ptrs.size() );
        }

        /** This function should be set to assign physical Vulkan objects to a descriptor binding
         *  at index @param in_binding_index for descriptor set @param in_n_set.
         *  Each binding can hold one or more objects. Which slots the specified objects should take can
         *  be configured by passing the right values to @param in_element_range.
         *  Objects are passed via @param in_elements argument. The argument must be one of the following
         *  types, depending on what object is to be attached to the specified descriptor binding:
         *
         *  CombinedImageSamplerBindingElement - for combined image+sampler bindings.
         *  DynamicStorageBufferBindingElement - for dynamic storage buffer bindings.
         *  DynamicUniformBufferBindingElement - for dynamic uniform buffer bindings.
         *  InputAttachmentBindingElement      - for input attachment bindings.
         *  SampledImageBindingElement         - for sampled image bindings.
         *  SamplerBindingElement              - for sampler bindings.
         *  StorageBufferBindingElement        - for storage buffer bindings.
         *  StorageImageBindingElement         - for storage image bindings.
         *  StorageTexelBufferBindingElement   - for storage texel buffer bindings. 
         *  UniformBufferBindingElement        - for uniform buffer bindings.
         *  UniformTexelBufferBindingElement   - for storage uniform buffer bindings.
         *
         *  @param in_n_set         As per documentation. Must not be equal to or larger than the number
         *                          of sets, specified for the DSG.
         *  @param in_binding_index As per documentation. Must correspond to a binding which has earlier
         *                          been added by calling add_binding() function.
         *  @param in_element_range As per documentation. Must not be equal to or larger than the array size,
         *                          specified when calling add_binding() function.
         *  @param in_elements      As per documentation. Must not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        template<typename BindingElementType>
        bool set_binding_array_items(uint32_t                  in_n_set,
                                     BindingIndex              in_binding_index,
                                     BindingElementArrayRange  in_element_range,
                                     const BindingElementType* in_elements_ptr)
        {
            anvil_assert(m_descriptor_sets.find(in_n_set) != m_descriptor_sets.end() );

            if (m_descriptor_sets[in_n_set]->descriptor_set_ptr == nullptr)
            {
                bake_descriptor_sets();

                anvil_assert(m_descriptor_sets[in_n_set]->descriptor_set_ptr != nullptr);
            }

            m_descriptor_sets[in_n_set]->descriptor_set_ptr->set_binding_array_items(in_binding_index,
                                                                                     in_element_range,
                                                                                     in_elements_ptr);

            return true;
        }

        template<typename BindingElementType>
        bool set_binding_array_items(uint32_t                         in_n_set,
                                     BindingIndex                     in_binding_index,
                                     BindingElementArrayRange         in_element_range,
                                     const BindingElementType* const* in_elements_ptr_ptr)
        {
            anvil_assert(m_descriptor_sets.find(in_n_set) != m_descriptor_sets.end() );

            if (m_descriptor_sets[in_n_set]->descriptor_set_ptr == nullptr)
            {
                bake_descriptor_sets();

                anvil_assert(m_descriptor_sets[in_n_set]->descriptor_set_ptr != nullptr);
            }

            m_descriptor_sets[in_n_set]->descriptor_set_ptr->set_binding_array_items(in_binding_index,
                                                                                     in_element_range,
                                                                                     in_elements_ptr_ptr);

            return true;
        }

        /** TODO
         *
         *  NOTE: Do NOT schedule multiple updates for overlapping inline uniform block memory regions without a bake operation in-between. Ignoring
         *        this requirement results in undefined behavior.
         *
         *  Requires VK_EXT_inline_uniform_block.
         *
         *  @param in_binding_index         Index of the inline uniform block binding to use for the update.
         *  @param in_start_offset          Start offset of the inline uniform block's memory region which should be updated. Must be a mul of 4.
         *  @param in_size                  Size of the inline uniform block's memory region to update. Must be a mul of 4.
         *  @param in_raw_data_ptr          Data to use for the update. Whether or not the data is cached internally depends on value passed to
         *                                  @param in_should_cache_raw_data. Must not be nullptr.
         *  @param in_should_cache_raw_data True if data provided via @param in_raw_data_ptr should be cached internally, false otherwise. In other
         *                                  words, if this argument is set to true, it is safe to release memory block, to which @param in_raw_data_ptr points.
         *
         *  @return true if successful, false otherwise.
         */
        bool set_inline_uniform_block_binding_data(const uint32_t&     in_n_set,
                                                   const BindingIndex& in_binding_index,
                                                   const uint32_t&     in_start_offset,
                                                   const uint32_t&     in_size,
                                                   const void*         in_raw_data_ptr,
                                                   const bool&         in_should_cache_raw_data)
        {
            anvil_assert(m_descriptor_sets.find(in_n_set) != m_descriptor_sets.end() );

            if (m_descriptor_sets[in_n_set]->descriptor_set_ptr == nullptr)
            {
                bake_descriptor_sets();

                anvil_assert(m_descriptor_sets[in_n_set]->descriptor_set_ptr != nullptr);
            }

            return m_descriptor_sets[in_n_set]->descriptor_set_ptr->set_inline_uniform_block_binding_data(in_binding_index,
                                                                                                          in_start_offset,
                                                                                                          in_size,
                                                                                                          in_raw_data_ptr,
                                                                                                          in_should_cache_raw_data);
        }

        /** This function works exactly like set_bindings(), except that it always replaces the zeroth element
         *  attached to the specified descriptor set's binding.
         */
        template<typename BindingElementType>
        bool set_binding_item(uint32_t                  in_n_set,
                              BindingIndex              in_binding_index,
                              const BindingElementType& in_element)
        {
            return set_binding_array_items(in_n_set,
                                           in_binding_index,
                                           BindingElementArrayRange(0,  /* StartBindingElementIndex */
                                                                    1), /* NumberOfBindingElements  */
                                          &in_element);
        }

    private:
        /* Private type declarations */

        /* Encapsulates all info related to a single descriptor set */
        typedef struct DescriptorSetInfoContainer
        {
            Anvil::DescriptorSetUniquePtr       descriptor_set_ptr;
            Anvil::DescriptorSetLayoutUniquePtr layout_ptr;

            /* Dummy constructor */
            DescriptorSetInfoContainer()
            {
                /* Stub */
            }

            /** Deinitializes a DescriptorSet instance by releasing the Vulkan descriptor set layout object,
             *  if it's not nullptr.
             *
             *  The function does not release the VkDescriptorSet instance it also holds. This object
             *  should be released by the DescriptorSetGroup destructor instead.
             */
            ~DescriptorSetInfoContainer();
        } DescriptorSetInfoContainer;

        typedef struct DescriptorTypeProperties
        {
            uint32_t n_overhead_allocations;
            uint32_t pool_size;

            DescriptorTypeProperties()
                :n_overhead_allocations(0),
                 pool_size             (0)
            {
                /* Stub */
            }
        } DescriptorTypeProperties;

        /* Private functions */

        /** Please see create() documentation for more details. */
        DescriptorSetGroup(const Anvil::BaseDevice*                      in_device_ptr,
                           std::vector<DescriptorSetCreateInfoUniquePtr> in_ds_create_info_ptrs,
                           bool                                          in_releaseable_sets,
                           MTSafety                                      in_mt_safety                = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE,
                           const std::vector<OverheadAllocation>&        in_opt_overhead_allocations = std::vector<OverheadAllocation>(),
                           const Anvil::DescriptorPoolCreateFlags&       in_opt_pool_extra_flags     = Anvil::DescriptorPoolCreateFlagBits::NONE);

        /** Please see create() documentation for more details. */
        DescriptorSetGroup(const DescriptorSetGroup* in_parent_dsg_ptr,
                           bool                      in_releaseable_sets);

        bool bake_descriptor_pool();
        bool bake_descriptor_sets();

        /* Private members */
        mutable DescriptorPoolUniquePtr                                          m_descriptor_pool_ptr;
        mutable std::map<uint32_t, std::unique_ptr<DescriptorSetInfoContainer> > m_descriptor_sets;
        const Anvil::BaseDevice*                                                 m_device_ptr;
        std::vector<const Anvil::DescriptorSetCreateInfo*>                       m_ds_create_info_ptrs;

        std::unordered_map<Anvil::DescriptorType, DescriptorTypeProperties, EnumClassHasher<Anvil::DescriptorType> > m_descriptor_type_properties;

        uint32_t                               m_n_unique_dses;
        const Anvil::DescriptorSetGroup*       m_parent_dsg_ptr;
        bool                                   m_releaseable_sets;
        const Anvil::DescriptorPoolCreateFlags m_user_specified_pool_flags;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(DescriptorSetGroup);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(DescriptorSetGroup);
    };
} /* Vulkan namespace */

#endif /* WRAPPERS_DESCRIPTOR_SET_H */