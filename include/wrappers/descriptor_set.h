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

/** Descriptor Set wrapper implementation.
 *
 *  Implemented to:
 *
 *  - reference-count wrapper instances
 *  - cache set binding information.
 *  - monitor layout adjustments and act accordingly.
 *  - monitor pool reset events and act accordingly.
 *
 *  Not thread-safe at the moment.
 */
#ifndef WRAPPERS_DESCRIPTOR_SET_H
#define WRAPPERS_DESCRIPTOR_SET_H

#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
    class DescriptorSet : public RefCounterSupportProvider
    {
    public:
        /** Represents a single buffer object, which can be bound to a specific descriptor set slot */
        typedef struct BufferBindingElement
        {
            Anvil::Buffer* buffer_ptr;
            VkDeviceSize   size;
            VkDeviceSize   start_offset;

            /** Constructor. Associates all available buffer memory with the binding.
             *
             *  @param in_buffer_ptr Buffer object to use for the binding. Must not be nullptr.
             *                       Retained. The object will be released at destruction time.
             **/
            BufferBindingElement(Anvil::Buffer* in_buffer_ptr);

            /** Constructor. Associates specified sub-region of the buffer memory with the binding.
             *
             *  @param in_buffer_ptr Buffer object to use for the binding. Must not be nullptr.
             *                       Retained. The object will be released at destruction time.
             **/
            BufferBindingElement(Anvil::Buffer* in_buffer_ptr,
                                 VkDeviceSize   in_start_offset,
                                 VkDeviceSize   in_size);

            /** Destructor. Releases the encapsulated buffer instance */
            ~BufferBindingElement();

            /** Copy assignment operator.
             *
             *  Retains the buffer object embedded in @param in.
             **/
            BufferBindingElement(const BufferBindingElement& in);

        private:
            BufferBindingElement& operator=(const BufferBindingElement& in);
        } BufferBindingElement;

        /** Holds a single buffer instance. Can be used to bind the object to a descriptor set slot
         *  as a dynamic storage buffer.
         **/
        typedef BufferBindingElement DynamicStorageBufferBindingElement;

        /** Holds a single buffer instance. Can be used to bind the object to a descriptor set slot
         *  as a dynamic uniform buffer.
         **/
        typedef BufferBindingElement DynamicUniformBufferBindingElement;

        /** Holds a single buffer instance. Can be used to bind the object to a descriptor set slot
         *  as a storage buffer.
         **/
        typedef BufferBindingElement StorageBufferBindingElement;

        /** Holds a single buffer instance. Can be used to bind the object to a descriptor set slot
         *  as a uniform buffer.
         **/
        typedef BufferBindingElement UniformBufferBindingElement;

        /** Holds a single combined image+sampler pair, along with other metadata required to bind the two
         *  to a specific descriptor set slot as a combined image+sampler
         **/
        typedef struct CombinedImageSamplerBindingElement
        {
            VkImageLayout     image_layout;
            Anvil::ImageView* image_view_ptr; 
            Anvil::Sampler*   sampler_ptr;

            /** Constructor.
             *
             *  @param in_image_layout   Image layout to use for the binding.
             *  @param in_image_view_ptr Image view to use for the binding. Must not be nullptr.
             *                           Retained. The object will be released at destruction time.
             *  @param in_sampler_ptr    Sampler to use for the binding. Can be nullptr, in which case
             *                           it will be assumed the element corresponds to an immutable
             *                           sampler and the specified instance will be retained.
             *                           The instance will be released at destruction time.
             **/
            CombinedImageSamplerBindingElement(VkImageLayout     in_image_layout,
                                               Anvil::ImageView* in_image_view_ptr,
                                               Anvil::Sampler*   in_sampler_ptr);

            /** Destructor.
             *
             *  Releases the embedded image view & sampler instances.
             **/
            ~CombinedImageSamplerBindingElement();

            /** Copy assignment operator.
             *
             *  Retains the image view and sampler objects embedded in @param in.
             **/
            CombinedImageSamplerBindingElement(const CombinedImageSamplerBindingElement& in);

        private:
            CombinedImageSamplerBindingElement& operator=(const CombinedImageSamplerBindingElement&);
        } CombinedImageSamplerBindingElement;

        /** Holds a single image view, along with other metadata required bound it to a specific descriptor set slot */
        typedef struct ImageBindingElement
        {
            VkImageLayout     image_layout;
            Anvil::ImageView* image_view_ptr;

            /** Constructor.
             *
             *  @param in_image_layout   Image layout to use for the binding.
             *  @param in_image_view_ptr Image view to use for the binding. Must not be nullptr.
             *                           Retained. The object will be released at destruction time.
             **/
            ImageBindingElement(VkImageLayout     in_image_layout,
                                Anvil::ImageView* in_image_view_ptr);

            /** Destructor.
             *
             *  Releases the embedded image view instance.
             **/
            ~ImageBindingElement();

            /** Copy assignment operator.
             *
             *  Retains the image view embedded in @param in.
             **/
            ImageBindingElement(const ImageBindingElement& in);

        private:
            ImageBindingElement& operator=(const ImageBindingElement&);
        } ImageBindingElement;

        /** Holds a single image view, along with other metadata required to bound it to a specific
         *  descriptor set slot as an input attachment
         **/
        typedef ImageBindingElement InputAttachmentBindingElement;

        /** Holds a single image view, along with other metadata required to bound it to a specific
         *  descriptor set slot as a sampled image.
         **/
        typedef ImageBindingElement SampledImageBindingElement;

        /** Holds a single image view, along with other metadata required to bound it to a specific
         *  descriptor set slot as a storage image.
         **/
        typedef ImageBindingElement StorageImageBindingElement;

        /** Holds a single sampler. Can be used to bind a sampler to a descriptor set slot **/
        typedef struct SamplerBindingElement
        {
            Anvil::Sampler* sampler_ptr;

            /** Constructor.
             *
             *  @param in_sampler_ptr Sampler to use for the binding. Can be nullptr, in which case
             *                        it will be assumed the element corresponds to an immutable
             *                        sampler and the specified instance will be retained.
             *                        The instance will be released at destruction time.
             **/
            SamplerBindingElement(Anvil::Sampler* in_sampler_ptr);

            /** Destructor.
             *
             *  Releases the embedded sampler instance.
             **/
            ~SamplerBindingElement();

            /** Copy assignment operator.
             *
             *  Retains the sampler embedded in @param in.
             **/
            SamplerBindingElement(const SamplerBindingElement& in);

        private:
            SamplerBindingElement& operator=(const SamplerBindingElement&);
        } SamplerBindingElement;

        /** Holds a single buffer view instance. Can be used to bind a sampler to a descriptor set slot */
        typedef struct TexelBufferBindingElement
        {
            Anvil::BufferView* buffer_view_ptr;

            /** Constructor.
             *
             *  @param in_buffer_view_ptr Buffer view to use for the binding. Must not be nullptr.
             *                            Retained. The object will be released at destruction time.
             **/
            TexelBufferBindingElement(Anvil::BufferView* in_buffer_view_ptr);

            /** Destructor.
             *
             *  Releases the embedded buffer view instance.
             **/
            ~TexelBufferBindingElement();

            /** Copy assignment operator.
             *
             *  Retains the buffer view embedded in @param in.
             **/
            TexelBufferBindingElement(const TexelBufferBindingElement& in);

        private:
            TexelBufferBindingElement& operator=(const TexelBufferBindingElement&);
        } TexelBufferBindingElement;

        /** Holds a single buffer view instance. Can be used to bind a sampler to a descriptor set slot
         *  as a storage texel buffer.
         **/
        typedef TexelBufferBindingElement StorageTexelBufferBindingElement;

        /** Holds a single buffer view instance. Can be used to bind a sampler to a descriptor set slot
         *  as a uniform texel buffer.
         **/
        typedef TexelBufferBindingElement UniformTexelBufferBindingElement;


        /* Public functions */

        /** Constructor.
         *
         *  @param device_ptr      Device the descriptor set has been initialized for.
         *  @param parent_pool_ptr Pool, from which the descriptor set has been allocated from.
         *                         Must not be nullptr.
         *  @param layout_ptr      Layout which has been used at descriptor set construction time.
         *  @param descriptor_set  Raw Vulkan handle the wrapper instance is being created for.
         **/
        DescriptorSet(Anvil::Device*              device_ptr,
                      Anvil::DescriptorPool*      parent_pool_ptr,
                      Anvil::DescriptorSetLayout* layout_ptr,
                      VkDescriptorSet             descriptor_set);

        /** Updates internally-maintained Vulkan descriptor set instances.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool bake();

        /** Retrieves raw Vulkan handle of the encapsulated descriptor set.
         *
         *  If the wrapper instance is marked as dirty, the function will bake the descriptor set,
         *  prior to returning the handle.
         *
         *  @return As per description.
         **/
        VkDescriptorSet get_descriptor_set_vk()
        {
            if (m_dirty)
            {
                bake();

                anvil_assert(!m_dirty);
            }

            return m_descriptor_set;
        }

        /** Returns information about the number of bindings described by the descriptor set. */
        uint32_t get_n_bindings() const
        {
            return static_cast<uint32_t>(m_bindings.size() );
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
         *  @param binding_index As per documentation. Must correspond to a binding which has earlier
         *                       been added by calling add_binding() function.
         *  @param element_range As per documentation. Must not be equal to or larger than the array size,
         *                       specified when calling add_binding() function.
         *  @param elements      As per documentation. Must not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        template<typename BindingElementType>
        bool set_binding_array_items(BindingIndex              binding_index,
                                     BindingElementArrayRange  element_range,
                                     const BindingElementType* elements)
        {
            anvil_assert(elements   != nullptr);
            anvil_assert(m_unusable == false);

            BindingItems&  binding_items      = m_bindings[binding_index];
            const uint32_t last_element_index = element_range.second + element_range.first;

            for (BindingElementIndex current_element_index = element_range.first;
                                     current_element_index < last_element_index;
                                   ++current_element_index)
            {
                m_dirty |= !(binding_items[current_element_index] == elements[current_element_index - element_range.first]);

                binding_items[current_element_index] = elements[current_element_index - element_range.first];
            }

            return true;
        }

        /** This function works exactly like set_binding_array_items(), except that it always replaces the zeroth element
         *  attached to the specified descriptor set's binding.
         */
        template<typename BindingElementType>
        bool set_binding_item(BindingIndex              binding_index,
                              const BindingElementType& element)
        {
            return set_binding_array_items(binding_index,
                                           BindingElementArrayRange(0,  /* StartBindingElementIndex */
                                                                    1), /* NumberOfBindingElements  */
                                          &element);
        }

        /** Assigns a new Vulkan descriptor set handle to the wrapper instance.
         *
         *  This function should only be used internally. Its purpose is to introduce support for "recycling" of
         *  deprecated descriptor sets. When a descriptor set pool is reset or the descriptor set layout is adjusted,
         *  Descriptor Set's Vulkan handle may become obsolete. When that happens, this function can be called to
         *  "revive" the object by assigning it a new handle, at which point the object becomes usable again.
         *  Furthermore, all cached binding information will be automatically writtne to the descriptor set at
         *  next baking time.
         *
         *  @param ds New Vulkan handle to use. Must not be VK_NULL_HANDLE.
         **/
        void set_new_vk_handle(VkDescriptorSet ds);

    private:
        /* Private type declarations */

        /** Structure which holds raw Vulkan objects, required to perform a write op against
         *  a descriptor set.
         *
         *  Each structure instance is assumed to describe a single binding's array item.
         **/
        typedef struct BindingItem
        {
            Anvil::Buffer*     buffer_ptr;
            Anvil::BufferView* buffer_view_ptr;
            VkImageLayout      image_layout;
            Anvil::ImageView*  image_view_ptr;
            Anvil::Sampler*    sampler_ptr;
            VkDeviceSize       size;
            VkDeviceSize       start_offset;

            bool dirty;

            bool operator==(const BufferBindingElement& in) const
            {
                return (buffer_ptr   == in.buffer_ptr      &&
                        size         == in.size            &&
                        start_offset == in.start_offset);
            }

            bool operator==(const CombinedImageSamplerBindingElement& in) const
            {
                return (image_layout   == in.image_layout     &&
                        image_view_ptr == in.image_view_ptr   &&
                        sampler_ptr    == in.sampler_ptr);
            }

            bool operator==(const ImageBindingElement& in) const
            {
                return (image_layout   == in.image_layout   &&
                        image_view_ptr == in.image_view_ptr);
            }

            bool operator==(const SamplerBindingElement& in) const
            {
                return (sampler_ptr == in.sampler_ptr);
            }

            bool operator==(const TexelBufferBindingElement& in) const
            {
                return (buffer_view_ptr == in.buffer_view_ptr);
            }

            /* Copy assignment operator.
             *
             * Retains the buffer instance embedded in @param element
             **/
            BindingItem& operator=(const BufferBindingElement& element);

            /* Copy assignment operator.
             *
             * Retains the image view & sampler instances embedded in @param element
             **/
            BindingItem& operator=(const CombinedImageSamplerBindingElement& element);

            /* Copy assignment operator.
             *
             * Retains the image view instance embedded in @param element
             **/
            BindingItem& operator=(const ImageBindingElement& element);

            /* Copy assignment operator.
             *
             * Retains the sampler instance embedded in @param element
             **/
            BindingItem& operator=(const SamplerBindingElement& element);

            /* Copy assignment operator.
             *
             * Retains the buffer view instance embedded in @param element
             **/
            BindingItem& operator=(const TexelBufferBindingElement& element);

            /* Default dummy constructor. */ 
            BindingItem()
            {
                memset(this,
                       0,
                       sizeof(*this) );
            }

            /* Destructor.
             *
             * Releases all wrapper instances embedded in the descriptor */
            ~BindingItem();
        } BindingItem;

        typedef std::vector<BindingItem>             BindingItems;
        typedef std::map<BindingIndex, BindingItems> BindingIndexToBindingItemsMap;

        /* Private functions */
        DescriptorSet           (const DescriptorSet&);
        DescriptorSet& operator=(const DescriptorSet&);

        virtual ~DescriptorSet();

        static void on_binding_added_to_layout(void* layout_raw_ptr,
                                               void* ds_raw_ptr);
        static void on_parent_pool_reset      (void* pool_raw_ptr,
                                               void* ds_raw_ptr);

        void alloc_bindings();

        /* Private variables */
        BindingIndexToBindingItemsMap m_bindings;
        VkDescriptorSet               m_descriptor_set;
        Anvil::Device*                m_device_ptr;
        bool                          m_dirty;
        Anvil::DescriptorSetLayout*   m_layout_ptr;
        bool                          m_unusable;
        Anvil::DescriptorPool*        m_parent_pool_ptr;

        std::vector<VkDescriptorBufferInfo> m_cached_ds_info_buffer_info_items_vk;
        std::vector<VkDescriptorImageInfo>  m_cached_ds_info_image_info_items_vk;
        std::vector<VkBufferView>           m_cached_ds_info_texel_buffer_info_items_vk;
        std::vector<VkWriteDescriptorSet>   m_cached_ds_write_items_vk;
    };
};

#endif /* WRAPPERS_DESCRIPTOR_SET_H */