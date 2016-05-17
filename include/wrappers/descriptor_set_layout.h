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

/** Implements a wrapper for a single Vulkan Descriptor Set Layout. Implemented in order to:
 *
 *  - encapsulate all layout-related state.
 *  - let ObjectTracker detect leaking layout wrapper instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_DESCRIPTOR_SET_LAYOUT_H
#define WRAPPERS_DESCRIPTOR_SET_LAYOUT_H

#include "misc/callbacks.h"
#include "misc/ref_counter.h"
#include "misc/types.h"
#include "wrappers/sampler.h"
#include <memory>

namespace Anvil
{
    enum
    {
        /* Notification fired whenever a new binding is added to the layout.
         *
         * callback argument: pointer to the originating DescriptorSetLayout instance
         **/
        DESCRIPTOR_SET_LAYOUT_CALLBACK_ID_BINDING_ADDED,

        /* Always last */
        DESCRIPTOR_SET_LAYOUT_CALLBACK_ID_COUNT
    };

    /* Descriptor Set Layout wrapper */
    class DescriptorSetLayout : public CallbacksSupportProvider,
                                public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  No Vulkan Descriptor Set Layout is instantiated at creation time. One will only be created
         *  at getter time.
         *
         *  @param device_ptr Device the layout will be created for.
         *
         **/
        DescriptorSetLayout(Anvil::Device* device_ptr);

        /** Adds a new binding to the layout instance.
         *
         *  It is an error to attempt to add a binding at an index, for which another binding has
         *  already been specified.
         *
         *  It is an error to attempt to define immutable samplers for descriptors of type other than
         *  sampler or combined image+sampler.
         *
         *  @param binding_index          Index of the binding to configure.
         *  @param descriptor_type        Type of the descriptor to use for the binding.
         *  @param descriptor_array_size  Size of the descriptor array to use for the binding.
         *  @param stage_flags            Rendering stages which are going to use the binding.
         *  @param immutable_sampler_ptrs If not nullptr, an array of @param descriptor_array_size samplers should
         *                                be passed. These sampler will be considered immutable, as per spec language.
         *                                May be nullptr.
         *
         *  @return true if successful, false otherwise.
         **/
        bool add_binding(uint32_t           binding_index,
                         VkDescriptorType   descriptor_type,
                         uint32_t           descriptor_array_size,
                         VkShaderStageFlags stage_flags,
                         Anvil::Sampler**   immutable_sampler_ptrs = nullptr);

        /** Converts internal layout representation to a Vulkan object.
         *
         *  The baking will only occur if the object is internally marked as dirty. If it is not,
         *  the function does nothing.
         *
         *  @return true if successful, false otherwise.
         **/
        bool bake();

        /** Retrieves properties of a single defined binding.
         *
         *  @param n_binding                              Index number of the binding to retrieve properties of.
         *  @param opt_out_binding_index_ptr              May be nullptr. If not, deref will be set to the index of the
         *                                                binding. This does NOT need to be equal to @param n_binding.
         *  @param opt_out_descriptor_type_ptr            May be nullptr. If not, deref will be set to the descriptor type
         *                                                for the specified binding.
         *  @param opt_out_descriptor_array_size_ptr      May be nullptr. If not, deref will be set to size of the descriptor
         *                                                array, associated with the specified binding.
         *  @param opt_out_stage_flags_ptr                May be nullptr. If not, deref will be set to stage flags,
         *                                                as configured for the specified binding.
         *  @param opt_out_immutable_samplers_enabled_ptr May be nullptr. If not, deref will be set to true if immutable samplers
         *                                                have been defined for the specified binding; otherwise, it will be
         *                                                set to false.
         *
         *  @return true if successful, false otherwise.
         **/
        bool get_binding_properties(uint32_t            n_binding,
                                    uint32_t*           opt_out_binding_index_ptr,
                                    VkDescriptorType*   opt_out_descriptor_type_ptr,
                                    uint32_t*           opt_out_descriptor_array_size_ptr,
                                    VkShaderStageFlags* opt_out_stage_flags_ptr,
                                    bool*               opt_out_immutable_samplers_enabled_ptr);

        /** Bakes a Vulkan object, if one is needed, and returns Vulkan DS layout handle.
         *
         *  Note that this call may invalidate previously returned layout handles, if the layout wrapper has
         *  been modified since the last getter call.
         *
         *  @return As per description.
         **/
        VkDescriptorSetLayout get_layout()
        {
            if (m_dirty)
            {
                bake();
            }

            return m_layout;
        }

        /** Returns the number of bindings defined for the layout. */
        uint32_t get_n_bindings() const
        {
            return static_cast<uint32_t>(m_bindings.size() );
        }

    private:
        /* Private type definitions */

        /** Describes a single descriptor set layout binding */
        typedef struct Binding
        {
            uint32_t                                      descriptor_array_size;
            VkDescriptorType                              descriptor_type;
            std::vector<std::shared_ptr<Anvil::Sampler> > immutable_samplers;
            VkShaderStageFlagBits                         stage_flags;

            /** Dummy constructor. Do not use. */
            Binding()
            {
                descriptor_array_size = 0;
                descriptor_type       = VK_DESCRIPTOR_TYPE_MAX_ENUM;
                stage_flags           = static_cast<VkShaderStageFlagBits>(0);
            }

            /** Constructor.
             *
             *  For argument discussion, please see Anvil::DescriptorSetLayout::add_binding() documentation.
             **/
            Binding(uint32_t           in_descriptor_array_size,
                    VkDescriptorType   in_descriptor_type,
                    VkShaderStageFlags in_stage_flags,
                    Anvil::Sampler**   in_immutable_sampler_ptrs)
            {
                descriptor_array_size = in_descriptor_array_size;
                descriptor_type       = in_descriptor_type;
                stage_flags           = static_cast<VkShaderStageFlagBits>(in_stage_flags);

                if (in_immutable_sampler_ptrs != nullptr)
                {
                    for (uint32_t n_sampler = 0;
                                  n_sampler < descriptor_array_size;
                                ++n_sampler)
                    {
                        immutable_samplers.push_back(std::shared_ptr<Anvil::Sampler>(in_immutable_sampler_ptrs[n_sampler],
                                                                                     Anvil::SamplerDeleter() ));

                        in_immutable_sampler_ptrs[n_sampler]->retain();
                    }
                }
            }
        } Binding;

        typedef std::map<BindingIndex, Binding> BindingIndexToBindingMap;

        /* Private functions */
        DescriptorSetLayout           (const DescriptorSetLayout&);
        DescriptorSetLayout& operator=(const DescriptorSetLayout&);

        virtual ~DescriptorSetLayout();

        /* Private variables */
        BindingIndexToBindingMap m_bindings;
        Anvil::Device*           m_device_ptr;
        bool                     m_dirty;
        VkDescriptorSetLayout    m_layout;
    };

    /* Delete functor. Useful for wrapping DescriptorSetLayout instances in auto pointers. */
    struct DescriptorSetLayoutDeleter
    {
        bool operator()(DescriptorSetLayout* layout_ptr)
        {
            layout_ptr->release();
        }
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_DESCRIPTOR_SET_LAYOUT */