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

#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "wrappers/device.h"
#include "wrappers/framebuffer.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/render_pass.h"
#include <algorithm>

/** Please see header for specification */
Anvil::Framebuffer::FramebufferAttachment::FramebufferAttachment(ImageView* in_image_view_ptr)
{
    anvil_assert(in_image_view_ptr != nullptr);

    image_view_ptr = in_image_view_ptr;
    in_image_view_ptr->retain();
}

/** Please see header for specification */
Anvil::Framebuffer::FramebufferAttachment::FramebufferAttachment(const FramebufferAttachment& in)
{
    image_view_ptr = in.image_view_ptr;

    image_view_ptr->retain();
}

/** Please see header for specification */
Anvil::Framebuffer::FramebufferAttachment& Anvil::Framebuffer::FramebufferAttachment::operator=(const Anvil::Framebuffer::FramebufferAttachment& in)
{
    image_view_ptr = in.image_view_ptr;

    image_view_ptr->retain();
    return *this;
}

/** Destructor. Releases the wrapped image view instance. */
Anvil::Framebuffer::FramebufferAttachment::~FramebufferAttachment()
{
    if (image_view_ptr != nullptr)
    {
        image_view_ptr->release();

        image_view_ptr = nullptr;
    }
}

/* Please see header for specification */
Anvil::Framebuffer::Framebuffer(Anvil::Device* device_ptr,
                                 uint32_t       width,
                                 uint32_t       height,
                                 uint32_t       n_layers)
    :m_device_ptr(device_ptr)
{
    anvil_assert(width    >= 1);
    anvil_assert(height   >= 1);
    anvil_assert(n_layers >= 1);

    m_framebuffer_size[0] = width;
    m_framebuffer_size[1] = height;
    m_framebuffer_size[2] = n_layers;

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::ObjectTracker::OBJECT_TYPE_FRAMEBUFFER,
                                                 this);
}

/** Destructor.
 *
 *  Releases the wrapped Vulkan framebuffer objects. Only called if the internal
 *  reference counter drops to zero.
 **/
Anvil::Framebuffer::~Framebuffer()
{
    for (auto fb_iterator  = m_baked_framebuffers.begin();
              fb_iterator != m_baked_framebuffers.end();
            ++fb_iterator)
    {
        anvil_assert(fb_iterator->second.framebuffer != VK_NULL_HANDLE);

        /* Destroy the Vulkan framebuffer object */
        vkDestroyFramebuffer(m_device_ptr->get_device_vk(),
                             fb_iterator->second.framebuffer,
                             nullptr /* pAllocator */);

        /* Carry on and release the renderpass the framebuffer had been baked for */
        fb_iterator->first->unregister_from_callbacks(RENDER_PASS_CALLBACK_ID_BAKING_NEEDED,
                                                      on_renderpass_changed,
                                                      this);

        fb_iterator->first->release();
    }

    m_attachments.clear();

    /* Unregister the object */
    Anvil::ObjectTracker::get()->unregister_object(Anvil::ObjectTracker::OBJECT_TYPE_FRAMEBUFFER,
                                                    this);
}

/* Please see header for specification */
bool Anvil::Framebuffer::add_attachment(ImageView*               image_view_ptr,
                                        FramebufferAttachmentID* out_opt_attachment_id_ptr)
{
    const Anvil::Image* parent_image_ptr = nullptr;
    bool                 result           = false;
    uint32_t             view_size[3];

    /* Sanity checks: Input image view must not be nullptr */
    anvil_assert(image_view_ptr != nullptr);

    /* Sanity checks: Input image view should not be a duplicate of already added attachments.
     *                This is not required by Vulkan spec, but simplifies implementation of the
     *                DAGRenderer class.
     **/
    for (auto fb_attachment_iterator  = m_attachments.cbegin();
              fb_attachment_iterator != m_attachments.cend();
            ++fb_attachment_iterator)
    {
        /* Operate on Vulkan handles, instead of objects, in case there are two wrapper instances
         * referring to the same Vulkan object */
        if (fb_attachment_iterator->image_view_ptr->get_image_view() == image_view_ptr->get_image_view() )
        {
            anvil_assert(false);

            goto end;
        }
    }

    /* Sanity checks: make sure the image views have the same, or larger size than the framebuffer's. */
    parent_image_ptr = image_view_ptr->get_parent_image();

    anvil_assert(parent_image_ptr != nullptr);

    if (!parent_image_ptr->get_image_mipmap_size(image_view_ptr->get_base_mipmap_level(),
                                                 view_size + 0,
                                                 view_size + 1,
                                                 view_size + 2) )
    {
        /* Why is the mipmap index invalid? */
        anvil_assert(false);

        goto end;
    }

    if (view_size[0] < m_framebuffer_size[0] ||
        view_size[1] < m_framebuffer_size[1] ||
        view_size[2] < m_framebuffer_size[2])
    {
        /* Attachment size is wrong */
        anvil_assert(false);

        goto end;
    }

    /* Spawn & store a new descriptor for the attachment. */
    if (out_opt_attachment_id_ptr != nullptr)
    {
        *out_opt_attachment_id_ptr = (FramebufferAttachmentID) m_attachments.size();
    }

    m_attachments.push_back(FramebufferAttachment(image_view_ptr) );

    /* Mark all baked framebuffers as dirty. */
    for (auto baked_fb_iterator  = m_baked_framebuffers.begin();
              baked_fb_iterator != m_baked_framebuffers.end();
            ++baked_fb_iterator)
    {
        baked_fb_iterator->second.dirty = true;
    }

    /* All done */
    result  = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::Framebuffer::bake(Anvil::RenderPass* render_pass_ptr)
{
    BakedFramebufferMap::iterator baked_fb_iterator;
    VkFramebufferCreateInfo       fb_create_info;
    std::vector<VkImageView>      image_view_attachments;
    bool                          result    = false;
    VkFramebuffer                 result_fb = VK_NULL_HANDLE;
    VkResult                      result_vk;

    /* Sanity check: Input render pass is not nullptr */
    if (render_pass_ptr == nullptr)
    {
        anvil_assert(false);

        goto end;
    }

    /* Sanity checks: Make sure none of the FB size dimensions are 0 */
    if (m_framebuffer_size[0] < 1 ||
        m_framebuffer_size[1] < 1 ||
        m_framebuffer_size[2] < 1)
    {
        anvil_assert(false);

        goto end;
    }

    /* Release the existing Vulkan object handle, if one is already present */
    baked_fb_iterator = m_baked_framebuffers.find(render_pass_ptr);

    if (baked_fb_iterator != m_baked_framebuffers.end() )
    {
        vkDestroyFramebuffer(m_device_ptr->get_device_vk(),
                             baked_fb_iterator->second.framebuffer,
                             nullptr /* pAllocator */);

        m_baked_framebuffers.erase(baked_fb_iterator);
    }

    /* Prepare the image view array we will use as input for the create info descriptor */
    image_view_attachments.reserve(m_attachments.size() );

    for (auto attachment_iterator  = m_attachments.begin();
              attachment_iterator != m_attachments.end();
            ++attachment_iterator)
    {
        image_view_attachments.push_back(attachment_iterator->image_view_ptr->get_image_view() );
    }

    /* Prepare the create info descriptor */
    fb_create_info.attachmentCount = (uint32_t) image_view_attachments.size();
    fb_create_info.flags           = 0;
    fb_create_info.height          = m_framebuffer_size[1];
    fb_create_info.layers          = m_framebuffer_size[2];
    fb_create_info.pAttachments    = (fb_create_info.attachmentCount > 0) ? &image_view_attachments[0]
                                                                          : nullptr;
    fb_create_info.pNext           = nullptr;
    fb_create_info.renderPass      = render_pass_ptr->get_render_pass();
    fb_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_create_info.width           = m_framebuffer_size[0];

    /* Create the framebuffer instance and store it */
    result_vk = vkCreateFramebuffer(m_device_ptr->get_device_vk(),
                                  &fb_create_info,
                                   nullptr, /* pAllocator */
                                  &result_fb);

    anvil_assert_vk_call_succeeded(result_vk);
    anvil_assert                  (result_fb != VK_NULL_HANDLE);

    m_baked_framebuffers[render_pass_ptr].dirty       = false;
    m_baked_framebuffers[render_pass_ptr].framebuffer = result_fb;

    /* If the render pass is ever changed, make sure we re-bake the framebuffer when needed */
    render_pass_ptr->retain                ();
    render_pass_ptr->register_for_callbacks(RENDER_PASS_CALLBACK_ID_BAKING_NEEDED,
                                           &on_renderpass_changed,
                                           this);

    /* All done */
    result = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::Framebuffer::get_attachment_id_for_image_view(ImageView*               image_view_ptr,
                                                          FramebufferAttachmentID* out_attachment_id_ptr)
{
    bool result              = false;
    auto attachment_iterator = std::find(m_attachments.cbegin(),
                                         m_attachments.cend(),
                                         image_view_ptr);

    if (attachment_iterator != m_attachments.end() )
    {
        *out_attachment_id_ptr = static_cast<Anvil::FramebufferAttachmentID>(attachment_iterator - m_attachments.begin() );
         result                = true;
    }

    return result;
}

/* Please see header for specification */
const VkFramebuffer Anvil::Framebuffer::get_framebuffer(Anvil::RenderPass* render_pass_ptr)
{
    auto          fb_iterator = m_baked_framebuffers.find(render_pass_ptr);
    VkFramebuffer result_fb   = VK_NULL_HANDLE;

    if (fb_iterator == m_baked_framebuffers.end() ||
        fb_iterator->second.dirty)
    {
        /* Need to bake the object.. */
        bool result = bake(render_pass_ptr);

        anvil_assert(result);

        if (!result)
        {
            goto end;
        }

        fb_iterator = m_baked_framebuffers.find(render_pass_ptr);

        if (fb_iterator == m_baked_framebuffers.end() )
        {
            /* No luck. */
            anvil_assert(false);

            goto end;
        }

        anvil_assert(!fb_iterator->second.dirty);
    }

    result_fb = fb_iterator->second.framebuffer;

end:
    return result_fb;
}

/** Called back whenever any renderpass using this framebuffer instance is changed.
 *
 *  Marks the framebuffer as dirty, forcing the framebuffer to be rebaked next time it is used.
 *
 *  @param raw_renderpass_ptr  RenderPass instance which has invoked the call-back. Never nullptr.
 *  @param raw_framebuffer_ptr Framebuffer instance this call-back is fired against. Never nullptr.
 *
 **/
void Anvil::Framebuffer::on_renderpass_changed(void* raw_renderpass_ptr,
                                               void* raw_framebuffer_ptr)
{
    Framebuffer* framebuffer_ptr   = reinterpret_cast<Framebuffer*>(raw_framebuffer_ptr);
    RenderPass*  renderpass_ptr    = reinterpret_cast<RenderPass*> (raw_renderpass_ptr);
    auto         baked_fb_iterator = framebuffer_ptr->m_baked_framebuffers.find(renderpass_ptr);

    if (baked_fb_iterator == framebuffer_ptr->m_baked_framebuffers.end() )
    {
        anvil_assert(false);

        goto end;
    }

    /* Mark as dirty so that the framebuffer is rebaked the next time it is requested. */
    baked_fb_iterator->second.dirty = true;
end:
    ;
}
