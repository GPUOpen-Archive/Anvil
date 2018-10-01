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
#include "misc/framebuffer_create_info.h"
#include "misc/image_view_create_info.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"

Anvil::FramebufferCreateInfo::FramebufferAttachment::FramebufferAttachment(ImageView* in_image_view_ptr)
    :image_view_ptr(in_image_view_ptr)
{
    anvil_assert(in_image_view_ptr != nullptr);
}

Anvil::FramebufferCreateInfo::FramebufferAttachment::~FramebufferAttachment()
{
    /* Stub */
}

Anvil::FramebufferCreateInfo::FramebufferAttachment::FramebufferAttachment(const FramebufferAttachment& in)
{
    image_view_ptr = in.image_view_ptr;
}

Anvil::FramebufferCreateInfo::FramebufferAttachment& Anvil::FramebufferCreateInfo::FramebufferAttachment::operator=(const Anvil::FramebufferCreateInfo::FramebufferAttachment& in)
{
    image_view_ptr = in.image_view_ptr;

    return *this;
}

Anvil::FramebufferCreateInfo::FramebufferCreateInfo(const Anvil::BaseDevice* in_device_ptr,
                                                    const uint32_t&          in_width,
                                                    const uint32_t&          in_height,
                                                    const uint32_t&          in_n_layers,
                                                    MTSafety                 in_mt_safety)
    :m_device_ptr(in_device_ptr),
     m_height    (in_height),
     m_mt_safety (in_mt_safety),
     m_n_layers  (in_n_layers),
     m_width     (in_width)
{
    anvil_assert(in_device_ptr != nullptr);
    anvil_assert(in_height     >= 1);
    anvil_assert(in_n_layers   >= 1);
    anvil_assert(in_width      >= 1);
}

bool Anvil::FramebufferCreateInfo::add_attachment(ImageView*               in_image_view_ptr,
                                                  FramebufferAttachmentID*  out_opt_attachment_id_ptr)
{
    const Anvil::Image* parent_image_ptr;
    bool                result           = false;
    uint32_t            view_size[3];

    /* Sanity checks: Input image view must not be nullptr */
    anvil_assert(in_image_view_ptr != nullptr);

    /* Sanity checks: make sure the image views have the same, or larger size than the framebuffer's. */
    parent_image_ptr = in_image_view_ptr->get_create_info_ptr()->get_parent_image();

    anvil_assert(parent_image_ptr != nullptr);

    if (!parent_image_ptr->get_image_mipmap_size(in_image_view_ptr->get_create_info_ptr()->get_base_mipmap_level(),
                                                 view_size + 0,
                                                 view_size + 1,
                                                 view_size + 2) )
    {
        /* Why is the mipmap index invalid? */
        anvil_assert_fail();

        goto end;
    }

    if (view_size[0] < m_width  ||
        view_size[1] < m_height)
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

    /* All done */
    result  = true;

end:
    return result;
}

Anvil::FramebufferCreateInfoUniquePtr Anvil::FramebufferCreateInfo::create(const Anvil::BaseDevice* in_device_ptr,
                                                                           const uint32_t&          in_width,
                                                                           const uint32_t&          in_height,
                                                                           const uint32_t&          in_n_layers)
{
    Anvil::FramebufferCreateInfoUniquePtr result_ptr(nullptr,
                                                     std::default_delete<Anvil::FramebufferCreateInfo>() );

    result_ptr.reset(
        new Anvil::FramebufferCreateInfo(in_device_ptr,
                                         in_width,
                                         in_height,
                                         in_n_layers,
                                         Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE)
    );

    return result_ptr;
}

bool Anvil::FramebufferCreateInfo::get_attachment_at_index(uint32_t    in_attachment_index,
                                                           ImageView** out_image_view_ptr_ptr) const
{
    bool result = false;

    if (m_attachments.size() <= in_attachment_index)
    {
        goto end;
    }
    else
    {
        *out_image_view_ptr_ptr = m_attachments.at(in_attachment_index).image_view_ptr;
        result                  = true;
    }

end:
    return result;
}

bool Anvil::FramebufferCreateInfo::get_attachment_id_for_image_view(ImageView*               in_image_view_ptr,
                                                                    FramebufferAttachmentID* out_attachment_id_ptr) const
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

void Anvil::FramebufferCreateInfo::get_size(uint32_t* out_framebuffer_width_ptr,
                                            uint32_t* out_framebuffer_height_ptr,
                                            uint32_t* out_framebuffer_depth_ptr) const
{
    *out_framebuffer_width_ptr  = m_width;
    *out_framebuffer_height_ptr = m_height;
    *out_framebuffer_depth_ptr  = m_n_layers;
}
