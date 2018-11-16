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

/** Defines an image view wrapper class which simplifies the following processes:
 *
 *  - Instantation of an image view, and its destruction.
 *  - Reference counting of Image and Image View instances
 **/
#ifndef WRAPPERS_IMAGE_VIEW_H
#define WRAPPERS_IMAGE_VIEW_H

#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"

namespace Anvil
{
    class ImageView : public DebugMarkerSupportProvider<ImageView>,
                      public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        static Anvil::ImageViewUniquePtr create(Anvil::ImageViewCreateInfoUniquePtr in_create_info_ptr);

        /** Destructor. Should only be called when the reference counter drops to zero.
         *
         *  Releases all internal Vulkan objects and the owned Image instance.
         **/
        virtual ~ImageView();

        /** Returns dimensions of the base mipmap, as seen from the image view.
         *
         *  @param out_opt_base_mipmap_width_ptr  If not nullptr, deref will be set to mipmap's width.
         *  @param out_opt_base_mipmap_height_ptr If not nullptr, deref will be set to mipmap's height.
         *  @param out_opt_base_mipmap_depth_ptr  If not nullptr, deref will be set to mipmap's depth.
         *
         **/
        void get_base_mipmap_size(uint32_t* out_opt_base_mipmap_width_ptr,
                                  uint32_t* out_opt_base_mipmap_height_ptr,
                                  uint32_t* out_opt_base_mipmap_depth_ptr) const;

        const Anvil::ImageViewCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        /** Returns the encapsulated raw Vulkan image view handle. */
        VkImageView get_image_view() const
        {
            return m_image_view;
        }

        /** Returns a struct that describes the range of subresources covered by this image view.
         */
        Anvil::ImageSubresourceRange get_subresource_range() const;

        /** Tells whether the subresource range described by this image view intersects
         *  with another image view's subres range.
         *
         *  NOTE: This function returns false if the underlying parent images do not match.
         *
         *  @param in_image_view_ptr Image view to use for the query. Must not be null.
         *
         *  @return True if an intersection was found, false otherwise.
         */
        bool intersects(const Anvil::ImageView* in_image_view_ptr) const;

    private:
        /* Private functions */
        ImageView           (const ImageView&);
        ImageView& operator=(const ImageView&);

        ImageView(Anvil::ImageViewCreateInfoUniquePtr in_create_info_ptr);

        bool init();

        /* Private members */
        Anvil::ImageViewCreateInfoUniquePtr m_create_info_ptr;
        VkImageView                         m_image_view;
    };
};

#endif /* WRAPPERS_IMAGE_VIEW_H */
