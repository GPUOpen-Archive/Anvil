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

#ifndef DESCRIPTOR_SET_LAYOUT_MANAGER_H
#define DESCRIPTOR_SET_LAYOUT_MANAGER_H

#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    class DescriptorSetLayoutManager : public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Destructor */
        ~DescriptorSetLayoutManager();

        bool get_layout(const DescriptorSetCreateInfo*       in_ds_create_info_ptr,
                        Anvil::DescriptorSetLayoutUniquePtr* out_ds_layout_ptr_ptr);

    protected:
        /* Protected functions */

    private:
        /* Private type declarations */
        typedef struct DescriptorSetLayoutContainer
        {
            DescriptorSetLayoutUniquePtr ds_layout_ptr;
            std::atomic<uint32_t>        n_references;

            DescriptorSetLayoutContainer()
                :n_references(1)
            {
                /* Stub */
            }
        } DescriptorSetLayoutContainer;

        typedef std::vector<std::unique_ptr<DescriptorSetLayoutContainer> > DescriptorSetLayouts;

        /* Private functions */
        DescriptorSetLayoutManager(const Anvil::BaseDevice* in_device_ptr,
                                   bool                     in_mt_safe);

        DescriptorSetLayoutManager           (const DescriptorSetLayoutManager&);
        DescriptorSetLayoutManager& operator=(const DescriptorSetLayoutManager&);

        void on_descriptor_set_layout_dereferenced(Anvil::DescriptorSetLayout* in_layout_ptr);

        static Anvil::DescriptorSetLayoutManagerUniquePtr create(const Anvil::BaseDevice* in_device_ptr,
                                                                 bool                     in_mt_safe);

        /* Private members */
        const Anvil::BaseDevice* m_device_ptr;
        DescriptorSetLayouts     m_descriptor_set_layouts;

        friend class BaseDevice;
    };
}; /* Vulkan namespace */

#endif /* DESCRIPTOR_SET_LAYOUT_MANAGER_H */