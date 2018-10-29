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

/** Defines a framebuffer wrapper class which simplifies the following processes:
 *
 *  - Framebuffer initialization and tear-down.
 *  - Life-time management
 *  - Support for adding new attachments with automatic Vulkan FB object re-creation.
 **/
#ifndef WRAPPERS_FRAMEBUFFER_H
#define WRAPPERS_FRAMEBUFFER_H

#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    class Framebuffer : public DebugMarkerSupportProvider<Framebuffer>,
                        public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        static Anvil::FramebufferUniquePtr create(Anvil::FramebufferCreateInfoUniquePtr in_create_info_ptr);

        /** Destructor.
         *
         *  Destroys baked Vulkan counterparts and unregisters the wrapper instance from the object tracker.
         **/
        ~Framebuffer();

        const Anvil::FramebufferCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        /** Returns a Vulkan framebuffer object instance for the specified render pass instance.
         *
         *  @param in_render_pass_ptr Render pass to return the framebuffer for.
         *
         *  @return Raw Vulkan framebuffer handle or VK_NULL_HANDLE, if the function failed.
         **/
        VkFramebuffer get_framebuffer(Anvil::RenderPass* in_render_pass_ptr);

    private:
        /* Private type declarations */
        typedef struct BakedFramebufferData
        {
            bool          dirty;
            VkFramebuffer framebuffer;

            BakedFramebufferData()
            {
                dirty       = false;
                framebuffer = VK_NULL_HANDLE;
            }
        } BakedFramebufferData;
        
        struct RenderPassComparator
        {
            bool operator()(Anvil::RenderPass* in_a_ptr,
                            Anvil::RenderPass* in_b_ptr) const;
        };

        typedef std::map<Anvil::RenderPass*, BakedFramebufferData, RenderPassComparator> BakedFramebufferMap;

        /* Private functions */

        /* Constructor. Please see specification of create() for more details */
        Framebuffer(Anvil::FramebufferCreateInfoUniquePtr in_create_info_ptr);

        Framebuffer& operator=(const Framebuffer&);
        Framebuffer           (const Framebuffer&);

        /** Re-creates a Vulkan framebuffer object for the specified render pass instance. If a FB
         *  has already been created in the past, the instance will be released.
         *
         *  At least one attachment must be defined for this function to succeed.
         *
         *  @param in_render_pass_ptr Render pass instance to create the new Vulkan framebuffer object
         *                            for. Must not be nullptr.
         *
         *  @return true if the function was successful, false otherwise.
         * */
        bool bake(Anvil::RenderPass* in_render_pass_ptr);

        /* Private members */
        BakedFramebufferMap                   m_baked_framebuffers;
        Anvil::FramebufferCreateInfoUniquePtr m_create_info_ptr;
        const Anvil::BaseDevice*              m_device_ptr;
    };
};

#endif /* WRAPPERS_FRAMEBUFFER_H */
