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

#include "misc/debug.h"
#include "misc/framebuffer_create_info.h"
#include "misc/object_tracker.h"
#include "misc/render_pass_create_info.h"
#include "wrappers/device.h"
#include "wrappers/framebuffer.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/render_pass.h"
#include <algorithm>

bool Anvil::Framebuffer::RenderPassComparator::operator()(Anvil::RenderPass* in_a_ptr,
                                                          Anvil::RenderPass* in_b_ptr) const
{
    return in_a_ptr->get_render_pass() < in_b_ptr->get_render_pass();
}


/* Please see header for specification */
Anvil::Framebuffer::Framebuffer(Anvil::FramebufferCreateInfoUniquePtr in_create_info_ptr)
    :DebugMarkerSupportProvider(in_create_info_ptr->get_device(),
                                Anvil::ObjectType::FRAMEBUFFER,
                                true), /* in_use_delegate_workers */
     MTSafetySupportProvider   (Anvil::Utils::convert_mt_safety_enum_to_boolean(in_create_info_ptr->get_mt_safety(),
                                                                                in_create_info_ptr->get_device   () )),
     m_device_ptr              (in_create_info_ptr->get_device                 () )
{
    m_create_info_ptr = std::move(in_create_info_ptr);

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectType::FRAMEBUFFER,
                                                 this);
}

/** Destructor.
 *
 *  Releases the wrapped Vulkan framebuffer objects. Only called if the internal
 *  reference counter drops to zero.
 **/
Anvil::Framebuffer::~Framebuffer()
{
    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectType::FRAMEBUFFER,
                                                    this);

    for (auto fb_iterator  = m_baked_framebuffers.begin();
              fb_iterator != m_baked_framebuffers.end();
            ++fb_iterator)
    {
        anvil_assert(fb_iterator->second.framebuffer != VK_NULL_HANDLE);

        /* Destroy the Vulkan framebuffer object */
        lock();
        {
            Anvil::Vulkan::vkDestroyFramebuffer(m_device_ptr->get_device_vk(),
                                                fb_iterator->second.framebuffer,
                                                nullptr /* pAllocator */);
        }
        unlock();
    }

    m_baked_framebuffers.clear();
}

/* Please see header for specification */
bool Anvil::Framebuffer::bake(Anvil::RenderPass* in_render_pass_ptr)
{
    BakedFramebufferMap::iterator baked_fb_iterator;
    VkFramebufferCreateInfo       fb_create_info;
    std::vector<VkImageView>      image_view_attachments;
    const auto                    n_attachments          = m_create_info_ptr->get_n_attachments();
    bool                          result                 = false;
    VkFramebuffer                 result_fb              = VK_NULL_HANDLE;
    VkResult                      result_vk              = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    /* Sanity check: Input render pass is not nullptr */
    if (in_render_pass_ptr == nullptr)
    {
        anvil_assert_fail();

        goto end;
    }

    /* Release the existing Vulkan object handle, if one is already present
     *
     * TODO: Leverage the concept of render pass compatibility here!
     */
    baked_fb_iterator = m_baked_framebuffers.find(in_render_pass_ptr);

    if (baked_fb_iterator != m_baked_framebuffers.end() )
    {
        lock();
        {
            Anvil::Vulkan::vkDestroyFramebuffer(m_device_ptr->get_device_vk(),
                                                baked_fb_iterator->second.framebuffer,
                                                nullptr /* pAllocator */);
        }
        unlock();

        DebugMarkerSupportProvider::remove_delegate(baked_fb_iterator->second.framebuffer);

        m_baked_framebuffers.erase(baked_fb_iterator);
    }

    /* Prepare the image view array we will use as input for the create info descriptor */
    image_view_attachments.reserve(n_attachments);

    for (uint32_t n_attachment = 0;
                  n_attachment < n_attachments;
                ++n_attachment)
    {
        Anvil::ImageView* image_view_ptr = nullptr;

        if (!m_create_info_ptr->get_attachment_at_index(n_attachment,
                                                       &image_view_ptr) )
        {
            anvil_assert_fail();

            goto end;
        }

        anvil_assert(image_view_ptr != nullptr);

        image_view_attachments.push_back(image_view_ptr->get_image_view() );
    }

    /* Prepare the create info descriptor */
    anvil_assert(in_render_pass_ptr->get_render_pass_create_info()->get_n_attachments() == image_view_attachments.size() );

    fb_create_info.attachmentCount = static_cast<uint32_t>(image_view_attachments.size() );
    fb_create_info.flags           = 0;
    fb_create_info.height          = m_create_info_ptr->get_height  ();
    fb_create_info.layers          = m_create_info_ptr->get_n_layers();
    fb_create_info.pAttachments    = (fb_create_info.attachmentCount > 0) ? &image_view_attachments[0]
                                                                          : nullptr;
    fb_create_info.pNext           = nullptr;
    fb_create_info.renderPass      = in_render_pass_ptr->get_render_pass();
    fb_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_create_info.width           = m_create_info_ptr->get_width();

    /* Create the framebuffer instance and store it */
    result_vk = Anvil::Vulkan::vkCreateFramebuffer(m_device_ptr->get_device_vk(),
                                                  &fb_create_info,
                                                   nullptr, /* pAllocator */
                                                  &result_fb);

    anvil_assert_vk_call_succeeded(result_vk);
    if (is_vk_call_successful(result_vk) )
    {
        anvil_assert(result_fb != VK_NULL_HANDLE);

        m_baked_framebuffers[in_render_pass_ptr].dirty       = false;
        m_baked_framebuffers[in_render_pass_ptr].framebuffer = result_fb;

        DebugMarkerSupportProvider::add_delegate(result_fb);
    }

    /* All done */
    result = true;

end:
    return result;
}

/* Please see header for specification */
Anvil::FramebufferUniquePtr Anvil::Framebuffer::create(Anvil::FramebufferCreateInfoUniquePtr in_create_info_ptr)
{
    Anvil::FramebufferUniquePtr result_ptr(nullptr,
                                           std::default_delete<Anvil::Framebuffer>() );

    result_ptr.reset(
        new Anvil::Framebuffer(
            std::move(in_create_info_ptr)
        )
    );

    return result_ptr;
}

/* Please see header for specification */
VkFramebuffer Anvil::Framebuffer::get_framebuffer(Anvil::RenderPass* in_render_pass_ptr)
{
    auto          fb_iterator = m_baked_framebuffers.find(in_render_pass_ptr);
    VkFramebuffer result_fb   = VK_NULL_HANDLE;

    if (fb_iterator == m_baked_framebuffers.end() ||
        fb_iterator->second.dirty)
    {
        /* Need to bake the object.. */
        bool result = bake(in_render_pass_ptr);

        anvil_assert(result);

        if (!result)
        {
            goto end;
        }

        fb_iterator = m_baked_framebuffers.find(in_render_pass_ptr);

        if (fb_iterator == m_baked_framebuffers.end() )
        {
            /* No luck. */
            anvil_assert_fail();

            goto end;
        }

        anvil_assert(!fb_iterator->second.dirty);
    }

    result_fb = fb_iterator->second.framebuffer;

end:
    return result_fb;
}
