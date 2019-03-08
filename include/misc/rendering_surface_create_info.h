//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef MISC_RENDERING_SURFACE_CREATE_INFO_H
#define MISC_RENDERING_SURFACE_CREATE_INFO_H

#include "misc/types.h"

namespace Anvil
{
    class RenderingSurfaceCreateInfo
    {
    public:
        /* Creates a new rendering surface create info instance. */
        static RenderingSurfaceCreateInfoUniquePtr create(Anvil::Instance*         in_instance_ptr,
                                                          const Anvil::BaseDevice* in_device_ptr,
                                                          const Anvil::Window*     in_window_ptr,
                                                          MTSafety                 in_mt_safety = Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE);

        ~RenderingSurfaceCreateInfo();

        const Anvil::BaseDevice* get_device_ptr() const
        {
            return m_device_ptr;
        }

        const MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }

        Anvil::Instance* get_instance_ptr() const
        {
            return m_instance_ptr;
        }

        const Anvil::Window* get_window_ptr() const
        {
            return m_window_ptr;
        }

        void set_device_ptr(const Anvil::BaseDevice* in_device_ptr)
        {
            m_device_ptr = in_device_ptr;
        }

        void set_mt_safety(const MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        void set_instance_ptr(Anvil::Instance* in_instance_ptr)
        {
            m_instance_ptr = in_instance_ptr;
        }

        void set_window_ptr(const Anvil::Window* in_window_ptr)
        {
            m_window_ptr = in_window_ptr;
        }

    private:
        /* Private type definitions */

        /* Private functions */

        RenderingSurfaceCreateInfo(Anvil::Instance*         in_instance_ptr,
                                   const Anvil::BaseDevice* in_device_ptr,
                                   const Anvil::Window*     in_window_ptr,
                                   MTSafety                 in_mt_safety);

        /* Private variables */
        const Anvil::BaseDevice* m_device_ptr;
        Anvil::Instance*         m_instance_ptr;
        const Anvil::Window*     m_window_ptr;

        MTSafety m_mt_safety;
    };
};

#endif /* MISC_RENDERING_SURFACE_CREATE_INFO_H */
