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
#ifndef MISC_FRAMEBUFFER_CREATE_INFO_H
#define MISC_FRAMEBUFFER_CREATE_INFO_H

#include "misc/types.h"


namespace Anvil
{
    class FramebufferCreateInfo
    {
    public:
        /* Public functions */

        /* TODO.
         *
         * NOTE: Unless specified later with a corresponding set_..() invocation, the following parameters are assumed by default:
         *
         * - MT safety: Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE
         */
        static Anvil::FramebufferCreateInfoUniquePtr create(const Anvil::BaseDevice* in_device_ptr,
                                                            const uint32_t&          in_width,
                                                            const uint32_t&          in_height,
                                                            const uint32_t&          in_n_layers);

        bool add_attachment(ImageView*               in_image_view_ptr,
                            FramebufferAttachmentID*  out_opt_attachment_id_ptr);

        /* Returns an attachment at user-specified index */
        bool get_attachment_at_index(uint32_t    in_attachment_index,
                                     ImageView** out_image_view_ptr_ptr) const;

        /** Checks if an attachment has already been created for the specified image view and, if so,
         *  returns the attachment's ID.
         *
         *  @param in_image_view_ptr     Image view to use for the query. Must not be nullptr.
         *  @param out_attachment_id_ptr If the function executes successfully, deref will be set to
         *                               the found attachment's ID, in which case the pointer must not
         *                               be nullptr. Otherwise, ignored.
         *
         *  @return true if successful (eg. the attachment corresponding to the specified image view was
         *          found), false otherwise.
         **/
        bool get_attachment_id_for_image_view(ImageView*               in_image_view_ptr,
                                              FramebufferAttachmentID* out_attachment_id_ptr) const;

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        uint32_t get_height() const
        {
            return m_height;
        }

        Anvil::MTSafety get_mt_safety() const
        {
            return m_mt_safety;
        }

        /** Returns the number of attachments defined for the framebuffer. */
        uint32_t get_n_attachments() const
        {
            return static_cast<uint32_t>(m_attachments.size() );
        }

        uint32_t get_n_layers() const
        {
            return m_n_layers;
        }

        /** Returns framebuffer size, specified at creation time */
        void get_size(uint32_t* out_framebuffer_width_ptr,
                      uint32_t* out_framebuffer_height_ptr,
                      uint32_t* out_framebuffer_depth_ptr) const;

        uint32_t get_width() const
        {
            return m_width;
        }

        void set_device(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        void set_height(const uint32_t& in_height)
        {
            m_height = in_height;
        }

        void set_mt_safety(const Anvil::MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        void set_n_layers(const uint32_t& in_n_layers)
        {
            m_n_layers = in_n_layers;
        }

        void set_width(const uint32_t& in_width)
        {
            m_width = in_width;
        }

    private:
        /* Private type definitions */

        typedef struct FramebufferAttachment
        {
            ImageView* image_view_ptr;

            /** Constructor. Retains the input image view instance.
             *
             *  @param in_image_view_ptr Image view instance to use for the FB attachment. Must not
             *                           be nullptr.
             **/
             FramebufferAttachment(ImageView* in_image_view_ptr);

             /** Destructor. Releases the encapsulated image view instance. */
             ~FramebufferAttachment();

             /** Copy constructor */
             FramebufferAttachment(const FramebufferAttachment& in);

             /** Assignment operator */
             FramebufferAttachment& operator=(const FramebufferAttachment& in);

             /** Returns true if the encapsulated image view instance is the same as the one
              *  specified uner @param in_image_view_ptr argument.
              **/
             bool operator==(const ImageView* in_image_view_ptr) const
             {
                 return (image_view_ptr == in_image_view_ptr);
             }
        } FramebufferAttachment;

        typedef std::vector<FramebufferAttachment> FramebufferAttachments;

        /* Private functions */

        FramebufferCreateInfo(const Anvil::BaseDevice* in_device_ptr,
                              const uint32_t&          in_width,
                              const uint32_t&          in_height,
                              const uint32_t&          in_n_layers,
                              MTSafety                 in_mt_safety);

        /* Private variables */
        FramebufferAttachments m_attachments;

        const Anvil::BaseDevice* m_device_ptr;
        uint32_t                 m_height;
        MTSafety                 m_mt_safety;
        uint32_t                 m_n_layers;
        uint32_t                 m_width;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(FramebufferCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(FramebufferCreateInfo);
    };
}; /* namespace Anvil */

#endif /* MISC_FRAMEBUFFER_CREATE_INFO_H */