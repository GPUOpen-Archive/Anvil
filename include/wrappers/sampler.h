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

/** Implements a wrapper for a single Vulkan sampler. Implemented in order to:
 *
 *  - simplify debugging, life-time management and usage.
 *  - let ObjectTracker detect leaking event instances.
 *
 *  The wrapper is NOT thread-safe.
 **/
#ifndef WRAPPERS_SAMPLER_H
#define WRAPPERS_SAMPLER_H

#include "misc/debug_marker.h"
#include "misc/mt_safety.h"
#include "misc/types.h"


namespace Anvil
{
    /** Wrapper class for Vulkan samplers */
    class Sampler : public DebugMarkerSupportProvider<Sampler>,
                    public MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  Creates a single Sampler instance and registers the object in Object Tracker.
         *
         *  For argument discussion, please consult Vulkan API specification.
         */
        static Anvil::SamplerUniquePtr create(Anvil::SamplerCreateInfoUniquePtr in_create_info_ptr);

        /** Destructor.
         *
         *  Releases the underlying Vulkan Sampler instance and signs the wrapper object out from
         *  the Object Tracker.
         **/
        virtual ~Sampler();

        const SamplerCreateInfo* get_create_info_ptr() const
        {
            return m_create_info_ptr.get();
        }

        /** Retrieves a raw Vulkan handle for the underlying VkSampler instance. */
        VkSampler get_sampler() const
        {
            return m_sampler;
        }

        /** Retrieves a pointer to the raw Vulkan handle for the underlying VkSampler instance. */
        const VkSampler* get_sampler_ptr() const
        {
            return &m_sampler;
        }

    private:
        /* Private functions */

        bool init();

        /* Please see create() for specification */
        Sampler(Anvil::SamplerCreateInfoUniquePtr in_create_info_ptr);

        Sampler           (const Sampler&);
        Sampler& operator=(const Sampler&);

        /* Private variables */
        SamplerCreateInfoUniquePtr m_create_info_ptr;
        VkSampler                  m_sampler;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_SAMPLER_H */