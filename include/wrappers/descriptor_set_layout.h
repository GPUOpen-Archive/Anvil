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
#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"
#include "wrappers/sampler.h"
#include <memory>

namespace Anvil
{
    /* Descriptor Set Layout wrapper */
    class DescriptorSetLayout : public DebugMarkerSupportProvider<DescriptorSetLayout>,
                                public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Destructor.
         *
         *  Destroys the Vulkan counterpart and unregisters the instance from the
         *  object tracker.
         **/
        virtual ~DescriptorSetLayout();

        /** Creates a new DescriptorSetLayout instance.
         *
         *  No Vulkan Descriptor Set Layout is instantiated at creation time. One will only be created
         *  at getter time.
         *
         *  @param in_device_ptr Device the layout will be created for.
         *
         **/
        static DescriptorSetLayoutUniquePtr create(DescriptorSetCreateInfoUniquePtr in_ds_create_info_ptr,
                                                   const Anvil::BaseDevice*         in_device_ptr,
                                                   MTSafety                         in_mt_safety = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE);

        const Anvil::DescriptorSetCreateInfo* get_create_info() const
        {
            return m_create_info_ptr.get();
        }

        /** Bakes a Vulkan object, if one is needed, and returns Vulkan DS layout handle.
         *
         *  Note that this call may invalidate previously returned layout handles, if the layout wrapper has
         *  been modified since the last getter call.
         *
         *  @return As per description.
         **/
        VkDescriptorSetLayout get_layout() const
        {
            anvil_assert(m_layout != VK_NULL_HANDLE);

            return m_layout;
        }

        /* Returns the maximum number of variable descriptor count binding size supported for the specified descriptor set layout.
         *
         * Requires VK_KHR_maintenance3 and VK_KHR_descriptor_indexing.
         *
         * @param in_ds_create_info_ptr Instance obtained by calling DescriptorSetInfo::create_descriptor_set_layout_create_info().
         *                              Must not be null.
         */
        static uint32_t get_maximum_variable_descriptor_count(const DescriptorSetLayoutCreateInfoContainer* in_ds_create_info_ptr,
                                                              const Anvil::BaseDevice*                      in_device_ptr);

        /* Checks if the specified descriptor set layout create info structure can be used to create a descriptor set layout instance.
         *
         * The app should call this function if the DS create info structure defines a number of descriptors that exceeds the 
         * VkPhysicalDeviceMaintenance3PropertiesKHR::maxPerSetDescriptors limit.
         *
         * Requires VK_KHR_maintenance3.
         *
         * @param in_ds_create_info_ptr Instance obtained by calling DescriptorSetInfo::create_descriptor_set_layout_create_info().
         *                              Must not be null.
         */
        static bool meets_max_per_set_descriptors_limit(const DescriptorSetLayoutCreateInfoContainer* in_ds_create_info_ptr,
                                                        const Anvil::BaseDevice*                      in_device_ptr);

    private:
        /* Private functions */

        /** Converts internal layout representation to a Vulkan object.
         *
         *  The baking will only occur if the object is internally marked as dirty. If it is not,
         *  the function does nothing.
         *
         *  @return true if successful, false otherwise.
         **/
        bool init();

        /* Please see create() documentation for more details */
        DescriptorSetLayout(DescriptorSetCreateInfoUniquePtr in_ds_create_info_ptr,
                            const Anvil::BaseDevice*         in_device_ptr,
                            bool                             in_mt_safe);

        DescriptorSetLayout           (const DescriptorSetLayout&);
        DescriptorSetLayout& operator=(const DescriptorSetLayout&);

        /* Private variables */
        DescriptorSetCreateInfoUniquePtr m_create_info_ptr;
        const Anvil::BaseDevice*         m_device_ptr;
        VkDescriptorSetLayout            m_layout;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_DESCRIPTOR_SET_LAYOUT */