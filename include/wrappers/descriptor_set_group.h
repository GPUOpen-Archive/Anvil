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

#include "misc/ref_counter.h"
#include "misc/types.h"
#include "wrappers/descriptor_set.h"

namespace Anvil
{
    class DescriptorSetGroup : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor for the DescriptorSetGroup class.
         *
         *  Apart from the usual stuff, this function also preallocates memory for a number of
         *  helper arrays
         *
         *  By using this constructor, you're explicitly stating you'd like the instance to maintain
         *  its own copy of DescriptorSetLayout and DescriptorSet objects. Such object can then be used
         *  as a parent to other DescriptorSetGroup class instances, initialized with another constructor that
         *  takes a ptr to DescriptorSetGroup instance, causing objects created in such fashion to treat the
         *  specified DescriptorSetGroup instance as a parent.
         *
         *  @param device_ptr       Device to use.
         *  @param releaseable_sets true if the created VkDescriptorSet instances should be releaseable
         *                          to the internal descriptor pool by invoking vkFreeDescriptorSets().
         *                          false otherwise.
         *  @param n_sets           Number of descriptor sets this instance should store information for.
         */
        DescriptorSetGroup(Anvil::Device* device_ptr,
                           bool           releaseable_sets,
                           uint32_t       n_sets);

        /** Constructor for the DescriptorSetGroup class.
         *
         *  By using this constructor, you explicitly state you'd like this DescriptorSetGroup instance
         *  to re-use layout of another DSG. This is useful if you'd like to re-use the same layout with
         *  a different combination of descriptor sets.
         *
         *  @param parent_dsg_ptr   Pointer to a DSG without a parent. Must not be nullptr.
         *  @param releaseable_sets See the documentation above for more details.
         **/
        DescriptorSetGroup(DescriptorSetGroup*  parent_dsg_ptr,
                           bool                 releaseable_sets);

        /** Adds a new descriptor set binding to the DSG. This lets you attach one or more descriptors
         *  to the binding's individual array items by calling set_binding() later on.
         *
         *  This call invalidates internally-maintained Vulkan DS and DS layout instances.
         *
         *  @param n_set         Index of the descriptor set the new binding should be created for.
         *                       This number must not be equal to or larger than the number of sets
         *                       specified for the DSG at creation time.
         *  @param binding       Index of the binding to create. This index must not have been used earlier
         *                       to create another binding.
         *  @param type          Type of descriptor(s), which are going to be used to configure the binding.
         *  @param n_elements    Binding array's size. Must be at least 1.
         *  @param shader_stages A bitfield combination of shader stage bits, telling which shader stages
         *                       this binding is going to be used for.
         *
         *  @return true if the function executed successfully, false otherwise.
         ***/
        bool add_binding(uint32_t           n_set,
                         uint32_t           binding,
                         VkDescriptorType   type,
                         uint32_t           n_elements,
                         VkShaderStageFlags shader_stages);

        /** Retrieves a Vulkan instance of the descriptor set, as configured for the DSG instance's set
         *  at index @param n_set.
         *
         *  This function may re-create internal Vulkan DS and DS layout instances if the DSG's configuration
         *  has been altered since the last time a get_() call has been made.
         *
         *  @param n_set As per description.
         *
         *  @return Pointer to the requested Anvil::DescriptorSet instance.
         **/
        Anvil::DescriptorSet* get_descriptor_set(uint32_t n_set);

        /** Returns a descriptor set binding index for a descriptor set at index @param n_set. */
        uint32_t get_descriptor_set_binding_index(uint32_t n_set) const;

        /** Retrieves a Vulkan instace of the descriptor set layout, as configured for the DSG instance's
         *  set at index @param n_set.
         *
         *  @param n_set As per description.
         *
         *  @return Requested Anvil::DescriptorSetLayout instance.
         */
        Anvil::DescriptorSetLayout* get_descriptor_set_layout(uint32_t n_set);

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
         *  at index @param binding_index for descriptor set @param n_set.
         *  Each binding can hold one or more objects. Which slots the specified objects should take can
         *  be configured by passing the right values to @param element_range.
         *  Objects are passed via @param elements argument. The argument must be one of the following
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
         *  @param n_set         As per documentation. Must not be equal to or larger than the number
         *                       of sets, specified for the DSG.
         *  @param binding_index As per documentation. Must correspond to a binding which has earlier
         *                       been added by calling add_binding() function.
         *  @param element_range As per documentation. Must not be equal to or larger than the array size,
         *                       specified when calling add_binding() function.
         *  @param elements      As per documentation. Must not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        template<typename BindingElementType>
        bool set_binding_array_items(uint32_t                  n_set,
                                     BindingIndex              binding_index,
                                     BindingElementArrayRange  element_range,
                                     const BindingElementType* elements)
        {
            anvil_assert(m_descriptor_sets.find(n_set) != m_descriptor_sets.end() );

            if (m_descriptor_sets[n_set].descriptor_set_ptr == nullptr)
            {
                bake_descriptor_sets();

                anvil_assert(m_descriptor_sets[n_set].descriptor_set_ptr != nullptr);
            }

            m_descriptor_sets[n_set].descriptor_set_ptr->set_binding_array_items(binding_index,
                                                                                 element_range,
                                                                                 elements);

            return true;
        }

        /** This function works exactly like set_bindings(), except that it always replaces the zeroth element
         *  attached to the specified descriptor set's binding.
         */
        template<typename BindingElementType>
        bool set_binding_item(uint32_t                  n_set,
                              BindingIndex              binding_index,
                              const BindingElementType& element)
        {
            return set_binding_array_items(n_set,
                                           binding_index,
                                           BindingElementArrayRange(0,  /* StartBindingElementIndex */
                                                                    1), /* NumberOfBindingElements  */
                                          &element);
        }

        /** Configures how many overhead allocations should be requested from the descriptor pool.
         *
         *  @param descriptor_type        Descriptor type to increase the number of requested descriptor allocations for.
         *  @param n_overhead_allocations Value specifying how many additional allocations should be made. 0 by default.
         **/
        void set_descriptor_pool_overhead_allocations(VkDescriptorType descriptor_type,
                                                      uint32_t         n_overhead_allocations);

    private:
        /* Private type declarations */

        /* Encapsulates all info related to a single descriptor set */
        typedef struct DescriptorSetInfo
        {
            Anvil::DescriptorSet*       descriptor_set_ptr;
            Anvil::DescriptorSetLayout* layout_ptr;

            /* Dummy constructor */
            DescriptorSetInfo()
            {
                descriptor_set_ptr = nullptr;
                layout_ptr         = nullptr;
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
        DescriptorSetGroup           (const DescriptorSetGroup&);
        DescriptorSetGroup& operator=(const DescriptorSetGroup&);

        virtual ~DescriptorSetGroup();

        void bake_descriptor_pool();
        bool bake_descriptor_sets();

        /* Private members */
        std::vector<Anvil::DescriptorSet*>       m_cached_ds;
        std::vector<Anvil::DescriptorSetLayout*> m_cached_ds_layouts;
        std::vector<VkDescriptorSet>             m_cached_ds_vk;
        std::vector<Anvil::Sampler*>             m_cached_immutable_samplers;

        bool                                  m_descriptor_pool_dirty;
        Anvil::DescriptorPool*                m_descriptor_pool_ptr;
        std::map<uint32_t, DescriptorSetInfo> m_descriptor_sets;
        Anvil::Device*                        m_device_ptr;
        uint32_t                              m_overhead_allocations[VK_DESCRIPTOR_TYPE_RANGE_SIZE];

        uint32_t                   m_n_instantiated_sets;
        uint32_t                   m_n_sets;
        bool                       m_releaseable_sets;

        bool                       m_layout_modifications_blocked;
        Anvil::DescriptorSetGroup* m_parent_dsg_ptr;
    };

    /** Delete functor. Useful if you need to wrap the DSG instance in an auto pointer */
    struct DescriptorSetGroupDeleter
    {
        void operator()(DescriptorSetGroup* dsg_ptr) const
        {
            dsg_ptr->release();
        }
    };
} /* Vulkan namespace */

#endif /* WRAPPERS_DESCRIPTOR_SET_H */