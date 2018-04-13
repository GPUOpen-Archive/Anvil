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

#ifndef MISC_MT_SAFETY_H
#define MISC_MT_SAFETY_H

#include <memory>
#include <mutex>

namespace Anvil
{
    class MTSafetySupportProvider
    {
    public:
        explicit MTSafetySupportProvider(const bool& in_enable)
        {
            if (in_enable)
            {
                m_mutex_ptr.reset(
                    new std::recursive_mutex()
                );
            }
        }

        virtual ~MTSafetySupportProvider()
        {
            /* Stub */
        }

        inline bool is_mt_safe() const
        {
            return (m_mutex_ptr != nullptr);
        }

        inline void lock() const
        {
            if (m_mutex_ptr != nullptr)
            {
                m_mutex_ptr->lock();
            }
        }

        inline void unlock() const
        {
            if (m_mutex_ptr != nullptr)
            {
                m_mutex_ptr->unlock();
            }
        }

    protected:
        std::recursive_mutex* get_mutex() const
        {
            return m_mutex_ptr.get();
        }

    private:
        std::unique_ptr<std::recursive_mutex> m_mutex_ptr;

        MTSafetySupportProvider           (const MTSafetySupportProvider&);
        MTSafetySupportProvider& operator=(const MTSafetySupportProvider&) const;
    };
}; /* namespace Anvil */

#endif /* MISC_MT_SAFETY_H */
