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

#ifndef WRAPPERS_RENDER_PASS_H
#define WRAPPERS_RENDER_PASS_H

#include "misc/callbacks.h"
#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    class RenderPass : public DebugMarkerSupportProvider<RenderPass>
    {
    public:
        /* Public functions */

        /** Creates a new RenderPass wrapper instance.
         *
         *  NOTE: This function takes a Swapchain instance, which is later passed to GFX pipelines at creation time.
         *        The actual Swapchain wrapper is only used, if the GFX pipeline is not specified a scissor area and/or
         *        viewport size, in which case it needs to deduce that info from the swapchain.
         *
         *        Passing a non-nullptr value through the <opt_swapchain_ptr> argument is optional, in which case Anvil will
         *        throw a NPE if it ever needs to deduce the window size.
         *
         *
         *  @param in_renderpass_create_info_ptr TODO
         *  @param in_opt_swapchain_ptr          Swapchain, that the render-pass will render to. See brief for more information.
         *                                       May be nullptr.
         *
         **/
        static Anvil::RenderPassUniquePtr create(Anvil::RenderPassCreateInfoUniquePtr in_renderpass_create_info_ptr,
                                                 Anvil::Swapchain*                    in_opt_swapchain_ptr);

        /** Destructor.
         *
         *  Destroys the Vulkan counterpart and unregisters the wrapper instance from the object tracker.
         **/
        virtual ~RenderPass();

        
        VkRenderPass get_render_pass() const
        {
            anvil_assert(m_render_pass != VK_NULL_HANDLE);

            return m_render_pass;
        }

        const Anvil::RenderPassCreateInfo* get_render_pass_create_info() const
        {
            return m_render_pass_create_info_ptr.get();
        }

        /** Returns the Swapchain instance, associated with the RenderPass wrapper instance at creation time. */
        Anvil::Swapchain* get_swapchain() const
        {
            return m_swapchain_ptr;
        }

    private:
        /* Private type definitions */
        
        /* Private functions */

        bool init_using_core_vk10           ();
        bool init_using_rp2_create_extension();

        /* Constructor. Please see create() for specification */
        RenderPass(Anvil::RenderPassCreateInfoUniquePtr in_renderpass_create_info_ptr,
                   Anvil::Swapchain*                    in_opt_swapchain_ptr);

        RenderPass& operator=(const RenderPass&);
        RenderPass           (const RenderPass&);

        /* Private members */
        VkRenderPass                         m_render_pass;
        Anvil::RenderPassCreateInfoUniquePtr m_render_pass_create_info_ptr;
        Swapchain*                           m_swapchain_ptr;
    };
};

#endif /* WRAPPERS_RENDER_PASS_H */