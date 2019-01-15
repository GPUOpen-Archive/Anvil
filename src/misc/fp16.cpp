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

#include "misc/fp16.h"

// Conversion tables
static const struct PrecalcedData
{
    uint32_t      basetable[512];
    unsigned char shifttable[512];
    uint32_t      tab1[256];
    uint32_t      tab2[256];
    uint32_t      tab3[256];

    PrecalcedData()
    {
        /* FP16->FP32 tables */
        Anvil::float16_t f16;
        Anvil::float32_t f32;

        for (int i = 0; i < 256; i++)
        {
            f16.u   = static_cast<unsigned short>(i << 8);
            f32     = Anvil::Utils::fp16_to_fp32_full(f16);
            tab1[i] = f32.u;
            tab2[i] = 1 << 13;

            f16.u   = static_cast<unsigned short>(i);
            f32     = Anvil::Utils::fp16_to_fp32_full(f16);
            tab3[i] = f32.u;
        }

        // Lower exponent end has some denormals
        tab2[0x03] = 1 << 14;
        tab2[0x02] = 1 << 14;
        tab2[0x01] = 1 << 15;
        tab2[0x83] = 1 << 14;
        tab2[0x82] = 1 << 14;
        tab2[0x81] = 1 << 15;

        /* FP32->FP16 tables */
        int i;
        int e;

        for (i = 0; i < 256; ++i)
        {
            e = i - 127;

            if (e < -24)
            {
                // Very small numbers map to zero
                basetable [i | 0x000] = 0x0000;
                basetable [i | 0x100] = 0x8000;
                shifttable[i | 0x000] = 24;
                shifttable[i | 0x100] = 24;
            }
            else
            if (e < -14)
            {
                // Small numbers map to denorms
                basetable [i | 0x000] = (0x0400u >> (-e - 14));
                basetable [i | 0x100] = (0x0400u >> (-e - 14)) | 0x8000u;
                shifttable[i | 0x000] = static_cast<unsigned char>(-e - 1);
                shifttable[i | 0x100] = static_cast<unsigned char>(-e - 1);
            }
            else
            if (e <= 15)
            {
                // Normal numbers just lose precision
                basetable [i | 0x000] = ((e + 15u) << 10u);
                basetable [i | 0x100] = ((e + 15u) << 10u) | 0x8000u;
                shifttable[i | 0x000] = 13u;
                shifttable[i | 0x100] = 13u;
            }
            else
            if (e < 128)
            {
                // Large numbers map to Infinity
                basetable [i | 0x000] = 0x7C00;
                basetable [i | 0x100] = 0xFC00;
                shifttable[i | 0x000] = 24;
                shifttable[i | 0x100] = 24;
            }
            else
            {
                // Infinity and NaN's stay Infinity and NaN's
                basetable [ i |0x000] = 0x7C00;
                basetable [ i |0x100] = 0xFC00;
                shifttable[ i |0x000] = 13;
                shifttable[ i |0x100] = 13;
            }
        }
    }
} precalc;


Anvil::float32_t Anvil::Utils::fp16_to_fp32_full(Anvil::float16_t in_h)
{
    Anvil::float32_t o = { 0 };

    // From ISPC ref code
    if (in_h.fields.Exponent == 0 &&
        in_h.fields.Mantissa == 0) // (Signed) zero
    {
        o.fields.Sign = in_h.fields.Sign;
    }
    else
    {
        if (in_h.fields.Exponent == 0) // Denormal (will convert to normalized)
        {
            // Adjust mantissa so it's normalized (and keep track of exp adjust)
            int      e = -1;
            uint32_t m = in_h.fields.Mantissa;

            do
            {
                e++;
                m <<= 1;
            }
            while ((m & 0x400) == 0);

            o.fields.Mantissa = (m & 0x3ff) << 13;
            o.fields.Exponent = static_cast<uint32_t>(127 - 15 - e);
            o.fields.Sign     = in_h.fields.Sign;
        }
        else
        if (in_h.fields.Exponent == 0x1f) // Inf/NaN
        {
            // NOTE: It's safe to treat both with the same code path by just truncating
            // lower Mantissa bits in NaNs (this is valid).
            o.fields.Mantissa = static_cast<uint32_t>(in_h.fields.Mantissa << 13);
            o.fields.Exponent = 255u;
            o.fields.Sign     = in_h.fields.Sign;
        }
        else // Normalized number
        {
            o.fields.Mantissa = static_cast<uint32_t>(in_h.fields.Mantissa << 13);
            o.fields.Exponent = static_cast<uint32_t>(127 - 15 + in_h.fields.Exponent);
            o.fields.Sign     = in_h.fields.Sign;
        }
    }

    return o;
}

Anvil::float32_t Anvil::Utils::fp16_to_fp32_fast(Anvil::float16_t in_h)
{
    Anvil::float32_t o;

    if (in_h.u & 0x7f00)
    {
        o.u = precalc.tab1[in_h.u >> 8] + precalc.tab2[in_h.u >> 8] * (in_h.u & 0xff);
    }
    else
    {
        o.u = ((in_h.u & 0x8000) << 16) | precalc.tab3[in_h.u & 0xff];
    }

    return o;
}

Anvil::float32_t Anvil::Utils::fp16_to_fp32_fast2(Anvil::float16_t in_h)
{
    const Anvil::float32_t magic = { 126 << 23 };
    Anvil::float32_t       o;

    if (in_h.fields.Exponent == 0) // Zero / Denormal
    {
        o.u  = magic.u + in_h.fields.Mantissa;
        o.f -= magic.f;
    }
    else
    {
        o.fields.Mantissa = static_cast<uint32_t>(in_h.fields.Mantissa << 13);

        if (in_h.fields.Exponent == 0x1f) // Inf/NaN
        {
            o.fields.Exponent = 255;
        }
        else
        {
            o.fields.Exponent = static_cast<uint32_t>(127 - 15 + in_h.fields.Exponent);
        }
    }

    o.fields.Sign = in_h.fields.Sign;

    return o;
}

Anvil::float32_t Anvil::Utils::fp16_to_fp32_fast3(Anvil::float16_t in_h)
{
    const Anvil::float32_t magic       = { 113 << 23 };
    const uint32_t         shifted_exp = 0x7c00 << 13; // exponent mask after shift
    Anvil::float32_t       o;

    // mantissa+exponent
    uint32_t shifted  = (in_h.u & 0x7fffu) << 13u;
    uint32_t exponent = shifted & shifted_exp;

    // exponent cases
    o.u = shifted;

    if (exponent == 0) // Zero / Denormal
    {
        o.u += magic.u;
        o.f -= magic.f;
    }
    else
    if (exponent == shifted_exp) // Inf/NaN
    {
        o.u += (255 - 31) << 23;
    }
    else
    {
        o.u += (127 - 15) << 23;
    }

    o.u |= (in_h.u & 0x8000) << 16; // copy sign bit

    return o;
}

Anvil::float32_t Anvil::Utils::fp16_to_fp32_fast4(Anvil::float16_t in_h)
{
    const Anvil::float32_t magic       = { 113 << 23 };
    const uint32_t         shifted_exp = 0x7c00 << 13; // exponent mask after shift
    Anvil::float32_t       o;

    o.u = (in_h.u & 0x7fffu) << 13u;     // exponent/mantissa bits

    uint32_t exp = shifted_exp & o.u; // just the exponent

    o.u += static_cast<uint32_t>((127 - 15) << 23); // exponent adjust

    // handle exponent special cases
    if (exp == shifted_exp) // Inf/NaN?
    {
        o.u += (128 - 16) << 23;    // extra exp adjust
    }
    else
    if (exp == 0) // Zero/Denormal?
    {
        o.u += 1 << 23;             // extra exp adjust
        o.f -= magic.f;             // renormalize
    }

    o.u |= (in_h.u & 0x8000) << 16;    // sign bit

    return o;
}

Anvil::float32_t Anvil::Utils::fp16_to_fp32_fast5(Anvil::float16_t in_h)
{
    const Anvil::float32_t magic      = { (254 - 15) << 23 };
    const Anvil::float32_t was_infnan = { (127 + 16) << 23 };
    Anvil::float32_t       o;

    o.u = (in_h.u & 0x7fffu) << 13u;// exponent/mantissa bits
    o.f *= magic.f;                 // exponent adjust

    if (o.f >= was_infnan.f) // make sure Inf/NaN survive
    {
        o.u |= 255 << 23;
    }

    o.u |= (in_h.u & 0x8000) << 16;    // sign bit

    return o;
}

// Original ISPC reference version; this always rounds ties up.
Anvil::float16_t Anvil::Utils::fp32_to_fp16_full(Anvil::float32_t in_f)
{
    Anvil::float16_t o;

    // Based on ISPC reference code (with minor modifications)
    if (in_f.fields.Exponent == 0) // Signed zero/denormal (which will underflow)
    {
        o.fields.Exponent = 0;
    }
    else
    if (in_f.fields.Exponent == 255) // Inf or NaN (all exponent bits set)
    {
        o.fields.Exponent = 31;
        o.fields.Mantissa = in_f.fields.Mantissa ? 0x200u : 0u; // NaN->qNaN and Inf->Inf
    }
    else // Normalized number
    {
        // Exponent unbias the single, then bias the halfp
        int newexp = static_cast<int>(in_f.fields.Exponent - 127 + 15);

        if (newexp >= 31) // Overflow, return signed infinity
        {
            o.fields.Exponent = 31;
        }
        else
        if (newexp <= 0) // Underflow
        {
            if ((14 - newexp) <= 24) // Mantissa might be non-zero
            {
                uint32_t mant = in_f.fields.Mantissa | 0x800000; // Hidden 1 bit

                o.fields.Mantissa = mant >> (14 - newexp);

                if ((mant >> (13 - newexp)) & 1) // Check for rounding
                {
                    o.u++; // Round, might overflow into exp bit, but this is OK
                }
            }
        }
        else
        {
            o.fields.Exponent = static_cast<uint16_t>(newexp);
            o.fields.Mantissa = in_f.fields.Mantissa >> 13;

            if (in_f.fields.Mantissa & 0x1000) // Check for rounding
            {
                o.u++; // Round, might overflow to inf, this is OK
            }
        }
    }

    o.fields.Sign = in_f.fields.Sign;

    return o;
}

// Same as above, but with full round-to-nearest-even.
Anvil::float16_t Anvil::Utils::fp32_to_fp16_full_rtne(Anvil::float32_t in_f)
{
    Anvil::float16_t o;

    // Based on ISPC reference code (with minor modifications)
    if (in_f.fields.Exponent == 0) // Signed zero/denormal (which will underflow)
    {
        o.fields.Exponent = 0;
    }
    else
    if (in_f.fields.Exponent == 255) // Inf or NaN (all exponent bits set)
    {
        o.fields.Exponent = 31;
        o.fields.Mantissa = in_f.fields.Mantissa ? 0x200u : 0u; // NaN->qNaN and Inf->Inf
    }
    else // Normalized number
    {
        // Exponent unbias the single, then bias the halfp
        int newexp = static_cast<int>(in_f.fields.Exponent - 127 + 15);

        if (newexp >= 31) // Overflow, return signed infinity
        {
            o.fields.Exponent = 31;
        }
        else if (newexp <= 0) // Underflow
        {
            if ((14 - newexp) <= 24) // Mantissa might be non-zero
            {
                uint32_t mant  = in_f.fields.Mantissa | 0x800000; // Hidden 1 bit
                uint32_t shift = static_cast<uint32_t>(14 - newexp);

                o.fields.Mantissa = mant >> shift;

                uint32_t lowmant = mant & ((1 << shift) - 1);
                uint32_t halfway = static_cast<uint32_t>(1 << (shift - 1));

                if (lowmant >= halfway) // Check for rounding
                {
                    if (lowmant > halfway || (o.fields.Mantissa & 1)) // if above halfway point or unrounded result is odd
                    {
                        o.u++; // Round, might overflow into exp bit, but this is OK
                    }
                }
            }
        }
        else
        {
            o.fields.Exponent = static_cast<uint16_t>(newexp);
            o.fields.Mantissa = in_f.fields.Mantissa >> 13;

            if (in_f.fields.Mantissa & 0x1000) // Check for rounding
            {
                if (((in_f.fields.Mantissa & 0x1fff) > 0x1000) || (o.fields.Mantissa & 1)) // if above halfway point or unrounded result is odd
                {
                    o.u++; // Round, might overflow to inf, this is OK
                }
            }
        }
    }

    o.fields.Sign = in_f.fields.Sign;

    return o;
}

Anvil::float16_t Anvil::Utils::fp32_to_fp16_fast(Anvil::float32_t in_f)
{
    Anvil::float16_t o;

    // Based on ISPC reference code (with minor modifications)
    if (in_f.fields.Exponent == 255) // Inf or NaN (all exponent bits set)
    {
        o.fields.Exponent = 31;
        o.fields.Mantissa = in_f.fields.Mantissa ? 0x200u : 0u; // NaN->qNaN and Inf->Inf
    }
    else // Normalized number
    {
        // Exponent unbias the single, then bias the halfp
        int newexp = static_cast<int>(in_f.fields.Exponent - 127 + 15);

        if (newexp >= 31) // Overflow, return signed infinity
        {
            o.fields.Exponent = 31;
        }

        else if (newexp <= 0) // Underflow
        {
            if ((14 - newexp) <= 24) // Mantissa might be non-zero
            {
                uint32_t mant = in_f.fields.Mantissa | 0x800000; // Hidden 1 bit

                o.fields.Mantissa = mant >> (14 - newexp);

                if ((mant >> (13 - newexp)) & 1) // Check for rounding
                {
                    o.u++; // Round, might overflow into exp bit, but this is OK
                }
            }
        }
        else
        {
            o.fields.Exponent = static_cast<uint16_t>(newexp);
            o.fields.Mantissa = in_f.fields.Mantissa >> 13;

            if (in_f.fields.Mantissa & 0x1000) // Check for rounding
            {
                o.u++; // Round, might overflow to inf, this is OK
            }
        }
    }

    o.fields.Sign = in_f.fields.Sign;

    return o;
}

Anvil::float16_t Anvil::Utils::fp32_to_fp16_fast2(Anvil::float32_t in_f)
{
    Anvil::float32_t infty = { 31 << 23 };
    Anvil::float32_t magic = { 15 << 23 };
    Anvil::float16_t o;

    uint32_t sign = in_f.fields.Sign;

    in_f.fields.Sign = 0;

    // Based on ISPC reference code (with minor modifications)
    if (in_f.fields.Exponent == 255) // Inf or NaN (all exponent bits set)
    {
        o.fields.Exponent = 31;
        o.fields.Mantissa = in_f.fields.Mantissa ? 0x200u : 0u; // NaN->qNaN and Inf->Inf
    }
    else // (De)normalized number or zero
    {
        in_f.u &= ~0xfff; // Make sure we don't get sticky bits

        // Not necessarily the best move in terms of accuracy, but matches behavior
        // of other versions.

        // Shift exponent down, denormalize if necessary.
        // NOTE This represents half-float denormals using single precision denormals.
        // The main reason to do this is that there's no shift with per-lane variable
        // shifts in SSE*, which we'd otherwise need. It has some funky side effects
        // though:
        // - This conversion will actually respect the FTZ (Flush To Zero) flag in
        //   MXCSR - if it's set, no half-float denormals will be generated. I'm
        //   honestly not sure whether this is good or bad. It's definitely interesting.
        // - If the underlying HW doesn't support denormals (not an issue with Intel
        //   CPUs, but might be a problem on GPUs or PS3 SPUs), you will always get
        //   flush-to-zero behavior. This is bad, unless you're on a CPU where you don't
        //   care.
        // - Denormals tend to be slow. FP32 denormals are rare in practice outside of things
        //   like recursive filters in DSP - not a typical half-float application. Whether
        //   FP16 denormals are rare in practice, I don't know. Whatever slow path your HW
        //   may or may not have for denormals, this may well hit it.
        in_f.f *= magic.f;

        in_f.u += 0x1000; // Rounding bias

        if (in_f.u > infty.u)
        {
            in_f.u = infty.u; // Clamp to signed infinity if overflowed
        }

        o.u = static_cast<unsigned short>(in_f.u >> 13); // Take the bits!
    }

    o.fields.Sign = sign;
    return o;
}

Anvil::float16_t Anvil::Utils::fp32_to_fp16_fast3(Anvil::float32_t in_f)
{
    Anvil::float32_t f32infty   = { 255 << 23 };
    Anvil::float32_t f16infty   = { 31 << 23 };
    Anvil::float32_t magic      = { 15 << 23 };
    uint32_t         sign_mask  = 0x80000000u;
    uint32_t         round_mask = ~0xfffu;
    Anvil::float16_t o;

    uint32_t sign = in_f.u & sign_mask;
    in_f.u ^= sign;

    // NOTE all the integer compares in this function can be safely
    // compiled into signed compares since all operands are below
    // 0x80000000. Important if you want fast straight SSE2 code
    // (since there's no unsigned PCMPGTD).

    if (in_f.u >= f32infty.u) // Inf or NaN (all exponent bits set)
    {
        o.u = (in_f.u > f32infty.u) ? 0x7e00u : 0x7c00u; // NaN->qNaN and Inf->Inf
    }
    else // (De)normalized number or zero
    {
        in_f.u &= round_mask;
        in_f.f *= magic.f;
        in_f.u -= round_mask;

        if (in_f.u > f16infty.u)
        {
            in_f.u = f16infty.u; // Clamp to signed infinity if overflowed
        }

        o.u = static_cast<unsigned short>(in_f.u >> 13); // Take the bits!
    }

    o.u |= sign >> 16;

    return o;
}

// Same, but rounding ties to nearest even instead of towards +inf
Anvil::float16_t Anvil::Utils::fp32_to_fp16_fast3_rtne(Anvil::float32_t in_f)
{
    Anvil::float32_t f32infty     = { 255 << 23 };
    Anvil::float32_t f16max       = { (127 + 16) << 23 };
    Anvil::float32_t denorm_magic = { ((127 - 15) + (23 - 10) + 1) << 23 };
    uint32_t         sign_mask    = 0x80000000u;
    Anvil::float16_t o;

    uint32_t sign = in_f.u & sign_mask;

    in_f.u ^= sign;

    // NOTE all the integer compares in this function can be safely
    // compiled into signed compares since all operands are below
    // 0x80000000. Important if you want fast straight SSE2 code
    // (since there's no unsigned PCMPGTD).

    if (in_f.u >= f16max.u) // result is Inf or NaN (all exponent bits set)
    {
        o.u = (in_f.u > f32infty.u) ? 0x7e00u : 0x7c00u; // NaN->qNaN and Inf->Inf
    }
    else // (De)normalized number or zero
    {
        if (in_f.u < (113 << 23)) // resulting FP16 is subnormal or zero
        {
            // use a magic value to align our 10 mantissa bits at the bottom of
            // the float. as long as FP addition is round-to-nearest-even this
            // just works.
            in_f.f += denorm_magic.f;

            // and one integer subtract of the bias later, we have our final float!
            o.u = static_cast<unsigned short>(in_f.u - denorm_magic.u);
        }
        else
        {
            uint32_t mant_odd = (in_f.u >> 13) & 1; // resulting mantissa is odd

            // update exponent, rounding bias part 1
            in_f.u += 0xc8000fff; // 0xc8000fff == static_cast<uint32_t>(((15 - 127) << 23) + 0xfff);
            // rounding bias part 2
            in_f.u += mant_odd;
            // take the bits!
            o.u = static_cast<unsigned short>(in_f.u >> 13);
        }
    }

    o.u |= sign >> 16;

    return o;
}

Anvil::float16_t Anvil::Utils::fp32_to_fp16_foxtk(Anvil::float32_t in_f)
{
    int32_t          f_i32 = *reinterpret_cast<int32_t*>(&in_f);
    Anvil::float16_t o;

    o.u = static_cast<unsigned short>(precalc.basetable[ (f_i32>>23) & 0x1ff]
                                   + ((f_i32 & 0x007fffff) >> precalc.shifttable[(f_i32 >> 23) & 0x1ff]) );

    return o;
}

// Approximate solution. This is faster but converts some sNaNs to
// infinity and doesn't round correctly. Handle with care.
Anvil::float16_t Anvil::Utils::fp32_to_fp16_approx(Anvil::float32_t in_f)
{
    Anvil::float32_t f32infty  = { 255 << 23 };
    Anvil::float32_t f16max    = { (127 + 16) << 23 };
    Anvil::float32_t magic     = { 15 << 23 };
    Anvil::float32_t expinf    = { (255 ^ 31) << 23 };
    uint32_t         sign_mask = 0x80000000u;
    Anvil::float16_t o;

    uint32_t sign = in_f.u & sign_mask;

    in_f.u ^= sign;

    if (!(in_f.f < f32infty.u)) // Inf or NaN
    {
        o.u = static_cast<unsigned short>(in_f.u ^ expinf.u);
    }
    else
    {
        if (in_f.f > f16max.f)
        {
            in_f.f = f16max.f;
        }

        in_f.f *= magic.f;
    }

    o.u = static_cast<unsigned short>(in_f.u >> 13); // Take the mantissa bits
    o.u |= sign >> 16;

    return o;
}
