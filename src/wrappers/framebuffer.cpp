//
// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/render_pass_info.h"
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


/** Please see header for specification */
Anvil::Framebuffer::FramebufferAttachment::FramebufferAttachment(ImageView* in_image_view_ptr)
{
    anvil_assert(in_image_view_ptr != nullptr);

    image_view_ptr = in_image_view_ptr;
}

/** Please see header for specification */
Anvil::Framebuffer::FramebufferAttachment::FramebufferAttachment(const FramebufferAttachment& in)
{
    image_view_ptr = in.image_view_ptr;
}

/** Please see header for specification */
Anvil::Framebuffer::FramebufferAttachment& Anvil::Framebuffer::FramebufferAttachment::operator=(const Anvil::Framebuffer::FramebufferAttachment& in)
{
    image_view_ptr = in.image_view_ptr;

    return *this;
}

/** Destructor. Releases the wrapped image view instance. */
Anvil::Framebuffer::FramebufferAttachment::~FramebufferAttachment()
{
    /* Stub */
}

/* Please see header for specification */
Anvil::Framebuffer::Framebuffer(const Anvil::BaseDevice* in_device_ptr,
                                uint32_t                 in_width,
                                uint32_t                 in_height,
                                uint32_t                 in_n_layers,
                                bool                     in_mt_safe)
    :DebugMarkerSupportProvider(in_device_ptr,
                                VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT,
                                true), /* in_use_delegate_workers */
     MTSafetySupportProvider   (in_mt_safe),
     m_device_ptr              (in_device_ptr)
{
    anvil_assert(in_width    >= 1);
    anvil_assert(in_height   >= 1);
    anvil_assert(in_n_layers >= 1);

    m_framebuffer_size[0] = in_width;
    m_framebuffer_size[1] = in_height;
    m_framebuffer_size[2] = in_n_layers;

    /* Register the object */
    Anvil::ObjectTracker::get()->register_object(Anvil::OBJECT_TYPE_FRAMEBUFFER,
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
    Anvil::ObjectTracker::get()->unregister_object(Anvil::OBJECT_TYPE_FRAMEBUFFER,
                                                    this);

    for (auto fb_iterator  = m_baked_framebuffers.begin();
              fb_iterator != m_baked_framebuffers.end();
            ++fb_iterator)
    {
        anvil_assert(fb_iterator->second.framebuffer != VK_NULL_HANDLE);

        /* Destroy the Vulkan framebuffer object */
        lock();
        {
            vkDestroyFramebuffer(m_device_ptr->get_device_vk(),
                                 fb_iterator->second.framebuffer,
                                 nullptr /* pAllocator */);
        }
        unlock();
    }

    m_baked_framebuffers.clear();
    m_attachments.clear();
}

/* Please see header for specification */
bool Anvil::Framebuffer::add_attachment(ImageView*               in_image_view_ptr,
                                        FramebufferAttachmentID* out_opt_attachment_id_ptr)
{
    const Anvil::Image* parent_image_ptr;
    bool                result           = false;
    uint32_t            view_size[3];

    /* Sanity checks: Input image view must not be nullptr */
    anvil_assert(in_image_view_ptr != nullptr);

    /* Sanity checks: make sure the image views have the same, or larger size than the framebuffer's. */
    parent_image_ptr = in_image_view_ptr->get_parent_image();

    anvil_assert(parent_image_ptr != nullptr);

    if (!parent_image_ptr->get_image_mipmap_size(in_image_view_ptr->get_base_mipmap_level(),
                                                 view_size + 0,
                                                 view_size + 1,
                                                 view_size + 2) )
    {
        /* Why is the mipmap index invalid? */
        anvil_assert_fail();

        goto end;
    }

    if (view_size[0] < m_framebuffer_size[0] ||
        view_size[1] < m_framebuffer_size[1])
    {
        /* Attachment size is wrong */
        anvil_assert_fail();

        goto end;
    }

    /* Spawn & store a new descriptor for the attachment. */
    if (out_opt_attachment_id_ptr != nullptr)
    {
        *out_opt_attachment_id_ptr = static_cast<FramebufferAttachmentID>(m_attachments.size() );
    }

    m_attachments.push_back(FramebufferAttachment(in_image_view_ptr) );

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
bool Anvil::Framebuffer::bake(Anvil::RenderPass* in_render_pass_ptr)
{
    BakedFramebufferMap::iterator baked_fb_iterator;
    VkFramebufferCreateInfo       fb_create_info;
    std::vector<VkImageView>      image_view_attachments;
    bool                          result    = false;
    VkFramebuffer                 result_fb = VK_NULL_HANDLE;
    VkResult                      result_vk = VK_ERROR_INITIALIZATION_FAILED;

    ANVIL_REDUNDANT_VARIABLE(result_vk);

    /* Sanity check: Input render pass is not nullptr */
    if (in_render_pass_ptr == nullptr)
    {
        anvil_assert_fail();

        goto end;
    }

    /* Sanity checks: Make sure none of the FB size dimensions are 0 */
    if (m_framebuffer_size[0] < 1 ||
        m_framebuffer_size[1] < 1 ||
        m_framebuffer_size[2] < 1)
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
            vkDestroyFramebuffer(m_device_ptr->get_device_vk(),
                                 baked_fb_iterator->second.framebuffer,
                                 nullptr /* pAllocator */);
        }
        unlock();

        DebugMarkerSupportProvider::remove_delegate(baked_fb_iterator->second.framebuffer);

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
    anvil_assert(in_render_pass_ptr->get_render_pass_info()->get_n_attachments() == image_view_attachments.size() );

    fb_create_info.attachmentCount = static_cast<uint32_t>(image_view_attachments.size() );
    fb_create_info.flags           = 0;
    fb_create_info.height          = m_framebuffer_size[1];
    fb_create_info.layers          = m_framebuffer_size[2];
    fb_create_info.pAttachments    = (fb_create_info.attachmentCount > 0) ? &image_view_attachments[0]
                                                                          : nullptr;
    fb_create_info.pNext           = nullptr;
    fb_create_info.renderPass      = in_render_pass_ptr->get_render_pass();
    fb_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_create_info.width           = m_framebuffer_size[0];

    /* Create the framebuffer instance and store it */
    result_vk = vkCreateFramebuffer(m_device_ptr->get_device_vk(),
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
Anvil::FramebufferUniquePtr Anvil::Framebuffer::create(const Anvil::BaseDevice* in_device_ptr,
                                                       uint32_t                 in_width,
                                                       uint32_t                 in_height,
                                                       uint32_t                 in_n_layers,
                                                       MTSafety                 in_mt_safety)
{
    const bool                  mt_safe    = Anvil::Utils::convert_mt_safety_enum_to_boolean(in_mt_safety,
                                                                                             in_device_ptr);
    Anvil::FramebufferUniquePtr result_ptr(nullptr,
                                           std::default_delete<Anvil::Framebuffer>() );

    result_ptr.reset(
        new Anvil::Framebuffer(in_device_ptr,
                               in_width,
                               in_height,
                               in_n_layers,
                               mt_safe)
    );

    return result_ptr;
}

/* Please see header for specification */
bool Anvil::Framebuffer::get_attachment_at_index(uint32_t    in_attachment_index,
                                                 ImageView** out_image_view_ptr_ptr) const
{
    bool result = false;

    if (m_attachments.size() <= in_attachment_index)
    {
        goto end;
    }
    else
    {
        *out_image_view_ptr_ptr = m_attachments[in_attachment_index].image_view_ptr;
        result                  = true;
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::Framebuffer::get_attachment_id_for_image_view(ImageView*               in_image_view_ptr,
                                                          FramebufferAttachmentID* out_attachment_id_ptr)
{
    bool result              = false;
    auto attachment_iterator = std::find(m_attachments.cbegin(),
                                         m_attachments.cend(),
                                         in_image_view_ptr);

    if (attachment_iterator != m_attachments.end() )
    {
        *out_attachment_id_ptr = static_cast<Anvil::FramebufferAttachmentID>(attachment_iterator - m_attachments.begin() );
         result                = true;
    }

    return result;
}

/* Please see header for specification */
const VkFramebuffer Anvil::Framebuffer::get_framebuffer(Anvil::RenderPass* in_render_pass_ptr)
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

/* Please see header for specification */
void Anvil::Framebuffer::get_size(uint32_t* out_framebuffer_width_ptr,
                                  uint32_t* out_framebuffer_height_ptr,
                                  uint32_t* out_framebuffer_depth_ptr) const
{
    *out_framebuffer_width_ptr  = m_framebuffer_size[0];
    *out_framebuffer_height_ptr = m_framebuffer_size[1];
    *out_framebuffer_depth_ptr  = m_framebuffer_size[2];
}
