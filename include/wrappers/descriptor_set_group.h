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

#include "misc/types.h"
#include "wrappers/descriptor_set.h"

namespace Anvil
{
    class DescriptorSetGroup
    {
    public:
        /* Public functions */

        /** Destructor */
        virtual ~DescriptorSetGroup();

        /** Adds a new descriptor set binding to the DSG. This lets you attach one or more descriptors
         *  to the binding's individual array items by calling set_binding() later on.
         *
         *  This call invalidates internally-maintained Vulkan DS and DS layout instances.
         *
         *  @param in_n_set                      Index of the descriptor set the new binding should be created for.
         *                                       This number must not be equal to or larger than the number of sets
         *                                       specified for the DSG at creation time.
         *  @param in_binding                    Index of the binding to create. This index must not have been used earlier
         *                                       to create another binding.
         *  @param in_type                       Type of descriptor(s), which are going to be used to configure the binding.
         *  @param in_n_elements                 Binding array's size. Must be at least 1.
         *  @param in_shader_stages              A bitfield combination of shader stage bits, telling which shader stages
         *                                       this binding is going to be used for.
         *  @param in_opt_immutable_sampler_ptrs If not nullptr, an array of @param in_n_elements samplers should
         *                                       be passed. The binding will then be considered immutable, as per spec language.
         *                                       May be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         ***/
        bool add_binding(uint32_t                               in_n_set,
                         uint32_t                               in_binding,
                         VkDescriptorType                       in_type,
                         uint32_t                               in_n_elements,
                         VkShaderStageFlags                     in_shader_stages,
                         const std::shared_ptr<Anvil::Sampler>* in_opt_immutable_sampler_ptrs = nullptr);

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
         *  @param in_device_ptr       Device to use.
         *  @param in_releaseable_sets true if the created VkDescriptorSet instances should be releaseable
         *                             to the internal descriptor pool by invoking vkFreeDescriptorSets().
         *                             false otherwise.
         *  @param in_n_sets           Number of descriptor sets this instance should store information for.
         */
        static std::shared_ptr<DescriptorSetGroup> create(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                          bool                             in_releaseable_sets,
                                                          uint32_t                         in_n_sets);

        /** Creates a new DescriptorSetGroup instance.
         *
         *  By using this function, you explicitly state you'd like this DescriptorSetGroup instance
         *  to re-use layout of another DSG. This is useful if you'd like to re-use the same layout with
         *  a different combination of descriptor sets.
         *
         *  @param in_parent_dsg_ptr   Pointer to a DSG without a parent. Must not be nullptr.
         *  @param in_releaseable_sets See the documentation above for more details.
         **/
        static std::shared_ptr<DescriptorSetGroup> create(std::shared_ptr<DescriptorSetGroup> in_parent_dsg_ptr,
                                                          bool                                in_releaseable_sets);

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
        std::shared_ptr<Anvil::DescriptorSet> get_descriptor_set(uint32_t in_n_set);

        /** Returns a descriptor set binding index for a descriptor set at index @param in_n_set. */
        uint32_t get_descriptor_set_binding_index(uint32_t in_n_set) const;

        /** Retrieves a Vulkan instace of the descriptor set layout, as configured for the DSG instance's
         *  set at index @param in_n_set.
         *
         *  @param n_set As per description.
         *
         *  @return Requested Anvil::DescriptorSetLayout instance.
         */
        std::shared_ptr<Anvil::DescriptorSetLayout> get_descriptor_set_layout(uint32_t in_n_set);

        /** Returns the total number of added descriptor sets.
         *
         *  Bear in mind that descriptor set bindings may not form a continuous range set.
         *  For example, if this function returns 2, it doesn't mean that the defined DS bindings
         *  can only use indices 0, and 1. What binding indices are used specifically for descriptor
         *  sets at a given index can be checked by calling get_descriptor_set_binding_index() for
         *  a <n_set> value of <0, (value returned by this func - 1)>.
         **/
        uint32_t get_n_of_descriptor_sets() const
        {
            return (uint32_t) m_descriptor_sets.size();
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
                                     const BindingElementType* in_elements)
        {
            anvil_assert(m_descriptor_sets.find(in_n_set) != m_descriptor_sets.end() );

            if (m_descriptor_sets[in_n_set].descriptor_set_ptr == nullptr)
            {
                bake_descriptor_sets();

                anvil_assert(m_descriptor_sets[in_n_set].descriptor_set_ptr != nullptr);
            }

            m_descriptor_sets[in_n_set].descriptor_set_ptr->set_binding_array_items(in_binding_index,
                                                                                    in_element_range,
                                                                                    in_elements);

            return true;
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

        /** Configures how many overhead allocations should be requested from the descriptor pool.
         *
         *  @param in_descriptor_type        Descriptor type to increase the number of requested descriptor allocations for.
         *  @param in_n_overhead_allocations Value specifying how many additional allocations should be made. 0 by default.
         **/
        void set_descriptor_pool_overhead_allocations(VkDescriptorType in_descriptor_type,
                                                      uint32_t         in_n_overhead_allocations);

    private:
        /* Private type declarations */

        /* Encapsulates all info related to a single descriptor set */
        typedef struct DescriptorSetInfo
        {
            std::shared_ptr<Anvil::DescriptorSet>       descriptor_set_ptr;
            std::shared_ptr<Anvil::DescriptorSetLayout> layout_ptr;

            /* Dummy constructor */
            DescriptorSetInfo()
            {
                /* Stub */
            }

            /** Copy constructor.
             *
             *  Retains non-null descriptor set and layout wrapper instances.
             **/
            DescriptorSetInfo(const DescriptorSetInfo& in);

            /** Assignment operator.
             *
             *  Retains non-null descriptor set and layout wrapper instances.
             **/
            DescriptorSetInfo& operator=(const DescriptorSetInfo& in);

            /** Deinitializes a DescriptorSet instance by releasing the Vulkan descriptor set layout object,
             *  if it's not nullptr.
             *
             *  The function does not release the VkDescriptorSet instance it also holds. This object
             *  should be released by the DescriptorSetGroup destructor instead.
             */
            ~DescriptorSetInfo();
        } DescriptorSetInfo;

        /* Private functions */

        /** Please see create() documentation for more details. */
        DescriptorSetGroup(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                           bool                             in_releaseable_sets,
                           uint32_t                         in_n_sets);

        /** Please see create() documentation for more details. */
        DescriptorSetGroup(std::shared_ptr<DescriptorSetGroup> in_parent_dsg_ptr,
                           bool                                in_releaseable_sets);

        DescriptorSetGroup           (const DescriptorSetGroup&);
        DescriptorSetGroup& operator=(const DescriptorSetGroup&);

        void bake_descriptor_pool();
        bool bake_descriptor_sets();

        /* Private members */
        std::vector<std::shared_ptr<Anvil::DescriptorSet> >       m_cached_ds;
        std::vector<std::shared_ptr<Anvil::DescriptorSetLayout> > m_cached_ds_layouts;
        std::vector<VkDescriptorSet>                              m_cached_ds_vk;

        bool                                   m_descriptor_pool_dirty;
        std::shared_ptr<Anvil::DescriptorPool> m_descriptor_pool_ptr;
        std::map<uint32_t, DescriptorSetInfo>  m_descriptor_sets;
        std::weak_ptr<Anvil::BaseDevice>       m_device_ptr;
        uint32_t                               m_overhead_allocations[VK_DESCRIPTOR_TYPE_RANGE_SIZE];

        uint32_t                   m_n_instantiated_sets;
        uint32_t                   m_n_sets;
        bool                       m_releaseable_sets;

        bool                                       m_layout_modifications_blocked;
        std::shared_ptr<Anvil::DescriptorSetGroup> m_parent_dsg_ptr;
    };
} /* Vulkan namespace */

#endif /* WRAPPERS_DESCRIPTOR_SET_H */