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

/** Implements dummy window wrappers.
 *
 *  Useful for off-screen rendering purposes, with optional support for PNG snapshot dumping.
 */
#ifndef DUMMY_WINDOW_H
#define DUMMY_WINDOW_H

#include "misc/window.h"

namespace Anvil
{
    class DummyWindow : public Window
    {
    public:
        /* Public functions */
        static std::shared_ptr<Anvil::Window> create(const std::string&     in_title,
                                                     unsigned int           in_width,
                                                     unsigned int           in_height,
                                                     PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                                                     void*                  in_present_callback_func_user_arg);

        virtual ~DummyWindow()
        {
            /* Stub */
        }

        virtual void close();

        /* Returns window's platform */
        WindowPlatform get_platform() const
        {
            return WINDOW_PLATFORM_DUMMY;
        }

        virtual void run();

        /* Tells if it's a dummy window (offscreen rendering thus no WSI/swapchain involved) */
        virtual bool is_dummy()
        {
            return true;
        }

        /** Returns system XCB connection, should be used by linux only */
        virtual void* get_connection() const
        {
            return nullptr;
        }

    protected:
        DummyWindow(const std::string&     in_title,
                    unsigned int           in_width,
                    unsigned int           in_height,
                    PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                    void*                  in_present_callback_func_user_arg);

        bool init();
    };

    class DummyWindowWithPNGSnapshots : public DummyWindow
    {
    public:
        /* Public methods */
        static std::shared_ptr<Anvil::Window> create(const std::string&     in_title,
                                                     unsigned int           in_width,
                                                     unsigned int           in_height,
                                                     PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                                                     void*                  in_present_callback_func_user_arg);

        /** Destructor */
        virtual ~DummyWindowWithPNGSnapshots()
        {
            /* Stub */
        }

        /* Returns window's platform */
        WindowPlatform get_platform() const
        {
            return WINDOW_PLATFORM_DUMMY_WITH_PNG_SNAPSHOTS;
        }

        void run();

        /** Assigns a swapchain to the window.
         *
         *  Must only be called once throughout window's lifetime.
         *
         *  @param in_swapchain_ptr Swapchain to assign. Must not be NULL.
         */
        void set_swapchain(std::weak_ptr<Anvil::Swapchain> in_swapchain_ptr);

    private:
        /* Private functions */
        DummyWindowWithPNGSnapshots(const std::string&     in_title,
                                    unsigned int           in_width,
                                    unsigned int           in_height,
                                    PFNPRESENTCALLBACKPROC in_present_callback_func_ptr,
                                    void*                  in_present_callback_func_user_arg);

        /** Grabs contents of the specified swapchain image and returns them in a raw R8G8B8A8_UNORM
         *  format.
         *
         *  NOTE: This solution is temporary. At some point, this function is going to be exposed 
         *        in Image interface.
         *
         *  @param in_swapchain_image_ptr Swapchain image, whose contents should be extracted.
         *
         *  @return As per summary.
         */
        std::shared_ptr<unsigned char> get_swapchain_image_raw_r8g8b8a8_unorm_data(std::shared_ptr<Anvil::Image> in_swapchain_image_ptr);

        /** Grabs fake swapchain image contents and stores it in a PNG file. */
        void store_swapchain_frame();

        /* Private members */
        uint32_t    m_height;
        uint32_t    m_n_frames_presented;
        std::string m_title;
        uint32_t    m_width;

        std::weak_ptr<Anvil::Swapchain> m_swapchain_ptr;
    };
}; /* namespace Anvil */

#endif /* DUMMY_WINDOW_H */