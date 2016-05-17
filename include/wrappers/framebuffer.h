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

/** Defines a framebuffer wrapper class which simplifies the following processes:
 *
 *  - Framebuffer initialization and tear-down.
 *  - Life-time management
 *  - Support for adding new attachments with automatic Vulkan FB object re-creation.
 **/
#ifndef WRAPPERS_FRAMEBUFFER_H
#define WRAPPERS_FRAMEBUFFER_H

#include "misc/ref_counter.h"
#include "misc/types.h"

namespace Anvil
{
    class Framebuffer : public RefCounterSupportProvider
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  Verifies input arguments and initializes the internal ref counter to 1.
         *
         *  @param device_ptr Device to use.
         *  @param width      Framebuffer width. Must be at least 1.
         *  @param height     Framebuffer height. Must be at least 1.
         *  @param n_layers   Number of layers the framebuffer should use. Must be at least 1.
         **/
        Framebuffer(Anvil::Device* device_ptr,
                    uint32_t       width,
                    uint32_t       height,
                    uint32_t       n_layers);

        /** Adds a new attachment to the framebuffer.
         *
         *  If this function succeeds, the maintained framebuffer objects will NOT be automatically
         *  re-created. Instead, the user should either call bake() at a suitable moment, or expect
         *  it to be called at the next get_framebuffer() call.
         *
         *  @param image_view_ptr            Image view instance to use as a framebuffer attachment.
         *                                   Must not be nullptr. This object will be retained.
         *  @param out_opt_attachment_id_ptr If not nullptr, deref will be set to a unique ID, under which
         *                                   the attachment can be accessed.
         *
         *  @return true if the function was successful, false otherwise.
         **/
        bool add_attachment(ImageView*               image_view_ptr,
                            FramebufferAttachmentID* out_opt_attachment_id_ptr);

        /** Re-creates a Vulkan framebuffer object for the specified render pass instance. If a FB
         *  has already been created in the past, the instance will be released.
         *
         *  At least one attachment must be defined for this function to succeed.
         *
         *  @param render_pass_ptr Render pass instance to create the new Vulkan framebuffer object
         *                         for. Must not be nullptr.
         *
         *  @return true if the function was successful, false otherwise.
         * */
        bool bake(Anvil::RenderPass* render_pass_ptr);

        /** Checks if an attachment has already been created for the specified image view and, if so,
         *  returns the attachment's ID.
         *
         *  @param image_view_ptr        Image view to use for the query. Must not be nullptr.
         *  @param out_attachment_id_ptr If the function executes successfully, deref will be set to
         *                               the found attachment's ID, in which case the pointer must not
         *                               be nullptr. Otherwise, ignored.
         *
         *  @return true if successful (eg. the attachment corresponding to the specified image view was
         *          found), false otherwise.
         **/
        bool get_attachment_id_for_image_view(ImageView*               image_view_ptr,
                                              FramebufferAttachmentID* out_attachment_id_ptr);

        /** Returns a Vulkan framebuffer object instance for the specified render pass instance.
         *
         *  If the object needs to be baked (because an attachment has been added since the last
         *  baking time, or if the object is being requested for the first time), a bake() call
         *  will be made automatically.
         *
         *  @param render_pass Render pass to return the framebuffer for.
         *
         *  @return Raw Vulkan framebuffer handle or VK_NULL_HANDLE, if the function failed.
         **/
        const VkFramebuffer get_framebuffer(Anvil::RenderPass* render_pass_ptr);


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
        
        typedef struct FramebufferAttachment
        {
            ImageView* image_view_ptr;

            /** Constructor. Retains the input image view instance.
             *
             *  @param in_image_view_ptr Image view instance to use for the FB attachment. Must not
             *                           be nullptr. Will be retained.
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

        struct RenderPassComparator
        {
            bool operator()(const Anvil::RenderPass* a,
                            const Anvil::RenderPass* b) const
            {
                return a < b;
            }
        };

        typedef std::map<Anvil::RenderPass*, BakedFramebufferData, RenderPassComparator> BakedFramebufferMap;
        typedef std::vector<FramebufferAttachment>                                       FramebufferAttachments;

        /* Private functions */
        Framebuffer& operator=(const Framebuffer&);
        Framebuffer           (const Framebuffer&);

        virtual ~Framebuffer();

        static void on_renderpass_changed(void* raw_renderpass_ptr,
                                          void* raw_framebuffer_ptr);

        /* Private members */
        FramebufferAttachments m_attachments;
        BakedFramebufferMap    m_baked_framebuffers;
        Anvil::Device*         m_device_ptr;
        uint32_t               m_framebuffer_size[3];
    };

    /* Delete functor. Useful if you need to wrap a Framebuffer instance inside an auto pointer */
    struct FramebufferDeleter
    {
        void operator()(Framebuffer* framebuffer_ptr)
        {
            framebuffer_ptr->release();
        }
    };
};

#endif /* WRAPPERS_FRAMEBUFFER_H */