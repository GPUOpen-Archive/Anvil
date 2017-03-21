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

/** Defines a framebuffer wrapper class which simplifies the following processes:
 *
 *  - Framebuffer initialization and tear-down.
 *  - Life-time management
 *  - Support for adding new attachments with automatic Vulkan FB object re-creation.
 **/
#ifndef WRAPPERS_FRAMEBUFFER_H
#define WRAPPERS_FRAMEBUFFER_H

#include "misc/types.h"

namespace Anvil
{
    class Framebuffer
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
        static std::shared_ptr<Framebuffer> create(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                                                   uint32_t                         width,
                                                   uint32_t                         height,
                                                   uint32_t                         n_layers);

        /** Destructor.
         *
         *  Destroys baked Vulkan counterparts and unregisters the wrapper instance from the object tracker.
         **/
        virtual ~Framebuffer();

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
        bool add_attachment(std::shared_ptr<ImageView> image_view_ptr,
                            FramebufferAttachmentID*   out_opt_attachment_id_ptr);

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
        bool bake(std::shared_ptr<Anvil::RenderPass> render_pass_ptr);

        /* Returns an attachment at user-specified index */
        bool get_attachment_at_index(uint32_t                    attachment_index,
                                     std::shared_ptr<ImageView>* out_image_view_ptr) const;

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
        bool get_attachment_id_for_image_view(std::shared_ptr<ImageView> image_view_ptr,
                                              FramebufferAttachmentID*   out_attachment_id_ptr);

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
        const VkFramebuffer get_framebuffer(std::shared_ptr<Anvil::RenderPass> render_pass_ptr);

        /** Returns name assigned to the framebuffer instance */
        const std::string& get_name() const
        {
            return m_name;
        }

        /** Returns framebuffer size, specified at creation time */
        void get_size(uint32_t* out_framebuffer_width_ptr,
                      uint32_t* out_framebuffer_height_ptr,
                      uint32_t* out_framebuffer_depth_ptr) const;

        /** Assigns a new name to the framebuffer instance */
        void set_name(const std::string& in_name)
        {
            m_name = in_name;
        }

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
            std::shared_ptr<ImageView> image_view_ptr;

            /** Constructor. Retains the input image view instance.
             *
             *  @param in_image_view_ptr Image view instance to use for the FB attachment. Must not
             *                           be nullptr.
             **/
             FramebufferAttachment(std::shared_ptr<ImageView> in_image_view_ptr);

             /** Destructor. Releases the encapsulated image view instance. */
             ~FramebufferAttachment();

             /** Copy constructor */
             FramebufferAttachment(const FramebufferAttachment& in);

             /** Assignment operator */
             FramebufferAttachment& operator=(const FramebufferAttachment& in);

             /** Returns true if the encapsulated image view instance is the same as the one
              *  specified uner @param in_image_view_ptr argument.
              **/
             bool operator==(std::shared_ptr<const ImageView> in_image_view_ptr) const
             {
                 return (image_view_ptr == in_image_view_ptr);
             }
        } FramebufferAttachment;

        struct RenderPassComparator
        {
            bool operator()(std::shared_ptr<Anvil::RenderPass> a,
                            std::shared_ptr<Anvil::RenderPass> b) const
            {
                return a < b;
            }
        };

        typedef std::map<std::shared_ptr<Anvil::RenderPass>, BakedFramebufferData, RenderPassComparator> BakedFramebufferMap;
        typedef std::vector<FramebufferAttachment>                                                       FramebufferAttachments;

        /* Private functions */

        /* Constructor. Please see specification of create() for more details */
        Framebuffer(std::weak_ptr<Anvil::BaseDevice> device_ptr,
                    uint32_t                         width,
                    uint32_t                         height,
                    uint32_t                         n_layers);

        Framebuffer& operator=(const Framebuffer&);
        Framebuffer           (const Framebuffer&);

        static void on_renderpass_changed(void* raw_renderpass_ptr,
                                          void* raw_framebuffer_ptr);

        /* Private members */
        FramebufferAttachments           m_attachments;
        BakedFramebufferMap              m_baked_framebuffers;
        std::weak_ptr<Anvil::BaseDevice> m_device_ptr;
        uint32_t                         m_framebuffer_size[3];
        std::string                      m_name;
    };
};

#endif /* WRAPPERS_FRAMEBUFFER_H */