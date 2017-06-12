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

/* Various routines for FP16 <-> FP32 conversions.
 *
 * Implementation heavily based on the following Public Domain work:
 *
 * - https://gist.github.com/rygorous/2144712
 * - https://gist.github.com/rygorous/2156668 
 *
 */
#ifndef WRAPPERS_FP16_H
#define WRAPPERS_FP16_H

#include "types.h"

namespace Anvil
{
    typedef struct float16_t
    {
        union
        {
            unsigned short u;
            struct
            {
                uint16_t Mantissa : 10;
                uint16_t Exponent : 5;
                uint16_t Sign : 1;
            } fields;
        };

        float16_t()
        {
            u = 0;
        }

        static const Anvil::float16_t infinity_negative()
        {
            Anvil::float16_t ref;

            ref.fields.Exponent = (1 << 5) - 1;
            ref.fields.Mantissa = 0;
            ref.fields.Sign     = 1;

            return ref;
        }

        static const Anvil::float16_t infinity_positive()
        {
            Anvil::float16_t ref;

            ref.fields.Exponent = (1 << 5) - 1;
            ref.fields.Mantissa = 0;
            ref.fields.Sign     = 0;

            return ref;
        }

        static const Anvil::float16_t max_value()
        {
            Anvil::float16_t ref;

            ref.fields.Exponent = 30;
            ref.fields.Mantissa = (1 << 10) - 1;
            ref.fields.Sign     = 0;

            return ref;
        }

        static const Anvil::float16_t min_value()
        {
            Anvil::float16_t ref;

            ref.fields.Exponent = 30;
            ref.fields.Mantissa = (1 << 10) - 1;
            ref.fields.Sign     = 1;

            return ref;
        }
    } float16_t;

    typedef struct float32_t
    {
        union
        {
            uint32_t u;
            float    f;

            struct
            {
                uint32_t Mantissa : 23;
                uint32_t Exponent : 8;
                uint32_t Sign : 1;
            } fields;
        };

        float32_t()
        {
            u = 0;
        }

        float32_t(const float& in_f)
        {
            f = in_f;
        }

        const float& operator()() const
        {
            return f;
        }
    } float32_t;

    namespace Utils
    {
        float32_t fp16_to_fp32_fast      (float16_t in_h);
        float32_t fp16_to_fp32_fast2     (float16_t in_h);
        float32_t fp16_to_fp32_fast3     (float16_t in_h);
        float32_t fp16_to_fp32_fast4     (float16_t in_h);
        float32_t fp16_to_fp32_fast5     (float16_t in_h);
        float32_t fp16_to_fp32_full      (float16_t in_h);
        float16_t fp32_to_fp16_approx    (float32_t in_f);
        float16_t fp32_to_fp16_fast      (float32_t in_f);
        float16_t fp32_to_fp16_fast2     (float32_t in_f);
        float16_t fp32_to_fp16_fast3     (float32_t in_f);
        float16_t fp32_to_fp16_fast3_rtne(float32_t in_f);
        float16_t fp32_to_fp16_foxtk     (float32_t in_f);
        float16_t fp32_to_fp16_full      (float32_t in_f);
        float16_t fp32_to_fp16_full_rtne (float32_t in_f);
    };
};

#endif /* WRAPPERS_FP16_H */