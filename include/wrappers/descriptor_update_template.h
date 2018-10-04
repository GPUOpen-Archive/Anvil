//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef WRAPPERS_DESCRIPTOR_UPDATE_TEMPLATE_H
#define WRAPPERS_DESCRIPTOR_UPDATE_TEMPLATE_H

#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    /* Descriptor Update Template wrapper */
    class DescriptorUpdateTemplate : public DebugMarkerSupportProvider<DescriptorUpdateTemplate>,
                                     public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Destructor.
         *
         *  Destroys the Vulkan counterpart and unregisters the instance from the
         *  object tracker.
         **/
        virtual ~DescriptorUpdateTemplate();

        /** Creates a new DescriptorUpdateTemplate instance.
         *
         *  @param in_device_ptr                Device the layout will be created for. Must not be null.
         *  @param in_descriptor_set_layout_ptr DS layout to use as reference when creating the template.
         *                                      The function caches layout info internaly, so the object may be safely released
         *                                      after this function leaves. Must not be null.
         *
         **/
        static Anvil::DescriptorUpdateTemplateUniquePtr create_for_descriptor_set_updates(const Anvil::BaseDevice*                                 in_device_ptr,
                                                                                          const Anvil::DescriptorSetLayout*                        in_descriptor_set_layout_ptr,
                                                                                          const std::vector<Anvil::DescriptorUpdateTemplateEntry>& in_update_entries,
                                                                                          MTSafety                                                 in_mt_safety = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE)
        {
            return create_for_descriptor_set_updates(in_device_ptr,
                                                     in_descriptor_set_layout_ptr,
                                                     &in_update_entries.at(0),
                                                     static_cast<uint32_t>(in_update_entries.size() ),
                                                     in_mt_safety);
        }

        static Anvil::DescriptorUpdateTemplateUniquePtr create_for_descriptor_set_updates(const Anvil::BaseDevice*                    in_device_ptr,
                                                                                          const Anvil::DescriptorSetLayout*           in_descriptor_set_layout_ptr,
                                                                                          const Anvil::DescriptorUpdateTemplateEntry* in_update_entries_ptr,
                                                                                          const uint32_t&                             in_n_update_entries,
                                                                                          MTSafety                                    in_mt_safety = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE);

        /* Issues a MT-safe (if needed) vkUpdateDescriptorSetWithTeeplateKHR() call against the specified descriptor set. */
        void update_descriptor_set(const Anvil::DescriptorSet* inout_ds_ptr,
                                   const void*                 in_data_ptr) const;

    private:
        /* Private functions */

        bool init(const Anvil::DescriptorSetLayout*           in_descriptor_set_layout_ptr,
                  const Anvil::DescriptorUpdateTemplateEntry* in_update_entries_ptr,
                  const uint32_t&                             in_n_update_entries);

        /* Please see create() documentation for more details */
        DescriptorUpdateTemplate(const Anvil::BaseDevice* in_device_ptr,
                                 bool                     in_mt_safe);

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(DescriptorUpdateTemplate);
        ANVIL_DISABLE_COPY_CONSTRUCTOR   (DescriptorUpdateTemplate);

        /* Private variables */
        const Anvil::BaseDevice*                m_device_ptr;
        Anvil::DescriptorSetCreateInfoUniquePtr m_ds_create_info_ptr;
        VkDescriptorUpdateTemplateKHR           m_vk_object;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_DESCRIPTOR_UPDATE_TEMPLATE_H */
