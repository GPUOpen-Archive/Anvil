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

/** Descriptor Set wrapper implementation.
 *
 *  Implemented to:
 *
 *  - cache set binding information.
 *  - monitor layout adjustments and act accordingly.
 *  - monitor pool reset events and act accordingly.
 */
#ifndef WRAPPERS_DESCRIPTOR_SET_H
#define WRAPPERS_DESCRIPTOR_SET_H

#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    class DescriptorSet : public DebugMarkerSupportProvider<DescriptorSet>,
                          public MTSafetySupportProvider
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
             **/
            BufferBindingElement(Anvil::Buffer* in_buffer_ptr);

            /** Constructor. Associates specified sub-region of the buffer memory with the binding.
             *
             *  @param in_buffer_ptr Buffer object to use for the binding. Must not be nullptr.
             **/
            BufferBindingElement(Anvil::Buffer* in_buffer_ptr,
                                 VkDeviceSize   in_start_offset,
                                 VkDeviceSize   in_size);

            /** Destructor. Releases the encapsulated buffer instance */
            virtual ~BufferBindingElement();

            /** Copy assignment operator.
             *
             *  Retains the buffer object embedded in @param in.
             **/
            BufferBindingElement(const BufferBindingElement& in);

            /* Returns Vulkan descriptor type for this structure */
            virtual Anvil::DescriptorType get_type() const = 0;

        private:
            BufferBindingElement& operator=(const BufferBindingElement& in);
        } BufferBindingElement;

        /** Holds a single buffer instance. Can be used to bind the object to a descriptor set slot
         *  as a dynamic storage buffer.
         **/
        struct DynamicStorageBufferBindingElement : public BufferBindingElement
        {
            DynamicStorageBufferBindingElement() = delete;

            DynamicStorageBufferBindingElement(Anvil::Buffer* in_buffer_ptr)
                : BufferBindingElement(in_buffer_ptr)
            {
                /* Stub */
            }

            DynamicStorageBufferBindingElement(Anvil::Buffer* in_buffer_ptr,
                                               VkDeviceSize   in_start_offset,
                                               VkDeviceSize   in_size)
                : BufferBindingElement(in_buffer_ptr,
                                       in_start_offset,
                                       in_size)
            {
                /* Stub */
            }

            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::STORAGE_BUFFER_DYNAMIC;
            }
        };

        /** Holds a single buffer instance. Can be used to bind the object to a descriptor set slot
         *  as a dynamic uniform buffer.
         **/
        struct DynamicUniformBufferBindingElement : public BufferBindingElement
        {
            DynamicUniformBufferBindingElement() = delete;

            DynamicUniformBufferBindingElement(Anvil::Buffer* in_buffer_ptr)
                : BufferBindingElement(in_buffer_ptr)
            {
                /* Stub */
            }

            DynamicUniformBufferBindingElement(Anvil::Buffer* in_buffer_ptr,
                                               VkDeviceSize   in_start_offset,
                                               VkDeviceSize   in_size)
                : BufferBindingElement(in_buffer_ptr,
                                       in_start_offset,
                                       in_size)
            {
                /* Stub */
            }

            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
            }
        };

        /** Holds a single buffer instance. Can be used to bind the object to a descriptor set slot
         *  as a storage buffer.
         **/
        struct StorageBufferBindingElement : public BufferBindingElement
        {
            StorageBufferBindingElement() = delete;

            StorageBufferBindingElement(Anvil::Buffer* in_buffer_ptr)
                :BufferBindingElement(in_buffer_ptr)
            {
                /* Stub */
            }

            StorageBufferBindingElement(Anvil::Buffer* in_buffer_ptr,
                                        VkDeviceSize   in_start_offset,
                                        VkDeviceSize   in_size)
                :BufferBindingElement(in_buffer_ptr,
                                      in_start_offset,
                                      in_size)
            {
                /* Stub */
            }

            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::STORAGE_BUFFER;
            }
        };

        /** Holds a single buffer instance. Can be used to bind the object to a descriptor set slot
         *  as a uniform buffer.
         **/
        struct UniformBufferBindingElement : BufferBindingElement
        {
            UniformBufferBindingElement() = delete;

            UniformBufferBindingElement(Anvil::Buffer* in_buffer_ptr)
                :BufferBindingElement(in_buffer_ptr)
            {
                /* Stub */
            }

            UniformBufferBindingElement(Anvil::Buffer* in_buffer_ptr,
                                        VkDeviceSize   in_start_offset,
                                        VkDeviceSize   in_size)
                :BufferBindingElement(in_buffer_ptr,
                                      in_start_offset,
                                      in_size)
            {
                /* Stub */
            }

            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::UNIFORM_BUFFER;
            }
        };

        /** Holds a single combined image+sampler pair, along with other metadata required to bind the two
         *  to a specific descriptor set slot as a combined image+sampler
         **/
        typedef struct CombinedImageSamplerBindingElement
        {
            Anvil::ImageLayout image_layout;
            Anvil::ImageView*  image_view_ptr; 
            Anvil::Sampler*    sampler_ptr;

            /** Constructor.
             *
             *  @param in_image_layout   Image layout to use for the binding.
             *  @param in_image_view_ptr Image view to use for the binding. Must not be nullptr.
             *  @param in_sampler_ptr    Sampler to use for the binding. Can be nullptr, in which case
             *                           it will be assumed the element corresponds to an immutable
             *                           sampler.
             **/
            CombinedImageSamplerBindingElement(Anvil::ImageLayout in_image_layout,
                                               Anvil::ImageView*  in_image_view_ptr,
                                               Anvil::Sampler*    in_sampler_ptr);

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

            /* Returns Vulkan descriptor type for this structure */
            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER;
            }

        private:
            CombinedImageSamplerBindingElement& operator=(const CombinedImageSamplerBindingElement&);
        } CombinedImageSamplerBindingElement;

        /** Holds a single image view, along with other metadata required bound it to a specific descriptor set slot */
        typedef struct ImageBindingElement
        {
            Anvil::ImageLayout image_layout;
            Anvil::ImageView*  image_view_ptr;

            /** Constructor.
             *
             *  @param in_image_layout   Image layout to use for the binding.
             *  @param in_image_view_ptr Image view to use for the binding. Must not be nullptr.
             **/
            ImageBindingElement(Anvil::ImageLayout in_image_layout,
                                Anvil::ImageView* in_image_view_ptr);

            /** Copy assignment operator.
             *
             *  Retains the image view embedded in @param in.
             **/
            ImageBindingElement(const ImageBindingElement& in);

            /** Destructor.
             *
             *  Releases the embedded image view instance.
             **/
            virtual ~ImageBindingElement();

            /* Returns Vulkan descriptor type for this structure */
            virtual Anvil::DescriptorType get_type() const = 0;

        private:
            ImageBindingElement& operator=(const ImageBindingElement&);
        } ImageBindingElement;

        /** Holds a single image view, along with other metadata required to bound it to a specific
         *  descriptor set slot as an input attachment
         **/
        struct InputAttachmentBindingElement : public ImageBindingElement
        {
            InputAttachmentBindingElement() = delete;

            InputAttachmentBindingElement(Anvil::ImageLayout in_image_layout,
                                          Anvil::ImageView*  in_image_view_ptr)
                :ImageBindingElement(in_image_layout,
                                     in_image_view_ptr)
            {
                /* Stub */
            }

            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::INPUT_ATTACHMENT;
            }
        };

        /** Holds a single image view, along with other metadata required to bound it to a specific
         *  descriptor set slot as a sampled image.
         **/
        struct SampledImageBindingElement : ImageBindingElement
        {
            SampledImageBindingElement() = delete;

            SampledImageBindingElement(Anvil::ImageLayout in_image_layout,
                                       Anvil::ImageView*  in_image_view_ptr)
                :ImageBindingElement(in_image_layout,
                                     in_image_view_ptr)
            {
                /* Stub */
            }

            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::SAMPLED_IMAGE;
            }
        };

        /** Holds a single image view, along with other metadata required to bound it to a specific
         *  descriptor set slot as a storage image.
         **/
        struct StorageImageBindingElement : public ImageBindingElement
        {
            StorageImageBindingElement() = delete;

            StorageImageBindingElement(Anvil::ImageLayout in_image_layout,
                                       Anvil::ImageView*  in_image_view_ptr)
                :ImageBindingElement(in_image_layout,
                                     in_image_view_ptr)
            {
                /* Stub */
            }

            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::STORAGE_IMAGE;
            }
        };

        /** Holds a single sampler. Can be used to bind a sampler to a descriptor set slot **/
        typedef struct SamplerBindingElement
        {
            Anvil::Sampler* sampler_ptr;

            /** Constructor.
             *
             *  @param in_sampler_ptr Sampler to use for the binding. Can be nullptr, in which case
             *                        it will be assumed the element corresponds to an immutable
             *                        sampler.
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

            /* Returns Vulkan descriptor type for this structure */
            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::SAMPLER;
            }

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
            virtual ~TexelBufferBindingElement();

            /** Copy assignment operator.
             *
             *  Retains the buffer view embedded in @param in.
             **/
            TexelBufferBindingElement(const TexelBufferBindingElement& in);

            /* Returns Vulkan descriptor type for this structure */
            virtual Anvil::DescriptorType get_type() const = 0;

        private:
            TexelBufferBindingElement& operator=(const TexelBufferBindingElement&);
        } TexelBufferBindingElement;

        /** Holds a single buffer view instance. Can be used to bind a sampler to a descriptor set slot
         *  as a storage texel buffer.
         **/
        struct StorageTexelBufferBindingElement : public TexelBufferBindingElement
        {
            StorageTexelBufferBindingElement() = delete;

            StorageTexelBufferBindingElement(Anvil::BufferView* in_buffer_view_ptr)
                : TexelBufferBindingElement(in_buffer_view_ptr)
            {
                /* Stub */
            }

            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::STORAGE_TEXEL_BUFFER;
            }
        };

        /** Holds a single buffer view instance. Can be used to bind a sampler to a descriptor set slot
         *  as a uniform texel buffer.
         **/
        struct UniformTexelBufferBindingElement : public TexelBufferBindingElement
        {
            UniformTexelBufferBindingElement() = delete;

            UniformTexelBufferBindingElement(Anvil::BufferView* in_buffer_view_ptr)
                : TexelBufferBindingElement(in_buffer_view_ptr)
            {
                /* Stub */
            }

            Anvil::DescriptorType get_type() const
            {
                return Anvil::DescriptorType::UNIFORM_TEXEL_BUFFER;
            }
        };


        /* Public functions */

        /** Destructor.
         *
         *  Unregisters the object from the object tracker.
         **/
        virtual ~DescriptorSet();

        /** Returns properties of a combined image/sampler descriptor binding.
         *
         *  @param in_n_binding             Binding index to use for the query.
         *  @param in_n_binding_array_item  Index of the array item to use for the query.
         *  @param out_opt_image_layout_ptr If the function reports success, deref will be set to the image layout
         *                                  specified for the binding. May be null.
         *  @param out_opt_image_view_ptr   If the function reports success, deref will be set to the image view
         *                                  specified for the binding. May be null.
         *  @param out_opt_sampler_ptr      If the function reports success, deref will be set to the sampler specified
         *                                  for the binding. May be null.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_combined_image_sampler_binding_properties(uint32_t            in_n_binding,
                                                           uint32_t            in_n_binding_array_item,
                                                           Anvil::ImageLayout* out_opt_image_layout_ptr,
                                                           Anvil::ImageView**  out_opt_image_view_ptr_ptr,
                                                           Anvil::Sampler**    out_opt_sampler_ptr_ptr);

        /** Retrieves raw Vulkan handle of the encapsulated descriptor set.
         *
         *  If the wrapper instance is marked as dirty, the function will bake the descriptor set,
         *  prior to returning the handle.
         *
         *  @return As per description.
         **/
        VkDescriptorSet get_descriptor_set_vk() const
        {
            if (m_dirty)
            {
                update();

                anvil_assert(!m_dirty);
            }

            return m_descriptor_set;
        }

        /** Returns a descriptor set layout wrapper instance, assigned to the descriptor set wrapper */
        const Anvil::DescriptorSetLayout* get_descriptor_set_layout() const
        {
            return m_layout_ptr;
        }

        /** Returns properties of an input attachment descriptor binding.
         *
         *  @param in_n_binding                 Binding index to use for the query.
         *  @param in_n_binding_array_item      Index of the array item to use for the query.
         *  @param out_opt_image_layout_ptr_ptr If not null, deref will be set to the image layout specified for
         *                                      the binding. May be null.
         *  @param out_opt_image_view_ptr_ptr   If not null, deref will be set to the image view specified for
         *                                      the binding. May be null.
         *
         *  @return true if successful, false otherwise.
         *
         */
        bool get_input_attachment_binding_properties(uint32_t            in_n_binding,
                                                     uint32_t            in_n_binding_array_item,
                                                     Anvil::ImageLayout* out_opt_image_layout_ptr_ptr,
                                                     Anvil::ImageView**  out_opt_image_view_ptr_ptr) const;

        /** Returns properties of a sampled image descriptor binding.
         *
         *  @param in_n_binding               Binding index to use for the query.
         *  @param in_n_binding_array_item    Index of the array item to use for the query.
         *  @param out_opt_image_layout_ptr   If not null, deref will be set to the image layout specified
         *                                    for the binding. May be null.
         *  @param out_opt_image_view_ptr_prt If not null, deref will be set to the image view specified for
         *                                    the binding. May be null.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_sampled_image_binding_properties(uint32_t            in_n_binding,
                                                  uint32_t            in_n_binding_array_item,
                                                  Anvil::ImageLayout* out_opt_image_layout_ptr,
                                                  Anvil::ImageView**  out_opt_image_view_ptr_prt) const
        {
            /* Re-use existing code */
            return get_input_attachment_binding_properties(in_n_binding,
                                                           in_n_binding_array_item,
                                                           out_opt_image_layout_ptr,
                                                           out_opt_image_view_ptr_prt);
        }

        /** Returns properties of a sampler descriptor binding.
         *
         *  @param in_n_binding            Binding index to use for the query.
         *  @param in_n_binding_array_item Index of the array item to use for the query.
         *  @param out_sampler_ptr_ptr     If not null, deref will be set to the sampler specified for
         *                                 the binding. May be null.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_sampler_binding_properties(uint32_t         in_n_binding,
                                            uint32_t         in_n_binding_array_item,
                                            Anvil::Sampler** out_sampler_ptr_ptr) const;

        /** Returns properties of a storage buffer descriptor binding.
         *
         *  @param in_n_binding             Binding index to use for the query.
         *  @param in_n_binding_array_item  Index of the array item to use for the query.
         *  @param out_opt_buffer_ptr_ptr   If not null, deref will be set to the buffer specified for
         *                                  the binding. May be null.
         *  @param out_opt_size_ptr         If not null, deref will be set to the size of the buffer
         *                                  memory region associated with the binding. May be null.
         *  @param out_opt_start_offset_ptr If not null, deref will be set to the start offset of the
         *                                  buffer memory region associated with the binding. May be
         *                                  null.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_storage_buffer_binding_properties(uint32_t        in_n_binding,
                                                   uint32_t        in_n_binding_array_item,
                                                   Anvil::Buffer** out_opt_buffer_ptr_ptr,
                                                   VkDeviceSize*   out_opt_size_ptr,
                                                   VkDeviceSize*   out_opt_start_offset_ptr) const;

        /** Returns properties of a storage image descriptor binding.
         *
         *  @param in_n_binding               Binding index to use for the query.
         *  @param in_n_binding_array_item    Index of the array item to use for the query.
         *  @param out_opt_image_layout_ptr   If not null, deref will be set to the image layout declared for
         *                                    the binding. May be null.
         *  @param out_opt_image_view_ptr_ptr If not null, deref will be set to the image view specified for
         *                                    the binding. May be null.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_storage_image_binding_properties(uint32_t            in_n_binding,
                                                  uint32_t            in_n_binding_array_item,
                                                  Anvil::ImageLayout* out_opt_image_layout_ptr,
                                                  Anvil::ImageView**  out_opt_image_view_ptr_ptr) const
        {
            /* Re-use existing code */
            return get_input_attachment_binding_properties(in_n_binding,
                                                           in_n_binding_array_item,
                                                           out_opt_image_layout_ptr,
                                                           out_opt_image_view_ptr_ptr);
        }

        /** Returns properties of a storage texel buffer descriptor binding.
         *
         *  @param in_n_binding                Binding index to use for the query.
         *  @param in_n_binding_array_item     Index of the array item to use for the query.
         *  @param out_opt_buffer_view_ptr_ptr If not null, deref will be set to the buffer view specified for
         *                                     the binding. May be null.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_storage_texel_buffer_binding_properties(uint32_t            in_n_binding,
                                                         uint32_t            in_n_binding_array_item,
                                                         Anvil::BufferView** out_opt_buffer_view_ptr_ptr) const;

        /** Returns properties of a uniform buffer descriptor binding.
         *
         *  @param in_n_binding             Binding index to use for the query.
         *  @param in_n_binding_array_item  Index of the array item to use for the query.
         *  @param out_opt_buffer_ptr_ptr   If not null, deref will be set to the buffer specified for
         *                                  the binding. May be null.
         *  @param out_opt_size_ptr         If not null, deref will be set to size of the buffer memory region
         *                                  declared for the binding. May be null.
         *  @param out_opt_start_offset_ptr If not null, deref will be set to start offset of the buffer memory
         *                                  region declared for the binding. May be null.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_uniform_buffer_binding_properties(uint32_t        in_n_binding,
                                                   uint32_t        in_n_binding_array_item,
                                                   Anvil::Buffer** out_opt_buffer_ptr_ptr,
                                                   VkDeviceSize*   out_opt_size_ptr,
                                                   VkDeviceSize*   out_opt_start_offset_ptr) const
        {
            /* Re-use existing code */
            return get_storage_buffer_binding_properties(in_n_binding,
                                                         in_n_binding_array_item,
                                                         out_opt_buffer_ptr_ptr,
                                                         out_opt_size_ptr,
                                                         out_opt_start_offset_ptr);
        }

        /** Returns properties of a uniform texel buffer descriptor binding.
         *
         *  @param in_n_binding                Binding index to use for the query.
         *  @param in_n_binding_array_item     Index of the array item to use for the query.
         *  @param out_opt_buffer_view_ptr_ptr If not null, deref will be set to the buffer view specified for
         *                                     the binding. May be null.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_uniform_texel_buffer_binding_properties(uint32_t            in_n_binding,
                                                         uint32_t            in_n_binding_array_item,
                                                         Anvil::BufferView** out_opt_buffer_view_ptr_ptr) const
        {
            /* Re-use existing code */
            return get_storage_texel_buffer_binding_properties(in_n_binding,
                                                               in_n_binding_array_item,
                                                               out_opt_buffer_view_ptr_ptr);
        }

        /** This function should be set to assign physical Vulkan objects to a descriptor binding
         *  at index @param in_binding_index for descriptor set @param in_n_set.
         *  Each binding can hold one or more objects. Which slots the specified objects should take can
         *  be configured by passing the right values to @param in_element_range.
         *  Objects are passed via @param in_elements argument. The argument must be passed an object of
         *  one of the following types, depending on what object is to be attached to the specified
         *  descriptor binding:
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
         *  This function CANNOT be used for inline uniform block binding updates. Instead, please use set_inline_uniform_block_binding_data().
         *
         *  @param in_binding_index As per documentation. Must correspond to a binding which has earlier
         *                          been added by calling add_binding() function.
         *  @param in_element_range As per documentation. Must not be equal to or larger than the array size,
         *                          specified when calling add_binding() function.
         *  @param in_elements      As per documentation. Must not be nullptr.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        template<typename BindingElementType>
        bool set_binding_array_items(BindingIndex              in_binding_index,
                                     BindingElementArrayRange  in_element_range,
                                     const BindingElementType* in_elements_ptr)
        {
            anvil_assert(in_elements_ptr != nullptr);
            anvil_assert(m_unusable       == false);

            BindingItemUniquePtrs& binding_item_ptrs  = m_binding_ptrs[in_binding_index];
            const uint32_t         last_element_index = in_element_range.second + in_element_range.first;

            for (BindingElementIndex current_element_index = in_element_range.first;
                                     current_element_index < last_element_index;
                                   ++current_element_index)
            {
                if (!( binding_item_ptrs[current_element_index]  != nullptr                                                         &&
                      *binding_item_ptrs[current_element_index] == in_elements_ptr[current_element_index - in_element_range.first]) )
                {
                    m_dirty = true;

                    binding_item_ptrs[current_element_index].reset(
                        new Anvil::DescriptorSet::BindingItem()
                    );

                    *binding_item_ptrs[current_element_index] = in_elements_ptr[current_element_index - in_element_range.first];
                }
            }

            return true;
        }

        /* TODO
         *
         * NOTE: This function CANNOT be used for inline uniform block binding updates. Instead, please use set_inline_uniform_block_binding_data().
         */
        template<typename BindingElementType>
        bool set_binding_array_items(BindingIndex                     in_binding_index,
                                     BindingElementArrayRange         in_element_range,
                                     const BindingElementType* const* in_elements_ptr_ptr)
        {
            anvil_assert(in_elements_ptr_ptr != nullptr);
            anvil_assert(m_unusable          == false);

            BindingItemUniquePtrs& binding_item_ptrs  = m_binding_ptrs[in_binding_index];
            const uint32_t         last_element_index = in_element_range.second + in_element_range.first;

            for (BindingElementIndex current_element_index = in_element_range.first;
                                     current_element_index < last_element_index;
                                   ++current_element_index)
            {
                m_dirty |= !(*binding_item_ptrs[current_element_index] == *in_elements_ptr_ptr[current_element_index - in_element_range.first]);

                binding_item_ptrs[current_element_index].reset(
                    new Anvil::DescriptorSet::BindingItem()
                );

                *binding_item_ptrs[current_element_index] = *in_elements_ptr_ptr[current_element_index - in_element_range.first];
            }

            return true;
        }

        /** This function works exactly like set_binding_array_items(), except that it always replaces the zeroth element
         *  attached to the specified descriptor set's binding.
         *
         *  NOTE: This function CANNOT be used for inline uniform block binding updates. Instead, please use set_inline_uniform_block_binding_data().
         *
         */
        template<typename BindingElementType>
        bool set_binding_item(BindingIndex              in_binding_index,
                              const BindingElementType& in_element)
        {
            return set_binding_array_items(in_binding_index,
                                           BindingElementArrayRange(0,  /* StartBindingElementIndex */
                                                                    1), /* NumberOfBindingElements  */
                                          &in_element);
        }

        /** TODO
         *
         *  NOTE: Multiple update requests for the same inline uniform block binding are supported and will be executed via consecutive API calls.
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
        bool set_inline_uniform_block_binding_data(const BindingIndex& in_binding_index,
                                                   const uint32_t&     in_start_offset,
                                                   const uint32_t&     in_size,
                                                   const void*         in_raw_data_ptr,
                                                   const bool&         in_should_cache_raw_data);

        /** Updates internally-maintained Vulkan descriptor set instances.
         *
         *  @param in_update_method Please see DescriptorSetUpdateMethod documentation for more details.
         *
         *  @return true if the function executed successfully, false otherwise.
         **/
        bool update(const DescriptorSetUpdateMethod& in_update_method = Anvil::DescriptorSetUpdateMethod::CORE) const;

    private:
        /* Private type declarations */

        /** Structure which holds raw Vulkan objects, required to perform a write op against
         *  a descriptor set.
         *
         *  Each structure instance is assumed to describe a single binding's array item.
         **/
        typedef struct BindingItem
        {
            Anvil::Buffer*                                           buffer_ptr;
            Anvil::BufferView*                                       buffer_view_ptr;
            Anvil::ImageLayout                                       image_layout;
            Anvil::ImageView*                                        image_view_ptr;
            std::unique_ptr<uint8_t, std::function<void(uint8_t*)> > iub_update_data_ptr;
            Anvil::Sampler*                                          sampler_ptr;
            VkDeviceSize                                             size;
            VkDeviceSize                                             start_offset;
            Anvil::DescriptorType                                    type_vk;


            bool dirty;

            bool operator==(const BufferBindingElement& in) const
            {
                return (buffer_ptr   == in.buffer_ptr      &&
                        size         == in.size            &&
                        start_offset == in.start_offset    &&
                        type_vk      == in.get_type() );
            }

            bool operator==(const CombinedImageSamplerBindingElement& in) const
            {
                return (image_layout   == in.image_layout     &&
                        image_view_ptr == in.image_view_ptr   &&
                        sampler_ptr    == in.sampler_ptr      &&
                        type_vk        == in.get_type() );
            }

            bool operator==(const ImageBindingElement& in) const
            {
                return (image_layout   == in.image_layout   &&
                        image_view_ptr == in.image_view_ptr &&
                        type_vk        == in.get_type() );
            }

            bool operator==(const SamplerBindingElement& in) const
            {
                return (sampler_ptr == in.sampler_ptr &&
                        type_vk     == in.get_type() );
            }

            bool operator==(const TexelBufferBindingElement& in) const
            {
                return (buffer_view_ptr == in.buffer_view_ptr &&
                        type_vk         == in.get_type() );
            }

            BindingItem& operator=(const BufferBindingElement&               in_element);
            BindingItem& operator=(const CombinedImageSamplerBindingElement& in_element);
            BindingItem& operator=(const ImageBindingElement&                in_element);
            BindingItem& operator=(const SamplerBindingElement&              in_element);
            BindingItem& operator=(const TexelBufferBindingElement&          in_element);

            /* Default dummy constructor. */ 
            BindingItem()
            {
                dirty        = false;
                image_layout = Anvil::ImageLayout::UNKNOWN;
                size         = 0;
                start_offset = 0;

                buffer_ptr      = nullptr;
                buffer_view_ptr = nullptr;
                image_view_ptr  = nullptr;
                sampler_ptr     = nullptr;

                type_vk = Anvil::DescriptorType::UNKNOWN;
            }

            /* Destructor.
             *
             * Releases all wrapper instances embedded in the descriptor */
            ~BindingItem();
        } BindingItem;

        typedef std::unique_ptr<BindingItem>                  BindingItemUniquePtr;
        typedef std::vector<BindingItemUniquePtr>             BindingItemUniquePtrs;
        typedef std::map<BindingIndex, BindingItemUniquePtrs> BindingIndexToBindingItemUniquePtrsMap;

        /* Private functions */

        /** Please see create() documentation for argument discussion */
        DescriptorSet(const Anvil::BaseDevice*          in_device_ptr,
                      Anvil::DescriptorPool*            in_parent_pool_ptr,
                      const Anvil::DescriptorSetLayout* in_layout_ptr,
                      VkDescriptorSet                   in_descriptor_set,
                      bool                              in_mt_safe);

        /** Creates a new DescriptorSet instance.
         *
         *  @param in_device_ptr      Device the descriptor set has been initialized for.
         *  @param in_parent_pool_ptr Pool, from which the descriptor set has been allocated from.
         *                            Must not be nullptr.
         *  @param in_layout_ptr      Layout which has been used at descriptor set construction time.
         *  @param in_descriptor_set  Raw Vulkan handle the wrapper instance is being created for.
         **/
        static DescriptorSetUniquePtr create(const Anvil::BaseDevice*          in_device_ptr,
                                             Anvil::DescriptorPool*            in_parent_pool_ptr,
                                             const Anvil::DescriptorSetLayout* in_layout_ptr,
                                             VkDescriptorSet                   in_descriptor_set,
                                             MTSafety                          in_mt_safety = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE);

        DescriptorSet           (const DescriptorSet&);
        DescriptorSet& operator=(const DescriptorSet&);

        void alloc_bindings                ();
        void fill_buffer_info_vk_descriptor(const Anvil::DescriptorSet::BindingItem&   in_binding_item,
                                            VkDescriptorBufferInfo*                    out_descriptor_ptr) const;
        void fill_image_info_vk_descriptor (const Anvil::DescriptorSet::BindingItem&   in_binding_item,
                                            const bool&                                in_immutable_samplers_enabled,
                                            VkDescriptorImageInfo*                     out_descriptor_ptr) const;
        void fill_iub_vk_descriptor        (const Anvil::DescriptorSet::BindingItem&   in_binding_item,
                                            VkWriteDescriptorSetInlineUniformBlockEXT* out_descriptor_ptr) const;
        void on_parent_pool_reset          ();
        bool update_using_core_method      () const;
        bool update_using_template_method  () const;

        /* Private variables */

        /* For descriptor types != INLINE_UNIFORM_BLOCK, this is the usual binding index->binding item map. Holds bindings associated
         * with corresponding array items.
         *
         * For INLINE_UNIFORM_BLOCK descriptors, this is a binding index->pending updates map.
         */
        mutable BindingIndexToBindingItemUniquePtrsMap m_binding_ptrs;


        VkDescriptorSet                                m_descriptor_set;
        const Anvil::BaseDevice*                       m_device_ptr;
        mutable bool                                   m_dirty;
        const Anvil::DescriptorSetLayout*              m_layout_ptr;
        Anvil::DescriptorPool*                         m_parent_pool_ptr;
        bool                                           m_unusable;

        mutable std::vector<VkDescriptorBufferInfo>                    m_cached_ds_info_buffer_info_items_vk;
        mutable std::vector<VkDescriptorImageInfo>                     m_cached_ds_info_image_info_items_vk;
        mutable std::vector<VkBufferView>                              m_cached_ds_info_texel_buffer_info_items_vk;
        mutable std::vector<VkWriteDescriptorSetInlineUniformBlockEXT> m_cached_ds_write_iub_items_vk;
        mutable std::vector<VkWriteDescriptorSet>                      m_cached_ds_write_items_vk;

        mutable std::vector<DescriptorUpdateTemplateEntry>                                                     m_template_entries;
        mutable std::map<std::vector<DescriptorUpdateTemplateEntry>, Anvil::DescriptorUpdateTemplateUniquePtr> m_template_object_map;
        mutable std::vector<uint8_t>                                                                           m_template_raw_data;

        friend class Anvil::DescriptorPool;
    };
};

#endif /* WRAPPERS_DESCRIPTOR_SET_H */
